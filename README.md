# DKYS ESP32 Dual-Mode Bridge (DK72240)

Repository ini berisi *source code* dan dokumentasi untuk menghubungkan **Web App (PWA)** dan **Aplikasi Resmi (DKYS)** ke **Controller DK72240** menggunakan **ESP32** sebagai jembatan (Bridge).

## 🚀 Fitur Utama

1. **Auto-Detect Dual-Mode:**
   - **Standalone Mode (Web App):** Jika tidak ada data dari HP, ESP32 otomatis melakukan *handshake* dan *polling* telemetri dari controller, lalu mengirimnya sebagai JSON via BLE.
   - **Bypass Mode (App DKYS):** Jika HP mengirimkan data (Hex/Command), ESP32 otomatis beralih menjadi *Transparent Bridge* (meneruskan data mentah HP <-> Controller).

2. **Protokol Handshake DK72240:**
   - Menangani pertukaran kunci sesi (Seed/Verify) secara otomatis.
   - Deteksi kunci XOR dinamis untuk membuka *payload* telemetri.

3. **Fitur Tambahan:**
   - Perhitungan ODO & TRIP disimpan di NVS (Non-Volatile Storage) ESP32.
   - Deteksi *Fault Flags* (kode error controller).
   - Output JSON yang siap dikonsumsi oleh Web Bluetooth API.

## 📂 Struktur Repository
- `docs/` : Penjelasan detail protokol JSON dan Handshake.
- `src/esp32/` : Source code firmware ESP32 (Arduino IDE).
- `src/web/` : Contoh implementasi Web Bluetooth API untuk membaca JSON.

## ⚠️ Hardware
- ESP32 (DevKit / WROOM)
- RS485 to TTL Module (MAX485 / MAX3485) jika controller menggunakan RS485.
- Pin: RX = 25, TX = 26 (Sesuaikan di kode). DKYS (Bypass).



Contoh Penggunaan:
https://eskero-speedo.netlify.app/