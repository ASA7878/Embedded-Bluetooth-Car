# Embedded Bluetooth Smart Vehicle (STM32F407)

A bare-metal STM32F407 vehicle utilizing custom peripheral drivers (USART, SPI, I2C, TIM, ADC) for interrupt-driven Bluetooth control, soft-start PWM actuation, and hardware-level safety overrides.

## Overview
This project implements a smart automotive control layer on an STM32F407 microcontroller using bare-metal C. Designed with a deterministic super-loop architecture and zero-blocking hardware interrupts, the system delivers smooth motor control, active collision monitoring, and dynamic environmental response without the overhead of an RTOS.

## Key Features & Architecture
* **Hardware UART Interrupts:** Allows for instant command execution from the Bluetooth module, completely bypassing delays[cite: 2].
* **Soft-Start PWM Motion Control:** The PWM duty cycle steps by 100 inside a `for` loop to gradually accelerate the motors, completely preventing battery-draining stall currents[cite: 2].
* **Active Safety Feedback:** The onboard SPI accelerometer calculates impact magnitude; if `abs(X) > 100`, it forces an emergency override on the current command to safely drop the speed (crawl mode)[cite: 2].
* **Robust State Management:** A global `system_active` flag dictates operation. Standby mode is strictly retained by filtering out any incoming interrupt commands and physically locking the motors[cite: 2].
* **Environmental Adaptation:** An ADC-driven LDR automatically controls the vehicle's headlights based on ambient lighting conditions.
* **Live Dashboard:** A 20x4 I2C LCD continuously updates with the current system state, speed, and sensor readings.

## Hardware Requirements
* STM32F407 Discovery Board
* HC-06 Bluetooth Module (UART)
* L298N Dual H-Bridge Motor Driver
* DC Motors & Robot Chassis
* LIS302DL Accelerometer (Onboard via SPI)
* LDR Light Sensor (ADC)
* 20x4 I2C LCD Display

## Working Mechanism
1. **Initialize:** Configure all peripheral clocks and registers (USART, SPI, I2C, TIM, ADC).
2. **Receive:** Capture mobile app commands instantly via UART hardware interrupts.
3. **Decode:** Translate ASCII characters into actionable system states.
4. **Actuate:** Drive the L298N with smooth PWM stepping logic.
5. **Monitor:** Continuously check the accelerometer and LDR for safety overrides.
6. **Feedback:** Push live telemetry to the LCD dashboard.

## Setup & Flashing
1. Open the project in STM32CubeIDE or your preferred bare-metal toolchain.
2. Compile the `main.c` and peripheral drivers.
3. Flash the `.bin` or `.elf` file to the STM32F407 Discovery board.
4. Connect to the HC-06 module via any standard Bluetooth Serial terminal app to begin sending commands.
