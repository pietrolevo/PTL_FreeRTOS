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
 * test3_skip.c
 *
 */
#include "ptl.h"
#include "test_utils.h"
#include "timers.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Example of a user task
static void VariableLongTask(void *pv) {
  unsigned int *seed = (unsigned int *)pv;
  // Some heavy code here
  PTL_vTaskDelay(rand_r(seed) % N);
  // Check after every heavy code if the abort signal is raised
  PTL_CheckAbort();
}

static PTL_TaskConfig_t t_longs[N_TASK];
static PTL_TaskHandle_t *pxHandles[N_TASK];

static void vSkipTimeoutCallback(TimerHandle_t xTimer) {
  (void)xTimer;
  BaseType_t xPassed = pdTRUE;
  uint32_t ulIdealJobs = TEST_LENGTH / PERIOD;

  for (int i = 0; i < N_TASK; i++) {
    PTL_TaskStats_t xStats;
    PTL_GetTaskStats(pxHandles[i], &xStats);
    vPrintTaskStats(t_longs[i].pcTaskName, &xStats);

    if (!xCheckJitter(&xStats)) {
      xPassed = pdFALSE;
    }
    /* SKIP suppresses releases, actual jobs can't exceed the ideal count */
    if (xStats.ulJobCounter > ulIdealJobs) {
      xPassed = pdFALSE;
    }
  }

  vFinishTest("SKIP POLICY", xPassed);
}

int main(void) {

#if RANDOM
  srand(time(NULL));
#else
  srand(0xDEADBEEF);
#endif

  UART_init();

  UART_printf("\n[TITLE] SKIP POLICY\n");

  // Policy SKIP
  SchedulerConfig_t sch_cfg = {
      .maxTasks = N_TASK, .xPolicy = PTL_OVERRUN_SKIP, .xTraceEnabled = pdTRUE};

  xPTL_Init(&sch_cfg);

  vPTL_PrintPTLCfg(sch_cfg);

  char buffer[128];
  static unsigned int seeds[N_TASK];

  for (int i = 0; i < N_TASK; i++) {
    seeds[i] = rand();
    sprintf(buffer, "LongTaskVariable_%d", i);
    t_longs[i].pcTaskName = strdup(buffer);
    t_longs[i].PTL_JobFunction_t = VariableLongTask;
    t_longs[i].usStack_size = 256;
    t_longs[i].xPriority = 2;
    t_longs[i].xPeriod = pdMS_TO_TICKS(PERIOD);
    t_longs[i].xDeadline = pdMS_TO_TICKS(DEADLINE);
    t_longs[i].pArg = &seeds[i];

    if (xPTL_CreateTask(&(t_longs[i]), &pxHandles[i]) != PTL_OK) {
      UART_printf("[TEST-FATAL] CreateTask failed\n");
      exit_qemu(1);
    }
    vPTL_PrintTaskParam(t_longs[i]);
  }

  // Timeout of x seconds before checking trace
  TimerHandle_t xTestTimer =
      xTimerCreate("Timeout", pdMS_TO_TICKS(TEST_LENGTH), pdFALSE, (void *)0,
                   vSkipTimeoutCallback);

  xTimerStart(xTestTimer, 0);

  xPTL_Start(); // Start PTL

  for (;;)
    ;
  return 0;
}
