#ifndef SKY_LWIP_OPTS_H
#define SKY_LWIP_OPTS_H
#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_ETHERNET 1
#define LWIP_ARP 1
#define LWIP_ICMP 1
#define LWIP_UDP 1
#define LWIP_TCP 0
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_DHCP 0
#define LWIP_DNS 0
#define IP_REASSEMBLY 0
#define IP_FRAG 0
#define ETH_PAD_SIZE 0
#define LWIP_SUPPORT_CUSTOM_PBUF 1
#define MEM_ALIGNMENT 8
#define MEM_SIZE (8U * 1024U)
#define PBUF_POOL_SIZE 8U
#define PBUF_POOL_BUFSIZE 1536U
#endif
