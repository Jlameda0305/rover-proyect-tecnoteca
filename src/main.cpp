#include <Arduino.h>
#include "arm.h"
#include "wifi_controller.h"

static bool s_client_connected = false;

static void handle_wifi_status(WifiControllerStatus status) {
    if (status == WIFI_CONTROLLER_CONNECTED) {
        Serial.println("[MAIN] Client connected via WebSocket");
        s_client_connected = true;
    } else {
        Serial.println("[MAIN] Client disconnected - stopping all joint movement");
        s_client_connected = false;
        arm_emergency_stop();
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n==================================================");
    Serial.println("  Mars Rover - Standalone 6-DOF Robotic Arm System ");
    Serial.println("==================================================");

    // Initialize robotic arm: I2C, PCA9685 driver, background FreeRTOS updater, and position at Home
    arm_init();
    Serial.println("[MAIN] Arm subsystem initialized at Home position");

    // Initialize WiFi in AP mode (SSID: "Robotic-Arm", IP: 192.168.4.5)
    wifi_controller_init("Robotic-Arm", nullptr, WIFI_CONTROLLER_AP);
    wifi_controller_register_connection_callback(&handle_wifi_status);
    Serial.println("[MAIN] WiFi AP 'Robotic-Arm' initialized (IP: 192.168.4.5)");

    Serial.println("[MAIN] Setup complete. Ready for client connections.");
}

void loop() {
    // If client is disconnected, hold position and prevent uncommanded drift
    if (!s_client_connected) {
        arm_emergency_stop();
        vTaskDelay(pdMS_TO_TICKS(20));
        return;
    }

    // Read joint channel values from wifi_controller
    // Channel mapping:
    // Ch 0: Base
    // Ch 1: Shoulder
    // Ch 2: Elbow
    // Ch 3: Wrist Pitch
    // Ch 4: Wrist Roll
    // Ch 5: Gripper
    for (uint8_t ch = 0; ch < ARM_NUM_JOINTS; ch++) {
        uint16_t pulse = wifi_controller_get_val(ch);
        if (pulse == 0) {
            pulse = 1500;
        }

        // Deadband between 1480us and 1520us
        if (pulse >= 1480 && pulse <= 1520) {
            arm_pause(static_cast<ArmJointId>(ch));
        } else if (pulse < 1480) {
            // Deflected negative towards minimum limit
            if (pulse < 1000) pulse = 1000;
            uint16_t deflection = 1500 - pulse; // Range 21..500
            uint8_t speed = static_cast<uint8_t>(map(deflection, 20, 500, ARM_SPEED_MIN, ARM_SPEED_MAX));
            if (speed < ARM_SPEED_MIN) speed = ARM_SPEED_MIN;
            if (speed > ARM_SPEED_MAX) speed = ARM_SPEED_MAX;
            arm_jog(static_cast<ArmJointId>(ch), -1, speed);
        } else {
            // Deflected positive towards maximum limit
            if (pulse > 2000) pulse = 2000;
            uint16_t deflection = pulse - 1500; // Range 21..500
            uint8_t speed = static_cast<uint8_t>(map(deflection, 20, 500, ARM_SPEED_MIN, ARM_SPEED_MAX));
            if (speed < ARM_SPEED_MIN) speed = ARM_SPEED_MIN;
            if (speed > ARM_SPEED_MAX) speed = ARM_SPEED_MAX;
            arm_jog(static_cast<ArmJointId>(ch), +1, speed);
        }
    }

    // Periodically broadcast telemetry to connected WebSocket clients (10 Hz)
    static uint32_t s_last_broadcast_ms = 0;
    uint32_t now_ms = millis();
    if (now_ms - s_last_broadcast_ms >= 100) {
        s_last_broadcast_ms = now_ms;
        wifi_controller_broadcast_arm_state();
    }

    vTaskDelay(pdMS_TO_TICKS(20));
}

