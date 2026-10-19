#pragma once
#include <stdint.h>

typedef enum RoverMode {
  DRIVE_TURN_NORMAL = 0,
  DRIVE_TURN_SPIN   = 1
} RoverMode;

void rover_driving_init(void);
void rover_driving_set_drive_mode(RoverMode mode);
void rover_driving_move(uint16_t throttle_signal, uint16_t steer_signal = 1500);
void rover_driving_steer(uint16_t steer_signal);
void rover_driving_stop(void);