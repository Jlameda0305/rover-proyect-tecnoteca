#pragma once

#define SERIAL_PORT_SPEED 115200

#define RC_NUM_CHANNELS   6
#define RC_FILTER_SAMPLES 2

#define RC_LOW    1000
#define RC_CENTER 1500
#define RC_HIGH   2000

#define RC_CH1  0
#define RC_CH2  1
#define RC_CH3  2
#define RC_CH4  3
#define RC_CH5  4
#define RC_CH6  5

// Input pins connected to each channel on RC Receiver
#define RC_CH1_INPUT  34
#define RC_CH2_INPUT  35
#define RC_CH3_INPUT  4
#define RC_CH4_INPUT  39
#define RC_CH5_INPUT  25
#define RC_CH6_INPUT  36

#define RC_STEER_CHANNEL      RC_CH1
#define RC_MOTOR_CHANNEL      RC_CH2
#define RC_HEAD_YAW_CHANNEL   RC_CH3
#define RC_HEAD_PITCH_CHANNEL RC_CH4

#define RC_ROVER_MODE_ROVER_CHANNEL RC_CH5
#define RC_ARM_MODE_ROVER_CHANNEL   RC_CH6

#define ARM_NUM_AXIS 6

// ============================================================================
// L298N DUAL H-BRIDGE MOTOR DRIVER PINS (4 DC MOTORS)
// ============================================================================
// Left Motors (OUT1 & OUT2 in parallel)
#define ROVER_L298N_ENA_PIN 27
#define ROVER_L298N_IN1_PIN 26
#define ROVER_L298N_IN2_PIN 25

// Right Motors (OUT3 & OUT4 in parallel)
#define ROVER_L298N_IN3_PIN        21    // Direction Pin 3
#define ROVER_L298N_IN4_PIN        22    // Direction Pin 4
#define ROVER_L298N_ENB_PIN        13    // PWM Speed Control (LEDC Channel 1)

// Backward compatibility alias definitions
#define ROVER_MOTORS_LEFT_PIN      ROVER_L298N_ENA_PIN
#define ROVER_MOTORS_RIGHT_PIN     ROVER_L298N_ENB_PIN

// PWM Hardware Configuration (LEDC on ESP32)
#define L298N_PWM_FREQ             1000  // 1 kHz (smooth operation for L298N)
#define L298N_PWM_RESOLUTION       8     // 8-bit resolution (0 - 255)
#define L298N_PWM_CHANNEL_LEFT     0
#define L298N_PWM_CHANNEL_RIGHT    1

#define ROVER_SETTINGS_SWITCH_1    2
#define ROVER_SETTINGS_SWITCH_2    23

const uint16_t config_default_ch_values[RC_NUM_CHANNELS] = { RC_CENTER, RC_CENTER, RC_CENTER, RC_CENTER, RC_LOW, RC_CENTER };
