/*
 * PTL V1.0.0
 * Copyright (C) 2026 Group3.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

 
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* PTL_BIT_RELEASE/PTL_BIT_ABORT are delivered via task notifications. */
#define configUSE_TASK_NOTIFICATIONS     1

#define INCLUDE_xTaskGetCurrentTaskHandle 1

/* enables vApplicationTickHook(), which drives prvPTL_SchedulerTick(). */
#define configUSE_TICK_HOOK              1
/* enables vApplicationIdleHook(), used for log flushing and idle-time accounting. */
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
#define configTICK_RATE_HZ              ( ( TickType_t ) 100000 )

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
#define configTIMER_TASK_PRIORITY       configMAX_PRIORITIES - 1
#define configTIMER_QUEUE_LENGTH        10
#define configTIMER_TASK_STACK_DEPTH    ( configMINIMAL_STACK_SIZE * 3 )

/* enables vApplicationStackOverflowHook() as a fatal-error handler. */
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191

#endif
