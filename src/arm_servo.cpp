#include "arm_servo.h"
#include "arm_kinematics.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char* TAG = "arm_servo";

struct ArmChannelState {
    uint16_t current_us;
    uint16_t target_us;
    uint8_t  speed;
    bool     is_moving;
};

static Adafruit_PWMServoDriver s_pwm_driver = Adafruit_PWMServoDriver(PCA9685_I2C_ADDR, Wire);
static SemaphoreHandle_t s_arm_mutex = nullptr;
static TaskHandle_t s_updater_task_handle = nullptr;
static ArmChannelState s_channels[ARM_NUM_JOINTS];
static bool s_is_initialized = false;

static void arm_servo_updater_task(void* arg) {
    (void)arg;
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(10); // 10ms interval

    while (true) {
        vTaskDelayUntil(&last_wake_time, interval);

        if (s_arm_mutex != nullptr && xSemaphoreTake(s_arm_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            for (uint8_t ch = 0; ch < ARM_NUM_JOINTS; ch++) {
                if (s_channels[ch].is_moving) {
                    uint16_t cur = s_channels[ch].current_us;
                    uint16_t tgt = s_channels[ch].target_us;
                    if (cur == tgt) {
                        s_channels[ch].is_moving = false;
                    } else {
                        uint16_t next_us = calculate_step(cur, tgt, s_channels[ch].speed, 10);
                        s_channels[ch].current_us = next_us;
                        if (next_us == tgt) {
                            s_channels[ch].is_moving = false;
                        }
                        uint16_t count = us_to_pca9685_count(next_us);
                        s_pwm_driver.setPWM(ch, 0, count);
                    }
                }
            }
            xSemaphoreGive(s_arm_mutex);
        }
    }
}

bool arm_servo_init(void) {
    if (s_is_initialized) {
        return true;
    }

    if (s_arm_mutex == nullptr) {
        s_arm_mutex = xSemaphoreCreateMutex();
        if (s_arm_mutex == nullptr) {
            ESP_LOGE(TAG, "Failed to create arm mutex");
            return false;
        }
    }

    // Configure Wire for PCA9685 on SDA pin 21, SCL pin 22
    Wire.begin(ARM_I2C_SDA_PIN, ARM_I2C_SCL_PIN);
    Wire.setClock(400000);

    xSemaphoreTake(s_arm_mutex, portMAX_DELAY);
    s_pwm_driver.begin();
    s_pwm_driver.setPWMFreq(PCA9685_PWM_FREQ);

    // Initialize all 6 channels to calibrated Home positions
    for (uint8_t i = 0; i < ARM_NUM_JOINTS; i++) {
        s_channels[i].current_us = ARM_HOME_POS_US[i];
        s_channels[i].target_us = ARM_HOME_POS_US[i];
        s_channels[i].speed = ARM_SPEED_DEFAULT;
        s_channels[i].is_moving = false;

        uint16_t count = us_to_pca9685_count(s_channels[i].current_us);
        s_pwm_driver.setPWM(i, 0, count);
    }
    xSemaphoreGive(s_arm_mutex);

    // Launch FreeRTOS background task ArmServoUpdater
    BaseType_t res = xTaskCreate(
        arm_servo_updater_task,
        "ArmServoUpdater",
        2048,
        nullptr,
        5,
        &s_updater_task_handle
    );

    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ArmServoUpdater task");
        return false;
    }

    s_is_initialized = true;
    ESP_LOGI(TAG, "ArmServo initialized successfully (60Hz, channels 0..5)");
    return true;
}

void arm_servo_write_us(uint8_t channel, uint16_t us) {
    if (channel >= ARM_NUM_JOINTS) {
        return;
    }
    uint16_t clamped_us = clamp_joint_pulse(static_cast<ArmJointId>(channel), us);
    uint16_t count = us_to_pca9685_count(clamped_us);

    if (s_arm_mutex != nullptr) {
        xSemaphoreTake(s_arm_mutex, portMAX_DELAY);
    }
    s_channels[channel].current_us = clamped_us;
    s_channels[channel].target_us = clamped_us;
    s_channels[channel].is_moving = false;
    s_pwm_driver.setPWM(channel, 0, count);
    if (s_arm_mutex != nullptr) {
        xSemaphoreGive(s_arm_mutex);
    }
}

void arm_servo_move_to_us(uint8_t channel, uint16_t target_us, uint8_t speed) {
    if (channel >= ARM_NUM_JOINTS) {
        return;
    }
    uint16_t clamped_us = clamp_joint_pulse(static_cast<ArmJointId>(channel), target_us);
    if (speed < ARM_SPEED_MIN) speed = ARM_SPEED_MIN;
    if (speed > ARM_SPEED_MAX) speed = ARM_SPEED_MAX;

    if (s_arm_mutex != nullptr) {
        xSemaphoreTake(s_arm_mutex, portMAX_DELAY);
    }
    s_channels[channel].target_us = clamped_us;
    s_channels[channel].speed = speed;
    s_channels[channel].is_moving = (s_channels[channel].current_us != clamped_us);
    if (s_arm_mutex != nullptr) {
        xSemaphoreGive(s_arm_mutex);
    }
}

void arm_servo_pause(uint8_t channel) {
    if (channel >= ARM_NUM_JOINTS) {
        return;
    }
    if (s_arm_mutex != nullptr) {
        xSemaphoreTake(s_arm_mutex, portMAX_DELAY);
    }
    s_channels[channel].is_moving = false;
    s_channels[channel].target_us = s_channels[channel].current_us;
    if (s_arm_mutex != nullptr) {
        xSemaphoreGive(s_arm_mutex);
    }
}

void arm_servo_emergency_stop(void) {
    if (s_arm_mutex != nullptr) {
        xSemaphoreTake(s_arm_mutex, portMAX_DELAY);
    }
    for (uint8_t i = 0; i < ARM_NUM_JOINTS; i++) {
        s_channels[i].is_moving = false;
        s_channels[i].target_us = s_channels[i].current_us;
    }
    if (s_arm_mutex != nullptr) {
        xSemaphoreGive(s_arm_mutex);
    }
}

uint16_t arm_servo_get_current_us(uint8_t channel) {
    if (channel >= ARM_NUM_JOINTS) {
        return 1500;
    }
    uint16_t val;
    if (s_arm_mutex != nullptr) {
        xSemaphoreTake(s_arm_mutex, portMAX_DELAY);
        val = s_channels[channel].current_us;
        xSemaphoreGive(s_arm_mutex);
    } else {
        val = s_channels[channel].current_us;
    }
    return val;
}

uint16_t arm_servo_get_target_us(uint8_t channel) {
    if (channel >= ARM_NUM_JOINTS) {
        return 1500;
    }
    uint16_t val;
    if (s_arm_mutex != nullptr) {
        xSemaphoreTake(s_arm_mutex, portMAX_DELAY);
        val = s_channels[channel].target_us;
        xSemaphoreGive(s_arm_mutex);
    } else {
        val = s_channels[channel].target_us;
    }
    return val;
}

bool arm_servo_is_moving(uint8_t channel) {
    if (channel >= ARM_NUM_JOINTS) {
        return false;
    }
    bool val;
    if (s_arm_mutex != nullptr) {
        xSemaphoreTake(s_arm_mutex, portMAX_DELAY);
        val = s_channels[channel].is_moving;
        xSemaphoreGive(s_arm_mutex);
    } else {
        val = s_channels[channel].is_moving;
    }
    return val;
}
