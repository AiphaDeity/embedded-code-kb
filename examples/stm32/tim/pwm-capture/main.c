#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define COUNTER_HZ 1000000U
#define TIMER_DIVIDER (16000000U / COUNTER_HZ - 1U)

static TIM_HandleTypeDef capture, pwm;
static uint32_t last_capture;
static bool have_previous;
static volatile uint32_t period_ticks, measured_hz, valid_periods, overcaptures;
static volatile uint32_t setup_stage;

void SysTick_Handler(void) { HAL_IncTick(); }
void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&capture); }

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *timer)
{
  if (timer != &capture || timer->Channel != HAL_TIM_ACTIVE_CHANNEL_2)
  {
    return;
  }

  // 1. CCR2 保存上升沿时的 CNT。检测到覆盖时放弃旧起点，重新等两个边沿。
  const uint32_t captured = HAL_TIM_ReadCapturedValue(timer, TIM_CHANNEL_2);
  if (__HAL_TIM_GET_FLAG(timer, TIM_FLAG_CC2OF))
  {
    __HAL_TIM_CLEAR_FLAG(timer, TIM_FLAG_CC2OF);
    ++overcaptures;
    have_previous = false;
    return;
  }

  // 2. 第一个边沿建立起点。之后两次捕获值相减，跨一个 32 位回绕仍得到周期。
  if (have_previous)
  {
    const uint32_t ticks = captured - last_capture;
    if (ticks != 0U)
    {
      period_ticks = ticks;
      measured_hz = COUNTER_HZ / ticks;
      ++valid_periods;
    }
  }
  last_capture = captured;
  have_previous = true;
}

static void gpio_init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef pin = {
    .Pin = GPIO_PIN_1, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLDOWN,
    .Speed = GPIO_SPEED_FREQ_LOW, .Alternate = GPIO_AF1_TIM2,
  };
  HAL_GPIO_Init(GPIOA, &pin);
  pin.Pin = GPIO_PIN_6;
  pin.Pull = GPIO_NOPULL;
  pin.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOA, &pin);
}

static HAL_StatusTypeDef start_capture(void)
{
  // 1. 用 TIM2 的完整 32 位计数区间，1 个计数代表标称 1 us。
  __HAL_RCC_TIM2_CLK_ENABLE();
  capture.Instance = TIM2;
  capture.Init = (TIM_Base_InitTypeDef){
    .Prescaler = TIMER_DIVIDER, .CounterMode = TIM_COUNTERMODE_UP, .Period = UINT32_MAX,
    .ClockDivision = TIM_CLOCKDIVISION_DIV1,
  };
  if (HAL_TIM_IC_Init(&capture) != HAL_OK) { return HAL_ERROR; }

  // 2. 将 CH2 绑定 PA1 上升沿。定时器锁存 CNT 后通过 IRQ 把结果交给回调。
  TIM_IC_InitTypeDef channel = {
    .ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING, .ICSelection = TIM_ICSELECTION_DIRECTTI,
    .ICPrescaler = TIM_ICPSC_DIV1, .ICFilter = 0U,
  };
  if (HAL_TIM_IC_ConfigChannel(&capture, &channel, TIM_CHANNEL_2) != HAL_OK) { return HAL_ERROR; }
  HAL_NVIC_SetPriority(TIM2_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  return HAL_TIM_IC_Start_IT(&capture, TIM_CHANNEL_2);
}

static HAL_StatusTypeDef start_pwm(void)
{
  // 1. TIM3 周期为 1000 个计数，CCR1 为 250。硬件将 PA6 输出 25% 占空比。
  __HAL_RCC_TIM3_CLK_ENABLE();
  pwm.Instance = TIM3;
  pwm.Init = (TIM_Base_InitTypeDef){
    .Prescaler = TIMER_DIVIDER, .CounterMode = TIM_COUNTERMODE_UP, .Period = 999U,
    .ClockDivision = TIM_CLOCKDIVISION_DIV1,
  };
  if (HAL_TIM_PWM_Init(&pwm) != HAL_OK) { return HAL_ERROR; }
  TIM_OC_InitTypeDef channel = {
    .OCMode = TIM_OCMODE_PWM1, .Pulse = 250U, .OCPolarity = TIM_OCPOLARITY_HIGH,
    .OCFastMode = TIM_OCFAST_DISABLE,
  };
  if (HAL_TIM_PWM_ConfigChannel(&pwm, &channel, TIM_CHANNEL_1) != HAL_OK) { return HAL_ERROR; }
  return HAL_TIM_PWM_Start(&pwm, TIM_CHANNEL_1);
}

int main(void)
{
  // 1. 配置引脚并先启动接收端。失败时保留对应阶段供调试器查看。
  HAL_Init();
  gpio_init();
  setup_stage = 1U;
  if (start_capture() != HAL_OK) { for (;;) {} }

  // 2. 输入捕获就绪后才启动 PWM。输出波形由 TIM3 驱动，不占用主循环。
  setup_stage = 2U;
  if (start_pwm() != HAL_OK) { for (;;) {} }
  setup_stage = 0U;

  for (;;)
  {
    // 3. TIM2 回调报告测量值。main 等待中断，调试器可以读取 period_ticks。
    __WFI();
  }
}
