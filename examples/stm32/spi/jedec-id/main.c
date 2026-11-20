#include "../../common/sky-spi1-mode0.h"

static SPI_HandleTypeDef spi;
static volatile HAL_StatusTypeDef status;
static volatile uint8_t jedec[3];

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. 片选空闲为高，先写输出锁存器再将 PA4 切为输出。
  HAL_Init();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  GPIO_InitTypeDef chip_select = {.Pin = GPIO_PIN_4, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOA, &chip_select);
  // 2. MCU 作为主机提供 Mode 0 时钟。
  status = sky_spi1_mode0_init(&spi);
  if (status != HAL_OK) { for (;;) {} }
  const uint8_t request[] = {0x9FU, 0, 0, 0};

  for (;;)
  {
    // 3. 一次事务从片选拉低开始。收到的数据在完成后才读取。
    uint8_t reply[sizeof(request)];
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(&spi, request, reply, sizeof(reply), 20U);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    if (status == HAL_OK)
    {
      for (uint32_t i = 0; i < sizeof(jedec); ++i) { jedec[i] = reply[i + 1U]; }
    }
    HAL_Delay(250U);
  }
}
