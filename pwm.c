#include "pwm.h"

// RCC Registers
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40))

// GPIOA Registers (For PA0 and PA1)
#define GPIOA_BASE      0x40020000UL
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20))

// TIM2 Registers
#define TIM2_BASE       0x40000000UL
#define TIM2_CR1        (*(volatile uint32_t *)(TIM2_BASE + 0x00))
#define TIM2_CCMR1      (*(volatile uint32_t *)(TIM2_BASE + 0x18))
#define TIM2_CCER       (*(volatile uint32_t *)(TIM2_BASE + 0x20))
#define TIM2_PSC        (*(volatile uint32_t *)(TIM2_BASE + 0x28))
#define TIM2_ARR        (*(volatile uint32_t *)(TIM2_BASE + 0x2C))
#define TIM2_CCR1       (*(volatile uint32_t *)(TIM2_BASE + 0x34))
#define TIM2_CCR2       (*(volatile uint32_t *)(TIM2_BASE + 0x38))

// PWM  pins
#define L_ENA 0
#define R_ENB 1

void PWM_Init(void)
{
    // 1. Enable GPIOA clock and TIM2 clock
    RCC_AHB1ENR |= (1U << 0);    // GPIOA
    RCC_APB1ENR |= (1U << 0);    // TIM2

    // 2. Set PA0 and PA1 to Alternate Function mode (10)

    // left
    GPIOA_MODER &= ~(3U << (2*L_ENA));
    GPIOA_MODER |= (2U << (2*L_ENA));

    // right
    GPIOA_MODER &= ~(3U << (2*R_ENB));
    GPIOA_MODER |= (2U << (2*R_ENB));

    // 3. Select AF1 (TIM2) for PA0 and PA1 in the Alternate Function Register Low
    GPIOA_AFRL &= ~(0xFU << (4*R_ENB));
    GPIOA_AFRL |= (1U << (4*R_ENB));

    GPIOA_AFRL &= ~(0xFU << (4*L_ENA));
    GPIOA_AFRL |= (1U << (4*L_ENA));

    // 4. Configure Timer 2 for PWM Generation
    // Prescaler 15 and ARR 1000 sets a standard frequency for smooth DC motor operation
    TIM2_PSC = 15;
    TIM2_ARR = 1000;

    // Set PWM Mode 1 on Channel 1 (OC1M = 110) and Channel 2 (OC2M = 110)
    // Also enable output compare preload (bit 3 and bit 11)
    // select the pwm mode 1 on channel 1
    TIM2_CCMR1 |= (6U << 4);
    //for enabling  preload for channel 1
    TIM2_CCMR1 |= (1U << 3);

    TIM2_CCMR1 |= (6U << 12);
    //for enabling  preload for channel 1
    TIM2_CCMR1 |= (1U << 11);

    // Enable Compare Output for CH1 and CH2
    TIM2_CCER |= (1U << 0);
    TIM2_CCER |= (1U << 4);

    // Initialize duty cycle to 0 (Motors Stopped)
    TIM2_CCR1 = 0;
    TIM2_CCR2 = 0;

    // Enable TIM2 Counter
    TIM2_CR1 |= (1U << 0);
}

void PWM_SetSpeed(uint16_t left_duty, uint16_t right_duty)
{
    // Cap the duty cycle at the ARR value (1000) to prevent overflow
    if(left_duty > 1000)
    {
    	left_duty = 1000;
    }
    if(right_duty > 1000)
    {
    	right_duty = 1000;
    }

    // Set the duty cycle for PA0 (Left Motor / ENA) and PA1 (Right Motor / ENB)
    TIM2_CCR1 = left_duty;
    TIM2_CCR2 = right_duty;
}
