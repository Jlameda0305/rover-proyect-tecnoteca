#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>
#include "rover_kinematics.h"

// Simple zero-dependency test framework
static int g_total_assertions = 0;
static int g_passed_assertions = 0;
static int g_failed_assertions = 0;

static int g_total_suites = 0;
static int g_passed_suites = 0;

#define ASSERT_TRUE(condition, message) do { \
    g_total_assertions++; \
    if (condition) { \
        g_passed_assertions++; \
    } else { \
        g_failed_assertions++; \
        std::cerr << "  [FAIL] " << message << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
    } \
} while (0)

#define ASSERT_EQ(actual, expected, message) do { \
    g_total_assertions++; \
    if ((actual) == (expected)) { \
        g_passed_assertions++; \
    } else { \
        g_failed_assertions++; \
        std::cerr << "  [FAIL] " << message << " | Expected: " << (expected) \
                  << ", Got: " << (actual) << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
    } \
} while (0)

void run_suite(const std::string& suite_name, void (*suite_func)()) {
    g_total_suites++;
    int initial_failures = g_failed_assertions;
    std::cout << "\n========================================\n";
    std::cout << "SUITE: " << suite_name << "\n";
    std::cout << "========================================\n";
    suite_func();
    if (g_failed_assertions == initial_failures) {
        g_passed_suites++;
        std::cout << "--> SUITE PASSED: " << suite_name << "\n";
    } else {
        std::cerr << "--> SUITE FAILED: " << suite_name << "\n";
    }
}

// ---------------------------------------------------------
// 1. Suite: filter_rc_signal
// ---------------------------------------------------------
void test_suite_filter_rc_signal() {
    // 1.1 Deadzone (1480-1520 µs -> 1500)
    {
        uint16_t s1[] = { 1500 };
        ASSERT_EQ(filter_rc_signal(s1, 1), 1500, "Deadzone center 1500us -> 1500");

        uint16_t s2[] = { 1480 };
        ASSERT_EQ(filter_rc_signal(s2, 1), 1500, "Deadzone lower bound 1480us -> 1500");

        uint16_t s3[] = { 1520 };
        ASSERT_EQ(filter_rc_signal(s3, 1), 1500, "Deadzone upper bound 1520us -> 1500");

        uint16_t s4[] = { 1495, 1505 };
        ASSERT_EQ(filter_rc_signal(s4, 2), 1500, "Deadzone average 1500us -> 1500");
    }

    // 1.2 Fail-safe (señal <= 800 µs -> 1500)
    {
        uint16_t s0[] = { 0 };
        ASSERT_EQ(filter_rc_signal(s0, 1), 1500, "Failsafe 0us -> 1500 (desconectado)");

        uint16_t s500[] = { 500 };
        ASSERT_EQ(filter_rc_signal(s500, 1), 1500, "Failsafe 500us -> 1500");

        uint16_t s800[] = { 800 };
        ASSERT_EQ(filter_rc_signal(s800, 1), 1500, "Failsafe límite 800us -> 1500");

        uint16_t s801[] = { 801 };
        ASSERT_TRUE(filter_rc_signal(s801, 1) < 1500, "Por encima de failsafe (801us) no debe ser forzado a neutral");
    }

    // 1.3 Extremos / Saturación (< 1010 -> 1000, > 1990 -> 2000)
    {
        uint16_t s_min1[] = { 1005 };
        ASSERT_EQ(filter_rc_signal(s_min1, 1), 1000, "Saturación mínima 1005us -> 1000");

        uint16_t s_min2[] = { 1000 };
        ASSERT_EQ(filter_rc_signal(s_min2, 1), 1000, "Extremo exacto 1000us -> 1000");

        uint16_t s_max1[] = { 1995 };
        ASSERT_EQ(filter_rc_signal(s_max1, 1), 2000, "Saturación máxima 1995us -> 2000");

        uint16_t s_max2[] = { 2000 };
        ASSERT_EQ(filter_rc_signal(s_max2, 1), 2000, "Extremo exacto 2000us -> 2000");
    }

    // 1.4 Promedio móvil normal
    {
        uint16_t samples[] = { 1600, 1700, 1800 };
        ASSERT_EQ(filter_rc_signal(samples, 3), 1700, "Promedio móvil (1600+1700+1800)/3 = 1700");
    }
}

// ---------------------------------------------------------
// 2. Suite: Skid-Steering Avance y Retroceso en Línea Recta
// ---------------------------------------------------------
void test_suite_straight_motion() {
    // 2.1 Neutro / Parada
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 1500, 1500);
        ASSERT_EQ(m.left_speed, 0, "Neutral: left speed = 0");
        ASSERT_EQ(m.right_speed, 0, "Neutral: right speed = 0");
        ASSERT_EQ(m.left_pwm, 0, "Neutral: left PWM = 0");
        ASSERT_EQ(m.right_pwm, 0, "Neutral: right PWM = 0");
        ASSERT_EQ(m.left_dir, MOTOR_STOP, "Neutral: left dir = MOTOR_STOP");
        ASSERT_EQ(m.right_dir, MOTOR_STOP, "Neutral: right dir = MOTOR_STOP");
    }

    // 2.2 Avance Máximo (2000 us, Steer 1500 us)
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 2000, 1500);
        ASSERT_EQ(m.left_speed, 500, "Max forward: left speed = 500");
        ASSERT_EQ(m.right_speed, 500, "Max forward: right speed = 500");
        ASSERT_EQ(m.left_pwm, 255, "Max forward: left PWM = 255");
        ASSERT_EQ(m.right_pwm, 255, "Max forward: right PWM = 255");
        ASSERT_EQ(m.left_dir, MOTOR_FORWARD, "Max forward: left dir = FORWARD");
        ASSERT_EQ(m.right_dir, MOTOR_FORWARD, "Max forward: right dir = FORWARD");
    }

    // 2.3 Retroceso Máximo (1000 us, Steer 1500 us)
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 1000, 1500);
        ASSERT_EQ(m.left_speed, -500, "Max backward: left speed = -500");
        ASSERT_EQ(m.right_speed, -500, "Max backward: right speed = -500");
        ASSERT_EQ(m.left_pwm, 255, "Max backward: left PWM = 255");
        ASSERT_EQ(m.right_pwm, 255, "Max backward: right PWM = 255");
        ASSERT_EQ(m.left_dir, MOTOR_BACKWARD, "Max backward: left dir = BACKWARD");
        ASSERT_EQ(m.right_dir, MOTOR_BACKWARD, "Max backward: right dir = BACKWARD");
    }

    // 2.4 Avance al 50% (1750 us)
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 1750, 1500);
        ASSERT_EQ(m.left_speed, 250, "Half forward: left speed = 250");
        ASSERT_EQ(m.right_speed, 250, "Half forward: right speed = 250");
        ASSERT_EQ(m.left_pwm, 127, "Half forward: left PWM = 127");
        ASSERT_EQ(m.right_pwm, 127, "Half forward: right PWM = 127");
        ASSERT_EQ(m.left_dir, MOTOR_FORWARD, "Half forward: left dir = FORWARD");
        ASSERT_EQ(m.right_dir, MOTOR_FORWARD, "Half forward: right dir = FORWARD");
    }
}

// ---------------------------------------------------------
// 3. Suite: Skid-Steering Giros Diferenciales (Izquierda / Derecha)
// ---------------------------------------------------------
void test_suite_differential_turns() {
    // 3.1 Avance con giro a la derecha (Throttle 1700, Steer 1700)
    // t_diff = +200, s_diff = +200 -> v_left = +400, v_right = 0
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 1700, 1700);
        ASSERT_EQ(m.left_speed, 400, "Turn right: left wheel faster (+400)");
        ASSERT_EQ(m.right_speed, 0, "Turn right: right wheel pivots at 0");
        ASSERT_TRUE(m.left_pwm > m.right_pwm, "Turn right: left PWM > right PWM");
        ASSERT_EQ(m.left_dir, MOTOR_FORWARD, "Turn right: left dir = FORWARD");
        ASSERT_EQ(m.right_dir, MOTOR_STOP, "Turn right: right dir = STOP");
    }

    // 3.2 Avance con giro a la izquierda (Throttle 1700, Steer 1300)
    // t_diff = +200, s_diff = -200 -> v_left = 0, v_right = +400
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 1700, 1300);
        ASSERT_EQ(m.left_speed, 0, "Turn left: left wheel pivots at 0");
        ASSERT_EQ(m.right_speed, 400, "Turn left: right wheel faster (+400)");
        ASSERT_TRUE(m.right_pwm > m.left_pwm, "Turn left: right PWM > left PWM");
        ASSERT_EQ(m.left_dir, MOTOR_STOP, "Turn left: left dir = STOP");
        ASSERT_EQ(m.right_dir, MOTOR_FORWARD, "Turn left: right dir = FORWARD");
    }

    // 3.3 Giro en el lugar sin avance (Throttle 1500, Steer 1800)
    // t_diff = 0, s_diff = +300 -> v_left = +300, v_right = -300
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 1500, 1800);
        ASSERT_EQ(m.left_speed, 300, "In-place right turn: left = +300");
        ASSERT_EQ(m.right_speed, -300, "In-place right turn: right = -300");
        ASSERT_EQ(m.left_dir, MOTOR_FORWARD, "In-place right turn: left dir = FORWARD");
        ASSERT_EQ(m.right_dir, MOTOR_BACKWARD, "In-place right turn: right dir = BACKWARD");
        ASSERT_EQ(m.left_pwm, m.right_pwm, "In-place right turn: equal speed on both sides");
    }

    // 3.4 Clamping cuando la suma excede 500 (Throttle 2000, Steer 1800)
    // t_diff = +500, s_diff = +300 -> v_left clamp(800) = 500, v_right = 500 - 300 = 200
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 2000, 1800);
        ASSERT_EQ(m.left_speed, 500, "Clamped left speed = 500");
        ASSERT_EQ(m.right_speed, 200, "Right speed = 200");
        ASSERT_EQ(m.left_pwm, 255, "Clamped left PWM = 255");
        ASSERT_EQ(m.left_dir, MOTOR_FORWARD, "Left dir = FORWARD");
        ASSERT_EQ(m.right_dir, MOTOR_FORWARD, "Right dir = FORWARD");
    }
}

// ---------------------------------------------------------
// 4. Suite: Modo Spin 360° (Giro Axial Puro)
// ---------------------------------------------------------
void test_suite_spin_360() {
    // 4.1 Spin horario (Clockwise): Steer 1800 us (s_diff = +300)
    // Izquierda avance (+300), Derecha retroceso (-300)
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_SPIN, 1900, 1800); // Throttle debe ser ignorado
        ASSERT_EQ(m.left_speed, 300, "Spin CW: left speed = +300");
        ASSERT_EQ(m.right_speed, -300, "Spin CW: right speed = -300");
        ASSERT_EQ(m.left_dir, MOTOR_FORWARD, "Spin CW: left dir = FORWARD");
        ASSERT_EQ(m.right_dir, MOTOR_BACKWARD, "Spin CW: right dir = BACKWARD");
        ASSERT_EQ(m.left_pwm, m.right_pwm, "Spin CW: identical PWM both sides");
    }

    // 4.2 Spin antihorario (Counter-Clockwise): Steer 1200 us (s_diff = -300)
    // Izquierda retroceso (-300), Derecha avance (+300)
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_SPIN, 1000, 1200); // Throttle debe ser ignorado
        ASSERT_EQ(m.left_speed, -300, "Spin CCW: left speed = -300");
        ASSERT_EQ(m.right_speed, 300, "Spin CCW: right speed = +300");
        ASSERT_EQ(m.left_dir, MOTOR_BACKWARD, "Spin CCW: left dir = BACKWARD");
        ASSERT_EQ(m.right_dir, MOTOR_FORWARD, "Spin CCW: right dir = FORWARD");
        ASSERT_EQ(m.left_pwm, m.right_pwm, "Spin CCW: identical PWM both sides");
    }

    // 4.3 Spin neutro: Steer 1500 us
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_SPIN, 1500, 1500);
        ASSERT_EQ(m.left_speed, 0, "Spin neutral: left speed = 0");
        ASSERT_EQ(m.right_speed, 0, "Spin neutral: right speed = 0");
        ASSERT_EQ(m.left_dir, MOTOR_STOP, "Spin neutral: left dir = STOP");
        ASSERT_EQ(m.right_dir, MOTOR_STOP, "Spin neutral: right dir = STOP");
        ASSERT_EQ(m.left_pwm, 0, "Spin neutral: left PWM = 0");
        ASSERT_EQ(m.right_pwm, 0, "Spin neutral: right PWM = 0");
    }
}

// ---------------------------------------------------------
// 5. Suite: Mapeo de Salidas L298N (PWM 0-255 & Pines Digitales)
// ---------------------------------------------------------
void test_suite_l298n_outputs() {
    // Escalamiento lineal de velocidad (-500 a +500) a PWM (0 a 255)
    // PWM = (|V| * 255) / 500
    for (int diff = 0; diff <= 500; diff += 50) {
        uint16_t throttle = 1500 + diff;
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, throttle, 1500);
        uint8_t expected_pwm = static_cast<uint8_t>((diff * 255) / 500);
        ASSERT_EQ(m.left_pwm, expected_pwm, "Linear PWM scaling matches formula");
        if (diff > 0) {
            ASSERT_EQ(m.left_dir, MOTOR_FORWARD, "Forward direction flag set");
        } else {
            ASSERT_EQ(m.left_dir, MOTOR_STOP, "Stop direction flag set for 0 speed");
        }
    }
}

// ---------------------------------------------------------
// 6. Suite: Fail-Safe y Resistencia a Señales Inválidas
// ---------------------------------------------------------
void test_suite_failsafe_handling() {
    // 6.1 Señal 0 us (receptor desconectado)
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 0, 0);
        ASSERT_EQ(m.left_speed, 0, "Failsafe 0us: left speed = 0");
        ASSERT_EQ(m.right_speed, 0, "Failsafe 0us: right speed = 0");
        ASSERT_EQ(m.left_pwm, 0, "Failsafe 0us: left PWM = 0");
        ASSERT_EQ(m.right_pwm, 0, "Failsafe 0us: right PWM = 0");
        ASSERT_EQ(m.left_dir, MOTOR_STOP, "Failsafe 0us: left dir = STOP");
        ASSERT_EQ(m.right_dir, MOTOR_STOP, "Failsafe 0us: right dir = STOP");
    }

    // 6.2 Señal <= 800 us (failsafe threshold)
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 750, 600);
        ASSERT_EQ(m.left_speed, 0, "Failsafe <=800us: neutral left");
        ASSERT_EQ(m.right_speed, 0, "Failsafe <=800us: neutral right");
        ASSERT_EQ(m.left_pwm, 0, "Failsafe <=800us: 0 PWM");
    }

    // 6.3 Valores fuera de rango (> 2000 us, < 1000 us)
    {
        MotorOutputs m = calculate_motors(DRIVE_TURN_NORMAL, 2500, 1500);
        ASSERT_EQ(m.left_speed, 500, "Clamp upper bound >2000us: speed = 500");
        ASSERT_EQ(m.left_pwm, 255, "Clamp upper bound >2000us: PWM = 255");

        MotorOutputs m2 = calculate_motors(DRIVE_TURN_NORMAL, 900, 1500);
        ASSERT_EQ(m2.left_speed, -500, "Clamp lower bound 900us: speed = -500");
        ASSERT_EQ(m2.left_pwm, 255, "Clamp lower bound 900us: PWM = 255");
    }
}

// ---------------------------------------------------------
// Main Entrypoint
// ---------------------------------------------------------
int main() {
    std::cout << "====================================================\n";
    std::cout << " MARS ROVER SKID-STEERING / L298N TEST SUITE        \n";
    std::cout << "====================================================\n";

    run_suite("filter_rc_signal (Deadzone, Fail-safe, Extremos)", test_suite_filter_rc_signal);
    run_suite("Skid-Steering: Avance y Retroceso en Linea Recta", test_suite_straight_motion);
    run_suite("Skid-Steering: Giros Diferenciales", test_suite_differential_turns);
    run_suite("Modo Spin 360 (Giro Axial Puro)", test_suite_spin_360);
    run_suite("L298N: Mapeo de Salidas PWM y Direcciones", test_suite_l298n_outputs);
    run_suite("Fail-safe y Limites de Senal", test_suite_failsafe_handling);

    std::cout << "\n====================================================\n";
    std::cout << "                 TEST RESULTS SUMMARY               \n";
    std::cout << "====================================================\n";
    std::cout << "  Total Suites:      " << g_total_suites << "\n";
    std::cout << "  Passed Suites:     " << g_passed_suites << "\n";
    std::cout << "  Failed Suites:     " << (g_total_suites - g_passed_suites) << "\n";
    std::cout << "  Total Assertions:  " << g_total_assertions << "\n";
    std::cout << "  Passed Assertions: " << g_passed_assertions << "\n";
    std::cout << "  Failed Assertions: " << g_failed_assertions << "\n";
    std::cout << "====================================================\n";

    if (g_failed_assertions == 0) {
        std::cout << ">>> OVERALL STATUS: ALL TESTS PASSED (100%) <<<\n";
        return 0;
    } else {
        std::cerr << ">>> OVERALL STATUS: SOME TESTS FAILED <<<\n";
        return 1;
    }
}
