#pragma once

#include <cstdint>

/**
 * @brief Pure abstract interface for PWM drivers.
 * Decouples kinematics and joint control from hardware access (e.g. PCA9685 vs Mock driver).
 */
class IPwmDriver {
public:
    virtual ~IPwmDriver() = default;

    /**
     * @brief Initialize PWM hardware and communications.
     * @return true if initialization succeeded, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Set PWM pulse width for a given channel in microseconds.
     * @param channel The output channel number (0..ARM_NUM_JOINTS-1).
     * @param pulse_us Pulse width in microseconds.
     */
    virtual void set_pulse_us(uint8_t channel, uint16_t pulse_us) = 0;
};
