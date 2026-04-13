# Arduino Servo + IMU Firmware (Arduino Uno)

This firmware runs on an **Arduino Uno** and provides a combined interface for **servo control** and **MPU9250 IMU sensing**. It exposes a simple **USB serial protocol** so an external system (Raspberry Pi) can send servo commands while receiving real-time IMU data. The code handles low-level hardware control, sensor fusion (roll, pitch, yaw), and continuous telemetry streaming.

The Arduino is connected to the Raspberry Pi via **USB serial (115200 baud)**.

---

## Features

- Control of **4 servos**
  - Right motor tilt
  - Left motor tilt
  - Camera rotation (pan)
  - Camera pitch (tilt)
- Reads **MPU9250 IMU (Accel + Gyro + Magnetometer)**
- Computes **roll, pitch, yaw** using a complementary filter
- Performs **magnetometer calibration at startup**
- Streams IMU data at **20 Hz**
- Supports a simple **serial command protocol**
- Uses **safe startup servo positions**

---

## Hardware Configuration

### Servos
| Function              | Pin |
|----------------------|-----|
| Right Motor Tilt     | 3   |
| Left Motor Tilt      | 5   |
| Camera Rotation      | 6   |
| Camera Pitch         | 9   |

### IMU
- Sensor: **MPU9250 + AK8963 magnetometer**
- Interface: **I2C (Wire library)**
- Addresses:
  - MPU9250: `0x68`
  - Magnetometer: `0x0C`

---

## Servo Behaviour

- Default startup positions:
  - Right motor: `90°`
  - Left motor: `90°`
  - Camera rotation: `90°`
  - Camera pitch: `45°`

- Angle limits:
  - General servos: `0° – 180°`
  - Camera pitch: `0° – 90°` (hardware constrained)

- Custom calibrated pulse widths:
  ```cpp
  servoRightMotor.attach(pin, 450, 2600);
  servoLeftMotor.attach(pin, 500, 2550);
  servoCamRot.attach(pin, 500, 2500);
  servoCamPitch.attach(pin, 500, 2500);
