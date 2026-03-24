# Reverse Guidance System for Automobiles using CAN Protocol

A dual-node Arduino reverse-guidance system that combines ultrasonic sensing with CAN-based communication to provide real-time visual and audible parking assistance.

## Overview

This project implements a low-cost reverse parking assistance system using off-the-shelf embedded hardware. The design is split into two Arduino-based nodes connected over a CAN bus:

- a **Sensor Node** that measures rear obstacle distance using ultrasonic sensors
- a **Display Node** that receives the encoded proximity information and presents it through an **OLED display** and a **buzzer**

Instead of sending raw distance values continuously, the system converts measurements into discrete proximity levels and transmits compact CAN messages between nodes. This keeps the communication simple, modular, and suitable for automotive-style embedded integration.

The project was built as a practical prototype to demonstrate how a distributed embedded system can provide reliable reverse guidance using inexpensive components and standard communication interfaces.

---

## Key Features

- **Distributed two-node architecture** using Arduino Uno boards
- **Ultrasonic obstacle detection** for rear proximity sensing
- **CAN-based inter-node communication** using MCP2515 modules
- **Real-time OLED visualization** of left / center / right obstacle levels
- **Adaptive buzzer feedback** with faster beeps for closer obstacles
- **Compact data encoding** by packing proximity levels into CAN payload bytes
- **Low-cost breadboard prototype** suitable for lab demonstration and embedded-systems learning
- **Modular design** that separates sensing and driver feedback into independent nodes

---

## System Architecture

The system is divided into two cooperating embedded nodes:

### 1. Sensor Node
The sensor node is responsible for:
- initializing the ultrasonic sensing hardware
- reading distance values from rear-facing sensors
- mapping measured distances into proximity levels
- packing those levels into a compact CAN frame
- transmitting the encoded data to the display node

### 2. Display Node
The display node is responsible for:
- receiving proximity data over CAN
- decoding the packed proximity values
- updating the OLED display with obstacle guidance
- generating buzzer alerts based on the nearest detected obstacle

### High-Level Block Diagram

<p align="center">
  <img src="Images/img1.png" alt="System architecture block diagram" width="700">
</p>

---

## Working Principle

The reverse guidance logic follows a simple real-time pipeline:

1. The **Sensor Node** reads the ultrasonic sensors.
2. Each reading is converted into a **discrete proximity level**.
3. The levels are packed into a small CAN payload.
4. The **Display Node** receives and decodes the message.
5. The OLED updates the left / center / right obstacle bars.
6. The buzzer changes its beep interval based on obstacle proximity.

This separation keeps sensing, communication, and feedback cleanly modular, which simplifies debugging and future extension.

---

## Control Flow

<p align="center">
  <img src="Images/img2.png" alt="System flowchart for sensor and display nodes" width="520">
</p>

---

## Hardware Used

### Core Components
- **Arduino Uno** × 2
- **MCP2515 CAN controller / transceiver modules** × 2
- **HC-SR04 ultrasonic sensors** × 3
- **0.96-inch OLED display** × 1
- **Buzzer** × 1
- Breadboards, jumper wires, USB power, and CAN interconnect wiring

### Communication and Interfaces
- **CAN bus** for node-to-node communication
- **SPI** for MCP2515 interfacing
- **I2C** for OLED interfacing
- Digital GPIO for ultrasonic sensing and buzzer control

---

## Communication Details

- **Bus Type:** Controller Area Network (CAN)
- **Bitrate:** **125 kbps**
- **Topology:** Two-node bus with MCP2515-based CAN interface
- **Termination:** 120 Ω termination at both ends of the CAN line
- **Payload Strategy:** Proximity levels are encoded into compact CAN data bytes for efficient transmission

The use of CAN makes the project more realistic from an automotive embedded-systems perspective, compared with simple UART-only point-to-point communication.

---

## Proximity Encoding Strategy

Each ultrasonic zone is mapped to a discrete level representing obstacle closeness.  
These levels are then packed into a compact CAN frame before transmission.

Example logic:
- farther object → lower urgency
- closer object → higher urgency
- no valid object / invalid reading → no active alert

This approach reduces message size while keeping the display and buzzer response intuitive.

---

## Repository Structure

```text
arduino-reverse-guidance-system/
├── README.md
├── Arduino_Code/
│   ├── sender_code/
│   │   └── sender.ino
│   └── display_code/
│       └── display.ino
└── Images/
    ├── img1.png
    ├── img2.png
    ├── img3.jpeg
    ├── img4.jpeg
    ├── img5.jpeg
    └── img6.png
