#include <stdint.h>
#include "i2c.h"

/* ================= RCC ================= */
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40))

/* ================= GPIOB ================= */
#define GPIOB_BASE      0x40020400UL
#define GPIOB_MODER     (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_AFRL      (*(volatile uint32_t *)(GPIOB_BASE + 0x20))
#define GPIOB_OTYPER    (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_PUPDR     (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

/* ================= I2C1 ================= */
#define I2C_BASE        0x40005400UL
#define I2C_CR1         (*(volatile uint32_t *)(I2C_BASE + 0x00))
#define I2C_CR2         (*(volatile uint32_t *)(I2C_BASE + 0x04))
#define I2C_CCR         (*(volatile uint32_t *)(I2C_BASE + 0x1C))
#define I2C_TRISE       (*(volatile uint32_t *)(I2C_BASE + 0x20))
#define I2C_SR1         (*(volatile uint32_t *)(I2C_BASE + 0x14))
#define I2C_DR          (*(volatile uint32_t *)(I2C_BASE + 0x10))
#define I2C_SR2         (*(volatile uint32_t *)(I2C_BASE + 0x18))

/* ================= Pins ================= */
#define SDA 7 // PB 7
#define SCL 6 // PB 6

///* ================= SysTick ================= */
//#define SYST_CSR (*(volatile uint32_t *)0xE000E010)
//#define SYST_RVR (*(volatile uint32_t *)0xE000E014)
//#define SYST_CVR (*(volatile uint32_t *)0xE000E018)
//
//void delay_ms(uint32_t ms)
//{
//    SYST_RVR = 16000 - 1;   // 16 MHz / 1000 = 1 ms per count-down
//    SYST_CVR = 0;           // clear the current value
//    SYST_CSR = 5;           // bit0 = enable, bit2 = use processor clock
//
//    while (ms--)
//    {
//        while (!(SYST_CSR & (1u<<16)));   // COUNTFLAG sets after 1 ms
//    }
//    SYST_CSR = 0;           // stop the timer
//}
extern void delay_ms(uint32_t ms);
void I2C_Init(void)
{
    /* 1. Enable Clocks */
    RCC_AHB1ENR |= (1u<<1);  // GPIOB
    RCC_APB1ENR |= (1u<<21); // I2C1

    /* 2. Configure PB6 and PB7 to Alternate Function Mode */
    GPIOB_MODER &= ~((3u<<(2*SDA)) | (3u<<(2*SCL)));
    GPIOB_MODER |= ((2u<<(2*SDA)) | (2u<<(2*SCL)));

    /* 3. Set Alternate Function 4 (AF4) for I2C */
    GPIOB_AFRL &= ~((0x0fu<<(4*SDA)) | (0x0fu<<(4*SCL)));
    GPIOB_AFRL |= ((4u<<(4*SDA)) | (4u<<(4*SCL)));

    /* 4. Set to Open-Drain (Required for I2C) */
    GPIOB_OTYPER |= (1u<<SDA) | (1u<<SCL);

    /* 5. Enable Pull-up Resistors */
    GPIOB_PUPDR &= ~((3u<<(2*SDA)) | (3u<<(2*SCL)));
    GPIOB_PUPDR |= ((1u<<(2*SDA)) | (1u<<(2*SCL)));

    /* 6. I2C Configuration */
    I2C_CR1 |= (1u<<15);      // Software reset
    I2C_CR1 &= ~(1u<<15);     // Release reset
    I2C_CR2 = 16;             // Peripheral clock = 16MHz
    I2C_CCR = 80;             // Standard mode 100kHz
    I2C_TRISE = 17;           // Max rise time

    I2C_CR1 |=(1u<<0);        // Enable Peripheral
}

void I2C_start(void)
{
    I2C_CR1 |=(1u<<8);
    while(!(I2C_SR1 & (1u<<0)));
}

int I2C_writeAddr(uint8_t address)
{
    I2C_DR = (address << 1);
    while (!(I2C_SR1 & (1u<<1)))
    {
        if (I2C_SR1 & (1u<<10)) // Acknowledge failure
        {
            I2C_SR1 &= ~(1u<<10);
            I2C_CR1 |= (1u<<9); // release the bus
            return -1;
        }
    }
    uint32_t dummy = I2C_SR1;
    dummy = I2C_SR2;
    (void)dummy;
    return 0;
}

void I2C_writeData(uint8_t data)
{
    while (!(I2C_SR1 & (1u<<7))); // wait for TXE
    I2C_DR = data;
    while (!(I2C_SR1 & (1u<<2))); // wait for BTF
}

void I2C_stop(void)
{
    I2C_CR1 |= (1u<<9);
    while (I2C_CR1 & (1u<<9));
}

/* ================= LCD I2C Functions ================= */

void LCD_pulse(uint8_t data)
{
    I2C_start();
    if (I2C_writeAddr(0x27) != 0) // Change 0x27 to 0x3F if LCD fails to initialize
    {
        return;
    }
    I2C_writeData(data | 0x04); // EN = 1
    I2C_writeData(data & ~(0x04)); // EN = 0
    I2C_stop();
}

void LCD_Send_nibble(uint8_t nibble, uint8_t mode)
{
    uint8_t loader = (nibble & 0xf0) | 0x08 | (mode & 0x01); // 0x08 keeps backlight on
    LCD_pulse(loader);
}

void LCD_sendByte(uint8_t value, uint8_t mode)
{
    LCD_Send_nibble((value & 0xf0), mode);
    LCD_Send_nibble(((value << 4) & 0xf0), mode);
}

void LCD_command(uint8_t cmd)
{
    LCD_sendByte(cmd, 0);
}

void LCD_data(uint8_t data)
{
    LCD_sendByte(data, 1);
}

void LCD_Init(void)
{
    delay_ms(50);
    LCD_Send_nibble(0x30, 0);
    delay_ms(5);
    LCD_Send_nibble(0x30, 0);
    delay_ms(1);
    LCD_Send_nibble(0x30, 0);
    delay_ms(1);

    LCD_Send_nibble(0x20, 0);
    delay_ms(1);

    LCD_command(0x28);
    LCD_command(0x0C);
    LCD_command(0x06);
    LCD_command(0x01);
    delay_ms(5);
}

void LCD_setCursor(uint8_t row, uint8_t col)
{
    uint8_t address;
    if (row == 0) address = 0x80 + col;
    else if (row == 1) address = 0xC0 + col;
    else if (row == 2) address = 0x94 + col;
    else if (row == 3) address = 0xD4 + col;
    else address = 0x80 + col;

    LCD_command(address);
}

void LCD_print(char *str)
{
    while (*str != '\0')
    {
        LCD_data(*str);
        str++;
    }
}
