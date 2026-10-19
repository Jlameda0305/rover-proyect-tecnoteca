#include "arm_kinematics.h"
#include <cmath>
#include <algorithm>

static const JointLimits& resolve_limits(ArmJointId joint, const JointLimits* custom_limits) {
    if (custom_limits != nullptr) {
        return *custom_limits;
    }
    int joint_idx = static_cast<int>(joint);
    if (joint_idx >= 0 && joint_idx < static_cast<int>(ARM_NUM_JOINTS)) {
        return DEFAULT_JOINT_LIMITS[joint_idx];
    }
    static constexpr JointLimits fallback_limits = { 1000, 2000, 0.0f, 180.0f };
    return fallback_limits;
}

uint16_t clamp_joint_pulse(ArmJointId joint, uint16_t pulse_us, const JointLimits* custom_limits) {
    const JointLimits& limits = resolve_limits(joint, custom_limits);
    if (pulse_us < limits.min_us) {
        return limits.min_us;
    }
    if (pulse_us > limits.max_us) {
        return limits.max_us;
    }
    return pulse_us;
}

float clamp_joint_angle(ArmJointId joint, float angle_deg, const JointLimits* custom_limits) {
    const JointLimits& limits = resolve_limits(joint, custom_limits);
    if (angle_deg < limits.min_deg) {
        return limits.min_deg;
    }
    if (angle_deg > limits.max_deg) {
        return limits.max_deg;
    }
    return angle_deg;
}

uint16_t angle_to_pulse(ArmJointId joint, float angle_deg, const JointLimits* custom_limits) {
    const JointLimits& limits = resolve_limits(joint, custom_limits);
    float clamped_deg = clamp_joint_angle(joint, angle_deg, custom_limits);
    float deg_span = limits.max_deg - limits.min_deg;
    if (deg_span <= 0.0001f) {
        return limits.min_us;
    }
    float ratio = (clamped_deg - limits.min_deg) / deg_span;
    float pulse = static_cast<float>(limits.min_us) + ratio * static_cast<float>(limits.max_us - limits.min_us);
    uint16_t pulse_int = static_cast<uint16_t>(pulse + 0.5f);
    return clamp_joint_pulse(joint, pulse_int, custom_limits);
}

float pulse_to_angle(ArmJointId joint, uint16_t pulse_us, const JointLimits* custom_limits) {
    const JointLimits& limits = resolve_limits(joint, custom_limits);
    uint16_t clamped_us = clamp_joint_pulse(joint, pulse_us, custom_limits);
    float us_span = static_cast<float>(limits.max_us - limits.min_us);
    if (us_span <= 0.0001f) {
        return limits.min_deg;
    }
    float ratio = static_cast<float>(clamped_us - limits.min_us) / us_span;
    float angle = limits.min_deg + ratio * (limits.max_deg - limits.min_deg);
    return clamp_joint_angle(joint, angle, custom_limits);
}

uint16_t us_to_pca9685_count(uint16_t pulse_us) {
    // PCA9685 count formula at 60 Hz:
    // Period = 1,000,000 us / 60 Hz = 16666.67 us
    // Count = (pulse_us * 4096 * 60) / 1,000,000 = pulse_us * 0.24576
    // 1000 us -> ~246, 1500 us -> ~369, 2000 us -> ~492
    float count_f = (static_cast<float>(pulse_us) * 4096.0f * static_cast<float>(PCA9685_PWM_FREQ)) / 1000000.0f;
    if (count_f < 0.0f) {
        count_f = 0.0f;
    }
    if (count_f > 4095.0f) {
        count_f = 4095.0f;
    }
    return static_cast<uint16_t>(count_f + 0.5f);
}

uint16_t calculate_step(uint16_t current_us, uint16_t target_us, uint8_t speed_level, uint32_t dt_ms) {
    if (current_us == target_us) {
        return target_us;
    }
    if (dt_ms == 0) {
        return current_us;
    }

    if (speed_level < ARM_SPEED_MIN) {
        speed_level = ARM_SPEED_MIN;
    }
    if (speed_level > ARM_SPEED_MAX) {
        speed_level = ARM_SPEED_MAX;
    }

    // Step calculation based on speed level and delta time
    float speed_factor = static_cast<float>(speed_level) * ARM_SPEED_BASE_US_PER_MS;
    float max_step_f = speed_factor * static_cast<float>(dt_ms);
    uint16_t max_step = static_cast<uint16_t>(max_step_f + 0.5f);
    if (max_step == 0) {
        max_step = 1;
    }

    if (current_us < target_us) {
        uint16_t remaining = target_us - current_us;
        if (remaining <= max_step) {
            return target_us;
        }
        return current_us + max_step;
    } else {
        uint16_t remaining = current_us - target_us;
        if (remaining <= max_step) {
            return target_us;
        }
        return current_us - max_step;
    }
}
