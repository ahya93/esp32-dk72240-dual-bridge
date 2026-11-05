/**
 * EsKero Web Bluetooth Client for ESP32 Controller
 * Digunakan untuk mendengarkan data JSON dari ESP32 (Mode Standalone)
 */

const CTRL_SERVICE = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const CTRL_TX      = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // Notify

let ctrlDevice;
let ctrlTxChar;
let ctrlBuffer = new Uint8Array(0);

let onTelemetryReceived = (data) => {
    console.log("Telemetry JSON:", data);
    // Di sini Anda update UI, contoh:
    // document.getElementById('speed').innerText = Math.round(data.r / 14.75);
    // document.getElementById('volt').innerText = data.v.toFixed(1);
};

async function connectController() {
    try {
        ctrlDevice = await navigator.bluetooth.requestDevice({
            filters: [{ name: 'SP-BLE' }],
            optionalServices: [CTRL_SERVICE]
        });

        ctrlDevice.addEventListener('gattserverdisconnected', onDisconnected);

        const server = await ctrlDevice.gatt.connect();
        const service = await server.getPrimaryService(CTRL_SERVICE);
        ctrlTxChar = await service.getCharacteristic(CTRL_TX);

        await ctrlTxChar.startNotifications();
        ctrlTxChar.addEventListener('characteristicvaluechanged', handleCtrlData);

        console.log("Connected to SP-BLE Controller!");

    } catch (error) {
        console.error("Connection failed:", error);
    }
}

function handleCtrlData(event) {
    const dv = event.target.value;
    const chunk = new Uint8Array(dv.buffer, dv.byteOffset, dv.byteLength);
    
    const newBuffer = new Uint8Array(ctrlBuffer.length + chunk.length);
    newBuffer.set(ctrlBuffer);
    newBuffer.set(chunk, ctrlBuffer.length);
    ctrlBuffer = newBuffer;

    if (ctrlBuffer.length > 8192) {
        ctrlBuffer = new Uint8Array(0);
        return;
    }

    let str = new TextDecoder().decode(ctrlBuffer);
    let jsonStart = str.indexOf('{');
    let jsonEnd = str.lastIndexOf('}');

    if (jsonStart !== -1 && jsonEnd !== -1 && jsonEnd > jsonStart) {
        let jsonStr = str.substring(jsonStart, jsonEnd + 1);
        try {
            let data = JSON.parse(jsonStr);
            ctrlBuffer = new Uint8Array(0);
            
            if (onTelemetryReceived) onTelemetryReceived(data);
            
        } catch (e) {
            // JSON tidak valid, tunggu chunk berikutnya
        }
    }
}

function onDisconnected() {
    console.log("Controller Disconnected");
    ctrlBuffer = new Uint8Array(0);
}