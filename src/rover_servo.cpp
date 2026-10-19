#include "rover_servo.h"

// Servos have been eliminated from the project.
// The rover now uses skid-steering differential drive with an L298N motor driver.
// These functions are retained as no-ops for API compatibility.

void rover_servo_init(void) {}
void rover_servo_write(RoverServoId servoId, uint16_t us, bool full_range) {
    (void)servoId; (void)us; (void)full_range;
}
void rover_servo_move(RoverServoId servoId, uint16_t pos, uint8_t speed) {
    (void)servoId; (void)pos; (void)speed;
}
void rover_servo_pause(RoverServoId servoId) { (void)servoId; }
void rover_servo_resume(RoverServoId servoId) { (void)servoId; }
