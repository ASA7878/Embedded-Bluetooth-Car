/*
 * interrupt driven
 *
 * In this the uart handler function is used instead of main to take the command
 */



#include <stdint.h>

/* ================= RCC ================= */
#define RCC_BASE        0x40023800UL

#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40))

/* ================= GPIOA ================= */
#define GPIOA_BASE      0x40020000UL

#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20))

/* ================= USART2 ================= */
#define USART2_BASE     0x40004400UL

#define USART2_SR       (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR       (*(volatile uint32_t *)(USART2_BASE + 0x04))
#define USART2_BRR      (*(volatile uint32_t *)(USART2_BASE + 0x08))
#define USART2_CR1      (*(volatile uint32_t *)(USART2_BASE + 0x0C))

/* ================= NVIC (Interrupt Controller) ================= */
// USART2 is IRQ 38. 38 / 32 = 1 (ISER1). 38 % 32 = 6 (Bit 6).
#define NVIC_ISER1      (*(volatile uint32_t *)(0xE000E100UL + 0x04))


void UART_Init(void)
{
    /* Enable GPIOA clock */
    RCC_AHB1ENR |= (1U << 0);

    /* Enable USART2 clock */
    RCC_APB1ENR |= (1U << 17);

    /*
     * PA2 -> Alternate Function 7
     * PA3 -> Alternate Function 7
     */
    GPIOA_MODER &= ~((3U << 4) | (3U << 6));
    GPIOA_MODER |= ((2U << 4) | (2U << 6));

    GPIOA_AFRL &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA_AFRL |= ((7U << 8) | (7U << 12));

    /*
     * Baud rate = 9600
     * Assuming USART2 clock = 16 MHz
     * 0x68 mantissa and fraction = 3
     */
    USART2_BRR = 0x0683;

    /*
     * Enable USART (Bit 13)
     * Enable transmitter (Bit 3)
     * Enable receiver (Bit 2)
     * Enable RXNE Interrupt (Bit 5)
     */
    USART2_CR1 |= (1U << 13) | (1U << 3) | (1U << 2) | (1U << 5);

    /* Enable USART2 global interrupt in the NVIC */
    NVIC_ISER1 |= (1U << 6);
}


/* ================= UART Send Character ================= */
void UART_SendChar(char ch)
{
    /* Wait until transmit data register is empty */
    while(!(USART2_SR & (1U << 7)));

    /* Send character */
    USART2_DR = ch;
}

/* ================= UART Send String ================= */
void UART_SendString(char *str)
{
    while(*str)
    {
        UART_SendChar(*str);
        str++;
    }
}

/* ================= Integer to ASCII ================= */
void UART_SendNumber(uint32_t num)
{
    char buffer[10];
    int i = 0;

    /* Special case for zero */
    if(num == 0)
    {
        UART_SendChar('0');
        return;
    }

    /* Convert number to reverse ASCII */
    while(num > 0)
    {
        buffer[i++] = (num % 10) + '0';
        num = num / 10;
    }

    /* Send in correct order */
    while(i > 0)
    {
        UART_SendChar(buffer[--i]);
    }
}



///*
//
// * polling driven  uart
// *
// * which we call all the function in while loop inside the int main
//
//*/
//#include <stdint.h>
//
///* ================= RCC ================= */
//#define RCC_BASE        0x40023800UL
//
//#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
//#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40))
//
///* ================= GPIOA ================= */
//#define GPIOA_BASE      0x40020000UL
//
//#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
//#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
//
///* ================= USART2 ================= */
//
//#define USART2_BASE     0x40004400UL
//
//#define USART2_SR       (*(volatile uint32_t *)(USART2_BASE + 0x00))
//#define USART2_DR       (*(volatile uint32_t *)(USART2_BASE + 0x04))
//#define USART2_BRR      (*(volatile uint32_t *)(USART2_BASE + 0x08))
//#define USART2_CR1      (*(volatile uint32_t *)(USART2_BASE + 0x0C))
//
//
//void delay_ms(uint32_t ms)
//{
//    volatile uint32_t i;
//
//    while(ms--)
//    {
//        for(i = 0; i < 16000; i++)
//        {
//            //__asm volatile ("nop");;
//        }
//    }
//}
//
//
//void UART_Init(void)
//{
//    /* Enable GPIOA clock */
//    RCC_AHB1ENR |= (1U << 0);
//
//    /* Enable USART2 clock */
//    RCC_APB1ENR |= (1U << 17);
//
//    /*
//     * PA2 -> Alternate Function
//     * PA3 -> Alternate Function
//     */
//    GPIOA_MODER &= ~(3U << 4);
//    GPIOA_MODER |= (2U << 4);
//
//    GPIOA_MODER &= ~(3U << 6);
//    GPIOA_MODER |= (2U << 6);
//
//    /*
//     * PA2 -> AF7
//     * PA3 -> AF7
//     */
//    GPIOA_AFRL &= ~(0xFU << 8);
//    GPIOA_AFRL |= (7U << 8);
//
//    GPIOA_AFRL &= ~(0xFU << 12);
//    GPIOA_AFRL |= (7U << 12);
//
//    /*
//     * Baud rate = 9600
//     * Assuming USART2 clock = 16 MHz
//     * 0x68 mantissa and fraction = 3
//     */
//    USART2_BRR = 0x0683;
//
//    /*
//     * Enable USART
//     * Enable transmitter
//     * Enable receiver
//     */
//    USART2_CR1 |= (1U << 13);
//    USART2_CR1 |= (1U << 3);
//    USART2_CR1 |= (1U << 2);
//}
//
//
///* ================= UART Send Character ================= */
//
//void UART_SendChar(char ch)
//{
//    /* Wait until transmit data register is empty */
//    while(!(USART2_SR & (1U << 7)));
//
//    /* Send character */
//    USART2_DR = ch;
//}
//
///* ================= UART Send String ================= */
//
//void UART_SendString(char *str)
//{
//    while(*str)
//    {
//        UART_SendChar(*str);
//        str++;
//    }
//}
//
///* ================= Integer to ASCII ================= */
//
//void UART_SendNumber(uint32_t num)
//{
//    char buffer[10];
//    int i = 0;
//
//    /* Special case for zero */
//    if(num == 0)
//    {
//        UART_SendChar('0');
//        return;
//    }
//
//    /* Convert number to reverse ASCII */
//    while(num > 0)
//    {
//        buffer[i++] = (num % 10) + '0';
//        num = num / 10;
//    }
//
//    /* Send in correct order */
//    while(i > 0)
//    {
//        UART_SendChar(buffer[--i]);
//    }
//}
//
///* ================= UART Check Data Available ================= */
//
//uint8_t UART_DataAvailable(void)
//{
//    /* Check if Read Data Register is Not Empty (RXNE bit 5) */
//    return (USART2_SR & (1U << 5));
//}
//
///* ================= UART Receive Character ================= */
//
//char UART_ReceiveChar(void)
//{
//    /* Wait until data is available (RXNE bit 5) */
//    while(!(USART2_SR & (1U << 5)));
//
//    /* Read and return the received character */
//    return (char)(USART2_DR & 0xFF);
//}
