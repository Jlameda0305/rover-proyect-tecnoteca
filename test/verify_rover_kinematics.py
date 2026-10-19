#!/usr/bin/env python3
"""
Test Suite Verification for Mars Rover Skid-Steering & L298N Motor Driver.
Replicates the C++ logic from rover_kinematics.cpp and validates all test cases.
"""

from enum import IntEnum

class RoverMode(IntEnum):
    DRIVE_TURN_NORMAL = 0
    DRIVE_TURN_SPIN = 1

class MotorDirection(IntEnum):
    MOTOR_STOP = 0
    MOTOR_FORWARD = 1
    MOTOR_BACKWARD = 2

RC_LOW = 1000
RC_CENTER = 1500
RC_HIGH = 2000
RC_DEADZONE_MIN = 1480
RC_DEADZONE_MAX = 1520
RC_FAILSAFE_THRESHOLD = 800

def clamp(val, low, high):
    return max(low, min(high, val))

def filter_rc_signal(samples):
    if not samples:
        return RC_CENTER
    avg = sum(samples) // len(samples)
    if avg <= RC_FAILSAFE_THRESHOLD:
        return RC_CENTER
    if avg < 1010:
        return RC_LOW
    if avg > 1990:
        return RC_HIGH
    if RC_DEADZONE_MIN <= avg <= RC_DEADZONE_MAX:
        return RC_CENTER
    return clamp(avg, RC_LOW, RC_HIGH)

def calculate_motors(mode, throttle_signal, steer_signal):
    throttle = RC_CENTER if throttle_signal <= RC_FAILSAFE_THRESHOLD else clamp(throttle_signal, RC_LOW, RC_HIGH)
    steer = RC_CENTER if steer_signal <= RC_FAILSAFE_THRESHOLD else clamp(steer_signal, RC_LOW, RC_HIGH)

    t_diff = throttle - RC_CENTER
    s_diff = steer - RC_CENTER

    if mode == RoverMode.DRIVE_TURN_SPIN:
        v_left = s_diff
        v_right = -s_diff
    else:
        v_left = clamp(t_diff + s_diff, -500, 500)
        v_right = clamp(t_diff - s_diff, -500, 500)

    # Left motor
    if v_left > 0:
        left_dir = MotorDirection.MOTOR_FORWARD
        left_pwm = (v_left * 255) // 500
    elif v_left < 0:
        left_dir = MotorDirection.MOTOR_BACKWARD
        left_pwm = ((-v_left) * 255) // 500
    else:
        left_dir = MotorDirection.MOTOR_STOP
        left_pwm = 0

    # Right motor
    if v_right > 0:
        right_dir = MotorDirection.MOTOR_FORWARD
        right_pwm = (v_right * 255) // 500
    elif v_right < 0:
        right_dir = MotorDirection.MOTOR_BACKWARD
        right_pwm = ((-v_right) * 255) // 500
    else:
        right_dir = MotorDirection.MOTOR_STOP
        right_pwm = 0

    return {
        "left": RC_CENTER + v_left,
        "right": RC_CENTER + v_right,
        "left_speed": v_left,
        "right_speed": v_right,
        "left_pwm": left_pwm,
        "right_pwm": right_pwm,
        "left_dir": left_dir,
        "right_dir": right_dir,
    }


def main():
    total_assertions = 0
    passed_assertions = 0
    failed_assertions = 0

    def assert_eq(actual, expected, msg):
        nonlocal total_assertions, passed_assertions, failed_assertions
        total_assertions += 1
        if actual == expected:
            passed_assertions += 1
            print(f"  [PASS] {msg}")
        else:
            failed_assertions += 1
            print(f"  [FAIL] {msg} | Expected: {expected}, Got: {actual}")

    def assert_true(condition, msg):
        nonlocal total_assertions, passed_assertions, failed_assertions
        total_assertions += 1
        if condition:
            passed_assertions += 1
            print(f"  [PASS] {msg}")
        else:
            failed_assertions += 1
            print(f"  [FAIL] {msg}")

    print("====================================================")
    print(" SUITE 1: filter_rc_signal (Deadzone, Failsafe, Saturation)")
    print("====================================================")
    assert_eq(filter_rc_signal([1500]), 1500, "Deadzone center 1500us -> 1500")
    assert_eq(filter_rc_signal([1480]), 1500, "Deadzone lower bound 1480us -> 1500")
    assert_eq(filter_rc_signal([1520]), 1500, "Deadzone upper bound 1520us -> 1500")
    assert_eq(filter_rc_signal([1495, 1505]), 1500, "Deadzone average 1500us -> 1500")
    assert_eq(filter_rc_signal([0]), 1500, "Failsafe 0us -> 1500 (disconnected)")
    assert_eq(filter_rc_signal([500]), 1500, "Failsafe 500us -> 1500")
    assert_eq(filter_rc_signal([800]), 1500, "Failsafe limit 800us -> 1500")
    assert_true(filter_rc_signal([801]) < 1500, "Above failsafe (801us) not neutral")
    assert_eq(filter_rc_signal([1005]), 1000, "Min saturation 1005us -> 1000")
    assert_eq(filter_rc_signal([1000]), 1000, "Min exact 1000us -> 1000")
    assert_eq(filter_rc_signal([1995]), 2000, "Max saturation 1995us -> 2000")
    assert_eq(filter_rc_signal([2000]), 2000, "Max exact 2000us -> 2000")
    assert_eq(filter_rc_signal([1600, 1700, 1800]), 1700, "Moving average (1600+1700+1800)/3 = 1700")

    print("\n====================================================")
    print(" SUITE 2: Skid-Steering Straight Motion")
    print("====================================================")
    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 1500, 1500)
    assert_eq(m["left_speed"], 0, "Neutral: left speed = 0")
    assert_eq(m["right_speed"], 0, "Neutral: right speed = 0")
    assert_eq(m["left_pwm"], 0, "Neutral: left PWM = 0")
    assert_eq(m["right_pwm"], 0, "Neutral: right PWM = 0")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_STOP, "Neutral: left dir = STOP")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_STOP, "Neutral: right dir = STOP")

    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 2000, 1500)
    assert_eq(m["left_speed"], 500, "Max forward: left speed = 500")
    assert_eq(m["right_speed"], 500, "Max forward: right speed = 500")
    assert_eq(m["left_pwm"], 255, "Max forward: left PWM = 255")
    assert_eq(m["right_pwm"], 255, "Max forward: right PWM = 255")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_FORWARD, "Max forward: left dir = FORWARD")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_FORWARD, "Max forward: right dir = FORWARD")

    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 1000, 1500)
    assert_eq(m["left_speed"], -500, "Max backward: left speed = -500")
    assert_eq(m["right_speed"], -500, "Max backward: right speed = -500")
    assert_eq(m["left_pwm"], 255, "Max backward: left PWM = 255")
    assert_eq(m["right_pwm"], 255, "Max backward: right PWM = 255")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_BACKWARD, "Max backward: left dir = BACKWARD")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_BACKWARD, "Max backward: right dir = BACKWARD")

    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 1750, 1500)
    assert_eq(m["left_speed"], 250, "Half forward: left speed = 250")
    assert_eq(m["right_speed"], 250, "Half forward: right speed = 250")
    assert_eq(m["left_pwm"], 127, "Half forward: left PWM = 127")
    assert_eq(m["right_pwm"], 127, "Half forward: right PWM = 127")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_FORWARD, "Half forward: left dir = FORWARD")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_FORWARD, "Half forward: right dir = FORWARD")

    print("\n====================================================")
    print(" SUITE 3: Skid-Steering Differential Turns")
    print("====================================================")
    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 1700, 1700)
    assert_eq(m["left_speed"], 400, "Turn right: left wheel faster (+400)")
    assert_eq(m["right_speed"], 0, "Turn right: right wheel pivots at 0")
    assert_true(m["left_pwm"] > m["right_pwm"], "Turn right: left PWM > right PWM")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_FORWARD, "Turn right: left dir = FORWARD")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_STOP, "Turn right: right dir = STOP")

    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 1700, 1300)
    assert_eq(m["left_speed"], 0, "Turn left: left wheel pivots at 0")
    assert_eq(m["right_speed"], 400, "Turn left: right wheel faster (+400)")
    assert_true(m["right_pwm"] > m["left_pwm"], "Turn left: right PWM > left PWM")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_STOP, "Turn left: left dir = STOP")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_FORWARD, "Turn left: right dir = FORWARD")

    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 1500, 1800)
    assert_eq(m["left_speed"], 300, "In-place right turn: left = +300")
    assert_eq(m["right_speed"], -300, "In-place right turn: right = -300")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_FORWARD, "In-place right turn: left dir = FORWARD")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_BACKWARD, "In-place right turn: right dir = BACKWARD")
    assert_eq(m["left_pwm"], m["right_pwm"], "In-place right turn: equal speed on both sides")

    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 2000, 1800)
    assert_eq(m["left_speed"], 500, "Clamped left speed = 500")
    assert_eq(m["right_speed"], 200, "Right speed = 200")
    assert_eq(m["left_pwm"], 255, "Clamped left PWM = 255")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_FORWARD, "Left dir = FORWARD")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_FORWARD, "Right dir = FORWARD")

    print("\n====================================================")
    print(" SUITE 4: Spin 360 Mode")
    print("====================================================")
    m = calculate_motors(RoverMode.DRIVE_TURN_SPIN, 1900, 1800)
    assert_eq(m["left_speed"], 300, "Spin CW: left speed = +300")
    assert_eq(m["right_speed"], -300, "Spin CW: right speed = -300")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_FORWARD, "Spin CW: left dir = FORWARD")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_BACKWARD, "Spin CW: right dir = BACKWARD")
    assert_eq(m["left_pwm"], m["right_pwm"], "Spin CW: identical PWM both sides")

    m = calculate_motors(RoverMode.DRIVE_TURN_SPIN, 1000, 1200)
    assert_eq(m["left_speed"], -300, "Spin CCW: left speed = -300")
    assert_eq(m["right_speed"], 300, "Spin CCW: right speed = +300")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_BACKWARD, "Spin CCW: left dir = BACKWARD")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_FORWARD, "Spin CCW: right dir = FORWARD")
    assert_eq(m["left_pwm"], m["right_pwm"], "Spin CCW: identical PWM both sides")

    m = calculate_motors(RoverMode.DRIVE_TURN_SPIN, 1500, 1500)
    assert_eq(m["left_speed"], 0, "Spin neutral: left speed = 0")
    assert_eq(m["right_speed"], 0, "Spin neutral: right speed = 0")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_STOP, "Spin neutral: left dir = STOP")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_STOP, "Spin neutral: right dir = STOP")
    assert_eq(m["left_pwm"], 0, "Spin neutral: left PWM = 0")
    assert_eq(m["right_pwm"], 0, "Spin neutral: right PWM = 0")

    print("\n====================================================")
    print(" SUITE 5: L298N PWM Scaling and Direction Mapping")
    print("====================================================")
    for diff in range(0, 501, 50):
        throttle = 1500 + diff
        m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, throttle, 1500)
        expected_pwm = (diff * 255) // 500
        assert_eq(m["left_pwm"], expected_pwm, f"Linear PWM scaling for diff={diff} -> {expected_pwm}")
        if diff > 0:
            assert_eq(m["left_dir"], MotorDirection.MOTOR_FORWARD, f"Forward dir flag for diff={diff}")
        else:
            assert_eq(m["left_dir"], MotorDirection.MOTOR_STOP, "Stop dir flag for diff=0")

    print("\n====================================================")
    print(" SUITE 6: Fail-Safe and Out-of-Bounds Protection")
    print("====================================================")
    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 0, 0)
    assert_eq(m["left_speed"], 0, "Failsafe 0us: left speed = 0")
    assert_eq(m["right_speed"], 0, "Failsafe 0us: right speed = 0")
    assert_eq(m["left_pwm"], 0, "Failsafe 0us: left PWM = 0")
    assert_eq(m["right_pwm"], 0, "Failsafe 0us: right PWM = 0")
    assert_eq(m["left_dir"], MotorDirection.MOTOR_STOP, "Failsafe 0us: left dir = STOP")
    assert_eq(m["right_dir"], MotorDirection.MOTOR_STOP, "Failsafe 0us: right dir = STOP")

    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 750, 600)
    assert_eq(m["left_speed"], 0, "Failsafe <=800us: neutral left")
    assert_eq(m["right_speed"], 0, "Failsafe <=800us: neutral right")
    assert_eq(m["left_pwm"], 0, "Failsafe <=800us: 0 PWM")

    m = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 2500, 1500)
    assert_eq(m["left_speed"], 500, "Clamp upper bound >2000us: speed = 500")
    assert_eq(m["left_pwm"], 255, "Clamp upper bound >2000us: PWM = 255")

    m2 = calculate_motors(RoverMode.DRIVE_TURN_NORMAL, 900, 1500)
    assert_eq(m2["left_speed"], -500, "Clamp lower bound 900us: speed = -500")
    assert_eq(m2["left_pwm"], 255, "Clamp lower bound 900us: PWM = 255")

    print("\n====================================================")
    print(" TEST RESULTS SUMMARY")
    print("====================================================")
    print(f"  Total Assertions:  {total_assertions}")
    print(f"  Passed Assertions: {passed_assertions}")
    print(f"  Failed Assertions: {failed_assertions}")
    print("====================================================")

    if failed_assertions == 0:
        print(">>> OVERALL STATUS: ALL TESTS PASSED (100%) <<<\n")
        return 0
    else:
        print(">>> OVERALL STATUS: SOME TESTS FAILED <<<\n")
        return 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
