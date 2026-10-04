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
 * test6_overhead_and_gaps.c
 */

#include "ptl.h"
#include "test_utils.h"
#include "timers.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

#define TIGHT_PERIOD 1 // ms - minimal gap between releases
#define TIGHT_TASKS 8

static PTL_TaskConfig_t t_cfgs[TIGHT_TASKS];
static PTL_TaskHandle_t *pxHandles[TIGHT_TASKS];

static void TinyTask(void *pv) {
  (void)pv;
  PTL_CheckAbort();
}

static void vOverheadTimeoutCallback(TimerHandle_t xTimer) {
  (void)xTimer;
  BaseType_t xPassed = pdTRUE;

  float xOverheadPct = xComputeOverheadPercent();
  char buf[64];
  snprintf(buf, sizeof(buf), "[STATS] cpu_overhead=%f%%\n",
           xOverheadPct);
  UART_printf(buf);

  if (xOverheadPct > 10) {
    xPassed = pdFALSE;
  }

  for (int i = 0; i < TIGHT_TASKS; i++) {
    PTL_TaskStats_t xStats;
    PTL_GetTaskStats(pxHandles[i], &xStats);
    vPrintTaskStats(t_cfgs[i].pcTaskName, &xStats);

    if (!xCheckJitter(&xStats)) {
      xPassed = pdFALSE;
    }
  }

  vFinishTest("OVERHEAD + MINIMAL GAPS", xPassed);
}

int main(void) {
  UART_init();
  UART_printf("\n[TITLE] OVERHEAD + MINIMAL TIME GAPS\n");

  SchedulerConfig_t sch_cfg = {.maxTasks = TIGHT_TASKS,
                               .xPolicy = PTL_OVERRUN_SKIP,
                               .xTraceEnabled = pdTRUE};

  xPTL_Init(&sch_cfg);
  vPTL_PrintPTLCfg(sch_cfg);

  char buffer[128];

  for (int i = 0; i < TIGHT_TASKS; i++) {
    sprintf(buffer, "TinyTask_%d", i);
    t_cfgs[i].pcTaskName = strdup(buffer);
    t_cfgs[i].PTL_JobFunction_t = TinyTask;
    t_cfgs[i].usStack_size = 256;
    t_cfgs[i].xPriority = 2;
    t_cfgs[i].xPeriod = pdMS_TO_TICKS(TIGHT_PERIOD);
    t_cfgs[i].xDeadline = pdMS_TO_TICKS(TIGHT_PERIOD);
    t_cfgs[i].pArg = NULL;

    if (xPTL_CreateTask(&(t_cfgs[i]), &pxHandles[i]) != PTL_OK) {
      UART_printf("[TEST-FATAL] CreateTask failed\n");
      exit_qemu(1);
    }
    vPTL_PrintTaskParam(t_cfgs[i]);
  }

  TimerHandle_t xTestTimer =
      xTimerCreate("Timeout", pdMS_TO_TICKS(TEST_LENGTH), pdFALSE, (void *)0,
                   vOverheadTimeoutCallback);

  xTimerStart(xTestTimer, 0);
  xPTL_Start();

  for (;;)
    ;
  return 0;
}
