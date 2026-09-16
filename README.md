# LiteWave — เว็บเบราว์เซอร์ความเร็วสูง

[![Download Windows Installer](https://img.shields.io/badge/Download_Windows-Installer_.exe-0078D4?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Setup-Windows-x64.exe)
[![Download Windows Portable](https://img.shields.io/badge/Download_Windows-Portable_.zip-0078D4?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Windows-x64-portable.zip)
[![Download Linux Package](https://img.shields.io/badge/Download_Linux-.tar.gz-FCC624?style=for-the-badge&logo=linux&logoColor=black)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Linux-x64.tar.gz)
[![GitHub Release](https://img.shields.io/github/v/release/ynmio55/LiteWave?style=for-the-badge&label=Latest%20Release&color=2ea44f)](https://github.com/ynmio55/LiteWave/releases/latest)

---

LiteWave คือเว็บเบราว์เซอร์ยุคใหม่ที่เน้นความเร็ว ความเบา และความเป็นส่วนตัว ขับเคลื่อนด้วยเอนจิน LiteWave Shield (Rust `adblock-rs` + High-Speed Scriptlets) บล็อกโฆษณา ข้ามโฆษณา YouTube อัตโนมัติ ป้องกันการติดตาม และปกป้องความเป็นส่วนตัวโดยไม่ลดทอนความเร็วในการท่องเว็บ

---

## ⚡ คุณสมบัติเด่น

* 🛡️ **LiteWave Shield (Rust Core)**: ขับเคลื่อนด้วยเอนจิน `adblock-rs` ประสิทธิภาพสูง พร้อมกฎ EasyList บล็อกโฆษณา ป๊อบอัพ และสคริปต์ติดตามโดยอัตโนมัติ
* 🎬 **YouTube Ad-Skipper**: สกัดกั้นโฆษณา YouTube ระดับโครงสร้าง JSON (`ytInitialPlayerResponse` & `ytInitialData`) และข้ามโฆษณาวิดีโออัตโนมัติ เล่นวิดีโอทันทีโดยไม่มีอาการจอดำค้างรอนาน
* 🚀 **O(1) Media Fast-Path**: ออกแบบสตรีมมิ่งเอนจินพิเศษ ปล่อยผ่านวิดีโอสตรีมและไฟล์สื่อหลักด้วยความเร็วสูงโดยไร้ความล่าช้า (Zero-Latency Buffering)
* 🔒 **Secure DNS (DNS-over-HTTPS)**: รองรับ Cloudflare (1.1.1.1) ในโหมด Secure-Only ปกป้องข้อมูลการท่องเว็บให้ปลอดภัย
* 💻 **Cross-Platform**: รองรับทั้ง Windows (Installer & Portable) และ Linux (Fedora, Ubuntu ฯลฯ)

---

## 📥 การดาวน์โหลดและติดตั้ง

### ตัวเลือกการติดตั้งสำหรับผู้ใช้ทั่วไป

* Windows (แนะนำ): ดาวน์โหลด `Download Windows Installer (.exe)` แล้วเปิดไฟล์เพื่อติดตั้ง
* Windows (Portable): ดาวน์โหลด `Download Windows Portable (.zip)` แตกไฟล์แล้วเปิด `LiteWave.exe`
* Linux: ดาวน์โหลด `Download Linux (.tar.gz)` แตกไฟล์แล้วเปิด `LiteWave` (รวมไลบรารี Qt ที่จำเป็นไว้พร้อมใช้งาน)

👉 ดูรายการดาวน์โหลดทั้งหมดได้ที่ [GitHub Releases Page](https://github.com/ynmio55/LiteWave/releases/latest)

---

## 🛡️ การใช้งาน LiteWave Shield

LiteWave Shield เปิดใช้งานเป็นค่าเริ่มต้น (Standard Mode) เพื่อปกป้องคุณจากโฆษณาและสคริปต์ติดตาม:

* คลิกไอคอน Shield บริเวณแถบเครื่องมือด้านบนเพื่อปรับเปลี่ยนโหมด:
  * Standard: บล็อกโฆษณา ป๊อบอัพ และโฆษณาวิดีโอ YouTube
  * Aggressive: บล็อกเพิ่มเติมถึงระดับสคริปต์วิเคราะห์พฤติกรรม (Trackers & Analytics)
  * ปิด Shield สำหรับเว็บนี้: ปิดการกรองชั่วคราวสำหรับเว็บไซต์ที่เลือก หากพบปัญหาการแสดงผล
* ตัวเลขบนปุ่ม Shield แสดงจำนวนรายการโฆษณาและสคริปต์ที่ถูกบล็อกจริงในรอบการใช้งาน

---

## 🔒 การเปิดใช้งาน Secure DNS (DNS-over-HTTPS)

1. เปิด การตั้งค่า LiteWave → Secure DNS
2. ติ๊ก เปิดใช้งาน Secure DNS และเลือก Cloudflare (1.1.1.1)
3. กด ตกลง แล้วรีสตาร์ตโปรแกรมหนึ่งครั้ง
4. สถานะจะแสดงเป็นสีเขียว Secure-only ปกป้องการค้นหาชื่อโดเมนจากการดักรับข้อมูล

---

## 🐧 การใช้งานบน Linux (Fedora / Ubuntu)

### กรณีวิดีโอ H.264/AAC เล่นไม่ได้บน Fedora
หากพบบางเว็บเปิดวิดีโอแล้วหมุนค้าง ให้ติดตั้ง Codec เพิ่มเติมจาก RPM Fusion:

```bash
sudo dnf install libavcodec-freeworld
```
หรือสลับไปใช้ FFmpeg เวอร์ชันสมบูรณ์:
```bash
sudo dnf swap ffmpeg-free ffmpeg --allowerasing
```

### การประกอบและติดตั้งจากซอร์สโค้ด (Build from Source)

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtwebengine-devel
cd ~/Documents/LiteWave
git pull origin main
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

---

## 📜 ลิขสิทธิ์และการพัฒนา
พัฒนาด้วยภาษา C++20, Qt 6.8, และ Rust (`adblock-rs`) ภายใต้สัญญาอนุญาตซอฟต์แวร์เสรี
