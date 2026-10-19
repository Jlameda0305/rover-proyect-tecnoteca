/*
 * Simplified Rover RC Firmware
 * Controlled exclusively as an RC car via WiFi Access Point ("Rover") using WebSockets.
 * Hardware features such as arm, camera head, LoRa and RMT receiver are disabled.
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include "rover_config.h"
#include "wifi_controller.h"
#include "rover_driving.h"
#include "rover_kinematics.h"

#define DEBUG

#ifdef DEBUG
#define LOG Serial.print
#define LOGF Serial.printf
#define LOGLN Serial.println
#else
#define LOG(msg)
#define LOGF(...)
#define LOGLN(msg)
#endif

static const char* TAG = "ROVER_MAIN";

static uint16_t rc_values[RC_NUM_CHANNELS][RC_FILTER_SAMPLES];
static uint8_t rc_value_index = 0;

static RoverMode current_rover_mode = DRIVE_TURN_NORMAL;
static bool wifi_control_enabled = false;

static void handle_wifi_controller_status(WifiControllerStatus status);
static void handle_controller_disconnected(uint16_t last_sampled_signal);
static uint16_t filter_signal(uint16_t* signals);

static uint32_t last_telemetry_ms = 0;
static uint16_t serial_steer = RC_CENTER;
static uint16_t serial_throttle = RC_CENTER;
static RoverMode serial_mode = DRIVE_TURN_NORMAL;
static bool serial_override = false;

static void process_serial_commands() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        char c = toupper(line[0]);
        if (c == 'F') { // Forward
            serial_override = true;
            serial_throttle = 1800;
            serial_steer = 1500;
            serial_mode = DRIVE_TURN_NORMAL;
            Serial.println("[CMD_ACK] FORWARD: Throttle 1800 us, Steer 1500 us");
        } else if (c == 'B') { // Backward
            serial_override = true;
            serial_throttle = 1200;
            serial_steer = 1500;
            serial_mode = DRIVE_TURN_NORMAL;
            Serial.println("[CMD_ACK] BACKWARD: Throttle 1200 us, Steer 1500 us");
        } else if (c == 'L') { // Left
            serial_override = true;
            serial_throttle = 1600;
            serial_steer = 1200;
            serial_mode = DRIVE_TURN_NORMAL;
            Serial.println("[CMD_ACK] TURN LEFT: Throttle 1600 us, Steer 1200 us");
        } else if (c == 'R') { // Right
            serial_override = true;
            serial_throttle = 1600;
            serial_steer = 1800;
            serial_mode = DRIVE_TURN_NORMAL;
            Serial.println("[CMD_ACK] TURN RIGHT: Throttle 1600 us, Steer 1800 us");
        } else if (c == 'S') { // Stop
            serial_override = false;
            serial_throttle = 1500;
            serial_steer = 1500;
            serial_mode = DRIVE_TURN_NORMAL;
            Serial.println("[CMD_ACK] STOP / NEUTRAL: Throttle 1500 us, Steer 1500 us");
        } else if (c == 'P') { // Pivot / Spin
            serial_override = true;
            serial_throttle = 1500;
            serial_steer = 1800;
            serial_mode = DRIVE_TURN_SPIN;
            Serial.println("[CMD_ACK] SPIN 360: Mode SPIN, Steer 1800 us");
        } else if (c == 'T') { // Run Self-Test
            Serial.println("[CMD_ACK] RUNNING SELF-TEST...");
            run_rover_kinematics_self_test();
        } else if (line.startsWith("CMD:")) {
            int comma1 = line.indexOf(',');
            int comma2 = line.indexOf(',', comma1 + 1);
            if (comma1 > 0 && comma2 > 0) {
                serial_override = true;
                serial_steer = line.substring(4, comma1).toInt();
                serial_throttle = line.substring(comma1 + 1, comma2).toInt();
                int m = line.substring(comma2 + 1).toInt();
                serial_mode = (m == 1) ? DRIVE_TURN_SPIN : DRIVE_TURN_NORMAL;
                Serial.printf("[CMD_ACK] CUSTOM: Steer=%u us, Throttle=%u us, Mode=%s\n",
                              serial_steer, serial_throttle,
                              (serial_mode == DRIVE_TURN_SPIN ? "SPIN" : "NORMAL"));
            }
        }
    }
}

void setup() {
    Serial.begin(SERIAL_PORT_SPEED);
    delay(500);
    Serial.println();
    Serial.println("=================================");
    Serial.println(" Mars Rover Firmware (Wokwi Sim)");
    Serial.println(" Mode: Standalone RC / WebSockets");
    Serial.println(" Commands: F, B, L, R, S, P");
    Serial.println("=================================");

    rover_driving_init();
    handle_controller_disconnected(0);

    // Run built-in kinematics verification
    run_rover_kinematics_self_test();

    wifi_controller_init("Rover", NULL, WIFI_CONTROLLER_AP);
    wifi_controller_register_connection_callback(&handle_wifi_controller_status);

    Serial.printf("Rover Ready (Simplified RC)! Core: %d\n", xPortGetCoreID());
}

void loop() {
    process_serial_commands();

    uint16_t steer_raw;
    uint16_t motor_raw;
    RoverMode mode;

    if (serial_override) {
        steer_raw = serial_steer;
        motor_raw = serial_throttle;
        mode = serial_mode;
    } else {
        steer_raw = wifi_controller_get_ch_val(RC_STEER_CHANNEL);
        motor_raw = wifi_controller_get_ch_val(RC_MOTOR_CHANNEL);
        uint16_t mode_raw  = wifi_controller_get_ch_val(RC_ROVER_MODE_ROVER_CHANNEL);
        mode = (mode_raw >= 1400 && mode_raw <= 1600) ? DRIVE_TURN_SPIN : DRIVE_TURN_NORMAL;
    }

    rc_values[RC_STEER_CHANNEL][rc_value_index] = steer_raw;
    rc_values[RC_MOTOR_CHANNEL][rc_value_index] = motor_raw;

    if (mode != current_rover_mode) {
        current_rover_mode = mode;
        rover_driving_set_drive_mode(current_rover_mode);
    }

    uint16_t motor_signal = filter_signal(rc_values[RC_MOTOR_CHANNEL]);
    uint16_t steer_signal = filter_signal(rc_values[RC_STEER_CHANNEL]);

    // Differential Skid-Steering: pass both throttle and steer signals
    rover_driving_move(motor_signal, steer_signal);

    uint32_t now = millis();
    if (now - last_telemetry_ms >= 1000) {
        last_telemetry_ms = now;
        Serial.printf("[TELEMETRY] Mode: %s | Motor: %u us | Steer: %u us\n",
                      (current_rover_mode == DRIVE_TURN_SPIN ? "SPIN" : "NORMAL"),
                      motor_signal,
                      steer_signal);
    }

    rc_value_index = (rc_value_index + 1) % RC_FILTER_SAMPLES;
    vTaskDelay(pdMS_TO_TICKS(20));
}

static void handle_wifi_controller_status(WifiControllerStatus status) {
    LOGF("Wifi Controller status: %d\n", status);
    if (status != WIFI_CONTROLLER_CONNECTED) {
        handle_controller_disconnected(0);
        wifi_control_enabled = false;
    } else {
        wifi_control_enabled = true;
    }
}

static void handle_controller_disconnected(uint16_t last_sampled_signal) {
    // Set all RC values to mid position if signal is 0 (controller disconnected)
    if (last_sampled_signal == 0) {
        for (uint8_t channel = 0; channel < RC_NUM_CHANNELS; channel++) {
            for (uint8_t sample = 0; sample < RC_FILTER_SAMPLES; sample++) {
                rc_values[channel][sample] = RC_CENTER;
            }
        }
    }
}

static uint16_t filter_signal(uint16_t* signals) {
    return filter_rc_signal(signals, RC_FILTER_SAMPLES);
}

