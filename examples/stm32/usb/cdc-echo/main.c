#include <stdbool.h>
#include "usbd_core.h"
#include "usbd_cdc.h"

typedef enum { RX_ARMED, TX_PENDING, TX_ACTIVE, RX_REARM } EchoState;
extern USBD_DescriptorsTypeDef sky_usb_descriptors;
static USBD_HandleTypeDef device;
static USBD_CDC_HandleTypeDef class_storage;
static bool class_used;
static _Alignas(4) uint8_t packet[CDC_DATA_FS_MAX_PACKET_SIZE];
static uint8_t coding[7] = {0x00U, 0xC2U, 0x01U, 0x00U, 0U, 0U, 8U};
static volatile EchoState phase;
static volatile uint16_t pending_size;
static volatile uint32_t rx_packets, tx_packets, start_failures, stage;

void SysTick_Handler(void) { HAL_IncTick(); }

void *usb_class_allocate(size_t size)
{
  if (class_used || size > sizeof(class_storage)) { return NULL; }
  class_used = true;
  return &class_storage;
}

void usb_class_free(void *memory)
{
  if (memory == &class_storage) { class_used = false; }
}

static int8_t cdc_init(void)
{
  // 1. 枚举配置生效后，把唯一的数据包缓冲交给 OUT。首次收包由类驱动启动。
  phase = RX_ARMED;
  pending_size = 0U;
  return (int8_t)USBD_CDC_SetRxBuffer(&device, packet);
}

static int8_t cdc_deinit(void)
{
  phase = RX_ARMED;
  pending_size = 0U;
  return USBD_OK;
}

static int8_t cdc_control(uint8_t request, uint8_t *data, uint16_t length)
{
  // 1. 这是虚拟串口的主机设置，存取 7 字节 line coding。USB 链路速率由硬件决定。
  if (request == CDC_SET_LINE_CODING && length == sizeof(coding))
  {
    memcpy(coding, data, sizeof(coding));
  }
  if (request == CDC_GET_LINE_CODING && length == sizeof(coding))
  {
    memcpy(data, coding, sizeof(coding));
  }
  return USBD_OK;
}

static int8_t cdc_receive(uint8_t *data, uint32_t *length)
{
  // 1. OUT 已接收完。保留当前缓冲，主循环接手发送期间先不重挂 OUT。
  if (data == packet && *length != 0U && *length <= sizeof(packet))
  {
    pending_size = (uint16_t)*length;
    ++rx_packets;
    phase = TX_PENDING;
  }
  else
  {
    phase = RX_REARM;
  }
  return USBD_OK;
}

static int8_t cdc_transmitted(uint8_t *data, uint32_t *length, uint8_t endpoint)
{
  (void)length;
  // 1. IN 连同需要的零长包都结束后，主循环才可把缓冲交还 OUT。
  if (data == packet && (endpoint & 0x7FU) == (CDC_IN_EP & 0x7FU) && phase == TX_ACTIVE)
  {
    ++tx_packets;
    phase = RX_REARM;
  }
  return USBD_OK;
}

static USBD_CDC_ItfTypeDef cdc_callbacks = {
  .Init = cdc_init, .DeInit = cdc_deinit, .Control = cdc_control,
  .Receive = cdc_receive, .TransmitCplt = cdc_transmitted,
};

static bool usb_clock(void)
{
  // 1. 天空星 HSE 8 MHz。PLL P 输出 84 MHz，Q 输出 USB 所需的 48 MHz。
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  RCC_OscInitTypeDef oscillator = {
    .OscillatorType = RCC_OSCILLATORTYPE_HSE, .HSEState = RCC_HSE_ON,
    .PLL = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE,
            .PLLM = 8U, .PLLN = 336U, .PLLP = RCC_PLLP_DIV4, .PLLQ = 7U},
  };
  if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) { return false; }
  RCC_ClkInitTypeDef clock = {
    .ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
    .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK, .AHBCLKDivider = RCC_SYSCLK_DIV1,
    .APB1CLKDivider = RCC_HCLK_DIV2, .APB2CLKDivider = RCC_HCLK_DIV1,
  };
  return HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_2) == HAL_OK;
}

static void echo_step(void)
{
  // 1. 端点提交时屏蔽 USB 中断，保证类状态、缓冲地址和 phase 一起切换。
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  if (device.dev_state == USBD_STATE_CONFIGURED)
  {
    if (phase == TX_PENDING)
    {
      if (USBD_CDC_SetTxBuffer(&device, packet, pending_size) == USBD_OK &&
          USBD_CDC_TransmitPacket(&device) == USBD_OK)
      {
        phase = TX_ACTIVE;
      }
      else
      {
        ++start_failures;
      }
    }
    else if (phase == RX_REARM)
    {
      if (USBD_CDC_ReceivePacket(&device) == USBD_OK)
      {
        phase = RX_ARMED;
      }
      else
      {
        ++start_failures;
      }
    }
  }
  __set_PRIMASK(mask);
}

int main(void)
{
  // 1. 依次准备时钟、设备、CDC 和接口回调，均成功后接入 USB。
  HAL_Init();
  stage = 1U;
  if (!usb_clock()) { for (;;) {} }
  stage = 2U;
  if (USBD_Init(&device, &sky_usb_descriptors, 0U) != USBD_OK ||
      USBD_RegisterClass(&device, &USBD_CDC) != USBD_OK ||
      USBD_CDC_RegisterInterface(&device, &cdc_callbacks) != USBD_OK ||
      USBD_Start(&device) != USBD_OK)
  {
    for (;;) {}
  }

  // 2. IRQ 负责收发事件，main 只推进待发或重挂，随后等下一次中断或 SysTick。
  stage = 0U;
  for (;;)
  {
    echo_step();
    __WFI();
  }
}
