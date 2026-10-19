#include "arm.h"
#include <Arduino.h>
#include <Wire.h>

static bool s_is_arm_initialized = false;

void arm_init(void) {
    if (s_is_arm_initialized) {
        return;
    }
    s_is_arm_initialized = true;

    // Initialize I2C with configured pins and clock
    Wire.begin(ARM_I2C_SDA_PIN, ARM_I2C_SCL_PIN);
    Wire.setClock(400000);

    // Initialize low-level PCA9685 driver and updater task
    arm_servo_init();

    // Smoothly position arm to calibrated Home positions {1900, 1600, 1200, 1200, 1500, 1500}
    arm_home(ARM_SPEED_DEFAULT);
}

void arm_home(uint8_t speed) {
    for (uint8_t i = 0; i < ARM_NUM_JOINTS; i++) {
        arm_set_joint_us(static_cast<ArmJointId>(i), ARM_HOME_POS_US[i], speed);
    }
}

void arm_rest(uint8_t speed) {
    for (uint8_t i = 0; i < ARM_NUM_JOINTS; i++) {
        arm_set_joint_us(static_cast<ArmJointId>(i), ARM_REST_POS_US[i], speed);
    }
}

void arm_emergency_stop(void) {
    arm_servo_emergency_stop();
}

void arm_set_joint_us(ArmJointId joint, uint16_t us, uint8_t speed) {
    if (joint >= ARM_NUM_JOINTS) {
        return;
    }
    uint16_t clamped_us = clamp_joint_pulse(joint, us);
    arm_servo_move_to_us(static_cast<uint8_t>(joint), clamped_us, speed);
}

void arm_set_joint_deg(ArmJointId joint, float deg, uint8_t speed) {
    if (joint >= ARM_NUM_JOINTS) {
        return;
    }
    uint16_t us = angle_to_pulse(joint, deg);
    arm_set_joint_us(joint, us, speed);
}

void arm_jog(ArmJointId joint, int8_t direction, uint8_t speed) {
    if (joint >= ARM_NUM_JOINTS) {
        return;
    }
    if (direction > 0) {
        arm_servo_move_to_us(static_cast<uint8_t>(joint), DEFAULT_JOINT_LIMITS[joint].max_us, speed);
    } else if (direction < 0) {
        arm_servo_move_to_us(static_cast<uint8_t>(joint), DEFAULT_JOINT_LIMITS[joint].min_us, speed);
    } else {
        arm_pause(joint);
    }
}

void arm_pause(ArmJointId joint) {
    if (joint >= ARM_NUM_JOINTS) {
        return;
    }
    arm_servo_pause(static_cast<uint8_t>(joint));
}

uint16_t arm_get_joint_us(ArmJointId joint) {
    if (joint >= ARM_NUM_JOINTS) {
        return 1500;
    }
    return arm_servo_get_current_us(static_cast<uint8_t>(joint));
}

float arm_get_joint_deg(ArmJointId joint) {
    if (joint >= ARM_NUM_JOINTS) {
        return 0.0f;
    }
    uint16_t us = arm_servo_get_current_us(static_cast<uint8_t>(joint));
    return pulse_to_angle(joint, us);
}

void arm_get_all_us(uint16_t positions[ARM_NUM_JOINTS]) {
    for (uint8_t i = 0; i < ARM_NUM_JOINTS; i++) {
        positions[i] = arm_servo_get_current_us(i);
    }
}
