# Controlador Autónomo para Brazo Robótico 6-DOF (Mars Rover)

[![ESP32](https://img.shields.io/badge/SoC-ESP32-red.svg?logo=espressif)](https://www.espressif.com/)
[![PCA9685](https://img.shields.io/badge/PWM%20Driver-PCA9685%20(I2C)-orange.svg)](https://www.nxp.com/)
[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg?logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![Unit Tests](https://img.shields.io/badge/Unit%20Tests-891%2F891%20Passed%20(100%25)-brightgreen.svg)](test/)
[![WebSockets](https://img.shields.io/badge/Communications-WebSockets%20Async-purple.svg)](https://github.com/ESP32Async/ESPAsyncWebServer)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Core%206.0%2B-orange.svg?logo=platformio)](https://platformio.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Firmware de alta precisión y arquitectura en tiempo real para el control de un **Brazo Robótico articulado de 6 Grados de Libertad (6-DOF)** montado en el vehículo de exploración terrestre Mars Rover.

El sistema ejecuta sobre un microcontrolador **ESP32 SoC** desacoplando la cinemática articular, la interpolación continua de trayectorias en **FreeRTOS** y la comunicación telemática asíncrona mediante un **Web Dashboard Dark Sci-Fi** con protocolo **WebSockets**.

---

## 📋 Tabla de Contenidos

- [Características Principales](#-características-principales)
- [Especificaciones de Hardware](#-especificaciones-de-hardware)
  - [Mapeo de Articulaciones y Canales](#mapeo-de-articulaciones-y-canales)
  - [Conexión I2C y Topología Eléctrica](#conexión-i2c-y-topología-eléctrica)
- [Arquitectura del Firmware](#-arquitectura-del-firmware)
  - [Capa Cinemática Desacoplada (`arm_kinematics`)](#capa-cinemática-desacoplada-arm_kinematics)
  - [Generador de Trayectorias Suaves (`calculate_step`)](#generador-de-trayectorias-suaves-calculate_step)
  - [Motor FreeRTOS a 10ms (`ArmServoUpdater`) y Concurrencia Thread-Safe](#motor-freertos-a-10ms-armservoupdater-y-concurrencia-thread-safe)
- [Web Dashboard Dark Sci-Fi](#-web-dashboard-dark-sci-fi)
  - [Protocolo de Comunicación WebSockets](#protocolo-de-comunicación-websockets)
  - [Interfaz Táctil y Presets Rápidos](#interfaz-táctil-y-presets-rápidos)
  - [Parada de Emergencia (E-Stop)](#parada-de-emergencia-e-stop)
- [Batería de Pruebas Unitarias](#-batería-de-pruebas-unitarias)
- [Compilación, Flasheo y Ejecución](#-compilación-flasheo-y-ejecución)
- [Estructura del Proyecto](#-estructura-del-proyecto)
- [Atribuciones y Licencia](#-atribuciones-y-licencia)

---

## 🚀 Características Principales

- **Control Articular 6-DOF Completo:** Control independiente y coordinado de Base (Yaw), Hombro (Pitch), Codo (Pitch), Muñeca Pitch, Muñeca Roll y Pinza/Gripper.
- **Interpolación Suave de Movimiento:** Algoritmo cinemático adaptativo con escalado de velocidad configurable (1..10 niveles) que elimina aceleraciones bruscas y preserva la integridad mecánica de los engranajes.
- **Planificación Asíncrona en Tiempo Real:** Tarea de FreeRTOS dedicada (`ArmServoUpdater`) ejecutada con cadencia fija de 10 ms (`100 Hz`), sincronizada mediante mutex con semáforo I2C.
- **Dashboard Web Autónomo (Dark Sci-Fi):** Servidor HTTP y WebSocket asíncrono embebido (`ESPAsyncWebServer`), servido directamente desde la memoria flash del ESP32 en modo Punto de Acceso Wi-Fi (`Robotic-Arm`).
- **Controles Táctiles Hold-to-Jog y Sliders:** Modos duales de manipulación: control continuo al mantener presionado (*jogging*) o fijación de ángulo/pulso exacto mediante sliders.
- **Presets Cinemáticos Precalibrados:** Poses de despliegue rápido preconfiguradas: `HOME`, `REST` (estacionamiento seguro), y apertura/cierre rápido de pinza.
- **Failsafe y Parada de Emergencia Activa:** Detención instantánea en caso de desconexión del cliente WebSocket o pulsación del botón `E-STOP`.
- **100% Testeado con C++17:** Suite completa de 891 pruebas unitarias independientes ejecutables en host (Linux/macOS/Windows) sin dependencias de hardware.

---

## 🔌 Especificaciones de Hardware

El subsistema del brazo se compone del microcontrolador principal ESP32 comunicado vía bus I2C con un controlador dedicado de modulación por ancho de pulsos PCA9685 de 16 canales a 12 bits.

```
                    +--------------------------------+
                    |           ESP32 SoC            |
                    |     (Controlador Principal)    |
                    +---------------+----------------+
                                    | I2C (SDA=21, SCL=22)
                                    | 400 kHz Fast-Mode
                                    v
                    +--------------------------------+
                    |     Driver PWM PCA9685         |
                    |   Dirección I2C: 0x40 (60Hz)   |
                    +---------------+----------------+
                                    |
          +---------+---------+-----+---+---------+---------+
          | Ch 0    | Ch 1    | Ch 2    | Ch 3    | Ch 4    | Ch 5
          v         v         v         v         v         v
       [Base]   [Hombro]   [Codo]   [Muñeca   [Muñeca   [Pinza /
        Yaw      Pitch     Pitch     Pitch]    Roll]    Gripper]
```

### Mapeo de Articulaciones y Canales

| Canal PCA9685 | Articulación | Rango Grados | Rango Pulsos ($\mu s$) | Preset HOME ($\mu s$) | Preset REST ($\mu s$) |
|:---:|:---|:---:|:---:|:---:|:---:|
| **0** | **Base** (Yaw) | 0° a 180° | 1000 - 2000 | 1900 | 1500 |
| **1** | **Hombro** (Pitch) | 0° a 180° | 1000 - 2000 | 1600 | 1000 |
| **2** | **Codo** (Pitch) | 0° a 180° | 1000 - 2000 | 1200 | 1000 |
| **3** | **Muñeca Pitch** | 0° a 180° | 1000 - 2000 | 1200 | 1000 |
| **4** | **Muñeca Roll** | 0° a 180° | 1000 - 2000 | 1500 | 1500 |
| **5** | **Pinza (Gripper)** | 0° a 180° | 1000 - 2000 | 1500 | 1500 |

### Conexión I2C y Topología Eléctrica

- **ESP32 Pin GPIO 21:** SDA (Bus de datos I2C).
- **ESP32 Pin GPIO 22:** SCL (Bus de reloj I2C).
- **Alimentación Lógica:** 3.3V / 5V común para ESP32 y VCC del PCA9685.
- **Alimentación de Servos:** Riel de potencia dedicado **V+** de 5V a 6V (mínimo 5A continuo / 10A pico) con línea de masa (*GND*) unificada en estrella con el microcontrolador.
- **Frecuencia PWM:** 60 Hz (período de 16.66 ms, resolución de 4096 pasos por ciclo = ~4.07 $\mu s$/paso).

---

## 🏗 Arquitectura del Firmware

El diseño de software prioriza el desacoplamiento modular, la robustez ante fallos y la ausencia de bloqueos en el bucle principal.

```
+-----------------------------------------------------------------------+
|                              Navegador Web                            |
|             (Dashboard Dark Sci-Fi en HTML5 / CSS3 / ES6)             |
+-----------------------------------+-----------------------------------+
                                    | JSON / Binary Packets (WebSockets)
                                    v
+-----------------------------------------------------------------------+
|                           ESPAsyncWebServer                           |
|      - Servidor de Assets Estáticos (index.html, virtualjoystick.js)  |
|      - Gestor Asíncrono de Eventos WebSocket                          |
+-----------------------------------+-----------------------------------+
                                    | Mensajes de Posición / Jogging
                                    v
+-----------------------------------------------------------------------+
|                      Capa de Aplicación (arm.cpp)                     |
|           - Despacho de Comandos y Coordinación Cinemática            |
+-----------------------------------+-----------------------------------+
                                    | arm_servo_move_to_us()
                                    v
+-----------------------------------------------------------------------+
|                   Capa de Servos & FreeRTOS (arm_servo)               |
|   - Tarea `ArmServoUpdater` (Tick cada 10ms)                          |
|   - Control de Mutex / Semáforo I2C Thread-Safe                       |
+-----------------------------------+-----------------------------------+
                                    | calculate_step()
                                    v
+-----------------------------------------------------------------------+
|                 Capa Cinemática Pura (arm_kinematics)                 |
|   - Conversión Grados <-> Microsegundos <-> Ticks PCA9685 (12 bits)   |
|   - Suavizado S-Curve / Rampa Trapezoidal de Trayectoria              |
|   - Clamping Estricto dentro de Límites Seguros                       |
+-----------------------------------------------------------------------+
                                    | setPWM(ch, 0, count)
                                    v
                        [ Hardware Driver PCA9685 ]
```

### Capa Cinemática Desacoplada (`arm_kinematics`)

Contiene la matemática pura sin dependencia del SDK de ESP32 ni de Arduino:
- **Conversión de Escala:** `angle_to_us()`, `us_to_angle()`, `us_to_pca9685_count()`.
- **Protección de Rango:** `clamp_joint_pulse()`, `clamp_joint_angle()`.
- **Fórmula de Conversión PCA9685:**
  $$\text{Count}_{12\text{-bit}} = \left\lfloor \frac{\text{Pulso}(\mu s) \times 4096 \times 60\,\text{Hz}}{1\,000\,000\,\mu s} + 0.5 \right\rfloor$$

### Generador de Trayectorias Suaves (`calculate_step`)

La función `calculate_step(current_us, target_us, speed, delta_time_ms)` calcula el incremento angular proporcional al tiempo transcurrido ($\Delta t$):
- **Velocidad Base:** $0.2\,\mu s/\text{ms}$ por nivel de velocidad.
- A velocidad estándar ($5$), la tasa de avance es de $1.0\,\mu s/\text{ms} = 1000\,\mu s/\text{s}$.
- Garantiza convergencia exacta al objetivo sin oscilaciones residuales ni sobrepasos (*overshoot*).

### Motor FreeRTOS a 10ms (`ArmServoUpdater`) y Concurrencia Thread-Safe

La tarea en segundo plano `ArmServoUpdater` se crea durante la inicialización con prioridad `5` y pila de `2048 bytes`:
- Utiliza `vTaskDelayUntil()` para mantener una cadencia exacta de 10 ms (`100 Hz`).
- Todos los accesos al bus I2C y a las estructuras de estado de los canales están protegidos por el mutex `s_arm_mutex`.
- Cuando un canal alcanza su objetivo, el flag `is_moving` se desactiva de forma atómica para ahorrar ciclos de CPU y tráfico I2C.

---

## 💻 Web Dashboard Dark Sci-Fi

El firmware hospeda una interfaz web avanzada diseñada con estética espacial táctil optimizada para teléfonos móviles, tablets y ordenadores.

### Protocolo de Comunicación WebSockets

El servidor WebSocket asíncrono opera en el puerto 80 del ESP32 (`/ws`):

1. **Comandos JSON de Articulación Individual:**
   ```json
   { "action": "set_joint", "joint": 0, "us": 1650, "speed": 5 }
   ```
2. **Comandos de Presets Globales:**
   ```json
   { "action": "preset", "name": "home" }
   { "action": "preset", "name": "rest" }
   { "action": "gripper", "state": "open" }
   { "action": "gripper", "state": "closed" }
   ```
3. **Comando de Parada de Emergencia:**
   ```json
   { "action": "estop" }
   ```
4. **Telemetría de Estado (ESP32 $\rightarrow$ Cliente):**
   ```json
   {
     "status": "ok",
     "joints": [1900, 1600, 1200, 1200, 1500, 1500],
     "moving": [false, false, false, false, false, false]
   }
   ```

### Interfaz Táctil y Presets Rápidos

- **Hold-to-Jog:** Botones táctiles (`+` y `-`) que incrementan o decrementan suavemente la posición del servo mientras se mantienen presionados, deteniéndose al soltar (`pointerup`/`pointercancel`).
- **Sliders Analógicos:** Entrada directa con deslizadores de alta resolución y visualización en tiempo real de microsegundos y grados calculados.
- **Botones de Macro:** Acceso con un solo clic a las posturas operacionales:
  - `HOME`: Postura erguida de trabajo y exploración.
  - `REST`: Postura replegada de transporte compacto.
  - `PINZA OPEN` / `PINZA CLOSED`: Accionamiento rápido del efector final.

### Parada de Emergencia (E-Stop)

El panel incorpora un botón prominente `EMERGENCY STOP` que congela inmediatamente el movimiento de los 6 servomotores en sus posiciones instantáneas y cancela las trayectorias en curso.

---

## 🧪 Batería de Pruebas Unitarias

El firmware cuenta con un marco de testing nativo en C++17 (`test/test_arm_kinematics.cpp`) que evalúa la corrección funcional y numérica de todo el subsistema cinemático sin necesidad de compilar para microcontrolador:

1. **`test_joint_naming_and_config`**: Valida los identificadores, conteo de articulaciones y nombres textuales.
2. **`test_joint_pulse_clamping`**: Verifica el confinamiento estricto a los límites de microsegundos configurados.
3. **`test_joint_angle_clamping`**: Comprueba el filtrado y saturación en rangos angulares de 0° a 180°.
4. **`test_angle_pulse_roundtrip`**: Evalúa la reversibilidad biyectiva de las transformaciones ángulo $\leftrightarrow$ pulso.
5. **`test_pca9685_pwm_count_conversion`**: Comprueba la precisión matemática del cómputo de ticks a 12 bits para el PCA9685 a 60 Hz.
6. **`test_smooth_trajectory_step`**: Valida la interpolación por rampa, escalado temporal y convergencia monótona.
7. **`test_home_and_rest_preset_validity`**: Asegura que las coordenadas predefinidas (`HOME` y `REST`) sean dimensionalmente válidas.
8. **`test_mock_pwm_driver`**: Simulación de hardware mediante mock en memoria de registros del driver I2C.

### Ejecución de Pruebas

```bash
bash test/run_tests.sh
```

**Resultado de la verificación:**
```text
========================================================
   Mars Rover - Robotic Arm Kinematics & Driver Tests   
========================================================
[RUN       ] test_joint_naming_and_config
[      PASS] test_joint_naming_and_config
[RUN       ] test_joint_pulse_clamping
[      PASS] test_joint_pulse_clamping
[RUN       ] test_joint_angle_clamping
[      PASS] test_joint_angle_clamping
[RUN       ] test_angle_pulse_roundtrip
[      PASS] test_angle_pulse_roundtrip
[RUN       ] test_pca9685_pwm_count_conversion
[      PASS] test_pca9685_pwm_count_conversion
[RUN       ] test_smooth_trajectory_step
[      PASS] test_smooth_trajectory_step
[RUN       ] test_home_and_rest_preset_validity
[      PASS] test_home_and_rest_preset_validity
[RUN       ] test_mock_pwm_driver
[      PASS] test_mock_pwm_driver
========================================================
                      TEST SUMMARY                      
========================================================
  Suites total:  8
  Suites passed: 8
  Assertions:    891
  Assert failed: 0
>>> ALL UNIT TESTS PASSED SUCCESSFULLY! <<<
```

---

## 🛠 Compilación, Flasheo y Ejecución

### Requisitos Previos

- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/installation/index.html) o extensión PlatformIO IDE para VS Code.
- Compilador GCC/G++ compatible con C++17 (para pruebas nativas en host).

### Compilación del Firmware

```bash
# Compilar imagen de firmware para el microcontrolador
pio run

# Compilar y subir por puerto serie (ajustar puerto si es necesario)
pio run -t upload

# Cargar archivos web estáticos (data/index.html) al sistema de archivos flash SPIFFS
pio run -t uploadfs
```

### Conexión Operativa

1. Encender el ESP32 y el riel de potencia de los servomotores.
2. Desde un smartphone o portátil, conectarse a la red Wi-Fi:
   - **SSID:** `Robotic-Arm`
   - **Seguridad:** Abierta / Sin contraseña
3. Abrir cualquier navegador web y navegar a la dirección IP por defecto:
   - **URL:** `http://192.168.4.5` (o `http://192.168.4.1`)
4. El panel cargará de inmediato la conexión WebSocket y reflejará el estado de los 6 servomotores.

---

## 📁 Estructura del Proyecto

```
Mars-Rover/
├── data/
│   ├── index.html               # Web Dashboard táctil Dark Sci-Fi
│   └── virtualjoystick.js       # Soporte de control táctil y gestos
├── platformio.ini               # Configuración de compilación y librerías ESP32
├── src/
│   ├── arm_config.h             # Geometría, pines I2C, límites y presets
│   ├── arm_driver.h             # Interfaz abstracta del driver PWM
│   ├── arm_kinematics.h         # Declaración de funciones cinemáticas puras
│   ├── arm_kinematics.cpp       # Implementación cinemática y trayectoria
│   ├── arm_servo.h              # Interfaz de control de servos y FreeRTOS
│   ├── arm_servo.cpp            # Driver PCA9685 y tarea ArmServoUpdater
│   ├── arm.h                    # API de alto nivel del brazo
│   ├── arm.cpp                  # Lógica de coordinación y presets
│   ├── main.cpp                 # Punto de entrada de firmware ESP32
│   ├── wifi_controller.h       # Cabecera de gestión Wi-Fi y WebSockets
│   └── wifi_controller.cpp     # Servidor asíncrono y despacho de comandos
├── test/
│   ├── run_tests.sh             # Script de compilación y ejecución de tests
│   └── test_arm_kinematics.cpp  # Suite de pruebas unitarias (891 aserciones)
├── LICENSE                      # Licencia MIT
└── README.md                    # Documentación técnica completa
```

---

## 📜 Atribuciones y Licencia

- **Licencia:** Distribuido bajo la Licencia [MIT](LICENSE).
- **Créditos:** Concepto original del chasis y prototipo mecánico inspirado en el proyecto de código abierto de **JakKra** (*Mars Rover*).
- **Desarrollo del Controlador 6-DOF:** Implementación del controlador cinemático, interpolador FreeRTOS, driver PCA9685, suite de tests y web dashboard por el equipo del proyecto Mars Rover Tecnoteca.
