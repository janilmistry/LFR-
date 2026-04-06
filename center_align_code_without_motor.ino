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

bool whiteOK = false;
bool blackOK = false;

/* ---------- MOTOR FUNCTIONS ---------- */
void beep(int ms) {
  digitalWrite(BUZZER, HIGH);
  delay(ms);
  digitalWrite(BUZZER, LOW);
}

void rotateLeft(int speed) {
  // Left motor backward
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, speed);

  // Right motor forward
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMB, speed);
}

void rotateRight(int speed) {
  // Left motor forward
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speed);

  // Right motor backward
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, speed);
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

/* ---------- SETUP ---------- */
void setup() {
  Serial.begin(9600);

  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(irPins[i], INPUT);
  }

  pinMode(BTN_WHITE, INPUT_PULLUP);
  pinMode(BTN_BLACK, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);

  beep(200);
  Serial.println("CENTERING MODE (FIXED LOGIC)");
  Serial.println("Goal: BLACK under A11");
}

/* ---------- LOOP ---------- */
void loop() {

  /* ---- READ RAW ---- */
  for (int i = 0; i < NUM_SENSORS; i++) {
    irRaw[i] = analogRead(irPins[i]);
  }

  /* ---- WHITE CAL ---- */
  if (digitalRead(BTN_WHITE) == LOW) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      irWhite[i] = irRaw[i];
    }
    whiteOK = true;
    beep(100);
    delay(500);
  }

  /* ---- BLACK CAL ---- */
  if (digitalRead(BTN_BLACK) == LOW) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      irBlack[i] = irRaw[i];
    }
    blackOK = true;
    beep(300);
    delay(500);
  }

  if (!(whiteOK && blackOK)) {
    stopMotors();
    Serial.println("Waiting for calibration...");
    delay(300);
    return;
  }

  /* ---- NORMALIZE ---- */
  for (int i = 0; i < NUM_SENSORS; i++) {
    irNorm[i] = (float)(irRaw[i] - irWhite[i]) /
                (float)(irBlack[i] - irWhite[i]);
    irNorm[i] = constrain(irNorm[i], 0.0, 1.0);
  }

  /* ---- DEBUG PRINT ---- */
  Serial.print("NORM: ");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(irNorm[i], 2);
    Serial.print(" ");
  }

  /* ---- CENTERING DECISION ---- */

  // If center sensor sees black → STOP
  if (irNorm[3] > 0.6) {
    stopMotors();
    Serial.println(" | CENTERED (A11 BLACK)");
  }
  else {
    // Sum black strength on left and right
    float leftBlack  = irNorm[0] + irNorm[1] + irNorm[2];
    float rightBlack = irNorm[4] + irNorm[5] + irNorm[6];

    if (leftBlack > rightBlack) {
      // Line is on LEFT → rotate LEFT to bring it under A11
      rotateLeft(120);
      Serial.println(" | LINE LEFT → ROTATE LEFT");
    }
    else {
      // Line is on RIGHT → rotate RIGHT
      rotateRight(120);
      Serial.println(" | LINE RIGHT → ROTATE RIGHT");
    }
  }

  delay(40);
}
