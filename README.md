# ⚡ ESP32 Dual-Mode BLE Bridge for DK72240

Firmware ESP32 untuk membaca telemetri Controller DK72240 (via RS485) ke Web App (JSON), atau meneruskan data mentah ke Aplikasi DKYS (Bypass).

## 🚀 Konsep Dual-Mode
ESP32 ini secara otomatis mendeteksi siapa yang terhubung:
1. **Standalone Mode (Web App):** Jika tidak ada data dari HP, ESP32 melakukan *handshake* ke controller dan mengirim data JSON via BLE. Cocok untuk Custom HUD Speedometer.
2. **Bypass Mode (App DKYS):** Jika HP mengirim data, ESP32 menjadi *Transparent Bridge* (meneruskan data mentah HP <-> Controller). Cocok untuk setting/diagnosa pakai aplikasi resmi DKYS.

## 🛠 Fitur
- **Auto-Detect Mode:** Tanpa perlu tombol fisik.
- **Protokol Handshake DK72240:** Menangani CRC16 dan XOR key otomatis.
- **Output JSON:** Siap dikonsumsi Web Bluetooth API.
- **Contoh Web HUD:** Tersedia file HTML siap pakai (tanpa BMS) untuk melihat telemetri langsung dari HP.
- **NVS Storage:** Menyimpan ODO & TRIP di memori ESP32.

## 📂 Struktur Repository
- `example/` : Contoh Web App (HTML+JS) siap pakai untuk melihat telemetri.
- `docs/` : Dokumentasi protokol BLE dan JSON.
- `src/esp32/` : Source code firmware ESP32 (Arduino IDE).
- `src/web/` : Kode Javascript murni untuk implementasi Web Bluetooth.

## ⚠️ Hardware yang Dibutuhkan
- ESP32 (DevKit / WROOM)
- Modul RS485 to TTL (jika controller pakai RS485).
- Pin: RX = 25, TX = 26.

## 🚀 Cara Cepat Mencoba (Quick Start)
1. **Flash Firmware:** Buka `src/esp32/esp32_controller_dual_bridge.ino` di Arduino IDE, lalu upload ke ESP32.
2. **Buka Web HUD:** Buka file `example/index.html` menggunakan **Google Chrome di Android**. (Pastikan file di-hosting via HTTPS atau localhost agar Web Bluetooth bisa berjalan).
3. **Connect:** Tekan tombol "CONNECT SP-BLE" di layar, dan telemetri motor Anda akan muncul secara *real-time*.
