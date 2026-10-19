# Esquemas de Ingeniería y Diagramas de Conexión: Mars Rover (L298N Skid-Steering)

Este documento reúne el conjunto completo de especificaciones técnicas y esquemáticos de ingeniería para el Mars Rover con tracción diferencial (**Skid-Steering**) empleando un controlador puente H dual **L298N** para 4 motores DC, sin servomotores.

---

## 1. Esquema 1: Conexión y Control de Señales (ESP32 -> Puente H L298N)

Control directo del módulo L298N mediante 6 líneas de control desde el ESP32: 2 canales PWM por hardware (LEDC) para control de velocidad y 4 pines GPIO digitales para selección de sentido de giro.

### Mapeo de Señales (Pinout ESP32 -> L298N)
| Pin ESP32 | Terminal L298N | Color de Línea | Función Técnica | Estado Previo |
| :---: | :---: | :---: | :--- | :--- |
| **GPIO 17** | **ENA** | Azul (`#2980B9`) | PWM Velocidad - Tren Izquierdo (Canal LEDC 0, 1 kHz) | ESC Izquierdo |
| **GPIO 18** | **IN1** | Verde (`#27AE60`) | Dirección Lógica A - Tren Izquierdo | Servo Delantero Izq. |
| **GPIO 19** | **IN2** | Amarillo (`#F1C40F`) | Dirección Lógica B - Tren Izquierdo | Servo Delantero Der. |
| **GPIO 21** | **IN3** | Púrpura (`#8E44AD`) | Dirección Lógica A - Tren Derecho | Servo Trasero Izq. |
| **GPIO 22** | **IN4** | Magenta (`#9B59B6`) | Dirección Lógica B - Tren Derecho | Servo Trasero Der. |
| **GPIO 13** | **ENB** | Naranja (`#D35400`) | PWM Velocidad - Tren Derecho (Canal LEDC 1, 1 kHz) | ESC Derecho |

---

## 2. Esquema 2: Subsistema de Motores DC (4WD en Tracción Diferencial)

Los 4 motores DC se configuran en dos pares laterales conectados en paralelo a las salidas de potencia del L298N:
* **Canal A (OUT1 / OUT2):** Conecta en paralelo los dos motores del costado izquierdo (Delantero Izquierdo y Trasero Izquierdo).
* **Canal B (OUT3 / OUT4):** Conecta en paralelo los dos motores del costado derecho (Delantero Derecho y Trasero Derecho).

### Tabla de Verdad L298N y Movimiento del Rover:
| Maniobra | IN1 | IN2 | ENA (PWM) | IN3 | IN4 | ENB (PWM) | Efecto Cinemático |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **Avance Recto** | `HIGH` | `LOW` | $V$ | `HIGH` | `LOW` | $V$ | Ambos laterales avanzan a la misma velocidad |
| **Retroceso Recto** | `LOW` | `HIGH` | $V$ | `LOW` | `HIGH` | $V$ | Ambos laterales retroceden a la misma velocidad |
| **Giro Derecha (Curva)** | `HIGH` | `LOW` | $V_{\text{alto}}$ | `HIGH` | `LOW` | $V_{\text{bajo}}$ | Tren izquierdo más rápido que el derecho |
| **Giro Izquierda (Curva)** | `HIGH` | `LOW` | $V_{\text{bajo}}$ | `HIGH` | `LOW` | $V_{\text{alto}}$ | Tren derecho más rápido que el izquierdo |
| **Spin 360° Horario** | `HIGH` | `LOW` | $V$ | `LOW` | `HIGH` | $V$ | Izquierda avance, derecha reversa (giro sobre eje) |
| **Spin 360° Antihorario**| `LOW` | `HIGH` | $V$ | `HIGH` | `LOW` | $V$ | Izquierda reversa, derecha avance (giro sobre eje) |
| **Freno / Parada** | `LOW` | `LOW` | $0$ | `LOW` | `LOW` | $0$ | Desconexión por inercia o parada activa |

---

## 3. Esquema 3: Distribución de Energía (Batería, L298N y ESP32)

1. **Alimentación de Motores (+7.4V / +12V):**
   - Terminal `+12V` / `VCC` del L298N conectado al positivo de la batería tras interruptor de encendido.
2. **GND Común:**
   - Terminal `GND` de la batería, terminal `GND` del L298N y terminal `GND` del ESP32 conectados al mismo bus de masa común.
3. **Regulación Lógica 5V:**
   - El L298N cuenta con regulador interno de 5V (habilitado con el jumper de 5V-EN activado). Su salida de 5V puede alimentar el pin `VIN` / `5V` del ESP32 cuando se usa batería de 7.4V - 12V.
