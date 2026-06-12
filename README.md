\# Autonomous Irrigation Controller (Closed-Loop Actuator Logic)



\## Project Overview

This repository contains the firmware implementation for an industrial-grade, automated closed-loop irrigation system developed during the 2026 DecodeLabs Industrial Training program. 



The core objective is to move away from basic sensor polling and implement reliable hardware-software logic that safely drives a high-power inductive load (a water pump) based on real-time environmental data. The project actively mitigates real-world deployment challenges such as high-frequency switching noise, mechanical relay chattering, out-of-bounds mapping errors, and inductive back-EMF spikes.



\---



\## System Architecture \& Features



\### 1. Signal Conditioning (EMA Filtering)

Raw analog readings from an ESP32 ADC are highly susceptible to high-frequency switching noise caused by the internal 240 MHz CPU clock and Wi-Fi radio. A single `analogRead()` approach causes downstream system failures. This system processes signals using an Exponential Moving Average (EMA) software filter combined with basic hardware oversampling:

\* \*\*Formula:\*\* $S\_t = \\alpha \\cdot Y\_t + (1 - \\alpha) \\cdot S\_{t-1}$

\* \*\*Smoothing Factor ($\\alpha$):\*\* Set to `0.15` to smooth out transient noise while maintaining accurate tracking of slow-moving soil moisture dynamics.



\### 2. Dual-Threshold Hysteresis (Anti-Chatter Deadband)

A single threshold limit (e.g., turning the pump off exactly at 30% moisture) creates rapid relay oscillation (chattering) when environmental telemetry hovers near the boundary. This causes thermal stress on motor windings and arcs mechanical contacts. 

\* \*\*Lower Activation Threshold ($T\_{on}$):\*\* 30% (Pump turns ON)

\* \*\*Upper Deactivation Threshold ($T\_{off}$):\*\* 45% (Pump turns OFF)

\* \*\*The Deadband (30% - 45%):\*\* The system maintains its previous state, preventing rapid cycling and protecting mechanical relays.



\### 3. Power Isolation \& Electrical Protection

\* \*\*Galvanic Isolation:\*\* Designed to run with the relay board's `JD-VCC` jumper \*\*removed\*\*. The ESP32 logic side is physically isolated via an optocoupler gap using a separate 5V power line to block high-voltage inductive back-EMF spikes ($>100\\text{V}$) from entering the microcontroller.

\* \*\*Safe Bootup Sequence:\*\* Most industrial opto-isolated relays operate on \*\*Active-LOW\*\* logic. Because microcontroller GPIO pins default to floating/LOW states during power cycles, a bootup sequence is written to force the control pin `HIGH` \*before\* initializing it as an `OUTPUT`, preventing accidental flooding.



\---



\## Hardware Configuration

\* \*\*Microcontroller:\*\* ESP32 / Atmel SAM D21 (Supports 12-bit ADC / 4096 discrete steps)

\* \*\*Actuator:\*\* 5V Opto-Isolated Mechanical Relay Module switching an industrial 24V DC Water Pump

\* \*\*Sensor:\*\* Analog Soil Moisture Sensor

\* \*\*ADC Resolution:\*\* 12-Bit ($0 \\text{ to } 4095$, approx. $0.81\\text{ mV}$ per step resolution)



\---



\## Calibration Settings

Raw ADC values fluctuate depending on soil composition and cable run lengths. The following baseline anchor values were established via field calibration:

\* `ADC\_DRY` = `3200` (Sensor suspended in dry air)

\* `ADC\_WET` = `1200` (Sensor fully submerged in water)



\---



\## How to Deploy

1\. Clone this repository to your local workstation.

2\. Open the project directory using VS Code with the PlatformIO extension installed, or compile using the Arduino IDE.

3\. Wire your hardware matching the pinouts declared in `src/main.cpp`.

4\. Ensure the `JD-VCC` jumper on your relay module is disconnected and supplied by an external 5V rail, sharing a common ground \*only\* on the actuator side.

5\. Compile and flash the firmware to your target device.

