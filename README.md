# Mars Rover — ESP32 Autonomous Robotic Vehicle & Skid-Steering Firmware

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Core%206.0%2B-orange.svg?logo=platformio)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20ESP32-blue.svg?logo=espressif)](https://github.com/espressif/arduino-esp32)
[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg?logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![Tests](https://img.shields.io/badge/Tests-108%2F108%20Passed%20(100%25)-brightgreen.svg)]()
[![Simulation](https://img.shields.io/badge/Wokwi-Simulated-success.svg?logo=web)](https://wokwi.com)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Firmware de alta ingeniería y control para un vehículo robótico de exploración terrestre inspirado en los rovers marcianos (*Curiosity / Perseverance*). El sistema está impulsado por un SoC **ESP32**, implementando tracción diferencial (**Skid-Steering**) para 4WD / 6WD, cinemática desacoplada en C++ puro, servidor web asíncrono con control en tiempo real mediante **WebSockets** y soporte nativo para simulación interactiva completa en **Wokwi**.

---

## 📋 Tabla de Contenidos

- [Características Principales](#-características-principales)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
  - [Cinemática Desacoplada y Failsafe](#cinemática-desacoplada-y-failsafe)
  - [Control de Tracción (L298N / ESCs)](#control-de-tracción-l298n--escs)
- [Interfaz Web y Telemetría en Tiempo Real](#-interfaz-web-y-telemetría-en-tiempo-real)
- [Esquemas Eléctricos y Distribución de Potencia](#-esquemas-eléctricos-y-distribución-de-potencia)
  - [Mapeo de Pines (ESP32 a L298N)](#mapeo-de-pines-esp32-a-l298n)
  - [Topología de Alimentación y Masa en Estrella](#topología-de-alimentación-y-masa-en-estrella)
- [Simulación Interactiva en Wokwi](#-simulación-interactiva-en-wokwi)
- [Instalación, Compilación y Flasheo](#-instalación-compilación-y-flasheo)
- [Batería de Pruebas Unitarias](#-batería-de-pruebas-unitarias)
- [Estructura del Repositorio](#-estructura-del-repositorio)
- [Atribuciones y Licencia](#-atribuciones-y-licencia)

---

## 🚀 Características Principales

- **SoC ESP32 Dual Core (240 MHz):** Asignación eficiente de recursos entre el subsistema de comunicaciones Wi-Fi/WebSockets y el bucle de control cinemático por hardware.
- **Tracción Diferencial Skid-Steering (4WD / 6WD):** Algoritmo de mezcla vectorial diferencial para giro continuo suave y modo axial **Spin 360°** (giro sobre su propio eje).
- **Cinemática Pura Desacoplada:** Lógica matemática (`rover_kinematics`) escrita en C++17 sin acoplamiento a frameworks de terceros, testeable nativamente tanto en microcontrolador como en PC host.
- **Punto de Acceso Autónomo (AP):** Genera su propia red Wi-Fi (`SSID: Rover`) sin requerir routers externos o conexión a Internet.
- **Control Web Táctil de Ultra Baja Latencia:** Servidor web asíncrono (`ESPAsyncWebServer`) y canal bidireccional WebSocket con joysticks virtuales interactivos y botón de parada de emergencia (*Active E-Stop*).
- **Simulación Wokwi Lista para Usar:** Configuración llave en mano (`diagram.json`, `wokwi.toml`, `sketch.ino`) para probar y validar el rover tanto en navegador como en VS Code.
- **Topología de Potencia Segura:** Aislamiento galvánico y de masa en estrella (*Star Ground Topology*) con doble riel de alimentación (7.4V/12V para motores y 5V regulados para lógica/servos).

---

## 🏗 Arquitectura del Sistema

```
                   +----------------------------------+
                   |    Dispositivo Móvil / Laptop    |
                   |   (Navegador Web: 192.168.4.1)   |
                   +-----------------+----------------+
                                     |  Wi-Fi 802.11b/g/n (SoftAP)
                                     v
                   +----------------------------------+
                   |          ESP32 SoC (Core)        |
                   |  - ESPAsyncWebServer (Puerto 80) |
                   |  - WebSockets Server (Puerto 81) |
                   +-----------------+----------------+
                                     |
                                     v
                   +----------------------------------+
                   |      rover_kinematics (C++17)    |
                   |  - Deadzone (1480 - 1520 µs)     |
                   |  - Failsafe (señal <= 800 µs)    |
                   |  - Algoritmo Skid-Steering       |
                   |  - Modo Spin 360° / In-Place     |
                   +-----------------+----------------+
                                     |
                  +------------------+------------------+
                  | LEDC PWM Hardware (1 kHz, 8 bits)  |
                  +------------------+------------------+
                                     |
          +--------------------------+--------------------------+
          |                                                     |
          v                                                     v
+--------------------+                                +--------------------+
| L298N / ESC Izq.   |                                | L298N / ESC Der.   |
| (ENA, IN1, IN2)    |                                | (ENB, IN3, IN4)    |
+---------+----------+                                +---------+----------+
          |                                                     |
          v                                                     v
+--------------------+                                +--------------------+
| Motores Izquierdos |                                | Motores Derechos   |
| (Front & Rear DC)  |                                | (Front & Rear DC)  |
+--------------------+                                +--------------------+
```

### Cinemática Desacoplada y Failsafe

El módulo `src/rover_kinematics.cpp` procesa las señales normalizadas de los controles de aceleración (*Throttle*) y dirección (*Steering*) en microsegundos de estándar radiocontrol (1000 µs a 2000 µs, neutro 1500 µs):

1. **Protección Fail-Safe:** Si la conexión se pierde o la señal cae a $\le 800\,\mu\text{s}$, el rover activa instantáneamente la condición de neutro seguro ($1500\,\mu\text{s}$, velocidad cero, motores en parada activa).
2. **Zona Muerta Dinámica (*Deadband*):** Un umbral entre $1480\,\mu\text{s}$ y $1520\,\mu\text{s}$ evita ruidos residuales o deriva mecánica de los joysticks.
3. **Control Diferencial:**
   - **Avance/Retroceso Lineal:** Velocidad idéntica en ambos trenes laterales.
   - **Giro en Curva:** Reducción proporcional de velocidad en el tren interior y aceleración en el exterior.
   - **Modo Spin 360°:** Inversión de fase donde las ruedas izquierdas giran en sentido inverso a las derechas a velocidades emparejadas, rotando el chasis sobre su centro de masa.

### Control de Tracción (L298N / ESCs)

Para la etapa de potencia se emplea un puente H dual **L298N** (o ESCs con señal RC), controlando cuatro motores con reductores DC:
- Los motores izquierdos se cablean en paralelo al Canal A (`OUT1`, `OUT2`).
- Los motores derechos se cablean en paralelo al Canal B (`OUT3`, `OUT4`).
- Canales LEDC de hardware en el ESP32 a **1 kHz** con resolución de 8 bits (0-255), garantizando transiciones de torque suaves y silenciosas.

---

## 🕹 Interfaz Web y Telemetría en Tiempo Real

Al alimentar el rover, el ESP32 levanta automáticamente un punto de acceso Wi-Fi:
- **SSID:** `Rover`
- **Dirección IP del Gateway:** `http://192.168.4.1`

La interfaz web alojada en `data/index.html` (servida directamente desde memoria Flash o mediante SPIFFS) ofrece:
- **Doble Joystick Táctil:** Control analógico intuitivo en pantallas móviles y computadoras.
- **Selector de Modo en Caliente:** Conmutación rápida entre modo de navegación ordinario (*Skid-Steering*) y modo de giro en el lugar (*Spin 360°*).
- **Parada de Emergencia Inmediata:** Botón de corte de potencia activo en caso de obstáculo imprevisto.
- **Telemetría en Vivo:** Estado de conexión WebSocket, modulación de señal PWM y porcentaje de empuje.

---

## ⚡ Esquemas Eléctricos y Distribución de Potencia

La documentación técnica y diagramas detallados se encuentran disponibles en la carpeta [`docs/`](docs/):

### Mapeo de Pines (ESP32 a L298N)

| Pin ESP32 | Terminal L298N | Color en Diagrama | Función Técnica | Canal Hardware |
| :---: | :---: | :---: | :--- | :---: |
| **GPIO 17** | **ENA** | Azul | PWM Velocidad — Tren Izquierdo | LEDC Ch 0 (1 kHz) |
| **GPIO 18** | **IN1** | Verde | Dirección Lógica A — Tren Izquierdo | GPIO Digital |
| **GPIO 19** | **IN2** | Amarillo | Dirección Lógica B — Tren Izquierdo | GPIO Digital |
| **GPIO 21** | **IN3** | Púrpura | Dirección Lógica A — Tren Derecho | GPIO Digital |
| **GPIO 22** | **IN4** | Magenta | Dirección Lógica B — Tren Derecho | GPIO Digital |
| **GPIO 13** | **ENB** | Naranja | PWM Velocidad — Tren Derecho | LEDC Ch 1 (1 kHz) |

### Topología de Alimentación y Masa en Estrella

Para prevenir caídas de tensión bruscas causadas por la corriente de arranque de los motores DC que reinicien el microcontrolador (*brownout resets*):
- **Línea de Tracción (+7.4V a +12V):** Conexión directa desde la batería LiPo / Li-Ion hacia el terminal de alta potencia del L298N / ESCs con capacitor electrolítico de desacoplo.
- **Línea Lógica (+5V):** Generada mediante el regulador interno del módulo L298N (o UBEC conmutado externo de 5V/5A) hacia el pin `VIN` / `5V` del ESP32.
- **GND Común en Estrella:** Todas las referencias de tierra convergen en un único nodo común para neutralizar bucles de masa y transitorios inductivos.

Para ver los diagramas generados en alta resolución:
- 📄 [Esquema Completo de Conexiones (PDF)](docs/schematic_circuit_diagram.pdf) | [Imagen PNG](docs/schematic_circuit_diagram.png)
- 📄 [Esquema del Subsistema de Tracción (PDF)](docs/schematic_motors_traction.pdf) | [Imagen PNG](docs/schematic_motors_traction.png)
- 📄 [Esquema de Distribución de Energía (PDF)](docs/schematic_power_supply.pdf) | [Imagen PNG](docs/schematic_power_supply.png)
- 📝 [Especificaciones Técnicas de Circuitos](docs/circuit_schematic.md)

---

## 🧪 Simulación Interactiva en Wokwi

El proyecto incluye simulación completa lista para ejecutar en **Wokwi** sin requerir hardware físico:

### En Visual Studio Code (con extensión Wokwi):
1. Instale la extensión **Wokwi Simulator** desde el marketplace de VS Code.
2. Abra este repositorio en VS Code.
3. Presione `F1` y seleccione `Wokwi: Start Simulator`.
4. El simulador cargará automáticamente el ESP32, el puente H virtual, las salidas PWM y la consola serie (`diagram.json` y `wokwi.toml`).

### En Wokwi Web:
- Cargue el código de [`sketch.ino`](sketch.ino) junto a [`diagram.json`](diagram.json) y [`libraries.txt`](libraries.txt) directamente en [wokwi.com](https://wokwi.com).

---

## 🛠 Instalación, Compilación y Flasheo

### Requisitos Previos

- [PlatformIO Core o PlatformIO IDE para VS Code](https://platformio.org/install)
- Python 3.8+
- Compilador C++17 (`g++` o `clang++`) para pruebas locales en PC.

### 1. Clonar el Repositorio

```bash
git clone https://github.com/jlameda0305/Mars-Rover.git
cd Mars-Rover
```

### 2. Compilar el Firmware con PlatformIO

```bash
pio run
```

### 3. Flashear el Firmware al ESP32

Conecte la placa ESP32 por USB y ejecute:

```bash
pio run -t upload
```

### 4. Subir la Interfaz Web a la Memoria SPIFFS

```bash
pio run -t uploadfs
```

---

## 🔬 Batería de Pruebas Unitarias

El repositorio cuenta con dos suites de verificación exhaustiva con **100% de aserciones superadas**:

### 1. Suite Nativa en C++17 (Host PC)

Ejecución sin dependencias de hardware externo:

```bash
g++ -std=c++17 test/test_rover_kinematics.cpp src/rover_kinematics.cpp -Isrc -o test/run_tests && ./test/run_tests && rm test/run_tests
```

**Resultado:**
```text
====================================================
 MARS ROVER SKID-STEERING / L298N TEST SUITE        
====================================================
SUITE: filter_rc_signal (Deadzone, Fail-safe, Extremos) -> PASSED
SUITE: Skid-Steering: Avance y Retroceso en Linea Recta -> PASSED
SUITE: Skid-Steering: Giros Diferenciales              -> PASSED
SUITE: Modo Spin 360 (Giro Axial Puro)                 -> PASSED
SUITE: L298N: Mapeo de Salidas PWM y Direcciones       -> PASSED
SUITE: Fail-safe y Limites de Senal                    -> PASSED
====================================================
  Total Suites: 6 | Passed: 6 | Total Assertions: 108 | Passed: 108
>>> OVERALL STATUS: ALL TESTS PASSED (100%) <<<
```

### 2. Suite de Validación Funcional en Python

```bash
python3 test/verify_rover_kinematics.py
```

Valida la respuesta cinemática, linealidad PWM y fail-safes en 108 aserciones adicionales.

---

## 📂 Estructura del Repositorio

```text
Mars-Rover/
├── data/
│   ├── index.html                # Interfaz web responsiva con WebSockets y joysticks táctiles
│   └── virtualjoystick.js        # Librería cliente para joysticks táctiles virtuales
├── docs/
│   ├── circuit_schematic.md      # Especificaciones técnicas de ingeniería eléctrica
│   ├── schematic_circuit_diagram.* # Diagrama esquemático maestro (PDF, PNG, SVG)
│   ├── schematic_motors_traction.* # Esquema de potencia para motores y puente H
│   └── schematic_power_supply.*    # Esquema de distribución de alimentación y filtrado
├── src/
│   ├── main.cpp                  # Punto de entrada de firmware y orquestación
│   ├── rover_config.h            # Definición centralizada de pines, frecuencias y canales
│   ├── rover_driving.cpp/.h      # Controlador de bajo nivel para pines e interfaz L298N
│   ├── rover_kinematics.cpp/.h   # Modelo cinemático puro C++ (Skid-Steering, Failsafe)
│   ├── rover_servo.cpp/.h        # Módulo de compatibilidad para servos desacoplados
│   └── wifi_controller.cpp/.h    # Administrador de Wi-Fi AP y servidor WebSocket
├── test/
│   ├── test_rover_kinematics.cpp # Suite unitaria nativa en C++17
│   └── verify_rover_kinematics.py# Suite de verificación funcional en Python
├── diagram.json                  # Definición gráfica de hardware para simulación Wokwi
├── generate_schematic.py         # Generador automatizado de esquemático principal
├── generate_detail_schematics.py # Generador automatizado de esquemáticos detallados
├── libraries.txt                 # Dependencias para simulación web en Wokwi
├── platformio.ini                # Configuración de compilación para ESP32 Dev Module
├── sketch.ino                    # Firmware autónomo para simulación directa en Wokwi
├── test_scenario.yaml            # Escenario de pruebas automatizadas Wokwi
├── wokwi.toml                    # Configuración de simulación Wokwi para VS Code
├── .gitignore                    # Reglas de exclusión para Git
├── LICENSE                       # Licencia MIT y reconocimientos
└── README.md                     # Documentación principal del proyecto
```

---

## 📜 Atribuciones y Licencia

Este proyecto está distribuido bajo la **Licencia MIT**. Consulte el archivo [`LICENSE`](LICENSE) para obtener más detalles.

### Reconocimientos
- *Portions and original concept based on Mars-Rover by JakKra ([https://github.com/jakkra/Mars-Rover](https://github.com/jakkra/Mars-Rover)), licensed under the MIT License.*
- Rediseño, cinemática desacoplada en C++17, suite de pruebas unitarias, arquitectura L298N Skid-Steering y esquemáticos técnicos desarrollados por **Jlameda (2026)**.
