#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "rover_driving.h"

#ifndef RC_LOW
#define RC_LOW 1000
#endif
#ifndef RC_CENTER
#define RC_CENTER 1500
#endif
#ifndef RC_HIGH
#define RC_HIGH 2000
#endif

typedef enum MotorDirection {
    MOTOR_STOP = 0,
    MOTOR_FORWARD = 1,
    MOTOR_BACKWARD = 2
} MotorDirection;

struct MotorOutputs {
    uint16_t left;          // RC us value [1000..2000] (legacy / telemetry compatibility)
    uint16_t right;         // RC us value [1000..2000] (legacy / telemetry compatibility)
    int16_t left_speed;     // Normalized speed: -500 (full reverse) to +500 (full forward), 0 = stop
    int16_t right_speed;    // Normalized speed: -500 to +500
    uint8_t left_pwm;       // 0 to 255 (duty cycle for L298N ENA)
    uint8_t right_pwm;      // 0 to 255 (duty cycle for L298N ENB)
    MotorDirection left_dir;   // Direction for L298N IN1, IN2
    MotorDirection right_dir;  // Direction for L298N IN3, IN4
};

MotorOutputs calculate_motors(RoverMode mode, uint16_t throttle_signal, uint16_t steer_signal);
uint16_t filter_rc_signal(const uint16_t* samples, uint8_t sample_count);
bool run_rover_kinematics_self_test(void);
