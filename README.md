![picokit-38-traffic-controller](https://raw.githubusercontent.com/mytechnotalent/picokit-38-traffic-controller/main/picokit-38-traffic-controller.png)

<br>

## FREE Reverse Engineering Self-Study Course [HERE](https://github.com/mytechnotalent/reverse-engineering)
## FREE Embedded Hacking Course [HERE](https://github.com/mytechnotalent/Embedded-Hacking)

<br>

# PICOKIT-38 TRAFFIC CONTROLLER

### Timed Intersection Phases, a Pedestrian Request, and an Authenticated Heartbeat
#### Lesson 38 of the Picokit Series

<br>

***
**LEGAL DISCLAIMER:**
The information, tools, and code provided in this repository and course are strictly for educational, research, and defensive purposes only.

You are explicitly prohibited from using any materials contained herein to access, test, modify, or exploit any device, network, or system that you do not own 100% or for which you do not have explicit, documented, and legally binding authorization to interact with.

By using this repository and course, you acknowledge and agree that:

1. Any illegal, unauthorized, or malicious use of this information is solely your responsibility.
2. The author(s) and contributor(s) of this repository and course shall not be held liable for any damages, legal repercussions, criminal charges, or unauthorized actions resulting from the use, misuse, or abuse of the contents herein.
3. You will comply with all applicable local, state, national, and international laws regarding cybersecurity and computer fraud.

**IF YOU DO NOT AGREE WITH THESE TERMS, DO NOT USE THIS REPOSITORY AND COURSE.**
***

<br>
<br>

## Overview

The thirty-eighth Picokit lesson. The node runs a small intersection state
machine on the red, yellow, and green lamps with timed phases. A pedestrian
button latches a request that inserts an extra red walk window into the cycle,
and every authenticated heartbeat reports the current phase.

<br>

## What it teaches

- A timed traffic phase state machine: red, green, yellow.
- Latching a pedestrian request from a debounced button.
- Inserting a pedestrian phase ahead of the next red phase.
- Reporting the current phase in the authenticated heartbeat.

<br>

## Hardware

| Peripheral | Pico 2 pin | Role |
| --- | --- | --- |
| Red / Yellow / Green | GP16 / GP17 / GP18 | the intersection |
| Button | GP15 | pedestrian request |
| Onboard LED | GP25 | heartbeat, one blink per transmit |
| RYLR998 | GP8 TX / GP9 RX | LoRa heartbeat |
| Debug Probe | SWCLK/SWDIO/GND, GP0/GP1 | SWD and the console |

<br>

## How it works

The node runs `monitor_step` in a loop. It advances the traffic phase on its
timed interval (red 3 s, green 4 s, yellow 2 s) and samples the pedestrian
button. A pending request turns the yellow-to-red transition into a 5 s
pedestrian phase. Every 5 seconds the heartbeat body
`{"n":38,"s":<seq>,"p":<phase>}` is sealed with the field key and sent over
LoRa.

<br>

## Build and flash

```bash
cd firmware
cmake -S . -B build -G Ninja -DPICO_BOARD=pico2 -DPICO_PLATFORM=rp2350-arm-s
cmake --build build
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
  -c "program build/picokit_38_traffic_controller.elf verify reset exit"
```

<br>

## Watch the node

Open the console at 115200 and reset:

```text
BOOT
=== PICOKIT-38 TRAFFIC CONTROLLER // TIMED PHASES + PED REQUEST ===
PHASE 1
PED REQUEST
PHASE 2
PHASE 3
RX from 0x0001, N bytes
```

<br>

## The gateway

```bash
cd gateway
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python3 listen.py --port /dev/cu.usbserial-A50285BI --hub 0001 --network 18 --db gateway.db
```

It prints `OK node=38 rssi=...` per authenticated heartbeat. The terminal
dashboard `python3 tui.py --db gateway.db` and the web dashboard
`python3 web/app.py --db gateway.db` show the same rows.

<br>

## Verify

```bash
python3 .opencode/skill/embedded-c-standard/audit_c_standard.py
python3 .opencode/skill/embedded-python-standard/audit_python_standard.py
python3 .opencode/skill/iot-readme-standard/validate_readme.py
python3 .opencode/skill/iot-banner-standard/validate_banner.py
python3 scripts/run_tests.py
python3 scripts/check_coverage.py
```

<br>

# Next
[picokit-39-authenticated-telemetry](https://github.com/mytechnotalent/picokit-39-authenticated-telemetry)

<br>

# License
[MIT License](https://github.com/mytechnotalent/picokit-38-traffic-controller/blob/main/LICENSE)
