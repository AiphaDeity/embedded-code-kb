#include "demo.h"

static TaskHandle_t consumer;
static volatile uint32_t irq_edges, handled_edges, batches, last_batch, max_batch;

void EXTI0_IRQHandler(void)
{
  // 1. 由 HAL 检查来源并清除 EXTI0 挂起位，再进入回调发布通知。
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
  if (pin != GPIO_PIN_0) { return; }
  // 1. 通知值累加一次。若唤醒更高优先级任务，申请在中断返回时切换。
  BaseType_t woken = pdFALSE;
  ++irq_edges;
  vTaskNotifyGiveFromISR(consumer, &woken);
  portYIELD_FROM_ISR(woken);
}

static void button_task(void *argument)
{
  (void)argument;
  // 1. 调度已开始，consumer 已有效。先清除旧挂起，再开放 PA0 上升沿中断。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef key = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_IT_RISING, .Pull = GPIO_NOPULL};
  HAL_GPIO_Init(GPIOA, &key);
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
  HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
  HAL_NVIC_SetPriority(EXTI0_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  for (;;)
  {
    // 2. 阻塞直到有通知。一次取走累计值并清零，随后到的中断留给下一批。
    const uint32_t count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    last_batch = count;
    if (count > max_batch) { max_batch = count; }
    handled_edges += count;
    ++batches;
    if ((count & 1U) != 0U) { HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2); }

    // 3. 模拟任务处理其他工作。延时期间到达的通知仍会累积。
    vTaskDelay(pdMS_TO_TICKS(50U));
  }
}

int main(void)
{
  // 1. 配置 LED 并保存唯一接收者的句柄，开放中断的动作由调度后的任务完成。
  demo_init();
  configASSERT(xTaskCreate(button_task, "button", 256U, NULL, 2U, &consumer) == pdPASS);
  vTaskStartScheduler();
  demo_fault();
}
