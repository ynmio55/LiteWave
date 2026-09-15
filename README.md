
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
1. มองไปที่แถบขวามือของหน้า GitHub นี้ ตรงหัวข้อ **`Releases`** (จะเห็นคำว่า **`LiteWave v0.9.1`**)
2. คลิกที่ชื่อ **`LiteWave Latest`** หรือคลิกที่ลิงก์ 👉 **[ไปที่หน้าดาวน์โหลด Releases](https://github.com/ynmio55/LiteWave/releases/latest)**
3. เลื่อนลงมาใต้หัวข้อ **Assets** แล้วกดดาวน์โหลดไฟล์ `.exe` หรือ `.zip` ได้ทันที! (ไม่ต้อง Login ก็โหลดได้)


## Shield 0.9.0

- รายการ AdAway 6,540 โดเมนแนบมาในโปรแกรม + กฎโดเมน/เส้นทางของ LiteWave
- เมนู **Shield → ป้องกันเว็บนี้** เปิด/ปิดเฉพาะ hostname ปัจจุบันได้; ปิดทั้งหมดจะหยุดทั้งการบล็อกคำขอ การซ่อน CSS และตัวสังเกตหน้าเว็บ
- **อัปเดตรายการบล็อก…** ดาวน์โหลด AdAway ผ่าน HTTPS หลังคุณกดยืนยันเท่านั้น (GitHub เห็น IP แต่ไม่ได้รับประวัติ/URL ที่ท่องเว็บ); ไม่เปลี่ยน DNS/hosts ของระบบ
- อัปเดตตรวจรูปแบบและขนาด เขียนไฟล์แบบ atomic; เมื่อเครือข่ายล้มเหลวหรือรายการผิดรูปแบบจะใช้รายการเดิมต่อ
- โหมด Private ไม่บันทึกข้อยกเว้นหรือรายการอัปเดตลงดิสก์
- ป้องกันป๊อปอัปที่เปิดเอง และปลายทางโฆษณาที่รู้จัก; ถ้าหน้าล็อกอิน/ชำระเงินเปิดไม่ได้ ให้ปิด Shield เฉพาะเว็บแล้วลองใหม่
- ซ่อนช่องโฆษณาที่โหลดตามหลัง และช่วยกด **ปุ่มข้ามที่มองเห็นได้** บน YouTube; ไม่เร่งวิดีโอ ไม่ปิดเสียง ไม่เปลี่ยนเวลา
- ตัวนับคือคำขอ/ป๊อปอัปที่ถูกบล็อกทั้งโปรไฟล์ในรอบเปิดโปรแกรม ไม่ใช่จำนวนโฆษณาหรือจำนวนที่ซ่อนด้วย CSS
- ไม่ใช่ตัวประมวลผล EasyList/uBlock เต็มรูปแบบ โฆษณาที่รวมมากับสตรีม, โฆษณาฝังในคลิป, anti-adblock และเนื้อหาในบาง iframe ยังบล็อกไม่ได้
- กฎ AdAway ใช้แบบ exact hostname; เฉพาะโดเมนเครือข่ายโฆษณาที่ LiteWave ระบุเองเท่านั้นที่รวม subdomain ไม่บล็อกตามหมวดเว็บผู้ใหญ่
- [ที่มาและเครดิตรายการ](THIRD_PARTY_NOTICES.md)

### อัปเดต Fedora ที่ติดตั้งจากซอร์ส

ปิด LiteWave ทุกหน้าต่างก่อน แล้วรัน (หยุดทันทีหากขั้นตอนใดผิดพลาด):

```bash
cd ~/LiteWave &&
git pull --ff-only origin main &&
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release &&
cmake --build build -j2 &&
sudo cmake --install build &&
LiteWave
```

### ทดสอบ Shield

```bash
node tests/shield-script.test.cjs
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

CI ทดสอบตัวกรอง C++ ทั้ง Windows/Qt 6.8 และ Linux/Qt 6.4 รวมถึงการซ่อน/คืน DOM,
การบล็อกคำขอจริง, toggle และค่าความเร็ว/เสียงวิดีโอบน Qt WebEngine ของ Linux
ผลนี้ไม่ใช่การรับรองว่าใช้งานทุกเว็บไซต์ได้

## ความเข้ากันได้ของเว็บไซต์และวิดีโอ

LiteWave ใช้ Qt WebEngine ไม่ได้มีข้อจำกัดตามหมวดเว็บไซต์ แต่ไม่รับประกันว่าเปิดได้ทุกเว็บเหมือน Chrome/Brave

- รองรับ HTML/JavaScript, cookies, local storage และ fullscreen ตามความสามารถของ Qt ที่ติดตั้ง
- กด **ตรวจวิดีโอ** เพื่อดูความสามารถที่เครื่องรายงานสำหรับ H.264/AAC, VP9/Opus, AV1 และ MediaSource
- H.264/AAC และ DRM ขึ้นกับ Qt build และส่วนประกอบที่ติดตั้ง ไม่ได้เพิ่มได้ด้วยการเปิด setting อย่างเดียว
- รุ่นนี้ไม่รวม Widevine และไม่ข้ามการยืนยันอายุ, CAPTCHA, DRM หรือข้อจำกัดการเข้าถึงของเว็บ
- [ข้อจำกัด codec/DRM ตามเอกสาร Qt](https://doc.qt.io/qt-6/qtwebengine-features.html)
- กล้อง ไมค์ ตำแหน่ง และการแจ้งเตือนต้องได้รับอนุญาต ไม่ได้อนุญาตทุกเว็บไซต์อัตโนมัติ
- ไม่ปลอม Windows/Chrome เวอร์ชันตายตัวบนทุกเครื่อง
- สคริปต์ช่วยข้ามโฆษณาไม่เปลี่ยนความเร็ว ไม่ปิดเสียง และไม่เลื่อนเวลาวิดีโอ
- Shield ไม่สามารถบล็อกโฆษณาได้ทั้งหมด และบางเว็บอาจไม่ยอมเล่นเมื่อมีตัวบล็อก

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
