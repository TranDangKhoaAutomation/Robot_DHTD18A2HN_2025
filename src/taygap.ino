static inline void setServo1b(uint8_t s1b, int angle) {
  // s1b dùng chỉ số 1-based cho dễ nhớ: 1..NUM_SERVOS
  if (s1b < 1 || s1b > NUM_SERVOS) return;
  servoAngle[s1b - 1] = constrain(angle, 0, 180);
}

static void vuontay() {  // Ví dụ vươn tay: bạn tuỳ chỉnh góc thực tế theo cơ khí
  setServo1b(2, 50);
  setServo1b(3, 100);
}

static void thutay() {   // Thu tay về
  setServo1b(2, 40);
  setServo1b(3, 50);
}

static void kep() {      // Kẹp (S1)
  setServo1b(1, 65);
}

static void thakep() {   // Thả kẹp (S1)
  setServo1b(1, 0);
}

// Ghi đồng loạt góc đang lưu trong servoAngle[]
static void forceWriteAll() {
  for (int i = 0; i < NUM_SERVOS; ++i) {
    servos[i].write(servoAngle[i]);
  }
}

// In map chân servo ra Serial để dễ kiểm tra khi khởi động
static void printServoMap() {
  Serial.print  (F("Servo map (1-based): "));
  Serial.printf ("S1->GPIO%d, ", SERVO_PINS[0]);
  Serial.printf ("S2->GPIO%d, ", SERVO_PINS[1]);
  Serial.printf ("S3->GPIO%d, ", SERVO_PINS[2]);
  Serial.printf ("S4->GPIO%d\n", SERVO_PINS[3]);
}
