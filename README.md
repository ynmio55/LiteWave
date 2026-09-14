# LiteWave

เบราว์เซอร์ขนาดเล็กสำหรับ Windows และ Fedora สร้างด้วย C++ และ Qt WebEngine

## ความสามารถตอนนี้

- เปิดเว็บไซต์สมัยใหม่
- ช่อง URL และค้นหาผ่าน Google
- ย้อนกลับ / ไปข้างหน้า / รีเฟรช
- แถบแสดงความคืบหน้าการโหลด
- ใช้โค้ดชุดเดียวกันบน Windows และ Fedora

## ติดตั้ง dependencies

### Fedora

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtwebengine-devel
```

### Windows

ติดตั้ง Qt 6 ผ่าน Qt Online Installer โดยเลือก:

- Qt 6.x Desktop
- Qt WebEngine
- Qt Tools / CMake

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

รันบน Fedora:

```bash
./build/LiteWave
```

บน Windows ไฟล์โปรแกรมจะอยู่ใน:

```text
build/Release/LiteWave.exe
```

หมายเหตุ: Qt WebEngine ใช้ Chromium เป็นระบบแสดงผลเว็บ จึงเปิดเว็บสมัยใหม่ได้ดี แต่เว็บไซต์วิดีโอหรือเว็บที่มีเนื้อหาหนักยังใช้ RAM ตามขนาดเว็บนั้น
