#include "gpio.h"

/* ================= Register Definitions ================= */

// Reset and Clock Control
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))

// GPIOB Registers (Motor Direction Pins)
#define GPIOB_BASE      0x40020400UL
#define GPIOB_MODER     (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_ODR       (*(volatile uint32_t *)(GPIOB_BASE + 0x14))

#define L_IN1 0
#define L_IN2 1
#define R_IN3 10
#define R_IN4 11

// GPIOD Registers (Onboard LEDs)
#define GPIOD_BASE      0x40020C00UL
#define GPIOD_MODER     (*(volatile uint32_t *)(GPIOD_BASE + 0x00))
#define GPIOD_ODR       (*(volatile uint32_t *)(GPIOD_BASE + 0x14))

#define HEAD_L  14
#define RIGHT_L 12
#define LEFT_L  13


/* ================= Initialization Logic ================= */
void GPIO_Init(void)
{
    // 1. Enable hardware clocks for Port B and Port D
    RCC_AHB1ENR |= (1U << 1);
    RCC_AHB1ENR |= (1U << 3);

    // 2. Configure PB0, PB1, PB10, PB11 as General Purpose Output (01) for the L298N
    GPIOB_MODER &= ~(3U << (2*L_IN1));
    GPIOB_MODER |= (1U << (2*L_IN1));         // left in1

    GPIOB_MODER &= ~(3U << (2*L_IN2));
    GPIOB_MODER |= (1U << (2*L_IN2));         // left in2

    GPIOB_MODER &= ~(3U << (2*R_IN3));
    GPIOB_MODER |= (1U << (2*R_IN3));         // right in3

    GPIOB_MODER &= ~(3U << (2*R_IN4));
    GPIOB_MODER |= (1U << (2*R_IN4));         // right in4

    // 3. Configure PD12, PD13, PD14 as General Purpose Output (01) for the LEDs

    // head light (Port D!)
    GPIOD_MODER &= ~(3U << (2*HEAD_L));
    GPIOD_MODER |= (1U << (2*HEAD_L));

    // right light (Port D!)
    GPIOD_MODER &= ~(3U << (2*RIGHT_L));
    GPIOD_MODER |= (1U << (2*RIGHT_L));

    // left light (Port D!)
    GPIOD_MODER &= ~(3U << (2*LEFT_L));
    GPIOD_MODER |= (1U << (2*LEFT_L));
}

/* ================= Motor direction & LED Logic ================= */

void Motor_Forward(void)
{
    // Motors: Both Forward
    GPIOB_ODR |= (1U << L_IN1);
    GPIOB_ODR &= ~(1U << L_IN2);
    GPIOB_ODR |= (1U << R_IN3);
    GPIOB_ODR &= ~(1U << R_IN4);

    // LEDs: Turn OFF both turn signals
    GPIOD_ODR &= ~(1U << RIGHT_L);
    GPIOD_ODR &= ~(1U << LEFT_L);
}

void Motor_Backward(void)
{
    // Motors: Both Backward
    GPIOB_ODR &= ~(1U << L_IN1);
    GPIOB_ODR |= (1U << L_IN2);
    GPIOB_ODR &= ~(1U << R_IN3);
    GPIOB_ODR |= (1U << R_IN4);

    // LEDs: Turn OFF both turn signals
    GPIOD_ODR &= ~(1U << RIGHT_L);
    GPIOD_ODR &= ~(1U << LEFT_L);
}

void Motor_TurnRight(void)
{
    // Motors: Left Forward, Right Backward
    GPIOB_ODR |= (1U << L_IN1);
    GPIOB_ODR &= ~(1U << L_IN2);
    GPIOB_ODR &= ~(1U << R_IN3);
    GPIOB_ODR |= (1U << R_IN4);

    // LEDs: Turn ON Right LED, Turn OFF Left LED
    GPIOD_ODR |= (1U << RIGHT_L);
    GPIOD_ODR &= ~(1U << LEFT_L);
}

void Motor_TurnLeft(void)
{
    // Motors: Left Backward, Right Forward
    GPIOB_ODR &= ~(1U << L_IN1);
    GPIOB_ODR |= (1U << L_IN2);
    GPIOB_ODR |= (1U << R_IN3);
    GPIOB_ODR &= ~(1U << R_IN4);

    // LEDs: Turn ON Left LED, Turn OFF Right LED
    GPIOD_ODR |= (1U << LEFT_L);
    GPIOD_ODR &= ~(1U << RIGHT_L);
}

void Motor_Stop(void)
{
    // Motors: Stop
    GPIOB_ODR &= ~(1U << L_IN1);
    GPIOB_ODR &= ~(1U << L_IN2);
    GPIOB_ODR &= ~(1U << R_IN3);
    GPIOB_ODR &= ~(1U << R_IN4);

    // LEDs: Turn OFF both turn signals
    GPIOD_ODR &= ~(1U << RIGHT_L);
    GPIOD_ODR &= ~(1U << LEFT_L);
}

/* ================= LED Control Logic ================= */
void LED_SetHeadlight(uint8_t state)
{
    if(state)
    {
        GPIOD_ODR |= (1U << HEAD_L);   // Turn ON Red Auto-Headlight
    }
    else
    {
        GPIOD_ODR &= ~(1U << HEAD_L);  // Turn OFF Red Auto-Headlight
    }
}
