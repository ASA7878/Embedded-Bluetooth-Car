#include <stdint.h>
#include "adc.h"

/* ================= RCC ================= */
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44))

/* ================= GPIOA ================= */
#define GPIOA_BASE      0x40020000UL
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))

/* ================= ADC1 ================= */
#define ADC_BASE        0x40012000UL
#define ADC_SR          (*(volatile uint32_t *)(ADC_BASE + 0x00))
#define ADC_CR1         (*(volatile uint32_t *)(ADC_BASE + 0x04))
#define ADC_CR2         (*(volatile uint32_t *)(ADC_BASE + 0x08))
#define ADC_SMPR2       (*(volatile uint32_t *)(ADC_BASE + 0x10))
#define ADC_SQR1        (*(volatile uint32_t *)(ADC_BASE + 0x2C))
#define ADC_SQR3        (*(volatile uint32_t *)(ADC_BASE + 0x34))
#define ADC_DR          (*(volatile uint32_t *)(ADC_BASE + 0x4C))

void ADC_Init(void)
{
    /* Enable GPIOA clock */
    RCC_AHB1ENR |= (1U << 0);

    /* SETTING PA4 -> Analog mode (for LDR) */
    GPIOA_MODER |= (3U << 8);

    /* SETTING ADC1 clock */
    RCC_APB2ENR |= (1U << 8);

    /* One conversion (L=0000) */
    ADC_SQR1 = 0;

    /* Select Channel 4 for the 1st conversion in the sequence */
    ADC_SQR3 = 4;

    /* Sampling time for Channel 4 (480 cycles) */
    ADC_SMPR2 |= (7U << 12);

    /* Enable ADC */
    ADC_CR2 |= (1U << 0);
}

uint16_t ADC_Read(void)
{
    /* Start conversion (SWSTART) */
    ADC_CR2 |= (1U << 30);

    /* Wait for conversion complete (EOC bit) */
    while (!(ADC_SR & (1U << 1)))
    {
    }

    /* Return ADC result */
    return ADC_DR;
}
