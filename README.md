00% โดยเฉพาะโฆษณาใน YouTube ซึ่งเปลี่ยนระบบอยู่เสมอ หากบล็อกมากเกินไปอาจทำให้เว็บหรือวิดีโอเล่นไม่ได้

## ดาวน์โหลด Windows

เข้า GitHub ของโปรเจกต์ แล้วเลือก:

**Actions → Build LiteWave → Run workflow → เลือก workflow ล่าสุด**

ดาวน์โหลดไฟล์:

- `LiteWave-Setup-Windows-x64.exe` — ดับเบิลคลิกเพื่อติดตั้ง
- `LiteWave-Windows-x64-portable.zip` — แตกไฟล์แล้วเปิดใช้งาน

ถ้าสร้าง Release จาก tag แล้ว ให้ดาวน์โหลดตัวติดตั้งจาก **Releases → Assets**

## ติดตั้ง Fedora จากซอร์ส

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtwebengine-devel
cd ~/LiteWave
git pull origin main
rm -rf build
cmake -S . -B build
cmake --build build -j$(nproc)
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

## ดาวน์โหลดแพ็กเกจ Linux

ดาวน์โหลด `LiteWave-Linux-x64.tar.gz` จาก **Actions** หรือ **Releases** แล้วรัน:

```bash
tar -xzf LiteWave-Linux-x64.tar.gz
cd LiteWave
sudo install -Dm755 LiteWave /usr/local/bin/LiteWave
sudo install -Dm644 LiteWave.desktop /usr/share/applications/LiteWave.desktop
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
git tag v0.7.0
git push origin v0.7.0
```

GitHub Actions จะสร้างไฟล์ Windows Installer และ Linux package ให้อัตโนมัติ
