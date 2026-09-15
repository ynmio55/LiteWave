
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
* **Linux:** กด **`Download Linux (.tar.gz)`** แตกไฟล์แล้วเปิด `LiteWave` ได้เลย — Qt WebEngine ที่จำเป็นรวมอยู่ในแพ็กเกจแล้ว

### ตัวเลือกที่ 2: ดาวน์โหลดจากหน้า Releases บน GitHub
1. คลิกลิงก์ 👉 **[ไปที่หน้าดาวน์โหลด Releases](https://github.com/ynmio55/LiteWave/releases/latest)**
2. ใต้หัวข้อ **Assets** เลือกไฟล์ตามระบบ: Windows ใช้ `.exe`/`.zip`, Linux ใช้ `.tar.gz`
3. ไม่ต้องล็อกอิน GitHub เพื่อดาวน์โหลด


## ความเข้ากันได้ของเว็บไซต์และวิดีโอ

LiteWave ไม่มีระบบบล็อกโฆษณา, cosmetic filter, popup filter หรือ request interceptor แล้ว เพื่อให้เว็บและตัวเล่นวิดีโอทำงานตามปกติของ Qt WebEngine

### Secure DNS ที่ทำงานใน LiteWave

ไม่ต้องเปลี่ยน DNS ของระบบเองสำหรับไฟล์ดาวน์โหลดล่าสุด:

1. เปิด **⚙ การตั้งค่า LiteWave** → **Secure DNS**
2. ติ๊ก **เปิดใช้งาน Secure DNS** และเลือก **Cloudflare (1.1.1.1)**
3. กด **ตกลง** จากนั้นปิด LiteWave ทุกหน้าต่างแล้วเปิดใหม่หนึ่งครั้ง
4. กลับมาที่หน้า Secure DNS: สถานะสีเขียวต้องขึ้นว่า **Secure-only**

LiteWave ใช้ตัว resolver ของ Qt WebEngine โดยตรงในโหมด **Secure-only** จึงไม่ย้อนกลับไปใช้ DNS ของระบบเงียบ ๆ หากผู้ให้บริการ DoH เข้าไม่ถึง เว็บจะแจ้งปัญหาแทนการใช้ DNS เดิม; ให้เลือก OS Default เพื่อกลับสู่ DNS ของระบบได้ทันทีในการเปิดครั้งถัดไป

ไฟล์ Linux ล่าสุดรวม Qt 6.8 และ Qt WebEngine ที่รองรับ Secure DNS แล้ว ส่วนการ build จากซอร์สต้องใช้ Qt WebEngine 6.6 ขึ้นไปเพื่อใช้ฟังก์ชันนี้

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

## ติดตั้งแพ็กเกจ Linux (ไม่ต้องติดตั้ง Qt เพิ่ม)

ดาวน์โหลด `LiteWave-Linux-x64.tar.gz` จาก **Releases** แล้วเปิดได้ทันที:

```bash
tar -xzf LiteWave-Linux-x64.tar.gz
cd LiteWave
./LiteWave
```

แพ็กเกจนี้พก Qt 6.8, Qt WebEngine, resources และ launcher มาด้วย อย่าแยกไฟล์ `bin`, `lib`, `plugins`, `resources` หรือ `libexec` ออกจากโฟลเดอร์ `LiteWave`

ถ้าต้องการเรียกจากเมนูแอปพลิเคชัน ให้ย้ายทั้งโฟลเดอร์ไปไว้ที่ `/opt` แล้วสร้างคำสั่งลัด:

```bash
sudo rm -rf /opt/LiteWave
sudo cp -a LiteWave /opt/LiteWave
sudo ln -sf /opt/LiteWave/LiteWave /usr/local/bin/LiteWave
sudo install -Dm644 /opt/LiteWave/LiteWave.desktop /usr/share/applications/LiteWave.desktop
sudo install -Dm644 /opt/LiteWave/litewave.svg /usr/share/icons/hicolor/scalable/apps/litewave.svg
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
