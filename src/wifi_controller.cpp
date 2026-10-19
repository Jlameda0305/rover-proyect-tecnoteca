#include "wifi_controller.h"
#include "arm_config.h"
#include "arm.h"
#include "WiFi.h"
#include "FS.h"
#include "SPIFFS.h"
#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include "esp_log.h"
#include <assert.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include "WiFiUdp.h"

#define MAX_REGISTRATED_CALLBACKS 4
#define UDP_PORT 8080

static const char* TAG = "wifi_controller";
static const char* broadcast_addr = "192.168.4.255";

static void handle_not_found(AsyncWebServerRequest *request);
static void handle_websocket_event(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *payload, size_t length);
static void list_dir(const char* dirname, uint8_t levels);
static void wifiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

static uint16_t channel_values[ARM_NUM_JOINTS] = {
    ARM_PULSE_NEUTRAL, ARM_PULSE_NEUTRAL, ARM_PULSE_NEUTRAL,
    ARM_PULSE_NEUTRAL, ARM_PULSE_NEUTRAL, ARM_PULSE_NEUTRAL
};

static const char* wifi_ssid = ARM_WIFI_DEFAULT_SSID;
static const char* wifi_password = nullptr;

static IPAddress local_ip(ARM_WIFI_DEFAULT_IP[0], ARM_WIFI_DEFAULT_IP[1], ARM_WIFI_DEFAULT_IP[2], ARM_WIFI_DEFAULT_IP[3]);
static IPAddress gateway(ARM_WIFI_DEFAULT_GW[0], ARM_WIFI_DEFAULT_GW[1], ARM_WIFI_DEFAULT_GW[2], ARM_WIFI_DEFAULT_GW[3]);
static IPAddress subnet(ARM_WIFI_DEFAULT_SUBNET[0], ARM_WIFI_DEFAULT_SUBNET[1], ARM_WIFI_DEFAULT_SUBNET[2], ARM_WIFI_DEFAULT_SUBNET[3]);

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static WiFiUDP udp;

static WifiControllerStatusCb* status_callbacks[MAX_REGISTRATED_CALLBACKS];
static uint8_t num_callbacks = 0;
static bool is_initialized = false;
static bool wifi_connected = false;

// JSON parser helpers
static bool json_find_key(const std::string& json, const std::string& key, size_t& val_start) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return false;

    pos += pattern.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        pos++;
    }
    if (pos >= json.length() || json[pos] != ':') return false;
    pos++; // skip ':'
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        pos++;
    }
    if (pos >= json.length()) return false;
    val_start = pos;
    return true;
}

static bool json_get_string(const std::string& json, const std::string& key, std::string& out) {
    size_t pos = 0;
    if (!json_find_key(json, key, pos)) return false;
    if (json[pos] != '"') return false;
    pos++; // skip opening quote
    size_t end = pos;
    while (end < json.length() && json[end] != '"') {
        if (json[end] == '\\' && end + 1 < json.length()) {
            end += 2;
        } else {
            end++;
        }
    }
    if (end >= json.length()) return false;
    out = json.substr(pos, end - pos);
    return true;
}

static bool json_get_int(const std::string& json, const std::string& key, int& out) {
    size_t pos = 0;
    if (!json_find_key(json, key, pos)) return false;
    char* endptr = nullptr;
    long val = std::strtol(json.c_str() + pos, &endptr, 10);
    if (endptr == json.c_str() + pos) return false;
    out = static_cast<int>(val);
    return true;
}

static bool json_get_float(const std::string& json, const std::string& key, float& out) {
    size_t pos = 0;
    if (!json_find_key(json, key, pos)) return false;
    char* endptr = nullptr;
    float val = std::strtof(json.c_str() + pos, &endptr);
    if (endptr == json.c_str() + pos) return false;
    out = val;
    return true;
}

static void reset_ch_values() {
    for (uint8_t i = 0; i < ARM_NUM_JOINTS; i++) {
        channel_values[i] = ARM_PULSE_NEUTRAL;
    }
}

static std::string build_arm_state_json() {
    char buf[512];
    uint16_t us[ARM_NUM_JOINTS];
    float deg[ARM_NUM_JOINTS];
    bool moving[ARM_NUM_JOINTS];
    bool any_moving = false;

    for (uint8_t i = 0; i < ARM_NUM_JOINTS; i++) {
        ArmJointId j = static_cast<ArmJointId>(i);
        us[i] = arm_get_joint_us(j);
        deg[i] = arm_get_joint_deg(j);
        moving[i] = arm_servo_is_moving(i);
        if (moving[i]) {
            any_moving = true;
        }
    }

    int len = snprintf(buf, sizeof(buf),
        "{\"type\":\"arm_state\","
        "\"moving\":%s,"
        "\"us\":[%u,%u,%u,%u,%u,%u],"
        "\"deg\":[%.1f,%.1f,%.1f,%.1f,%.1f,%.1f],"
        "\"joints\":["
        "{\"id\":0,\"name\":\"%s\",\"us\":%u,\"deg\":%.1f,\"moving\":%s},"
        "{\"id\":1,\"name\":\"%s\",\"us\":%u,\"deg\":%.1f,\"moving\":%s},"
        "{\"id\":2,\"name\":\"%s\",\"us\":%u,\"deg\":%.1f,\"moving\":%s},"
        "{\"id\":3,\"name\":\"%s\",\"us\":%u,\"deg\":%.1f,\"moving\":%s},"
        "{\"id\":4,\"name\":\"%s\",\"us\":%u,\"deg\":%.1f,\"moving\":%s},"
        "{\"id\":5,\"name\":\"%s\",\"us\":%u,\"deg\":%.1f,\"moving\":%s}"
        "]}",
        any_moving ? "true" : "false",
        us[0], us[1], us[2], us[3], us[4], us[5],
        deg[0], deg[1], deg[2], deg[3], deg[4], deg[5],
        get_joint_name(ARM_JOINT_BASE), us[0], deg[0], moving[0] ? "true" : "false",
        get_joint_name(ARM_JOINT_SHOULDER), us[1], deg[1], moving[1] ? "true" : "false",
        get_joint_name(ARM_JOINT_ELBOW), us[2], deg[2], moving[2] ? "true" : "false",
        get_joint_name(ARM_JOINT_WRIST_PITCH), us[3], deg[3], moving[3] ? "true" : "false",
        get_joint_name(ARM_JOINT_WRIST_ROLL), us[4], deg[4], moving[4] ? "true" : "false",
        get_joint_name(ARM_JOINT_GRIPPER), us[5], deg[5], moving[5] ? "true" : "false"
    );

    return std::string(buf, len > 0 ? len : 0);
}

void wifi_controller_broadcast_arm_state(void) {
    if (!is_initialized || ws.count() == 0) {
        return;
    }
    std::string json = build_arm_state_json();
    ws.textAll(json.c_str(), json.length());
}

void wifi_controller_init(const char* ssid, const char* password, WifiControllerMode mode)
{
  reset_ch_values();
  num_callbacks = 0;

  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  list_dir("/", 0);
  wifi_ssid = (ssid != nullptr && strlen(ssid) > 0) ? ssid : ARM_WIFI_DEFAULT_SSID;
  wifi_password = password;

  switch (mode)
  {
  case WIFI_CONTROLLER_STATION:
    WiFi.begin(wifi_ssid, wifi_password);
    WiFi.config(local_ip, gateway, subnet);
    WiFi.setAutoReconnect(true);
    WiFi.onEvent(wifiEvent);
    break;
  case WIFI_CONTROLLER_AP:
    WiFi.softAP(wifi_ssid, wifi_password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    break;
  default:
    assert(false);
    break;
  }

  ws.onEvent(handle_websocket_event);
  server.addHandler(&ws);

  server.serveStatic("/", SPIFFS, "/index.html", "");
  server.serveStatic("/virtualjoystick.js", SPIFFS, "/virtualjoystick.js", "");
  server.onNotFound(handle_not_found);
  server.begin();

  udp.begin(UDP_PORT);

  is_initialized = true;
}

void wifi_controller_register_connection_callback(WifiControllerStatusCb* cb) {
  assert(num_callbacks < MAX_REGISTRATED_CALLBACKS);
  assert(is_initialized);
  status_callbacks[num_callbacks] = cb;
  num_callbacks++;
}

void wifi_controller_ws_send_bin(uint8_t* data, uint32_t length)
{
  ws.binaryAll(data, length);
}

void wifi_controller_udp_send_bin(uint8_t* data, uint32_t length)
{
  if (wifi_connected) {
    uint16_t udp_buf_size = length + 2;
    uint8_t udp_buf[udp_buf_size];

    // Encapsulate data in start and end tag to validate correct-ish packet on other side.
    udp_buf[0] = '[';
    memcpy(&udp_buf[1], data, length);
    udp_buf[udp_buf_size - 1] = ']';
    udp.beginPacket(broadcast_addr, UDP_PORT);
    udp.write((uint8_t*)udp_buf, udp_buf_size);
    udp.endPacket();
  }
}

uint16_t wifi_controller_get_val(uint8_t channel)
{
  assert(channel < ARM_NUM_JOINTS);
  return channel_values[channel];
}

static void handle_not_found(AsyncWebServerRequest *request)
{
  request->send(SPIFFS, "/index.html");
}

static void wifiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  if (event == SYSTEM_EVENT_STA_DISCONNECTED) {
    wifi_connected = false;
    if (info.disconnected.reason == 6) {
      Serial.println("NOT_AUTHED reconnect");
      WiFi.reconnect();
    }
  } else if (event == SYSTEM_EVENT_STA_GOT_IP) {
    wifi_connected = true;
  }
}

static void handle_websocket_event(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *payload, size_t length)
{
  (void)server;
  switch (type)
  {
    case WS_EVT_DISCONNECT:
      ESP_LOGI(TAG, "Client disconnected");
      reset_ch_values();
      for (uint8_t i = 0; i < num_callbacks; i++) {
        status_callbacks[i](WIFI_CONTROLLER_DISCONNECTED);
      }
      break;
    case WS_EVT_CONNECT:
      ESP_LOGI(TAG, "Client connected");
      reset_ch_values();
      for (uint8_t i = 0; i < num_callbacks; i++) {
        status_callbacks[i](WIFI_CONTROLLER_CONNECTED);
      }
      if (client != nullptr) {
        std::string json = build_arm_state_json();
        client->text(json.c_str(), json.length());
      }
      break;
    case WS_EVT_DATA:
    {
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if (!(info->final && info->index == 0 && info->len == length)) {
        break;
      }
      if (info->opcode == WS_BINARY) {
        if (length >= ARM_NUM_JOINTS * sizeof(uint16_t)) {
          uint16_t* values = (uint16_t*)payload;
          for (uint8_t i = 0; i < ARM_NUM_JOINTS; i++) {
            if (values[i] >= DEFAULT_JOINT_LIMITS[i].min_us && values[i] <= DEFAULT_JOINT_LIMITS[i].max_us) {
              channel_values[i] = values[i];
            } else {
              ESP_LOGE(TAG, "Expected channel values to be in range 1000 - 2000 but was: %d\n", values[i]);
              channel_values[i] = ARM_PULSE_NEUTRAL;
            }
          }
        } else {
          ESP_LOGI(TAG, "Invalid binary length");
          reset_ch_values();
        }
      } else {
        // Text / JSON frame
        std::string json_str(reinterpret_cast<const char*>(payload), length);
        std::string cmd;
        if (json_get_string(json_str, "cmd", cmd)) {
          if (cmd == "home") {
            int speed = ARM_SPEED_DEFAULT;
            json_get_int(json_str, "speed", speed);
            if (speed < ARM_SPEED_MIN) speed = ARM_SPEED_MIN;
            if (speed > ARM_SPEED_MAX) speed = ARM_SPEED_MAX;
            arm_home(static_cast<uint8_t>(speed));
            wifi_controller_broadcast_arm_state();
          } else if (cmd == "rest") {
            int speed = ARM_SPEED_DEFAULT;
            json_get_int(json_str, "speed", speed);
            if (speed < ARM_SPEED_MIN) speed = ARM_SPEED_MIN;
            if (speed > ARM_SPEED_MAX) speed = ARM_SPEED_MAX;
            arm_rest(static_cast<uint8_t>(speed));
            wifi_controller_broadcast_arm_state();
          } else if (cmd == "stop") {
            arm_emergency_stop();
            wifi_controller_broadcast_arm_state();
          } else if (cmd == "jog") {
            int joint = -1;
            int dir = 0;
            int speed = ARM_SPEED_DEFAULT;
            if (json_get_int(json_str, "joint", joint) && joint >= 0 && joint < ARM_NUM_JOINTS) {
              json_get_int(json_str, "dir", dir);
              json_get_int(json_str, "speed", speed);
              if (speed < ARM_SPEED_MIN) speed = ARM_SPEED_MIN;
              if (speed > ARM_SPEED_MAX) speed = ARM_SPEED_MAX;
              int8_t dir_val = (dir > 0) ? 1 : ((dir < 0) ? -1 : 0);
              arm_jog(static_cast<ArmJointId>(joint), dir_val, static_cast<uint8_t>(speed));
            }
          } else if (cmd == "set_joint") {
            int joint = -1;
            int pos = ARM_PULSE_NEUTRAL;
            int speed = ARM_SPEED_DEFAULT;
            if (json_get_int(json_str, "joint", joint) && joint >= 0 && joint < ARM_NUM_JOINTS) {
              if (json_get_int(json_str, "pos", pos)) {
                json_get_int(json_str, "speed", speed);
                if (speed < ARM_SPEED_MIN) speed = ARM_SPEED_MIN;
                if (speed > ARM_SPEED_MAX) speed = ARM_SPEED_MAX;
                arm_set_joint_us(static_cast<ArmJointId>(joint), static_cast<uint16_t>(pos), static_cast<uint8_t>(speed));
                wifi_controller_broadcast_arm_state();
              }
            }
          } else if (cmd == "set_deg") {
            int joint = -1;
            float deg = 0.0f;
            int speed = ARM_SPEED_DEFAULT;
            if (json_get_int(json_str, "joint", joint) && joint >= 0 && joint < ARM_NUM_JOINTS) {
              if (json_get_float(json_str, "deg", deg)) {
                json_get_int(json_str, "speed", speed);
                if (speed < ARM_SPEED_MIN) speed = ARM_SPEED_MIN;
                if (speed > ARM_SPEED_MAX) speed = ARM_SPEED_MAX;
                arm_set_joint_deg(static_cast<ArmJointId>(joint), deg, static_cast<uint8_t>(speed));
                wifi_controller_broadcast_arm_state();
              }
            }
          } else if (cmd == "get_state") {
            std::string state_json = build_arm_state_json();
            if (client != nullptr) {
              client->text(state_json.c_str(), state_json.length());
            } else {
              ws.textAll(state_json.c_str(), state_json.length());
            }
          } else {
            ESP_LOGW(TAG, "Unknown JSON cmd: %s", cmd.c_str());
          }
        }
      }
      break;
    }
    default:
      break;
  }
}

static void list_dir(const char* dirname, uint8_t levels) {
    File root = SPIFFS.open(dirname);
    if (!root){
      Serial.println("- failed to open directory");
      return;
    }
    if (!root.isDirectory()) {
      Serial.println(" - not a directory");
      return;
    }

    File file = root.openNextFile();
    while (file) {
      if (file.isDirectory()) {
        Serial.print("  DIR : ");
        Serial.println(file.name());
        if(levels){
          list_dir(file.name(), levels -1);
        }
      } else {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("\tSIZE: ");
        Serial.println(file.size());
      }
      file = root.openNextFile();
    }
}