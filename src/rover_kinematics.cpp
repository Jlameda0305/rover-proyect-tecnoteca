#include "rover_kinematics.h"
#include <algorithm>
#include <cstdlib>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdio>
struct HostSerial {
    template<typename... Args>
    void printf(const char* fmt, Args... args) {
        std::printf(fmt, args...);
    }
    void println(const char* msg) {
        std::printf("%s\n", msg);
    }
};
static HostSerial Serial;
#endif

template <typename T>
static inline T clamp_val(T val, T low, T high) {
    return (val < low) ? low : ((val > high) ? high : val);
}

MotorOutputs calculate_motors(RoverMode mode, uint16_t throttle_signal, uint16_t steer_signal) {
    MotorOutputs out;

    // Fail-safe handling: if signal is 0 or below failsafe threshold (800 us), treat as neutral (1500 us)
    uint16_t throttle = (throttle_signal <= 800) ? RC_CENTER : clamp_val<uint16_t>(throttle_signal, RC_LOW, RC_HIGH);
    uint16_t steer    = (steer_signal <= 800)    ? RC_CENTER : clamp_val<uint16_t>(steer_signal, RC_LOW, RC_HIGH);

    int16_t t_diff = static_cast<int16_t>(throttle) - RC_CENTER; // [-500, +500]
    int16_t s_diff = static_cast<int16_t>(steer) - RC_CENTER;    // [-500, +500]

    int16_t v_left = 0;
    int16_t v_right = 0;

    if (mode == DRIVE_TURN_SPIN) {
        // Spin Mode (360-degree point turn in place):
        // Steering right (s_diff > 0): Left motors forward, Right motors backward (clockwise spin)
        // Steering left (s_diff < 0): Left motors backward, Right motors forward (counter-clockwise spin)
        v_left = s_diff;
        v_right = -s_diff;
    } else {
        // DRIVE_TURN_NORMAL: Skid-Steering Differential Mixer
        // Turn right (s_diff > 0): Left runs faster, Right runs slower
        // Turn left (s_diff < 0): Left runs slower, Right runs faster
        v_left = clamp_val<int16_t>(t_diff + s_diff, -500, 500);
        v_right = clamp_val<int16_t>(t_diff - s_diff, -500, 500);
    }

    out.left_speed = v_left;
    out.right_speed = v_right;
    out.left = static_cast<uint16_t>(RC_CENTER + v_left);
    out.right = static_cast<uint16_t>(RC_CENTER + v_right);

    // Left motor L298N output
    if (v_left > 0) {
        out.left_dir = MOTOR_FORWARD;
        out.left_pwm = static_cast<uint8_t>((static_cast<int32_t>(v_left) * 255) / 500);
    } else if (v_left < 0) {
        out.left_dir = MOTOR_BACKWARD;
        out.left_pwm = static_cast<uint8_t>((static_cast<int32_t>(-v_left) * 255) / 500);
    } else {
        out.left_dir = MOTOR_STOP;
        out.left_pwm = 0;
    }

    // Right motor L298N output
    if (v_right > 0) {
        out.right_dir = MOTOR_FORWARD;
        out.right_pwm = static_cast<uint8_t>((static_cast<int32_t>(v_right) * 255) / 500);
    } else if (v_right < 0) {
        out.right_dir = MOTOR_BACKWARD;
        out.right_pwm = static_cast<uint8_t>((static_cast<int32_t>(-v_right) * 255) / 500);
    } else {
        out.right_dir = MOTOR_STOP;
        out.right_pwm = 0;
    }

    return out;
}

uint16_t filter_rc_signal(const uint16_t* samples, uint8_t sample_count) {
    if (samples == nullptr || sample_count == 0) {
        return RC_CENTER;
    }

    uint32_t sum = 0;
    for (uint8_t i = 0; i < sample_count; ++i) {
        sum += samples[i];
    }
    uint16_t avg = static_cast<uint16_t>(sum / sample_count);

    // Fail-safe: disconnected controller or invalid signal
    if (avg <= 800) {
        return RC_CENTER;
    }
    // Extremo inferior
    if (avg < 1010) {
        return RC_LOW;
    }
    // Extremo superior
    if (avg > 1990) {
        return RC_HIGH;
    }
    // Deadzone (zona muerta): 1480-1520 µs -> 1500
    if (avg >= 1480 && avg <= 1520) {
        return RC_CENTER;
    }

    return avg;
}

bool run_rover_kinematics_self_test(void) {
    int total = 0;
    int passed = 0;
    int failed = 0;

    auto assert_eq = [&](int actual, int expected, const char* msg) {
        total++;
        if (actual == expected) {
            passed++;
        } else {
            failed++;
            Serial.printf("  [FAIL] %s | Exp: %d, Got: %d\n", msg, expected, actual);
        }
    };

    auto assert_true = [&](bool cond, const char* msg) {
        total++;
        if (cond) {
            passed++;
        } else {
            failed++;
            Serial.printf("  [FAIL] %s\n", msg);
        }
    };

    Serial.println("--- [SELF-TEST] Testing Skid-Steering Kinematics ---");

    // 1. filter_rc_signal
    uint16_t s1[] = { 1500 }; assert_eq(filter_rc_signal(s1, 1), 1500, "Deadzone center");
    uint16_t s2[] = { 1480 }; assert_eq(filter_rc_signal(s2, 1), 1500, "Deadzone lower");
    uint16_t s3[] = { 1520 }; assert_eq(filter_rc_signal(s3, 1), 1500, "Deadzone upper");
    uint16_t s0[] = { 0 };    assert_eq(filter_rc_signal(s0, 1), 1500, "Failsafe 0us");
    uint16_t s8[] = { 800 };  assert_eq(filter_rc_signal(s8, 1), 1500, "Failsafe 800us");
    uint16_t s81[] = { 801 }; assert_true(filter_rc_signal(s81, 1) < 1500, "Above failsafe 801us");
    uint16_t sm1[] = { 1005 }; assert_eq(filter_rc_signal(sm1, 1), 1000, "Min saturation");
    uint16_t sm2[] = { 1995 }; assert_eq(filter_rc_signal(sm2, 1), 2000, "Max saturation");

    // 2. Straight motion
    MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 1500, 1500);
    assert_eq(m.left_speed, 0, "Neutral speed");
    assert_eq(m.left_pwm, 0, "Neutral PWM");
    assert_eq(m.left_dir, MOTOR_STOP, "Neutral dir");

    m = calculate_motors(DRIVE_TURN_NORMAL, 2000, 1500);
    assert_eq(m.left_speed, 500, "Forward 500 speed");
    assert_eq(m.left_pwm, 255, "Forward 255 PWM");
    assert_eq(m.left_dir, MOTOR_FORWARD, "Forward dir");

    m = calculate_motors(DRIVE_TURN_NORMAL, 1000, 1500);
    assert_eq(m.left_speed, -500, "Backward -500 speed");
    assert_eq(m.left_pwm, 255, "Backward 255 PWM");
    assert_eq(m.left_dir, MOTOR_BACKWARD, "Backward dir");

    // 3. Differential turns
    m = calculate_motors(DRIVE_TURN_NORMAL, 1700, 1700);
    assert_eq(m.left_speed, 400, "Turn right left=400");
    assert_eq(m.right_speed, 0, "Turn right right=0");
    assert_true(m.left_pwm > m.right_pwm, "Turn right left PWM > right PWM");

    m = calculate_motors(DRIVE_TURN_NORMAL, 1700, 1300);
    assert_eq(m.left_speed, 0, "Turn left left=0");
    assert_eq(m.right_speed, 400, "Turn left right=400");
    assert_true(m.right_pwm > m.left_pwm, "Turn left right PWM > left PWM");

    m = calculate_motors(DRIVE_TURN_NORMAL, 1500, 1800);
    assert_eq(m.left_speed, 300, "In-place right left=+300");
    assert_eq(m.right_speed, -300, "In-place right right=-300");
    assert_eq(m.left_dir, MOTOR_FORWARD, "In-place right left=FWD");
    assert_eq(m.right_dir, MOTOR_BACKWARD, "In-place right right=REV");
    assert_eq(m.left_pwm, m.right_pwm, "In-place right equal PWM");

    // 4. Spin 360
    m = calculate_motors(DRIVE_TURN_SPIN, 1900, 1800);
    assert_eq(m.left_speed, 300, "Spin CW left=300");
    assert_eq(m.right_speed, -300, "Spin CW right=-300");

    m = calculate_motors(DRIVE_TURN_SPIN, 1000, 1200);
    assert_eq(m.left_speed, -300, "Spin CCW left=-300");
    assert_eq(m.right_speed, 300, "Spin CCW right=300");

    // 5. PWM scaling
    for (int diff = 0; diff <= 500; diff += 50) {
        m = calculate_motors(DRIVE_TURN_NORMAL, 1500 + diff, 1500);
        int expected_pwm = (diff * 255) / 500;
        assert_eq(m.left_pwm, expected_pwm, "PWM linear scale");
    }

    // 6. Failsafe
    m = calculate_motors(DRIVE_TURN_NORMAL, 0, 0);
    assert_eq(m.left_speed, 0, "Failsafe 0us");
    assert_eq(m.left_pwm, 0, "Failsafe 0us PWM");
    assert_eq(m.left_dir, MOTOR_STOP, "Failsafe 0us STOP");

    Serial.printf("[SELF-TEST RESULT] Assertions: %d | Passed: %d | Failed: %d\n", total, passed, failed);
    return (failed == 0);
}
