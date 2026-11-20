#include "stm32f4xx_hal.h"

enum { FROM_EXTI0 = 1, FROM_EXTI9_5 };
#define SHARED_PINS (GPIO_PIN_6 | GPIO_PIN_7)
#define ALL_LINES (GPIO_PIN_0 | SHARED_PINS)

typedef struct
{
  uint32_t pa0, pb6, pb7;
} EdgeCounts;

static volatile EdgeCounts edges, observed;
static volatile uint32_t first_irq;

void SysTick_Handler(void) { HAL_IncTick(); }

static void record_first_irq(uint32_t source)
{
  // 1. 两个入口的优先级不同。短暂屏蔽可配置中断，保护这次检查和写入。
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  if (first_irq == 0U) { first_irq = source; }
  __set_PRIMASK(mask);
}

void EXTI0_IRQHandler(void)
{
  record_first_irq(FROM_EXTI0);
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI9_5_IRQHandler(void)
{
  // 1. EXTI6 和 EXTI7 共用这条 IRQ。让 HAL 分别检查并清除各线的 pending。
  record_first_irq(FROM_EXTI9_5);
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
  // 1. HAL 已清除该线的挂起位。回调只累计来源，主循环稍后读取。
  switch (pin)
  {
    case GPIO_PIN_0: ++edges.pa0; break;
    case GPIO_PIN_6: ++edges.pb6; break;
    case GPIO_PIN_7: ++edges.pb7; break;
    default: break;
  }
}

static void init_inputs(void)
{
  // 1. PA0 用板载上升沿。PB6 / PB7 各自上拉，外部按键对地下拉触发下降沿。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef pin = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_IT_RISING, .Pull = GPIO_NOPULL};
  HAL_GPIO_Init(GPIOA, &pin);
  pin = (GPIO_InitTypeDef){.Pin = SHARED_PINS, .Mode = GPIO_MODE_IT_FALLING, .Pull = GPIO_PULLUP};
  HAL_GPIO_Init(GPIOB, &pin);
}

int main(void)
{
  // 1. 建立线路和两个优先级。配置后清掉先前状态，避免把初始化阶段当成测量。
  HAL_Init();
  init_inputs();
  __disable_irq();
  __HAL_GPIO_EXTI_CLEAR_IT(ALL_LINES);
  HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
  HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5U, 0U);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  // 2. 同一个写入让三条 EXTI 线先挂起，再允许 NVIC 派发两个入口。
  __HAL_GPIO_EXTI_GENERATE_SWIT(ALL_LINES);
  __DSB();
  __enable_irq();
  __ISB();

  for (;;)
  {
    // 3. 主循环在一个短临界区取三项计数，然后让中断继续。观察值供调试器读取。
    __disable_irq();
    const EdgeCounts current = edges;
    __enable_irq();
    observed = current;
    HAL_Delay(20U);
  }
}
