
# LiteWave — เว็บเบราว์เซอร์ความเร็วสูง

[![Download Windows Installer](https://img.shields.io/badge/Download_Windows-Installer_.exe-0078D4?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Setup-Windows-x64.exe)
[![Download Windows Portable](https://img.shields.io/badge/Download_Windows-Portable_.zip-0078D4?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Windows-x64-portable.zip)
[![Download Linux Package](https://img.shields.io/badge/Download_Linux-.tar.gz-FCC624?style=for-the-badge&logo=linux&logoColor=black)](https://github.com/ynmio55/LiteWave/releases/download/latest/LiteWave-Linux-x64.tar.gz)
[![GitHub Release](https://img.shields.io/github/v/release/ynmio55/LiteWave?style=for-the-badge&label=Latest%20Release&color=2ea44f)](https://github.com/ynmio55/LiteWave/releases/latest)

---

##  วิธีดาวน์โหลดติดตั้งง่ายๆ (สำหรับผู้ใช้ทั่วไป)

### ตัวเลือกที่ 1: กดปุ่มดาวน์โหลดโดยตรงข้างบนนี้
* **Windows (แนะนำ):** กดปุ่ม **`Download Windows Installer (.exe)`** แล้วกดรันเพื่อติดตั้งได้ทันที
* **Windows ไม่ต้องติดตั้ง:** กด **`Download Windows Portable (.zip)`** แตกไฟล์แล้วเปิด `LiteWave.exe` เล่นได้เลย
* **Linux:** กด **`Download Linux (.tar.gz)`** (สร้างบน Ubuntu 24.04 / Qt 6)

### ตัวเลือกที่ 2: ดาวน์โหลดจากหน้า Releases บน GitHub
1. มองไปที่แถบขวามือของหน้า GitHub นี้ ตรงหัวข้อ **`Releases`** (จะเห็นคำว่า **`LiteWave v0.9.3`**)
2. คลิกที่ชื่อ **`LiteWave Latest`** หรือคลิกที่ลิงก์ 👉 **[ไปที่หน้าดาวน์โหลด Releases](https://github.com/ynmio55/LiteWave/releases/latest)**
3. เลื่อนลงมาใต้หัวข้อ **Assets** แล้วกดดาวน์โหลดไฟล์ `.exe` หรือ `.zip` ได้ทันที! (ไม่ต้อง Login ก็โหลดได้)


## ความเข้ากันได้ของเว็บไซต์และวิดีโอ

LiteWave ไม่มีระบบบล็อกโฆษณา, cosmetic filter, popup filter หรือ request interceptor แล้ว เพื่อให้เว็บและตัวเล่นวิดีโอทำงานตามปกติของ Qt WebEngine

### Fedora: วิดีโอ H.264/AAC

เว็บสตรีมจำนวนมากใช้ H.264/AAC (รวมถึงตัวเล่นฝัง iframe) แต่แพ็กเกจ Qt WebEngine จาก Fedora อาจไม่มี codec เหล่านี้จากข้อจำกัดลิขสิทธิ์ จึงทำให้หน้าเว็บเปิดได้แต่วิดีโอค้าง/หมุน

หลังเปิด RPM Fusion ให้เพิ่ม codec ที่ Fedora ตัดออก แล้วเปิด LiteWave ใหม่:

```bash
sudo dnf install libavcodec-freeworld
```

หากติดตั้งไม่ได้หรือวิดีโอยังไม่เล่น ให้สลับ FFmpeg รุ่น Fedora ที่จำกัด codec ไปเป็นรุ่น RPM Fusion:

```bash
sudo dnf swap ffmpeg-free ffmpeg --allowerasing
```

ไม่ต้องใช้ `qt6-qtwebengine-freeworld`: Fedora 44 ไม่มีแพ็กเกจชื่อนี้

DRM เช่น Widevine, การล็อกอิน, การจำกัดพื้นที่ หรือเซิร์ฟเวอร์ของเว็บ ยังเป็นเงื่อนไขของเว็บนั้นและ LiteWave ไม่ข้ามให้

## ตรวจหลังติดตั้ง

1. พิมพ์คำค้นภาษาไทยและเปิด URL ปกติ
2. เปิดหลายแท็บ ดาวน์โหลดไฟล์หนึ่งครั้ง ตรวจว่าไม่มีการเริ่มซ้ำ
3. ใช้ Ctrl+S ตรวจว่าไฟล์ถูกบันทึกตามตำแหน่งที่เลือก
4. เล่นวิดีโอที่มีสิทธิ์เข้าถึง ทดสอบเสียง ความเร็ว Fullscreen และ Esc
5. เปิดหน้าทดสอบกล้อง/ไมค์ที่เชื่อถือได้ ตรวจว่ามีคำถามอนุญาตก่อนใช้งาน

## ติดตั้ง Fedora จากซอร์ส

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtwebengine-devel
cd ~/LiteWave
git pull --ff-only origin main &&
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release &&
cmake --build build -j2 &&
./build/LiteWave
```

ติดตั้งเข้าเมนู Applications:

```bash
sudo cmake --install build
sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
sudo update-desktop-database /usr/share/applications 2>/dev/null || true
```

เปิดภายหลังได้ด้วย:

```bash
LiteWave
```

## ติดตั้งแพ็กเกจ Linux (Ubuntu 24.04 / Qt 6.4 ที่เข้ากันได้)

ดาวน์โหลด `LiteWave-Linux-x64.tar.gz` จาก **Actions** หรือ **Releases** แล้วรัน:

```bash
tar -xzf LiteWave-Linux-x64.tar.gz
cd LiteWave
sudo install -Dm755 LiteWave /usr/local/bin/LiteWave
sudo install -Dm644 LiteWave.desktop /usr/share/applications/LiteWave.desktop
sudo install -Dm644 litewave.svg /usr/share/icons/hicolor/scalable/apps/litewave.svg
sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
sudo update-desktop-database /usr/share/applications 2>/dev/null || true
LiteWave
```

## Build Windows เอง

ติดตั้ง Qt 6 พร้อม Qt WebEngine, Visual Studio 2022 และ CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

ไฟล์อยู่ที่ `build/Release/LiteWave.exe`

## สร้าง Release

```bash
git tag v0.9.0
git push origin v0.9.0
```

GitHub Actions จะสร้างไฟล์ Windows Installer และ Linux package ให้อัตโนมัติ
