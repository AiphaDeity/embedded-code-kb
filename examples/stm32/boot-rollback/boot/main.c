#include "../state_flash.h"
#include "../../boot-app/boot/image.h"
#include "../../boot-app/boot/handoff.h"

static volatile ImageResult integrity_a, integrity_b;
static volatile BootState boot_state;
static volatile ImageSlot selected;
static volatile bool journal_failed;

void SysTick_Handler(void) { HAL_IncTick(); }

static void wait_for_debugger(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOB, &led);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
  for (;;) { __WFI(); }
}

int main(void)
{
  // 1. 从实际 Flash 读取状态与两个镜像。先判断各自内容能否支持启动。
  HAL_Init();
  state_flash_init();
  const BootJournal snapshot = load_boot_state();
  boot_state = snapshot.state;
  AppEntry a, b;
  integrity_a = validate_at((const uint8_t *)A_HEADER, (const uint8_t *)A_VECTOR,
                            A_VECTOR, A_CAPACITY, &a);
  integrity_b = validate_at((const uint8_t *)B_HEADER, (const uint8_t *)B_VECTOR,
                            B_VECTOR, B_CAPACITY, &b);

  // 2. 试跑前必须确认旧槽可退，并把本次试跑机会记入 Flash。写失败就启动旧槽。
  const BootPlan plan = plan_boot(snapshot.state, integrity_a == IMAGE_OK, integrity_b == IMAGE_OK);
  bool persisted = true;
  if (plan.save_before_jump != snapshot.state)
  {
    persisted = persist_boot_state(snapshot, plan.save_before_jump);
    journal_failed = !persisted;
  }
  selected = chosen_slot(plan, persisted);
  if (selected == SLOT_NONE) { wait_for_debugger(); }

  // 3. Boot 先开启硬件期限。App 已获确认才喂狗，未确认的试跑超时会硬件复位。
  IWDG_HandleTypeDef deadline = {
    .Instance = IWDG, .Init = {.Prescaler = IWDG_PRESCALER_64, .Reload = 4095U},
  };
  if (HAL_IWDG_Init(&deadline) != HAL_OK) { wait_for_debugger(); }
  if (selected == SLOT_A) { handoff_to(A_VECTOR, a); }
  handoff_to(B_VECTOR, b);
}
