#include "arm_config.h"
#include "arm_driver.h"
#include "arm_kinematics.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <array>

// ============================================================================
// Lightweight Self-Contained Test Framework
// ============================================================================

struct TestContext {
    int total_suites = 0;
    int passed_suites = 0;
    int failed_suites = 0;
    int total_assertions = 0;
    int failed_assertions = 0;
    bool current_suite_passed = true;
};

static TestContext g_ctx;

#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_BLUE    "\033[1;34m"
#define ANSI_CYAN    "\033[1;36m"
#define ANSI_BOLD    "\033[1m"

#define ASSERT_TRUE(condition, msg) do { \
    g_ctx.total_assertions++; \
    if (!(condition)) { \
        g_ctx.failed_assertions++; \
        g_ctx.current_suite_passed = false; \
        std::cerr << "  " ANSI_RED "[FAIL]" ANSI_RESET " " << __FILE__ << ":" << __LINE__ \
                  << " Assertion failed: " << #condition << " (" << (msg) << ")" << std::endl; \
    } \
} while (0)

#define ASSERT_FALSE(condition, msg) ASSERT_TRUE(!(condition), msg)

#define ASSERT_EQ(expected, actual, msg) do { \
    g_ctx.total_assertions++; \
    auto exp_val = (expected); \
    auto act_val = (actual); \
    if (exp_val != act_val) { \
        g_ctx.failed_assertions++; \
        g_ctx.current_suite_passed = false; \
        std::cerr << "  " ANSI_RED "[FAIL]" ANSI_RESET " " << __FILE__ << ":" << __LINE__ \
                  << " Expected: " << exp_val << ", got: " << act_val << " (" << (msg) << ")" << std::endl; \
    } \
} while (0)

#define ASSERT_STR_EQ(expected, actual, msg) do { \
    g_ctx.total_assertions++; \
    const char* exp_s = (expected); \
    const char* act_s = (actual); \
    if (std::strcmp(exp_s, act_s) != 0) { \
        g_ctx.failed_assertions++; \
        g_ctx.current_suite_passed = false; \
        std::cerr << "  " ANSI_RED "[FAIL]" ANSI_RESET " " << __FILE__ << ":" << __LINE__ \
                  << " Expected: \"" << exp_s << "\", got: \"" << act_s << "\" (" << (msg) << ")" << std::endl; \
    } \
} while (0)

#define ASSERT_NEAR(expected, actual, tolerance, msg) do { \
    g_ctx.total_assertions++; \
    double exp_val = static_cast<double>(expected); \
    double act_val = static_cast<double>(actual); \
    double diff = std::abs(exp_val - act_val); \
    if (diff > (tolerance)) { \
        g_ctx.failed_assertions++; \
        g_ctx.current_suite_passed = false; \
        std::cerr << "  " ANSI_RED "[FAIL]" ANSI_RESET " " << __FILE__ << ":" << __LINE__ \
                  << " Expected: " << exp_val << " +/- " << (tolerance) \
                  << ", got: " << act_val << " (diff=" << diff << ") (" << (msg) << ")" << std::endl; \
    } \
} while (0)

static void run_suite(const std::string& name, const std::function<void()>& fn) {
    g_ctx.total_suites++;
    g_ctx.current_suite_passed = true;
    std::cout << ANSI_CYAN "[RUN       ]" ANSI_RESET " " << ANSI_BOLD << name << ANSI_RESET << std::endl;
    fn();
    if (g_ctx.current_suite_passed) {
        g_ctx.passed_suites++;
        std::cout << ANSI_GREEN "[      PASS]" ANSI_RESET " " << name << std::endl;
    } else {
        g_ctx.failed_suites++;
        std::cout << ANSI_RED "[      FAIL]" ANSI_RESET " " << name << std::endl;
    }
}

// ============================================================================
// Mock PWM Driver Implementation
// ============================================================================

class MockPwmDriver : public IPwmDriver {
public:
    bool initialized = false;
    std::array<uint16_t, ARM_NUM_JOINTS> channel_pulses{};
    std::array<uint32_t, ARM_NUM_JOINTS> write_counts{};
    uint32_t total_writes = 0;

    MockPwmDriver() {
        channel_pulses.fill(0);
        write_counts.fill(0);
    }

    bool init() override {
        initialized = true;
        return true;
    }

    void set_pulse_us(uint8_t channel, uint16_t pulse_us) override {
        if (channel < ARM_NUM_JOINTS) {
            channel_pulses[channel] = pulse_us;
            write_counts[channel]++;
            total_writes++;
        }
    }

    void reset() {
        initialized = false;
        channel_pulses.fill(0);
        write_counts.fill(0);
        total_writes = 0;
    }
};

// ============================================================================
// Test Suites
// ============================================================================

/**
 * @brief Test 1: Check joint names, num joints = 6, limit bounds validity.
 */
void test_joint_naming_and_config() {
    // Number of joints must be exactly 6
    ASSERT_EQ(6, static_cast<int>(ARM_NUM_JOINTS), "ARM_NUM_JOINTS must be 6");

    // Check joint names
    ASSERT_STR_EQ("Base", get_joint_name(ARM_JOINT_BASE), "Joint 0 name");
    ASSERT_STR_EQ("Shoulder", get_joint_name(ARM_JOINT_SHOULDER), "Joint 1 name");
    ASSERT_STR_EQ("Elbow", get_joint_name(ARM_JOINT_ELBOW), "Joint 2 name");
    ASSERT_STR_EQ("Wrist Pitch", get_joint_name(ARM_JOINT_WRIST_PITCH), "Joint 3 name");
    ASSERT_STR_EQ("Wrist Roll", get_joint_name(ARM_JOINT_WRIST_ROLL), "Joint 4 name");
    ASSERT_STR_EQ("Gripper", get_joint_name(ARM_JOINT_GRIPPER), "Joint 5 name");
    ASSERT_STR_EQ("Unknown", get_joint_name(static_cast<ArmJointId>(99)), "Invalid joint name");

    // Check default joint limits validity for all 6 joints
    for (int i = 0; i < ARM_NUM_JOINTS; ++i) {
        const auto& limits = DEFAULT_JOINT_LIMITS[i];
        ASSERT_TRUE(limits.min_us < limits.max_us, "min_us < max_us");
        ASSERT_TRUE(limits.min_deg < limits.max_deg, "min_deg < max_deg");
        ASSERT_EQ(1000, static_cast<int>(limits.min_us), "Default min_us should be 1000");
        ASSERT_EQ(2000, static_cast<int>(limits.max_us), "Default max_us should be 2000");
        ASSERT_NEAR(0.0f, limits.min_deg, 0.001f, "Default min_deg should be 0.0");
        ASSERT_NEAR(180.0f, limits.max_deg, 0.001f, "Default max_deg should be 180.0");
    }

    // Check speed definitions
    ASSERT_TRUE(ARM_SPEED_MIN <= ARM_SPEED_DEFAULT, "ARM_SPEED_MIN <= ARM_SPEED_DEFAULT");
    ASSERT_TRUE(ARM_SPEED_DEFAULT <= ARM_SPEED_MAX, "ARM_SPEED_DEFAULT <= ARM_SPEED_MAX");
    ASSERT_EQ(1, static_cast<int>(ARM_SPEED_MIN), "ARM_SPEED_MIN must be 1");
    ASSERT_EQ(10, static_cast<int>(ARM_SPEED_MAX), "ARM_SPEED_MAX must be 10");
    ASSERT_EQ(5, static_cast<int>(ARM_SPEED_DEFAULT), "ARM_SPEED_DEFAULT must be 5");
    ASSERT_NEAR(0.2f, ARM_SPEED_BASE_US_PER_MS, 0.001f, "ARM_SPEED_BASE_US_PER_MS check");

    // Check PCA9685 and I2C hardware constants
    ASSERT_EQ(21, static_cast<int>(ARM_I2C_SDA_PIN), "I2C SDA Pin check");
    ASSERT_EQ(22, static_cast<int>(ARM_I2C_SCL_PIN), "I2C SCL Pin check");
    ASSERT_EQ(0x40, static_cast<int>(PCA9685_I2C_ADDR), "PCA9685 I2C address check");
    ASSERT_EQ(60, static_cast<int>(PCA9685_PWM_FREQ), "PCA9685 PWM frequency check");
}

/**
 * @brief Test 2: Test below min, within range, above max for all joints.
 */
void test_joint_pulse_clamping() {
    for (int i = 0; i < ARM_NUM_JOINTS; ++i) {
        ArmJointId joint = static_cast<ArmJointId>(i);

        // Below min (underflow / low pulse)
        ASSERT_EQ(1000, clamp_joint_pulse(joint, 0), "Clamp 0 us to min");
        ASSERT_EQ(1000, clamp_joint_pulse(joint, 500), "Clamp 500 us to min");
        ASSERT_EQ(1000, clamp_joint_pulse(joint, 999), "Clamp 999 us to min");

        // Within range
        ASSERT_EQ(1000, clamp_joint_pulse(joint, 1000), "Boundary 1000 us");
        ASSERT_EQ(1250, clamp_joint_pulse(joint, 1250), "Within range 1250 us");
        ASSERT_EQ(1500, clamp_joint_pulse(joint, 1500), "Midpoint 1500 us");
        ASSERT_EQ(1750, clamp_joint_pulse(joint, 1750), "Within range 1750 us");
        ASSERT_EQ(2000, clamp_joint_pulse(joint, 2000), "Boundary 2000 us");

        // Above max
        ASSERT_EQ(2000, clamp_joint_pulse(joint, 2001), "Clamp 2001 us to max");
        ASSERT_EQ(2000, clamp_joint_pulse(joint, 2500), "Clamp 2500 us to max");
        ASSERT_EQ(2000, clamp_joint_pulse(joint, 65535), "Clamp 65535 us to max");
    }

    // Custom limits test
    JointLimits custom = { 1200, 1800, 10.0f, 170.0f };
    ASSERT_EQ(1200, clamp_joint_pulse(ARM_JOINT_BASE, 1100, &custom), "Custom limits min clamp");
    ASSERT_EQ(1500, clamp_joint_pulse(ARM_JOINT_BASE, 1500, &custom), "Custom limits within");
    ASSERT_EQ(1800, clamp_joint_pulse(ARM_JOINT_BASE, 1900, &custom), "Custom limits max clamp");
}

/**
 * @brief Test 3: Test below min, within range, above max for all joints.
 */
void test_joint_angle_clamping() {
    for (int i = 0; i < ARM_NUM_JOINTS; ++i) {
        ArmJointId joint = static_cast<ArmJointId>(i);

        // Below min
        ASSERT_NEAR(0.0f, clamp_joint_angle(joint, -50.0f), 0.001f, "Clamp negative angle -50 deg");
        ASSERT_NEAR(0.0f, clamp_joint_angle(joint, -0.01f), 0.001f, "Clamp negative angle -0.01 deg");

        // Within range
        ASSERT_NEAR(0.0f, clamp_joint_angle(joint, 0.0f), 0.001f, "Boundary 0 deg");
        ASSERT_NEAR(45.0f, clamp_joint_angle(joint, 45.0f), 0.001f, "Within range 45 deg");
        ASSERT_NEAR(90.0f, clamp_joint_angle(joint, 90.0f), 0.001f, "Midpoint 90 deg");
        ASSERT_NEAR(135.0f, clamp_joint_angle(joint, 135.0f), 0.001f, "Within range 135 deg");
        ASSERT_NEAR(180.0f, clamp_joint_angle(joint, 180.0f), 0.001f, "Boundary 180 deg");

        // Above max
        ASSERT_NEAR(180.0f, clamp_joint_angle(joint, 180.1f), 0.001f, "Clamp 180.1 deg to max");
        ASSERT_NEAR(180.0f, clamp_joint_angle(joint, 250.0f), 0.001f, "Clamp 250 deg to max");
    }

    // Custom limits test
    JointLimits custom = { 1000, 2000, -90.0f, 90.0f };
    ASSERT_NEAR(-90.0f, clamp_joint_angle(ARM_JOINT_BASE, -120.0f, &custom), 0.001f, "Custom negative clamp");
    ASSERT_NEAR(0.0f, clamp_joint_angle(ARM_JOINT_BASE, 0.0f, &custom), 0.001f, "Custom center angle");
    ASSERT_NEAR(90.0f, clamp_joint_angle(ARM_JOINT_BASE, 100.0f, &custom), 0.001f, "Custom positive clamp");
}

/**
 * @brief Test 4: Test 0 deg -> 1000 us, 90 deg -> 1500 us, 180 deg -> 2000 us, and roundtrip accuracy.
 */
void test_angle_pulse_roundtrip() {
    for (int i = 0; i < ARM_NUM_JOINTS; ++i) {
        ArmJointId joint = static_cast<ArmJointId>(i);

        // Required canonical conversions: 0 deg -> 1000 us, 90 deg -> 1500 us, 180 deg -> 2000 us
        ASSERT_EQ(1000, angle_to_pulse(joint, 0.0f), "0 deg must map to 1000 us");
        ASSERT_EQ(1500, angle_to_pulse(joint, 90.0f), "90 deg must map to 1500 us");
        ASSERT_EQ(2000, angle_to_pulse(joint, 180.0f), "180 deg must map to 2000 us");

        // Reverse conversions
        ASSERT_NEAR(0.0f, pulse_to_angle(joint, 1000), 0.01f, "1000 us must map to 0 deg");
        ASSERT_NEAR(90.0f, pulse_to_angle(joint, 1500), 0.01f, "1500 us must map to 90 deg");
        ASSERT_NEAR(180.0f, pulse_to_angle(joint, 2000), 0.01f, "2000 us must map to 180 deg");

        // Intermediate fractional steps
        ASSERT_EQ(1250, angle_to_pulse(joint, 45.0f), "45 deg must map to 1250 us");
        ASSERT_EQ(1750, angle_to_pulse(joint, 135.0f), "135 deg must map to 1750 us");
        ASSERT_NEAR(45.0f, pulse_to_angle(joint, 1250), 0.01f, "1250 us must map to 45 deg");
        ASSERT_NEAR(135.0f, pulse_to_angle(joint, 1750), 0.01f, "1750 us must map to 135 deg");

        // Out-of-bounds input conversions should clamp
        ASSERT_EQ(1000, angle_to_pulse(joint, -25.0f), "Negative angle clamped to 1000 us");
        ASSERT_EQ(2000, angle_to_pulse(joint, 210.0f), "Over-limit angle clamped to 2000 us");
        ASSERT_NEAR(0.0f, pulse_to_angle(joint, 500), 0.01f, "Under-limit pulse clamped to 0 deg");
        ASSERT_NEAR(180.0f, pulse_to_angle(joint, 2500), 0.01f, "Over-limit pulse clamped to 180 deg");

        // Sweep angle -> pulse -> angle roundtrip across 0..180 deg
        for (float deg = 0.0f; deg <= 180.0f; deg += 5.0f) {
            uint16_t pulse = angle_to_pulse(joint, deg);
            float back_deg = pulse_to_angle(joint, pulse);
            // 1 us quantization corresponds to 0.18 degrees (180 deg / 1000 us)
            ASSERT_NEAR(deg, back_deg, 0.2f, "Angle roundtrip quantization tolerance");
        }

        // Sweep pulse -> angle -> pulse roundtrip across 1000..2000 us
        for (uint16_t pulse = 1000; pulse <= 2000; pulse += 25) {
            float deg = pulse_to_angle(joint, pulse);
            uint16_t back_pulse = angle_to_pulse(joint, deg);
            ASSERT_EQ(pulse, back_pulse, "Pulse roundtrip accuracy");
        }
    }
}

/**
 * @brief Test 5: Verify counts at 1000us, 1500us, 2000us, 600us, 2400us.
 */
void test_pca9685_pwm_count_conversion() {
    // Expected counts at 60Hz:
    // Period = 1,000,000 / 60 = 16,666.67 us
    // count = pulse_us * 4096 * 60 / 1,000,000 = pulse_us * 0.24576
    // 1000 us -> round(245.76) = 246
    // 1500 us -> round(368.64) = 369
    // 2000 us -> round(491.52) = 492
    // 600 us  -> round(147.456) = 147
    // 2400 us -> round(589.824) = 590

    ASSERT_EQ(246, us_to_pca9685_count(1000), "PCA9685 count at 1000 us");
    ASSERT_EQ(369, us_to_pca9685_count(1500), "PCA9685 count at 1500 us");
    ASSERT_EQ(492, us_to_pca9685_count(2000), "PCA9685 count at 2000 us");
    ASSERT_EQ(147, us_to_pca9685_count(600), "PCA9685 count at 600 us");
    ASSERT_EQ(590, us_to_pca9685_count(2400), "PCA9685 count at 2400 us");

    // Boundary checks
    ASSERT_EQ(0, us_to_pca9685_count(0), "PCA9685 count at 0 us");
    ASSERT_TRUE(us_to_pca9685_count(65535) <= 4095, "PCA9685 count max clamp at 4095");
}

/**
 * @brief Test 6: Trajectory stepping tests:
 * - Step toward higher target does not overshoot.
 * - Step toward lower target does not overshoot.
 * - Speed scaling: higher speed produces larger or equal step.
 * - Zero delta returns current pos.
 */
void test_smooth_trajectory_step() {
    // 1. Step toward higher target does not overshoot
    uint16_t cur = 1000;
    uint16_t target = 1500;
    uint16_t next = calculate_step(cur, target, ARM_SPEED_DEFAULT, 50); // speed 5, 50ms -> step = 5 * 0.2 * 50 = 50us
    ASSERT_TRUE(next > cur, "Step should advance toward higher target");
    ASSERT_TRUE(next <= target, "Step must not overshoot higher target");
    ASSERT_EQ(1050, next, "Step with speed 5 for 50ms should add 50 us");

    // Close to target: does not overshoot
    cur = 1490;
    next = calculate_step(cur, target, ARM_SPEED_DEFAULT, 50);
    ASSERT_EQ(target, next, "Close target must snap to target without overshooting");

    // 2. Step toward lower target does not overshoot
    cur = 1500;
    target = 1000;
    next = calculate_step(cur, target, ARM_SPEED_DEFAULT, 50);
    ASSERT_TRUE(next < cur, "Step should decrease toward lower target");
    ASSERT_TRUE(next >= target, "Step must not overshoot lower target");
    ASSERT_EQ(1450, next, "Step with speed 5 for 50ms should subtract 50 us");

    // Close to target: does not undershoot
    cur = 1010;
    next = calculate_step(cur, target, ARM_SPEED_DEFAULT, 50);
    ASSERT_EQ(target, next, "Close lower target must snap to target without overshooting");

    // 3. Speed scaling: higher speed produces larger or equal step
    uint16_t prev_step_size = 0;
    for (uint8_t spd = ARM_SPEED_MIN; spd <= ARM_SPEED_MAX; ++spd) {
        uint16_t step_pos = calculate_step(1000, 2000, spd, 20); // 20ms
        uint16_t step_size = step_pos - 1000;
        ASSERT_TRUE(step_size >= prev_step_size, "Higher speed must produce larger or equal step");
        prev_step_size = step_size;
    }

    // 4. Zero delta tests
    // dt_ms == 0 returns current position
    ASSERT_EQ(1350, calculate_step(1350, 1800, ARM_SPEED_DEFAULT, 0), "Zero dt_ms returns current pos");
    ASSERT_EQ(1350, calculate_step(1350, 1100, ARM_SPEED_DEFAULT, 0), "Zero dt_ms returns current pos");

    // current == target returns target
    ASSERT_EQ(1500, calculate_step(1500, 1500, ARM_SPEED_DEFAULT, 100), "Zero pos delta returns current pos");

    // Speed clamping: below min uses min, above max uses max
    uint16_t step_below = calculate_step(1000, 2000, 0, 50);
    uint16_t step_min = calculate_step(1000, 2000, ARM_SPEED_MIN, 50);
    ASSERT_EQ(step_min, step_below, "Speed 0 clamped to ARM_SPEED_MIN");

    uint16_t step_above = calculate_step(1000, 2000, 99, 50);
    uint16_t step_max = calculate_step(1000, 2000, ARM_SPEED_MAX, 50);
    ASSERT_EQ(step_max, step_above, "Speed 99 clamped to ARM_SPEED_MAX");

    // Simulation convergence loop: verify multi-step trajectory reaches target exactly
    uint16_t sim_pos = 1000;
    uint16_t sim_target = 1800;
    int iterations = 0;
    while (sim_pos != sim_target && iterations < 1000) {
        sim_pos = calculate_step(sim_pos, sim_target, 5, 20); // 20ms cycles
        iterations++;
    }
    ASSERT_EQ(sim_target, sim_pos, "Trajectory simulation must converge exactly to target");
    ASSERT_TRUE(iterations < 50, "Convergence should happen within expected cycle count");
}

/**
 * @brief Test 7: Ensure all 6 joints in HOME and REST presets are within valid limits.
 */
void test_home_and_rest_preset_validity() {
    // HOME preset values check
    const uint16_t expected_home[ARM_NUM_JOINTS] = {1900, 1600, 1200, 1200, 1500, 1500};
    for (int i = 0; i < ARM_NUM_JOINTS; ++i) {
        ArmJointId joint = static_cast<ArmJointId>(i);
        uint16_t home_us = ARM_HOME_POS_US[i];

        ASSERT_EQ(expected_home[i], home_us, "HOME preset match expected value");
        ASSERT_TRUE(home_us >= DEFAULT_JOINT_LIMITS[i].min_us, "HOME pulse >= min_us");
        ASSERT_TRUE(home_us <= DEFAULT_JOINT_LIMITS[i].max_us, "HOME pulse <= max_us");
        ASSERT_EQ(home_us, clamp_joint_pulse(joint, home_us), "HOME preset is inside clamped bounds");

        // Verify angle conversion is also valid
        float home_deg = pulse_to_angle(joint, home_us);
        ASSERT_TRUE(home_deg >= DEFAULT_JOINT_LIMITS[i].min_deg, "HOME deg >= min_deg");
        ASSERT_TRUE(home_deg <= DEFAULT_JOINT_LIMITS[i].max_deg, "HOME deg <= max_deg");
    }

    // REST preset values check
    const uint16_t expected_rest[ARM_NUM_JOINTS] = {1500, 1000, 1000, 1000, 1500, 1500};
    for (int i = 0; i < ARM_NUM_JOINTS; ++i) {
        ArmJointId joint = static_cast<ArmJointId>(i);
        uint16_t rest_us = ARM_REST_POS_US[i];

        ASSERT_EQ(expected_rest[i], rest_us, "REST preset match expected value");
        ASSERT_TRUE(rest_us >= DEFAULT_JOINT_LIMITS[i].min_us, "REST pulse >= min_us");
        ASSERT_TRUE(rest_us <= DEFAULT_JOINT_LIMITS[i].max_us, "REST pulse <= max_us");
        ASSERT_EQ(rest_us, clamp_joint_pulse(joint, rest_us), "REST preset is inside clamped bounds");

        // Verify angle conversion is also valid
        float rest_deg = pulse_to_angle(joint, rest_us);
        ASSERT_TRUE(rest_deg >= DEFAULT_JOINT_LIMITS[i].min_deg, "REST deg >= min_deg");
        ASSERT_TRUE(rest_deg <= DEFAULT_JOINT_LIMITS[i].max_deg, "REST deg <= max_deg");
    }
}

/**
 * @brief Test 8: Implement a MockPwmDriver : public IPwmDriver, test initialization
 * and set_pulse_us(channel, pulse). Verify channels 0..5 receive expected pulse widths.
 */
void test_mock_pwm_driver() {
    MockPwmDriver driver;

    // Initial state before init()
    ASSERT_FALSE(driver.initialized, "Mock driver not initialized initially");
    ASSERT_EQ(0u, driver.total_writes, "No writes before start");
    for (int i = 0; i < ARM_NUM_JOINTS; ++i) {
        ASSERT_EQ(0, static_cast<int>(driver.channel_pulses[i]), "Channel pulse initialized to 0");
        ASSERT_EQ(0u, driver.write_counts[i], "Channel write count initialized to 0");
    }

    // Test initialization
    bool init_res = driver.init();
    ASSERT_TRUE(init_res, "Driver init must return true");
    ASSERT_TRUE(driver.initialized, "Driver initialized flag must be true");

    // Test setting arbitrary distinct pulses on channels 0..5
    const uint16_t test_pulses[ARM_NUM_JOINTS] = {1100, 1300, 1500, 1700, 1900, 1450};
    for (uint8_t ch = 0; ch < ARM_NUM_JOINTS; ++ch) {
        driver.set_pulse_us(ch, test_pulses[ch]);
        ASSERT_EQ(test_pulses[ch], driver.channel_pulses[ch], "Channel pulse received correctly");
        ASSERT_EQ(1u, driver.write_counts[ch], "Channel write count incremented");
    }
    ASSERT_EQ(6u, driver.total_writes, "Total writes equals 6");

    // Test driving arm to HOME preset via mock driver
    for (uint8_t ch = 0; ch < ARM_NUM_JOINTS; ++ch) {
        driver.set_pulse_us(ch, ARM_HOME_POS_US[ch]);
    }
    for (uint8_t ch = 0; ch < ARM_NUM_JOINTS; ++ch) {
        ASSERT_EQ(ARM_HOME_POS_US[ch], driver.channel_pulses[ch], "Mock received HOME pulse");
        ASSERT_EQ(2u, driver.write_counts[ch], "Channel write count is 2");
    }
    ASSERT_EQ(12u, driver.total_writes, "Total writes equals 12");

    // Test driving arm to REST preset via mock driver
    for (uint8_t ch = 0; ch < ARM_NUM_JOINTS; ++ch) {
        driver.set_pulse_us(ch, ARM_REST_POS_US[ch]);
    }
    for (uint8_t ch = 0; ch < ARM_NUM_JOINTS; ++ch) {
        ASSERT_EQ(ARM_REST_POS_US[ch], driver.channel_pulses[ch], "Mock received REST pulse");
        ASSERT_EQ(3u, driver.write_counts[ch], "Channel write count is 3");
    }

    // Out-of-range channel safety check
    driver.set_pulse_us(ARM_NUM_JOINTS, 1500);     // Channel 6 (out of range)
    driver.set_pulse_us(255, 1500);                 // Channel 255 (out of range)
    ASSERT_EQ(18u, driver.total_writes, "Total writes should not increment for invalid channels");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << ANSI_BOLD ANSI_CYAN
              << "========================================================\n"
              << "   Mars Rover - Robotic Arm Kinematics & Driver Tests   \n"
              << "========================================================"
              << ANSI_RESET << std::endl;

    run_suite("test_joint_naming_and_config", test_joint_naming_and_config);
    run_suite("test_joint_pulse_clamping", test_joint_pulse_clamping);
    run_suite("test_joint_angle_clamping", test_joint_angle_clamping);
    run_suite("test_angle_pulse_roundtrip", test_angle_pulse_roundtrip);
    run_suite("test_pca9685_pwm_count_conversion", test_pca9685_pwm_count_conversion);
    run_suite("test_smooth_trajectory_step", test_smooth_trajectory_step);
    run_suite("test_home_and_rest_preset_validity", test_home_and_rest_preset_validity);
    run_suite("test_mock_pwm_driver", test_mock_pwm_driver);

    std::cout << ANSI_BOLD ANSI_CYAN
              << "========================================================\n"
              << "                      TEST SUMMARY                      \n"
              << "========================================================"
              << ANSI_RESET << std::endl;

    std::cout << "  Suites total:  " << g_ctx.total_suites << "\n"
              << "  Suites passed: " ANSI_GREEN << g_ctx.passed_suites << ANSI_RESET "\n";
    if (g_ctx.failed_suites > 0) {
        std::cout << "  Suites failed: " ANSI_RED << g_ctx.failed_suites << ANSI_RESET "\n";
    }
    std::cout << "  Assertions:    " << g_ctx.total_assertions << "\n"
              << "  Assert failed: " << (g_ctx.failed_assertions > 0 ? ANSI_RED : ANSI_GREEN)
              << g_ctx.failed_assertions << ANSI_RESET << "\n";

    if (g_ctx.failed_suites == 0 && g_ctx.failed_assertions == 0) {
        std::cout << ANSI_BOLD ANSI_GREEN
                  << ">>> ALL UNIT TESTS PASSED SUCCESSFULLY! <<<\n"
                  << ANSI_RESET << std::endl;
        return 0;
    } else {
        std::cout << ANSI_BOLD ANSI_RED
                  << ">>> UNIT TESTS FAILED! <<<\n"
                  << ANSI_RESET << std::endl;
        return 1;
    }
}
