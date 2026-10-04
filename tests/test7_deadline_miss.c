/*
 * PTL V1.0.0
 * Copyright (C) 2026 Group3.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * test7_deadline_miss.c
 *
 * Purpose: exercise the DEADLINE check in isolation from the OVERRUN check.
 * Jobs run for a random time strictly greater than 0 and strictly less than
 * the period, so xJobRunning is always cleared before the next release
 * (ulOverruns must stay 0), while the execution time is allowed to exceed
 * the (shorter) deadline, so ulDeadlinesMisses is expected to grow.
 */
#include "ptl.h"
#include "test_utils.h"
#include "timers.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DL_N_TASKS 2
#define DL_PERIOD 20   // ms
#define DL_DEADLINE 5 // ms
#define DL_MIN_EXEC 0  // ms

static void VariableExecTask(void *pv) {
  PTL_vTaskDelay(DL_DEADLINE + 5);
  PTL_CheckAbort();
}

static PTL_TaskConfig_t t_dl[DL_N_TASKS];
static PTL_TaskHandle_t *pxHandles[DL_N_TASKS];

static void vDeadlineMissTimeoutCallback(TimerHandle_t xTimer) {
  (void)xTimer;
  BaseType_t xPassed = pdTRUE;

  for (int i = 0; i < DL_N_TASKS; i++) {
    PTL_TaskStats_t xStats;
    PTL_GetTaskStats(pxHandles[i], &xStats);
    vPrintTaskStats(t_dl[i].pcTaskName, &xStats);

    if (!xCheckJitter(&xStats)) {
      xPassed = pdFALSE;
    }
    if (xStats.ulOverruns != 0) {
      xPassed = pdFALSE;
    }
    if (xStats.ulDeadlinesMisses == 0) {
      xPassed = pdFALSE;
    }
    if (xStats.ulDeadlinesMisses > xStats.ulJobCounter) {
      xPassed = pdFALSE;
    }
  }

  vFinishTest("DEADLINE MISS ISOLATION (NO OVERRUN)", xPassed);
}

int main(void) {

#if RANDOM
  srand(time(NULL));
#else
  srand(0xDEADBEEF);
#endif

  UART_init();

  UART_printf("\n[TITLE] DEADLINE MISS ISOLATION (NO OVERRUN)\n");

  SchedulerConfig_t sch_cfg = {
    .maxTasks = DL_N_TASKS,
    .xPolicy = PTL_OVERRUN_SKIP,
    .xTraceEnabled = pdTRUE
    };

  xPTL_Init(&sch_cfg);
  vPTL_PrintPTLCfg(sch_cfg);

  static unsigned int seeds[DL_N_TASKS];
  char buffer[128];

  for (int i = 0; i < DL_N_TASKS; i++) {
    seeds[i] = rand();
    sprintf(buffer, "DeadlineTask_%d", i);
    t_dl[i].pcTaskName = strdup(buffer);
    t_dl[i].PTL_JobFunction_t = VariableExecTask;
    t_dl[i].usStack_size = 256;
    t_dl[i].xPriority = 0;
    t_dl[i].xPeriod = pdMS_TO_TICKS(DL_PERIOD);
    t_dl[i].xDeadline = pdMS_TO_TICKS(DL_DEADLINE);
    t_dl[i].pArg = &seeds[i];

    if (xPTL_CreateTask(&(t_dl[i]), &pxHandles[i]) != PTL_OK) {
      UART_printf("[TEST-FATAL] CreateTask failed\n");
      exit_qemu(1);
    }
    vPTL_PrintTaskParam(t_dl[i]);
  }

  TimerHandle_t xTestTimer = xTimerCreate("Timeout", 
    pdMS_TO_TICKS(TEST_LENGTH), 
    pdFALSE, 
    (void *)0,
    vDeadlineMissTimeoutCallback
    );

  xTimerStart(xTestTimer, 0);

  xPTL_Start(); // Start PTL

  for (;;);

  return 0;
}
