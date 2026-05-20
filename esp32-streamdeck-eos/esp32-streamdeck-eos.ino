/**
 * ESP32-S3 Stream Deck Mini -> EOS Macro Controller
 *
 * Reads button presses from an Elgato Stream Deck Mini via USB Host
 * and sends OSC commands over WiFi to fire macros on an ETC Element
 * console running EOS software.
 *
 * Hardware: ESP32-S3 DevKit with USB-OTG port connected to Stream Deck Mini
 * Libraries: EspUsbHost (install via Arduino Library Manager)
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include "EspUsbHost.h"
#include "config.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
EspUsbHost usbHost;
WiFiUDP udp;

// Track previous button states for edge detection (fire on press only)
uint8_t prevButtonState[NUM_BUTTONS] = {0};

// WiFi reconnect tracking
unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000; // ms

// Stream Deck connection state
bool streamDeckConnected = false;

// ---------------------------------------------------------------------------
// OSC helpers
// ---------------------------------------------------------------------------

// Pad a string length up to the next 4-byte boundary (OSC requirement)
static int oscPaddedLen(int len) {
  return (len + 4) & ~3;
}

// Build and send an OSC message: /eos/macro/<num>/fire (no arguments)
void sendMacroFire(int macroNum) {
  // Build the OSC address string
  char address[64];
  snprintf(address, sizeof(address), "/eos/macro/%d/fire", macroNum);

  int addrLen = strlen(address) + 1; // include null terminator
  int addrPadded = oscPaddedLen(addrLen);

  // Type tag: comma + no type chars + null, padded to 4 bytes
  // ",\0\0\0" = 4 bytes (already aligned)
  int typeTagPadded = 4;

  int packetLen = addrPadded + typeTagPadded;
  uint8_t packet[128];
  memset(packet, 0, sizeof(packet));

  // Write address string (null-padded)
  memcpy(packet, address, addrLen);

  // Write type tag string
  packet[addrPadded] = ',';

  // Send via UDP
  udp.beginPacket(EOS_IP, EOS_OSC_PORT);
  udp.write(packet, packetLen);
  udp.endPacket();

  Serial.printf("OSC -> %s:%d  %s\n", EOS_IP, EOS_OSC_PORT, address);
}

// ---------------------------------------------------------------------------
// USB Host HID callback
// ---------------------------------------------------------------------------

void onHIDInput(const EspUsbHostHIDInput &input) {
  // Stream Deck Mini input report: 65 bytes total
  // Byte 0: Report ID (0x01)
  // Bytes 1-6: Button states (0x00=released, 0x01=pressed)
  //
  // Note: The EspUsbHost library may strip the report ID, so we check both
  // cases: data starting with 0x01 (report ID present) or raw button bytes.

  if (input.length < NUM_BUTTONS) {
    return;
  }

  // Determine where button data starts
  const uint8_t *buttonData;
  if (input.length >= NUM_BUTTONS + 1 && input.data[0] == 0x01) {
    // Report ID is present at byte 0
    buttonData = &input.data[1];
  } else {
    // Report ID stripped by library
    buttonData = input.data;
  }

  if (!streamDeckConnected) {
    streamDeckConnected = true;
    Serial.println("Stream Deck Mini connected and sending data");
  }

  // Process each button - fire macro on press (rising edge)
  for (int i = 0; i < NUM_BUTTONS; i++) {
    uint8_t current = buttonData[i] ? 1 : 0;

    if (current && !prevButtonState[i]) {
      // Button just pressed
      Serial.printf("Button %d pressed -> firing Macro %d\n", i, MACRO_MAP[i]);
      sendMacroFire(MACRO_MAP[i]);
    }

    prevButtonState[i] = current;
  }
}

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------

void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi connection failed. Will retry...");
  }
}

// ---------------------------------------------------------------------------
// Setup & Loop
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=================================");
  Serial.println("ESP32-S3 Stream Deck EOS Controller");
  Serial.println("=================================");

  // Connect to WiFi
  connectWiFi();

  // Initialize UDP for OSC
  udp.begin(0); // Use any available local port

  // Initialize USB Host
  usbHost.onHIDInput(onHIDInput);

  if (!usbHost.begin()) {
    Serial.printf("USB Host init failed: %s\n", usbHost.lastErrorName());
    Serial.println("Check that you are using the USB-OTG port, not the USB-UART port.");
  } else {
    Serial.println("USB Host initialized - plug in Stream Deck Mini");
  }

  Serial.printf("OSC target: %s:%d\n", EOS_IP, EOS_OSC_PORT);
  Serial.println("Ready.");
}

void loop() {
  // Reconnect WiFi if disconnected
  unsigned long now = millis();
  if (now - lastWifiCheck > WIFI_CHECK_INTERVAL) {
    lastWifiCheck = now;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, reconnecting...");
      connectWiFi();
    }
  }

  // Small delay to avoid busy-looping
  // USB HID processing happens in a background task via EspUsbHost
  delay(10);
}
