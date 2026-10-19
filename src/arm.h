#pragma once

#include <cstdint>
#include "arm_config.h"
#include "arm_driver.h"
#include "arm_kinematics.h"
#include "arm_servo.h"

// Backward compatibility definitions
typedef ArmJointId ArmAxis;
#define ARM_NUM_AXIS ARM_NUM_JOINTS
#define ARM_AXIS_1 ARM_JOINT_BASE
#define ARM_AXIS_2 ARM_JOINT_SHOULDER
#define ARM_AXIS_3 ARM_JOINT_ELBOW
#define ARM_AXIS_4 ARM_JOINT_WRIST_PITCH
#define ARM_AXIS_5 ARM_JOINT_WRIST_ROLL
#define ARM_AXIS_6 ARM_JOINT_GRIPPER

#define ARM_MAX_SPEED ARM_SPEED_MAX
#define ARM_MIN_SPEED ARM_SPEED_MIN

/**
 * @brief Initializes the robotic arm subsystem.
 * Initializes I2C, calls arm_servo_init(), and smoothly positions arm to calibrated Home {1900, 1600, 1200, 1200, 1500, 1500}.
 */
void arm_init(void);

/**
 * @brief Smoothly moves all joints to their calibrated Home positions.
 * @param speed Speed level (ARM_SPEED_MIN..ARM_SPEED_MAX, default ARM_SPEED_DEFAULT).
 */
void arm_home(uint8_t speed = ARM_SPEED_DEFAULT);

/**
 * @brief Smoothly moves all joints to their calibrated Rest/Park positions.
 * @param speed Speed level (ARM_SPEED_MIN..ARM_SPEED_MAX, default ARM_SPEED_DEFAULT).
 */
void arm_rest(uint8_t speed = ARM_SPEED_DEFAULT);

/**
 * @brief Emergency stop for the arm. Halts all joint movements immediately.
 */
void arm_emergency_stop(void);

/**
 * @brief Commands a joint to move towards a target pulse in microseconds.
 * @param joint The joint identifier (ARM_JOINT_BASE..ARM_JOINT_GRIPPER).
 * @param us Target pulse width in microseconds (clamped to joint limits).
 * @param speed Speed level (ARM_SPEED_MIN..ARM_SPEED_MAX, default ARM_SPEED_DEFAULT).
 */
void arm_set_joint_us(ArmJointId joint, uint16_t us, uint8_t speed = ARM_SPEED_DEFAULT);

/**
 * @brief Commands a joint to move towards a target angle in degrees.
 * @param joint The joint identifier (ARM_JOINT_BASE..ARM_JOINT_GRIPPER).
 * @param deg Target angle in degrees (clamped to joint limits).
 * @param speed Speed level (ARM_SPEED_MIN..ARM_SPEED_MAX, default ARM_SPEED_DEFAULT).
 */
void arm_set_joint_deg(ArmJointId joint, float deg, uint8_t speed = ARM_SPEED_DEFAULT);

/**
 * @brief Jogs a joint towards its limit.
 * @param joint The joint identifier.
 * @param direction +1 = towards max, -1 = towards min, 0 = pause.
 * @param speed Speed level (ARM_SPEED_MIN..ARM_SPEED_MAX, default ARM_SPEED_DEFAULT).
 */
void arm_jog(ArmJointId joint, int8_t direction, uint8_t speed = ARM_SPEED_DEFAULT);

/**
 * @brief Pauses movement of a specific joint.
 * @param joint The joint identifier.
 */
void arm_pause(ArmJointId joint);

/**
 * @brief Gets current pulse width of a joint in microseconds.
 * @param joint The joint identifier.
 * @return Current pulse in microseconds.
 */
uint16_t arm_get_joint_us(ArmJointId joint);

/**
 * @brief Gets current angle of a joint in degrees.
 * @param joint The joint identifier.
 * @return Current angle in degrees.
 */
float arm_get_joint_deg(ArmJointId joint);

/**
 * @brief Reads current pulse widths for all 6 joints.
 * @param positions Array of size ARM_NUM_JOINTS to populate.
 */
void arm_get_all_us(uint16_t positions[ARM_NUM_JOINTS]);

// Backward compatibility delegates
inline void arm_move_axis_us(ArmAxis axisNum, uint16_t pos, uint8_t speed) {
    arm_set_joint_us(axisNum, pos, speed);
}

inline void arm_resume(ArmAxis axisNum) {
    (void)axisNum;
}
