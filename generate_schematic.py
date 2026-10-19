import os
import subprocess

def create_schematic_svg():
    width = 1600
    height = 1130
    
    # Colors matching professional Tinkercad EDA schematic export
    c_frame = "#C0392B" # Engineering drawing frame red
    c_wire_red = "#E74C3C" # 5V Power
    c_wire_black = "#1A252F" # GND Common
    c_wire_pwm1 = "#2980B9" # Blue (ESC Left GPIO 17)
    c_wire_pwm2 = "#D35400" # Orange (ESC Right GPIO 13)
    c_wire_srv1 = "#27AE60" # Green (FL GPIO 18)
    c_wire_srv2 = "#16A085" # Teal (FR GPIO 19)
    c_wire_srv3 = "#8E44AD" # Purple (BL GPIO 21)
    c_wire_srv4 = "#9B59B6" # Magenta (BR GPIO 22)
    
    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" style="background-color: #FCFDFD; font-family: -apple-system, BlinkMacSystemFont, Arial, sans-serif;">')
    
    # Grid background pattern
    svg.append('''
    <defs>
      <pattern id="grid" width="20" height="20" patternUnits="userSpaceOnUse">
        <path d="M 20 0 L 0 0 0 20" fill="none" stroke="#F0F3F4" stroke-width="0.8"/>
      </pattern>
      <filter id="shadow" x="-4%" y="-3%" width="108%" height="108%">
        <feDropShadow dx="1" dy="2" stdDeviation="2.5" flood-opacity="0.08"/>
      </filter>
    </defs>
    <rect width="100%" height="100%" fill="url(#grid)" />
    ''')
    
    # ----------------------------------------------------
    # 1. ENGINEERING FRAME & TITLE BLOCK (Tinkercad Style)
    # ----------------------------------------------------
    # Outer Border
    svg.append(f'<rect x="40" y="40" width="{width-80}" height="{height-80}" fill="none" stroke="{c_frame}" stroke-width="2.5"/>')
    # Inner Border
    svg.append(f'<rect x="58" y="58" width="{width-116}" height="{height-116}" fill="none" stroke="{c_frame}" stroke-width="1"/>')
    
    # Grid Coordinates (A-E rows, 1-6 columns)
    cols = 6
    col_w = (width - 116) / cols
    for i in range(cols):
        cx = 58 + i * col_w
        svg.append(f'<line x1="{cx}" y1="40" x2="{cx}" y2="58" stroke="{c_frame}" stroke-width="1"/>')
        svg.append(f'<text x="{cx + col_w/2}" y="52" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{i+1}</text>')
        svg.append(f'<line x1="{cx}" y1="{height-58}" x2="{cx}" y2="{height-40}" stroke="{c_frame}" stroke-width="1"/>')
        svg.append(f'<text x="{cx + col_w/2}" y="{height-44}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{i+1}</text>')
    
    rows = 5
    row_h = (height - 116) / rows
    row_labels = ["A", "B", "C", "D", "E"]
    for j in range(rows):
        ry = 58 + j * row_h
        svg.append(f'<line x1="40" y1="{ry}" x2="58" y2="{ry}" stroke="{c_frame}" stroke-width="1"/>')
        svg.append(f'<text x="50" y="{ry + row_h/2 + 5}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{row_labels[j]}</text>')
        svg.append(f'<line x1="{width-58}" y1="{ry}" x2="{width-40}" y2="{ry}" stroke="{c_frame}" stroke-width="1"/>')
        svg.append(f'<text x="{width-48}" y="{ry + row_h/2 + 5}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{row_labels[j]}</text>')

    # Bottom Left Brand Note
    svg.append(f'<text x="70" y="{height-68}" fill="#7F8C8D" font-size="13" font-weight="bold">Mars Rover Project &#8226; Circuit Schematic Diagram (Standalone Direct GPIO Control)</text>')
    
    # Title Block (Bottom Right)
    tb_x = width - 58 - 490
    tb_y = height - 58 - 85
    tb_w = 490
    tb_h = 85
    svg.append(f'<rect x="{tb_x}" y="{tb_y}" width="{tb_w}" height="{tb_h}" fill="#FFFFFF" stroke="{c_frame}" stroke-width="1.5"/>')
    svg.append(f'<line x1="{tb_x}" y1="{tb_y+32}" x2="{tb_x+tb_w}" y2="{tb_y+32}" stroke="{c_frame}" stroke-width="1"/>')
    svg.append(f'<line x1="{tb_x}" y1="{tb_y+58}" x2="{tb_x+tb_w}" y2="{tb_y+58}" stroke="{c_frame}" stroke-width="1"/>')
    svg.append(f'<line x1="{tb_x+330}" y1="{tb_y+32}" x2="{tb_x+330}" y2="{tb_y+tb_h}" stroke="{c_frame}" stroke-width="1"/>')
    
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+22}" fill="{c_frame}" font-size="14" font-weight="bold">Title: <tspan fill="#2C3E50" font-weight="normal">Mars Rover - 6-Wheel Drive &amp; 4WS Direct GPIO</tspan></text>')
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+48}" fill="{c_frame}" font-size="12" font-weight="bold">Date: <tspan fill="#34495E" font-weight="normal">15/10/2026</tspan></text>')
    svg.append(f'<text x="{tb_x+340}" y="{tb_y+48}" fill="{c_frame}" font-size="12" font-weight="bold">Sheet: <tspan fill="#34495E" font-weight="normal">1/1</tspan></text>')
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+74}" fill="{c_frame}" font-size="12" font-weight="bold">Arch: <tspan fill="#34495E" font-weight="normal">ESP32 DevKit-C v4 (No PCA9685)</tspan></text>')
    svg.append(f'<text x="{tb_x+340}" y="{tb_y+74}" fill="{c_frame}" font-size="12" font-weight="bold">Rev: <tspan fill="#34495E" font-weight="normal">2.0 (Direct)</tspan></text>')

    # ----------------------------------------------------
    # 2. SCHEMATIC COMPONENTS
    # ----------------------------------------------------
    u1_x = 580
    u1_y = 130
    u1_w = 260
    u1_h = 760
    
    svg.append(f'<rect x="{u1_x}" y="{u1_y}" width="{u1_w}" height="{u1_h}" fill="#FFFFFF" stroke="#2C3E50" stroke-width="2" filter="url(#shadow)"/>')
    svg.append(f'<rect x="{u1_x}" y="{u1_y}" width="{u1_w}" height="40" fill="#2C3E50"/>')
    svg.append(f'<text x="{u1_x+u1_w/2}" y="{u1_y+25}" fill="#FFFFFF" font-size="15" font-weight="bold" text-anchor="middle">U1 : ESP32 DEVKIT-C V4</text>')
    
    left_pins = [
        ("3V3", "3.3V Power Out"),
        ("EN", "Reset / Enable"),
        ("VP / 36", "Sensor VP"),
        ("VN / 39", "Sensor VN"),
        ("D34", "GPI 34"),
        ("D35", "GPI 35"),
        ("D32", "GPIO 32"),
        ("D33", "GPIO 33"),
        ("D25", "GPIO 25"),
        ("D26", "GPIO 26"),
        ("D27", "GPIO 27"),
        ("D14", "GPIO 14"),
        ("D12", "GPIO 12"),
        ("GND.1", "Ground")
    ]
    
    right_pins = [
        ("GND.2", "Ground (Power Ref)"),
        ("D23", "GPIO 23"),
        ("D22", "SERVO_BACK_RIGHT (PWM)"),
        ("TX0 / 1", "UART TX0 (Serial Mon)"),
        ("RX0 / 3", "UART RX0 (Serial Mon)"),
        ("D21", "SERVO_BACK_LEFT (PWM)"),
        ("D19", "SERVO_FRONT_RIGHT (PWM)"),
        ("D18", "SERVO_FRONT_LEFT (PWM)"),
        ("D5", "GPIO 5"),
        ("D17", "ROVER_MOTORS_LEFT (PWM)"),
        ("D16", "GPIO 16"),
        ("D4", "GPIO 4"),
        ("D0", "GPIO 0 (BOOT)"),
        ("D2", "GPIO 2 (LED)"),
        ("D15", "GPIO 15"),
        ("D13", "ROVER_MOTORS_RIGHT (PWM)"),
        ("5V / VIN", "Power Input (5V)")
    ]
    
    lp_y_start = u1_y + 70
    lp_step = 50
    left_pin_coords = {}
    for idx, (pname, pdesc) in enumerate(left_pins):
        py = lp_y_start + idx * lp_step
        svg.append(f'<line x1="{u1_x-20}" y1="{py}" x2="{u1_x}" y2="{py}" stroke="#7F8C8D" stroke-width="1.5"/>')
        svg.append(f'<circle cx="{u1_x-20}" cy="{py}" r="3" fill="#C0392B"/>')
        svg.append(f'<text x="{u1_x+10}" y="{py+4}" fill="#2C3E50" font-size="12" font-weight="bold">{pname}</text>')
        left_pin_coords[pname] = (u1_x - 20, py)

    rp_y_start = u1_y + 60
    rp_step = 42
    right_pin_coords = {}
    for idx, (pname, pdesc) in enumerate(right_pins):
        py = rp_y_start + idx * rp_step
        svg.append(f'<line x1="{u1_x+u1_w}" y1="{py}" x2="{u1_x+u1_w+20}" y2="{py}" stroke="#7F8C8D" stroke-width="1.5"/>')
        svg.append(f'<circle cx="{u1_x+u1_w+20}" cy="{py}" r="3" fill="#C0392B"/>')
        
        is_active = any(k in pname for k in ["17", "13", "18", "19", "21", "22", "5V", "GND.2", "TX0", "RX0"])
        txt_fill = "#C0392B" if is_active else "#7F8C8D"
        weight = "bold" if is_active else "normal"
        svg.append(f'<text x="{u1_x+u1_w-10}" y="{py+4}" fill="{txt_fill}" font-size="12" font-weight="{weight}" text-anchor="end">{pname}</text>')
        right_pin_coords[pname] = (u1_x + u1_w + 20, py)

    # ----------------------------------------------------
    # 3. RIGHT SIDE: ACTUATORS (6x Servos & ESCs)
    # ----------------------------------------------------
    actuators = [
        ("S1", "SRV_FRONT_LEFT", "Servo Dirección Delantero Izq", 18, c_wire_srv1, 130),
        ("S2", "SRV_FRONT_RIGHT", "Servo Dirección Delantero Der", 19, c_wire_srv2, 250),
        ("S3", "SRV_BACK_LEFT", "Servo Dirección Trasero Izq", 21, c_wire_srv3, 370),
        ("S4", "SRV_BACK_RIGHT", "Servo Dirección Trasero Der", 22, c_wire_srv4, 490),
        ("M1", "ESC_LEFT", "ESC Tracción Izquierda (3 Motores)", 17, c_wire_pwm1, 640),
        ("M2", "ESC_RIGHT", "ESC Tracción Derecha (3 Motores)", 13, c_wire_pwm2, 780),
    ]
    
    act_x = 1140
    act_w = 260
    act_h = 95
    
    for tag, name, desc, pin_num, color, ay in actuators:
        svg.append(f'<rect x="{act_x}" y="{ay}" width="{act_w}" height="{act_h}" rx="6" fill="#FFFFFF" stroke="{color}" stroke-width="2" filter="url(#shadow)"/>')
        svg.append(f'<rect x="{act_x}" y="{ay}" width="{act_w}" height="26" rx="4" fill="{color}"/>')
        svg.append(f'<text x="{act_x+10}" y="{ay+18}" fill="#FFFFFF" font-size="12" font-weight="bold">{tag} : {name}</text>')
        svg.append(f'<text x="{act_x+10}" y="{ay+44}" fill="#7F8C8D" font-size="11">{desc}</text>')
        
        # PWM Pin
        pwm_y = ay + 65
        svg.append(f'<line x1="{act_x-20}" y1="{pwm_y}" x2="{act_x}" y2="{pwm_y}" stroke="{color}" stroke-width="1.8"/>')
        svg.append(f'<circle cx="{act_x-20}" cy="{pwm_y}" r="3.5" fill="{color}"/>')
        svg.append(f'<text x="{act_x+8}" y="{pwm_y+4}" fill="{color}" font-size="11" font-weight="bold">PWM (Pin {pin_num})</text>')
        
        # V+ Pin
        v_y = ay + 80
        svg.append(f'<line x1="{act_x-20}" y1="{v_y}" x2="{act_x}" y2="{v_y}" stroke="{c_wire_red}" stroke-width="1.5"/>')
        svg.append(f'<circle cx="{act_x-20}" cy="{v_y}" r="3" fill="{c_wire_red}"/>')
        svg.append(f'<text x="{act_x+8}" y="{v_y+4}" fill="{c_wire_red}" font-size="10" font-weight="bold">V+ (5V-6V Ext)</text>')
        
        # GND Pin
        gnd_y = ay + 80
        svg.append(f'<line x1="{act_x+act_w}" y1="{gnd_y}" x2="{act_x+act_w+20}" y2="{gnd_y}" stroke="{c_wire_black}" stroke-width="1.5"/>')
        svg.append(f'<circle cx="{act_x+act_w+20}" cy="{gnd_y}" r="3" fill="{c_wire_black}"/>')
        svg.append(f'<text x="{act_x+act_w-8}" y="{gnd_y+4}" fill="{c_wire_black}" font-size="10" font-weight="bold" text-anchor="end">GND</text>')
        
        # ROUTE PWM WIRES
        esp_pin_name = [k for k in right_pins if f"D{pin_num}" in k[0]][0][0]
        esp_x, esp_y = right_pin_coords[esp_pin_name]
        bus_x = 880 + (pin_num % 10) * 16
        svg.append(f'<path d="M {esp_x} {esp_y} L {bus_x} {esp_y} L {bus_x} {pwm_y} L {act_x-20} {pwm_y}" fill="none" stroke="{color}" stroke-width="2.2" stroke-linejoin="round"/>')

    # ----------------------------------------------------
    # 4. POWER SUPPLY SECTION (Left Side)
    # ----------------------------------------------------
    pwr_x = 100
    pwr_y = 150
    pwr_w = 260
    
    # Battery B1
    b_y = pwr_y
    svg.append(f'<rect x="{pwr_x}" y="{b_y}" width="{pwr_w}" height="100" rx="6" fill="#FFFFFF" stroke="#E67E22" stroke-width="2" filter="url(#shadow)"/>')
    svg.append(f'<rect x="{pwr_x}" y="{b_y}" width="{pwr_w}" height="26" rx="4" fill="#E67E22"/>')
    svg.append(f'<text x="{pwr_x+10}" y="{b_y+18}" fill="#FFFFFF" font-size="12" font-weight="bold">B1 : BATERÍA PRINCIPAL (LiPo 2S / 7.4V)</text>')
    svg.append(f'<text x="{pwr_x+15}" y="{b_y+55}" fill="#E74C3C" font-size="13" font-weight="bold">+ POSITIVO (7.4V)</text>')
    svg.append(f'<text x="{pwr_x+15}" y="{b_y+85}" fill="#2C3E50" font-size="13" font-weight="bold">- NEGATIVO (GND)</text>')
    
    # Power Switch SW1
    sw_x = pwr_x + 70
    sw_y = b_y + 130
    svg.append(f'<rect x="{sw_x}" y="{sw_y}" width="120" height="50" rx="4" fill="#FFFFFF" stroke="#34495E" stroke-width="1.8"/>')
    svg.append(f'<text x="{sw_x+60}" y="{sw_y+30}" fill="#2C3E50" font-size="12" font-weight="bold" text-anchor="middle">SW1 : POWER</text>')
    
    # Voltage Regulator / UBEC (5V/6V 5A)
    reg_y = sw_y + 80
    svg.append(f'<rect x="{pwr_x}" y="{reg_y}" width="{pwr_w}" height="120" rx="6" fill="#FFFFFF" stroke="#27AE60" stroke-width="2" filter="url(#shadow)"/>')
    svg.append(f'<rect x="{pwr_x}" y="{reg_y}" width="{pwr_w}" height="26" rx="4" fill="#27AE60"/>')
    svg.append(f'<text x="{pwr_x+10}" y="{reg_y+18}" fill="#FFFFFF" font-size="12" font-weight="bold">U2 : UBEC / REGULADOR DC-DC (5V 5A)</text>')
    svg.append(f'<text x="{pwr_x+15}" y="{reg_y+55}" fill="#E74C3C" font-size="11">VIN (+) &lt;- Desde Batería</text>')
    svg.append(f'<text x="{pwr_x+15}" y="{reg_y+75}" fill="#2C3E50" font-size="11">GND (-) &lt;- Tierra Común</text>')
    svg.append(f'<text x="{pwr_x+15}" y="{reg_y+105}" fill="#E74C3C" font-size="12" font-weight="bold">VOUT (+5V Servos &amp; ESP32)</text>')

    # ----------------------------------------------------
    # 5. LEGEND BOX (Left Side Bottom - Cleanly separated)
    # ----------------------------------------------------
    leg_x = 100
    leg_y = 560
    leg_w = 340
    leg_h = 330
    svg.append(f'<rect x="{leg_x}" y="{leg_y}" width="{leg_w}" height="{leg_h}" rx="6" fill="#FFFFFF" stroke="#BDC3C7" stroke-width="1.5" filter="url(#shadow)"/>')
    svg.append(f'<rect x="{leg_x}" y="{leg_y}" width="{leg_w}" height="30" rx="4" fill="#34495E"/>')
    svg.append(f'<text x="{leg_x+12}" y="{leg_y+20}" fill="#FFFFFF" font-size="13" font-weight="bold">NOTAS Y LEYENDA DEL CIRCUITO</text>')
    
    notes = [
        ("&#8226; Tracción 6x6:", "3 Motores DC conectados en paralelo a cada ESC."),
        ("&#8226; Dirección 4WS:", "4 Servos en esquinas (GPIO 18, 19, 21, 22)."),
        ("&#8226; Sin PCA9685:", "Control PWM directo por hardware LEDC (ESP32)."),
        ("&#8226; Conexión WiFi:", "SoftAP autónomo 'Rover' (IP: 192.168.4.5)."),
        ("&#8226; Masa Común (GND):", "GND de Batería, Regulador, ESP32 y Servos"),
        ("", "deben estar unificados en un único nodo común."),
        ("&#8226; Potencia Externa:", "Los servos se alimentan a través del UBEC"),
        ("", "independiente, nunca desde el pin 3.3V/5V.")
    ]
    for i, (n_title, n_desc) in enumerate(notes):
        ny = leg_y + 60 + i * 32
        svg.append(f'<text x="{leg_x+14}" y="{ny}" fill="#C0392B" font-size="11" font-weight="bold">{n_title} <tspan fill="#2C3E50" font-weight="normal">{n_desc}</tspan></text>')

    # ----------------------------------------------------
    # 6. POWER DISTRIBUTION BUS WIRES (Routed around boxes)
    # ----------------------------------------------------
    # Battery -> Switch -> Regulator
    svg.append(f'<path d="M {pwr_x+130} {b_y+100} L {pwr_x+130} {sw_y}" fill="none" stroke="{c_wire_red}" stroke-width="2.5"/>')
    svg.append(f'<path d="M {pwr_x+130} {sw_y+50} L {pwr_x+130} {reg_y}" fill="none" stroke="{c_wire_red}" stroke-width="2.5"/>')
    
    # Ground wire from Battery & Regulator to Common Bus (route down via x=470)
    gnd_bus_x = 1440
    svg.append(f'<path d="M {pwr_x+pwr_w} {b_y+80} L 470 {b_y+80} L 470 940 L {gnd_bus_x} 940" fill="none" stroke="{c_wire_black}" stroke-width="3" stroke-linecap="round"/>')
    svg.append(f'<path d="M {pwr_x+pwr_w} {reg_y+70} L 470 {reg_y+70}" fill="none" stroke="{c_wire_black}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="470" cy="{reg_y+70}" r="4" fill="{c_wire_black}"/>')
    
    # Connect ESP32 GND.2 to ground rail
    esp_gnd_x, esp_gnd_y = right_pin_coords["GND.2"]
    svg.append(f'<path d="M {esp_gnd_x} {esp_gnd_y} L 860 {esp_gnd_y} L 860 940" fill="none" stroke="{c_wire_black}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="860" cy="940" r="4.5" fill="{c_wire_black}"/>')
    
    # Connect each Actuator GND to ground rail (x=1440)
    for tag, name, desc, pin_num, color, ay in actuators:
        g_y = ay + 80
        svg.append(f'<path d="M {act_x+act_w+20} {g_y} L {gnd_bus_x} {g_y}" fill="none" stroke="{c_wire_black}" stroke-width="2"/>')
        svg.append(f'<circle cx="{gnd_bus_x}" cy="{g_y}" r="4" fill="{c_wire_black}"/>')
    
    svg.append(f'<line x1="{gnd_bus_x}" y1="180" x2="{gnd_bus_x}" y2="940" stroke="{c_wire_black}" stroke-width="3"/>')
    svg.append(f'<text x="{gnd_bus_x}" y="110" fill="{c_wire_black}" font-size="11" font-weight="bold" text-anchor="middle">BUS TIERRA COMÚN (GND)</text>')
    
    # 5V VCC Power Rail (route via x=510, then under ESP32 to x=1080)
    vcc_bus_x = 1080
    svg.append(f'<line x1="{vcc_bus_x}" y1="190" x2="{vcc_bus_x}" y2="870" stroke="{c_wire_red}" stroke-width="3"/>')
    svg.append(f'<text x="{vcc_bus_x}" y="110" fill="{c_wire_red}" font-size="11" font-weight="bold" text-anchor="middle">BUS +5V ALIMENTACIÓN</text>')
    
    for tag, name, desc, pin_num, color, ay in actuators:
        v_y = ay + 80
        svg.append(f'<path d="M {vcc_bus_x} {v_y} L {act_x-20} {v_y}" fill="none" stroke="{c_wire_red}" stroke-width="2"/>')
        svg.append(f'<circle cx="{vcc_bus_x}" cy="{v_y}" r="4" fill="{c_wire_red}"/>')
        
    # Connect Regulator Output to 5V bus & ESP32 VIN (route via x=500 down to y=920, then over to x=1080)
    svg.append(f'<path d="M {pwr_x+pwr_w} {reg_y+100} L 500 {reg_y+100} L 500 920 L {vcc_bus_x} 920 L {vcc_bus_x} 870" fill="none" stroke="{c_wire_red}" stroke-width="3"/>')
    
    # Tap to ESP32 VIN (Pin 5V / VIN)
    esp_vin_x, esp_vin_y = right_pin_coords["5V / VIN"]
    svg.append(f'<path d="M {u1_x+u1_w+30} {esp_vin_y} L 960 {esp_vin_y} L 960 920" fill="none" stroke="{c_wire_red}" stroke-width="2"/>')
    svg.append(f'<circle cx="960" cy="920" r="4.5" fill="{c_wire_red}"/>')

    svg.append('</svg>')
    
    content = "\n".join(svg)
    with open("docs/schematic_circuit_diagram.svg", "w", encoding="utf-8") as f:
        f.write(content)
    with open("schematic_circuit_diagram.svg", "w", encoding="utf-8") as f:
        f.write(content)
    print("Refined SVG generated successfully.")

if __name__ == "__main__":
    create_schematic_svg()
