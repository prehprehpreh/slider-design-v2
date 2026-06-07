# PTZ Slider Project Status

## Hardware

### Slider (On-Device) — Arduino Mega
- 4x stepper motors (Slide, Pan, Tilt, Zoom) via `AccelStepper` + `MultiStepper`
- WIZ5100 Ethernet shield for network control
- 128x64 OLED status display (SSD1306, I2C @ 0x3C)

### Network Controller — Windows PC
- Python/Kivy app with Xbox gamepad via `inputs` library
- UDP communication to/from Mega on ports 44158/44159
- JSON persistence at `~/Documents/Controller2/motor_params.json`

### Physical Wireless Controller — In Progress
- **Prototype board:** Arduino Uno (same pinout as final Nano, easier breadboarding)
- **Final board:** Arduino Nano (ATmega328P)
- **Radio:** NRF24L01+PA+LNA with breakout adapter (3.3V regulator on-board)
- **Display:** 0.96" SSD1306 OLED 128x64 I2C
- **Inputs:** 2x HW-504 analog joysticks, 7x tactile buttons, 1x mini SPDT toggle (power)
- **Button layout:** D-pad Left/Right (left side), A/B/X/Y (right side), Start (center)
- **Archived NRF24 Mega code:** `ORIGINAL FILES DO NOT EDIT/` — prior dual UDP+NRF24 firmware, kept as reference

---

## Current State

### Completed
- Network-based control system fully functional (Mega + Python/Kivy + Xbox gamepad)
- Real-time stepper control with acceleration curves and deadzone remapping
- Preset system: Save/Recall A and B positions across all 4 motors
- Preset looping (A ↔ B) orchestrated by Python controller via `PRESET_A/B_DONE` messages
- Virtual loop mode (software position tracking) in Python controller
- JSON persistence on PC for positions, presets, speed, inversion, sensitivity
- Initial setup handshake: Mega requests saved state from PC on boot
- Full code analysis documented in `code_map.md`
- Wiring diagram for Uno/Nano prototype documented in `wiring_diagram_uno_nano.md`

### Completed — NRF24 Physical Controller
- Dual-mode Mega firmware receives UDP or NRF24 (auto-switches based on Ethernet link)
- Compact binary packet protocol: 8-byte `ControllerPacket` with XOR checksum
- All 4 joystick axes, 6 buttons, OLED popup messages on actions
- **Loop control:** Nano broadcasts `loopEnabled` in bit 7 of every packet; Mega drains radio FIFO between `syncMove()` calls to get latest state
- **Dead battery handling:** If no packet for 5s, Mega defaults to continuing the loop
- Freeze bug fixed: `syncMove()` early-returns when already at target position

### Decided — Nano Controller Design
- **No virtual loop on Nano** — too complex, not needed on the physical remote
- **Nano sends:** joystick values + button states in compact binary every 50ms
- **NRF24 protocol:** 8-byte binary `ControllerPacket`, magic byte `0xAB`, bit 7 of `buttons` = loop state
- **Menu mode:** Start button toggles between normal control and OLED settings menu
  - In menu mode, no signals sent to Mega
  - Menu controls: speed, joystick sensitivity, invert axes
  - Settings saved to Nano EEPROM on Start press (exit)
  - Skeleton menu system to be built from the start, populated later
- **Prototyping on Uno** before finalizing on Nano — identical pinout

### Nano Pin Assignments (Uno + Nano)
| Function | Pin |
|----------|-----|
| NRF24 CE | D8 |
| NRF24 CSN | D9 |
| NRF24 MOSI | D11 |
| NRF24 MISO | D12 |
| NRF24 SCK | D13 |
| OLED SDA | A4 |
| OLED SCL | A5 |
| Left Joystick X | A0 |
| Left Joystick Y | A1 |
| Right Joystick X | A2 |
| Right Joystick Y | A3 |
| Start Button | D2 |
| Button X (Save A) | D3 |
| Button Y (Loop ON) | D4 |
| D-pad Left (Recall A) | D5 |
| D-pad Right (Recall B) | D6 |
| Button A (Loop OFF) | D7 |
| Button B (Save B) | D10 |

---

## Open Decisions
1. **Where do presets live when PC is offline?**
   - Mega EEPROM? Nano EEPROM? Volatile only?
2. **Position feedback to Nano OLED?**
   - Requires Mega to broadcast `CURRENT_POS` back over NRF24

---

## Next Steps
1. Test loop stop/start repeatedly in a live run to confirm reliability
2. Add menu system to Nano (speed, sensitivity, invert axes) via Start button toggle
3. Save/load settings to Nano EEPROM
4. Update `README.md` to reflect current dual-mode firmware architecture
5. Consider Mega EEPROM persistence for presets when PC is offline
