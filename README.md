# Smart Security System using Arduino Uno

**Unit:** SIT315  
**Task:** M1.T1P-C-D (QP4)  
**Platform:** Arduino Uno and Tinkercad

## Project Summary

This project is a Smart Security System made using Arduino Uno. It uses three push buttons to simulate a door sensor, a window sensor, and a motion sensor. The system monitors these inputs and responds using a green LED, a red LED, and a buzzer.

The program uses **Pin Change Interrupts (PCI)** to detect button changes and a **Timer1 interrupt** to update the system every second. The interrupt routines are short, while the main loop processes events and updates the outputs.

## Components

- Arduino Uno
- Three push buttons (door, window, and motion)
- Green LED
- Red LED
- Buzzer
- Breadboard and connecting wires

## Arduino Pin Connections

| Component | Arduino Pin | Purpose |
|---|---|---|
| Button 1 | D8 | Door sensor |
| Button 2 | D9 | Window sensor |
| Button 3 | D10 | Motion sensor |
| Green LED | D4 | SAFE indication |
| Red LED | D5 | WARNING / ALARM indication |
| Buzzer | D6 | Audible alarm |

The buttons use `INPUT_PULLUP`. A pressed button is read as `LOW`.

## System States

| State | Condition | Output |
|---|---|---|
| SAFE | No active sensors or stored recent events | Green LED blinks; red LED and buzzer are off |
| WARNING | One recent security event or an active sensor, without an alarm combination | Red LED blinks; green LED and buzzer are off |
| ALARM | Motion and door, or motion and window, are detected within the event-memory period | Red LED stays on; buzzer turns on; green LED is off |

The system remembers sensor events for approximately **3 seconds**. When an alarm is triggered, it remains active for approximately **5 seconds**, then returns to monitoring.

## Interrupt Features

### Pin Change Interrupts (PCI)

Buttons on pins D8, D9, and D10 are monitored using Pin Change Interrupts. When an input changes, the interrupt sets a flag. The main program then reads the sensor states and processes the change.

### Timer1 Interrupt

Timer1 is configured to generate an interrupt every second. It updates the system time and sets a flag for periodic processing. The Serial Monitor displays timer messages such as:

```text
[TIMER EVENT] 10 second(s)
[TIMER EVENT] 11 second(s)
```

PCI events and Timer1 updates can be seen in the **same simulation run**. This demonstrates that sensor changes are detected while regular timer updates continue.

## How to Run the Project

1. Open the Arduino circuit in **Tinkercad Circuits**.
2. Open the **Code** panel and make sure the final code from `TaskM1.cpp` is used.
3. Click **Start Simulation**.
4. Open the **Serial Monitor** to see sensor events, timer events, and the system status.
5. Follow the tests below.
6. Click **Stop Simulation** when testing is complete.

**Tinkercad project:** https://www.tinkercad.com/things/5Se9uHzKdJI-cool-gogo-hango/editel

## How to Test the System

### Test 1: SAFE

Do not press any buttons. The Serial Monitor should show the door and window closed, no motion detected, and `System status: SAFE`. The green LED should blink.

### Test 2: Door WARNING

Press and release **Button 1 (D8 — Door)**. The Serial Monitor should show a door PCI event and the system should enter WARNING. The red LED should blink.

### Test 3: Window WARNING

Wait until the system returns to SAFE. Press and release **Button 2 (D9 — Window)**. The Serial Monitor should show a window PCI event and WARNING.

### Test 4: Motion WARNING

Wait until the system returns to SAFE. Press and release **Button 3 (D10 — Motion)**. The Serial Monitor should show a motion PCI event and WARNING.

### Test 5: ALARM

Wait until SAFE. Press and release **Button 3 (D10 — Motion)**, then press and release **Button 1 (D8 — Door)** within **3 seconds**. The system should enter ALARM: the red LED stays on and the buzzer sounds. After approximately 5 seconds, the system returns to monitoring.

### Test 6: PCI and Timer1 Together

Keep the Serial Monitor open while the simulation runs. Watch the `[TIMER EVENT]` messages appear each second, then press and release **Button 1 (D8 — Door)**. The monitor should show `[PCI EVENT]` messages between continuing `[TIMER EVENT]` messages.

## Project Files

- `TaskM1.cpp` — Arduino program source code
- `README.md` — Project summary and testing instructions
- Circuit diagram or screenshot — Arduino wiring and components (uploaded separately)

## Demonstration

The demonstration video shows the circuit, code, SAFE/WARNING/ALARM tests, and PCI events operating alongside Timer1 updates.

**Demonstration video link:** [Add your working video link here.
](https://deakin.au.panopto.com/Panopto/Pages/Viewer.aspx?id=3a3708b5-6598-4c86-ad56-b4e000548461) 
#
