#include "Arduino.h"
#include "rover_driving.h"
#include "rover_kinematics.h"
#include "rover_config.h"

static RoverMode current_rover_mode = DRIVE_TURN_NORMAL;

static void write_pwm_left(uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(ROVER_L298N_ENA_PIN, duty);
#else
    ledcWrite(L298N_PWM_CHANNEL_LEFT, duty);
#endif
}

static void write_pwm_right(uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(ROVER_L298N_ENB_PIN, duty);
#else
    ledcWrite(L298N_PWM_CHANNEL_RIGHT, duty);
#endif
}

static void apply_l298n_left(uint8_t pwm, MotorDirection dir) {
    if (dir == MOTOR_FORWARD) {
        digitalWrite(ROVER_L298N_IN1_PIN, HIGH);
        digitalWrite(ROVER_L298N_IN2_PIN, LOW);
    } else if (dir == MOTOR_BACKWARD) {
        digitalWrite(ROVER_L298N_IN1_PIN, LOW);
        digitalWrite(ROVER_L298N_IN2_PIN, HIGH);
    } else {
        digitalWrite(ROVER_L298N_IN1_PIN, LOW);
        digitalWrite(ROVER_L298N_IN2_PIN, LOW);
        pwm = 0;
    }
    write_pwm_left(pwm);
}

static void apply_l298n_right(uint8_t pwm, MotorDirection dir) {
    if (dir == MOTOR_FORWARD) {
        digitalWrite(ROVER_L298N_IN3_PIN, HIGH);
        digitalWrite(ROVER_L298N_IN4_PIN, LOW);
    } else if (dir == MOTOR_BACKWARD) {
        digitalWrite(ROVER_L298N_IN3_PIN, LOW);
        digitalWrite(ROVER_L298N_IN4_PIN, HIGH);
    } else {
        digitalWrite(ROVER_L298N_IN3_PIN, LOW);
        digitalWrite(ROVER_L298N_IN4_PIN, LOW);
        pwm = 0;
    }
    write_pwm_right(pwm);
}

void rover_driving_init(void) {
    // Direction GPIO pins for Left H-bridge
    pinMode(ROVER_L298N_IN1_PIN, OUTPUT);
    pinMode(ROVER_L298N_IN2_PIN, OUTPUT);
    digitalWrite(ROVER_L298N_IN1_PIN, LOW);
    digitalWrite(ROVER_L298N_IN2_PIN, LOW);

    // Direction GPIO pins for Right H-bridge
    pinMode(ROVER_L298N_IN3_PIN, OUTPUT);
    pinMode(ROVER_L298N_IN4_PIN, OUTPUT);
    digitalWrite(ROVER_L298N_IN3_PIN, LOW);
    digitalWrite(ROVER_L298N_IN4_PIN, LOW);

    // Hardware PWM (LEDC) for speed control
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(ROVER_L298N_ENA_PIN, L298N_PWM_FREQ, L298N_PWM_RESOLUTION);
    ledcAttach(ROVER_L298N_ENB_PIN, L298N_PWM_FREQ, L298N_PWM_RESOLUTION);
#else
    ledcSetup(L298N_PWM_CHANNEL_LEFT, L298N_PWM_FREQ, L298N_PWM_RESOLUTION);
    ledcAttachPin(ROVER_L298N_ENA_PIN, L298N_PWM_CHANNEL_LEFT);

    ledcSetup(L298N_PWM_CHANNEL_RIGHT, L298N_PWM_FREQ, L298N_PWM_RESOLUTION);
    ledcAttachPin(ROVER_L298N_ENB_PIN, L298N_PWM_CHANNEL_RIGHT);
#endif

    rover_driving_stop();
}

void rover_driving_set_drive_mode(RoverMode mode) {
    current_rover_mode = mode;
}

void rover_driving_move(uint16_t throttle_signal, uint16_t steer_signal) {
    MotorOutputs outputs = calculate_motors(current_rover_mode, throttle_signal, steer_signal);
    apply_l298n_left(outputs.left_pwm, outputs.left_dir);
    apply_l298n_right(outputs.right_pwm, outputs.right_dir);
}

void rover_driving_steer(uint16_t steer_signal) {
    // In differential drive, steering is applied synchronously in rover_driving_move.
    (void)steer_signal;
}

void rover_driving_stop(void) {
    apply_l298n_left(0, MOTOR_STOP);
    apply_l298n_right(0, MOTOR_STOP);
}
