/*
 * code with stand alone mode with interrupt
 */

#include <stdint.h>
#include <stdlib.h>
#include "gpio.h"
#include "pwm.h"
#include "spi.h"
#include "uart.h"
#include "adc.h"
#include "i2c.h"

#define LDR_DARK_THRESHOLD   1023
#define ACCEL_CRASH_LIMIT    30

#ifndef USART2_BASE
#define USART2_BASE     0x40004400UL
#define USART2_SR       (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR       (*(volatile uint32_t *)(USART2_BASE + 0x04))
#endif

volatile char current_cmd = '0';
volatile uint8_t new_cmd_received = 0;
volatile uint32_t sys_tick = 0;
volatile uint8_t system_active = 0; // 0 = Standby (OFF), 1 = Active (ON)

void SysTick_Handler(void) { sys_tick++; }

void SysTick_Init(void) {
    *(volatile uint32_t *)0xE000E014 = 16000 - 1;
    *(volatile uint32_t *)0xE000E018 = 0;
    *(volatile uint32_t *)0xE000E010 = 7;
}

void delay_ms(uint32_t ms) {
    uint32_t start_time = sys_tick;
    while ((sys_tick - start_time) < ms);
}

void USART2_IRQHandler(void) {
    if (USART2_SR & (1U << 5)) {
        char received = (char)(USART2_DR & 0xFF);

        if (received != '\r' && received != '\n') {
            // Check for Power ON/OFF commands first
            if (received == 'S') {
                system_active = 1;
                current_cmd = '0'; // Default to stopped when turning on
                new_cmd_received = 1;
            }
            else if (received == 'd') {
                system_active = 0;
                current_cmd = '0';
                new_cmd_received = 1;
            }
            // Only accept movement commands if the system is active
            else if (system_active) {
                current_cmd = received;
                new_cmd_received = 1;
            }
        }
    }
}

// ... [Keep LCD_printSigned and UART_SendSigned here] ...

/* =======================================================
 * FORMATTING HELPER FUNCTIONS (For Negative Numbers)
 * ======================================================= */
void LCD_printSigned(int16_t num) {
    if (num < 0) {
        LCD_data('-');
        num = -num;
    }
    char buffer[10];
    int i = 0;
    if (num == 0) {
        LCD_data('0');
        return;
    }
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (i > 0) {
        LCD_data(buffer[--i]);
    }
}

void UART_SendSigned(int16_t num) {
    if (num < 0) {
        UART_SendChar('-');
        num = -num;
    }
    UART_SendNumber((uint32_t)num);
}

void Check_Safety_Sensors(void) {
    int8_t accel_x = (int8_t)Accel_ReadReg(0x29);
    int8_t accel_y = (int8_t)Accel_ReadReg(0x2B);
    int8_t accel_z = (int8_t)Accel_ReadReg(0x2D);

    if (abs(accel_x) > ACCEL_CRASH_LIMIT || abs(accel_y) > ACCEL_CRASH_LIMIT) {
        LCD_setCursor(0, 0);
        LCD_print("WARNING: IMPACT!    ");
        PWM_SetSpeed(250, 250);
//        Motor_Stop();
//        current_cmd = '0';
    } else {
        LCD_setCursor(0, 0);
        LCD_print("Status: SAFE        ");
    }

    LCD_setCursor(2, 0);
    LCD_print("X:"); LCD_printSigned(accel_x); LCD_print(" ");
    LCD_setCursor(2, 7);
    LCD_print("Y:"); LCD_printSigned(accel_y); LCD_print(" ");
    LCD_setCursor(2, 14);
    LCD_print("Z:"); LCD_printSigned(accel_z); LCD_print(" ");
}

void Check_Headlights(void) {
    uint16_t light_level = ADC_Read();
    uint8_t led_is_on = 0;

    if (light_level < LDR_DARK_THRESHOLD) {
        LED_SetHeadlight(1);
        led_is_on = 1;
    } else {
        LED_SetHeadlight(0);
        led_is_on = 0;
    }

    LCD_setCursor(3, 0);
    LCD_print("LDR:"); LCD_printSigned(light_level);
    LCD_print(" LED:");
    if(led_is_on) { LCD_print("ON   "); } else { LCD_print("OFF  "); }
}

int main(void) {
    clock();
    SysTick_Init();
    GPIO_Init();
    PWM_Init();
    gpio_spi_init();
    spi_init();
    UART_Init();
    ADC_Init();
    I2C_Init();
    LCD_Init();

    Accel_WriteReg(0x20, 0x67);
    PWM_SetSpeed(0, 0);
    Motor_Stop();

    // Initial Standby State
    LCD_setCursor(0, 0);
    LCD_print("STANDBY MODE        ");
    LCD_setCursor(1, 0);
    LCD_print("Press 'S' to start  ");

    uint32_t last_accel_check = 0;
    uint32_t last_ldr_check = 0;
    uint32_t last_uart_telemetry = 0;

    while(1) {
        if (new_cmd_received) {
            new_cmd_received = 0;
            if (system_active) {
                LCD_setCursor(1, 0);
                LCD_print("Cmd:            ");
                LCD_setCursor(1, 5);
                LCD_data(current_cmd);
            }
        }

        // --- System State Machine ---
        if (system_active) {
            // --- B. Execute Movement ---
            switch (current_cmd) {
                case '2':
                    Motor_Stop(); delay_ms(100); Motor_Forward();
                    for (uint16_t speed = 300; speed <= 800; speed += 100) { PWM_SetSpeed(speed, speed); delay_ms(50); }
                    current_cmd = 'X'; break;
                case '4':
                    Motor_Stop(); delay_ms(100); Motor_Backward();
                    for (uint16_t speed = 300; speed <= 800; speed += 100) { PWM_SetSpeed(speed, speed); delay_ms(50); }
                    current_cmd = 'X'; break;
                case '3':
                    PWM_SetSpeed(600, 600); Motor_TurnLeft(); current_cmd = 'X'; break;
                case '1':
                    PWM_SetSpeed(600, 600); Motor_TurnRight(); current_cmd = 'X'; break;
                case '0':
                    PWM_SetSpeed(0, 0); Motor_Stop(); current_cmd = 'X'; break;
                case 'X': break;
            }

            // --- C. Task Scheduler ---
            if ((sys_tick - last_accel_check) >= 50) {
                last_accel_check = sys_tick; Check_Safety_Sensors();
            }
            if ((sys_tick - last_ldr_check) >= 100) {
                last_ldr_check = sys_tick; Check_Headlights();
            }
            if ((sys_tick - last_uart_telemetry) >= 500) {
                last_uart_telemetry = sys_tick;
                int8_t ax = (int8_t)Accel_ReadReg(0x29), ay = (int8_t)Accel_ReadReg(0x2B), az = (int8_t)Accel_ReadReg(0x2D);
                uint16_t ldr = ADC_Read();
                UART_SendString("X:"); UART_SendSigned(ax); UART_SendString(" Y:"); UART_SendSigned(ay);
                UART_SendString(" Z:"); UART_SendSigned(az); UART_SendString(" | LDR:"); UART_SendSigned(ldr);
                if (ldr < LDR_DARK_THRESHOLD) { UART_SendString(" | LED:ON\r\n"); } else { UART_SendString(" | LED:OFF\r\n"); }
            }
        } else {
            // --- STANDBY MODE LOGIC ---
            PWM_SetSpeed(0, 0);
            Motor_Stop();
            LED_SetHeadlight(0);

            // Periodically refresh the standby screen so it doesn't get stuck
            if ((sys_tick - last_accel_check) >= 500) {
                last_accel_check = sys_tick;
                LCD_setCursor(0, 0); LCD_print("STANDBY MODE        ");
                LCD_setCursor(1, 0); LCD_print("Press 'S' to start  ");
                LCD_setCursor(2, 0); LCD_print("                    ");
                LCD_setCursor(3, 0); LCD_print("                    ");
            }
        }
    }
}

///*
// * interrupt driven main code for all fn
// */
//#include <stdint.h>
//#include <stdlib.h>
//#include "gpio.h"
//#include "pwm.h"
//#include "spi.h"
//#include "uart.h"
//#include "adc.h"
//#include "i2c.h"
//
//#define LDR_DARK_THRESHOLD   1023
//#define ACCEL_CRASH_LIMIT    60
//
///* ================= USART2 REGISTERS ================= */
//// (Included here in case they were not added to uart.h)
//#ifndef USART2_BASE
//#define USART2_BASE     0x40004400UL
//#define USART2_SR       (*(volatile uint32_t *)(USART2_BASE + 0x00))
//#define USART2_DR       (*(volatile uint32_t *)(USART2_BASE + 0x04))
//#endif
//
///* =======================================================
// * GLOBAL VARIABLES & INTERRUPT FLAGS
// * ======================================================= */
//volatile char current_cmd = '0'; // Default to Stop state
//volatile uint8_t new_cmd_received = 0;
//volatile uint32_t sys_tick = 0; // The master 1ms software tick counter
//
///* =======================================================
// * SYSTICK INTERRUPT (Fires exactly every 1 millisecond)
// * ======================================================= */
//void SysTick_Handler(void)
//{
//    sys_tick++;
//}
//
//void SysTick_Init(void)
//{
//    // Assuming 16MHz clock. 16,000,000 / 1000 = 16000 cycles per millisecond
//    *(volatile uint32_t *)0xE000E014 = 16000 - 1; // SYST_RVR (Reload Value)
//    *(volatile uint32_t *)0xE000E018 = 0;         // SYST_CVR (Current Value)
//    *(volatile uint32_t *)0xE000E010 = 7;         // SYST_CSR: Enable=1, TickInt=1, ClkSource=1
//}
//
//// Global delay function using the SysTick interrupt
//void delay_ms(uint32_t ms)
//{
//    uint32_t start_time = sys_tick;
//    while ((sys_tick - start_time) < ms);
//}
//
///* =======================================================
// * USART2 INTERRUPT SERVICE ROUTINE (ISR)
// * ======================================================= */
//void USART2_IRQHandler(void)
//{
//    if (USART2_SR & (1U << 5))
//    {
//        char received = (char)(USART2_DR & 0xFF);
//
//        // Ignore hidden carriage return and newline characters from terminal apps
//        if (received != '\r' && received != '\n') {
//            current_cmd = received;
//            new_cmd_received = 1;
//        }
//    }
//}
//
///* =======================================================
// * FORMATTING HELPER FUNCTIONS (For Negative Numbers)
// * ======================================================= */
//void LCD_printSigned(int16_t num) {
//    if (num < 0) {
//        LCD_data('-');
//        num = -num;
//    }
//    char buffer[10];
//    int i = 0;
//    if (num == 0) {
//        LCD_data('0');
//        return;
//    }
//    while (num > 0) {
//        buffer[i++] = (num % 10) + '0';
//        num /= 10;
//    }
//    while (i > 0) {
//        LCD_data(buffer[--i]);
//    }
//}
//
//void UART_SendSigned(int16_t num) {
//    if (num < 0) {
//        UART_SendChar('-');
//        num = -num;
//    }
//    UART_SendNumber((uint32_t)num);
//}
//
///* =======================================================
// * SENSOR HELPER FUNCTIONS
// * ======================================================= */
//void Check_Safety_Sensors(void) {
//    // Read X, Y, and Z axes
//    int8_t accel_x = (int8_t)Accel_ReadReg(0x29);
//    int8_t accel_y = (int8_t)Accel_ReadReg(0x2B);
//    int8_t accel_z = (int8_t)Accel_ReadReg(0x2D);
//
//    // 1. Safety Logic checks for excessive motion
//    if (abs(accel_x) > ACCEL_CRASH_LIMIT || abs(accel_y) > ACCEL_CRASH_LIMIT) {
//        LCD_setCursor(0, 0);
//        LCD_print("WARNING: IMPACT!    ");
//        PWM_SetSpeed(200, 200);
////        Motor_Stop();
////        current_cmd = '0'; // Force safe stop command state
//    } else {
//        LCD_setCursor(0, 0);
//        LCD_print("Status: SAFE        ");
//    }
//
//    // 2. LCD Display (Row 2) - Print XYZ axes
//    LCD_setCursor(2, 0);
//    LCD_print("X:"); LCD_printSigned(accel_x); LCD_print(" ");
//    LCD_setCursor(2, 7);
//    LCD_print("Y:"); LCD_printSigned(accel_y); LCD_print(" ");
//    LCD_setCursor(2, 14);
//    LCD_print("Z:"); LCD_printSigned(accel_z); LCD_print(" ");
//}
//
//void Check_Headlights(void) {
//    uint16_t light_level = ADC_Read();
//    uint8_t led_is_on = 0;
//
//    // Headlights turn ON automatically in low-light conditions
//    // Headlights turn OFF in bright conditions
//    if (light_level < LDR_DARK_THRESHOLD) {
//        LED_SetHeadlight(0);
//        led_is_on = 0;
//    } else {
//        LED_SetHeadlight(1);
//        led_is_on = 1;
//    }
//
//    // LCD Display (Row 3) - Print LDR value and LED status
//    LCD_setCursor(3, 0);
//    LCD_print("LDR:"); LCD_printSigned(light_level);
//    LCD_print(" LED:");
//    if(led_is_on) {
//        LCD_print("ON   ");
//    } else {
//        LCD_print("OFF  ");
//    }
//}
//
///* =======================================================
// * MAIN APPLICATION
// * ======================================================= */
//int main(void) {
//    // 1. Initialize Hardware
//    clock();
//    SysTick_Init();
//    GPIO_Init();
//    PWM_Init();
//    gpio_spi_init();
//    spi_init();
//    UART_Init();
//    ADC_Init();
//    I2C_Init();
//    LCD_Init();
//
//    // 2. Configure LIS3DSH Accelerometer (Power on, 100Hz, XYZ enabled)
//    Accel_WriteReg(0x20, 0x67);
//
//    PWM_SetSpeed(0, 0);
//    Motor_Stop();
//
//    LCD_setCursor(0, 0);
//    LCD_print("System Booted.      ");
//    LCD_setCursor(1, 0);
//    LCD_print("Awaiting Cmd...     ");
//
//    // Scheduler timestamp variables
//    uint32_t last_accel_check = 0;
//    uint32_t last_ldr_check = 0;
//    uint32_t last_uart_telemetry = 0;
//
//    // 3. The Super-Loop State Machine
//    while(1) {
//        // --- A. Process Interrupt Flags ---
//        if (new_cmd_received) {
//            new_cmd_received = 0;
//            LCD_setCursor(1, 0);
//            LCD_print("Cmd:            ");
//            LCD_setCursor(1, 5);
//            LCD_data(current_cmd);
//        }
//
//        // --- B. Execute Movement (Mapped to Mobile App Protocol) ---
//        switch (current_cmd) {
//            case '2': // App Command: Forward
//                Motor_Stop();
//                delay_ms(100);
//                Motor_Forward();
//                for (uint16_t speed = 300; speed <= 800; speed += 100) {
//                    PWM_SetSpeed(speed, speed);
//                    delay_ms(50);
//                }
//                current_cmd = 'X';
//                break;
//
//            case '4': // App Command: Backward
//                Motor_Stop();
//                delay_ms(100);
//                Motor_Backward();
//                for (uint16_t speed = 300; speed <= 800; speed += 100) {
//                    PWM_SetSpeed(speed, speed);
//                    delay_ms(50);
//                }
//                current_cmd = 'X';
//                break;
//
//            case '3': // App Command: Left
//                PWM_SetSpeed(600, 600);
//                Motor_TurnLeft();
//                current_cmd = 'X';
//                break;
//
//            case '1': // App Command: Right
//                PWM_SetSpeed(600, 600);
//                Motor_TurnRight();
//                current_cmd = 'X';
//                break;
//
//            case '0': // App Command: Stop (Button Release)
//                PWM_SetSpeed(0, 0);
//                Motor_Stop();
//                current_cmd = 'X';
//                break;
//
//            case 'X':
//                // Holding state. Allows motors to run at designated speed without repeating the startup loop.
//                break;
//        }
//
//        // --- C. Non-Blocking Task Scheduler ---
//        // Check accelerometer every 50ms
//        if ((sys_tick - last_accel_check) >= 50) {
//            last_accel_check = sys_tick;
//            Check_Safety_Sensors();
//        }
//
//        // Check LDR every 100ms
//        if ((sys_tick - last_ldr_check) >= 100) {
//            last_ldr_check = sys_tick;
//            Check_Headlights();
//        }
//
//        // Output Telemetry to Bluetooth every 500ms
//        if ((sys_tick - last_uart_telemetry) >= 500) {
//            last_uart_telemetry = sys_tick;
//
//            int8_t ax = (int8_t)Accel_ReadReg(0x29);
//            int8_t ay = (int8_t)Accel_ReadReg(0x2B);
//            int8_t az = (int8_t)Accel_ReadReg(0x2D);
//            uint16_t ldr = ADC_Read();
//
//            UART_SendString("X:"); UART_SendSigned(ax);
//            UART_SendString(" Y:"); UART_SendSigned(ay);
//            UART_SendString(" Z:"); UART_SendSigned(az);
//            UART_SendString(" | LDR:"); UART_SendSigned(ldr);
//
//            if (ldr < LDR_DARK_THRESHOLD) {
//                UART_SendString(" | LED:ON\r\n");
//            } else {
//                UART_SendString(" | LED:OFF\r\n");
//            }
//        }
//    }
//}
//


///*
// * polling driven main code for all fn
// */
//
//#include <stdint.h>
//#include <stdlib.h> // For abs() function
//#include "gpio.h"
//#include "pwm.h"
//#include "spi.h"
//#include "uart.h"
//#include "adc.h"
//#include "i2c.h"
//
//// Thresholds
//#define LDR_DARK_THRESHOLD   2000  // Adjust based on your ambient light
//#define ACCEL_CRASH_LIMIT    100   // Sensitivity for aggressive movement
//
//// Helper function to read LIS3DSH Accelerometer X and Y axes
//void Check_Safety_Sensors(void) {
//    // Read X (0x29) and Y (0x2B) high-byte axes
//    int8_t accel_x = (int8_t)Accel_ReadReg(0x29);
//    int8_t accel_y = (int8_t)Accel_ReadReg(0x2B);
//
//    // Check for aggressive movement / collision
//    if (abs(accel_x) > ACCEL_CRASH_LIMIT || abs(accel_y) > ACCEL_CRASH_LIMIT) {
//        LCD_setCursor(0, 0);
//        LCD_print("WARNING: IMPACT!");
//        PWM_SetSpeed(0, 0); // Emergency stop
//        Motor_Stop();       // Also turns off turn signals automatically
//    } else {
//        LCD_setCursor(0, 0);
//        LCD_print("Status: SAFE    ");
//    }
//}
//
//// Helper function for automatic headlights
//void Check_Headlights(void) {
//    uint16_t light_level = ADC_Read();
//
//    if (light_level < LDR_DARK_THRESHOLD) {
//        // Dark: Turn on Red LED (PD14) using your existing function
//        LED_SetHeadlight(1);
//    } else {
//        // Bright: Turn off Red LED
//        LED_SetHeadlight(0);
//    }
//}
//
//int main(void) {
//    // 1. Initialize System Clocks and Hardware
//    clock();
//    GPIO_Init();
//    PWM_Init();
//    gpio_spi_init();
//    spi_init();
//    UART_Init();
//    ADC_Init();
//    I2C_Init();
//    LCD_Init();
//
//    // 2. Configure LIS3DSH Accelerometer (Power on, 100Hz, XYZ enabled)
//    Accel_WriteReg(0x20, 0x67);
//
//    // Initial State
//    char current_cmd = 'S';
//    PWM_SetSpeed(0, 0);
//    Motor_Stop();
//
//    LCD_setCursor(0, 0);
//    LCD_print("System Booted.  ");
//    LCD_setCursor(1, 0);
//    LCD_print("Awaiting Cmd... ");
//
//    // 3. The Super-Loop State Machine
//    while(1) {
//        // --- A. Read Bluetooth Commands ---
//        if (UART_DataAvailable()) {
//            current_cmd = UART_ReceiveChar();
//
//            // Update second row of LCD with the new command
//            LCD_setCursor(1, 0);
//            LCD_print("Cmd:            ");
//            LCD_setCursor(1, 5);
//            LCD_data(current_cmd);
//        }
//
//        // --- B. Execute Movement (Turn signals are handled automatically in gpio.c) ---
//        switch (current_cmd) {
//            case 'F': // Forward
//                PWM_SetSpeed(300, 300);
//                Motor_Forward();
//                break;
//
//            case 'B': // Backward
//                PWM_SetSpeed(300, 300);
//                Motor_Backward();
//                break;
//
//            case 'L': // Turn Left
//                PWM_SetSpeed(300, 300);
//                Motor_TurnLeft(); // Automatically turns on Left LED (PD13)
//                break;
//
//            case 'R': // Turn Right
//                PWM_SetSpeed(600, 600);
//                Motor_TurnRight(); // Automatically turns on Right LED (PD12)
//                break;
//
//            case 'S': // Stop
//            default:
//                PWM_SetSpeed(0, 0);
//                Motor_Stop(); // Automatically turns off both turn signals
//                break;
//        }
//
//        // --- C. Execute Environmental Safety Checks ---
//        Check_Headlights();
//        Check_Safety_Sensors();
//
//        // Small delay to prevent I2C/SPI bus spamming
//        for (volatile uint32_t i = 0; i < 50000; i++);
//    }
//}
