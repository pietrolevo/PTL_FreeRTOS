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
 * test8_phase_offset.c
 */
#include "ptl.h"
#include "test_utils.h"
#include "timers.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

#define PH_N_TASKS 3
#define PH_PERIOD 15 // ms

static const TickType_t xPhasesMs[PH_N_TASKS] = {0, 7, 15};
static int xTaskIndex[PH_N_TASKS] = {0, 1, 2};

static PTL_TaskConfig_t t_ph[PH_N_TASKS];
static PTL_TaskHandle_t *pxHandles[PH_N_TASKS];
#define PH_JITTER_TOLERANCE 2
static volatile TickType_t xFirstStart[PH_N_TASKS];
static volatile BaseType_t xFirstStartRecorded[PH_N_TASKS];
static volatile uint32_t xInvocationCount[PH_N_TASKS];

static void PhaseTask(void *pv) {
  int idx = *(int *)pv;

  taskENTER_CRITICAL();
  xInvocationCount[idx]++;

  if ( xInvocationCount[idx] == 2 && xFirstStartRecorded[idx] == pdFALSE ) {
    xFirstStart[idx] = PTL_GetTotalTicks();
    xFirstStartRecorded[idx] = pdTRUE;
  }
  taskEXIT_CRITICAL();

  PTL_vTaskDelay(10);
  PTL_CheckAbort();
}

static void vPhaseOffsetTimeoutCallback(TimerHandle_t xTimer) {
  (void)xTimer;
  BaseType_t xPassed = pdTRUE;

  for (int i = 0; i < PH_N_TASKS; i++) {
    PTL_TaskStats_t xStats;
    PTL_GetTaskStats(pxHandles[i], &xStats);
    vPrintTaskStats(t_ph[i].pcTaskName, &xStats);

    if (!xCheckJitter(&xStats)) {
      xPassed = pdFALSE;
    }
    if (xFirstStartRecorded[i] == pdFALSE) {
      xPassed = pdFALSE;
    }
  }

  for (int i = 1; i < PH_N_TASKS; i++) {
    TickType_t xExpectedDelta = pdMS_TO_TICKS(xPhasesMs[i]) - pdMS_TO_TICKS(xPhasesMs[0]);
    TickType_t xActualDelta = xFirstStart[i] - xFirstStart[0];
    TickType_t xDiff = (xActualDelta > xExpectedDelta)
      ? (xActualDelta - xExpectedDelta)
      : (xExpectedDelta - xActualDelta);

    char buf[96];
    snprintf(buf, sizeof(buf),
             "[STATS] phase_check[%d] expectedDelta=%lu actualDelta=%lu\n", i,
             (unsigned long)xExpectedDelta, (unsigned long)xActualDelta);
    UART_printf(buf);

    if (xDiff > PH_JITTER_TOLERANCE) {
      xPassed = pdFALSE;
    }
  }

  vFinishTest("PHASE OFFSET", xPassed);
}

int main(void) {
  UART_init();

  UART_printf("\n[TITLE] PHASE OFFSET (STAGGERED FIRST RELEASE)\n");

  SchedulerConfig_t sch_cfg = {
    .maxTasks = PH_N_TASKS, 
    .xPolicy = PTL_OVERRUN_SKIP, 
    .xTraceEnabled = pdTRUE
  };

  xPTL_Init(&sch_cfg);
  vPTL_PrintPTLCfg(sch_cfg);

  char buffer[128];

  for (int i = 0; i < PH_N_TASKS; i++) {
    sprintf(buffer, "PhaseTask_%d", i);
    t_ph[i].pcTaskName = strdup(buffer);
    t_ph[i].PTL_JobFunction_t = PhaseTask;
    t_ph[i].usStack_size = 256;
    t_ph[i].xPriority = 2;
    t_ph[i].xPeriod = pdMS_TO_TICKS(PH_PERIOD);
    t_ph[i].xDeadline = pdMS_TO_TICKS(PH_PERIOD);
    t_ph[i].xPhase = pdMS_TO_TICKS(xPhasesMs[i]);
    t_ph[i].pArg = &xTaskIndex[i];

    if (xPTL_CreateTask(&(t_ph[i]), &pxHandles[i]) != PTL_OK) {
      UART_printf("[TEST-FATAL] CreateTask failed\n");
      exit_qemu(1);
    }
    vPTL_PrintTaskParam(t_ph[i]);
  }

  TimerHandle_t xTestTimer = xTimerCreate(
    "Timeout", 
    pdMS_TO_TICKS(TEST_LENGTH), 
    pdFALSE, 
    (void *)0, 
    vPhaseOffsetTimeoutCallback
  );


  xTimerStart(xTestTimer, 0);

  xPTL_Start(); // Start PTL

  for (;;);
  return 0;
}
