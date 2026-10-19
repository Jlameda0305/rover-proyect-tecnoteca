#pragma once

#include <Arduino.h>

#define SERVO_MAX_SPEED 10
#define SERVO_MIN_SPEED 1

typedef enum RoverServoId {
  SERVO_FRONT_LEFT = 0,
  SERVO_FRONT_RIGHT,
  SERVO_BACK_LEFT,
  SERVO_BACK_RIGHT,
  SERVO_LAST
} RoverServoId;

void rover_servo_init(void);
void rover_servo_write(RoverServoId servoId, uint16_t us, bool full_range = false);

void rover_servo_move(RoverServoId servoId, uint16_t pos, uint8_t speed);
void rover_servo_pause(RoverServoId servoId);
void rover_servo_resume(RoverServoId servoId);
