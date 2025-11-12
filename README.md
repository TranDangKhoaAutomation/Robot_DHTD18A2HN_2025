# Robot_DHTD18A2HN_2025 — Bluetooth Robot (UNETI)
**Tác giả:** Trần Đăng Khoa (TranDangKhoaTechnology) · **Năm:** 2025  
**Đơn vị:** Khoa Điện — Trường Đại học Kinh tế – Kỹ thuật Công nghiệp (UNETI)  
**CLB:** CLB Robot & Technology (thuộc Khoa Điện, UNETI)

Dự án xe robot điều khiển **Bluetooth Classic (SPP)** bằng **ESP32** + **4 servo tay gắp**.  
Phục vụ bài thi/biểu diễn **“Truy tìm kho báu”** của Khoa Điện & CLB Robot & Technology.

---

## Mục lục
- [Cấu trúc thư mục](#cấu-trúc-thư-mục)
- [Tính năng chính](#tính-năng-chính)
- [Phần cứng](#phần-cứng)
- [Nạp firmware (Arduino IDE)](#nạp-firmware-arduino-ide)
- [Ứng dụng điều khiển Bluetooth](#ứng-dụng-điều-khiển-bluetooth)
- [Bản đồ phím & điều khiển](#bản-đồ-phím--điều-khiển)
- [Gợi ý đấu dây](#gợi-ý-đấu-dây)
- [Demo](#demo)
- [Ghi công & giấy phép](#ghi-công--giấy-phép)

---

## Cấu trúc thư mục
```
Robot_DHTD18A2HN_2025/
├─ app/
│  └─ bluetoothRCcontroller.apk      # APK điều khiển BT (kết nối trực tiếp ESP32)
├─ demo/
│  └─ demo1.mp4                       # Video demo
├─ src/                               # Mã nguồn Arduino
│  ├─ dichuyen.ino                    # Điều khiển di chuyển
│  ├─ src.ino                         # (Main / hợp nhất)
│  └─ taygap.ino                      # Điều khiển tay gắp (4 servo)
├─ LICENSE
└─ README.md
```
> Lưu ý: tên file có thể thay đổi theo phiên bản. `src.ino` là điểm vào chính khi build.

---

## Tính năng chính
- ESP32 điều khiển 2 động cơ DC (tiến/lùi/trái/phải/chéo) bằng PWM `analogWrite()` (core **3.3.3**, không dùng LEDC trực tiếp).
- **4 servo tay gắp** (kẹp + tay) điều khiển mượt 50 Hz qua `ESP32Servo`.
- Điều khiển **qua Bluetooth Classic** bằng app Android (APK kèm trong `app/`), kết nối trực tiếp tới ESP32 (không qua HC‑05).
- **Chế độ X/x**:  
  - `X` (hoa): vào **Servo‑Steer Mode** — phím `L/R` sẽ xoay **SV4 từ từ** (mỗi nhịp 2°).
  - `x` (thường): quay về **Drive‑Steer Mode** — `L/R` lái bánh như bình thường.
- Failsafe: nếu **>500 ms** không nhận lệnh → dừng xe an toàn.

---

## Phần cứng
- **MCU:** ESP32‑DEVKIT (no‑PSRAM).  
- **Driver:** L298N/BTS7960 (cấu hình ENA/ENB + IN1..IN4).  
- **Servo:** 4× SG90/MG996R (tuỳ tải).  
- **Nguồn:** 2S‑3S + BEC 5 V/≥3 A cho servo (không lấy trực tiếp từ 5 V của ESP32).

---

## Nạp firmware (Arduino IDE)
1. **Board:** `ESP32 Dev Module`  
2. **Core:** `esp32` **v3.3.3** (Boards Manager).  
3. **Tools → CPU Freq:** 240 MHz · **Flash Freq:** 80 MHz · **Flash Size:** 4 MB · **Partition:** `Default`.  
4. **Library:** `ESP32Servo` ≥ 3.0.9 (Library Manager).  
5. Mở `src/src.ino` (hoặc file main hiện dùng) → **Upload**.  
> Ghi chú: Code dùng `analogWriteResolution(pin,bits)` và `analogWriteFrequency(pin,freq)` theo API của core 3.3.3.

---

## Ứng dụng điều khiển Bluetooth
- Tìm trong **`app/bluetoothRCcontroller.apk`** (ứng dụng điều khiển dạng joystick/phím).  
- Cài APK → Mở → **Scan** → chọn **`DHTD18A2HN_CAR`** (tên thiết bị ESP32 trong code).  
- Gán phím theo **bản đồ bên dưới** hoặc dùng phím mặc định của app (nếu có).

> **Kết nối:** Bluetooth Classic (SPP), ghép nối trực tiếp ESP32 — **không cần HC‑05/06**.

---

## Bản đồ phím & điều khiển
- **Lái cơ bản:** `F` tiến · `B` lùi · `L` trái · `R` phải · `S` dừng.  
- **Lái chéo:** `I` tiến‑phải · `G` tiến‑trái · `J` lùi‑phải · `H` lùi‑trái.  
- **Tốc độ:** `0..9` (map ra duty 0–1023).  
- **Chế độ:** `X` bật Servo‑Steer (L/R xoay SV4 từ từ) · `x` về lái thường.
- **Tay gắp:**  
  - `W` vươn tay · `w` thu tay  
  - `V` kẹp · `v` thả kẹp

> **SV4 step:** mặc định **2°/tick**, chu kỳ **15 ms** (có thể chỉnh trong biến `s4StepDeg`, `s4TickMs`).

---

## Gợi ý đấu dây
- **ENA/ENB:** PWM đến driver (20 kHz, 10‑bit).  
- **IN1..IN4:** điều khiển chiều.  
- **Servo S1..S4:** 33, 26, 27, 14 (50 Hz).  
- **LED_BT:** 25 (báo trạng thái kết nối).  
- **Nguồn servo riêng:** 5 V‑6 V, **GND chung** với ESP32.

> Khi test servo: gắn dần từng cái, kiểm tra **nhiễu nguồn** (tụ 470–1000 µF gần rail 5 V).

---

## Demo
- Xem **`demo/demo1.mp4`** (minh hoạ thao tác lái & tay gắp).  
- Khuyến nghị quay thêm video **đường ziczac + thao tác kẹp** để minh chứng khả năng điều khiển mượt.

---

## Ghi công & giấy phép
- **Tác giả:** Trần Đăng Khoa — lớp **DHTD18A2HN**, Khoa Điện, **UNETI**.  
- **CLB:** Robot & Technology — trực thuộc **Khoa Điện, UNETI**.  
- **Bản quyền & License:** MIT (xem `LICENSE`). Vui lòng ghi nguồn **TranDangKhoaTechnology** khi sử dụng lại.

---

### Liên hệ
- YouTube/Github: **TranDangKhoaTechnology**  
- Email: trandangkhoa31122006@gmail.com
- Issues/PR: chào mừng mọi đóng góp từ cộng đồng!
