# PTZ Slider Project Status

## Hardware
- **Slider (On-Device):** Arduino Mega
  - 4x stepper motors (Slide, Pan, Tilt, Zoom) via `AccelStepper` + `MultiStepper`
  - WIZ5100 Ethernet shield for network control
  - 128x64 OLED status display
- **Network Controller:** Windows PC running Python/Kivy app with Xbox gamepad
- **Planned Physical Controller:** Arduino Nano + NRF24L01+PA+LNA (SMA, 2.4 GHz) with breakout adapter
  - DIY gamepad-style layout
  - OLED screen for info display
  - Buttons for speed/sensitivity adjustments (no pots)

## Current State

### Completed
- Network-based control system fully functional (Arduino + Python controller)
- Python controller parses Xbox gamepad input and sends UDP packets to Arduino
- Arduino handles real-time stepper control with acceleration curves and deadzone remapping
- Preset system: Save/Recall A and B positions
- Preset looping (A -> B -> A ...) orchestrated by Python controller
- Virtual loop mode (software-driven position tracking) in Python controller
- JSON persistence on PC for positions, presets, speed, inversion settings, and sensitivity
- Initial setup handshake: Arduino requests saved positions/presets from controller on boot

### Code Analysis (Done)
Analyzed both the Arduino sketch and Python controller to understand:
- UDP message protocol between controller and slider
- Motor/input mapping (Left stick = slide, Right stick = pan/tilt, Triggers = zoom)
- Command set (`SET_JOYSTICK`, `SAVE_A`/`B`, `RECALL_A`/`B`, `UPDATE_LOOP`, `INCREASE_SPEED`, etc.)
- How `syncMove` works (blocking, proportional speed scaling across 4 motors)
- How preset loop and virtual loop are implemented
- Where state/persistence currently lives (on the Windows PC in JSON)

## Open Decisions (Before Building Nano Remote)
1. **Where do presets live?**
   - Nano EEPROM?
   - Mega EEPROM / SD card?
   - Volatile only (lose on power cycle)?
2. **Which loop modes to support?**
   - Preset syncMove loop (A <-> B)
   - Virtual joystick loop (position-tracking driven)
   - Both?
3. **Adjustable speeds beyond slide?**
   - Pan/tilt/zoom max speeds are currently hardcoded `const` on the Mega
   - Changing them requires Mega firmware changes
4. **Position feedback to Nano OLED?**
   - Requires Mega to broadcast `CURRENT_POS` over NRF24
5. **Protocol design for NRF24?**
   - NRF24 has a 32-byte payload limit
   - Existing ASCII string protocol is too verbose for direct use
   - Needs compact binary or terse translation layer on the Mega

## Next Step
User will confirm readiness to begin building the Nano remote firmware, at which point we'll define:
- Compact NRF24 packet protocol
- Nano menu system and OLED UI
- Mega-side NRF24 receiver/translation layer
- How to handle persistence and loop logic without the PC
