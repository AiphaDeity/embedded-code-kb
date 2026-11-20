#include <stdbool.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "../../common/clock84.h"
#include "lwip/init.h"
#include "lwip/etharp.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "echo.h"

enum { FRAME_BYTES = 1536, RX_BUFFERS = ETH_RX_DESC_CNT + 4, LINK_POLL_MS = 200 };
enum { LAN_REG_BCR = 0, LAN_REG_BSR = 1, LAN_REG_ID1 = 2, LAN_REG_ID2 = 3, LAN_REG_SCSR = 31 };

typedef struct
{
  struct pbuf_custom custom;
  _Alignas(8) uint8_t data[FRAME_BYTES];
  bool in_use;
} RxBuffer;

static ETH_HandleTypeDef eth;
static _Alignas(4) ETH_DMADescTypeDef rx_desc[ETH_RX_DESC_CNT], tx_desc[ETH_TX_DESC_CNT];
static _Alignas(4) uint8_t tx_data[FRAME_BYTES];
static RxBuffer pool[RX_BUFFERS];
static uint8_t mac[6] = {0x02U, 0x4BU, 0x42U, 0x00U, 0x00U, 0x02U};
static struct netif ethernet;
static uint32_t phy;
static bool running, tx_latched;
static EchoStats echo_stats;
static volatile unsigned stage, rx_frames, no_rx_buffer, dropped_frames, tx_faults, phy_mode;
static const char *volatile assert_reason;

void SysTick_Handler(void) { HAL_IncTick(); }
u32_t sys_now(void) { return HAL_GetTick(); }

_Noreturn void lwip_platform_assert(const char *message)
{
  assert_reason = message;
  for (;;) { __WFI(); }
}

static void release_rx(struct pbuf *packet)
{
  // 1. pbuf 与 pbuf_custom 都是宿主的首成员。最后一个引用释放才归还缓冲。
  RxBuffer *slot = (RxBuffer *)packet;
  slot->in_use = false;
}

void HAL_ETH_RxAllocateCallback(uint8_t **buffer)
{
  // 1. HAL 重建描述符时借出空闲内存。池满时返回空指针，等待下一次回收。
  for (unsigned i = 0; i < RX_BUFFERS; ++i)
  {
    if (!pool[i].in_use)
    {
      pool[i].in_use = true;
      *buffer = pool[i].data;
      return;
    }
  }
  *buffer = NULL;
  ++no_rx_buffer;
}

void HAL_ETH_RxLinkCallback(void **start, void **end, uint8_t *buffer, uint16_t count)
{
  // 1. 描述符把段交给 lwIP。自定义 pbuf 引用 DMA 缓冲，必要时加入同一条帧的链。
  for (unsigned i = 0; i < RX_BUFFERS; ++i)
  {
    RxBuffer *slot = &pool[i];
    if (slot->data != buffer) { continue; }
    slot->custom.custom_free_function = release_rx;
    struct pbuf *packet = pbuf_alloced_custom(PBUF_RAW, count, PBUF_REF, &slot->custom,
                                               slot->data, sizeof(slot->data));
    if (packet == NULL) { lwip_platform_assert("invalid RX segment"); }
    if (*start == NULL) { *start = packet; }
    else { pbuf_cat(*start, packet); }
    *end = packet;
    return;
  }
  lwip_platform_assert("unknown RX buffer");
}

static err_t link_output(struct netif *net, struct pbuf *packet)
{
  (void)net;
  // 1. 将分段 pbuf 拷到独立 TX 缓冲，阻塞发送完成以前留给 DMA。
  const u16_t size = packet->tot_len;
  if (!running || tx_latched) { return ERR_IF; }
  if (size == 0U || size > sizeof(tx_data) ||
      pbuf_copy_partial(packet, tx_data, size, 0U) != size)
  {
    return ERR_BUF;
  }
  ETH_BufferTypeDef segment = {.buffer = tx_data, .len = size};
  ETH_TxPacketConfigTypeDef config = {
    .Length = size, .TxBuffer = &segment, .Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD,
    .CRCPadCtrl = ETH_CRC_PAD_INSERT,
  };

  // 2. 失败后转入锁定，避免覆盖可能仍被 DMA 使用的数据。
  if (HAL_ETH_Transmit(&eth, &config, 20U) != HAL_OK)
  {
    tx_latched = true;
    ++tx_faults;
    return ERR_IF;
  }
  return ERR_OK;
}

static err_t ethernet_setup(struct netif *net)
{
  net->name[0] = 'e';
  net->name[1] = 'n';
  net->output = etharp_output;
  net->linkoutput = link_output;
  net->hwaddr_len = ETH_HWADDR_LEN;
  memcpy(net->hwaddr, mac, sizeof(mac));
  net->mtu = 1500U;
  net->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
  return ERR_OK;
}

void HAL_ETH_MspInit(ETH_HandleTypeDef *handle)
{
  if (handle->Instance != ETH) { return; }
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_ETH_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7, .Mode = GPIO_MODE_AF_PP,
    .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Alternate = GPIO_AF11_ETH,
  };
  HAL_GPIO_Init(GPIOA, &pins);
  pins.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
  HAL_GPIO_Init(GPIOB, &pins);
  pins.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
  HAL_GPIO_Init(GPIOC, &pins);
}

static bool phy_reset_and_init(void)
{
  // 1. 先释放底板 PHY 复位并等 REF_CLK 稳定，再初始化 MAC。
  __HAL_RCC_GPIOE_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_RESET);
  GPIO_InitTypeDef reset = {.Pin = GPIO_PIN_12, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOE, &reset);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_SET);
  HAL_Delay(50U);
  eth.Instance = ETH;
  eth.Init = (ETH_InitTypeDef){.MACAddr = mac, .MediaInterface = ETH_MEDIA_INTERFACE_RMII,
                              .RxDesc = rx_desc, .TxDesc = tx_desc, .RxBuffLen = FRAME_BYTES};
  if (HAL_ETH_Init(&eth) != HAL_OK) { return false; }
  HAL_ETH_SetMDIOClockRange(&eth);

  // 2. 在管理总线上定位 LAN8720A，识别以后启动自动协商。
  for (uint32_t addr = 0; addr < 32U; ++addr)
  {
    uint32_t high, low, control;
    if (HAL_ETH_ReadPHYRegister(&eth, addr, LAN_REG_ID1, &high) == HAL_OK && high == 0x0007U &&
        HAL_ETH_ReadPHYRegister(&eth, addr, LAN_REG_ID2, &low) == HAL_OK && (low & 0xFFF0U) == 0xC0F0U &&
        HAL_ETH_ReadPHYRegister(&eth, addr, LAN_REG_BCR, &control) == HAL_OK)
    {
      phy = addr;
      return HAL_ETH_WritePHYRegister(&eth, phy, LAN_REG_BCR, control | 0x1200U) == HAL_OK;
    }
  }
  return false;
}

static void sync_link(void)
{
  // 1. 连读两次状态寄存器越过锁存值，协商完成才使用速率和单双工。
  uint32_t status, mode;
  const bool active = !tx_latched &&
    HAL_ETH_ReadPHYRegister(&eth, phy, LAN_REG_BSR, &status) == HAL_OK &&
    HAL_ETH_ReadPHYRegister(&eth, phy, LAN_REG_BSR, &status) == HAL_OK && (status & 0x0024U) == 0x0024U &&
    HAL_ETH_ReadPHYRegister(&eth, phy, LAN_REG_SCSR, &mode) == HAL_OK;
  const unsigned speed_code = active ? (mode >> 2) & 7U : 0U;
  const bool valid = speed_code == 1U || speed_code == 2U || speed_code == 5U || speed_code == 6U;
  if (running && (!valid || speed_code != phy_mode))
  {
    netif_set_link_down(&ethernet);
    if (HAL_ETH_Stop(&eth) != HAL_OK) { tx_latched = true; }
    running = false;
    phy_mode = 0U;
  }
  if (!valid || running || tx_latched) { return; }

  // 2. 把协商结果写给 MAC。启动成功以后才向 lwIP 发布链路就绪。
  ETH_MACConfigTypeDef config;
  if (HAL_ETH_GetMACConfig(&eth, &config) != HAL_OK) { return; }
  config.Speed = speed_code == 2U || speed_code == 6U ? ETH_SPEED_100M : ETH_SPEED_10M;
  config.DuplexMode = speed_code == 5U || speed_code == 6U ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
  if (HAL_ETH_SetMACConfig(&eth, &config) == HAL_OK && HAL_ETH_Start(&eth) == HAL_OK)
  {
    phy_mode = speed_code;
    running = true;
    netif_set_link_up(&ethernet);
  }
}

int main(void)
{
  // 1. 准备时钟和网卡，给裸机 lwIP 注册固定地址与 UDP 应用。
  HAL_Init();
  stage = 1U;
  if (sky_clock84() != HAL_OK || !phy_reset_and_init()) { for (;;) { __WFI(); } }
  stage = 2U;
  lwip_init();
  ip4_addr_t addr, mask, gateway;
  IP4_ADDR(&addr, 192, 168, 77, 2);
  IP4_ADDR(&mask, 255, 255, 255, 0);
  IP4_ADDR(&gateway, 0, 0, 0, 0);
  if (netif_add(&ethernet, &addr, &mask, &gateway, NULL, ethernet_setup, ethernet_input) == NULL ||
      start_udp_echo(&echo_stats) == NULL)
  {
    for (;;) { __WFI(); }
  }
  netif_set_default(&ethernet);
  netif_set_up(&ethernet);
  stage = 0U;
  uint32_t last_poll = HAL_GetTick() - LINK_POLL_MS;

  for (;;)
  {
    // 2. 检查链路，每轮限量收包。网卡收到的帧交给 lwIP，未被接管时由 main 释放。
    const uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last_poll) >= LINK_POLL_MS) { last_poll = now; sync_link(); }
    for (unsigned i = 0; running && !tx_latched && i < 8U; ++i)
    {
      void *received;
      if (HAL_ETH_ReadData(&eth, &received) != HAL_OK) { break; }
      struct pbuf *packet = received;
      ++rx_frames;
      if (ethernet.input(packet, &ethernet) != ERR_OK) { ++dropped_frames; pbuf_free(packet); }
    }

    // 3. 网络输入、UDP 回调和超时都由当前主循环推进。
    sys_check_timeouts();
    HAL_Delay(1U);
  }
}
