# ArduinoISP-for-ATmega32u4

Using an Arduino as an ISP (In-System Programming) device means that a standard Arduino board (such as an Arduino Uno or Nano) is repurposed to serve as a programmer for other microcontrollers (e.g., ATmega chips or ATtiny devices).

## Usage

### 1. Arduino IDE

Make sure you have selected following:

```
Tools --> Board --> esp32 --> ESP32 Dev Module
Port --> COM3
```

Then you click `Upload`.

### 2. Powershell

Please run:

```
& "C:\Users\Admin\AppData\Local\Arduino15\packages\arduino\tools\avrdude\8.0.0-arduino1\bin\avrdude.exe" -c stk500v1 -p atmega32u4 -P COM3 -b 19200 -D -U "flash:w:C:\Users\Admin\AppData\Local\Arduino15\packages\arduino\hardware\avr\1.8.7\bootloaders\caterina\Caterina-Leonardo.hex:i"
```

Mabye you have to replace the `username` `Admin` with the `username` of your computer.
