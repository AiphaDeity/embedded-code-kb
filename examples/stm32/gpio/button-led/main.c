#include "stm32f4xx_hal.h"

#define BUTTON_PORT GPIOA
#define BUTTON_PIN GPIO_PIN_0
#define LED_PORT GPIOB
#define LED_PIN GPIO_PIN_2

// 配置天空星核心板的按键输入和 LED 输出。
static void gpio_init(void)
{
  // 1. PA0 和 PB2 分属两个端口。先打开两个端口的时钟，再写 GPIO 寄存器。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // 2. 核心板的下拉电阻让 PA0 松开时为低电平。配置为输入，由主循环读电平。
  GPIO_InitTypeDef pins = {0};
  pins.Pin = BUTTON_PIN;
  pins.Mode = GPIO_MODE_INPUT;
  pins.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BUTTON_PORT, &pins);

  // 3. 先把 PB2 的输出值预置为低，再切为推挽输出，让主循环从熄灯开始控制。
  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
  pins.Pin = LED_PIN;
  pins.Mode = GPIO_MODE_OUTPUT_PP;
  pins.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &pins);
}

// 默认 SysTick 时基供 HAL_Delay 和练习里的 HAL_GetTick 使用。
void SysTick_Handler(void)
{
  HAL_IncTick();
}

int main(void)
{
  // 1. main 先初始化 HAL 时基和引脚。HAL 初始化失败时停在这里，供调试器检查。
  if (HAL_Init() != HAL_OK)
  {
    for (;;)
    {
    }
  }
  gpio_init();

  for (;;)
  {
    // 2. 主循环直接采样 PA0。第 3 步 用这个采样值决定本轮灯的状态。
    const GPIO_PinState button = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);

    // 3. PA0 按下为高，PB2 写高点亮。松开时把采样到的低电平写给 LED。
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, button);

    // 4. 等待至少一个 HAL 时基节拍，再回到第 2 步读取下一次电平。
    HAL_Delay(1U);
  }
}
