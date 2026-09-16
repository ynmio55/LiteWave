# LiteWave

[![Download Windows Installer](https://img.shields.io/badge/Download_Windows-Installer_.exe-0078D4?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Setup-Windows-x64.exe)
[![Download Windows Portable](https://img.shields.io/badge/Download_Windows-Portable_.zip-0078D4?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Windows-x64-portable.zip)
[![Download Linux Package](https://img.shields.io/badge/Download_Linux-.tar.gz-FCC624?style=for-the-badge&logo=linux&logoColor=black)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Linux-x64.tar.gz)
[![GitHub Release](https://img.shields.io/github/v/release/ynmio55/LiteWave?style=for-the-badge&label=Latest%20Release&color=2ea44f)](https://github.com/ynmio55/LiteWave/releases/latest)

---

LiteWave คือเว็บเบราว์เซอร์ที่เน้นความเร็วและความเป็นส่วนตัว ขับเคลื่อนด้วย LiteWave Shield ซึ่งรวมเอนจินบล็อกโฆษณาที่เขียนด้วย Rust ไว้ภายใน สามารถข้ามโฆษณา YouTube โดยอัตโนมัติ ป้องกันสคริปต์ติดตาม และรองรับการใช้งานบน Windows และ Linux

---

## คุณสมบัติหลัก

**LiteWave Shield**
เอนจินบล็อกโฆษณาที่สร้างด้วย Rust (`adblock-rs`) โดยใช้กฎ EasyList เป็นฐาน ทำงานในระดับ network-layer เพื่อสกัดกั้นคำขอที่ไม่ต้องการก่อนที่จะถึงเบราว์เซอร์

**YouTube Ad-Skipper**
ระบบสกัดกั้นโฆษณาในระดับโครงสร้างข้อมูล JSON (`ytInitialPlayerResponse` และ `ytInitialData`) ทำให้ตัวเล่นวิดีโอ YouTube รับรู้ตั้งแต่ต้นว่าไม่มีโฆษณา ส่งผลให้วิดีโอเริ่มเล่นได้ทันทีโดยไม่มีอาการหน้าจอดำรอโหลด

**Media Fast-Path**
ทราฟฟิกที่เกี่ยวกับสตรีมวิดีโอ เช่น `googlevideo.com` และ `ytimg.com` จะถูกยกเว้นจากกระบวนการตรวจสอบทั้งหมด เพื่อลดความล่าช้าในการโหลดสื่อ

**Secure DNS (DNS-over-HTTPS)**
รองรับ Cloudflare (1.1.1.1) และผู้ให้บริการ DoH อื่น ๆ ในโหมด Secure-only เพื่อป้องกันการดักรับข้อมูลในระดับ DNS

---

## การดาวน์โหลดและติดตั้ง

ดูรายการไฟล์ทั้งหมดได้ที่ [GitHub Releases](https://github.com/ynmio55/LiteWave/releases/latest)

| ระบบปฏิบัติการ | รูปแบบ | วิธีใช้งาน |
|---|---|---|
| Windows | `LiteWave-Setup-Windows-x64.exe` | รันไฟล์ติดตั้ง |
| Windows (Portable) | `LiteWave-Windows-x64-portable.zip` | แตกไฟล์และเปิด `LiteWave.exe` |
| Linux | `LiteWave-Linux-x64.tar.gz` | แตกไฟล์และเปิด `LiteWave` |

แพ็กเกจ Linux รวมไลบรารี Qt WebEngine ไว้แล้ว ไม่จำเป็นต้องติดตั้งเพิ่มเติม

---

## การใช้งาน LiteWave Shield

LiteWave Shield เปิดใช้งานโดยอัตโนมัติในโหมด Standard เมื่อเปิดโปรแกรม

การเปลี่ยนโหมดทำได้โดยคลิกที่ปุ่ม Shield บนแถบเครื่องมือ:

- **Standard** — บล็อกโฆษณาและสคริปต์โฆษณา third-party โดยไม่กระทบการทำงานของเว็บโดยรวม
- **Aggressive** — บล็อกเพิ่มเติมถึงระดับสคริปต์วิเคราะห์พฤติกรรมผู้ใช้ (Trackers และ Analytics)
- **ปิด Shield สำหรับเว็บนี้** — ยกเว้นการกรองสำหรับเว็บไซต์ปัจจุบัน เหมาะสำหรับกรณีที่เว็บแสดงผลผิดปกติ

ตัวเลขบนปุ่ม Shield แสดงจำนวนคำขอที่ถูกบล็อกจริงในการใช้งานรอบนั้น

---

## Secure DNS

1. เปิด การตั้งค่า LiteWave แล้วไปที่ Secure DNS
2. เลือก เปิดใช้งาน Secure DNS และเลือกผู้ให้บริการ เช่น Cloudflare (1.1.1.1)
3. กด ตกลง แล้วรีสตาร์ตโปรแกรม
4. สถานะจะแสดงเป็น Secure-only เมื่อระบบทำงานสมบูรณ์

---

## Linux — กรณีวิดีโอเล่นไม่ได้บน Fedora

Qt WebEngine จาก Fedora อาจไม่รวม Codec H.264/AAC เนื่องจากข้อจำกัดด้านลิขสิทธิ์ หากพบว่าวิดีโอโหลดค้างหรือไม่เล่น ให้ติดตั้ง Codec เพิ่มเติมจาก RPM Fusion:

```bash
sudo dnf install libavcodec-freeworld
```

หรือสลับไปใช้ FFmpeg เวอร์ชันสมบูรณ์:

```bash
sudo dnf swap ffmpeg-free ffmpeg --allowerasing
```

---

## การประกอบจากซอร์สโค้ด (Linux)

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtwebengine-devel
git clone https://github.com/ynmio55/LiteWave.git
cd LiteWave
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build && cp -f build/LiteWave ~/.local/bin/LiteWave 2>/dev/null || true
```

---

## การประกอบจากซอร์สโค้ด (Windows)

ต้องการ Qt 6, Visual Studio 2022 และ CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

ไฟล์ที่ได้จะอยู่ที่ `build/Release/LiteWave.exe`

---

## เทคโนโลยีที่ใช้

- C++20 และ Qt 6.8 (Qt WebEngine บน Chromium)
- Rust — เอนจินบล็อกโฆษณา (`adblock-rs`)
- CMake — ระบบประกอบโปรแกรม
