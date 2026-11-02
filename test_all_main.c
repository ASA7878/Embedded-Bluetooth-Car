//#include <stdint.h>
//#include "gpio.h"
//#include "pwm.h"
//#include "spi.h"
//#include "uart.h"
//#include "adc.h"
//#include "i2c.h"
//
//// Simple delay function (assuming 16MHz clock)
//void delay_ms(uint32_t ms)
//{
//    for (volatile uint32_t i = 0; i < ms * 4000; i++) {
//        __asm__("nop");
//    }
//}
//
///* =======================================================
// * TEST 1: MOTOR & PWM
// * Expected: Motors spin forward at 50%, wait 2s, stop for 2s.
// * ======================================================= */
//void Test_Motors(void)
//{
//    PWM_SetSpeed(500, 500);
//    Motor_Forward();
//    delay_ms(2000);
//
//    PWM_SetSpeed(0, 0);
//    Motor_Stop();
//    delay_ms(2000);
//}
//
///* =======================================================
// * TEST 2: BLUETOOTH (UART)
// * Expected: Connect your phone via Bluetooth terminal.
// * Send a character (e.g., 'A'). The STM32 should echo it back.
// * ======================================================= */
//void Test_Bluetooth(void)
//{
//    if (UART_DataAvailable()) {
//        char received = UART_ReceiveChar();
//        UART_SendString("Received: ");
//        UART_SendChar(received);
//        UART_SendString("\r\n");
//    }
//}
//
///* =======================================================
// * TEST 3: LDR SENSOR (ADC)
// * Expected: Reads the light level and prints it to your phone
// * via Bluetooth. Cover the sensor with your hand to see it drop.
// * ======================================================= */
//void Test_LDR(void)
//{
//    uint16_t light_val = ADC_Read();
//    UART_SendString("LDR Value: ");
//    UART_SendNumber(light_val);
//    UART_SendString("\r\n");
//    delay_ms(500);
//}
//
///* =======================================================
// * TEST 4: ACCELEROMETER (SPI)
// * Expected: Reads the WHO_AM_I register (0x0F). For the
// * LIS302DL, this should ALWAYS return 59 (0x3B). Prints to phone.
// * ======================================================= */
//void Test_Accelerometer(void)
//{
//    // Read WHO_AM_I register
//    uint8_t who_am_i = Accel_ReadReg(0x0F);
//
//    UART_SendString("LIS302DL ID (Expect 59): ");
//    UART_SendNumber(who_am_i);
//    UART_SendString("\r\n");
//    delay_ms(1000);
//}
//
///* =======================================================
// * TEST 5: LCD DISPLAY (I2C)
// * Expected: LCD turns on, prints "STM32 Test" on row 0,
// * and a continuously incrementing number on row 1.
// * ======================================================= */
//uint32_t lcd_counter = 0;
//void Test_LCD(void)
//{
//    LCD_setCursor(0, 0);
//    LCD_print("STM32 LCD Test  ");
//
//    LCD_setCursor(1, 0);
//    LCD_print("Count: ");
//
//    // Convert counter to string manually to print on LCD
//    char numStr[10];
//    uint32_t temp = lcd_counter;
//    int i = 0;
//    if (temp == 0) { numStr[i++] = '0'; }
//    while (temp > 0) {
//        numStr[i++] = (temp % 10) + '0';
//        temp /= 10;
//    }
//    // Print reverse
//    while (i > 0) {
//        LCD_data(numStr[--i]);
//    }
//    LCD_print("    "); // Clear trailing characters
//
//    lcd_counter++;
//    delay_ms(1000);
//}
//
//
///* =======================================================
// * MAIN FUNCTION
// * ======================================================= */
//int main(void)
//{
//    // 1. Initialize all peripheral clocks
//    clock(); // From your SPI/I2C setup
//
//    // 2. Initialize all drivers
//    GPIO_Init();
//    PWM_Init();
//    gpio_spi_init();
//    spi_init();
//    UART_Init();
//    ADC_Init();
//    I2C_Init();
//    LCD_Init();
//
//    // 3. Power on the Accelerometer (Required before reading data)
//    // Write to CTRL_REG1 (0x20): Power up, 400Hz, Enable X, Y, Z (0x47)
//    Accel_WriteReg(0x20, 0x67);
//
//    // Give peripherals a moment to stabilize
//    delay_ms(100);
//
//    UART_SendString("System Booted. Starting Tests...\r\n");
//
//    while(1)
//    {
//        /*
//         * UNCOMMENT ONLY ONE TEST AT A TIME TO VERIFY HARDWARE
//         */
//
//        Test_Motors();
//        // Test_Bluetooth();
//        // Test_LDR();
//        // Test_Accelerometer();
//        // Test_LCD();
//    }
//}
