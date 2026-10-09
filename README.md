# ESP32 ISP for ATmega32u4

Turn your ESP32 into an **AVR ISP Programmer** implementing the **STK500v1** protocol. This firmware allows you to program target microcontrollers like the **ATmega32U4** (e.g., Arduino Leonardo, Pro Micro) directly via your ESP32 over SPI.

---

## 📌 Features & Specifications

- **Protocol:** STK500v1 over Serial
- **Baudrate:** `19200` baud
- **ISP Clock Frequency:** `10 kHz`
- **Supported Operations:**
  - Reading chip signature
  - Writing Flash memory (page-based, up to 128 bytes per page)
  - Reading Flash memory

---

## ⚠️ Important Limitations & Safety Features

To prevent accidental bricking or corrupting critical chip configurations, the following behavior is programmed into this sketch:

- ❌ **CHIP ERASE is disabled:** The `STK_CHIP_ERASE` command is explicitly rejected by the programmer. Always use `avrdude` with the `-D` flag (disable auto-erase).
- ❌ **No Fuse Programming:** Fuse bits cannot be altered automatically with this firmware.
- ❌ **No Lock-Bit Programming:** Lock-bit modifications are not supported.

---

## 🔌 Wiring / Pinout

Connect your **ESP32** to the **ATmega32U4** (e.g., Arduino Leonardo / Pro Micro / bare chip) as follows:

| ESP32 Pin | Function | ATmega32U4 Pin | Function |
| :--- | :--- | :--- | :--- |
| **GPIO 23** | MOSI | **MOSI** (PB2 / Pin 16 / ICSP Pin 4) | Data In |
| **GPIO 19** | MISO | **MISO** (PB3 / Pin 14 / ICSP Pin 1) | Data Out |
| **GPIO 18** | SCK | **SCK** (PB1 / Pin 15 / ICSP Pin 3) | Serial Clock |
| **GPIO 5** | RESET | **RESET** | Target Reset |
| **3V3** | Power (3.3V) | **VCC** | Power |
| **GND** | Ground | **GND** | Ground |

> **Note on Voltage:** Make sure your target ATmega32U4 is powered at 3.3V or use appropriate logic level converters if the target runs at 5V to protect the ESP32 GPIOs.

---

## 🚀 How to Use

### 1. Flashing the Sketch to the ESP32 (Arduino IDE)

1. Open the `.ino` sketch in the **Arduino IDE**.
2. Select your board and port:
   - **Board:** `Tools` ➔ `Board` ➔ `esp32` ➔ **ESP32 Dev Module**
   - **Port:** Select your ESP32 COM Port (e.g., `COM3`)
3. Click **Upload**.

---

### 2. Flashing the Target via `avrdude`

Once the ESP32 is running the ISP firmware, you can use `avrdude` from your terminal or PowerShell to program the target ATmega32U4.

> **Crucial Note:** Always include the **`-D`** option to disable full chip erase, as `CHIP_ERASE` is intentionally disabled in this programmer implementation.

#### Example Command (PowerShell / Windows):

```powershell
& "C:\Users\<YourUsername>\AppData\Local\Arduino15\packages\arduino\tools\avrdude\8.0.0-arduino1\bin\avrdude.exe" `
  -c stk500v1 `
  -p atmega32u4 `
  -P COM3 `
  -b 19200 `
  -D `
  -U "flash:w:C:\Path\To\Your\Firmware.hex:i"
```

#### Command Line Parameters Explained:
- `-c stk500v1`: Uses the STK500v1 protocol implemented by the ESP32.
- `-p atmega32u4`: Specifies the target chip type.
- `-P COM3`: The serial port where your ESP32 is connected (adjust for your system, e.g., `/dev/ttyUSB0` on Linux/macOS).
- `-b 19200`: Baudrate specified by the sketch for communication.
- `-D`: **Disables chip erase** (required for this firmware).
- `-U flash:w:<filename>:i`: Flashes the specified Intel HEX file to the target memory.
