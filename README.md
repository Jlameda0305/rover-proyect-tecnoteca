# Mars Rover — Firmware de Referencia Original (JakKra)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg?logo=platformio)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20ESP32-blue.svg?logo=espressif)](https://github.com/espressif/arduino-esp32)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Source: jakkra/Mars-Rover](https://img.shields.io/badge/Source-jakkra%2FMars--Rover%20(85823c3)-informational.svg?logo=github)](https://github.com/jakkra/Mars-Rover)

---

## 📌 Descripción

Esta rama (`reference`) preserva una copia limpia, fidedigna y sin modificaciones del firmware original del repositorio oficial [jakkra/Mars-Rover](https://github.com/jakkra/Mars-Rover) (commit [`85823c3`](https://github.com/jakkra/Mars-Rover/commit/85823c30ba935148a07f5b883f9e1ce0de1699e5)), desarrollado por **Jakob Krantz** y publicado bajo **Licencia MIT**.

---

## 🎯 Propósito de esta Rama (*Vendor Branch*)

El objetivo de mantener esta rama de referencia dentro del proyecto es disponer de una fuente canónica e inalterada de los algoritmos y controladores originales, permitiendo:

1. **Consulta directa de algoritmos avanzados:**
   - **Cinemática 4WS (4-Wheel Steering):** Modos de dirección normal (contragiro delantero/trasero) y modo *Spin* (giro en el lugar / pivot en 360° mediante posicionamiento angular tangencial de las ruedas).
   - **Mástil Pan/Tilt de cámara:** Control angular en acimut (*yaw*) y elevación (*pitch*) del cabezal de exploración.
   - **Brazo Robótico Articulado (6-DOF):** Interpolación de trayectorias con rampas suaves en los 6 ejes y control de gripper.
   - **Comunicación por radio LoRa:** Protocolo bidireccional sobre SPI con chips Semtech SX127x (433/868/915 MHz).
   - **Decodificación RMT de receptor RC:** Captura de pulsos PWM multicanal de alta precisión utilizando el periférico de hardware RMT (*Remote Control*) del ESP32 sin jitter ni sobrecarga de CPU.
   - **Gestión de modos y conmutación:** Lógica de conmutador de arranque (AP, Station+LoRa, LoRa Only) y máquina de estados de modos de operación (conducción vs. brazo).
2. **Repositorio ultra liviano:**
   - Evita arrastrar los **más de 500 MB** del historial git acumulado y los voluminosos archivos CAD de Autodesk Fusion 360 (`.f3d`, `.step`, `.stl`) del repositorio original.
   - Mantiene el código fuente de referencia accesible en cualquier momento mediante un simple cambio de rama: `git checkout reference`.

---

## 📂 Índice de Módulos Originales en `src/` (22 Archivos C++)

El firmware se compone de **22 archivos fuente C++** (`.h` y `.cpp`) organizados modularmente por subsistema:

### 1. Chasis, Dirección y Tracción (4WS)
* **`rover_driving.h` / `rover_driving.cpp`**:  
  Implementa la cinemática de dirección en las 4 ruedas esquineras (4-Wheel Steering) y la tracción diferencial:
  - `steer_normal()`: Giro coordinado donde los servos delanteros y traseros giran en sentidos opuestos para minimizar el radio de giro y reducir el arrastre lateral de los neumáticos.
  - `steer_spin()`: Orientación de las 4 ruedas a ángulos de 45° tangenciales a la circunferencia del chasis, permitiendo girar sobre su propio eje central (*Zero-Radius Turn / Spin Mode*).
  - Control de los variadores electrónicos de velocidad (ESC) para motores izquierdos y derechos con manejo de marcha atrás e inversión controlada.
* **`rover_servo.h` / `rover_servo.cpp`**:  
  Capa de abstracción para el control de servomotores a través del driver I2C PCA9685 (dirección `0x40`). Soporta interpolación temporal mediante timers de FreeRTOS (`esp_timer`) y la librería `Ramp`, permitiendo movimientos progresivos con velocidad programable (`ARM_MIN_SPEED` a `ARM_MAX_SPEED`) y calibración por anchos de pulso en microsegundos.
* **`rover_config.h`**:  
  Definición central de constantes de hardware: mapa de pines para canales RC (GPIO 34, 35, 4, 39, 25, 36), velocidad serial (115200 baudios), umbrales PWM (1000 µs, 1500 µs, 2000 µs), frecuencias y períodos de muestreo.

### 2. Brazo Robótico Articulado (6-DOF)
* **`arm.h` / `arm.cpp`**:  
  Controlador del manipulador robótico de 6 grados de libertad (`ARM_AXIS_1` a `ARM_AXIS_6`):
  - `arm_init()`: Posicionamiento inicial en postura de reposo/estibado.
  - `arm_move_axis_us()`: Movimiento angular de un eje específico con velocidad controlada.
  - `arm_pause()`: Detención inmediata de un eje ante condiciones de límite o pausa.

### 3. Cabeza y Mástil de Cámara (Pan / Tilt)
* **`rover_head.h` / `rover_head.cpp`**:  
  Control de posicionamiento del mástil de observación:
  - `rover_head_yaw()`: Control del servomotor de acimut / paneo horizontal.
  - `rover_head_pitch()`: Control del servomotor de elevación / inclinación vertical.

### 4. Sensores y Telemetría
* **`gyro_accel_sensor.h` / `gyro_accel_sensor.cpp`**:  
  Controlador del sensor inercial IMU MPU-6050 a través de I2C:
  - Lectura de aceleraciones en 3 ejes ($A_x, A_y, A_z$), giroscopio ($G_x, G_y, G_z$) y temperatura interna.
  - Cálculo de ángulos de inclinación y orientación (*roll*, *pitch*, *yaw*).
  - Sincronización multi-hilo mediante semáforo FreeRTOS (`xSemaphoreHandle`) para evitar colisiones en el bus I2C compartido con el PCA9685.

### 5. Conectividad, Radio y Receptores RC
* **`rc_receiver_rmt.h` / `rc_receiver_rmt.cpp`**:  
  Decodificador de receptor RC multicanal (6 canales) utilizando el periférico hardware **RMT** (*Remote Control*) de Espressif ESP32. Captura duraciones de pulsos con precisión de microsegundos mediante interrupciones de hardware e introduce filtrado de promedios móviles para descartar ruido.
* **`lora_controller.h` / `lora_controller.cpp`**:  
  Transmisión y recepción de paquetes de control y telemetría mediante radio LoRa (driver `arduino-LoRa` sobre SPI). Incluye detección de enlace caído (*failsafe*), envío de comandos binarios y retransmisión de estados.
* **`wifi_controller.h` / `wifi_controller.cpp`**:  
  Módulo de red inalámbrica con soporte dual (Modo Access Point y Modo Station). Levanta un servidor web asíncrono (`ESPAsyncWebServer`), un socket WebSocket para telemetría bidireccional de baja latencia y un socket UDP para recepción continua de paquetes de control.

### 6. Control de Modos y Orquestación Principal
* **`rover_settings_switch.h` / `rover_settings_switch.cpp`**:  
  Lectura de conmutadores físicos de selección de conectividad al arranque:
  - `ROVER_SWITCH_STATE_STATION_LORA`
  - `ROVER_SWITCH_STATE_LORA_ONLY`
  - `ROVER_SWITCH_STATE_AP`
* **`switch_checker.h` / `switch_checker.cpp`**:  
  Máquina de estados para conmutación dinámica en caliente de los modos de operación a través de los canales auxiliares (CH5 / CH6) del radiocontrol o comandos web:
  - `DRIVE_TURN_NORMAL`: Conducción 4WS estándar.
  - `DRIVE_TURN_SPIN`: Giro sobre el eje central.
  - `ROBOT_ARM`: Reasignación de joysticks para operar las articulaciones del brazo robótico.
* **`main.cpp`**:  
  Punto de entrada (`setup()` / `loop()`), despacho de tareas concurrentes de FreeRTOS, sincronización de periféricos, control de *failsafe* por desconexión de señal y distribución de comandos a los subsistemas correspondientes.

---

## 🌐 Recursos Web en `data/` (SPIFFS)

* **`data/index.html`**:  
  Interfaz de usuario web responsive construida con HTML5, Canvas y CSS, con joysticks virtuales táctiles para la conducción y el accionamiento del brazo desde dispositivos móviles o PC.
* **`data/virtualjoystick.js`**:  
  Librería JavaScript para la captura y normalización de eventos táctiles y de puntero para los controles en pantalla.

---

## ⚙️ Configuración de PlatformIO (`platformio.ini`)

El archivo `platformio.ini` original define la placa de desarrollo objetivo y las librerías requeridas:

```ini
[env:esp-wrover-kit]
platform = espressif32
framework = arduino
board = ttgo-lora32-v1

upload_port = COM8
monitor_port = COM8
upload_speed = 921600
monitor_speed = 115200

build_flags = -DCORE_DEBUG_LEVEL=5

lib_deps =
    ESP32Servo,
    Ramp,
    MPU6050_tockn,
    Adafruit PWM Servo Driver Library,
    AsyncTCP,
    ESP Async WebServer,
    https://github.com/sandeepmistry/arduino-LoRa
```

---

## 📄 Licencia y Atribución

Este software es obra original de **Jakob Krantz (2019)** y se distribuye bajo los términos de la **Licencia MIT**. Consulte el archivo [LICENSE](LICENSE) para ver el texto completo.
