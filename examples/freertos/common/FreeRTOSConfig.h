#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f4xx.h"

void demo_fault(void);

#define configCPU_CLOCK_HZ SystemCoreClock
#define configTICK_RATE_HZ 1000U
#define configUSE_PREEMPTION 1
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE 128
#define configTOTAL_HEAP_SIZE (8U * 1024U)
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_16_BIT_TICKS 0
#define configUSE_MUTEXES 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_xTaskGetSchedulerState 1
#define configPRIO_BITS __NVIC_PRIO_BITS
#define configKERNEL_INTERRUPT_PRIORITY (15U << (8U - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5U << (8U - configPRIO_BITS))
#define configASSERT(expr) do { if (!(expr)) { demo_fault(); } } while (0)
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler

#ifdef SKY_USE_STATIC_ALLOCATION
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 0
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#endif

#ifdef SKY_USE_TIMERS
#define configUSE_TIMERS 1
#define configTIMER_TASK_PRIORITY 3U
#define configTIMER_QUEUE_LENGTH 5U
#define configTIMER_TASK_STACK_DEPTH 256U
#endif

#ifdef SKY_USE_RUN_TIME_STATS
void run_time_clock_init(void);
#define configUSE_TRACE_FACILITY 1
#define configGENERATE_RUN_TIME_STATS 1
#define INCLUDE_xTaskGetIdleTaskHandle 1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() run_time_clock_init()
#define portGET_RUN_TIME_COUNTER_VALUE() (TIM2->CNT)
#endif

#endif
