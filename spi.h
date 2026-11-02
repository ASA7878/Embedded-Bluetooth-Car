#ifndef SPI_H
#define SPI_H

#include<stdint.h>
void clock(void);
void gpio_spi_init(void);
void spi_init(void);
void cs_enable(void);
void cs_disable(void);
void SPI1_Send(uint8_t data);
uint8_t SPI1_Receive(void);
void Accel_WriteReg(uint8_t reg, uint8_t value);
uint8_t Accel_ReadReg(uint8_t reg);

#endif









