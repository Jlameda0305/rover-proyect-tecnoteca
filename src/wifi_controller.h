#pragma once

#include <Arduino.h>
#include "arm_config.h"

typedef enum WifiControllerStatus
{
  WIFI_CONTROLLER_CONNECTED,
  WIFI_CONTROLLER_DISCONNECTED,
} WifiControllerStatus;

typedef enum WifiControllerMode
{
  WIFI_CONTROLLER_STATION,
  WIFI_CONTROLLER_AP,
} WifiControllerMode;

typedef void(WifiControllerStatusCb(WifiControllerStatus status));

void wifi_controller_init(const char* ssid = ARM_WIFI_DEFAULT_SSID, const char* password = nullptr, WifiControllerMode mode = WIFI_CONTROLLER_AP);
void wifi_controller_register_connection_callback(WifiControllerStatusCb* cb);
void wifi_controller_ws_send_bin(uint8_t* data, uint32_t length);
void wifi_controller_udp_send_bin(uint8_t* data, uint32_t length);
void wifi_controller_deinit(void);

uint16_t wifi_controller_get_val(uint8_t channel);
inline uint16_t wifi_controller_get_ch_val(uint8_t channel) {
    return wifi_controller_get_val(channel);
}

void wifi_controller_broadcast_arm_state(void);