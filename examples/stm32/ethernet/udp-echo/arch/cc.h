#ifndef SKY_LWIP_ARCH_CC_H
#define SKY_LWIP_ARCH_CC_H
#define LWIP_PLATFORM_DIAG(message) do { } while (0)
_Noreturn void lwip_platform_assert(const char *message);
#define LWIP_PLATFORM_ASSERT(message) lwip_platform_assert(message)
#endif
