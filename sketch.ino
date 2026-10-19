/**
 * ============================================================================
 * MARS ROVER - STANDALONE RC & SIMULATION FIRMWARE
 * Compatible with Wokwi Web & Wokwi CLI (ESP32 DevKit-C v4)
 * ============================================================================
 * 
 * Hardware Connections - L298N Dual H-Bridge Motor Driver (4 DC Motors):
 *  - GPIO 17: ENA (Left Motors PWM Speed Control - LEDC Channel 0)
 *  - GPIO 18: IN1 (Left Motors Direction A)
 *  - GPIO 19: IN2 (Left Motors Direction B)
 *  - GPIO 21: IN3 (Right Motors Direction A)
 *  - GPIO 22: IN4 (Right Motors Direction B)
 *  - GPIO 13: ENB (Right Motors PWM Speed Control - LEDC Channel 1)
 *  - TX / RX: Interactive Serial Console (115200 baud)
 * 
 * Steering Architecture:
 *  - Differential Skid-Steering (turning by speed & direction difference between left/right wheels)
 *  - No servomotors required.
 * 
 * Controls:
 *  - Serial Commands (Wokwi Monitor):
 *      'F' -> Forward (Throttle: 1800 us, Steer: 1500 us)
 *      'B' -> Backward (Throttle: 1200 us, Steer: 1500 us)
 *      'L' -> Turn Left (Throttle: 1600 us, Steer: 1200 us)
 *      'R' -> Turn Right (Throttle: 1600 us, Steer: 1800 us)
 *      'P' -> Spin 360 Mode (Differential opposite rotation)
 *      'S' -> Stop / Neutral (1500 us)
 *      'CMD:<steer>,<throttle>,<mode>' -> Custom values (e.g. CMD:1350,1750,0)
 *  - Web Dashboard:
 *      Connect to WiFi AP "Rover" or Wokwi-GUEST and open http://<IP>/
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <cmath>
#include <cstdlib>

// ============================================================================
// CONFIGURATION & HARDWARE CONSTANTS
// ============================================================================
#define ROVER_L298N_ENA_PIN        17    // PWM Speed Control Left (LEDC Channel 0)
#define ROVER_L298N_IN1_PIN        18    // Left Direction A
#define ROVER_L298N_IN2_PIN        19    // Left Direction B
#define ROVER_L298N_IN3_PIN        21    // Right Direction A
#define ROVER_L298N_IN4_PIN        22    // Right Direction B
#define ROVER_L298N_ENB_PIN        13    // PWM Speed Control Right (LEDC Channel 1)

#define L298N_PWM_FREQ             1000  // 1 kHz PWM frequency
#define L298N_PWM_RESOLUTION       8     // 8-bit resolution (0 - 255)
#define L298N_PWM_CHANNEL_LEFT     0
#define L298N_PWM_CHANNEL_RIGHT    1

#define SERIAL_PORT_SPEED          115200

#define RC_LOW                     1000
#define RC_CENTER                  1500
#define RC_HIGH                    2000
#define RC_DEADZONE_MIN            1480
#define RC_DEADZONE_MAX            1520
#define RC_FAILSAFE_THRESHOLD      800
#define RC_NUM_CHANNELS            6
#define RC_FILTER_SAMPLES          5

#define RC_STEER_CHANNEL           0
#define RC_MOTOR_CHANNEL           1
#define RC_MODE_CHANNEL            4

// ============================================================================
// DATA TYPES & ENUMS
// ============================================================================
enum RoverMode {
    DRIVE_TURN_NORMAL = 0,
    DRIVE_TURN_SPIN   = 1
};

enum MotorDirection {
    MOTOR_STOP = 0,
    MOTOR_FORWARD = 1,
    MOTOR_BACKWARD = 2
};

struct MotorOutputs {
    uint16_t left;             // RC equivalent us (1000..2000)
    uint16_t right;            // RC equivalent us (1000..2000)
    int16_t left_speed;        // Normalized speed: -500 to +500
    int16_t right_speed;       // Normalized speed: -500 to +500
    uint8_t left_pwm;          // Duty cycle for ENA (0 - 255)
    uint8_t right_pwm;         // Duty cycle for ENB (0 - 255)
    MotorDirection left_dir;   // IN1, IN2
    MotorDirection right_dir;  // IN3, IN4
};

// ============================================================================
// PURE KINEMATICS ENGINE
// ============================================================================
static inline int32_t clamp_val(int32_t val, int32_t low, int32_t high) {
    return (val < low) ? low : ((val > high) ? high : val);
}

static MotorOutputs calculate_motors(RoverMode mode, uint16_t throttle_signal, uint16_t steer_signal) {
    MotorOutputs out;

    uint16_t throttle = (throttle_signal <= RC_FAILSAFE_THRESHOLD) ? RC_CENTER : static_cast<uint16_t>(clamp_val(throttle_signal, RC_LOW, RC_HIGH));
    uint16_t steer    = (steer_signal <= RC_FAILSAFE_THRESHOLD)    ? RC_CENTER : static_cast<uint16_t>(clamp_val(steer_signal, RC_LOW, RC_HIGH));

    int16_t t_diff = static_cast<int16_t>(throttle) - RC_CENTER; // [-500, +500]
    int16_t s_diff = static_cast<int16_t>(steer) - RC_CENTER;    // [-500, +500]

    int16_t v_left = 0;
    int16_t v_right = 0;

    if (mode == DRIVE_TURN_SPIN) {
        // Spin Mode (360-degree point turn in place):
        // Steer Right (s_diff > 0): Left forward, Right backward
        // Steer Left (s_diff < 0): Left backward, Right forward
        v_left = s_diff;
        v_right = -s_diff;
    } else {
        // DRIVE_TURN_NORMAL: Skid-Steering Differential Mixer
        v_left = static_cast<int16_t>(clamp_val(t_diff + s_diff, -500, 500));
        v_right = static_cast<int16_t>(clamp_val(t_diff - s_diff, -500, 500));
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

static uint16_t filter_rc_signal(const uint16_t* samples, uint8_t sample_count) {
    if (!samples || sample_count == 0) return RC_CENTER;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < sample_count; ++i) {
        sum += samples[i];
    }
    uint16_t avg = static_cast<uint16_t>(sum / sample_count);

    if (avg <= RC_FAILSAFE_THRESHOLD) return RC_CENTER;
    if (avg >= RC_DEADZONE_MIN && avg <= RC_DEADZONE_MAX) return RC_CENTER;
    if (avg < 1010) return RC_LOW;
    if (avg > 1990) return RC_HIGH;

    return static_cast<uint16_t>(clamp_val(avg, RC_LOW, RC_HIGH));
}

// ============================================================================
// HARDWARE DRIVERS & STATE (L298N DUAL H-BRIDGE)
// ============================================================================
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

static void rover_driving_move(uint16_t throttle_signal, uint16_t steer_signal) {
    MotorOutputs outputs = calculate_motors(current_rover_mode, throttle_signal, steer_signal);
    apply_l298n_left(outputs.left_pwm, outputs.left_dir);
    apply_l298n_right(outputs.right_pwm, outputs.right_dir);
}

static void rover_driving_steer(uint16_t signal) {
    (void)signal; // Direction is applied via differential skid-steering in rover_driving_move
}

static bool run_rover_kinematics_self_test(void) {
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

// ============================================================================
// WEB SERVER & WEBSOCKET INTERFACE (Standard & Reliable for Wokwi Web)
// ============================================================================
static WebServer server(80);
static WebSocketsServer webSocket(81);

static uint16_t channel_values[RC_NUM_CHANNELS] = { RC_CENTER, RC_CENTER, RC_CENTER, RC_CENTER, 1000, RC_CENTER };
static uint16_t rc_filter_buffers[2][RC_FILTER_SAMPLES];
static uint8_t rc_buf_idx = 0;

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="utf-8">
  <title>Mars Rover RC Controller (Skid-Steering)</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <style>
    * { box-sizing: border-box; -webkit-user-select: none; user-select: none; margin: 0; padding: 0; }
    body { background: #11141a; color: #e2e8f0; font-family: -apple-system, sans-serif; overflow: hidden; height: 100vh; display: flex; flex-direction: column; }
    header { background: #1a202c; padding: 10px 16px; display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #334155; }
    .title { font-weight: 800; font-size: 16px; color: #f97316; letter-spacing: 1px; }
    .badge { padding: 4px 10px; border-radius: 12px; font-size: 12px; font-weight: bold; background: #dc2626; }
    .badge.connected { background: #16a34a; }
    .controls { display: flex; gap: 8px; justify-content: center; padding: 10px; background: #1e293b; }
    button { padding: 8px 16px; border-radius: 6px; border: none; font-weight: bold; cursor: pointer; background: #334155; color: white; }
    button.active { background: #f97316; }
    button.stop { background: #dc2626; }
    .telemetry { text-align: center; font-family: monospace; font-size: 13px; padding: 6px; background: #0f172a; color: #38bdf8; }
    .joysticks { flex: 1; display: flex; position: relative; }
    .pad { flex: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; border: 1px dashed #334155; position: relative; touch-action: none; }
    .pad-label { position: absolute; bottom: 20px; font-size: 12px; color: #64748b; font-weight: bold; }
    .base { width: 130px; height: 130px; border-radius: 50%; background: #1e293b; border: 2px solid #475569; position: relative; display: flex; align-items: center; justify-content: center; }
    .stick { width: 60px; height: 60px; border-radius: 50%; background: #38bdf8; border: 2px solid #bae6fd; position: absolute; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.5); pointer-events: none; }
  </style>
</head>
<body>
  <header>
    <div class="title">MARS ROVER RC (L298N)</div>
    <div id="statusBadge" class="badge">DISCONNECTED</div>
  </header>
  <div class="controls">
    <button id="btnNorm" class="active" onclick="setMode(1000)">NORMAL (SKID)</button>
    <button id="btnSpin" onclick="setMode(1500)">SPIN 360</button>
    <button class="stop" onclick="emergencyStop()">STOP</button>
  </div>
  <div class="telemetry">
    MODE: <span id="telMode">NORMAL</span> | MOTOR: <span id="telThrot">1500</span> us | STEER: <span id="telSteer">1500</span> us
  </div>
  <div class="joysticks">
    <div class="pad" id="padL">
      <div class="base"><div class="stick" id="stickL"></div></div>
      <div class="pad-label">STEERING (LEFT / RIGHT)</div>
    </div>
    <div class="pad" id="padR">
      <div class="base"><div class="stick" id="stickR"></div></div>
      <div class="pad-label">THROTTLE (FWD / REV)</div>
    </div>
  </div>

  <script>
    let ws = null;
    let steerVal = 1500, throttleVal = 1500, modeVal = 1000;

    function initWS() {
      const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const wsUrl = `${proto}//${window.location.hostname}:81/`;
      ws = new WebSocket(wsUrl);
      ws.binaryType = 'arraybuffer';
      ws.onopen = () => {
        document.getElementById('statusBadge').innerText = 'CONNECTED';
        document.getElementById('statusBadge').className = 'badge connected';
      };
      ws.onclose = () => {
        document.getElementById('statusBadge').innerText = 'DISCONNECTED';
        document.getElementById('statusBadge').className = 'badge';
        setTimeout(initWS, 1000);
      };
    }
    window.addEventListener('load', initWS);

    function setMode(m) {
      modeVal = m;
      document.getElementById('btnNorm').classList.toggle('active', m === 1000);
      document.getElementById('btnSpin').classList.toggle('active', m === 1500);
      document.getElementById('telMode').innerText = (m === 1500) ? 'SPIN 360' : 'NORMAL';
    }
    function emergencyStop() {
      throttleVal = 1500; steerVal = 1500;
      updateStick('stickL', 0, 0); updateStick('stickR', 0, 0);
      sendPacket();
    }
    function sendPacket() {
      if (ws && ws.readyState === WebSocket.OPEN) {
        const buf = new Uint16Array(6);
        buf[0] = steerVal; buf[1] = throttleVal; buf[2] = 1500; buf[3] = 1500; buf[4] = modeVal; buf[5] = 1500;
        ws.send(buf.buffer);
      }
      document.getElementById('telThrot').innerText = throttleVal;
      document.getElementById('telSteer').innerText = steerVal;
    }
    setInterval(sendPacket, 50);

    function setupStick(padId, stickId, onMove) {
      const pad = document.getElementById(padId);
      let active = false, startY = 0, startX = 0;
      function handleStart(e) { active = true; const t = e.touches ? e.touches[0] : e; startX = t.clientX; startY = t.clientY; }
      function handleMove(e) {
        if (!active) return;
        const t = e.touches ? e.touches[0] : e;
        let dx = t.clientX - startX, dy = t.clientY - startY;
        const dist = Math.hypot(dx, dy);
        if (dist > 50) { dx = (dx / dist) * 50; dy = (dy / dist) * 50; }
        updateStick(stickId, dx, dy);
        onMove(dx / 50, -dy / 50);
      }
      function handleEnd() { if (!active) return; active = false; updateStick(stickId, 0, 0); onMove(0, 0); }
      pad.addEventListener('mousedown', handleStart); window.addEventListener('mousemove', handleMove); window.addEventListener('mouseup', handleEnd);
      pad.addEventListener('touchstart', handleStart); window.addEventListener('touchmove', handleMove); window.addEventListener('touchend', handleEnd);
    }
    function updateStick(id, dx, dy) {
      document.getElementById(id).style.transform = `translate(${dx}px, ${dy}px)`;
    }

    setupStick('padL', 'stickL', (x, y) => {
      steerVal = Math.round(1500 + x * 500);
      if (steerVal >= 1480 && steerVal <= 1520) steerVal = 1500;
    });

    setupStick('padR', 'stickR', (x, y) => {
      throttleVal = Math.round(1500 + y * 500);
      if (throttleVal >= 1480 && throttleVal <= 1520) throttleVal = 1500;
    });
  </script>
</body>
</html>
)rawliteral";

static void handle_websocket_event(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_BIN && length >= 12) {
        const uint16_t* u16_data = reinterpret_cast<const uint16_t*>(payload);
        for (uint8_t i = 0; i < RC_NUM_CHANNELS; i++) {
            channel_values[i] = u16_data[i];
        }
    }
}

// ============================================================================
// SERIAL COMMAND PROCESSOR (Wokwi CLI & Serial Monitor Automation)
// ============================================================================
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

        char c = line.charAt(0);
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

// ============================================================================
// ARDUINO SETUP & MAIN LOOP
// ============================================================================
void setup() {
    Serial.begin(SERIAL_PORT_SPEED);
    delay(500);

    Serial.println();
    Serial.println("=================================");
    Serial.println(" Mars Rover Firmware (Wokwi Sim)");
    Serial.println(" Mode: Standalone RC / WebSockets");
    Serial.println(" Commands: F, B, L, R, S, P");
    Serial.println("=================================");

    // Run built-in kinematics self-test
    run_rover_kinematics_self_test();

    // Initialize L298N Direction Pins
    pinMode(ROVER_L298N_IN1_PIN, OUTPUT);
    pinMode(ROVER_L298N_IN2_PIN, OUTPUT);
    digitalWrite(ROVER_L298N_IN1_PIN, LOW);
    digitalWrite(ROVER_L298N_IN2_PIN, LOW);

    pinMode(ROVER_L298N_IN3_PIN, OUTPUT);
    pinMode(ROVER_L298N_IN4_PIN, OUTPUT);
    digitalWrite(ROVER_L298N_IN3_PIN, LOW);
    digitalWrite(ROVER_L298N_IN4_PIN, LOW);

    // Initialize Hardware PWM (LEDC) for ENA and ENB
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(ROVER_L298N_ENA_PIN, L298N_PWM_FREQ, L298N_PWM_RESOLUTION);
    ledcAttach(ROVER_L298N_ENB_PIN, L298N_PWM_FREQ, L298N_PWM_RESOLUTION);
#else
    ledcSetup(L298N_PWM_CHANNEL_LEFT, L298N_PWM_FREQ, L298N_PWM_RESOLUTION);
    ledcAttachPin(ROVER_L298N_ENA_PIN, L298N_PWM_CHANNEL_LEFT);

    ledcSetup(L298N_PWM_CHANNEL_RIGHT, L298N_PWM_FREQ, L298N_PWM_RESOLUTION);
    ledcAttachPin(ROVER_L298N_ENB_PIN, L298N_PWM_CHANNEL_RIGHT);
#endif

    // Initial state: stop
    rover_driving_move(RC_CENTER, RC_CENTER);

    // WiFi Setup: AP mode + optional station for Wokwi-GUEST
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Rover", NULL);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 5), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.begin("Wokwi-GUEST", "", 6);

    // Setup HTTP web server on port 80
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", INDEX_HTML);
    });
    server.begin();

    // Setup WebSocket server on port 81
    webSocket.begin();
    webSocket.onEvent(handle_websocket_event);

    Serial.printf("Rover Ready (Simplified RC)! Core: %d\n", xPortGetCoreID());
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Wokwi Web IP: http://");
        Serial.println(WiFi.localIP());
    }
}

void loop() {
    // Process Network Clients
    server.handleClient();
    webSocket.loop();

    // Process Serial Monitor Commands
    process_serial_commands();

    uint16_t steer_raw;
    uint16_t motor_raw;
    RoverMode mode;

    if (serial_override) {
        steer_raw = serial_steer;
        motor_raw = serial_throttle;
        mode = serial_mode;
    } else {
        steer_raw = channel_values[RC_STEER_CHANNEL];
        motor_raw = channel_values[RC_MOTOR_CHANNEL];
        uint16_t mode_ch = channel_values[RC_MODE_CHANNEL];
        mode = (mode_ch >= 1400 && mode_ch <= 1600) ? DRIVE_TURN_SPIN : DRIVE_TURN_NORMAL;
    }

    rc_filter_buffers[RC_STEER_CHANNEL][rc_buf_idx] = steer_raw;
    rc_filter_buffers[RC_MOTOR_CHANNEL][rc_buf_idx] = motor_raw;

    if (mode != current_rover_mode) {
        current_rover_mode = mode;
    }

    uint16_t filtered_motor = filter_rc_signal(rc_filter_buffers[RC_MOTOR_CHANNEL], RC_FILTER_SAMPLES);
    uint16_t filtered_steer = filter_rc_signal(rc_filter_buffers[RC_STEER_CHANNEL], RC_FILTER_SAMPLES);

    // Apply differential skid-steering
    rover_driving_move(filtered_motor, filtered_steer);

    // Stream periodic telemetry at 1 Hz
    uint32_t now = millis();
    if (now - last_telemetry_ms >= 1000) {
        last_telemetry_ms = now;
        Serial.printf("[TELEMETRY] Mode: %s | Motor: %u us | Steer: %u us\n",
                      (current_rover_mode == DRIVE_TURN_SPIN ? "SPIN" : "NORMAL"),
                      filtered_motor,
                      filtered_steer);
    }

    rc_buf_idx = (rc_buf_idx + 1) % RC_FILTER_SAMPLES;
    vTaskDelay(pdMS_TO_TICKS(20));
}
