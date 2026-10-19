#pragma once

#include <cstdint>
#include "arm_config.h"

/**
 * @brief Clamps a pulse width in microseconds within the specified joint's limits.
 * @param joint The arm joint identifier.
 * @param pulse_us Pulse width in microseconds.
 * @param custom_limits Optional custom limits table; defaults to DEFAULT_JOINT_LIMITS if null.
 * @return Clamped pulse width in microseconds.
 */
uint16_t clamp_joint_pulse(ArmJointId joint, uint16_t pulse_us, const JointLimits* custom_limits = nullptr);

/**
 * @brief Clamps an angle in degrees within the specified joint's limits.
 * @param joint The arm joint identifier.
 * @param angle_deg Angle in degrees.
 * @param custom_limits Optional custom limits table; defaults to DEFAULT_JOINT_LIMITS if null.
 * @return Clamped angle in degrees.
 */
float clamp_joint_angle(ArmJointId joint, float angle_deg, const JointLimits* custom_limits = nullptr);

/**
 * @brief Converts a joint angle in degrees to a PWM pulse width in microseconds.
 * @param joint The arm joint identifier.
 * @param angle_deg Angle in degrees.
 * @param custom_limits Optional custom limits table; defaults to DEFAULT_JOINT_LIMITS if null.
 * @return Pulse width in microseconds.
 */
uint16_t angle_to_pulse(ArmJointId joint, float angle_deg, const JointLimits* custom_limits = nullptr);

/**
 * @brief Converts a PWM pulse width in microseconds to a joint angle in degrees.
 * @param joint The arm joint identifier.
 * @param pulse_us Pulse width in microseconds.
 * @param custom_limits Optional custom limits table; defaults to DEFAULT_JOINT_LIMITS if null.
 * @return Joint angle in degrees.
 */
float pulse_to_angle(ArmJointId joint, uint16_t pulse_us, const JointLimits* custom_limits = nullptr);

/**
 * @brief Translates pulse width in microseconds to a 12-bit PCA9685 timer count at 60 Hz.
 * @param pulse_us Pulse width in microseconds (1000us -> ~246, 1500us -> ~369, 2000us -> ~492).
 * @return 12-bit count value (0..4095).
 */
uint16_t us_to_pca9685_count(uint16_t pulse_us);

/**
 * @brief Calculates the next pulse position towards a target position based on speed and elapsed time.
 * @param current_us Current pulse width in microseconds.
 * @param target_us Target pulse width in microseconds.
 * @param speed_level Speed level (1 to 10).
 * @param dt_ms Elapsed delta time in milliseconds.
 * @return Updated pulse width in microseconds moving smoothly towards target.
 */
uint16_t calculate_step(uint16_t current_us, uint16_t target_us, uint8_t speed_level, uint32_t dt_ms);
