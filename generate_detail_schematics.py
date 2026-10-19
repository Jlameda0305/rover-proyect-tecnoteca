#!/usr/bin/env python3
"""
generate_detail_schematics.py
Generates two high-precision, engineering-style SVG schematics:
1. docs/schematic_motors_traction.svg (6 DC motors connected to 2 ESCs)
2. docs/schematic_power_supply.svg (Battery, UBEC, Power rails, Ground bus)
Matches the exact style of the reference engineering diagram (red borders, coordinates, title blocks).
"""

import os

def create_motors_schematic():
    width = 1600
    height = 1050
    c_frame = "#C0392B"
    c_wire_pwm1 = "#2980B9"     # Blue for ESC L
    c_wire_pwm2 = "#D35400"     # Orange for ESC R
    c_motor_pos = "#E74C3C"     # Red for Motor +
    c_motor_neg = "#2C3E50"     # Dark Slate for Motor -
    c_wire_red  = "#E74C3C"     # 7.4V
    c_wire_black = "#111111"    # GND

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}">')
    svg.append('<defs>')
    svg.append('  <pattern id="grid_mot" width="20" height="20" patternUnits="userSpaceOnUse">')
    svg.append('    <path d="M 20 0 L 0 0 0 20" fill="none" stroke="#F0F4F8" stroke-width="0.8"/>')
    svg.append('  </pattern>')
    svg.append('  <filter id="shadow1" x="-4%" y="-4%" width="108%" height="108%">')
    svg.append('    <feDropShadow dx="2" dy="2" stdDeviation="3" flood-color="#000000" flood-opacity="0.12"/>')
    svg.append('  </filter>')
    svg.append('</defs>')
    
    # Background
    svg.append(f'<rect width="{width}" height="{height}" fill="#FFFFFF"/>')
    svg.append(f'<rect width="{width}" height="{height}" fill="url(#grid_mot)"/>')

    # Engineering Red Outer & Inner Frames
    svg.append(f'<rect x="35" y="35" width="{width-70}" height="{height-70}" fill="none" stroke="{c_frame}" stroke-width="2.5"/>')
    svg.append(f'<rect x="55" y="55" width="{width-110}" height="{height-110}" fill="none" stroke="{c_frame}" stroke-width="1.2"/>')

    # Grid labels (Columns 1-6, Rows A-E)
    col_w = (width - 110) / 6
    for i in range(1, 6):
        cx = 55 + i * col_w
        svg.append(f'<line x1="{cx}" y1="35" x2="{cx}" y2="55" stroke="{c_frame}" stroke-width="1"/>')
        svg.append(f'<line x1="{cx}" y1="{height-55}" x2="{cx}" y2="{height-35}" stroke="{c_frame}" stroke-width="1"/>')
    for i in range(6):
        cx = 55 + i * col_w + col_w / 2
        svg.append(f'<text x="{cx}" y="49" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{i+1}</text>')
        svg.append(f'<text x="{cx}" y="{height-39}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{i+1}</text>')

    row_h = (height - 110) / 5
    row_labels = ["A", "B", "C", "D", "E"]
    for j in range(1, 5):
        ry = 55 + j * row_h
        svg.append(f'<line x1="35" y1="{ry}" x2="55" y2="{ry}" stroke="{c_frame}" stroke-width="1"/>')
        svg.append(f'<line x1="{width-55}" y1="{ry}" x2="{width-35}" y2="{ry}" stroke="{c_frame}" stroke-width="1"/>')
    for j in range(5):
        ry = 55 + j * row_h + row_h / 2 + 5
        svg.append(f'<text x="45" y="{ry}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{row_labels[j]}</text>')
        svg.append(f'<text x="{width-45}" y="{ry}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{row_labels[j]}</text>')

    svg.append(f'<text x="70" y="{height-68}" fill="#7F8C8D" font-size="13" font-weight="bold">Mars Rover Project &#8226; 6-Wheel Traction Subsystem (Motors &amp; ESCs)</text>')
    
    # Title Block (Bottom Right)
    tb_x = width - 55 - 470
    tb_y = height - 55 - 80
    tb_w = 470
    tb_h = 80
    svg.append(f'<rect x="{tb_x}" y="{tb_y}" width="{tb_w}" height="{tb_h}" fill="#FFFFFF" stroke="{c_frame}" stroke-width="1.5"/>')
    svg.append(f'<line x1="{tb_x}" y1="{tb_y+30}" x2="{tb_x+tb_w}" y2="{tb_y+30}" stroke="{c_frame}" stroke-width="1"/>')
    svg.append(f'<line x1="{tb_x}" y1="{tb_y+55}" x2="{tb_x+tb_w}" y2="{tb_y+55}" stroke="{c_frame}" stroke-width="1"/>')
    svg.append(f'<line x1="{tb_x+320}" y1="{tb_y+30}" x2="{tb_x+320}" y2="{tb_y+tb_h}" stroke="{c_frame}" stroke-width="1"/>')
    
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+20}" fill="{c_frame}" font-size="13" font-weight="bold">Title: <tspan fill="#2C3E50" font-weight="normal">Mars Rover - Tracción 6x6 (ESCs y Motores DC)</tspan></text>')
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+46}" fill="{c_frame}" font-size="12" font-weight="bold">Date: <tspan fill="#34495E" font-weight="normal">15/10/2026</tspan></text>')
    svg.append(f'<text x="{tb_x+330}" y="{tb_y+46}" fill="{c_frame}" font-size="12" font-weight="bold">Sheet: <tspan fill="#34495E" font-weight="normal">1/1</tspan></text>')
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+71}" fill="{c_frame}" font-size="12" font-weight="bold">System: <tspan fill="#34495E" font-weight="normal">2x ESC Bidireccional + 6x Motores DC</tspan></text>')
    svg.append(f'<text x="{tb_x+330}" y="{tb_y+71}" fill="{c_frame}" font-size="12" font-weight="bold">Rev: <tspan fill="#34495E" font-weight="normal">1.0</tspan></text>')

    # ----------------------------------------------------
    # CONTROLLER BLOCK (ESP32 Signal Source)
    # ----------------------------------------------------
    esp_x = 85
    esp_y = 280
    esp_w = 240
    esp_h = 240
    svg.append(f'<rect x="{esp_x}" y="{esp_y}" width="{esp_w}" height="{esp_h}" rx="6" fill="#FFFFFF" stroke="#2C3E50" stroke-width="2" filter="url(#shadow1)"/>')
    svg.append(f'<rect x="{esp_x}" y="{esp_y}" width="{esp_w}" height="32" rx="4" fill="#2C3E50"/>')
    svg.append(f'<text x="{esp_x+esp_w/2}" y="{esp_y+21}" fill="#FFFFFF" font-size="13" font-weight="bold" text-anchor="middle">U1 : ESP32 (CONTROLADOR)</text>')
    
    # Pins on ESP32
    # GPIO 17 (ESC Left)
    svg.append(f'<line x1="{esp_x+esp_w}" y1="{esp_y+70}" x2="{esp_x+esp_w+20}" y2="{esp_y+70}" stroke="{c_wire_pwm1}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="{esp_x+esp_w+20}" cy="{esp_y+70}" r="4" fill="{c_wire_pwm1}"/>')
    svg.append(f'<text x="{esp_x+esp_w-10}" y="{esp_y+74}" fill="{c_wire_pwm1}" font-size="12" font-weight="bold" text-anchor="end">GPIO 17 (PWM ESC L)</text>')
    
    # GPIO 13 (ESC Right)
    svg.append(f'<line x1="{esp_x+esp_w}" y1="{esp_y+135}" x2="{esp_x+esp_w+20}" y2="{esp_y+135}" stroke="{c_wire_pwm2}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="{esp_x+esp_w+20}" cy="{esp_y+135}" r="4" fill="{c_wire_pwm2}"/>')
    svg.append(f'<text x="{esp_x+esp_w-10}" y="{esp_y+139}" fill="{c_wire_pwm2}" font-size="12" font-weight="bold" text-anchor="end">GPIO 13 (PWM ESC R)</text>')
    
    # GND
    svg.append(f'<line x1="{esp_x+esp_w}" y1="{esp_y+195}" x2="{esp_x+esp_w+20}" y2="{esp_y+195}" stroke="{c_wire_black}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="{esp_x+esp_w+20}" cy="{esp_y+195}" r="4" fill="{c_wire_black}"/>')
    svg.append(f'<text x="{esp_x+esp_w-10}" y="{esp_y+199}" fill="{c_wire_black}" font-size="12" font-weight="bold" text-anchor="end">GND (Masa Común)</text>')

    # ----------------------------------------------------
    # TOP HALF: LEFT TANK / BANK (ESC 1 + 3 MOTORS)
    # ----------------------------------------------------
    esc1_x = 420
    esc1_y = 120
    esc_w = 260
    esc_h = 230
    
    svg.append(f'<rect x="{esc1_x}" y="{esc1_y}" width="{esc_w}" height="{esc_h}" rx="8" fill="#FFFFFF" stroke="{c_wire_pwm1}" stroke-width="2.2" filter="url(#shadow1)"/>')
    svg.append(f'<rect x="{esc1_x}" y="{esc1_y}" width="{esc_w}" height="32" rx="4" fill="{c_wire_pwm1}"/>')
    svg.append(f'<text x="{esc1_x+esc_w/2}" y="{esc1_y+22}" fill="#FFFFFF" font-size="13" font-weight="bold" text-anchor="middle">ESC 1 : TRACCIÓN IZQUIERDA</text>')
    
    # Inputs on ESC1
    svg.append(f'<text x="{esc1_x+12}" y="{esc1_y+65}" fill="{c_wire_pwm1}" font-size="12" font-weight="bold">&#8226; PWM SIGNAL IN (50 Hz)</text>')
    svg.append(f'<text x="{esc1_x+12}" y="{esc1_y+105}" fill="{c_wire_red}" font-size="12" font-weight="bold">&#8226; VCC BATERÍA (+7.4V)</text>')
    svg.append(f'<text x="{esc1_x+12}" y="{esc1_y+145}" fill="{c_wire_black}" font-size="12" font-weight="bold">&#8226; GND BATERÍA (-)</text>')
    
    # Outputs on ESC1
    svg.append(f'<text x="{esc1_x+esc_w-12}" y="{esc1_y+85}" fill="{c_motor_pos}" font-size="13" font-weight="bold" text-anchor="end">MOTOR L (+) &#8594;</text>')
    svg.append(f'<text x="{esc1_x+esc_w-12}" y="{esc1_y+175}" fill="{c_motor_neg}" font-size="13" font-weight="bold" text-anchor="end">MOTOR L (-) &#8594;</text>')
    
    # Connect ESP32 GPIO 17 to ESC1 PWM
    svg.append(f'<path d="M {esp_x+esp_w+20} {esp_y+70} L 370 {esp_y+70} L 370 {esc1_y+60} L {esc1_x} {esc1_y+60}" fill="none" stroke="{c_wire_pwm1}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="{esc1_x}" cy="{esc1_y+60}" r="4" fill="{c_wire_pwm1}"/>')
    
    # 3 Motors Left Side (M_FL, M_ML, M_BL)
    m_left = [
        ("M1", "MOTOR DELANTERO IZQ (FL)", 95),
        ("M2", "MOTOR MEDIO IZQ (ML)", 210),
        ("M3", "MOTOR TRASERO IZQ (BL)", 325)
    ]
    
    mot_x = 1000
    mot_w = 270
    mot_h = 90
    
    # Bus wires for Left Motors
    bus_l_pos_x = 840
    bus_l_neg_x = 900
    
    svg.append(f'<line x1="{esc1_x+esc_w}" y1="{esc1_y+80}" x2="{bus_l_pos_x}" y2="{esc1_y+80}" stroke="{c_motor_pos}" stroke-width="3"/>')
    svg.append(f'<line x1="{bus_l_pos_x}" y1="120" x2="{bus_l_pos_x}" y2="380" stroke="{c_motor_pos}" stroke-width="3.5"/>')
    svg.append(f'<text x="{bus_l_pos_x-10}" y="112" fill="{c_motor_pos}" font-size="11" font-weight="bold" text-anchor="end">RAMAL (+) IZQUIERDO</text>')
    
    svg.append(f'<line x1="{esc1_x+esc_w}" y1="{esc1_y+170}" x2="{bus_l_neg_x}" y2="{esc1_y+170}" stroke="{c_motor_neg}" stroke-width="3"/>')
    svg.append(f'<line x1="{bus_l_neg_x}" y1="140" x2="{bus_l_neg_x}" y2="400" stroke="{c_motor_neg}" stroke-width="3.5"/>')
    svg.append(f'<text x="{bus_l_neg_x+10}" y="112" fill="{c_motor_neg}" font-size="11" font-weight="bold">RAMAL (-) IZQUIERDO</text>')
    
    for tag, mname, my in m_left:
        svg.append(f'<rect x="{mot_x}" y="{my}" width="{mot_w}" height="{mot_h}" rx="8" fill="#FFFFFF" stroke="#2980B9" stroke-width="2" filter="url(#shadow1)"/>')
        svg.append(f'<rect x="{mot_x}" y="{my}" width="{mot_w}" height="26" rx="4" fill="#2980B9"/>')
        svg.append(f'<text x="{mot_x+10}" y="{my+18}" fill="#FFFFFF" font-size="12" font-weight="bold">{tag} : {mname}</text>')
        # Circle
        svg.append(f'<circle cx="{mot_x+mot_w-32}" cy="{my+55}" r="20" fill="#EBF5FB" stroke="#2980B9" stroke-width="2"/>')
        svg.append(f'<text x="{mot_x+mot_w-32}" y="{my+61}" fill="#2980B9" font-size="14" font-weight="bold" text-anchor="middle">M</text>')
        # Terminals
        svg.append(f'<text x="{mot_x+12}" y="{my+48}" fill="{c_motor_pos}" font-size="12" font-weight="bold">+ Terminal Positivo</text>')
        svg.append(f'<text x="{mot_x+12}" y="{my+74}" fill="{c_motor_neg}" font-size="12" font-weight="bold">- Terminal Negativo</text>')
        
        pos_y = my + 44
        neg_y = my + 70
        svg.append(f'<line x1="{bus_l_pos_x}" y1="{pos_y}" x2="{mot_x}" y2="{pos_y}" stroke="{c_motor_pos}" stroke-width="2.5"/>')
        svg.append(f'<circle cx="{bus_l_pos_x}" cy="{pos_y}" r="4" fill="{c_motor_pos}"/>')
        svg.append(f'<line x1="{bus_l_neg_x}" y1="{neg_y}" x2="{mot_x}" y2="{neg_y}" stroke="{c_motor_neg}" stroke-width="2.5"/>')
        svg.append(f'<circle cx="{bus_l_neg_x}" cy="{neg_y}" r="4" fill="{c_motor_neg}"/>')

    # ----------------------------------------------------
    # BOTTOM HALF: RIGHT TANK / BANK (ESC 2 + 3 MOTORS)
    # ----------------------------------------------------
    esc2_x = 420
    esc2_y = 450
    
    svg.append(f'<rect x="{esc2_x}" y="{esc2_y}" width="{esc_w}" height="{esc_h}" rx="8" fill="#FFFFFF" stroke="{c_wire_pwm2}" stroke-width="2.2" filter="url(#shadow1)"/>')
    svg.append(f'<rect x="{esc2_x}" y="{esc2_y}" width="{esc_w}" height="32" rx="4" fill="{c_wire_pwm2}"/>')
    svg.append(f'<text x="{esc2_x+esc_w/2}" y="{esc2_y+22}" fill="#FFFFFF" font-size="13" font-weight="bold" text-anchor="middle">ESC 2 : TRACCIÓN DERECHA</text>')
    
    svg.append(f'<text x="{esc2_x+12}" y="{esc2_y+65}" fill="{c_wire_pwm2}" font-size="12" font-weight="bold">&#8226; PWM SIGNAL IN (50 Hz)</text>')
    svg.append(f'<text x="{esc2_x+12}" y="{esc2_y+105}" fill="{c_wire_red}" font-size="12" font-weight="bold">&#8226; VCC BATERÍA (+7.4V)</text>')
    svg.append(f'<text x="{esc2_x+12}" y="{esc2_y+145}" fill="{c_wire_black}" font-size="12" font-weight="bold">&#8226; GND BATERÍA (-)</text>')
    
    svg.append(f'<text x="{esc2_x+esc_w-12}" y="{esc2_y+85}" fill="{c_motor_pos}" font-size="13" font-weight="bold" text-anchor="end">MOTOR R (+) &#8594;</text>')
    svg.append(f'<text x="{esc2_x+esc_w-12}" y="{esc2_y+175}" fill="{c_motor_neg}" font-size="13" font-weight="bold" text-anchor="end">MOTOR R (-) &#8594;</text>')
    
    # Connect ESP32 GPIO 13 to ESC2 PWM
    svg.append(f'<path d="M {esp_x+esp_w+20} {esp_y+135} L 370 {esp_y+135} L 370 {esc2_y+60} L {esc2_x} {esc2_y+60}" fill="none" stroke="{c_wire_pwm2}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="{esc2_x}" cy="{esc2_y+60}" r="4" fill="{c_wire_pwm2}"/>')
    
    # 3 Motors Right Side (M_FR, M_MR, M_BR)
    m_right = [
        ("M4", "MOTOR DELANTERO DER (FR)", 445),
        ("M5", "MOTOR MEDIO DER (MR)", 560),
        ("M6", "MOTOR TRASERO DER (BR)", 675)
    ]
    
    bus_r_pos_x = 840
    bus_r_neg_x = 900
    
    svg.append(f'<line x1="{esc2_x+esc_w}" y1="{esc2_y+80}" x2="{bus_r_pos_x}" y2="{esc2_y+80}" stroke="{c_motor_pos}" stroke-width="3"/>')
    svg.append(f'<line x1="{bus_r_pos_x}" y1="470" x2="{bus_r_pos_x}" y2="730" stroke="{c_motor_pos}" stroke-width="3.5"/>')
    svg.append(f'<text x="{bus_r_pos_x-10}" y="462" fill="{c_motor_pos}" font-size="11" font-weight="bold" text-anchor="end">RAMAL (+) DERECHO</text>')
    
    svg.append(f'<line x1="{esc2_x+esc_w}" y1="{esc2_y+170}" x2="{bus_r_neg_x}" y2="{esc2_y+170}" stroke="{c_motor_neg}" stroke-width="3"/>')
    svg.append(f'<line x1="{bus_r_neg_x}" y1="490" x2="{bus_r_neg_x}" y2="750" stroke="{c_motor_neg}" stroke-width="3.5"/>')
    svg.append(f'<text x="{bus_r_neg_x+10}" y="462" fill="{c_motor_neg}" font-size="11" font-weight="bold">RAMAL (-) DERECHO</text>')
    
    for tag, mname, my in m_right:
        svg.append(f'<rect x="{mot_x}" y="{my}" width="{mot_w}" height="{mot_h}" rx="8" fill="#FFFFFF" stroke="#D35400" stroke-width="2" filter="url(#shadow1)"/>')
        svg.append(f'<rect x="{mot_x}" y="{my}" width="{mot_w}" height="26" rx="4" fill="#D35400"/>')
        svg.append(f'<text x="{mot_x+10}" y="{my+18}" fill="#FFFFFF" font-size="12" font-weight="bold">{tag} : {mname}</text>')
        # Circle
        svg.append(f'<circle cx="{mot_x+mot_w-32}" cy="{my+55}" r="20" fill="#FBEEE6" stroke="#D35400" stroke-width="2"/>')
        svg.append(f'<text x="{mot_x+mot_w-32}" y="{my+61}" fill="#D35400" font-size="14" font-weight="bold" text-anchor="middle">M</text>')
        # Terminals
        svg.append(f'<text x="{mot_x+12}" y="{my+48}" fill="{c_motor_pos}" font-size="12" font-weight="bold">+ Terminal Positivo</text>')
        svg.append(f'<text x="{mot_x+12}" y="{my+74}" fill="{c_motor_neg}" font-size="12" font-weight="bold">- Terminal Negativo</text>')
        
        pos_y = my + 44
        neg_y = my + 70
        svg.append(f'<line x1="{bus_r_pos_x}" y1="{pos_y}" x2="{mot_x}" y2="{pos_y}" stroke="{c_motor_pos}" stroke-width="2.5"/>')
        svg.append(f'<circle cx="{bus_r_pos_x}" cy="{pos_y}" r="4" fill="{c_motor_pos}"/>')
        svg.append(f'<line x1="{bus_r_neg_x}" y1="{neg_y}" x2="{mot_x}" y2="{neg_y}" stroke="{c_motor_neg}" stroke-width="2.5"/>')
        svg.append(f'<circle cx="{bus_r_neg_x}" cy="{neg_y}" r="4" fill="{c_motor_neg}"/>')

    # Connect Common Ground from ESP32 to both ESCs
    svg.append(f'<path d="M {esp_x+esp_w+20} {esp_y+195} L 350 {esp_y+195} L 350 {esc1_y+140} L {esc1_x} {esc1_y+140}" fill="none" stroke="{c_wire_black}" stroke-width="2"/>')
    svg.append(f'<circle cx="{esc1_x}" cy="{esc1_y+140}" r="3.5" fill="{c_wire_black}"/>')
    svg.append(f'<path d="M 350 {esp_y+195} L 350 {esc2_y+140} L {esc2_x} {esc2_y+140}" fill="none" stroke="{c_wire_black}" stroke-width="2"/>')
    svg.append(f'<circle cx="{esc2_x}" cy="{esc2_y+140}" r="3.5" fill="{c_wire_black}"/>')
    svg.append(f'<circle cx="350" cy="{esp_y+195}" r="3.5" fill="{c_wire_black}"/>')

    # ----------------------------------------------------
    # NOTES / CALLOUT BOX (Bottom Wide Banner: x=85 to x=980)
    # ----------------------------------------------------
    leg_x = 85
    leg_y = 770
    leg_w = 880
    leg_h = 175
    svg.append(f'<rect x="{leg_x}" y="{leg_y}" width="{leg_w}" height="{leg_h}" rx="6" fill="#FFFFFF" stroke="#BDC3C7" stroke-width="1.5" filter="url(#shadow1)"/>')
    svg.append(f'<rect x="{leg_x}" y="{leg_y}" width="{leg_w}" height="28" rx="4" fill="#34495E"/>')
    svg.append(f'<text x="{leg_x+16}" y="{leg_y+19}" fill="#FFFFFF" font-size="12" font-weight="bold">NOTAS CRÍTICAS DE CONEXIÓN DE MOTORES Y DIRECCIONALIDAD</text>')
    
    # Left column notes
    col1_x = leg_x + 16
    svg.append(f'<text x="{col1_x}" y="{leg_y+55}" fill="#C0392B" font-size="11" font-weight="bold">&#8226; Conexión en Paralelo:<tspan fill="#2C3E50" font-weight="normal"> Los 3 motores de cada lado comparten la misma salida del ESC (M+ y M-).</tspan></text>')
    svg.append(f'<text x="{col1_x}" y="{leg_y+85}" fill="#C0392B" font-size="11" font-weight="bold">&#8226; Regla de Inversión de Giro:<tspan fill="#2C3E50" font-weight="normal"> Si alguna rueda gira al revés, invierte sus 2 cables únicamente en ese motor.</tspan></text>')
    svg.append(f'<text x="{col1_x}" y="{leg_y+115}" fill="#C0392B" font-size="11" font-weight="bold">&#8226; Corriente de Tracción:<tspan fill="#2C3E50" font-weight="normal"> Cada ESC soporta ~15-20A pico. El cableado a motores debe ser mín. 18-20 AWG.</tspan></text>')
    svg.append(f'<text x="{col1_x}" y="{leg_y+145}" fill="#C0392B" font-size="11" font-weight="bold">&#8226; Giro 360° (Modo Spin):<tspan fill="#2C3E50" font-weight="normal"> Los ESCs giran en sentidos opuestos mientras los 4 servos forman una circunferencia.</tspan></text>')

    svg.append('</svg>')
    
    with open("docs/schematic_motors_traction.svg", "w", encoding="utf-8") as f:
        f.write("\n".join(svg))
    print("Motors schematic SVG created.")


def create_power_schematic():
    width = 1600
    height = 1050
    c_frame = "#C0392B"
    c_wire_red  = "#E74C3C"     # 7.4V Battery Power
    c_wire_5v   = "#F39C12"     # 5.0V Regulated Logic/Servos
    c_wire_black = "#111111"    # GND Star Bus

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}">')
    svg.append('<defs>')
    svg.append('  <pattern id="grid_pwr" width="20" height="20" patternUnits="userSpaceOnUse">')
    svg.append('    <path d="M 20 0 L 0 0 0 20" fill="none" stroke="#F0F4F8" stroke-width="0.8"/>')
    svg.append('  </pattern>')
    svg.append('  <filter id="shadow2" x="-4%" y="-4%" width="108%" height="108%">')
    svg.append('    <feDropShadow dx="2" dy="2" stdDeviation="3" flood-color="#000000" flood-opacity="0.12"/>')
    svg.append('  </filter>')
    svg.append('</defs>')
    
    # Background
    svg.append(f'<rect width="{width}" height="{height}" fill="#FFFFFF"/>')
    svg.append(f'<rect width="{width}" height="{height}" fill="url(#grid_pwr)"/>')

    # Frames
    svg.append(f'<rect x="35" y="35" width="{width-70}" height="{height-70}" fill="none" stroke="{c_frame}" stroke-width="2.5"/>')
    svg.append(f'<rect x="55" y="55" width="{width-110}" height="{height-110}" fill="none" stroke="{c_frame}" stroke-width="1.2"/>')

    # Grid labels (Columns 1-6, Rows A-E)
    col_w = (width - 110) / 6
    for i in range(1, 6):
        cx = 55 + i * col_w
        svg.append(f'<line x1="{cx}" y1="35" x2="{cx}" y2="55" stroke="{c_frame}" stroke-width="1"/>')
        svg.append(f'<line x1="{cx}" y1="{height-55}" x2="{cx}" y2="{height-35}" stroke="{c_frame}" stroke-width="1"/>')
    for i in range(6):
        cx = 55 + i * col_w + col_w / 2
        svg.append(f'<text x="{cx}" y="49" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{i+1}</text>')
        svg.append(f'<text x="{cx}" y="{height-39}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{i+1}</text>')

    row_h = (height - 110) / 5
    row_labels = ["A", "B", "C", "D", "E"]
    for j in range(1, 5):
        ry = 55 + j * row_h
        svg.append(f'<line x1="35" y1="{ry}" x2="55" y2="{ry}" stroke="{c_frame}" stroke-width="1"/>')
        svg.append(f'<line x1="{width-55}" y1="{ry}" x2="{width-35}" y2="{ry}" stroke="{c_frame}" stroke-width="1"/>')
    for j in range(5):
        ry = 55 + j * row_h + row_h / 2 + 5
        svg.append(f'<text x="45" y="{ry}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{row_labels[j]}</text>')
        svg.append(f'<text x="{width-45}" y="{ry}" fill="{c_frame}" font-size="13" font-weight="bold" text-anchor="middle">{row_labels[j]}</text>')

    svg.append(f'<text x="70" y="{height-68}" fill="#7F8C8D" font-size="13" font-weight="bold">Mars Rover Project &#8226; Power Distribution Architecture (Battery, UBEC &amp; Regulators)</text>')

    # Title Block
    tb_x = width - 55 - 470
    tb_y = height - 55 - 80
    tb_w = 470
    tb_h = 80
    svg.append(f'<rect x="{tb_x}" y="{tb_y}" width="{tb_w}" height="{tb_h}" fill="#FFFFFF" stroke="{c_frame}" stroke-width="1.5"/>')
    svg.append(f'<line x1="{tb_x}" y1="{tb_y+30}" x2="{tb_x+tb_w}" y2="{tb_y+30}" stroke="{c_frame}" stroke-width="1"/>')
    svg.append(f'<line x1="{tb_x}" y1="{tb_y+55}" x2="{tb_x+tb_w}" y2="{tb_y+55}" stroke="{c_frame}" stroke-width="1"/>')
    svg.append(f'<line x1="{tb_x+320}" y1="{tb_y+30}" x2="{tb_x+320}" y2="{tb_y+tb_h}" stroke="{c_frame}" stroke-width="1"/>')
    
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+20}" fill="{c_frame}" font-size="13" font-weight="bold">Title: <tspan fill="#2C3E50" font-weight="normal">Mars Rover - Esquema de Alimentación y Distribución</tspan></text>')
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+46}" fill="{c_frame}" font-size="12" font-weight="bold">Date: <tspan fill="#34495E" font-weight="normal">15/10/2026</tspan></text>')
    svg.append(f'<text x="{tb_x+330}" y="{tb_y+46}" fill="{c_frame}" font-size="12" font-weight="bold">Sheet: <tspan fill="#34495E" font-weight="normal">1/1</tspan></text>')
    svg.append(f'<text x="{tb_x+12}" y="{tb_y+71}" fill="{c_frame}" font-size="12" font-weight="bold">Topology: <tspan fill="#34495E" font-weight="normal">Doble Riel (7.4V Potencia / 5V Lógica &amp; Servos)</tspan></text>')
    svg.append(f'<text x="{tb_x+330}" y="{tb_y+71}" fill="{c_frame}" font-size="12" font-weight="bold">Rev: <tspan fill="#34495E" font-weight="normal">1.0</tspan></text>')

    # ----------------------------------------------------
    # POWER GENERATION BLOCKS (Left Column: x=85 to x=335)
    # ----------------------------------------------------
    # B1: Battery (LiPo 2S 7.4V)
    bat_x = 85
    bat_y = 100
    bat_w = 250
    bat_h = 115
    svg.append(f'<rect x="{bat_x}" y="{bat_y}" width="{bat_w}" height="{bat_h}" rx="8" fill="#FFFFFF" stroke="#E67E22" stroke-width="2.2" filter="url(#shadow2)"/>')
    svg.append(f'<rect x="{bat_x}" y="{bat_y}" width="{bat_w}" height="28" rx="4" fill="#E67E22"/>')
    svg.append(f'<text x="{bat_x+bat_w/2}" y="{bat_y+19}" fill="#FFFFFF" font-size="13" font-weight="bold" text-anchor="middle">B1 : BATERÍA PRINCIPAL (LiPo 2S)</text>')
    svg.append(f'<text x="{bat_x+14}" y="{bat_y+52}" fill="{c_wire_red}" font-size="12" font-weight="bold">+ POSITIVO (+7.4V / 8.4V MÁX)</text>')
    svg.append(f'<text x="{bat_x+14}" y="{bat_y+80}" fill="{c_wire_black}" font-size="12" font-weight="bold">- NEGATIVO (GND MASA COMÚN)</text>')
    svg.append(f'<text x="{bat_x+14}" y="{bat_y+102}" fill="#7F8C8D" font-size="11">Capacidad: 2200 - 5000 mAh (25C+)</text>')

    # FUSE1: Fuse (20A)
    fuse_x = bat_x + 75
    fuse_y = bat_y + 140
    fuse_w = 100
    fuse_h = 36
    svg.append(f'<rect x="{fuse_x}" y="{fuse_y}" width="{fuse_w}" height="{fuse_h}" rx="4" fill="#FFFFFF" stroke="#E74C3C" stroke-width="2"/>')
    svg.append(f'<text x="{fuse_x+fuse_w/2}" y="{fuse_y+23}" fill="#E74C3C" font-size="12" font-weight="bold" text-anchor="middle">FUSIBLE : 20A</text>')

    # SW1: Switch SPST
    sw_x = bat_x + 75
    sw_y = fuse_y + 60
    sw_w = 100
    sw_h = 36
    svg.append(f'<rect x="{sw_x}" y="{sw_y}" width="{sw_w}" height="{sw_h}" rx="4" fill="#FFFFFF" stroke="#34495E" stroke-width="2"/>')
    svg.append(f'<text x="{sw_x+sw_w/2}" y="{sw_y+23}" fill="#2C3E50" font-size="12" font-weight="bold" text-anchor="middle">SW1 : POWER</text>')

    # Wire Battery(+) -> Fuse -> Switch
    svg.append(f'<line x1="{bat_x+60}" y1="{bat_y+55}" x2="{fuse_x+fuse_w/2}" y2="{bat_y+55}" stroke="{c_wire_red}" stroke-width="3"/>')
    svg.append(f'<line x1="{fuse_x+fuse_w/2}" y1="{bat_y+55}" x2="{fuse_x+fuse_w/2}" y2="{fuse_y}" stroke="{c_wire_red}" stroke-width="3"/>')
    svg.append(f'<line x1="{fuse_x+fuse_w/2}" y1="{fuse_y+fuse_h}" x2="{sw_x+sw_w/2}" y2="{sw_y}" stroke="{c_wire_red}" stroke-width="3"/>')

    # U2: UBEC 5V 5A (Step Down DC-DC)
    ubec_x = 85
    ubec_y = sw_y + 80
    ubec_w = 250
    ubec_h = 135
    svg.append(f'<rect x="{ubec_x}" y="{ubec_y}" width="{ubec_w}" height="{ubec_h}" rx="8" fill="#FFFFFF" stroke="#27AE60" stroke-width="2.2" filter="url(#shadow2)"/>')
    svg.append(f'<rect x="{ubec_x}" y="{ubec_y}" width="{ubec_w}" height="28" rx="4" fill="#27AE60"/>')
    svg.append(f'<text x="{ubec_x+ubec_w/2}" y="{ubec_y+19}" fill="#FFFFFF" font-size="13" font-weight="bold" text-anchor="middle">U2 : UBEC / STEP-DOWN DC-DC</text>')
    svg.append(f'<text x="{ubec_x+14}" y="{ubec_y+55}" fill="{c_wire_red}" font-size="12" font-weight="bold">VIN (+) : 7.4V Entrada</text>')
    svg.append(f'<text x="{ubec_x+14}" y="{ubec_y+85}" fill="{c_wire_black}" font-size="12" font-weight="bold">GND (-) : Tierra Común</text>')
    svg.append(f'<text x="{ubec_x+14}" y="{ubec_y+115}" fill="{c_wire_5v}" font-size="12" font-weight="bold">VOUT (+) : 5.0V / 5A REGULADO</text>')

    # ----------------------------------------------------
    # ----------------------------------------------------
    # DISTRIBUTION BUSES & ROUTING (No crossing into boxes)
    # ----------------------------------------------------
    sw_out_x = sw_x + sw_w/2
    sw_out_y = sw_y + sw_h
    bus_7v4_x = 440
    bus_5v_x = 550
    bus_gnd_x = 1450
    
    # Switch output wire goes down to y=355, where it splits cleanly
    svg.append(f'<line x1="{sw_out_x}" y1="{sw_out_y}" x2="{sw_out_x}" y2="355" stroke="{c_wire_red}" stroke-width="3"/>')
    svg.append(f'<circle cx="{sw_out_x}" cy="355" r="4.5" fill="{c_wire_red}"/>')
    
    # Branch A to 7.4V High Power Rail
    svg.append(f'<line x1="{sw_out_x}" y1="355" x2="{bus_7v4_x}" y2="355" stroke="{c_wire_red}" stroke-width="3.5"/>')
    
    # Branch B to UBEC VIN: loops left outside the UBEC and enters cleanly from the left
    svg.append(f'<path d="M {sw_out_x} 355 L 60 355 L 60 {ubec_y+55} L {ubec_x} {ubec_y+55}" fill="none" stroke="{c_wire_red}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="{ubec_x}" cy="{ubec_y+55}" r="4" fill="{c_wire_red}"/>')
    
    # 7.4V Rail (Vertical)
    svg.append(f'<line x1="{bus_7v4_x}" y1="95" x2="{bus_7v4_x}" y2="355" stroke="{c_wire_red}" stroke-width="4"/>')
    svg.append(f'<text x="{bus_7v4_x}" y="82" fill="{c_wire_red}" font-size="12" font-weight="bold" text-anchor="middle">RIEL DE ALTA POTENCIA (+7.4V)</text>')

    # 5V Rail (Vertical)
    # UBEC VOUT exits right from UBEC to 5V rail at y = ubec_y + 115
    ubec_vout_y = ubec_y + 115
    svg.append(f'<line x1="{ubec_x+ubec_w}" y1="{ubec_vout_y}" x2="{bus_5v_x}" y2="{ubec_vout_y}" stroke="{c_wire_5v}" stroke-width="3.5"/>')
    svg.append(f'<circle cx="{bus_5v_x}" cy="{ubec_vout_y}" r="4.5" fill="{c_wire_5v}"/>')
    svg.append(f'<line x1="{bus_5v_x}" y1="440" x2="{bus_5v_x}" y2="800" stroke="{c_wire_5v}" stroke-width="4"/>')
    svg.append(f'<text x="{bus_5v_x}" y="422" fill="{c_wire_5v}" font-size="11.5" font-weight="bold" text-anchor="middle">RIEL REGULADO +5.0V (SERVOS Y ESP32)</text>')

    # GND Bus (Star topology on Right side)
    svg.append(f'<line x1="{bus_gnd_x}" y1="85" x2="{bus_gnd_x}" y2="880" stroke="{c_wire_black}" stroke-width="4.5"/>')
    svg.append(f'<text x="{bus_gnd_x}" y="72" fill="{c_wire_black}" font-size="12" font-weight="bold" text-anchor="middle">BUS CENTRAL DE MASA COMÚN (GND)</text>')

    # Battery GND line -> runs underneath to GND Bus at y=880
    svg.append(f'<path d="M {bat_x+bat_w} {bat_y+75} L 390 {bat_y+75} L 390 880 L {bus_gnd_x} 880" fill="none" stroke="{c_wire_black}" stroke-width="3.5"/>')
    svg.append(f'<circle cx="{bus_gnd_x}" cy="880" r="5" fill="{c_wire_black}"/>')

    # UBEC GND line to GND Bus
    svg.append(f'<path d="M {ubec_x+ubec_w} {ubec_y+85} L 390 {ubec_y+85}" fill="none" stroke="{c_wire_black}" stroke-width="2.5"/>')
    svg.append(f'<circle cx="390" cy="{ubec_y+85}" r="4" fill="{c_wire_black}"/>')

    # ----------------------------------------------------
    # CONSUMERS (ESCs, Servos, ESP32)
    # ----------------------------------------------------
    c_x = 670
    c_w = 540

    # Load 1: ESC Tracción Izquierda
    c1_y = 100
    svg.append(f'<rect x="{c_x}" y="{c1_y}" width="{c_w}" height="100" rx="8" fill="#FFFFFF" stroke="#2980B9" stroke-width="2" filter="url(#shadow2)"/>')
    svg.append(f'<text x="{c_x+16}" y="{c1_y+28}" fill="#2980B9" font-size="13" font-weight="bold">CARGA 1 : ESC TRACCIÓN IZQUIERDA (3 Motores DC en Paralelo)</text>')
    svg.append(f'<text x="{c_x+16}" y="{c1_y+58}" fill="{c_wire_red}" font-size="12">&#8226; Alimentación Potencia: +7.4V Directo de Batería (Hasta 20A pico)</text>')
    svg.append(f'<text x="{c_x+16}" y="{c1_y+82}" fill="{c_wire_black}" font-size="12">&#8226; Retorno de Corriente: GND Común hacia el Bus Central</text>')

    # Load 2: ESC Tracción Derecha
    c2_y = 240
    svg.append(f'<rect x="{c_x}" y="{c2_y}" width="{c_w}" height="100" rx="8" fill="#FFFFFF" stroke="#D35400" stroke-width="2" filter="url(#shadow2)"/>')
    svg.append(f'<text x="{c_x+16}" y="{c2_y+28}" fill="#D35400" font-size="13" font-weight="bold">CARGA 2 : ESC TRACCIÓN DERECHA (3 Motores DC en Paralelo)</text>')
    svg.append(f'<text x="{c_x+16}" y="{c2_y+58}" fill="{c_wire_red}" font-size="12">&#8226; Alimentación Potencia: +7.4V Directo de Batería (Hasta 20A pico)</text>')
    svg.append(f'<text x="{c_x+16}" y="{c2_y+82}" fill="{c_wire_black}" font-size="12">&#8226; Retorno de Corriente: GND Común hacia el Bus Central</text>')

    # Connect 7.4V Rail to ESC1 & ESC2
    svg.append(f'<line x1="{bus_7v4_x}" y1="{c1_y+54}" x2="{c_x}" y2="{c1_y+54}" stroke="{c_wire_red}" stroke-width="3"/>')
    svg.append(f'<circle cx="{bus_7v4_x}" cy="{c1_y+54}" r="4.5" fill="{c_wire_red}"/>')
    svg.append(f'<line x1="{bus_7v4_x}" y1="{c2_y+54}" x2="{c_x}" y2="{c2_y+54}" stroke="{c_wire_red}" stroke-width="3"/>')
    svg.append(f'<circle cx="{bus_7v4_x}" cy="{c2_y+54}" r="4.5" fill="{c_wire_red}"/>')
    
    # Connect GND from ESC1 & ESC2 to GND Bus
    svg.append(f'<line x1="{c_x+c_w}" y1="{c1_y+78}" x2="{bus_gnd_x}" y2="{c1_y+78}" stroke="{c_wire_black}" stroke-width="3"/>')
    svg.append(f'<circle cx="{bus_gnd_x}" cy="{c1_y+78}" r="4.5" fill="{c_wire_black}"/>')
    svg.append(f'<line x1="{c_x+c_w}" y1="{c2_y+78}" x2="{bus_gnd_x}" y2="{c2_y+78}" stroke="{c_wire_black}" stroke-width="3"/>')
    svg.append(f'<circle cx="{bus_gnd_x}" cy="{c2_y+78}" r="4.5" fill="{c_wire_black}"/>')

    # Load 3: 4x Servomotores de Dirección (5V Rail)
    c3_y = 440
    svg.append(f'<rect x="{c_x}" y="{c3_y}" width="{c_w}" height="145" rx="8" fill="#FFFFFF" stroke="#27AE60" stroke-width="2" filter="url(#shadow2)"/>')
    svg.append(f'<text x="{c_x+16}" y="{c3_y+28}" fill="#27AE60" font-size="13" font-weight="bold">CARGA 3 : 4x SERVOMOTORES DE DIRECCIÓN (MG996R)</text>')
    svg.append(f'<text x="{c_x+16}" y="{c3_y+58}" fill="{c_wire_5v}" font-size="12">&#8226; Alimentación V+: 5.0V Regulada desde UBEC (Consumo ~2.5A pico)</text>')
    svg.append(f'<text x="{c_x+16}" y="{c3_y+85}" fill="{c_wire_black}" font-size="12">&#8226; Retorno V-: Conexión rígida a GND Común</text>')
    svg.append(f'<text x="{c_x+16}" y="{c3_y+115}" fill="#7F8C8D" font-size="11">Protección: El UBEC filtra los picos inductivos de los servos aislando la lógica</text>')

    # Connect 5V to Servos
    svg.append(f'<line x1="{bus_5v_x}" y1="{c3_y+54}" x2="{c_x}" y2="{c3_y+54}" stroke="{c_wire_5v}" stroke-width="3"/>')
    svg.append(f'<circle cx="{bus_5v_x}" cy="{c3_y+54}" r="4.5" fill="{c_wire_5v}"/>')
    svg.append(f'<line x1="{c_x+c_w}" y1="{c3_y+81}" x2="{bus_gnd_x}" y2="{c3_y+81}" stroke="{c_wire_black}" stroke-width="3"/>')
    svg.append(f'<circle cx="{bus_gnd_x}" cy="{c3_y+81}" r="4.5" fill="{c_wire_black}"/>')

    # Load 4: ESP32 Microcontrolador (5V Rail)
    c4_y = 635
    svg.append(f'<rect x="{c_x}" y="{c4_y}" width="{c_w}" height="145" rx="8" fill="#FFFFFF" stroke="#8E44AD" stroke-width="2" filter="url(#shadow2)"/>')
    svg.append(f'<text x="{c_x+16}" y="{c4_y+28}" fill="#8E44AD" font-size="13" font-weight="bold">CARGA 4 : ESP32 DEVKIT-C V4 (LÓGICA Y COMUNICACIONES)</text>')
    svg.append(f'<text x="{c_x+16}" y="{c4_y+58}" fill="{c_wire_5v}" font-size="12">&#8226; Entrada de Alimentación: Pin "5V / VIN" (Consumo ~180 - 260 mA)</text>')
    svg.append(f'<text x="{c_x+16}" y="{c4_y+85}" fill="{c_wire_black}" font-size="12">&#8226; Referencia: Pin "GND" unido rígidamente al Bus de Masa Común</text>')
    svg.append(f'<text x="{c_x+16}" y="{c4_y+115}" fill="#7F8C8D" font-size="11">Filtro interno: El LDO onboard del ESP32 baja los 5V a 3.3V limpios para el SoC</text>')

    # Connect 5V to ESP32
    svg.append(f'<line x1="{bus_5v_x}" y1="{c4_y+54}" x2="{c_x}" y2="{c4_y+54}" stroke="{c_wire_5v}" stroke-width="3"/>')
    svg.append(f'<circle cx="{bus_5v_x}" cy="{c4_y+54}" r="4.5" fill="{c_wire_5v}"/>')
    svg.append(f'<line x1="{c_x+c_w}" y1="{c4_y+81}" x2="{bus_gnd_x}" y2="{c4_y+81}" stroke="{c_wire_black}" stroke-width="3"/>')
    svg.append(f'<circle cx="{bus_gnd_x}" cy="{c4_y+81}" r="4.5" fill="{c_wire_black}"/>')

    # ----------------------------------------------------
    # CRITICAL POWER RULES (Bottom Left Box: x=85 to x=490)
    # ----------------------------------------------------
    leg_x = 85
    leg_y = 660
    leg_w = 410
    leg_h = 240
    svg.append(f'<rect x="{leg_x}" y="{leg_y}" width="{leg_w}" height="{leg_h}" rx="6" fill="#FFFFFF" stroke="#BDC3C7" stroke-width="1.5" filter="url(#shadow2)"/>')
    svg.append(f'<rect x="{leg_x}" y="{leg_y}" width="{leg_w}" height="28" rx="4" fill="#34495E"/>')
    svg.append(f'<text x="{leg_x+14}" y="{leg_y+19}" fill="#FFFFFF" font-size="12" font-weight="bold">REGLAS CRÍTICAS DE ENERGÍA Y CABLEADO</text>')
    
    rules = [
        ("1. Doble Riel:", "La batería alimenta directo a los ESCs (7.4V) y el"),
        ("", "UBEC alimenta exclusivamente a servos y ESP32 (5.0V)."),
        ("2. Calibre de Cables:", "Usa cable grueso (14-16 AWG) para batería y ESCs."),
        ("", "Usa cable standard (22-24 AWG) para servos y ESP32."),
        ("3. Masa Única (Estrella):", "Une todos los cables negativos en un punto"),
        ("", "central para evitar lazos de tierra (ground loops)."),
        ("4. Jamás alimentar servos", "desde el pin 3.3V ni 5V del ESP32;"),
        ("", "provocaría reinicios continuos (brownouts).")
    ]
    for i, (r_title, r_desc) in enumerate(rules):
        ry = leg_y + 50 + i * 22
        svg.append(f'<text x="{leg_x+12}" y="{ry}" fill="#C0392B" font-size="10.5" font-weight="bold">{r_title} <tspan fill="#2C3E50" font-weight="normal">{r_desc}</tspan></text>')

    svg.append('</svg>')
    
    with open("docs/schematic_power_supply.svg", "w", encoding="utf-8") as f:
        f.write("\n".join(svg))
    print("Power supply schematic SVG created.")


if __name__ == "__main__":
    create_motors_schematic()
    create_power_schematic()
