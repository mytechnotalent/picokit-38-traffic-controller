# Parts

## Bill of materials

| Qty | Part | Role |
| --- | --- | --- |
| 1 | Raspberry Pi Pico 2 (with header) | the node |
| 1 | Raspberry Pi Pico Debug Probe | SWD flash and debug, UART0 console |
| 2 | USB A to USB Micro-B cable | one for the Pico, one for the Debug Probe |
| 1 | Full-size breadboard | assembly |
| 1 | Jumper wire set (M-M, M-F, F-F) | assembly |
| 3 | 5 mm LEDs (red, yellow, green) | the intersection |
| 3 | 220 or 330 ohm resistors | LED current limit |
| 1 | Tactile push button | pedestrian request |
| 2 | RYLR998 LoRa module | one on the node, one on the gateway |

## Roles

- Node: Pico 2 plus the intersection LEDs, the button, and the first RYLR998.
- Gateway: the computer with the second RYLR998 on a USB serial adapter.
- Debug Probe: flashing, SWD debugging, and the UART0 console.

## Notes

- The button shorts its pin to ground when pressed; the internal pull-up is on.
- The RYLR998 and the Pico logic both run at 3.3 V.
