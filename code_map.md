# PTZ Slider — Code Map & System Analysis

> Generated after full analysis of the active Arduino Mega firmware and Python Windows controller.

---

## 1. System Architecture

```
PC Controller (Python/Kivy + Xbox gamepad)
    │ UDP send → Mega :44158
    │ UDP recv ← Mega :44159
    │ JSON persistence (~\Documents\Controller2\motor_params.json)
    ▼
Arduino Mega 2560
    ├── WIZ5100 Ethernet Shield
    ├── 128x64 I2C OLED (SSD1306 @ 0x3C)
    └── 4x stepper drivers (AccelStepper + MultiStepper)

Planned:
Arduino Nano + NRF24L01+PA+LNA ←→ Mega (needs NRF24 input path)
```

---

## 2. Hardware Pinout (Mega)

| Motor | Role | Step | Dir | MS1 | MS2 | TMC EN |
|-------|------|------|-----|-----|-----|--------|
| 1 | **Slide** | 3 | 2 | 10 | 11 | — |
| 2 | **Pan** | 31 | 30 | 32 | 33 | — |
| 3 | **Tilt** | 45 | 44 | 40 | 41 | — |
| 4 | **Zoom** | 48 | 49 | 37 | 35 | 50 |

- All MS pins set `HIGH` in `setup()` (likely 1/16 microstep).
- **Bug:** `ms1Pin2` is never set `HIGH` in `setup()`; line 81 writes `ms2Pin2` twice.

---

## 3. Input → Motor Mapping

| Source | Python Var | Arduino Var | Motor | Role |
|--------|------------|-------------|-------|------|
| Left Stick X | `x_axis_left` | `x_axis_value_left` | Stepper1 | Slide |
| Left Stick Y | `y_axis_left` | — | — | **Unused** |
| Right Stick X | `x_axis_right` | `x_axis_value_right` | Stepper2 | Pan |
| Right Stick Y | `y_axis_right` | `y_axis_value` | Stepper3 | Tilt |
| Left Trigger | `trigger_left_mapped` | `trigger_left` | Stepper4 | Zoom (+) |
| Right Trigger | `trigger_right_mapped` | `trigger_right` | Stepper4 | Zoom (-) |

- Triggers are combined in Arduino: `target = (trigger_right + trigger_left) * 100`.
- Right trigger is stored negative in Python (`-(event.state / 255.0)`).

---

## 4. UDP Protocol

### PC → Mega (Commands)

| Command | Payload | Handler | Notes |
|---------|---------|---------|-------|
| `SET_JOYSTICK` | `xL,xR,yR,tL,tR` (5 floats) | `handleUDPRequests()` | Real-time analog control |
| `SEND_CURRENT_POS` | — | `sendCurrentMotorPositions()` | Triggers reply with positions |
| `SAVE_POS` | — | `saveMotorPositions()` | **Dead code** — no Python caller |
| `RECALL_POS` | — | `recallMotorPositions()` | **Dead code** — no Python caller |
| `SAVE_A` | — | `savePresetA()` | Save current pos to Preset A |
| `SAVE_B` | — | `savePresetB()` | Save current pos to Preset B |
| `RECALL_A` | — | `recallPresetA()` | Blocking syncMove to Preset A |
| `RECALL_B` | — | `recallPresetB()` | Blocking syncMove to Preset B |
| `UPDATE_LOOP` | `true` or `false` | Sets `loopPresets` | Enables/disables loop flag on Mega |
| `INCREASE_SPEED` | — | `msSpeed += 200` | Bumps slide max speed |
| `DECREASE_SPEED` | — | `msSpeed -= 200` | Drops slide max speed |

### Mega → PC (Replies)

| Message | Triggered By | Python Handler |
|---------|--------------|----------------|
| `CURRENT_POS:m1,m2,m3,m4` | `SEND_CURRENT_POS` | `update_motor_positions()` |
| `PRESET_POS_A:m1,m2,m3,m4,PRESET_POS_B:m1,m2,m3,m4` | `SAVE_A` or `SAVE_B` | `update_preset_positions()` |
| `MSSPEED:val` | Speed change / `SET_MSSPEED` | Updates label + saves JSON |
| `PRESET_A_DONE` | End of `recallPresetA()` | If `loop_state`, sends `RECALL_B` |
| `PRESET_B_DONE` | End of `recallPresetB()` | If `loop_state`, sends `RECALL_A` |

### Setup Handshake (Mega Boot)

1. Mega waits for **any** UDP packet to learn controller IP/port.
2. Mega sends `GET_CURRENT_POS`.
3. Python loads `motor_params.json` and replies with:
   - `SET_POS m1,m2,m3,m4`
   - `SET_PRESET_A m1,m2,m3,m4`
   - `SET_PRESET_B m1,m2,m3,m4`
   - `SET_MSSPEED val`
4. Mega sets stepper current positions from saved values.
5. OLED shows `Data Loaded` or `Data Failed`.

---

## 5. Mega Firmware — Key Behaviors

### Main Loop (`loop()`)

```cpp
loop() {
    handleUDPRequests();          // Parse UDP, may BLOCK during preset recall
    if (5ms elapsed) {
        controlStepperMotors();  // Calculate target speeds, apply accel curves
    }
    stepper1.runSpeed();          // Execute steps
    stepper2.runSpeed();
    stepper3.runSpeed();
    stepper4.runSpeed();
}
```

### Motor Control (`controlStepperMotors()`)

- **Slide (Stepper1):** Custom asymmetric acceleration.
  - If speeding up: `current += min(10, diff * 0.002 + 0.02)`
  - If slowing down: `current -= min(10, -diff * 0.002 + 0.02)`
- **Pan (Stepper2), Tilt (Stepper3), Zoom (Stepper4):** Simple linear ramping.
  - `accel = 0.6` (pan/tilt), `0.75` (zoom)
  - `decel = 0.6` (pan/tilt), `0.75` (zoom)
- **Deadzone:** 10 (after scaling joystick *100), so ~10% deadzone.
- `max_speed_left` (slide) is dynamic, default 2000.
- Pan/tilt/zoom max speeds are hardcoded `const`: 100, 100, 75.

### syncMove (Blocking Preset Recall)

- Called by `recallPresetA()` and `recallPresetB()`.
- If already within `tolerance` (1000 steps) on **all 4 axes**, immediately sends `PRESET_X_DONE`.
- Otherwise:
  - Sets individual `moveTo()` targets.
  - Computes proportional max speed per motor: `speed = maxSpeed * (motorDistance / longestDistance)`.
  - **Blocking while-loop** calls `stepper.run()` until all motors arrive.
  - After arrival, resets all max speeds to `max_speed_left` and accel to 400.
  - Sends `PRESET_X_DONE`.
- **Critical:** During `syncMove`, `handleUDPRequests()` is NOT called. The Mega is unresponsive to new UDP packets and joystick updates.

### MultiStepper

- Added to all 4 steppers but only used by `recallMotorPositions()` (`SAVE_POS`/`RECALL_POS`).
- These commands have no callers in the Python app, so `MultiStepper` is effectively **unused**.

---

## 6. Python Controller — Key Behaviors

### Threads

| Thread | Purpose |
|--------|---------|
| Main | Kivy UI event loop |
| `joystick_input_thread` | Blocks on `inputs.get_gamepad()`, parses ABS/Key events, updates globals |
| `udp_receive_thread` | Blocks on `sock.recvfrom()`, parses Mega replies, orchestrates preset loop |

### Kivy Clock Intervals

| Function | Interval | Purpose |
|----------|----------|---------|
| `send_joystick_data()` | 0.1s | Sends `SET_JOYSTICK` + `SEND_CURRENT_POS` |
| `virtualloop()` | 0.1s | If enabled, computes slider direction and sends `SET_JOYSTICK` |
| `save_params_periodically()` | 1.0s | Saves JSON if 5s elapsed since last save |

### Gamepad Mapping

| Input | Event Code | Action |
|-------|------------|--------|
| Left Stick X | `ABS_X` | `x_axis_left` |
| Left Stick Y | `ABS_Y` | `y_axis_left` (**unused**) |
| Right Stick X | `ABS_RX` | `x_axis_right` |
| Right Stick Y | `ABS_RY` | `y_axis_right` |
| Left Trigger | `ABS_Z` | `trigger_left_mapped` |
| Right Trigger | `ABS_RZ` | `trigger_right_mapped` (negative) |
| D-Pad Left | `ABS_HAT0X = -1` | `RECALL_A` |
| D-Pad Right | `ABS_HAT0X = +1` | `RECALL_B` |
| D-Pad Up | `ABS_HAT0Y = -1` | Enable `virtualloop` |
| D-Pad Down | `ABS_HAT0Y = +1` | Disable `virtualloop` |
| A (South) | `BTN_SOUTH` | `UPDATE_LOOP false` + disable `loop_state` |
| Y (North) | `BTN_NORTH` | `UPDATE_LOOP true` + enable `loop_state` |
| X (West) | `BTN_WEST` | `SAVE_A` |
| B (East) | `BTN_EAST` | `SAVE_B` |
| RB | `BTN_TR` | `INCREASE_SPEED` |
| LB | `BTN_TL` | `DECREASE_SPEED` |

### Two Different Loop Modes

1. **`loop_state` (Preset Sync Loop):**
   - Enabled by `UPDATE_LOOP true` / disabled by `UPDATE_LOOP false`.
   - Python orchestrates A↔B by listening for `PRESET_A_DONE` / `PRESET_B_DONE` from Mega.
   - When `PRESET_A_DONE` arrives, Python waits 0.5s then sends `RECALL_B`.
   - When `PRESET_B_DONE` arrives, Python waits 0.5s then sends `RECALL_A`.
   - Mega handles the actual synchronized motion via `syncMove`.
   - **Result:** All 4 motors move proportionally between full preset positions.

2. **`virtualloop_enabled` (Software Position Loop):**
   - Python reads `current_pos1_label.text` to know slider position.
   - Computes `direction = +1` or `-1` to reach next target (`preset_a_slide` or `preset_b_slide`).
   - Sends `SET_JOYSTICK direction,xR,yR,tL,tR` every 0.1s.
   - Only drives Stepper1 (slide). Pan/tilt/zoom remain manually controlled.
   - Toggles target when within `tolerance` (600 steps).
   - **Result:** Slider ping-pongs between A and B using the normal joystick control path.

### JSON Persistence

**File:** `~/Documents/Controller2/motor_params.json`

```json
{
    "current_positions": { "motor1": 0, "motor2": 0, "motor3": 0, "motor4": 0 },
    "preset_a": { "motor1": 0, "motor2": 0, "motor3": 0, "motor4": 0 },
    "preset_b": { "motor1": 0, "motor2": 0, "motor3": 0, "motor4": 0 },
    "ms_speed": 800,
    "udp_ip": "192.168.5.60",
    "invert_left": false,
    "invert_right": false,
    "invert_ry": false,
    "sensitivity_right": 1.0
}
```

- Loaded at app startup; defaults used if missing.
- `current_positions` and presets updated from Mega replies.
- `sensitivity_right` scales `x_axis_right` and `y_axis_right` in Python.
- Saved automatically every 5 seconds.

---

## 7. Known Issues / Observations

- **`ms1Pin2` never set HIGH** in `setup()` (typo on line 81).
- **`findBiggestNumFloatArray`** takes unused `String option` parameter.
- **`y_axis_left` parsed but never sent** to Arduino (left stick Y is dead).
- **`SAVE_POS` / `RECALL_POS`** have Arduino handlers but no Python callers.
- **`multiStepper`** configured but unused for preset recalls.
- **Mega cannot distinguish multiple UDP clients.** `sendUDPMessage()` replies to `Udp.remoteIP()` / `Udp.remotePort()`. If PC and Nano both send, replies may go to the wrong device.
- **`virtualloop`** parses label text to get position rather than using a dedicated state variable.
- **Pan/tilt/zoom max speeds are hardcoded** `const` on Mega. Changing them requires firmware modification.

---

## 8. Implications for Nano+NRF24 Controller

To add the Nano remote, the Mega firmware will likely need the following changes:

### Dual-Path Input
The Mega must accept commands from both Ethernet (PC) and NRF24 (Nano). The archived code in `ORIGINAL FILES DO NOT EDIT/` had a dual-path structure that can be used as reference.

### Reply Routing
`sendUDPMessage()` currently replies to the last UDP sender. With two controllers, the Mega needs to either:
- Broadcast state updates to both paths, or
- Track which controller sent the request and reply accordingly, or
- Make the Nano a "fire-and-forget" sender that does not wait for replies.

### NRF24 Payload Limit (32 bytes)
The current ASCII protocol (`SET_JOYSTICK 0.123,0.456, ...`) exceeds 32 bytes easily. A **compact binary protocol** is strongly recommended for the Nano↔Mega radio link.

### Persistence Without PC
If the PC is offline, the Nano must provide initial positions/presets on Mega boot, or the Mega must store them internally (EEPROM / SD card).

### Nano Menu System
The Nano OLED will display:
- Slider speed (`msSpeed`)
- Joystick sensitivity
- Invert controls toggles
- A **Start button** to exit the menu and save changes.

These settings must either be applied on the Nano before transmission or communicated to the Mega.

### syncMove Blocking
During preset recall, the Mega is unresponsive for the duration of the move. The Nano should expect this, or the firmware should be refactored to make `syncMove` non-blocking (state-machine driven in `loop()`).

### Loop Orchestration
Currently `loop_state` (A↔B preset sync loop) is managed entirely by Python. If the Nano replaces the PC, it must either:
- Replicate the Python loop logic on the Nano, or
- Teach the Mega to loop autonomously (store a `loopActive` flag and self-trigger recalls).

### State Conflicts
If both PC and Nano are online simultaneously, they may send conflicting `SET_JOYSTICK` values or commands. The firmware currently processes every packet in arrival order with no priority or arbitration.
