/*
 * DEMO - PTL
 * Group3 - 2026
 *
 * FreeRTOSConfig.h DEMO
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_TASK_NOTIFICATIONS     1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define configUSE_TICK_HOOK              1
#define configUSE_IDLE_HOOK              1
#define configSUPPORT_STATIC_ALLOCATION  0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configUSE_MALLOC_FAILED_HOOK     1
#define INCLUDE_vTaskDelete              1
#define INCLUDE_vTaskSuspend             1
#define configUSE_TICKLESS_IDLE          0

/* Cortex-M */
#define SVC_Handler     vPortSVCHandler
#define PendSV_Handler  xPortPendSVHandler
#define SysTick_Handler xPortSysTickHandler


#define tskIDLE_PRIORITY                ( ( UBaseType_t ) 0U )
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1

#define configCPU_CLOCK_HZ              ( ( unsigned long ) 50000000 )
#define configTICK_RATE_HZ              ( ( TickType_t ) 1000 )

#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        128
#define configTOTAL_HEAP_SIZE           ( 64 * 1024 )
#define configMAX_TASK_NAME_LEN         20
#define configUSE_16_BIT_TICKS          0

#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1

#define configUSE_TRACE_FACILITY        1
#define INCLUDE_eTaskGetState            1

#define configUSE_TIMERS                1
#define configTIMER_TASK_PRIORITY       1
#define configTIMER_QUEUE_LENGTH        10
#define configTIMER_TASK_STACK_DEPTH    ( configMINIMAL_STACK_SIZE * 3 )

#define configCHECK_FOR_STACK_OVERFLOW  2

#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_vTaskCleanUpResources   0
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelay              1

#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191

#endif /* FREERTOS_CONFIG_H */