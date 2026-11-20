#include "stm32f4xx_hal.h"

static TIM_HandleTypeDef timer;
static volatile uint32_t updates;
static volatile HAL_StatusTypeDef timer_status;

void SysTick_Handler(void) { HAL_IncTick(); }
void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&timer); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *handle)
{
  if (handle == &timer)
  {
    // 1. HAL 分派本次 TIM2 更新后，由回调计数并翻转 PB2。
    ++updates;
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
  }
}

int main(void)
{
  // 1. 准备板载 LED 和 TIM2 的外设时钟。
  HAL_Init();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOB, &led);

  // 2. 以 1 kHz 计数，每 500 个计数触发一次更新。
  timer.Instance = TIM2;
  timer.Init = (TIM_Base_InitTypeDef){
    .Prescaler = 15999U, .CounterMode = TIM_COUNTERMODE_UP, .Period = 499U,
    .ClockDivision = TIM_CLOCKDIVISION_DIV1, .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
  };
  timer_status = HAL_TIM_Base_Init(&timer);
  if (timer_status != HAL_OK) { for (;;) {} }

  // 3. 清掉初始化产生的更新标志，然后启动中断定时。
  __HAL_TIM_CLEAR_FLAG(&timer, TIM_FLAG_UPDATE);
  HAL_NVIC_SetPriority(TIM2_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  timer_status = HAL_TIM_Base_Start_IT(&timer);
  // 4. 启动成功后由 TIM2 中断驱动回调。启动失败时在这里保留 timer_status。
  for (;;) {}
}
