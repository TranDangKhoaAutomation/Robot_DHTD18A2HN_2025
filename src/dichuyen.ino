// Cấu hình mode cho các GPIO
static void initPins() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(LED_BT, OUTPUT);
  digitalWrite(LED_BT, LOW);
}

// Khởi tạo PWM cho ENA/ENB (analogWrite API core 3.x)
static void pwmInit() {
  // Đặt độ phân giải CHO TỪNG PIN
  analogWriteResolution(ENA, PWM_RES_BITS);
  analogWriteResolution(ENB, PWM_RES_BITS);

  // Đặt tần số CHO TỪNG PIN
  analogWriteFrequency(ENA, PWM_FREQ_HZ);
  analogWriteFrequency(ENB, PWM_FREQ_HZ);

  // Khởi tạo duty = 0
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// Quy đổi speedLevel (0..9) → duty (0..1023)
static inline uint16_t dutyFromLevel(uint8_t lv) {
  if (lv > 9) lv = 9;
  return (uint16_t)map((int)lv, 0, 9, 0, 1023);
}

// Áp dụng điều khiển lái: set chiều & duty 2 kênh
static void applyDrive() {
  // Scale theo kA/kB để tạo chéo (ví dụ kA=40%, kB=100%)
  dutyA = (uint16_t)((uint32_t)baseDuty * kA / 100);
  dutyB = (uint16_t)((uint32_t)baseDuty * kB / 100);

  // Chiều kênh A
  if (dirA > 0)      { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
  else if (dirA < 0) { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  else               { digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  } // phanh thả

  // Chiều kênh B
  if (dirB > 0)      { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }
  else if (dirB < 0) { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
  else               { digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);  }

  // Ghi duty (10-bit). Nếu dir=0 thì duty=0 để đảm bảo dừng.
  analogWrite(ENA, (dirA == 0) ? 0 : dutyA);
  analogWrite(ENB, (dirB == 0) ? 0 : dutyB);
}

// --------- Các hành vi lái cơ bản ----------
static void forward()       { dirA = +1; dirB = +1; kA = 100; kB = 100; applyDrive(); }
static void backward()      { dirA = -1; dirB = -1; kA = 100; kB = 100; applyDrive(); }
static void left_turn()     { dirA = -1; dirB = +1; kA = 100; kB = 100; applyDrive(); }
static void right_turn()    { dirA = +1; dirB = -1; kA = 100; kB = 100; applyDrive(); }
static void stopAll()       { dirA =  0; dirB =  0; kA = 100; kB = 100; applyDrive(); }

// --------- Lái chéo (giảm 1 bên) ----------
static void forward_right() { dirA = +1; dirB = +1; kA = 100; kB =  40; applyDrive(); } // I
static void forward_left()  { dirA = +1; dirB = +1; kA =  40; kB = 100; applyDrive(); } // G
static void backward_right(){ dirA = -1; dirB = -1; kA = 100; kB =  40; applyDrive(); } // J
static void backward_left() { dirA = -1; dirB = -1; kA =  40; kB = 100; applyDrive(); } // H
