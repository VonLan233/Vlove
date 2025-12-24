#pragma once

#include <Arduino.h>
#include <Wire.h>

// MPU6050 registers
#define MPU6050_ADDR         0x68
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_SMPLRT_DIV   0x19
#define MPU6050_CONFIG       0x1A
#define MPU6050_GYRO_CONFIG  0x1B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H  0x43

// Quaternion structure
struct Quaternion {
    float w, x, y, z;

    Quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}

    void normalize() {
        float n = sqrt(w*w + x*x + y*y + z*z);
        if (n > 0) {
            w /= n; x /= n; y /= n; z /= n;
        }
    }
};

class IMU {
private:
    bool initialized = false;

    // Raw sensor data
    int16_t accelRaw[3];  // X, Y, Z
    int16_t gyroRaw[3];   // X, Y, Z

    // Calibration offsets
    int16_t gyroOffset[3] = {0, 0, 0};
    int16_t accelOffset[3] = {0, 0, 0};

    // Processed data
    float accel[3];  // g
    float gyro[3];   // deg/s

    // Orientation
    Quaternion quat;
    float yaw, pitch, roll;  // degrees

    // Timing
    unsigned long lastUpdate = 0;
    float dt = 0.01f;

    // Madgwick filter parameter
    float beta = 0.1f;

    void writeRegister(uint8_t reg, uint8_t value) {
        Wire.beginTransmission(MPU6050_ADDR);
        Wire.write(reg);
        Wire.write(value);
        Wire.endTransmission();
    }

    uint8_t readRegister(uint8_t reg) {
        Wire.beginTransmission(MPU6050_ADDR);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU6050_ADDR, (uint8_t)1);
        return Wire.read();
    }

    void readRegisters(uint8_t reg, uint8_t* buffer, uint8_t len) {
        Wire.beginTransmission(MPU6050_ADDR);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU6050_ADDR, len);
        for (uint8_t i = 0; i < len && Wire.available(); i++) {
            buffer[i] = Wire.read();
        }
    }

    // Madgwick AHRS filter
    void madgwickUpdate(float gx, float gy, float gz, float ax, float ay, float az) {
        float q0 = quat.w, q1 = quat.x, q2 = quat.y, q3 = quat.z;

        // Convert gyro to rad/s
        gx *= 0.0174533f;
        gy *= 0.0174533f;
        gz *= 0.0174533f;

        // Normalize accelerometer
        float norm = sqrt(ax*ax + ay*ay + az*az);
        if (norm == 0) return;
        ax /= norm; ay /= norm; az /= norm;

        // Gradient descent algorithm
        float s0, s1, s2, s3;
        float _2q0 = 2.0f * q0;
        float _2q1 = 2.0f * q1;
        float _2q2 = 2.0f * q2;
        float _2q3 = 2.0f * q3;
        float _4q0 = 4.0f * q0;
        float _4q1 = 4.0f * q1;
        float _4q2 = 4.0f * q2;
        float _8q1 = 8.0f * q1;
        float _8q2 = 8.0f * q2;
        float q0q0 = q0 * q0;
        float q1q1 = q1 * q1;
        float q2q2 = q2 * q2;
        float q3q3 = q3 * q3;

        s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
        s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
        s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
        s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;

        norm = sqrt(s0*s0 + s1*s1 + s2*s2 + s3*s3);
        if (norm > 0) {
            s0 /= norm; s1 /= norm; s2 /= norm; s3 /= norm;
        }

        // Apply feedback
        float qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz) - beta * s0;
        float qDot1 = 0.5f * (q0 * gx + q2 * gz - q3 * gy) - beta * s1;
        float qDot2 = 0.5f * (q0 * gy - q1 * gz + q3 * gx) - beta * s2;
        float qDot3 = 0.5f * (q0 * gz + q1 * gy - q2 * gx) - beta * s3;

        // Integrate
        q0 += qDot0 * dt;
        q1 += qDot1 * dt;
        q2 += qDot2 * dt;
        q3 += qDot3 * dt;

        // Normalize quaternion
        norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
        quat.w = q0 / norm;
        quat.x = q1 / norm;
        quat.y = q2 / norm;
        quat.z = q3 / norm;
    }

    void quaternionToEuler() {
        // Roll (x-axis rotation)
        float sinr_cosp = 2.0f * (quat.w * quat.x + quat.y * quat.z);
        float cosr_cosp = 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y);
        roll = atan2(sinr_cosp, cosr_cosp) * 57.2958f;

        // Pitch (y-axis rotation)
        float sinp = 2.0f * (quat.w * quat.y - quat.z * quat.x);
        if (fabs(sinp) >= 1)
            pitch = copysign(90.0f, sinp);
        else
            pitch = asin(sinp) * 57.2958f;

        // Yaw (z-axis rotation)
        float siny_cosp = 2.0f * (quat.w * quat.z + quat.x * quat.y);
        float cosy_cosp = 1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z);
        yaw = atan2(siny_cosp, cosy_cosp) * 57.2958f;
    }

public:
    bool begin(int sda = 21, int scl = 22) {
        Wire.begin(sda, scl);
        Wire.setClock(400000);  // 400kHz I2C

        // Check connection
        uint8_t whoAmI = readRegister(0x75);  // WHO_AM_I register
        if (whoAmI != 0x68 && whoAmI != 0x98) {
            Serial.println("IMU: MPU6050 not found!");
            return false;
        }

        // Wake up MPU6050
        writeRegister(MPU6050_PWR_MGMT_1, 0x00);
        delay(100);

        // Sample rate = 1kHz / (1 + SMPLRT_DIV)
        writeRegister(MPU6050_SMPLRT_DIV, 9);  // 100Hz

        // DLPF config
        writeRegister(MPU6050_CONFIG, 0x03);  // ~43Hz bandwidth

        // Gyro config: +/- 250 deg/s
        writeRegister(MPU6050_GYRO_CONFIG, 0x00);

        // Accel config: +/- 2g
        writeRegister(MPU6050_ACCEL_CONFIG, 0x00);

        initialized = true;
        lastUpdate = micros();

        Serial.println("IMU: MPU6050 initialized");
        return true;
    }

    void calibrate(int samples = 500) {
        if (!initialized) return;

        Serial.println("IMU: Calibrating... Keep device still!");

        int32_t gyroSum[3] = {0, 0, 0};
        int32_t accelSum[3] = {0, 0, 0};

        for (int i = 0; i < samples; i++) {
            readRawData();
            for (int j = 0; j < 3; j++) {
                gyroSum[j] += gyroRaw[j];
                accelSum[j] += accelRaw[j];
            }
            delay(2);
        }

        for (int j = 0; j < 3; j++) {
            gyroOffset[j] = gyroSum[j] / samples;
            accelOffset[j] = accelSum[j] / samples;
        }
        // Z accel should be ~16384 (1g) when flat
        accelOffset[2] -= 16384;

        Serial.println("IMU: Calibration complete");
        Serial.printf("  Gyro offset: %d, %d, %d\n", gyroOffset[0], gyroOffset[1], gyroOffset[2]);
        Serial.printf("  Accel offset: %d, %d, %d\n", accelOffset[0], accelOffset[1], accelOffset[2]);
    }

    void readRawData() {
        uint8_t buffer[14];
        readRegisters(MPU6050_ACCEL_XOUT_H, buffer, 14);

        accelRaw[0] = (buffer[0] << 8) | buffer[1];
        accelRaw[1] = (buffer[2] << 8) | buffer[3];
        accelRaw[2] = (buffer[4] << 8) | buffer[5];
        // buffer[6], buffer[7] = temperature
        gyroRaw[0] = (buffer[8] << 8) | buffer[9];
        gyroRaw[1] = (buffer[10] << 8) | buffer[11];
        gyroRaw[2] = (buffer[12] << 8) | buffer[13];
    }

    void update() {
        if (!initialized) return;

        // Calculate dt
        unsigned long now = micros();
        dt = (now - lastUpdate) / 1000000.0f;
        lastUpdate = now;

        // Read raw data
        readRawData();

        // Apply calibration and convert to physical units
        // Accel: 16384 LSB/g at +/-2g
        accel[0] = (accelRaw[0] - accelOffset[0]) / 16384.0f;
        accel[1] = (accelRaw[1] - accelOffset[1]) / 16384.0f;
        accel[2] = (accelRaw[2] - accelOffset[2]) / 16384.0f;

        // Gyro: 131 LSB/(deg/s) at +/-250 deg/s
        gyro[0] = (gyroRaw[0] - gyroOffset[0]) / 131.0f;
        gyro[1] = (gyroRaw[1] - gyroOffset[1]) / 131.0f;
        gyro[2] = (gyroRaw[2] - gyroOffset[2]) / 131.0f;

        // Update orientation using Madgwick filter
        madgwickUpdate(gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2]);

        // Convert to Euler angles
        quaternionToEuler();
    }

    bool isInitialized() { return initialized; }

    // Getters
    Quaternion getQuaternion() { return quat; }
    float getYaw() { return yaw; }
    float getPitch() { return pitch; }
    float getRoll() { return roll; }

    float* getAccel() { return accel; }
    float* getGyro() { return gyro; }

    // For debugging
    void printData() {
        Serial.printf("IMU: Y=%.1f P=%.1f R=%.1f | Q=(%.3f,%.3f,%.3f,%.3f)\n",
            yaw, pitch, roll, quat.w, quat.x, quat.y, quat.z);
    }
};
