/*
 * these are header fn for interrupt driven
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/* ================= USART2 REGISTERS ================= */
#define USART2_BASE     0x40004400UL
#define USART2_SR       (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR       (*(volatile uint32_t *)(USART2_BASE + 0x04))

void UART_Init(void);
void UART_SendChar(char ch);
void UART_SendString(char *str);
void UART_SendNumber(uint32_t num);

#endif



///*
// * this are header file for the polling driven
// */
//
//#ifndef UART_H
//#define UART_H
//
//#include<stdint.h>
//
//void delay_ms(uint32_t ms);
//void UART_Init(void);
//void UART_SendChar(char ch);
//void UART_SendString(char *str);
//void UART_SendNumber(uint32_t num);
//uint8_t UART_DataAvailable(void);
//char UART_ReceiveChar(void);
//
//#endif
