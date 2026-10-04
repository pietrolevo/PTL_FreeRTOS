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
 * test5_preemption.c
 *
 */
#include "ptl.h"
#include "test_utils.h"
#include "timers.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

#define LOW_PERIOD 50  // ms
#define HIGH_PERIOD 2 // ms

static PTL_TaskHandle_t *pxLowHandle;
static PTL_TaskHandle_t *pxHighHandle;

static void LowPrioTask(void *pv) {
  (void)pv;
  volatile uint32_t ulDummy = 0;
  for (volatile uint32_t i = 0; i < 500000; i++) {
    ulDummy += i; // TOFIX : PROBLEM HERE, QEMU OPTIMIZE THIS ACCORDINGLY TO YOUR PC, SO TO TEST IN OTHER PC
  }
  PTL_CheckAbort();
}

static void HighPrioTask(void *pv) {
  (void)pv;
  PTL_CheckAbort();
}

static void vPreemptionTimeoutCallback(TimerHandle_t xTimer) {
  (void)xTimer;
  BaseType_t xPassed = pdTRUE;

  PTL_TaskStats_t xLowStats, xHighStats;
  PTL_GetTaskStats(pxLowHandle, &xLowStats);
  PTL_GetTaskStats(pxHighHandle, &xHighStats);

  vPrintTaskStats("LowPrio", &xLowStats);
  vPrintTaskStats("HighPrio", &xHighStats);

  uint32_t ulExpectedHighRuns = TEST_LENGTH / HIGH_PERIOD;

  /* If HighPrio was starved by LowPrio, it would run far fewer times */
  if (xHighStats.ulJobCounter < (ulExpectedHighRuns / 2)) {
    xPassed = pdFALSE;
  }
  /* Strongest signal preemption worked: jitter stayed within NFR */
  if (!xCheckJitter(&xHighStats)) {
    xPassed = pdFALSE;
  }

  vFinishTest("PREEMPTION CHECK", xPassed);
}

int main(void) {
  UART_init();
  UART_printf("\n[TITLE] PREEMPTION CHECK - HIGH PRIO OVER BUSY LOW PRIO\n");

  SchedulerConfig_t sch_cfg = {
      .maxTasks = 2, .xPolicy = PTL_OVERRUN_SKIP, .xTraceEnabled = pdTRUE};

  xPTL_Init(&sch_cfg);
  vPTL_PrintPTLCfg(sch_cfg);

  PTL_TaskConfig_t xLowCfg = {
    .pcTaskName = "LowPrio",
    .PTL_JobFunction_t = LowPrioTask,
    .usStack_size = 256,
    .xPriority = 1,
    .xPeriod = pdMS_TO_TICKS(LOW_PERIOD),
    .xDeadline = pdMS_TO_TICKS(LOW_PERIOD),
    .pArg = NULL
  };

  PTL_TaskConfig_t xHighCfg = {
    .pcTaskName = "HighPrio",
    .PTL_JobFunction_t = HighPrioTask,
    .usStack_size = 256,
    .xPriority = 3,
    .xPeriod = pdMS_TO_TICKS(HIGH_PERIOD),
    .xDeadline = pdMS_TO_TICKS(HIGH_PERIOD),
    .pArg = NULL
  };

  if (xPTL_CreateTask(&xLowCfg, &pxLowHandle) != PTL_OK ||
      xPTL_CreateTask(&xHighCfg, &pxHighHandle) != PTL_OK) {
    UART_printf("[TEST-FATAL] CreateTask failed\n");
    exit_qemu(1);
  }

  vPTL_PrintTaskParam(xLowCfg);
  vPTL_PrintTaskParam(xHighCfg);

  TimerHandle_t xTestTimer =
  xTimerCreate("Timeout",
              pdMS_TO_TICKS(TEST_LENGTH), 
              pdFALSE, 
              (void *)0,
              vPreemptionTimeoutCallback
  );

  xTimerStart(xTestTimer, 0);
  xPTL_Start();

  for (;;);
  return 0;
}
