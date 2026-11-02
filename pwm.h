#ifndef PWM_H
#define PWM_H

#include <stdint.h>
void PWM_Init(void);
void PWM_SetSpeed(uint16_t left_duty, uint16_t right_duty);

#endif
