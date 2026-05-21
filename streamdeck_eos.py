#!/usr/bin/env python3
"""
Stream Deck Mini -> ETC EOS Macro Controller

Reads button presses from an Elgato Stream Deck Mini and sends OSC
commands over WiFi to fire macros on an ETC Element lighting console.

Hardware: Raspberry Pi Zero 2W + Elgato Stream Deck Mini
"""

import json
import os
import signal
import sys
import time
from io import BytesIO
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from pythonosc.udp_client import SimpleUDPClient
from StreamDeck.DeviceManager import DeviceManager
from StreamDeck.ImageHelpers import PILHelper

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

CONFIG_PATH = Path(__file__).parent / "config.json"


def load_config():
    with open(CONFIG_PATH) as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# Button image generation
# ---------------------------------------------------------------------------

def render_button_image(deck, label, color_hex):
    """Create a button image with a colored background and text label."""
    image = PILHelper.create_image(deck)
    draw = ImageDraw.Draw(image)

    # Fill with background color
    draw.rectangle([(0, 0), image.size], fill=color_hex)

    # Try to load a nice font, fall back to default
    font = None
    font_paths = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
    ]
    for fp in font_paths:
        if os.path.exists(fp):
            font = ImageFont.truetype(fp, 14)
            break
    if font is None:
        font = ImageFont.load_default()

    # Draw label centered
    bbox = draw.textbbox((0, 0), label, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]
    x = (image.width - text_w) // 2
    y = (image.height - text_h) // 2
    draw.text((x, y), label, font=font, fill="white")

    return PILHelper.to_native_format(deck, image)


def render_icon_button(deck, icon_path, label, color_hex):
    """Create a button image with a colored background, icon, and label."""
    image = PILHelper.create_image(deck)
    draw = ImageDraw.Draw(image)
    draw.rectangle([(0, 0), image.size], fill=color_hex)

    # Load and resize icon
    icon = Image.open(icon_path).convert("RGBA")
    icon_size = int(image.width * 0.5)
    icon = icon.resize((icon_size, icon_size), Image.LANCZOS)

    # Paste icon centered horizontally, in upper portion
    icon_x = (image.width - icon_size) // 2
    icon_y = int(image.height * 0.1)
    image.paste(icon, (icon_x, icon_y), icon)

    # Draw label below icon
    font = None
    font_paths = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
    ]
    for fp in font_paths:
        if os.path.exists(fp):
            font = ImageFont.truetype(fp, 12)
            break
    if font is None:
        font = ImageFont.load_default()

    draw = ImageDraw.Draw(image)
    bbox = draw.textbbox((0, 0), label, font=font)
    text_w = bbox[2] - bbox[0]
    x = (image.width - text_w) // 2
    y = int(image.height * 0.72)
    draw.text((x, y), label, font=font, fill="white")

    return PILHelper.to_native_format(deck, image)


# ---------------------------------------------------------------------------
# OSC
# ---------------------------------------------------------------------------

def create_osc_client(config):
    eos = config["eos"]
    client = SimpleUDPClient(eos["ip"], eos["osc_port"])
    print(f"OSC target: {eos['ip']}:{eos['osc_port']}")
    return client


def fire_macro(osc_client, macro_num):
    """Send /eos/macro/<num>/fire to the EOS console."""
    address = f"/eos/macro/{macro_num}/fire"
    osc_client.send_message(address, None)
    print(f"OSC -> {address}")


# ---------------------------------------------------------------------------
# Stream Deck button handler
# ---------------------------------------------------------------------------

def make_key_callback(osc_client, button_config):
    """Create the callback for Stream Deck key events."""
    # Build a lookup from button index to macro number
    macro_map = {}
    for btn in button_config:
        macro_map[btn["index"]] = btn["macro"]

    def key_callback(deck, key, state):
        if state:  # Fire on key press (not release)
            macro_num = macro_map.get(key)
            if macro_num is not None:
                print(f"Button {key} pressed -> Macro {macro_num}")
                fire_macro(osc_client, macro_num)

    return key_callback


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("=================================")
    print("Stream Deck EOS Macro Controller")
    print("=================================")

    # Load config
    config = load_config()
    print(f"Loaded config from {CONFIG_PATH}")

    # Set up OSC client
    osc_client = create_osc_client(config)

    # Find Stream Deck
    decks = DeviceManager().enumerate()
    if not decks:
        print("ERROR: No Stream Deck found. Is it plugged in?")
        sys.exit(1)

    deck = decks[0]
    deck.open()
    deck.reset()

    print(f"Found: {deck.deck_type()} (serial: {deck.get_serial_number()})")
    print(f"Keys: {deck.key_count()}")

    # Set button images
    for btn in config["buttons"]:
        idx = btn["index"]
        if idx >= deck.key_count():
            continue

        if btn.get("icon") and os.path.exists(btn["icon"]):
            image = render_icon_button(deck, btn["icon"], btn["label"], btn["color"])
        else:
            image = render_button_image(deck, btn["label"], btn["color"])

        deck.set_key_image(idx, image)

    # Set brightness
    deck.set_brightness(80)

    # Register button callback
    deck.set_key_callback(make_key_callback(osc_client, config["buttons"]))

    print("Ready. Press Ctrl+C to exit.")

    # Handle clean shutdown
    def shutdown(signum, frame):
        print("\nShutting down...")
        deck.reset()
        deck.close()
        sys.exit(0)

    signal.signal(signal.SIGINT, shutdown)
    signal.signal(signal.SIGTERM, shutdown)

    # Keep running
    while True:
        time.sleep(1)


if __name__ == "__main__":
    main()
