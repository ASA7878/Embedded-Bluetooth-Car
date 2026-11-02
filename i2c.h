#ifndef I2C_H
#define I2C_H

#include<stdint.h>

// timer - delay
//void delay_ms(uint32_t ms);

// i2c things
void I2C_Init(void);
void I2C_start(void);
int I2C_writeAddr(uint8_t address);
void I2C_writeData(uint8_t data);
void I2C_stop(void);

// LCD things
void LCD_pulse(uint8_t data);
void LCD_Send_nibble(uint8_t nibble, uint8_t mode);
void LCD_sendByte(uint8_t value, uint8_t mode);
void LCD_command(uint8_t cmd);
void LCD_data(uint8_t data);
void LCD_Init(void);
void LCD_setCursor(uint8_t row, uint8_t col);
void LCD_print(char *str);

#endif












