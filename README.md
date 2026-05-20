# ESP32-S3 Stream Deck Mini -> EOS Macro Controller

Turns an Elgato Stream Deck Mini into a dedicated macro trigger panel for ETC Element (EOS) lighting consoles. The ESP32-S3 acts as a USB host for the Stream Deck and sends OSC commands over WiFi to fire macros on the console.

```
Stream Deck Mini  --USB-->  ESP32-S3  --WiFi/OSC-->  ETC Element (EOS)
```

## Hardware Requirements

- **ESP32-S3 DevKit** (any board with USB-OTG port, e.g. ESP32-S3-DevKitC-1)
- **Elgato Stream Deck Mini** (original or MK.2)
- **USB-A to USB-C OTG adapter/cable** to connect the Stream Deck to the ESP32-S3's USB-OTG port
- WiFi network shared with the ETC Element console

### Important: USB Port Selection

The ESP32-S3 DevKit typically has **two USB ports**:
- **USB-UART** - for programming/serial monitor (usually labeled UART or COM)
- **USB-OTG** - for USB Host mode (connect the Stream Deck here)

You must connect the Stream Deck Mini to the **USB-OTG** port.

## Software Setup

### 1. Arduino IDE Configuration

1. Open Arduino IDE
2. Go to **File > Preferences**
3. Add the ESP32 board manager URL if not already present:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
4. Go to **Tools > Board > Board Manager**, search for **esp32**, and install **esp32 by Espressif Systems**
5. Select your board: **Tools > Board > ESP32S3 Dev Module**
6. Set **USB Mode** to **USB-OTG (TinyUSB)** under Tools menu
7. Set **Upload Mode** to **UART0 / Hardware CDC** (upload via the UART port)

### 2. Install Required Library

1. Go to **Sketch > Include Library > Manage Libraries**
2. Search for **EspUsbHost** by tanakamasayuki
3. Click **Install**

### 3. Configure the Project

1. Open `esp32-streamdeck-eos/esp32-streamdeck-eos.ino` in Arduino IDE
2. Edit `config.h` with your settings:

```c
// Your WiFi credentials
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// IP address of your ETC Element console
#define EOS_IP        "192.168.1.100"

// OSC receive port (default 8000)
#define EOS_OSC_PORT  8000
```

3. Adjust the macro mapping if needed (default maps buttons 0-5 to macros 1-6):

```c
const int MACRO_MAP[6] = {
  1,  // Button 0 -> Macro 1
  2,  // Button 1 -> Macro 2
  3,  // Button 2 -> Macro 3
  4,  // Button 3 -> Macro 4
  5,  // Button 4 -> Macro 5
  6   // Button 5 -> Macro 6
};
```

### 4. Upload

1. Connect the ESP32-S3 via the **USB-UART** port
2. Select the correct COM port under **Tools > Port**
3. Click **Upload**
4. Open **Serial Monitor** (115200 baud) to see status messages

## EOS Console Setup

### Enable OSC on the Element

1. Press **Displays** on the console
2. Navigate to **Shell > Settings > Show Control > OSC**
3. Enable **OSC RX** (receive)
4. Set the **OSC RX Port** to match `EOS_OSC_PORT` in config.h (default: 8000)
5. Note the console's **IP address** from Shell > Settings > Network and update `EOS_IP` in config.h

### Create Macros

Create the macros on the Element that you want to trigger. For example:
- **Macro 1**: Go Cue 1
- **Macro 2**: Go Cue 2
- **Macro 3**: Blackout
- etc.

## Network Setup

Both the ESP32-S3 and the ETC Element must be on the **same network**:

1. Connect the ETC Element to your network via Ethernet (its primary network port)
2. Ensure your WiFi router/access point is on the same subnet
3. The ESP32-S3 connects via WiFi to the same network
4. Verify connectivity: the ESP32 serial monitor will show its IP address on boot

If your network uses VLANs or firewall rules, ensure UDP traffic on port 8000 is allowed between the ESP32 and the console.

## Button Layout

The Stream Deck Mini has a 3x2 grid:

```
+--------+--------+
| Btn 0  | Btn 1  |
| Macro1 | Macro2 |
+--------+--------+
| Btn 2  | Btn 3  |
| Macro3 | Macro4 |
+--------+--------+
| Btn 4  | Btn 5  |
| Macro5 | Macro6 |
+--------+--------+
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "USB Host init failed" | Make sure Stream Deck is on the USB-OTG port, not USB-UART |
| WiFi won't connect | Check SSID/password in config.h, ensure network is 2.4GHz (ESP32 doesn't support 5GHz) |
| No response from console | Verify EOS_IP and EOS_OSC_PORT match console settings, check OSC RX is enabled |
| Buttons not detected | Open Serial Monitor to see if HID data is being received, try unplugging/replugging the Stream Deck |

## Supported Devices

| Device | VID | PID | Supported |
|--------|-----|-----|-----------|
| Stream Deck Mini | 0x0FD9 | 0x0063 | Yes |
| Stream Deck Mini MK.2 | 0x0FD9 | 0x0090 | Yes |
