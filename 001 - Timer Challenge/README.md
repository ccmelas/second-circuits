# Timer Challenge

## What you need

- ESP32 dev board
- TM1637 4-digit 7-segment display module
- Push button
- Passive buzzer
- 2 breadboards and jumper wires

## Wiring

| Part | ESP32 |
| --- | --- |
| Display CLK | GPIO 5 |
| Display DIO | GPIO 18 |
| Display VCC / GND | 3V3 / GND |
| Buzzer | GPIO 19 (other leg to GND) |
| Button | GPIO 23 (other leg to GND, uses the internal pull-up) |

## Setup

1. In the Arduino IDE, add the ESP32 boards under Tools > Board > Boards Manager ("esp32" by Espressif Systems).
2. Install the "TM1637" library by Avishay Orpaz from the Library Manager.
3. Pick your ESP32 board and port, then upload.
