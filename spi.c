#include <stdint.h>
#include "spi.h"

/* =========================================================================
 * RCC (Reset and Clock Control) Registers
 * ========================================================================= */
#define RCC_BASE        0x40023800U
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30U)) // GPIO clocks
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44U)) // SPI1 clock

/* =========================================================================
 * GPIOA Registers (Used for SPI SCK, MISO, MOSI)
 * ========================================================================= */
#define GPIOA_BASE      0x40020000U
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00U))
#define GPIOA_OTYPER    (*(volatile uint32_t *)(GPIOA_BASE + 0x04U))
#define GPIOA_OSPEEDR   (*(volatile uint32_t *)(GPIOA_BASE + 0x08U))
#define GPIOA_PUPDR     (*(volatile uint32_t *)(GPIOA_BASE + 0x0CU))
#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20U))

/* =========================================================================
 * GPIOE Registers (Used for Chip Select on PE3)
 * ========================================================================= */
#define GPIOE_BASE      0x40021000U
#define GPIOE_MODER     (*(volatile uint32_t *)(GPIOE_BASE + 0x00U))
#define GPIOE_OTYPER    (*(volatile uint32_t *)(GPIOE_BASE + 0x04U))
#define GPIOE_OSPEEDR   (*(volatile uint32_t *)(GPIOE_BASE + 0x08U))
#define GPIOE_PUPDR     (*(volatile uint32_t *)(GPIOE_BASE + 0x0CU))
#define GPIOE_BSRR      (*(volatile uint32_t *)(GPIOE_BASE + 0x18U)) // Bit set/reset for fast toggling

/* =========================================================================
 * SPI1 Registers
 * ========================================================================= */
#define SPI1_BASE       0x40013000U
#define SPI1_CR1        (*(volatile uint32_t *)(SPI1_BASE + 0x00U))
#define SPI1_SR         (*(volatile uint32_t *)(SPI1_BASE + 0x08U))
#define SPI1_DR         (*(volatile uint32_t *)(SPI1_BASE + 0x0CU))

/* =========================================================================
 * SPI Pin Definitions
 * ========================================================================= */
#define SCK             5U  // PA5: Serial Clock
#define MISO            6U  // PA6: Master In Slave Out
#define MOSI            7U  // PA7: Master Out Slave In
#define CS              3U  // PE3: Chip Select for LIS302DL

/* =========================================================================
 * SPI Status Register Flags
 * ========================================================================= */
#define SPI_RXNE        (1U << 0) // Receive buffer Not Empty flag
#define SPI_TXE         (1U << 1) // Transmit buffer Empty flag
#define SPI_BSY         (1U << 7) // Busy flag

/*--------------------------------------------------
 * clock()
 * Enables hardware clocks for GPIOA, GPIOE, and SPI1.
 *--------------------------------------------------*/
void clock(void)
{
    /* Enable GPIOA (SPI pins) and GPIOE (CS pin) clocks */
    RCC_AHB1ENR |= (1U << 0) | (1U << 4);
    /* Enable SPI1 clock */
    RCC_APB2ENR |= (1U << 12);
}

/*--------------------------------------------------
 * gpio_spi_init()
 * Configures PA5, PA6, PA7 for SPI and PE3 for CS.
 *--------------------------------------------------*/
void gpio_spi_init(void)
{
    /* 1. Set PA5, PA6, PA7 to Alternate Function mode (10) */
    GPIOA_MODER &= ~((3U << (2U * SCK)) | (3U << (2U * MISO)) | (3U << (2U * MOSI)));
    GPIOA_MODER |=  ((2U << (2U * SCK)) | (2U << (2U * MISO)) | (2U << (2U * MOSI)));

    /* 2. Map PA5, PA6, PA7 to Alternate Function 5 (SPI1) */
    GPIOA_AFRL &= ~(0xFU << (4U * SCK));
    GPIOA_AFRL |=  (5U << (4U * SCK));

    GPIOA_AFRL &= ~(0xFU << (4U * MISO));
    GPIOA_AFRL |=  (5U << (4U * MISO));

    GPIOA_AFRL &= ~(0xFU << (4U * MOSI));
    GPIOA_AFRL |=  (5U << (4U * MOSI));

    /* 3. Configure PE3 (Chip Select) as General-Purpose Output (01) */
    GPIOE_MODER &= ~(3U << (2U * CS));
    GPIOE_MODER |=  (1U << (2U * CS));

    /* 4. Set push-pull output type */
    GPIOA_OTYPER &= ~((1U << SCK) | (1U << MOSI));
    GPIOE_OTYPER &= ~(1U << CS); // PE3 push-pull

    /* 5. Set high-speed mode */
    GPIOA_OSPEEDR |= ((3U << (2U * SCK)) | (3U << (2U * MISO)) | (3U << (2U * MOSI)));
    GPIOE_OSPEEDR |= (3U << (2U * CS)); // PE3 high speed

    /* 6. Disable internal pull-up/pull-down resistors */
    GPIOA_PUPDR &= ~((3U << (2U * SCK)) | (3U << (2U * MISO)) | (3U << (2U * MOSI)));
    GPIOE_PUPDR &= ~(3U << (2U * CS)); // PE3 no pull-up/down

    /* 7. Initialize Chip Select HIGH (LIS302DL is deselected initially) */
    GPIOE_BSRR = (1U << CS);
}

/*--------------------------------------------------
 * spi_init()
 * Configures the SPI peripheral settings.
 *--------------------------------------------------*/
void spi_init(void)
{
    SPI1_CR1 = 0U;
    SPI1_CR1 |= (1U << 2);              // Master mode
    SPI1_CR1 |= (1U << 1) | (1U << 0);  // CPOL=1, CPHA=1

    SPI1_CR1 &= ~(7U << 3);
    SPI1_CR1 |= (4U << 3);              // Baud rate = fPCLK/32

    SPI1_CR1 &= ~(1U << 11);            // 8-bit data frame
    SPI1_CR1 &= ~(1U << 7);             // MSB first
    SPI1_CR1 |= (1U << 9) | (1U << 8);  // Software slave management (SSM=1, SSI=1)

    SPI1_CR1 |= (1U << 6);              // Enable SPI1
}

/*--------------------------------------------------
 * cs_enable()
 * Pulls the PE3 pin LOW to select the LIS302DL.
 *--------------------------------------------------*/
void cs_enable(void)
{
    GPIOE_BSRR = (1U << (CS + 16U)); // Set PE3 LOW
}

/*--------------------------------------------------
 * cs_disable()
 * Pushes the PE3 pin HIGH to deselect the LIS302DL.
 *--------------------------------------------------*/
void cs_disable(void)
{
    GPIOE_BSRR = (1U << CS);         // Set PE3 HIGH
}

/*--------------------------------------------------
 * SPI1_Send()
 *--------------------------------------------------*/
void SPI1_Send(uint8_t data)
{
    while (!(SPI1_SR & SPI_TXE));
    SPI1_DR = data;

    while (!(SPI1_SR & SPI_RXNE));
    (void)SPI1_DR;

    while (SPI1_SR & SPI_BSY);
}

/*--------------------------------------------------
 * SPI1_Receive()
 *--------------------------------------------------*/
uint8_t SPI1_Receive(void)
{
    uint8_t data;
    while (!(SPI1_SR & SPI_TXE));

    SPI1_DR = 0xFFU;              // Dummy byte to generate clock

    while (!(SPI1_SR & SPI_RXNE));
    data = (uint8_t)SPI1_DR;

    while (SPI1_SR & SPI_BSY);
    return data;
}

/* =========================================================================
 * LIS302DL Specific High-Level Register Controls
 * ========================================================================= */

/*--------------------------------------------------
 * Accel_WriteReg()
 *--------------------------------------------------*/
void Accel_WriteReg(uint8_t reg, uint8_t value)
{
    cs_enable();
    SPI1_Send(reg);
    SPI1_Send(value);
    cs_disable();
}

/*--------------------------------------------------
 * Accel_ReadReg()
 *--------------------------------------------------*/
uint8_t Accel_ReadReg(uint8_t reg)
{
    uint8_t data;
    cs_enable();

    // Set Bit 7 to 1 for LIS302DL Read Operation
    SPI1_Send(reg | 0x80);
    data = SPI1_Receive();

    cs_disable();

    return data;
}
