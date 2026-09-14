# LiteWave

LiteWave เป็นเว็บเบราว์เซอร์ขนาดเล็กสำหรับ Windows และ Fedora สร้างด้วย C++/Qt WebEngine ใช้ Google สำหรับการค้นหา และมี Shield สำหรับบล็อกโฆษณา/ตัวติดตามที่รู้จัก

## ความสามารถ

- ค้นหาด้วยคำปกติ ไม่ต้องพิมพ์ URL
- เปิด URL โดยตรงเมื่อพิมพ์โดเมนหรือ `https://`
- แท็บหลายหน้า
- ย้อนกลับ ถัดไป รีเฟรช และหน้าแรก
- Bookmark บันทึกถาวร
- Shield เปิด/ปิดได้
- บล็อกโดเมนโฆษณา ตัวติดตาม และ URL โฆษณาที่พบบ่อย
- โหมดมืด/สว่าง
- Cookies และ Cache ถาวร
- สร้างไฟล์ดาวน์โหลด Windows/Linux ผ่าน GitHub Actions

> ไม่มี AdBlock ใดรับประกันบล็อกโฆษณาได้ 100% โดยเฉพาะโฆษณาใน YouTube ซึ่งเปลี่ยนระบบอยู่เสมอ หากบล็อกมากเกินไปอาจทำให้เว็บหรือวิดีโอเล่นไม่ได้

## ติดตั้ง Fedora

### วิธีเปิดจากไฟล์ที่ Build เอง

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtwebengine-devel
cd ~/LiteWave
git pull origin main
rm -rf build
cmake -S . -B build
cmake --build build -j$(nproc)
./build/LiteWave
```

### ติดตั้งเข้าเครื่องและเมนู Applications

```bash
sudo cmake --install build
sudo cp packaging/LiteWave.desktop /usr/share/applications/
sudo update-desktop-database /usr/share/applications 2>/dev/null || true
```

เปิดจาก Terminal:

```bash
LiteWave
```

ถ้าเมนูยังไม่พบ ให้ Logout แล้ว Login ใหม่

### ดาวน์โหลดแพ็กเกจ Linux

เข้าแท็บ **Actions** หรือ **Releases** ของ GitHub แล้วดาวน์โหลด `LiteWave-Linux-x64.tar.gz`

```bash
tar -xzf LiteWave-Linux-x64.tar.gz
cd LiteWave
sudo install -Dm755 LiteWave /usr/local/bin/LiteWave
sudo install -Dm644 LiteWave.desktop /usr/share/applications/LiteWave.desktop
LiteWave
```

## Windows

### ดาวน์โหลดไฟล์พร้อมใช้

เข้า GitHub ที่แท็บ **Actions > Build LiteWave > Artifacts** แล้วดาวน์โหลด `LiteWave-Windows-x64.zip` จากนั้นแตกไฟล์และดับเบิลคลิก `LiteWave.exe`

ถ้ามี GitHub Release ให้ดาวน์โหลดไฟล์เดียวกันจากส่วน **Assets**

### Build เอง

ติดตั้ง Qt 6 พร้อม Qt WebEngine และ CMake แล้วรัน:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

ไฟล์อยู่ที่ `build/Release/LiteWave.exe`

## สร้างไฟล์ดาวน์โหลดด้วย GitHub

Workflow จะทำงานเมื่อกด **Actions > Build LiteWave > Run workflow** หรือเมื่อสร้าง tag เช่น:

```bash
git tag v0.6.0
git push origin v0.6.0
```

เมื่อใช้ tag จะสร้าง GitHub Release พร้อมไฟล์ Windows และ Linux อัตโนมัติ
