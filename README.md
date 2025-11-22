┌─────────────────────────────────────────────────────────────┐
│                         ESP32 DevKit                         │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ANALOG INPUTS (Tank Sensors — 0–3V)                         │
│  ┌──────────────────────────────────────┐                    │
│  │ GPIO33 → Starboard Fuel Tank         │                    │
│  │ GPIO34 → Port Fuel Tank              │                    │
│  │ GPIO39 → Black Water Tank            │                    │
│  │ GPIO32 → Port Aft Fresh Water        │                    │
│  │ GPIO35 → Starboard Fresh Water       │                    │
│  │ GPIO36 → Port Forward Fresh Water    │                    │
│  └──────────────────────────────────────┘                    │
│                                                               │
│  TOUCH PADS (Capacitive Input)                               │
│  ┌──────────────────────────────────────┐                    │
│  │ GPIO12 → Touch Pad 1 (Buzzer Ctrl)   │                    │
│  │ GPIO4  → Touch Pad 2 (Display Ctrl)  │                    │
│  └──────────────────────────────────────┘                    │
│                                                               │
│  BUZZER & LED OUTPUT                                          │
│  ┌──────────────────────────────────────┐                    │
│  │ GPIO19 → Buzzer (Active LOW)         │                    │
│  │ GPIO2  → Status LED                  │                    │
│  └──────────────────────────────────────┘                    │
│                                                               │
│  SPI / DISPLAY (E-Paper)                                      │
│  ┌──────────────────────────────────────┐                    │
│  │ (Configured in epaper.cpp)           │                    │
│  │ SCLK, MOSI, CS, DC, RST, BUSY        │                    │
│  └──────────────────────────────────────┘                    │
│                                                               │
└─────────────────────────────────────────────────────────────┘

Quick Reference — Pin Summary
Component	GPIO	Function
Buzzer Pad	12	Touch input (buzzer enable/disable)
Display Pad	4	Touch input (display update)
Buzzer Output	19	Control buzzer (active LOW)
Status LED	2	Restart feedback indicator
Stbd Fuel Sensor	33	Analog input (0–3V)
Port Fuel Sensor	34	Analog input (0–3V)
Black Water Sensor	39	Analog input (0–3V)
Port Aft Fresh Sensor	32	Analog input (0–3V)
Stbd Fresh Sensor	35	Analog input (0–3V)
Port Fwd Fresh Sensor	36	Analog input (0–3V)
Device Functionality Overview
What It Does
Reads six tank level sensors (0–3V analogue inputs) continuously.
Calibrates each sensor reading using configured multiplier + offset values.
Sends calibrated tank levels to Signal K (ratio units: 0.0 = empty, 1.0 = full).
Displays six bar graphs on E-paper showing tank levels as percentage bars with icons.
Monitors black water tank level and sounds buzzer alarm if level exceeds 90% for 10+ seconds (if buzzer function is enabled).
Responds to two touch pads for user control (display update, buzzer enable/disable, device reset).
Display — What You See

┌─────────────────────────────────────────┐
│ Status Line: IP / Time / Version / WiFi │  ← Boot info, then normal status
├─────────────────────────────────────────┤
│ [■■■■■  ] Stbd Fuel      [●] Buzzer ON  │  ← Bars show tank level %
│ [■■■   ] Port Fuel       (or OFF)       │
│ [■     ] Black Water                    │
│ [■■■■■■] Port Aft Fresh Water           │
│ [■■■   ] Stbd Fresh Water               │
│ [■■    ] Port Fwd Fresh Water           │
└─────────────────────────────────────────┘

Touch Pad Controls
Touch Pad 1 — Buzzer Enable/Disable (GPIO12)
Action: Short tap (1–2 seconds)
Effect:
Toggles buzzer function ON ↔ OFF
Enabling: brief confirmation beep (short, non-blocking)
Disabling: immediate guaranteed silence
Icon on display updates to show current state (ON/OFF)
Result on display: Buzzer icon changes immediately after tap
Touch Pad 2 — Display Update (GPIO4)
Action: Short tap (1–2 seconds)
Effect:
Forces immediate display refresh (partial update)
Shows current tank levels without waiting for 60-second timer
Rate-limited to minimum ~500ms between updates to avoid corruption
Result on display: Bars and status refresh instantly
Both Pads Held Together — Device Reset
Action: Press and hold both pads simultaneously for ~5 seconds
Feedback sequence:
Immediate (< 1 sec): one-time buzzer beep (audio confirmation)
~2 sec: LED turns ON (visual indicator restart is scheduled)
~5 sec: device restarts (ESP.restart() called)
Cancel: Release both pads before 5 seconds to cancel restart
Buzzer Behaviour — When It Sounds
Alarm Condition
The buzzer will sound only if BOTH conditions are true:

Buzzer function is ENABLED (user has toggled it ON via touch pad 1)
Black water tank level > 90% continuously for 10+ seconds
Timing Details
Startup suppression: Alarm checks are paused for 30 seconds after boot to allow sensors to stabilize
Alarm delay: Black water must stay > 90% for 10+ seconds before buzzer sounds (prevents false triggers on brief spikes)
Manual beep: When you enable the buzzer, you get a short ~200ms confirmation beep
How to Stop the Alarm
Option A: Black water level drops below 90% → buzzer stops automatically
Option B: Disable buzzer via touch pad 1 → buzzer stops immediately (stays off until re-enabled)
Buzzer Hardware Detail
Active LOW: GPIO19 is pulled LOW (0V) to activate buzzer; HIGH (3.3V) to silence
SensESP manages the GPIO state; you control it via the touch pad
How to Reset the Device
Step-by-Step Reset Procedure
Locate the two touch pads (GPIO12 and GPIO4, marked as Pad 1 and Pad 2)
Press and hold both pads simultaneously with your fingers
Hold for ~5 seconds:
After ~1 second: you hear a single buzzer beep (confirmation)
After ~2 seconds: LED on GPIO2 turns ON (visual indicator)
Continue holding until the device restarts
Result: Device performs ESP.restart() and reboots to main menu
Cancel a Pending Restart
Release both pads before the 5-second countdown completes → restart is cancelled, LED turns OFF
Normal Boot Sequence
Power on → Serial output shows version, WiFi SSID, IP address
Wait ~5 seconds → E-paper displays boot status (IP, version, timestamp)
Wait ~30 seconds → Alarm checks enabled (sensors have stabilized)
Display shows tank levels → System ready, bar graphs update every 60 seconds
Touch pads are active from boot
Troubleshooting Quick Guide
Problem	Check
Buzzer sounds on boot	Restart device; check GPIO19 wiring
Touch pads not responding	Confirm GPIO12 / GPIO4 are capacitive sensors
Display corrupted/garbled	Avoid rapid repeated updates; check SPI wiring
Readings don't match Signal K	Verify calibration multiplier/offset values
WiFi / IP not shown at boot	Ensure WiFi is configured in SensESP web UI
Reset doesn't work	Hold both pads for full 5 seconds; try again
Sensor Calibration Reference
Each tank sensor uses a linear calibration. To adjust readings in the web UI:

Go to SensESP config page → select tank → edit Multiplier and Offset
Changes take effect immediately and are saved to SPIFFS



Contour Tank System upgrade Contour tank system is a SENSESP instantiaton which takes readings from 6 analogue inputs: 3 x fresh water tanks 2 x fuel tanks 1 x black water (sewage) tank.

The tanks are read by barometic pressure sensing devices which output 0-5vDC.

The SENSESP framework outputs these values to the SENSESP server which is connected on a WIFI lan. This system works extremely well.

HOWEVER - the server can fail and we want to ensure independent visibility of the tank level, regardless of whether the server is operating. We also want there to be an independent buzzer.

As such:

Data from the tanks is averaged and output to bar graphs on a 4.2" e-paper display

The display updates every 60 seconds

If the 'display update' touch pin is activated (connected to a screw on the case) the display is updated.

If the black wate tank exceeds a certain level, then the buzzer will be activated IF the buzzer function is enabled (this is controlled by a second touch pin).

The display also includes an icon for buzzer on / off, an icon showing wifi signal strenght, and a status line which:

on boot displays software version number and IP address
Thereafter displays the last update time of the tank data. Time is drawn from the signalk server.
TO DO:

Shift accross to a good display front-lit e-paper display
activate the front light for a defined period of time when the button is touched
