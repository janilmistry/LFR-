#include <Encoder.h>

/* ---------- PINS ---------- */
#define STBY 28
#define AIN1 25 
#define AIN2 24
#define PWMA 5
#define BIN1 27   
#define BIN2 26
#define PWMB 6
#define BUTTON_PIN 31
#define BUZZER_PIN 7

Encoder leftEnc(2, 23);
Encoder rightEnc(3, 22);

/* ---------- SENSORS ---------- */
const int numSensors = 6;
int irPins[6] = {A0, A1, A2, A3, A4, A5};
int irMin[6], irMax[6], threshold[6]; 

/* ---------- TUNING ---------- */
int baseSpeed = 160;   
float Kp = 0.032;      
float Kd = 1.8;        
int lastError = 0;
float filteredD = 0;   
float alpha = 0.08; 

// --- BUMP DETECTION VARIABLES ---
long lastEncoderCheck = 0;
long lastLeftPos = 0;
int bumpBoost = 0;

void setup() {
  pinMode(STBY, OUTPUT);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  digitalWrite(STBY, HIGH);
  Serial.begin(9600);

  while(digitalRead(BUTTON_PIN) == HIGH); 
  beep(100); delay(500);
  
  autoCalibrate(3, 80); 
  for(int i=0; i<6; i++) threshold[i] = (irMin[i] + irMax[i]) / 2;
  
  leftEnc.write(0); rightEnc.write(0);
  findLineCenter();
  beep(500);
}

void loop() {
  int sensorValues[6];
  bool sensorState[6];
  int position = getStablePosition(sensorValues, sensorState);
  
  // --- BUMP LOGIC: SENSOR LOSS PROTECTION ---
  // If we lose the line for a tiny fraction of a second (hitting a bump),
  // don't enter recovery immediately. Keep driving straight for 50ms.
  static unsigned long lineLossTime = 0;
  if (position == -1) { 
    if (lineLossTime == 0) lineLossTime = millis();
    if (millis() - lineLossTime < 50) { 
        driveEncoderRail(baseSpeed + 20, 0); // "Push" through the bump
        return;
    }
    recoverLine(); 
    return;         
  } else {
    lineLossTime = 0; // Reset once line is found
  }

  int error = position - 2500;
  
  // --- BUMP LOGIC: TORQUE BOOST ---
  // Check if encoders show we are slowing down (hitting an obstacle)
  if (millis() - lastEncoderCheck > 20) {
    long currentLeft = leftEnc.read();
    if (abs(currentLeft - lastLeftPos) < 5) { // We are stuck/slowing on a bump
        bumpBoost = 30; // Add extra PWM power
    } else {
        bumpBoost = 0;
    }
    lastLeftPos = currentLeft;
    lastEncoderCheck = millis();
  }

  // --- STANDARD STABLE DRIVE ---
  if (abs(error) < 150) {
    driveEncoderRail(baseSpeed + bumpBoost, error * 0.05);
    lastError = 0;
    filteredD = 0;
  } else {
    float rawD = error - lastError;
    filteredD = (alpha * rawD) + ((1.0 - alpha) * filteredD);
    int steerCorrection = (error * Kp) + (filteredD * Kd);
    driveMotors(baseSpeed + bumpBoost + steerCorrection, baseSpeed + bumpBoost - steerCorrection);
    lastError = error;
  }
}

/* ---------- ENCODER RAIL SYSTEM ---------- */

void driveEncoderRail(int speed, float bias) {
  long diff = leftEnc.read() - rightEnc.read();
  int correction = (diff * 5.0) + (int)bias;
  driveMotors(speed, speed + correction);
}

void driveMotors(int leftPWM, int rightPWM) {
  leftPWM = constrain(leftPWM, -255, 255);
  rightPWM = constrain(rightPWM, -255, 255);
  
  if (abs(leftPWM) < 25 && leftPWM != 0) leftPWM = (leftPWM > 0) ? 25 : -25;
  if (abs(rightPWM) < 25 && rightPWM != 0) rightPWM = (rightPWM > 0) ? 25 : -25;

  digitalWrite(AIN1, leftPWM >= 0 ? HIGH : LOW);
  digitalWrite(AIN2, leftPWM >= 0 ? LOW : HIGH);
  digitalWrite(BIN1, rightPWM >= 0 ? HIGH : LOW);
  digitalWrite(BIN2, rightPWM >= 0 ? LOW : HIGH);
  analogWrite(PWMA, abs(leftPWM));
  analogWrite(PWMB, abs(rightPWM));
}

/* ---------- SENSOR LOGIC ---------- */

int getStablePosition(int* values, bool* states) {
  long avg = 0; int count = 0; bool any = false;
  for (int i = 0; i < numSensors; i++) {
    int raw = analogRead(irPins[i]);
    if (raw > threshold[i]) {
      states[i] = true; any = true;
      avg += (i * 1000); count++;
    } else states[i] = false;
  }
  return any ? (int)(avg / count) : -1;
}

// ... (Calibration, recoverLine, findLineCenter remain the same)
void autoCalibrate(int cycles, int speed) {
  for (int i = 0; i < cycles; i++) {
    driveMotors(speed, -speed); sampleSensors(400);
    driveMotors(-speed, speed); sampleSensors(800);
    driveMotors(speed, -speed); sampleSensors(400);
  }
  driveMotors(0, 0);
}

void sampleSensors(int duration) {
  unsigned long start = millis();
  while (millis() - start < duration) {
    for (int i = 0; i < numSensors; i++) {
      int val = analogRead(irPins[i]);
      if (val < irMin[i]) irMin[i] = val;
      if (val > irMax[i]) irMax[i] = val;
    }
  }
}

void recoverLine() {
  int v[6]; bool s[6];
  while (getStablePosition(v, s) == -1) {
    if (lastError > 0) driveMotors(110, -110); 
    else driveMotors(-110, 110);
  }
}

void findLineCenter() {
  int v[6]; bool s[6];
  while (true) {
    int pos = getStablePosition(v, s);
    if (pos > 2450 && pos < 2550) break; 
    driveMotors(70, -70); 
  }
  driveMotors(0, 0);
  delay(200);
}

void beep(int ms) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(ms);
  digitalWrite(BUZZER_PIN, LOW);
}
