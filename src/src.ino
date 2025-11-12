/*
 * ================================================================
 *  ROBOT BLUETOOTH + 4 SERVO — ESP32 core 3.3.3
 *  - Không dùng LEDC trực tiếp; chỉ dùng analogWrite(*) cho motor
 *  - Điều khiển qua BluetoothSerial (SPP)
 *  - 4 servo điều khiển bằng thư viện ESP32Servo
 *
 *  PHÍM (gửi qua BT):
 *    Lái cơ bản:  F (tiến), B (lùi), L (trái), R (phải), S (dừng)
 *    Lái chéo:    I (tiến-phải), G (tiến-trái), J (lùi-phải), H (lùi-trái)
 *    Tốc độ:      '0'..'9' (0 = chậm nhất, 9 = nhanh nhất)
 *    Tay/kẹp:     W (vươn tay), w (thu tay), V (kẹp), v (thả kẹp)
 *    Chế độ lái:  X = bật "Servo-steer": L/R sẽ quay SV4 từ từ
 *                 x = về "Drive-steer" bình thường: L/R lái bánh
 *
 *  Tác giả: Trần Đăng Khoa (CodeWithKhoa) — 2025 — MIT
 * ================================================================
 */

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ESP32Servo.h>

// ================== SƠ ĐỒ CHÂN (PIN MAP) ==================
// Hai ENA/ENB là chân PWM cấp độ rộng xung cho driver motor.
// Bốn chân IN1..IN4 là chiều quay (HIGH/LOW) cho 2 kênh A/B.
#define ENA     2
#define IN1     4
#define IN2     5
#define IN3    18
#define IN4    19
#define ENB    21

// Đèn báo Bluetooth có client (bật/tắt).
#define LED_BT 25

// ------------------ Servo pins ------------------
// Lưu ý: S1_PIN=33, S2_PIN=26, S3_PIN=27, S4_PIN=14 như bạn yêu cầu.
// Không dùng các chân có xung đột đặc biệt (TX0/RX0, strapping) cho servo.
#define NUM_SERVOS 4
#define S1_PIN    33
#define S2_PIN    26
#define S3_PIN    27
#define S4_PIN    14
const int SERVO_PINS[NUM_SERVOS] = { S1_PIN, S2_PIN, S3_PIN, S4_PIN };

// ================== BIẾN TOÀN CỤC ==================
BluetoothSerial SerialBT;   // Kênh Bluetooth SPP (classic) cho ESP32

// Đối tượng servo và góc hiện tại (độ) cho 4 servo.
// Mặc định bạn đặt: S1=0, S2=90, S3=20, S4=90
Servo servos[NUM_SERVOS];
int   servoAngle[NUM_SERVOS] = { 0, 90, 20, 90 };

// Trạng thái lái động cơ (dùng kiểu 2-motor: A & B)
int8_t   dirA = 0, dirB = 0;    // Hướng: -1 lùi, 0 dừng, +1 tiến
uint8_t  kA   = 100, kB = 100;  // Hệ số phần trăm cho lái chéo (giảm 1 bên)
uint8_t  speedLevel = 5;        // Mức tốc độ 0..9
uint16_t baseDuty   = 0;        // Duty 10-bit (0..1023)
uint16_t dutyA      = 0, dutyB = 0;  // Duty hai kênh sau khi scale kA/kB

// ===== Chế độ "Servo-steer" bằng phím X/x =====
// Khi bật (X), nhấn L/R sẽ không quay bánh mà sẽ quay servo S4 từ từ.
bool     servoSteerMode = false;   // false = lái thường; true = L/R quay SV4
int8_t   s4Dir = 0;                // Hướng quay SV4: -1 giảm góc, +1 tăng, 0 dừng
uint8_t  s4StepDeg = 2;            // Mỗi nhịp tăng/giảm bao nhiêu độ
uint16_t s4TickMs  = 15;           // Chu kỳ “nhích” (ms) → nhỏ = mượt/nhanh hơn

// ===== Cấu hình PWM cho analogWrite(*) (ESP32 core 3.x) =====
// Lưu ý: core 3.x hỗ trợ analogWrite(pin, duty), analogWriteResolution(pin,bits),
//        analogWriteFrequency(pin,hz) với dạng THAM SỐ CÓ "pin".
const int PWM_FREQ_HZ   = 20000; // 20 kHz (cao để động cơ êm, ít rít)
const int PWM_RES_BITS  = 10;    // 10-bit (0..1023)

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println(F("\n=== Robot BT + 4 Servo — ESP32 3.3.3 (analogWrite-only) ==="));
  Serial.println(F("Keys: F,B,L,R,S | I,G,J,H | 0..9 | W/w | V/v | X/x"));

  initPins();  // Cấu hình mode cho các GPIO

  // *** BƯỚC 1: CẤP KÊNH (TIMER) CHO SERVO TRƯỚC ***
  // Điều này giúp tránh xung đột kênh PWM khi sau đó dùng analogWrite cho motor.
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Dự phòng: nếu sketch trước đã từng gán LEDC vào các chân servo,
  // ta gọi ledcDetach(pin) để tháo kênh đó ra, tránh tranh chấp.
  for (int i = 0; i < NUM_SERVOS; ++i) {
    ledcDetach(SERVO_PINS[i]);                  // hàm có sẵn trong core 3.x
    servos[i].setPeriodHertz(50);               // servo 50 Hz tiêu chuẩn
    servos[i].attach(SERVO_PINS[i], 500, 2500); // 0..180° ↔ 500..2500 µs
    servos[i].write(servoAngle[i]);             // đưa về góc mặc định
    delay(3);                                   // nhỏ thôi để êm
  }
  printServoMap();

  // *** BƯỚC 2: KHỞI TẠO PWM CHO MOTOR (analogWrite) ***
  pwmInit();

  // Đặt tốc độ mặc định & dừng an toàn
  speedLevel = 5;
  baseDuty   = dutyFromLevel(speedLevel);
  stopAll();

  // Bluetooth: "DHTD18A2HN_CAR" là tên hiển thị khi bạn dò thiết bị
  if (!SerialBT.begin("DHTD18A2HN_CAR")) {
    Serial.println(F("[ERR] Bluetooth start failed, restarting..."));
    delay(1200);
    esp_restart();
  }
  Serial.println(F("[OK] Bluetooth SPP ready."));
}

// ======================= LOOP =======================
// Vòng lặp chính:
// - Nhận lệnh BT và cập nhật hành vi
// - Fail-safe: nếu >500ms không nhận lệnh → dừng 1 lần
// - Chớp LED theo trạng thái kết nối
// - Nếu ở Servo-steer: nhích SV4 theo tick
// - Ghi servo định kỳ ~50 Hz để giữ/gia trễ mượt
void loop() {
  static uint32_t lastCmdMs  = millis();  // mốc thời gian lệnh cuối
  static bool     stoppedByTimeout = false; // để chỉ dừng 1 lần
  const uint32_t  now = millis();

  // ====== NHẬN LỆNH BLUETOOTH ======
  while (SerialBT.available()) {
    char c = (char)SerialBT.read();
    Serial.print(F("[BT] ")); Serial.println(c);

    // ---- Chuyển chế độ X/x ----
    if (c == 'X') {                 // Bật Servo-steer: L/R quay SV4
      servoSteerMode = true;
      s4Dir = 0;                    // dừng quay SV4 ngay khi vào chế độ
      stopAll();                    // an toàn, tránh xe đang chạy
      Serial.println(F("[Mode] Servo-steer ON (L/R quay SV4)"));
      lastCmdMs = now;
      continue;
    }
    if (c == 'x') {                 // Về lái thường
      servoSteerMode = false;
      s4Dir = 0;                    // dừng quay SV4
      stopAll();
      Serial.println(F("[Mode] Drive-steer ON (L/R quay xe)"));
      lastCmdMs = now;
      continue;
    }

    // ---- Nếu đang ở chế độ Servo-steer ----
    if (servoSteerMode) {
      switch (c) {
        case 'L': s4Dir = -1; break;            // L: giảm góc SV4 từ từ
        case 'R': s4Dir = +1; break;            // R: tăng góc SV4 từ từ
        case 'S': s4Dir =  0; stopAll(); break; // S: dừng quay SV4 + dừng xe

        // Cho phép vẫn kết hợp di chuyển (tùy nhu cầu):
        case 'F': forward();        break;
        case 'B': backward();       break;
        case 'I': forward_right();  break;
        case 'G': forward_left();   break;
        case 'J': backward_right(); break;
        case 'H': backward_left();  break;

        // Servo nhanh cho "tay" và "kẹp"
        case 'W': vuontay();  forceWriteAll(); break;
        case 'w': thutay();   forceWriteAll(); break;
        case 'V': kep();      forceWriteAll(); break;
        case 'v': thakep();   forceWriteAll(); break;

        // Tốc độ 0..9
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
          speedLevel = (uint8_t)(c - '0');
          baseDuty   = dutyFromLevel(speedLevel);
          applyDrive();                      // áp dụng ngay duty mới
        } break;

        default: break;
      }
      stoppedByTimeout = false;
      lastCmdMs = now;
      continue; // đã xử lý xong trong chế độ Servo-steer
    }

    // ---- Chế độ lái thường (Drive-steer) ----
    switch (c) {
      // Lái cơ bản
      case 'F': forward();        stoppedByTimeout=false; break;
      case 'B': backward();       stoppedByTimeout=false; break;
      case 'L': left_turn();      stoppedByTimeout=false; break;
      case 'R': right_turn();     stoppedByTimeout=false; break;
      case 'S': stopAll();        stoppedByTimeout=false; break;

      // Lái chéo (giảm 1 bên để chéo)
      case 'I': forward_right();  stoppedByTimeout=false; break;
      case 'G': forward_left();   stoppedByTimeout=false; break;
      case 'J': backward_right(); stoppedByTimeout=false; break;
      case 'H': backward_left();  stoppedByTimeout=false; break;

      // Servo nhanh
      case 'W': vuontay();  forceWriteAll(); break;
      case 'w': thutay();   forceWriteAll(); break;
      case 'V': kep();      forceWriteAll(); break;
      case 'v': thakep();   forceWriteAll(); break;

      // Thay đổi tốc độ tức thì
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        speedLevel = (uint8_t)(c - '0');
        baseDuty   = dutyFromLevel(speedLevel);
        applyDrive();
        stoppedByTimeout=false;
      } break;

      default: break;
    }
    lastCmdMs = now;
  }

  // ====== FAIL-SAFE: Không có lệnh > 500 ms thì dừng 1 lần ======
  if (now - lastCmdMs > 500) {
    if (!stoppedByTimeout) { stopAll(); stoppedByTimeout = true; }
  }

  // ====== LED báo có client Bluetooth ======
  digitalWrite(LED_BT, SerialBT.hasClient() ? HIGH : LOW);

  // ====== Nhích SV4 từ từ khi đang Servo-steer ======
  static uint32_t lastS4Tick = 0;
  if (servoSteerMode && s4Dir != 0 && (now - lastS4Tick) >= s4TickMs) {
    int newAng = servoAngle[3] + s4Dir * (int)s4StepDeg;   // SV4 = index 3
    newAng = constrain(newAng, 0, 180);
    if (newAng != servoAngle[3]) {
      servoAngle[3] = newAng;
      servos[3].write(servoAngle[3]);  // ghi riêng SV4 ngay lập tức
    }
    lastS4Tick = now;
  }

  // ====== Ghi lại toàn bộ servo theo chu kỳ ~50 Hz ======
  // Dù ta đã ghi riêng SV4, vẫn giữ nhịp chung để các servo khác ổn định.
  static uint32_t lastServo = 0;
  if (now - lastServo >= 20) {   // 20 ms ≈ 50 Hz
    forceWriteAll();
    lastServo = now;
  }

  // Nhịp thở CPU tí xíu cho êm
  delay(1);
}
