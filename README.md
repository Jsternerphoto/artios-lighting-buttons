# Stream Deck Mini -> EOS Macro Controller

Turns an Elgato Stream Deck Mini into a dedicated macro trigger panel for ETC Element (EOS) lighting consoles. A Raspberry Pi Zero 2W reads button presses and sends OSC commands over WiFi to fire macros on the console.

```
Stream Deck Mini  --USB-->  Raspberry Pi Zero 2W  --WiFi/OSC-->  ETC Element (EOS)
```

## Hardware Requirements

- **Raspberry Pi Zero 2W** (with micro-USB OTG port)
- **Elgato Stream Deck Mini** (original or MK.2)
- **Micro-USB OTG adapter** (micro-USB male to USB-A female)
- **Micro-USB power supply** (5V 2.5A recommended) for the Pi's power port
- **MicroSD card** (8GB+ with Raspberry Pi OS)
- WiFi network shared with the ETC Element console

### Wiring

The Pi Zero 2W has two micro-USB ports:

```
[HDMI]  [USB/OTG]  [PWR]
         Stream     Power
         Deck       Supply
```

- **PWR** (right) — connect to power supply
- **USB** (left) — connect Stream Deck Mini via OTG adapter

## Software Setup

### 1. Prepare the Raspberry Pi

1. Flash **Raspberry Pi OS Lite (64-bit)** to your microSD card using [Raspberry Pi Imager](https://www.raspberrypi.com/software/)
2. In the imager **gear menu (settings)**, configure:
   - **Hostname**: `streamdeck`
   - **Username/password**: `pi` / your choice (save it to a password manager)
   - **Set authorized SSH key**: yes (uses your local public key, lets you skip the password)
   - **WiFi**: your 2.4 GHz network SSID and password (Pi Zero 2W is 2.4 GHz only — confirm your SSID actually broadcasts on 2.4 GHz)
   - **WiFi country**: USA (required for WiFi to come up)
3. Insert the SD card and boot the Pi. First boot takes 1–2 minutes.
4. SSH in from your computer:
   ```bash
   ssh pi@streamdeck.local
   ```
   If `streamdeck.local` doesn't resolve, find the Pi's IP via your router's DHCP client list or `arp -a` (look for a MAC starting with `b8:27:eb`, `dc:a6:32`, `d8:3a:dd`, `e4:5f:01`, or `2c:cf:67`) and SSH to that IP.

### 2. Install System Dependencies

```bash
sudo apt update
sudo apt install -y git python3-pip python3-venv libusb-1.0-0-dev libhidapi-libusb0 libhidapi-hidraw0 libjpeg-dev zlib1g-dev
```

### 3. Set Up USB Permissions

The Stream Deck needs udev rules to be accessible without root. Use `MODE="0666"` rather than the `uaccess` tag — `uaccess` requires an active logind session, which a plain SSH connection does not provide, and you'll get "Could not open HID device" errors at runtime.

```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", MODE="0666"' | sudo tee /etc/udev/rules.d/70-streamdeck.rules
sudo udevadm control --reload-rules
sudo udevadm trigger --action=add
```

**Important**: after installing the rule, **unplug the Stream Deck from the Pi and plug it back in**. Already-connected devices don't pick up new rules until they're re-attached.

### 4. Install the Application

```bash
cd ~
git clone https://github.com/Jsternerphoto/artios-lighting-buttons.git
cd artios-lighting-buttons
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### 5. Configure

Edit `config.json` with your EOS console settings:

```json
{
  "eos": {
    "ip": "192.168.1.51",
    "osc_port": 8000
  },
  "buttons": [
    { "index": 0, "macro": 1, "label": "Macro 1", "color": "#1E90FF" },
    { "index": 1, "macro": 2, "label": "Macro 2", "color": "#FF6347" },
    ...
  ]
}
```

Each button has:
- **index**: button position (0-5)
- **macro**: EOS macro number to fire
- **label**: text shown on the button
- **color**: background color (hex)
- **icon**: optional path to a PNG icon file

### 6. Test

```bash
source venv/bin/activate
python streamdeck_eos.py
```

You should see the Stream Deck light up with your button labels, and pressing buttons should fire macros on the Element.

### 7. Auto-Start on Boot

```bash
sudo cp streamdeck-eos.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable streamdeck-eos
sudo systemctl start streamdeck-eos
```

Check status:
```bash
sudo systemctl status streamdeck-eos
```

View logs:
```bash
journalctl -u streamdeck-eos -f
```

## EOS Console Setup

### Enable OSC on the Element

1. Press **Displays** on the console
2. Navigate to **Shell > Settings > Show Control > OSC**
3. Enable **OSC RX** (receive)
4. Set the **OSC RX Port** to `8000` (or match your config.json)
5. Note the console's **IP address** from Shell > Settings > Network

### Create Macros

Create the macros on the Element that you want to trigger. For example:
- **Macro 1**: Go Cue 1
- **Macro 2**: Go Cue 2
- **Macro 3**: Blackout
- etc.

## Network Setup

Both the Pi and the ETC Element must be on the **same network**:

1. Connect the ETC Element via Ethernet
2. The Pi Zero 2W connects via WiFi to the same network
3. Ensure UDP traffic on port 8000 is allowed between them

## Button Layout

```
+----------+----------+
|  Btn 0   |  Btn 1   |
|  Macro 1 |  Macro 2 |
+----------+----------+
|  Btn 2   |  Btn 3   |
|  Macro 3 |  Macro 4 |
+----------+----------+
|  Btn 4   |  Btn 5   |
|  Macro 5 |  Macro 6 |
+----------+----------+
```

## Customizing Buttons

Edit `config.json` to change each button's label, color, and macro assignment.

To use a custom icon on a button, add a PNG file and reference it:

```json
{
  "index": 0,
  "macro": 1,
  "label": "Blackout",
  "color": "#000000",
  "icon": "icons/blackout.png"
}
```

Icons should be square PNGs (any size — they'll be resized automatically).

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "No Stream Deck found" | Check USB OTG adapter, try `lsusb` to see if the device appears |
| "Could not open HID device" | udev rule needs `MODE="0666"` (not `uaccess`) — see step 3. After fixing, **unplug + replug the Stream Deck**. |
| "Probe failed to find any functional HID backend" | Missing `libhidapi-libusb0` — install it (`sudo apt install -y libhidapi-libusb0`) |
| Permission denied | Make sure udev rules are installed (step 3), unplug + replug the Stream Deck |
| WiFi won't connect | Pi Zero 2W is 2.4 GHz only — confirm SSID broadcasts on 2.4 GHz, double-check the password (no way to verify until first boot), and that WiFi country is set in the imager |
| No response from console | Verify IP/port in config.json, check OSC RX is enabled on the Element |
| Buttons show but don't fire | Check `journalctl -u streamdeck-eos -f` for OSC errors |
