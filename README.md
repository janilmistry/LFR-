# LFR with Solenoid Striker & Custom IR Array

An advanced Autonomous Line Follower Robot (LFR) designed for high-precision tasks. This robot features a custom-built IR sensor array for navigation and a solenoid-based mechanism for striking/moving objects.

## 🚀 Key Features
* **Custom IR Array:** 7-sensor configuration with real-time normalization logic.
* **Precision Centering:** Auto-aligns using a center-weighted algorithm before triggering tasks.
* **Feedback System:** * **OLED Display:** Real-time status and calibration data.
    * **RGB LED:** Visual mode indicators (Calibration/Ready/Action).
    * **Buzzer:** Audio alerts for sensor triggers and calibration completion.
* **Solenoid Actuator:** DIY high-power solenoid for task execution.

## 🛠️ Hardware Specifications
* **Microcontroller:** Arduino Mega 2560
* **Motor Driver:** TB6612FNG (Dual Channel)
* **Motors:** N20 Micro Gear Motors
* **Sensors:** 7x Custom IR Sensors (Analog)
* **Display:** SSD1306 OLED (I2C)
* **Indication:** Common Cathode RGB LED & Active Buzzer

## 📂 Repository Structure
* `LFR_Solenoid_Main.ino`: The primary control logic including sensor normalization and motor movement.
* `README.md`: Project documentation (this file).

## 🔧 Calibration Guide
1. Power the robot and wait for the **OLED** to show "WAITING CALIB".
2. Place all sensors on a **White** surface and press the **White Button**.
3. Place all sensors on a **Black** surface and press the **Black Button**.
4. The **RGB LED** will turn green when the robot is ready to follow the line.

---
*Developed for Robofest Gujarat 5.0 | Government Polytechnic Ahmedabad*
