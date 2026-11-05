# ESP32 JSON & RS485 Protocol (DK72240)

## 1. Koneksi BLE
ESP32 menggunakan standar **Nordic UART Service (NUS)** dengan nama device `SP-BLE`.
- **Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX (Write)**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- **TX (Notify)**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`

## 2. Mode Bridge (Bypass)
Jika HP (Aplikasi DKYS) mengirim data ke karakteristik RX, ESP32 akan meneruskan *raw byte* tersebut ke UART RS485 (Pin 25/26) secara langsung.
*Berguna untuk mengubah setting atau diagnosa via aplikasi resmi.*

## 3. Mode Standalone (Web App)
Jika tidak ada data dari HP selama 2 detik, ESP32 akan mengirimkan JSON ke karakteristik TX setiap 200ms.

**Format JSON:**
```json
{
  "r": 1500,       // RPM
  "v": 78.5,       // Tegangan (Volt)
  "a": 12.3,       // Arus (Ampere)
  "b": 85,         // Estimasi Baterai (%) dari Controller
  "g": 2,          // Gear (1-3)
  "p": false,      // Parkir (true/false)
  "stand": false,  // Stand (true/false)
  "tempCtrl": 45,  // Suhu Controller
  "thr": 1.50,     // Tegangan Throttle
  "odo": 120.5,    // Odometer (km)
  "trip": 15.2,    // Trip (km)
  "f1": 0,         // Fault Flag 1
  "f2": 0,         // Fault Flag 2
  "connected": true
}