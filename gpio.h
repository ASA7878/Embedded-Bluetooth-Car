#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

// Hardware Initialization
void GPIO_Init(void);

// Motor and Turn Signal Logic
void Motor_Forward(void);
void Motor_Backward(void);
void Motor_TurnRight(void);
void Motor_TurnLeft(void);
void Motor_Stop(void);

// Status LED Control
void LED_SetHeadlight(uint8_t state);

#endif
