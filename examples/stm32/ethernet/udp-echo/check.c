#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/init.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "echo.h"

static const uint8_t host_mac[6] = {2U, 0x4BU, 0x42U, 0U, 0U, 1U};
static const uint8_t board_mac[6] = {2U, 0x4BU, 0x42U, 0U, 0U, 2U};
static const uint8_t host_ip[4] = {192U, 168U, 77U, 1U};
static const uint8_t board_ip[4] = {192U, 168U, 77U, 2U};
static uint8_t captured[1536];
static size_t captured_size;
static unsigned transmissions;

u32_t sys_now(void) { return 0U; }

_Noreturn void lwip_platform_assert(const char *message)
{
  fprintf(stderr, "%s\n", message);
  abort();
}

static void be16(uint8_t *out, unsigned value)
{
  out[0] = (uint8_t)(value >> 8);
  out[1] = (uint8_t)value;
}

static unsigned get16(const uint8_t *in)
{
  return ((unsigned)in[0] << 8) | in[1];
}

static unsigned ipv4_checksum(const uint8_t *header)
{
  unsigned sum = 0U;
  for (unsigned i = 0; i < 20U; i += 2U) { sum += get16(header + i); }
  while (sum > 0xFFFFU) { sum = (sum & 0xFFFFU) + (sum >> 16); }
  return (~sum) & 0xFFFFU;
}

static err_t capture_frame(struct netif *network, struct pbuf *frame)
{
  (void)network;
  assert(frame->tot_len <= sizeof(captured));
  captured_size = pbuf_copy_partial(frame, captured, frame->tot_len, 0U);
  assert(captured_size == frame->tot_len);
  ++transmissions;
  return ERR_OK;
}

static err_t attach_mock(struct netif *network)
{
  network->name[0] = 's';
  network->name[1] = 'w';
  network->hwaddr_len = ETH_HWADDR_LEN;
  memcpy(network->hwaddr, board_mac, sizeof(board_mac));
  network->mtu = 1500U;
  network->flags = NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP | NETIF_FLAG_BROADCAST;
  network->output = etharp_output;
  network->linkoutput = capture_frame;
  return ERR_OK;
}

static void inject(struct netif *network, const uint8_t *bytes, u16_t size)
{
  struct pbuf *frame = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
  assert(frame != NULL && pbuf_take(frame, bytes, size) == ERR_OK);
  assert(network->input(frame, network) == ERR_OK);
}

static void inject_arp(struct netif *network)
{
  uint8_t frame[42] = {0};
  memset(frame, 0xFF, 6);
  memcpy(frame + 6, host_mac, 6);
  be16(frame + 12, 0x0806U);
  uint8_t *arp = frame + 14;
  be16(arp, 1U);
  be16(arp + 2, 0x0800U);
  arp[4] = 6U; arp[5] = 4U;
  be16(arp + 6, 1U);
  memcpy(arp + 8, host_mac, 6);
  memcpy(arp + 14, host_ip, 4);
  memcpy(arp + 24, board_ip, 4);
  inject(network, frame, sizeof(frame));
}

static void inject_udp(struct netif *network, const uint8_t *payload, u16_t size)
{
  uint8_t frame[14U + 20U + 8U + ECHO_MAX + 1U] = {0};
  assert(size <= ECHO_MAX + 1U);
  memcpy(frame, board_mac, 6);
  memcpy(frame + 6, host_mac, 6);
  be16(frame + 12, 0x0800U);
  uint8_t *ip = frame + 14;
  ip[0] = 0x45U;
  be16(ip + 2, 20U + 8U + size);
  ip[8] = 64U; ip[9] = 17U;
  memcpy(ip + 12, host_ip, 4);
  memcpy(ip + 16, board_ip, 4);
  be16(ip + 10, ipv4_checksum(ip));
  uint8_t *udp = ip + 20;
  be16(udp, 40000U);
  be16(udp + 2, ECHO_PORT);
  be16(udp + 4, 8U + size);
  memcpy(udp + 8, payload, size);
  inject(network, frame, 14U + 20U + 8U + size);
}

int main(void)
{
  lwip_init();
  struct netif network;
  ip4_addr_t addr, mask, gateway;
  IP4_ADDR(&addr, 192, 168, 77, 2);
  IP4_ADDR(&mask, 255, 255, 255, 0);
  IP4_ADDR(&gateway, 0, 0, 0, 0);
  assert(netif_add(&network, &addr, &mask, &gateway, NULL, attach_mock, ethernet_input) != NULL);
  netif_set_default(&network);
  netif_set_up(&network);
  netif_set_link_up(&network);
  EchoStats stats = {0};
  struct udp_pcb *pcb = start_udp_echo(&stats);
  assert(pcb != NULL);

  // 1. 用真实 lwIP 收 ARP 请求，确认它在同一模拟网口发出 ARP 应答。
  unsigned before = transmissions;
  inject_arp(&network);
  assert(transmissions == before + 1U && get16(captured + 20U) == 2U);
  assert(memcmp(captured, host_mac, sizeof(host_mac)) == 0);

  // 2. 将 UDP 交给同一裸机服务，并检查返回帧端口、长度和二进制负载。
  before = transmissions;
  const uint8_t sample[] = {0x41U, 0x00U, 0x7EU};
  inject_udp(&network, sample, sizeof(sample));
  assert(transmissions == before + 1U && captured_size == 42U + sizeof(sample));
  assert(get16(captured + 34U) == ECHO_PORT && get16(captured + 36U) == 40000U);
  assert(memcmp(captured + 42U, sample, sizeof(sample)) == 0);

  // 3. 超长数据负载进入回调但不回显。清理 PCB 和虚拟网口后结束。
  uint8_t oversized[ECHO_MAX + 1U] = {0};
  inject_udp(&network, oversized, sizeof(oversized));
  assert(transmissions == before + 1U && stats.received == 2U && stats.sent == 1U && stats.rejected == 1U);
  udp_remove(pcb);
  netif_remove(&network);
  puts("lwIP Ethernet: ARP, binary UDP echo, oversize rejected");
  return 0;
}
