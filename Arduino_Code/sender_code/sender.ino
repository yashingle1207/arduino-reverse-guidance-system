/*
 * Project: Reverse Guidance System for Automobiles using CAN Protocol
 * File: sensor.ino
 * Author: Yash Daniel Ingle
 * Description:
 *   Sensor node for a reverse parking assistance system.
 *   Reads three ultrasonic sensors, converts measured distance
 *   into discrete proximity levels, packs the levels into a CAN frame,
 *   and transmits them to the display node.
 */

#include <SPI.h>
#include <mcp2515.h>

// -----------------------------
// Serial / CAN Configuration
// -----------------------------
constexpr uint32_t SERIAL_BAUDRATE = 115200;
constexpr uint8_t CAN_CS_PIN       = 10;
constexpr uint16_t CAN_FRAME_ID    = 0x100;

MCP2515 canController(CAN_CS_PIN);
struct can_frame txFrame;

// -----------------------------
// Sensor Configuration
// Using single-pin HC-SR04 style connection per sensor
// -----------------------------
constexpr uint8_t NUM_SENSORS = 3;
const uint8_t sensorPins[NUM_SENSORS] = {4, 5, 6};

// pulseIn timeout: 30 ms ~ 5 meters
constexpr unsigned long SENSOR_TIMEOUT_US = 30000UL;

// reading delay between sensors
constexpr uint16_t SENSOR_SETTLE_DELAY_MS = 50;
constexpr uint16_t MAIN_LOOP_DELAY_MS     = 200;

// invalid distance marker
constexpr uint16_t INVALID_DISTANCE_CM = 0xFFFF;

// -----------------------------
// Last known proximity levels
// -----------------------------
uint8_t lastLevels[NUM_SENSORS] = {0, 0, 0};

// -----------------------------
// Read ultrasonic distance
// Returns distance in cm or INVALID_DISTANCE_CM if invalid
// -----------------------------
uint16_t readDistanceCm(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(2);

  digitalWrite(pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin, LOW);

  pinMode(pin, INPUT);
  unsigned long duration = pulseIn(pin, HIGH, SENSOR_TIMEOUT_US);

  if (duration == 0) {
    return INVALID_DISTANCE_CM;
  }

  float distanceCm = (duration * 0.0343f) / 2.0f;
  return static_cast<uint16_t>(distanceCm);
}

// -----------------------------
// Map distance to proximity level
// 0 = invalid / no data
// 1 = very close
// 5 = far
// -----------------------------
uint8_t distanceToLevel(uint16_t distanceCm) {
  if (distanceCm == INVALID_DISTANCE_CM) return 0;
  if (distanceCm > 50) return 5;
  if (distanceCm > 35) return 4;
  if (distanceCm > 20) return 3;
  if (distanceCm > 10) return 2;
  return 1;
}

// -----------------------------
// Pack 3 levels (3 bits each) into 2 bytes
// [ sensor0(3 bits) | sensor1(3 bits) | sensor2(3 bits) ]
// -----------------------------
uint16_t packLevels(const uint8_t levels[NUM_SENSORS]) {
  return ((levels[0] & 0x07) << 6) |
         ((levels[1] & 0x07) << 3) |
          (levels[2] & 0x07);
}

// -----------------------------
// Check if readings changed
// -----------------------------
bool levelsChanged(const uint8_t currentLevels[NUM_SENSORS],
                   const uint8_t previousLevels[NUM_SENSORS]) {
  for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
    if (currentLevels[i] != previousLevels[i]) {
      return true;
    }
  }
  return false;
}

// -----------------------------
// Copy levels
// -----------------------------
void copyLevels(uint8_t dst[NUM_SENSORS], const uint8_t src[NUM_SENSORS]) {
  for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
    dst[i] = src[i];
  }
}

// -----------------------------
// Send levels over CAN
// -----------------------------
void sendLevelsOverCan(const uint8_t levels[NUM_SENSORS]) {
  uint16_t packed = packLevels(levels);

  txFrame.can_id  = CAN_FRAME_ID;
  txFrame.can_dlc = 2;
  txFrame.data[0] = highByte(packed);
  txFrame.data[1] = lowByte(packed);

  MCP2515::ERROR result = canController.sendMessage(&txFrame);

  Serial.print(F("Packed CAN: 0x"));
  Serial.print(txFrame.data[0], HEX);
  Serial.print(F(" 0x"));
  Serial.print(txFrame.data[1], HEX);
  Serial.print(F(" -> "));

  if (result == MCP2515::ERROR_OK) {
    Serial.println(F("sent"));
  } else {
    Serial.println(F("send failed"));
  }
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
  Serial.begin(SERIAL_BAUDRATE);

  canController.reset();
  canController.setBitrate(CAN_125KBPS);
  canController.setNormalMode();

  for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
    pinMode(sensorPins[i], OUTPUT);
    digitalWrite(sensorPins[i], LOW);
  }

  Serial.println(F("Sensor node initialized."));
}

// -----------------------------
// Main Loop
// -----------------------------
void loop() {
  uint8_t currentLevels[NUM_SENSORS] = {0, 0, 0};

  for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
    uint16_t distanceCm = readDistanceCm(sensorPins[i]);
    currentLevels[i] = distanceToLevel(distanceCm);

    Serial.print(F("Sensor "));
    Serial.print(i);
    Serial.print(F(": "));

    if (distanceCm == INVALID_DISTANCE_CM) {
      Serial.print(F("Invalid"));
    } else {
      Serial.print(distanceCm);
      Serial.print(F(" cm"));
    }

    Serial.print(F(" -> Level "));
    Serial.println(currentLevels[i]);

    delay(SENSOR_SETTLE_DELAY_MS);
  }

  if (levelsChanged(currentLevels, lastLevels)) {
    sendLevelsOverCan(currentLevels);
    copyLevels(lastLevels, currentLevels);
  } else {
    Serial.println(F("No level change; skipping CAN transmission."));
  }

  delay(MAIN_LOOP_DELAY_MS);
}