#pragma once

#include <cstdint>

/**
 * @brief Enumeration of the 6 joints of the robotic arm.
 */
typedef enum ArmJointId {
    ARM_JOINT_BASE = 0,         // Base turntable yaw
    ARM_JOINT_SHOULDER = 1,     // Shoulder pitch
    ARM_JOINT_ELBOW = 2,        // Elbow pitch
    ARM_JOINT_WRIST_PITCH = 3,  // Wrist pitch
    ARM_JOINT_WRIST_ROLL = 4,   // Wrist roll
    ARM_JOINT_GRIPPER = 5,      // Gripper claw
    ARM_NUM_JOINTS = 6
} ArmJointId;

/**
 * @brief Returns the human-readable string name of a joint.
 */
inline const char* get_joint_name(ArmJointId joint) {
    switch (joint) {
        case ARM_JOINT_BASE:        return "Base";
        case ARM_JOINT_SHOULDER:    return "Shoulder";
        case ARM_JOINT_ELBOW:       return "Elbow";
        case ARM_JOINT_WRIST_PITCH: return "Wrist Pitch";
        case ARM_JOINT_WRIST_ROLL:  return "Wrist Roll";
        case ARM_JOINT_GRIPPER:     return "Gripper";
        default:                    return "Unknown";
    }
}

/**
 * @brief Physical and pulse limits for an individual joint.
 */
struct JointLimits {
    uint16_t min_us;
    uint16_t max_us;
    float min_deg;
    float max_deg;
};

// Safe default limits table for all 6 joints (1000..2000 us, 0..180 deg)
constexpr JointLimits DEFAULT_JOINT_LIMITS[ARM_NUM_JOINTS] = {
    { 1000, 2000, 0.0f, 180.0f }, // ARM_JOINT_BASE
    { 1000, 2000, 0.0f, 180.0f }, // ARM_JOINT_SHOULDER
    { 1000, 2000, 0.0f, 180.0f }, // ARM_JOINT_ELBOW
    { 1000, 2000, 0.0f, 180.0f }, // ARM_JOINT_WRIST_PITCH
    { 1000, 2000, 0.0f, 180.0f }, // ARM_JOINT_WRIST_ROLL
    { 1000, 2000, 0.0f, 180.0f }  // ARM_JOINT_GRIPPER
};

// Calibrated Home positions (in microseconds)
constexpr uint16_t ARM_HOME_POS_US[ARM_NUM_JOINTS] = {1900, 1600, 1200, 1200, 1500, 1500};

// Calibrated Rest/Park positions (in microseconds)
constexpr uint16_t ARM_REST_POS_US[ARM_NUM_JOINTS] = {1500, 1000, 1000, 1000, 1500, 1500};

// Speed range definitions
constexpr uint8_t ARM_SPEED_MIN = 1;
constexpr uint8_t ARM_SPEED_MAX = 10;
constexpr uint8_t ARM_SPEED_DEFAULT = 5;

// Base speed factor (0.2 us/ms = 200 us/s per speed level; at speed 5 -> 1000 us/s)
constexpr float ARM_SPEED_BASE_US_PER_MS = 0.2f;

// Hardware & I2C configuration for ESP32 and PCA9685
constexpr uint8_t ARM_I2C_SDA_PIN = 21;
constexpr uint8_t ARM_I2C_SCL_PIN = 22;
constexpr uint8_t PCA9685_I2C_ADDR = 0x40;
constexpr uint16_t PCA9685_PWM_FREQ = 60; // 60 Hz PWM update rate

// WiFi AP & Controller defaults
constexpr const char* ARM_WIFI_DEFAULT_SSID = "Robotic-Arm";
constexpr uint8_t ARM_WIFI_DEFAULT_IP[4] = {192, 168, 4, 5};
constexpr uint8_t ARM_WIFI_DEFAULT_GW[4] = {192, 168, 4, 1};
constexpr uint8_t ARM_WIFI_DEFAULT_SUBNET[4] = {255, 255, 255, 0};
constexpr uint16_t ARM_PULSE_NEUTRAL = 1500;

