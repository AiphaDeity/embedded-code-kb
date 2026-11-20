#ifndef SKY_UDP_ECHO_H
#define SKY_UDP_ECHO_H
#include "lwip/udp.h"

enum { ECHO_PORT = 7007, ECHO_MAX = 256 };
typedef struct { volatile unsigned received, sent, rejected; } EchoStats;

static inline void echo_receive(void *arg, struct udp_pcb *pcb, struct pbuf *packet,
                                const ip_addr_t *peer, u16_t port)
{
  EchoStats *stats = arg;
  if (packet == NULL) { return; }

  // 1. 本回调持有接收 pbuf。符合长度的原样回复给同一端点，过长或发送失败记为拒绝。
  ++stats->received;
  if (packet->tot_len <= ECHO_MAX && udp_sendto(pcb, packet, peer, port) == ERR_OK)
  {
    ++stats->sent;
  }
  else
  {
    ++stats->rejected;
  }

  // 2. UDP 发送提交后释放接收引用。MAC 的发送使用单独内存。
  pbuf_free(packet);
}

static inline struct udp_pcb *start_udp_echo(EchoStats *stats)
{
  struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_V4);
  if (pcb == NULL) { return NULL; }
  if (udp_bind(pcb, IP_ADDR_ANY, ECHO_PORT) != ERR_OK)
  {
    udp_remove(pcb);
    return NULL;
  }
  udp_recv(pcb, echo_receive, stats);
  return pcb;
}
#endif
