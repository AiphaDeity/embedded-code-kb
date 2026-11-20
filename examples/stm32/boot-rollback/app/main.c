#include "../state_flash.h"
#include "../../../c/debounce/debounce.h"

static const ImageSlot self = SKY_APP_SLOT;
static volatile bool trial, write_failed;
static volatile uint32_t toggles;
static volatile BootState state;
void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. 从日志了解 Boot 的决定。已确认槽按自身节奏闪动，试跑槽保持硬件超时约束。
  HAL_Init();
  state_flash_init();
  BootJournal snapshot = load_boot_state();
  state = snapshot.state;
  trial = is_trial(state, self);
  IWDG_HandleTypeDef deadline = {.Instance = IWDG};
  if (!trial) { HAL_IWDG_Refresh(&deadline); }

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef pins = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_INPUT};
  HAL_GPIO_Init(GPIOA, &pins);
  pins = (GPIO_InitTypeDef){.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_Init(GPIOB, &pins);
  const bool pressed = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
  const uint32_t started = HAL_GetTick();
  Debounce button = {.stable = pressed, .candidate = pressed, .changed_at = started};
  uint32_t last = started;
  const uint32_t led_period = self == SLOT_A ? 500U : 150U;

  for (;;)
  {
    // 2. 用时基和 LED 实际运行作为演示中的健康条件。试跑时只允许在运行满一秒后确认。
    const uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last) >= led_period)
    {
      last = now;
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
      ++toggles;
    }

    // 3. 真正的新按下事件才提交状态。确认只影响本次槽，请求则让下次重启试跑另一槽。
    const bool raw = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
    if (debounce_update(&button, raw, now, 20U) == KEY_PRESSED)
    {
      if (trial && (uint32_t)(now - started) >= 1000U && toggles != 0U)
      {
        const BootState confirmed = confirmed_state(self);
        if (persist_boot_state(snapshot, confirmed))
        {
          snapshot = load_boot_state();
          state = snapshot.state;
          trial = false;
        }
        else
        {
          write_failed = true;
          snapshot = load_boot_state();
        }
      }
      else if (!trial && confirmed_slot(state) == self)
      {
        const BootState request = self == SLOT_A ? REQUEST_B : REQUEST_A;
        if (persist_boot_state(snapshot, request)) { NVIC_SystemReset(); }
        write_failed = true;
        snapshot = load_boot_state();
      }
    }

    // 4. 持久化确认以前不刷新 IWDG。发生断电或看门狗复位时由 Boot 处理回退。
    if (!trial) { HAL_IWDG_Refresh(&deadline); }
    __WFI();
  }
}
