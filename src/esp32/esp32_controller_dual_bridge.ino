/*
 * ESP32 DUAL-MODE BLE BRIDGE v4.3.1
 * ====================================
 * - Revert ke v4 base (yang proven jalan)
 * - Tambah fault flags f1/f2 di JSON
 * - GPIO RS485: RX=25, TX=26 (dipindah dari 16/17)
 * - Log uptime disconnect
 * - Tidak pakai patch eksperimental (sleep_disable, conn_params, throttle)
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HardwareSerial.h>
#include <Preferences.h>

// ==== RS485 GPIO ====
#define RXD2 25
#define TXD2 26
#define RS485_BAUD 9600

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_RX   "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_TX   "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

enum Mode { MODE_UNKNOWN, MODE_BRIDGE, MODE_STANDALONE };

BLECharacteristic *pTxCharacteristic;
HardwareSerial RS485(2);
Preferences prefs;

bool deviceConnected = false;
Mode currentMode = MODE_UNKNOWN;
unsigned long connectTime = 0;
bool receivedDataFromApp = false;

uint8_t sessionKey = 0;
bool verified = false;
unsigned long lastPoll = 0;

// ODO/TRIP
float odoKm = 0.0;
float tripKm = 0.0;
unsigned long lastSpeedTime = 0;
unsigned long lastSaveTime = 0;

// ==== CRC16 Modbus ====
uint16_t crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }
    return crc;
}

// ==== BLE Callbacks ====
class ServerCB : public BLEServerCallbacks {
    void onConnect(BLEServer* pSrv) {
        deviceConnected = true;
        currentMode = MODE_UNKNOWN;
        connectTime = millis();
        receivedDataFromApp = false;
        verified = false;
        while (RS485.available()) RS485.read();
        Serial.println("[BLE] HP connected");
    }
    void onDisconnect(BLEServer* pSrv) {
        unsigned long uptime = (millis() - connectTime) / 1000;
        Serial.printf("[BLE] HP disconnected — uptime %lus\n", uptime);

        deviceConnected = false;
        currentMode = MODE_UNKNOWN;
        verified = false;
        prefs.putFloat("odo", odoKm);
        prefs.putFloat("trip", tripKm);
        Serial.println("[BLE] odo/trip saved");

        delay(300);
        BLEDevice::startAdvertising();
        Serial.println("[BLE] Advertising restarted");
    }
};

class RxCB : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pChar) {
        String val = pChar->getValue();
        if (val.length() > 0) {
            receivedDataFromApp = true;

            if (val.indexOf("RESET_TRIP") >= 0) {
                tripKm = 0.0;
                prefs.putFloat("trip", 0.0);
                Serial.println("[TRIP] Reset");
                return;
            }
            if (val.indexOf("RESET_ODO") >= 0) {
                odoKm = 0.0;
                prefs.putFloat("odo", 0.0);
                Serial.println("[ODO] Reset");
                return;
            }
            if (currentMode != MODE_BRIDGE) {
                currentMode = MODE_BRIDGE;
                verified = false;
                Serial.println("[MODE] BRIDGE");
            }
            RS485.write((uint8_t*)val.c_str(), val.length());
            RS485.flush();
            Serial.printf("[BLE→RS485] %d bytes\n", val.length());
        }
    }
};

// ==== Setup ====
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== SP-BLE DUAL-MODE v4.3.1 ===");

    RS485.begin(RS485_BAUD, SERIAL_8N1, RXD2, TXD2);
    Serial.printf("[RS485] RX=%d TX=%d @ %d\n", RXD2, TXD2, RS485_BAUD);

    prefs.begin("dkys", false);
    odoKm = prefs.getFloat("odo", 0.0);
    tripKm = prefs.getFloat("trip", 0.0);
    Serial.printf("[NVS] ODO=%.1f km, TRIP=%.1f km\n", odoKm, tripKm);

    BLEDevice::init("SP-BLE");
    BLEDevice::setMTU(512);

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCB());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_TX, BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxChar = pService->createCharacteristic(
        CHARACTERISTIC_RX,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    pRxChar->setCallbacks(new RxCB());

    pService->start();

    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true);
    pAdv->setMinPreferred(0x06);
    pAdv->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("[BLE] Advertising: SP-BLE");
}

// ==== Handshake ====
bool doHandshake() {
    uint8_t pkt[7] = {0x68, 0x02, 0x10, 0x20, 0x00, 0x74, 0x5C};
    while (RS485.available()) RS485.read();
    RS485.write(pkt, 7);
    RS485.flush();

    unsigned long start = millis();
    uint8_t rx[16];
    int idx = 0;
    while (millis() - start < 500 && idx < 16) {
        if (RS485.available()) { rx[idx++] = RS485.read(); start = millis(); }
    }
    if (idx < 8 || rx[3] != 0xA0) return false;
    uint8_t seed = rx[5];
    sessionKey = (seed - 0x66) & 0xFF;
    Serial.printf("[DK72240] Seed=0x%02X K=0x%02X\n", seed, sessionKey);
    return true;
}

// ==== Verify ====
bool doVerify() {
    uint8_t payload[4] = {0x00, 0x04, 0x00, (uint8_t)((sessionKey ^ 0x04) & 0xFF)};
    uint8_t plain[9] = {0x68, 0x02, 0x10, 0x21, 0x04, 0x00, 0x04, 0x00, payload[3]};
    uint16_t crc = crc16(plain, 9);

    uint8_t pkt[11];
    pkt[0]=0x68; pkt[1]=0x02; pkt[2]=0x10; pkt[3]=0x21; pkt[4]=0x04;
    for (int i = 0; i < 4; i++) pkt[5+i] = (payload[i] + sessionKey) & 0xFF;
    pkt[9] = (crc >> 8) & 0xFF;
    pkt[10] = crc & 0xFF;

    while (RS485.available()) RS485.read();
    RS485.write(pkt, 11);
    RS485.flush();

    unsigned long start = millis();
    uint8_t rx[16];
    int idx = 0;
    while (millis() - start < 500 && idx < 16) {
        if (RS485.available()) { rx[idx++] = RS485.read(); start = millis(); }
    }
    return (idx >= 5 && rx[3] == 0xA1);
}

// ==== Poll Telemetry ====
bool pollTelemetry() {
    uint8_t pkt[7] = {0x68, 0x02, 0x10, 0x27, 0x00, 0x44, 0x5E};
    while (RS485.available()) RS485.read();
    RS485.write(pkt, 7);
    RS485.flush();

    unsigned long start = millis();
    uint8_t rx[64];
    int idx = 0;
    while (millis() - start < 300 && idx < 64) {
        if (RS485.available()) { rx[idx++] = RS485.read(); start = millis(); }
    }

    int hdr = -1;
    for (int i = 0; i < idx - 5; i++) {
        if (rx[i]==0x68 && rx[i+1]==0x10 && rx[i+2]==0x02 && rx[i+3]==0xA7 && rx[i+4]==0x0E) {
            hdr = i; break;
        }
    }
    if (hdr < 0 || hdr + 19 > idx) return false;

    uint8_t *p = &rx[hdr + 5];

    // XOR key = byte paling sering
    int cnt[256] = {0};
    for (int i = 0; i < 14; i++) cnt[p[i]]++;
    int bestKey = 0, bestCnt = 0;
    for (int i = 0; i < 256; i++) {
        if (cnt[i] > bestCnt) { bestCnt = cnt[i]; bestKey = i; }
    }

    uint8_t d[14];
    for (int i = 0; i < 14; i++) d[i] = (p[i] - bestKey) & 0xFF;

    float voltage = ((d[0] << 8) | d[1]) / 10.0f;
    int16_t currentRaw = (int16_t)((d[2] << 8) | d[3]);
    int16_t rpm = (int16_t)((d[4] << 8) | d[5]);
    int tempCtrl = d[6] - 25;
    uint8_t s1 = d[8], s2 = d[9];
    bool parking = (s1 & 0x01) != 0;
    bool stand = (s1 & 0x04) != 0;
    int gear = s2 & 0x07;
    if (gear == 0 || gear > 3) gear = 0;
    float throttle = ((d[12] << 8) | d[13]) / 100.0f;

    // Fault flags
    uint8_t fault1 = d[10];
    uint8_t fault2 = d[11];

    // ODO/TRIP
    float speedEst = abs(rpm) / 14.75f;
    unsigned long now = millis();
    if (lastSpeedTime > 0) {
        float dtHours = (now - lastSpeedTime) / 3600000.0f;
        float kmDelta = speedEst * dtHours;
        if (kmDelta > 0 && kmDelta < 1.0f) {
            tripKm += kmDelta;
            odoKm += kmDelta;
        }
    }
    lastSpeedTime = now;

    if (now - lastSaveTime > 10000) {
        lastSaveTime = now;
        prefs.putFloat("odo", odoKm);
        prefs.putFloat("trip", tripKm);
    }

    // Battery % (fallback — HTML pakai template sendiri)
    float batt = (voltage - 48.0f) / (67.2f - 48.0f) * 100.0f;
    if (batt < 0) batt = 0;
    if (batt > 100) batt = 100;

    char json[280];
    snprintf(json, sizeof(json),
        "{\"r\":%d,\"v\":%.1f,\"a\":%.1f,\"b\":%d,"
        "\"g\":%d,\"p\":%s,\"stand\":%s,\"tempCtrl\":%d,"
        "\"thr\":%.2f,\"odo\":%.1f,\"trip\":%.1f,"
        "\"f1\":%u,\"f2\":%u,\"connected\":true}",
        rpm, voltage, currentRaw / 10.0f, (int)batt,
        gear, parking ? "true" : "false", stand ? "true" : "false",
        tempCtrl, throttle, odoKm, tripKm,
        fault1, fault2);

    pTxCharacteristic->setValue((uint8_t*)json, strlen(json));
    pTxCharacteristic->notify();
    return true;
}

// ==== Main Loop ====
void loop() {
    if (!deviceConnected) { delay(10); return; }

    if (currentMode == MODE_UNKNOWN && millis() - connectTime > 2000) {
        if (!receivedDataFromApp) {
            currentMode = MODE_STANDALONE;
            Serial.println("[MODE] STANDALONE");
        }
    }

    if (currentMode == MODE_BRIDGE) {
        if (RS485.available()) {
            uint8_t buf[256];
            int len = RS485.readBytes(buf, min((int)sizeof(buf), RS485.available()));
            if (len > 0) {
                pTxCharacteristic->setValue(buf, len);
                pTxCharacteristic->notify();
                Serial.printf("[RS485→BLE] %d bytes\n", len);
            }
        }
    }

    if (currentMode == MODE_STANDALONE) {
        if (!verified) {
            if (doHandshake()) {
                delay(200);
                verified = doVerify();
                Serial.println(verified ? "[DK72240] Verified ✓" : "[DK72240] Failed");
                if (!verified) delay(1000);
            } else {
                delay(500);
            }
        } else {
            if (millis() - lastPoll > 200) {
                lastPoll = millis();
                if (!pollTelemetry()) verified = false;
            }
        }
    }

    delay(5);
}