#include "stm32f4xx_hal.h"
#include "../../common/clock84.h"

#define CALENDAR_MAGIC 0x4B420001U
#define ALARM_DELAY 5U

static RTC_HandleTypeDef rtc;
static volatile unsigned phase, alarms, seconds_of_day, resumed_clock_hz;
void SysTick_Handler(void) { HAL_IncTick(); }
void RTC_Alarm_IRQHandler(void) { HAL_RTC_AlarmIRQHandler(&rtc); }

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *handle)
{
  if (handle == &rtc)
  {
    // 1. 记录 Alarm A 事件。主循环恢复时钟并安排下一次。
    ++alarms;
  }
}

static HAL_StatusTypeDef rtc_init(void)
{
  // 1. 允许访问备份域。已有其他 RTC 时钟源时停下来，保留其日历。
  HAL_PWR_EnableBkUpAccess();
  if ((RCC->BDCR & RCC_BDCR_RTCSEL) != 0U &&
      (RCC->BDCR & RCC_BDCR_RTCSEL) != RCC_RTCCLKSOURCE_LSI)
  {
    return HAL_ERROR;
  }
  RCC_OscInitTypeDef osc = {.OscillatorType = RCC_OSCILLATORTYPE_LSI, .LSIState = RCC_LSI_ON};
  if (HAL_RCC_OscConfig(&osc) != HAL_OK)
  {
    return HAL_ERROR;
  }
  RCC_PeriphCLKInitTypeDef clocks = {
    .PeriphClockSelection = RCC_PERIPHCLK_RTC, .RTCClockSelection = RCC_RTCCLKSOURCE_LSI,
  };
  if (HAL_RCCEx_PeriphCLKConfig(&clocks) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // 2. LSI 标称 32 kHz，以 (127+1) 和 (249+1) 预分频得到约 1 Hz 的日历。
  __HAL_RCC_RTC_ENABLE();
  rtc.Instance = RTC;
  rtc.Init = (RTC_InitTypeDef){
    .HourFormat = RTC_HOURFORMAT_24, .AsynchPrediv = 127U, .SynchPrediv = 249U,
    .OutPut = RTC_OUTPUT_DISABLE, .OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH,
    .OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN,
  };
  if (HAL_RTC_Init(&rtc) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_RTCEx_BKUPRead(&rtc, RTC_BKP_DR0) != CALENDAR_MAGIC)
  {
    RTC_TimeTypeDef time = {.Hours = 23U, .Minutes = 59U, .Seconds = 55U};
    RTC_DateTypeDef date = {.Year = 26U, .Month = 1U, .Date = 1U, .WeekDay = RTC_WEEKDAY_THURSDAY};
    if (HAL_RTC_SetTime(&rtc, &time, RTC_FORMAT_BIN) != HAL_OK ||
        HAL_RTC_SetDate(&rtc, &date, RTC_FORMAT_BIN) != HAL_OK)
    {
      return HAL_ERROR;
    }
    HAL_RTCEx_BKUPWrite(&rtc, RTC_BKP_DR0, CALENDAR_MAGIC);
  }
  return HAL_RTC_DeactivateAlarm(&rtc, RTC_ALARM_A);
}

static HAL_StatusTypeDef arm_alarm(void)
{
  // 1. 先读时间再读日期，解除 RTC 影子寄存器锁定。此处只用当日秒数。
  RTC_TimeTypeDef now;
  RTC_DateTypeDef date;
  if (HAL_RTC_GetTime(&rtc, &now, RTC_FORMAT_BIN) != HAL_OK ||
      HAL_RTC_GetDate(&rtc, &date, RTC_FORMAT_BIN) != HAL_OK)
  {
    return HAL_ERROR;
  }
  seconds_of_day = now.Hours * 3600U + now.Minutes * 60U + now.Seconds;
  const uint32_t next = (seconds_of_day + ALARM_DELAY) % 86400U;

  // 2. 匹配时分秒，屏蔽日期比较。跨零点时，下一天 00:00 的事件同样可唤醒。
  RTC_AlarmTypeDef alarm = {
    .Alarm = RTC_ALARM_A, .AlarmMask = RTC_ALARMMASK_DATEWEEKDAY,
    .AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL,
    .AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE, .AlarmDateWeekDay = 1U,
    .AlarmTime = {.Hours = next / 3600U, .Minutes = next / 60U % 60U, .Seconds = next % 60U},
  };
  return HAL_RTC_SetAlarm_IT(&rtc, &alarm, RTC_FORMAT_BIN);
}

int main(void)
{
  // 1. 先以 84 MHz 运行，再准备日历、闹钟和板载 LED。
  HAL_Init();
  phase = 1U;
  if (sky_clock84() != HAL_OK) { for (;;) {} }
  phase = 2U;
  if (rtc_init() != HAL_OK) { for (;;) {} }
  HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5U, 0U);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_Init(GPIOB, &led);

  for (;;)
  {
    // 2. 安排下一次闹钟后暂停 SysTick。Alarm IRQ 保留可唤醒资格。
    phase = 3U;
    const unsigned previous = alarms;
    if (arm_alarm() != HAL_OK) { for (;;) {} }
    HAL_SuspendTick();
    while (alarms == previous)
    {
      // 3. 掩码保护条件检查到 WFI 的窗口。闹钟挂起能唤醒，解除掩码后再执行回调。
      __disable_irq();
      if (alarms == previous)
      {
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
      }
      __enable_irq();
    }

    // 4. STOP 醒来先用 HSI。恢复 HAL 时基与 PLL，再关闭当前闹钟并翻灯。
    HAL_ResumeTick();
    phase = 4U;
    if (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK && sky_clock84() != HAL_OK)
    {
      for (;;) {}
    }
    resumed_clock_hz = SystemCoreClock;
    if (HAL_RTC_DeactivateAlarm(&rtc, RTC_ALARM_A) != HAL_OK) { for (;;) {} }
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
  }
}
