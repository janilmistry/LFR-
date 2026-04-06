#define NUM_SENSORS 7

/* ---------- IR ---------- */
int irPins[NUM_SENSORS] = { A8, A9, A10, A11, A12, A13, A14 };
int irRaw[NUM_SENSORS];
int irWhite[NUM_SENSORS];
int irBlack[NUM_SENSORS];
float irNorm[NUM_SENSORS];

/* ---------- BUTTON & BUZZER ---------- */
#define BTN_WHITE 24
#define BTN_BLACK 26
#define BUZZER    25

/* ---------- TB6612FNG ---------- */
#define AIN1 4
#define AIN2 5
#define PWMA 3
#define BIN1 6
#define BIN2 7
#define PWMB 9
#define STBY 8

/* ---------- SPEED ---------- */
int baseSpeed = 200;
int turnSpeed = 140;

/* ---------- LINE MEMORY ---------- */
int lastLineSide = 0;

/* ---------- BLACK BOX ---------- */
byte sensorBits = 0;
bool boxStopped = false;
bool stopCandidate = false;

/* ---------- STATES ---------- */
enum RobotState {
  CALIBRATION,
  CENTERING,
  FOLLOWING,
  LOST_LINE
};

RobotState state = CALIBRATION;

/* ---------- MOTOR ---------- */
void beep(int ms) {
  digitalWrite(BUZZER, HIGH);
  delay(ms);
  digitalWrite(BUZZER, LOW);
}

void rotateLeft(int sp) {
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMA, sp); analogWrite(PWMB, sp);
}

void rotateRight(int sp) {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
  analogWrite(PWMA, sp); analogWrite(PWMB, sp);
}

void forward(int l, int r) {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMA, l); analogWrite(PWMB, r);
}

bool isCenterDominant() {
  float center = irNorm[2] + irNorm[3] + irNorm[4];
  float edges  = irNorm[0] + irNorm[6];

  return (center > 2.0 && edges > 1.2);
}


void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

/* ---------- SENSOR ---------- */
void readSensorBits() {
  sensorBits = 0;
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (irNorm[i] > 0.7) sensorBits |= (1 << i); // BLACK
  }
}

/* ---------- BLACK BOX STOP (OPTIMIZED) ---------- */
void checkBlackBoxStop() {

  if (boxStopped) return;

  // Step 1: possible stop detected
  if (!stopCandidate && sensorBits == 127) {

    stopCandidate = true;

    // Move forward ≈ 3 cm
    forward(120, 120);
    delay(80);
    stopMotors();
    delay(20);

    return;   // wait for next loop
  }

  // Step 2: confirm after movement
  if (stopCandidate) {

    readSensorBits();

    // 🔑 FINAL STRONG CONDITION
    if (sensorBits == 127 && isCenterDominant()) {

      // ✅ REAL BLACK BOX
      boxStopped = true;
      stopMotors();
      beep(600);

      while (1);   // FINAL STOP
    }
    else {
      // ❌ T / CROSS / THICK LINE
      stopCandidate = false;
    }
  }
}

/* ---------- SETUP ---------- */
void setup() {
  Serial.begin(9600);

  for (int i = 0; i < NUM_SENSORS; i++) pinMode(irPins[i], INPUT);
  pinMode(BTN_WHITE, INPUT_PULLUP);
  pinMode(BTN_BLACK, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);
  beep(200);
}

/* ---------- LOOP ---------- */
void loop() {

  /* ---- RAW READ ---- */
  for (int i = 0; i < NUM_SENSORS; i++)
    irRaw[i] = analogRead(irPins[i]);

  /* ---- CALIBRATION ---- */
  if (state == CALIBRATION) {

    if (digitalRead(BTN_WHITE) == LOW) {
      for (int i = 0; i < NUM_SENSORS; i++) irWhite[i] = irRaw[i];
      beep(100); delay(400);
    }

    if (digitalRead(BTN_BLACK) == LOW) {
      for (int i = 0; i < NUM_SENSORS; i++) irBlack[i] = irRaw[i];
      beep(300); delay(400);
      state = CENTERING;
    }

    stopMotors();
    return;
  }

  /* ---- NORMALIZE ---- */
  for (int i = 0; i < NUM_SENSORS; i++) {
    irNorm[i] = (float)(irRaw[i] - irWhite[i]) /
                (float)(irBlack[i] - irWhite[i]);
    irNorm[i] = constrain(irNorm[i], 0.0, 1.0);
  }

  readSensorBits();

  float centerBlack = irNorm[3];
  float leftBlack   = irNorm[0] + irNorm[1] + irNorm[2];
  float rightBlack  = irNorm[4] + irNorm[5] + irNorm[6];

  /* ---- CENTERING ---- */
  if (state == CENTERING) {
    if (centerBlack > 0.6) {
      stopMotors();
      state = FOLLOWING;
      beep(200);
    } else if (leftBlack > rightBlack) rotateLeft(turnSpeed);
    else rotateRight(turnSpeed);
  }

  /* ---- FOLLOWING ---- */
  else if (state == FOLLOWING) {

    checkBlackBoxStop();   // 🔴 STOP LOGIC FIRST

    if ((leftBlack + centerBlack + rightBlack) < 0.3) {
      stopMotors();
      state = LOST_LINE;
      return;
    }

    lastLineSide = (leftBlack > rightBlack) ? -1 : 1;

    float error =
      (-3 * irNorm[0]) + (-2 * irNorm[1]) + (-1 * irNorm[2]) +
      ( 0 * irNorm[3]) +
      ( 1 * irNorm[4]) + ( 2 * irNorm[5]) + ( 3 * irNorm[6]);

    int l = baseSpeed - error * 40;
    int r = baseSpeed + error * 40;

    forward(constrain(l,0,255), constrain(r,0,255));
  }

  /* ---- LOST LINE ---- */
  else if (state == LOST_LINE) {
    if (centerBlack > 0.4) {
      state = FOLLOWING;
      return;
    }
    if (lastLineSide < 0) rotateLeft(turnSpeed);
    else rotateRight(turnSpeed);
  }

  delay(15);
}
