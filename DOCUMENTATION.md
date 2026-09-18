# คู่มือการติดตั้งและใช้งาน LiteWave (LiteWave Documentation)

เอกสารฉบับทางการสำหรับโครงการ **LiteWave** เวอร์ชัน 1.1.0 (โอเพนซอร์ส GPL-3.0)  
จัดทำขึ้นเพื่อระบุข้อมูลทางเทคนิค วิธีการติดตั้งผ่านแพ็กเกจสำเร็จรูป การคอมไพล์จากซอร์สโค้ด และการแก้ปัญหาทั่วไปตามความเป็นจริง

---

## 1. ข้อมูลภาพรวมของระบบ (Project Overview)

LiteWave เป็นเบราว์เซอร์สำหรับคอมพิวเตอร์ (Windows & Linux) ที่พัฒนาขึ้นด้วย:
- **ภาษาหลัก:** C++17 / C++20
- **UI & Web Engine:** Qt 6.4+ (Qt Widgets และ Qt WebEngine ซึ่งใช้ Chromium เป็นเบสในการเรนเดอร์หน้าเว็บ)
- **ระบบบล็อกโฆษณา (LiteWave Shield):** เขียนด้วยภาษา Rust โดยใช้ไลบรารี `adblock-rs` (เวอร์ชัน 0.13.3) พร้อมฐานกฎจาก EasyList
- **ระบบประกอบโปรแกรม (Build System):** CMake และ Cargo (Rust)

> **ข้อจำกัดที่ควรทราบในเวอร์ชันปัจจุบัน:**
> - ยังไม่รองรับการติดตั้ง Extension จาก Chrome Web Store
> - ยังไม่มีระบบบัญชีหรือ Cloud Sync คั่นหน้าข้ามอุปกรณ์
> - บน Linux ดิสโทรบางตัว (เช่น Fedora) อาจต้องติดตั้งไลบรารี Codec (H.264/AAC) เพิ่มเติมเพื่อให้เล่นวิดีโอบนเว็บบางแห่งได้สมบูรณ์

---

## 2. วิธีการติดตั้งและใช้งานผ่านแพ็กเกจสำเร็จรูป (Prebuilt Binaries)

หากไม่ต้องการคอมไพล์โค้ดเอง สามารถดาวน์โหลดไฟล์สำเร็จรูปได้จาก [GitHub Releases](https://github.com/ynmio55/LiteWave/releases/v1.1.0):

### 2.1 บนระบบ Linux (x86_64)

แพ็กเกจ `LiteWave-Linux-x64.tar.gz` ได้รวมไลบรารีที่จำเป็นไว้แล้ว

1. ดาวน์โหลดไฟล์:
   ```bash
   wget https://github.com/ynmio55/LiteWave/releases/download/v1.1.0/LiteWave-Linux-x64.tar.gz
   ```
2. แตกไฟล์ tar.gz:
   ```bash
   tar -xzf LiteWave-Linux-x64.tar.gz
   ```
3. รันโปรแกรม:
   ```bash
   cd LiteWave-Linux-x64  # หรือไดเรกทอรีที่แตกไฟล์
   ./LiteWave
   ```

*(ทางเลือก)* คัดลอกไปยัง `~/.local/bin` เพื่อให้เรียกใช้ผ่าน Terminal ได้สะดวก:
```bash
cp LiteWave ~/.local/bin/
```

---

### 2.2 บนระบบ Windows (x64)

รองรับ Windows 10 และ Windows 11 (64-bit):
- **แบบตัวติดตั้ง (Installer):** ดาวน์โหลด `LiteWave-Setup-Windows-x64.exe` แล้วดับเบิลคลิกเพื่อติดตั้งตามขั้นตอนในหน้าจอ
- **แบบพกพา (Portable):** ดาวน์โหลด `LiteWave-Windows-x64-portable.zip` แตกไฟล์โฟลเดอร์ออกมาแล้วเปิด `LiteWave.exe` ใช้งานได้ทันทีโดยไม่ต้องติดตั้ง

---

## 3. วิธีการคอมไพล์จากซอร์สโค้ด (Building from Source)

### 3.1 การเตรียมสภาพแวดล้อมบน Linux

#### สำหรับ Fedora / RHEL:
```bash
sudo dnf install gcc-c++ cmake rust cargo qt6-qtbase-devel qt6-qtwebengine-devel
```

#### สำหรับ Ubuntu / Debian (Ubuntu 22.04 LTS ขึ้นไป):
```bash
sudo apt update
sudo apt install build-essential cmake cargo rustc \
    qt6-base-dev qt6-webengine-dev qt6-tools-dev \
    libgl1-mesa-dev
```

#### สำหรับ Arch Linux / Manjaro:
```bash
sudo pacman -S base-devel cmake rust qt6-base qt6-webengine
```

---

### 3.2 คำสั่งคอมไพล์บน Linux

```bash
# 1. Clone ซอร์สโค้ด
git clone https://github.com/ynmio55/LiteWave.git
cd LiteWave

# 2. สร้างโฟลเดอร์ build และสร้าง Makefile ด้วย CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 3. เริ่มคอมไพล์ (จะทำการคอมไพล์โมดูล Rust ก่อน จากนั้นจึงคอมไพล์ C++)
cmake --build build -j$(nproc)

# 4. ทดสอบรันโปรแกรม
./build/LiteWave
```

---

### 3.3 การคอมไพล์บน Windows

**สิ่งที่ต้องมีในเครื่อง:**
1. Visual Studio 2022 (พร้อมติ๊กเลือก "Desktop development with C++")
2. CMake (เวอร์ชัน 3.21 ขึ้นไป)
3. Rust และ Cargo (`rustup-init.exe`)
4. Qt 6 (Qt 6.4 ขึ้นไป ติดตั้งโมดูล MSVC 2022 64-bit และ Qt WebEngine)

**คำสั่งบน PowerShell:**
```powershell
# 1. Clone ซอร์สโค้ด
git clone https://github.com/ynmio55/LiteWave.git
cd LiteWave

# 2. รัน CMake (ระบุที่อยู่ของ Qt6 หากไม่อยู่ใน PATH)
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.6.2\msvc2022_64"

# 3. สั่งคอมไพล์ Release
cmake --build build --config Release

# 4. ไฟล์ปฏิบัติการจะอยู่ที่
.\build\Release\LiteWave.exe
```

---

## 4. การแก้ปัญหาเฉพาะทาง (Troubleshooting)

### 4.1 วิดีโอบน Fedora เล่นไม่ได้ หรือมีอาการหมุนค้าง
เกิดจาก Fedora ตัด Codec ที่ติดสิทธิบัตร (เช่น H.264 และ AAC) ออกจากเวอร์ชันมาตรฐานของแพ็กเกจฟรี วิธีแก้ไขให้ติดตั้ง Codec เพิ่มเติมจาก RPM Fusion:

```bash
# ติดตั้ง libavcodec จาก RPM Fusion
sudo dnf install libavcodec-freeworld

# หรือสลับไปใช้ ffmpeg เวอร์ชันสมบูรณ์
sudo dnf swap ffmpeg-free ffmpeg --allowerasing
```

### 4.2 หน้าเว็บแสดงผลเพี้ยน หรือเข้าใช้งานฟังก์ชันบางอย่างไม่ได้
เว็บไซต์บางแห่งอาจมีการตรวจสอบ Adblocker หรือมีสคริปต์ที่ขึ้นอยู่กับเซิร์ฟเวอร์โฆษณา:
1. คลิกที่ไอคอน **Shield** บริเวณมุมขวาบนของแถบเครื่องมือ
2. เลือก **"ปิด Shield สำหรับเว็บนี้"**
3. รีเฟรชหน้าเว็บเพื่อเข้าใช้งานตามปกติ

---

## 5. การตั้งค่าฟีเจอร์สำคัญ (Configuration)

### 5.1 การปรับระดับการทำงานของ LiteWave Shield
- **Standard (ค่าเริ่มต้น):** บล็อกคำขอโฆษณาและสคริปต์ภายนอกทั่วไป เหมาะกับการใช้งานปกติ
- **Aggressive:** บล็อกตัวติดตาม (Trackers) และสคริปต์สถิติเพิ่มเติม (อาจมีผลกระทบกับเว็บบางแห่ง)
- **Whitelist:** ปิดการทำงานเฉพาะเว็บไซต์ที่กำลังเปิดอยู่

### 5.2 การเปิดใช้งาน Secure DNS (DNS-over-HTTPS)
1. เปิดเมนู **การตั้งค่า (Settings)** ของ LiteWave
2. ไปที่หัวข้อ **Secure DNS**
3. ติ๊ก **เปิดใช้งาน Secure DNS** และเลือกผู้ให้บริการ เช่น Cloudflare (1.1.1.1)
4. กดบันทึก แล้วรีสตาร์ตเบราว์เซอร์
