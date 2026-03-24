/*
 * Project: Reverse Guidance System for Automobiles using CAN Protocol
 * File: display.ino
 * Author: Yash Daniel Ingle
 * Description:
 *   Display node for a reverse parking assistance system.
 *   Receives packed proximity levels from the sensor node over CAN,
 *   renders bar-style guidance on an OLED, and drives a buzzer
 *   with distance-based beep intervals.
 */

#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -----------------------------
// OLED Configuration
// -----------------------------
constexpr uint8_t OLED_WIDTH  = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr int8_t  OLED_RESET  = -1;
constexpr uint8_t OLED_ADDR   = 0x3C;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// -----------------------------
// CAN Configuration
// -----------------------------
constexpr uint8_t CAN_CS_PIN       = 10;
constexpr uint16_t CAN_FRAME_ID    = 0x100;
constexpr uint32_t SERIAL_BAUDRATE = 115200;

MCP2515 canController(CAN_CS_PIN);
struct can_frame rxFrame;

// -----------------------------
// Buzzer Configuration
// -----------------------------
constexpr uint8_t BUZZER_PIN = 9;
constexpr uint16_t BEEP_ON_TIME_MS = 100;

// -----------------------------
// Proximity Level Definitions
// 0 = no data / invalid
// 1 = very close
// 5 = far
// -----------------------------
constexpr uint8_t NUM_ZONES = 3;
uint8_t currentLevels[NUM_ZONES] = {0, 0, 0};

unsigned long lastBeepToggleMs = 0;
bool buzzerOn = false;
uint16_t currentBeepIntervalMs = 0;

// -----------------------------
// Helper: Level Label
// -----------------------------
const char* levelToLabel(uint8_t level) {
  switch (level) {
    case 1: return "Very Short";
    case 2: return "Short";
    case 3: return "Medium";
    case 4: return "High";
    case 5: return "Very High";
    default: return "No Data";
  }
}

// -----------------------------
// Helper: Decode packed 3x3-bit levels
// bits: [L2 L1 L0] = 9 bits packed into 2 bytes
// [ sensor0(3 bits) | sensor1(3 bits) | sensor2(3 bits) ]
// -----------------------------
void unpackLevels(uint16_t packed, uint8_t levels[NUM_ZONES]) {
  levels[0] = (packed >> 6) & 0x07;
  levels[1] = (packed >> 3) & 0x07;
  levels[2] =  packed       & 0x07;
}

// -----------------------------
// OLED Rendering
// -----------------------------
void drawBars(const uint8_t levels[NUM_ZONES]) {
  static const char* zoneLabels[NUM_ZONES] = {"Back L", "Back", "Back R"};

  constexpr int barWidth = 10;
  constexpr int xStart = 8;
  constexpr int xSpacing = 40;
  constexpr int barBaseY = 34;
  constexpr int barStep = 5;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println(F("Reverse Proximity"));

  for (uint8_t i = 0; i < NUM_ZONES; ++i) {
    int x = xStart + (i * xSpacing);
    uint8_t level = levels[i];

    for (uint8_t l = 0; l < level; ++l) {
      int y = barBaseY - (l * barStep);
      display.drawLine(x, y, x + barWidth, y, SSD1306_WHITE);
    }

    display.setCursor(x - 4, 40);
    display.print(zoneLabels[i]);

    display.setCursor(x - 4, 52);
    display.print(levelToLabel(level));
  }

  display.display();
}

// -----------------------------
// Buzzer Interval Mapping
// Smaller level => closer object => faster beeps
// -----------------------------
uint16_t levelToBeepInterval(uint8_t nearestLevel) {
  switch (nearestLevel) {
    case 1: return 150;
    case 2: return 400;
    case 3: return 800;
    case 4: return 1500;
    default: return 0;  // no valid object / no beep
  }
}

// -----------------------------
// Find nearest active level
// -----------------------------
uint8_t getNearestActiveLevel(const uint8_t levels[NUM_ZONES]) {
  uint8_t nearest = 255;

  for (uint8_t i = 0; i < NUM_ZONES; ++i) {
    if (levels[i] > 0 && levels[i] < nearest) {
      nearest = levels[i];
    }
  }

  return (nearest == 255) ? 0 : nearest;
}

// -----------------------------
// Buzzer Control
// -----------------------------
void updateBuzzer(const uint8_t levels[NUM_ZONES]) {
  uint8_t nearestLevel = getNearestActiveLevel(levels);
  currentBeepIntervalMs = levelToBeepInterval(nearestLevel);

  if (currentBeepIntervalMs == 0) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
    return;
  }

  unsigned long now = millis();

  if (!buzzerOn && (now - lastBeepToggleMs >= currentBeepIntervalMs)) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerOn = true;
    lastBeepToggleMs = now;
  } else if (buzzerOn && (now - lastBeepToggleMs >= BEEP_ON_TIME_MS)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
    lastBeepToggleMs = now;
  }
}

// -----------------------------
// CAN Receive
// -----------------------------
bool receiveCanLevels(uint8_t levels[NUM_ZONES]) {
  if (canController.readMessage(&rxFrame) != MCP2515::ERROR_OK) {
    return false;
  }

  if (rxFrame.can_id != CAN_FRAME_ID || rxFrame.can_dlc < 2) {
    return false;
  }

  uint16_t packed = (static_cast<uint16_t>(rxFrame.data[0]) << 8) | rxFrame.data[1];
  unpackLevels(packed, levels);

  return true;
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
  Serial.begin(SERIAL_BAUDRATE);

  // CAN init
  canController.reset();
  canController.setBitrate(CAN_125KBPS);
  canController.setNormalMode();

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED initialization failed."));
    while (true) { /* halt */ }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Waiting for CAN data..."));
  display.display();

  // Buzzer init
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println(F("Display node initialized."));
}

// -----------------------------
// Main Loop
// -----------------------------
void loop() {
  if (receiveCanLevels(currentLevels)) {
    Serial.print(F("Received levels: "));
    for (uint8_t i = 0; i < NUM_ZONES; ++i) {
      Serial.print(currentLevels[i]);
      Serial.print(' ');
    }
    Serial.println();
  }

  drawBars(currentLevels);
  updateBuzzer(currentLevels);
}