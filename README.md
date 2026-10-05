# ArduinoISP-for-ATmega32u4

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
