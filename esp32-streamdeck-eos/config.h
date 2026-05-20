#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi Configuration
// ============================================================
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ============================================================
// ETC EOS / Element OSC Configuration
// ============================================================
// IP address of the ETC Element console on your network.
// Find this on the console: Shell > Settings > Network
#define EOS_IP        "192.168.1.100"

// Default EOS OSC receive port (set in Shell > Settings > Show Control > OSC)
#define EOS_OSC_PORT  8000

// ============================================================
// Macro Mapping
// ============================================================
// Map each Stream Deck Mini button (0-5) to an EOS macro number.
// Physical button layout:
//   [Btn 0]  [Btn 1]
//   [Btn 2]  [Btn 3]
//   [btn 4]  [btn 5]

const int MACRO_MAP[6] = {
  1,  // Button 0 -> Macro 1
  2,  // Button 1 -> Macro 2
  3,  // Button 2 -> Macro 3
  4,  // Button 3 -> Macro 4
  5,  // Button 4 -> Macro 5
  6   // Button 5 -> Macro 6
};

// ============================================================
// Stream Deck Mini USB IDs
// ============================================================
#define STREAMDECK_VID        0x0FD9
#define STREAMDECK_MINI_PID   0x0063  // Original Mini
#define STREAMDECK_MINI2_PID  0x0090  // Mini MK.2

// Number of buttons on the Stream Deck Mini
#define NUM_BUTTONS 6

#endif
