#include <Wire.h>
#include <math.h>
#include <Servo.h>

// =========================
// Servo pins
// =========================
#define SERVO_RIGHT_MOTOR_PIN 3
#define SERVO_LEFT_MOTOR_PIN 5
#define SERVO_CAM_ROT_PIN 6
#define SERVO_CAM_PITCH_PIN 9

Servo servoRightMotor;
Servo servoLeftMotor;
Servo servoCamRot;
Servo servoCamPitch;

// =========================
// MPU9250 / AK8963
// =========================
#define MPU_ADDR     0x68
#define AK8963_ADDR  0x0C

// MPU registers
#define PWR_MGMT_1    0x6B
#define ACCEL_CONFIG  0x1C
#define GYRO_CONFIG   0x1B
#define ACCEL_XOUT_H  0x3B
#define INT_PIN_CFG   0x37

// Magnetometer registers
#define AK8963_ST1    0x02
#define AK8963_XOUT_L 0x03
#define AK8963_CNTL1  0x0A

int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;
int16_t magX, magY, magZ;

const float ACCEL_SCALE = 16384.0;   // +-2g
const float GYRO_SCALE  = 131.0;     // +-250 deg/s

float roll = 0.0;
float pitch = 0.0;
float yaw = 0.0;

unsigned long prevTime = 0;
const float alpha = 0.98;

// Magnetometer calibration offsets
float magOffsetX = 0;
float magOffsetY = 0;
float magOffsetZ = 0;

// Optional scale correction
float magScaleX = 1.0;
float magScaleY = 1.0;
float magScaleZ = 1.0;

// =========================
// Servo command state
// =========================
int rightMotorAngle = 90;
int leftMotorAngle = 90;
int camRotAngle = 90;
int camPitchAngle = 45;

// Serial input buffer
String inputLine = "";

// IMU streaming rate
unsigned long lastImuSend = 0;
const unsigned long imuPeriodMs = 50;   // 20 Hz

// =========================
// I2C helpers
// =========================
void writeByte(uint8_t address, uint8_t reg, uint8_t data) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t readByte(uint8_t address, uint8_t reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(address, (uint8_t)1);

  if (Wire.available()) return Wire.read();
  return 0xFF;
}

void readBytes(uint8_t address, uint8_t reg, uint8_t count, uint8_t *dest) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(address, count);

  uint8_t i = 0;
  while (Wire.available() && i < count) {
    dest[i++] = Wire.read();
  }
}

// =========================
// IMU setup
// =========================
void setupMPU() {
  writeByte(MPU_ADDR, PWR_MGMT_1, 0x00);
  delay(100);

  writeByte(MPU_ADDR, ACCEL_CONFIG, 0x00); // +-2g
  writeByte(MPU_ADDR, GYRO_CONFIG, 0x00);  // +-250 dps
  delay(100);
}

void setupMag() {
  writeByte(MPU_ADDR, INT_PIN_CFG, 0x02); // bypass mode
  delay(10);

  writeByte(AK8963_ADDR, AK8963_CNTL1, 0x00);
  delay(10);

  writeByte(AK8963_ADDR, AK8963_CNTL1, 0x16); // continuous mode 2, 16-bit
  delay(10);
}

// =========================
// IMU read functions
// =========================
void readAccelGyro() {
  uint8_t rawData[14];
  readBytes(MPU_ADDR, ACCEL_XOUT_H, 14, rawData);

  accelX = ((int16_t)rawData[0] << 8) | rawData[1];
  accelY = ((int16_t)rawData[2] << 8) | rawData[3];
  accelZ = ((int16_t)rawData[4] << 8) | rawData[5];

  gyroX = ((int16_t)rawData[8] << 8) | rawData[9];
  gyroY = ((int16_t)rawData[10] << 8) | rawData[11];
  gyroZ = ((int16_t)rawData[12] << 8) | rawData[13];
}

bool readMag() {
  uint8_t st1 = readByte(AK8963_ADDR, AK8963_ST1);
  if (!(st1 & 0x01)) return false;

  uint8_t magData[7];
  readBytes(AK8963_ADDR, AK8963_XOUT_L, 7, magData);

  magX = ((int16_t)magData[1] << 8) | magData[0];
  magY = ((int16_t)magData[3] << 8) | magData[2];
  magZ = ((int16_t)magData[5] << 8) | magData[4];

  return true;
}

void calibrateMagnetometer() {
  Serial.println("INFO,Mag calibration starting");
  Serial.println("INFO,Move IMU in all directions for 15 seconds");

  int16_t minX = 32767, minY = 32767, minZ = 32767;
  int16_t maxX = -32768, maxY = -32768, maxZ = -32768;

  unsigned long startTime = millis();

  while (millis() - startTime < 15000) {
    if (readMag()) {
      if (magX < minX) minX = magX;
      if (magY < minY) minY = magY;
      if (magZ < minZ) minZ = magZ;

      if (magX > maxX) maxX = magX;
      if (magY > maxY) maxY = magY;
      if (magZ > maxZ) maxZ = magZ;
    }

    delay(50);
  }

  magOffsetX = (maxX + minX) / 2.0;
  magOffsetY = (maxY + minY) / 2.0;
  magOffsetZ = (maxZ + minZ) / 2.0;

  float rangeX = (maxX - minX) / 2.0;
  float rangeY = (maxY - minY) / 2.0;
  float rangeZ = (maxZ - minZ) / 2.0;

  float avgRange = (rangeX + rangeY + rangeZ) / 3.0;

  if (rangeX != 0) magScaleX = avgRange / rangeX;
  if (rangeY != 0) magScaleY = avgRange / rangeY;
  if (rangeZ != 0) magScaleZ = avgRange / rangeZ;

  Serial.print("INFO,Mag offsets,");
  Serial.print(magOffsetX); Serial.print(",");
  Serial.print(magOffsetY); Serial.print(",");
  Serial.println(magOffsetZ);

  Serial.print("INFO,Mag scales,");
  Serial.print(magScaleX, 3); Serial.print(",");
  Serial.print(magScaleY, 3); Serial.print(",");
  Serial.println(magScaleZ, 3);

  Serial.println("INFO,Calibration complete");
}

// =========================
// Angle helpers
// =========================
int clampAngle(int val, int minVal = 0, int maxVal = 180) {
  if (val < minVal) return minVal;
  if (val > maxVal) return maxVal;
  return val;
}

int clampCamPitch(int val) {
  if (val < 0) return 0;
  if (val > 90) return 90;
  return val;
}

void applyServoPositions() {
  servoRightMotor.write(rightMotorAngle);
  servoLeftMotor.write(leftMotorAngle);
  servoCamRot.write(camRotAngle);
  servoCamPitch.write(camPitchAngle);
}

void setDriveServos(int rightDeg, int leftDeg) {
  rightMotorAngle = clampAngle(rightDeg);
  leftMotorAngle = clampAngle(leftDeg);
  applyServoPositions();
}

void setCameraServos(int rotDeg, int pitchDeg) {
  camRotAngle = clampAngle(rotDeg);
  camPitchAngle = clampCamPitch(pitchDeg);
  applyServoPositions();
}

void setAllServos(int rightDeg, int leftDeg, int rotDeg, int pitchDeg) {
  rightMotorAngle = clampAngle(rightDeg);
  leftMotorAngle = clampAngle(leftDeg);
  camRotAngle = clampAngle(rotDeg);
  camPitchAngle = clampCamPitch(pitchDeg);
  applyServoPositions();
}

// =========================
// Serial command parser
// =========================
void processCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  if (line.startsWith("DRV,")) {
    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);

    if (secondComma > 0) {
      int s1 = line.substring(firstComma + 1, secondComma).toInt();
      int s2 = line.substring(secondComma + 1).toInt();
      setDriveServos(s1, s2);
      Serial.println("ACK,DRV");
    } else {
      Serial.println("ERR,BAD_DRV");
    }
    return;
  }

  if (line.startsWith("CAM,")) {
    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);

    if (secondComma > 0) {
      int rot = line.substring(firstComma + 1, secondComma).toInt();
      int pit = line.substring(secondComma + 1).toInt();
      setCameraServos(rot, pit);
      Serial.println("ACK,CAM");
    } else {
      Serial.println("ERR,BAD_CAM");
    }
    return;
  }

  if (line.startsWith("ALL,")) {
    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    int c3 = line.indexOf(',', c2 + 1);
    int c4 = line.indexOf(',', c3 + 1);

    if (c2 > 0 && c3 > 0 && c4 > 0) {
      int s1 = line.substring(c1 + 1, c2).toInt();
      int s2 = line.substring(c2 + 1, c3).toInt();
      int rot = line.substring(c3 + 1, c4).toInt();
      int pit = line.substring(c4 + 1).toInt();
      setAllServos(s1, s2, rot, pit);
      Serial.println("ACK,ALL");
    } else {
      Serial.println("ERR,BAD_ALL");
    }
    return;
  }

  if (line == "PING") {
    Serial.println("PONG");
    return;
  }

  Serial.println("ERR,UNKNOWN_CMD");
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {
      processCommand(inputLine);
      inputLine = "";
    } else if (c != '\r') {
      inputLine += c;
    }
  }
}

// =========================
// IMU update and output
// =========================
void updateIMU() {
  readAccelGyro();

  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0;
  prevTime = currentTime;

  if (dt <= 0) dt = 0.01;

  float ax = accelX / ACCEL_SCALE;
  float ay = accelY / ACCEL_SCALE;
  float az = accelZ / ACCEL_SCALE;

  float gx = gyroX / GYRO_SCALE;
  float gy = gyroY / GYRO_SCALE;
  float gz = gyroZ / GYRO_SCALE;

  float accelRoll  = atan2(ay, az) * 180.0 / PI;
  float accelPitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  roll  = alpha * (roll  + gx * dt) + (1.0 - alpha) * accelRoll;
  pitch = alpha * (pitch + gy * dt) + (1.0 - alpha) * accelPitch;

  if (readMag()) {
    float mx = (magX - magOffsetX) * magScaleX;
    float my = (magY - magOffsetY) * magScaleY;
    float mz = (magZ - magOffsetZ) * magScaleZ;

    float rollRad = roll * PI / 180.0;
    float pitchRad = pitch * PI / 180.0;

    float Xh = mx * cos(pitchRad) + mz * sin(pitchRad);
    float Yh = mx * sin(rollRad) * sin(pitchRad) + my * cos(rollRad) - mz * sin(rollRad) * cos(pitchRad);

    yaw = atan2(-Yh, Xh) * 180.0 / PI;
    if (yaw < 0) yaw += 360.0;
  }
}

void sendIMU() {
  float ax = accelX / ACCEL_SCALE;
  float ay = accelY / ACCEL_SCALE;
  float az = accelZ / ACCEL_SCALE;

  float gx = gyroX / GYRO_SCALE;
  float gy = gyroY / GYRO_SCALE;
  float gz = gyroZ / GYRO_SCALE;

  float mx = (magX - magOffsetX) * magScaleX;
  float my = (magY - magOffsetY) * magScaleY;
  float mz = (magZ - magOffsetZ) * magScaleZ;

  Serial.print("IMU,");
  Serial.print(ax, 3); Serial.print(",");
  Serial.print(ay, 3); Serial.print(",");
  Serial.print(az, 3); Serial.print(",");
  Serial.print(gx, 3); Serial.print(",");
  Serial.print(gy, 3); Serial.print(",");
  Serial.print(gz, 3); Serial.print(",");
  Serial.print(mx, 3); Serial.print(",");
  Serial.print(my, 3); Serial.print(",");
  Serial.print(mz, 3); Serial.print(",");
  Serial.print(roll, 2); Serial.print(",");
  Serial.print(pitch, 2); Serial.print(",");
  Serial.println(yaw, 2);
}

// =========================
// Setup / Loop
// =========================
void setup() {
  Wire.begin();
  Serial.begin(115200);
  delay(1000);

  // Final calibrated servo pulse widths
  servoRightMotor.attach(SERVO_RIGHT_MOTOR_PIN, 450, 2600);
  servoLeftMotor.attach(SERVO_LEFT_MOTOR_PIN, 500, 2550);
  servoCamRot.attach(SERVO_CAM_ROT_PIN, 500, 2500);
  servoCamPitch.attach(SERVO_CAM_PITCH_PIN, 500, 2500);

  // Safe startup positions
  rightMotorAngle = 90;
  leftMotorAngle = 90;
  camRotAngle = 90;
  camPitchAngle = 45;
  applyServoPositions();

  setupMPU();
  setupMag();
  calibrateMagnetometer();

  prevTime = millis();

  Serial.println("READY");
}

void loop() {
  readSerialCommands();

  unsigned long now = millis();
  if (now - lastImuSend >= imuPeriodMs) {
    lastImuSend = now;
    updateIMU();
    sendIMU();
  }
}