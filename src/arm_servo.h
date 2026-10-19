#pragma once

#include <cstdint>
#include <stdbool.h>
#include "arm_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the PCA9685 servo driver and start the FreeRTOS ArmServoUpdater task.
 * Configures I2C on SDA 21, SCL 22, PCA9685 at 0x40 with 60 Hz PWM frequency.
 * @return true on success, false otherwise.
 */
bool arm_servo_init(void);

/**
 * @brief Directly writes pulse width in microseconds to a channel on the PCA9685.
 * Clamps pulse within safe limits, converts via us_to_pca9685_count, and sets PWM.
 * Also stops background motion interpolation for this channel.
 * @param channel Joint channel (0..ARM_NUM_JOINTS-1).
 * @param us Pulse width in microseconds.
 */
void arm_servo_write_us(uint8_t channel, uint16_t us);

/**
 * @brief Commands a joint to move smoothly towards a target pulse width.
 * @param channel Joint channel (0..ARM_NUM_JOINTS-1).
 * @param target_us Target pulse width in microseconds.
 * @param speed Speed level (ARM_SPEED_MIN..ARM_SPEED_MAX).
 */
void arm_servo_move_to_us(uint8_t channel, uint16_t target_us, uint8_t speed);

/**
 * @brief Pauses movement for a specific joint channel.
 * @param channel Joint channel (0..ARM_NUM_JOINTS-1).
 */
void arm_servo_pause(uint8_t channel);

/**
 * @brief Emergency stop for all arm channels (stops motion immediately and holds current position).
 */
void arm_servo_emergency_stop(void);

/**
 * @brief Get the current pulse width of a channel in microseconds.
 * @param channel Joint channel (0..ARM_NUM_JOINTS-1).
 * @return Current pulse in microseconds.
 */
uint16_t arm_servo_get_current_us(uint8_t channel);

/**
 * @brief Get the target pulse width of a channel in microseconds.
 * @param channel Joint channel (0..ARM_NUM_JOINTS-1).
 * @return Target pulse in microseconds.
 */
uint16_t arm_servo_get_target_us(uint8_t channel);

/**
 * @brief Checks if a joint channel is actively moving.
 * @param channel Joint channel (0..ARM_NUM_JOINTS-1).
 * @return true if moving, false if paused or reached target.
 */
bool arm_servo_is_moving(uint8_t channel);

#ifdef __cplusplus
}
#endif
