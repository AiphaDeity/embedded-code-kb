#include "stm32f4xx_hal.h"

#define BUTTON_PORT GPIOA
#define BUTTON_PIN GPIO_PIN_0
#define LED_PORT GPIOB
#define LED_PIN GPIO_PIN_2

typedef struct
{
  uint32_t gpio_gate_bits;
  uint32_t sysclk_hz;
  uint32_t hclk_hz;
  uint32_t pclk1_hz;
  uint32_t pclk2_hz;
  uint32_t systick_load;
} RccSnapshot;

static volatile RccSnapshot rcc_before_gates;
static volatile RccSnapshot rcc_after_gates;
static volatile RccSnapshot rcc_after_divider;
static volatile HAL_StatusTypeDef rcc_clock_status;

// 记录 GPIO 时钟使能位、HAL 根据时钟配置计算的频率，以及 SysTick 重装值。
static RccSnapshot capture_rcc(void)
{
  return (RccSnapshot){
    .gpio_gate_bits = RCC->AHB1ENR & (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN),
    .sysclk_hz = HAL_RCC_GetSysClockFreq(),
    .hclk_hz = HAL_RCC_GetHCLKFreq(),
    .pclk1_hz = HAL_RCC_GetPCLK1Freq(),
    .pclk2_hz = HAL_RCC_GetPCLK2Freq(),
    .systick_load = SysTick->LOAD,
  };
}

// 调用前已打开 GPIOA 和 GPIOB 的时钟。
static void configure_button_and_led(void)
{
  // 1. PA0 用板载下拉保持松开时的低电平，主循环按高电平判断按下。
  GPIO_InitTypeDef pins = {0};
  pins.Pin = BUTTON_PIN;
  pins.Mode = GPIO_MODE_INPUT;
  pins.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BUTTON_PORT, &pins);

  // 2. 先给 PB2 写低电平，再切换到推挽输出，让 LED 从熄灭状态开始。
  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
  pins.Pin = LED_PIN;
  pins.Mode = GPIO_MODE_OUTPUT_PP;
  pins.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &pins);
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

int main(void)
{
  // 1. 从复位启动，保留默认 HSI 时钟。建立 HAL 时基后，记下两个 GPIO 门控的初态。
  HAL_Init();
  rcc_before_gates = capture_rcc();

  // 2. 先打开 GPIOA 和 GPIOB 的时钟，再配置按键和 LED。此时总线分频仍和复位后一致。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  rcc_after_gates = capture_rcc();
  configure_button_and_led();

  // 3. SYSCLK 继续使用 HSI，只将 AHB 分频设为 /2。HAL 会按新的 HCLK 更新 SysTick。
  const RCC_ClkInitTypeDef clocks = {
    .ClockType = RCC_CLOCKTYPE_HCLK,
    .AHBCLKDivider = RCC_SYSCLK_DIV2,
  };
  rcc_clock_status = HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_0);
  if (rcc_clock_status != HAL_OK)
  {
    for (;;)
    {
      // 记录返回值后停在切换流程里，方便查看 RCC 和 Flash 配置。
    }
  }
  rcc_after_divider = capture_rcc();

  for (;;)
  {
    // 4. 用同一个按键点灯流程观察分频后的 GPIO。回到这一行可同时查看三份时钟快照。
    const GPIO_PinState button = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, button);
    HAL_Delay(1U);
  }
}
