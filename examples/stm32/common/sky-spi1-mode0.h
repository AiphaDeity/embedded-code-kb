#ifndef SKY_SPI1_MODE0_H
#define SKY_SPI1_MODE0_H

#include "stm32f4xx_hal.h"

// 天空星 SPI1 的 PA5 / PA6 / PA7，Mode 0，8 位，每字节高位先行。
static inline HAL_StatusTypeDef sky_spi1_mode0_init(SPI_HandleTypeDef *spi)
{
  // 1. 只配置 SPI 总线。各调用者要在打开 SPI 前准备好自己的片选。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_SPI1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, .Mode = GPIO_MODE_AF_PP,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF5_SPI1,
  };
  HAL_GPIO_Init(GPIOA, &pins);

  // 2. HSI 为 16 MHz 时，PCLK2 /16 得到约 1 MHz SPI 时钟。
  spi->Instance = SPI1;
  spi->Init = (SPI_InitTypeDef){
    .Mode = SPI_MODE_MASTER, .Direction = SPI_DIRECTION_2LINES, .DataSize = SPI_DATASIZE_8BIT,
    .CLKPolarity = SPI_POLARITY_LOW, .CLKPhase = SPI_PHASE_1EDGE,
    .NSS = SPI_NSS_SOFT, .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16,
    .FirstBit = SPI_FIRSTBIT_MSB, .TIMode = SPI_TIMODE_DISABLE,
    .CRCCalculation = SPI_CRCCALCULATION_DISABLE,
  };
  return HAL_SPI_Init(spi);
}

#endif
