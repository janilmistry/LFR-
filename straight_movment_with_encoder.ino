#include <Encoder.h>

// Motor Driver Pins
#define STBY 28
#define AIN1 25 
#define AIN2 24
#define PWMA 5
#define BIN1 27 
#define BIN2 26
#define PWMB 6

// Encoder Pins
Encoder leftEnc(2, 23);
Encoder rightEnc(3, 22);

// Constants
const int TICKS_PER_ROTATION = 1388;
const int BASE_PWM = 150; // Standard speed (0-255)

void setup() {
  pinMode(STBY, OUTPUT);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  
  digitalWrite(STBY, HIGH);
  leftEnc.write(0);
  rightEnc.write(0);
  
  // Start Forward
  moveForward();
}

void loop() {
  // Target: Move exactly 5 rotations and then stop
  long targetTicks = 5 * TICKS_PER_ROTATION;

  if (abs(leftEnc.read()) < targetTicks) {
    keepStraight();
  } else {
    stopMotors();
  }
}

void keepStraight() {
  long leftPos = abs(leftEnc.read());
  long rightPos = abs(rightEnc.read());

  // How much is the right motor lagging or leading?
  long error = leftPos - rightPos;
  
  // Adjust Right Motor (PWMB)
  // If error is positive, right is slow -> increase speed
  // Kp (1.5) is the correction strength
  int adjustedRightPWM = BASE_PWM + (error * 1.5);
  
  // Safety limits
  adjustedRightPWM = constrain(adjustedRightPWM, 0, 255);

  analogWrite(PWMA, BASE_PWM);
  analogWrite(PWMB, adjustedRightPWM);
}

void moveForward() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(STBY, LOW);
}