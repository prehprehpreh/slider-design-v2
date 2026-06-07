# Uno/Nano Controller - Wiring Diagram

## Pin Reference

```
                    ┌─────────────────────────────────────────┐
                    │           ARDUINO UNO/NANO              │
                    │                                         │
    NRF24 CE ───────┤ D8                                      │
    NRF24 CSN ──────┤ D9                                      │
    Button B ───────┤ D10                                     │
                    │                                         │
    NRF24 MOSI ─────┤ D11 (ICSP)                              │
    NRF24 MISO ─────┤ D12 (ICSP)                              │
    NRF24 SCK ──────┤ D13 (ICSP)                              │
                    │                                         │
    OLED SDA ───────┤ A4 (I2C)                                │
    OLED SCL ───────┤ A5 (I2C)                                │
                    │                                         │
    L-Joystick Y ───┤ A1                                      │
    L-Joystick X ───┤ A0                                      │
    R-Joystick Y ───┤ A3                                      │
    R-Joystick X ───┤ A2                                      │
                    │                                         │
    Start Btn ──────┤ D2                                      │
    Button X ───────┤ D3                                      │
    Button Y ───────┤ D4                                      │
    D-pad Left ─────┤ D5                                      │
    D-pad Right ────┤ D6                                      │
    Button A ───────┤ D7                                      │
                    │                                         │
                    │   VIN ────┐                             │
                    │   GND ────┼──┐                          │
                    │   5V ─────┼──┼──┐                       │
                    │   3.3V ───┤  │  │                       │
                    └───────────┼──┼──┼───────────────────────┘
                                │  │  │
                                │  │  │
            ┌───────────────────┘  │  └───────────────┐
            │                      │                  │
            ▼                      ▼                  ▼
     ┌────────────┐        ┌────────────┐    ┌────────────┐
     │ TOGGLE     │        │  GND BUS   │    │  5V BUS    │
     │ SWITCH     │        │            │    │            │
     │ (Battery+) │        │ •────────  │    │ •────────  │
     └────────────┘        └────────────┘    └────────────┘
```

---

## Component Wiring Details

### 1. NRF24L01+ Breakout Adapter (Top-Left Area)

```
         ┌────────────────────────────────┐
         │   NRF24L01+ PA+LNA Breakout    │
         │                                │
         │  ┌──────────────────────────┐  │
         │  │  IRQ  │ CE │ CSN │ MOSI │  │
         │  │   NC  │ D8 │ D9  │  D11 │  │
         │  ├──────────────────────────┤  │
         │  │ MISO  │ SCK│ GND │  VCC │  │
         │  │  D12  │ D13│ GND │  5V  │  │
         │  └──────────────────────────┘  │
         │         8-Pin Header           │
         └────────────────────────────────┘
```

**Connections:**
- VCC → 5V (breakout has 3.3V regulator)
- GND → GND
- MOSI → D11
- MISO → D12  
- SCK → D13
- CE → D8
- CSN → D9
- IRQ → Not connected

---

### 2. OLED SSD1306 128x64 I2C (Top Area)

```
        ┌─────────────────────┐
        │    OLED DISPLAY   │
        │   ┌───────────┐   │
        │   │           │   │
        │   │  128x64   │   │
        │   │   I2C     │   │
        │   │           │   │
        │   └───────────┘   │
        │                   │
        │  VCC GND SCL SDA  │
        │   │   │   │   │   │
        └───┼───┼───┼───┼───┘
            │   │   │   │
            5V  GND A5  A4
```

**Connections:**
- VCC → 5V
- GND → GND
- SCL → A5
- SDA → A4

---

### 3. Left Joystick - HW-504 (Slide Control)

```
           ┌───────────────────┐
           │   HW-504 Module   │
           │                   │
           │        ↑          │
           │     Y-axis        │
           │    (unused)       │
           │        ↓          │
           │                   │
           │ ←  X-axis  →      │
           │   (Slide L/R)     │
           │                   │
           │  ┌─────────────┐  │
           │  │   Push-btn  │  │
           │  │   (unused)  │  │
           │  └─────────────┘  │
           └───────────────────┘
           
           Pinout (from left to right):
           ┌───┬───┬───┬───┬───┐
           │GND│+5V│VRX│VRY│ SW│
           └───┴───┴───┴───┴───┘
            │   │   │   │   │
            │   │   │   │   └── NC (or D2 for joystick button)
            │   │   │   └────── A1 (Y-axis, currently unused)
            │   │   └────────── A0 (X-axis, Slide control)
            │   └────────────── 5V
            └────────────────── GND
```

---

### 4. Right Joystick - HW-504 (Pan/Tilt Control)

```
           ┌───────────────────┐
           │   HW-504 Module   │
           │                   │
           │        ↑          │
           │    Y-axis         │
           │   (Tilt U/D)      │
           │        ↓          │
           │                   │
           │ ←  X-axis  →      │
           │  (Pan L/R)        │
           │                   │
           │  ┌─────────────┐  │
           │  │   Push-btn  │  │
           │  │   (unused)  │  │
           │  └─────────────┘  │
           └───────────────────┘
           
           Pinout:
           ┌───┬───┬───┬───┬───┐
           │GND│+5V│VRX│VRY│ SW│
           └───┴───┴───┴───┴───┘
            │   │   │   │   │
            │   │   │   │   └── NC (or D10 for joystick button)
            │   │   │   └────── A3 (Y-axis, Tilt)
            │   │   └────────── A2 (X-axis, Pan)
            │   └────────────── 5V
            └────────────────── GND
```

---

### 5. Button Layout - Controller Face

```
         ╔═══════════════════════════════════════╗
         ║                                       ║
         ║           ┌─────────┐      OLED       ║
         ║           │ START   │    ┌─────┐      ║
         ║           │   D2    │    │     │      ║
         ║           └─────────┘    │ SSD │      ║
         ║                          │1306 │      ║
         ║   ┌─────┐                │12864│      ║
         ║   │  ←  │                └─────┘      ║
         ║   │ D5  │                           ║
         ║   ├─────┤                             ║
         ║   │  →  │                             ║
         ║   │ D6  │                             ║
         ║   └─────┘                             ║
         ║                                       ║
         ║   D-PAD (Left)        BUTTONS (Right) ║
         ║   (2-way)                               ║
         ║                              ┌───┐    ║
         ║                         ┌────┤ Y │    ║
         ║                         │ D4 └───┘    ║
         ║                    ┌────┴────┐        ║
         ║                   ┌┤   X     ├┐       ║
         ║                   ├┤  D3     ├┤       ║
         ║                   └┤    ┌────┼┘       ║
         ║                    └───┬┘ B  │        ║
         ║                        │ D10 │        ║
         ║                   ┌────┴────┘        ║
         ║                   │    A             ║
         ║                   │   D7             ║
         ║                   └────────          ║
         ║                                       ║
         ╚═══════════════════════════════════════╝
```

**Button Wiring (ALL buttons):**
- One side of button → Arduino Pin
- Other side of button → GND
- Code uses `INPUT_PULLUP`, so LOW = pressed

**Button Functions in Normal Mode:**
- **D-pad Left (D5):** Recall Preset A
- **D-pad Right (D6):** Recall Preset B
- **A (D7):** Save Preset A
- **B (D10):** Save Preset B
- **X (D3):** Loop ON
- **Y (D4):** Loop OFF
- **Start (D2):** Enter/Exit Menu

---

### 6. Power Switch

```
    Battery (+) ──┬──► (Toggle Switch Center Pin)
                  │
                  │   ┌───────────────┐
                  └──►│  Toggle Switch │
                      │   (SPDT)       │
                      │   ┌──┬──┬──┐   │
                      └───┤NC│ C │NO├───┘
                          └──┴──┴──┘
                               │
                               ▼
                         To Arduino
                            VIN
                               
    Battery (-) ─────────► To Arduino
                              GND
```

**Simple Inline Wiring:**
- Cut positive battery wire
- Connect one end to switch center pin
- Connect other end to one outer pin
- When switch is flipped that direction, circuit completes

---

## Breadboard Layout - Visual Guide

```
    ┌─────────────────────────────────────────────────────────────────────────┐
    │                          BREADBOARD TOP VIEW                            │
    │                                                                         │
    │  ┌─────────────────────────────────────────────────────────────────┐   │
    │  │                         POWER RAILS                              │   │
    │  │  ┌───────────────┐    ┌───────────────┐    ┌───────────────┐  │   │
    │  │  │ RED (5V)      │    │ BLU (3.3V)    │    │ BLK (GND)     │  │   │
    │  │  │ █████████████ │    │ (unused)      │    │ █████████████ │  │   │
    │  │  └───────────────┘    └───────────────┘    └───────────────┘  │   │
    │  └─────────────────────────────────────────────────────────────────┘   │
    │                                                                         │
    │  ┌─────────────────────────────────────────────────────────────────┐   │
    │  │  AREA 1: NRF24 (Top Left)                                        │   │
    │  │                                                                  │   │
    │  │        ┌────────────────────────┐                               │   │
    │  │        │   NRF24 Breakout       │                               │   │
    │  │        │                        │                               │   │
    │  │        │  VCC ─────► RED rail  │                               │   │
    │  │        │  GND ─────► BLK rail   │                               │   │
    │  │        │  D11 ─────► Blue wire  │                               │   │
    │  │        │  D12 ─────► Yel wire   │                               │   │
    │  │        │  D13 ─────► Wht wire   │                               │   │
    │  │        │  D8  ─────► Org wire   │                               │   │
    │  │        │  D9  ─────► Ppl wire   │                               │   │
    │  │        └────────────────────────┘                               │   │
    │  └─────────────────────────────────────────────────────────────────┘   │
    │                                                                         │
    │  ┌─────────────────────────────────────────────────────────────────┐   │
    │  │  AREA 2: OLED (Top Center)                                       │   │
    │  │                                                                  │   │
    │  │        ┌────────────────────────┐                               │   │
    │  │        │    OLED 128x64       │                               │   │
    │  │        │                      │                               │   │
    │  │        │  VCC ─────► RED rail │                               │   │
    │  │        │  GND ─────► BLK rail  │                               │   │
    │  │        │  SCL ─────► A5        │                               │   │
    │  │        │  SDA ─────► A4        │                               │   │
    │  │        └────────────────────────┘                               │   │
    │  └─────────────────────────────────────────────────────────────────┘   │
    │                                                                         │
    │  ┌─────────────────────────────────────────────────────────────────┐   │
    │  │  AREA 3: JOYSTICKS (Middle)                                      │   │
    │  │                                                                  │   │
    │  │   Left Joystick              Right Joystick                     │   │
    │  │   ┌────────────┐             ┌────────────┐                   │   │
    │  │   │   ──────   │             │   ──────   │                   │   │
    │  │   │  │    │   │             │  │    │   │                   │   │
    │  │   │   ──────   │             │   ──────   │                   │   │
    │  │   └────────────┘             └────────────┘                   │   │
    │  │      │  │  │  │                │  │  │  │                     │   │
    │  │      │  │  │  │                │  │  │  │                     │   │
    │  │     GND 5V A0 A1              GND 5V A2 A3                    │   │
    │  │      │  │  │  │                │  │  │  │                     │   │
    │  │      │  │  │  │                │  │  │  │                     │   │
    │  │      └──┴──┴──┘                └──┴──┴──┘                     │   │
    │  │         │                        │                            │   │
    │  │         └───────► Both share RED and BLK rails                │   │
    │  └─────────────────────────────────────────────────────────────────┘   │
    │                                                                         │
    │  ┌─────────────────────────────────────────────────────────────────┐   │
    │  │  AREA 4: BUTTONS (Bottom) - Left Side = D-Pad (2-way)           │   │
    │  │                                                                  │   │
    │  │                                                                  │   │
    │  │           ┌───┐    ┌───┐                                         │   │
    │  │           │ ← │    │ → │                                         │   │
    │  │       D5  └───┘    └───┘  D6                                   │   │
    │  │      (Recall A)   (Recall B)                                     │   │
    │  │                                                                  │   │
    │  │  Center: ┌─────────┐  (Start/Menu)                             │   │
    │  │          │   ●     │  D2                                        │   │
    │  │          └─────────┘                                             │   │
    │  │                                                                  │   │
    │  │  Right Side: ┌───┐ ┌───┐                                       │   │
    │  │              │ Y │ │ X │  D4, D3                               │   │
    │  │              └───┘ └───┘                                       │   │
    │  │              ┌───┐ ┌───┐                                       │   │
    │  │              │ B │ │ A │  D10, D7                              │   │
    │  │              └───┘ └───┘                                       │   │
    │  │                                                                  │   │
    │  │  ALL buttons: One pin to Arduino, other pin to GND             │   │
    │  └─────────────────────────────────────────────────────────────────┘   │
    │                                                                         │
    │  ┌─────────────────────────────────────────────────────────────────┐   │
    │  │  AREA 5: ARDUINO (Right Edge)                                    │   │
    │  │                                                                  │   │
    │  │   ┌──────────────────────────────────────┐                      │   │
    │  │   │          ARDUINO UNO/NANO            │                      │   │
    │  │   │                                      │                      │   │
    │  │   │  D0  D1  D2  D3  D4  D5  D6  D7  D8  │                      │   │
    │  │   │  NC  NC  ○   ○   ○   ○   ○   ○   ○   │  ○ = used            │   │
    │  │   │                                      │                      │   │
    │  │   │  D9  D10 D11 D12 D13 GND AREF SDA  │                      │   │
    │  │   │  ○   ○   ○   ○   ○   BLK NC  A4    │                      │   │
    │  │   │                                      │                      │   │
    │  │   │  SCL ─► A5                           │                      │   │
    │  │   │  ICSP header (for NRF24 SPI)          │                      │   │
    │  │   │                                      │                      │   │
    │  │   │  PWR: VIN GND 5V  3.3V              │                      │   │
    │  │   │       │   │   │   NC                │                      │   │
    │  │   │       │   │   │                     │                      │   │
    │  │   │       └───┴───┘                     │                      │   │
    │  │   │           │                        │                      │   │
    │  │   │           └──► To Power Switch     │                      │   │
    │  │   └──────────────────────────────────────┘                      │   │
    │  └─────────────────────────────────────────────────────────────────┘   │
    │                                                                         │
    └─────────────────────────────────────────────────────────────────────────┘
```

---

## Wire Color Code Suggestion

| Color | Use | Examples |
|-------|-----|----------|
| **Red** | 5V Power | VCC, +5V pins |
| **Black** | Ground | GND pins |
| **White** | Analog signals | Joystick X/Y |
| **Blue** | Digital/SPI | D11, D12, D13 |
| **Green** | Control signals | CE, CSN, Buttons |
| **Yellow** | I2C | SDA, SCL |
| **Orange** | Special functions | D8, D9 |
| **Purple** | Unused/Extra | — |

---

## Quick Reference - All Connections

| Component | Pin/Function | Arduino Pin | Notes |
|-----------|--------------|-------------|-------|
| **NRF24** | VCC | 5V | Has regulator |
| | GND | GND | |
| | CE | D8 | |
| | CSN | D9 | |
| | MOSI | D11 | SPI |
| | MISO | D12 | SPI |
| | SCK | D13 | SPI |
| **OLED** | VCC | 5V or 3.3V | Check module |
| | GND | GND | |
| | SCL | A5 | I2C |
| | SDA | A4 | I2C |
| **L-Joystick** | +5V | 5V | |
| | GND | GND | |
| | VRX (X) | A0 | Slide |
| | VRY (Y) | A1 | Unused currently |
| **R-Joystick** | +5V | 5V | |
| | GND | GND | |
| | VRX (X) | A2 | Pan |
| | VRY (Y) | A3 | Tilt |
| **Buttons** | All one side | Pins D2, D3, D4, D5, D6, D7, D10 | Pullup enabled |
| | All other side | GND | Common ground |
| **Power** | Switch → VIN | VIN | 7-12V input |
| | Battery - | GND | Common ground |

---

## Uno vs Nano Differences

| Feature | Uno | Nano |
|---------|-----|------|
| **Button X (D3)** | ✅ Available | ✅ Available |
| **Button Y (D4)** | ✅ Available | ✅ Available |
| **Breadboard** | Headers pre-installed | Solder or use adapter |
| **USB** | Standard B | Mini-B or Micro |
| **Size** | Larger | Smaller |

All 7 buttons work identically on both Uno and Nano using digital pins D2-D7 and D10.
