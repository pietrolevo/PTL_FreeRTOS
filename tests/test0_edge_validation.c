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
 * test0_edge_validation.c
 *
 */
#include "ptl.h"
#include "test_utils.h"
#include "timers.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void DummyTask(void *pv) {
  (void)pv;
  PTL_CheckAbort();
}

int main(void) {
  UART_init();
  UART_printf("\n[TITLE] EDGE CASE - CONFIG VALIDATION\n");

  PTL_TaskConfig_t xCfg;
  PTL_Status_t xStatus;

  /* CreateTask before Init must fail */
  memset(&xCfg, 0, sizeof(xCfg));
  xCfg.pcTaskName = "Pre_Init";
  xCfg.PTL_JobFunction_t = DummyTask;
  xCfg.usStack_size = 256;
  xCfg.xPriority = 1;
  xCfg.xPeriod = pdMS_TO_TICKS(10);
  xCfg.xDeadline = pdMS_TO_TICKS(10);

  xStatus = xPTL_CreateTask(&xCfg, NULL);
  run_test("CreateTask before Init", xStatus, PTL_ERROR);

  /* Init with maxTasks == 0 */
  SchedulerConfig_t xBadCfg = {
      .maxTasks = 0, .xPolicy = PTL_OVERRUN_SKIP, .xTraceEnabled = pdTRUE};
  xStatus = xPTL_Init(&xBadCfg);
  run_test("Init with maxTasks=0", xStatus, PTL_INVALID_PARAM);

  /* Init with NULL config */
  xStatus = xPTL_Init(NULL);
  run_test("Init with NULL config", xStatus, PTL_INVALID_PARAM);

  /* Valid Init */
  SchedulerConfig_t xGoodCfg = {
      .maxTasks = 4, .xPolicy = PTL_OVERRUN_SKIP, .xTraceEnabled = pdTRUE};
  xStatus = xPTL_Init(&xGoodCfg);
  run_test("Init valid config", xStatus, PTL_OK);

  /* Double Init must fail */
  xStatus = xPTL_Init(&xGoodCfg);
  run_test("Double Init", xStatus, PTL_ERROR);

  /* CreateTask with NULL config */
  xStatus = xPTL_CreateTask(NULL, NULL);
  run_test("CreateTask NULL config", xStatus, PTL_INVALID_PARAM);

  /* CreateTask with NULL task name */
  memset(&xCfg, 0, sizeof(xCfg));
  xCfg.pcTaskName = NULL;
  xCfg.PTL_JobFunction_t = DummyTask;
  xCfg.usStack_size = 256;
  xCfg.xPriority = 1;
  xCfg.xPeriod = pdMS_TO_TICKS(10);
  xCfg.xDeadline = pdMS_TO_TICKS(10);
  xStatus = xPTL_CreateTask(&xCfg, NULL);
  run_test("CreateTask NULL name", xStatus, PTL_INVALID_PARAM);

  /* CreateTask with NULL job function*/
  xCfg.pcTaskName = "NoJob";
  xCfg.PTL_JobFunction_t = NULL;
  xStatus = xPTL_CreateTask(&xCfg, NULL);
  run_test("CreateTask NULL job function", xStatus, PTL_INVALID_PARAM);

  /* CreateTask with xPeriod == 0 */
  xCfg.PTL_JobFunction_t = DummyTask;
  xCfg.xPeriod = 0;
  xStatus = xPTL_CreateTask(&xCfg, NULL);
  run_test("CreateTask period=0", xStatus, PTL_INVALID_PARAM);

  /* CreateTask with xDeadline > xPeriod */
  xCfg.xPeriod = pdMS_TO_TICKS(10);
  xCfg.xDeadline = pdMS_TO_TICKS(20);
  xStatus = xPTL_CreateTask(&xCfg, NULL);
  run_test("CreateTask deadline>period", xStatus, PTL_INVALID_PARAM);

  /* CreateTask with xPriority out of range */
  xCfg.xDeadline = pdMS_TO_TICKS(10);
  xCfg.xPriority = configMAX_PRIORITIES; /* first invalid value */
  xStatus = xPTL_CreateTask(&xCfg, NULL);
  run_test("CreateTask priority out of range", xStatus, PTL_INVALID_PARAM);

  /* CreateTask valid, deadline implicit (xDeadline = 0 -> = period) */
  xCfg.xPriority = 1;
  xCfg.xDeadline = 0;
  xStatus = xPTL_CreateTask(&xCfg, NULL);
  run_test("CreateTask implicit deadline (0)", xStatus, PTL_OK);

  /* Fill the remaining slots (maxTasks = 4, one slot already used)*/
  for (int i = 0; i < 3; i++) {
    char buffer[32];
    sprintf(buffer, "Filler_%d", i);
    xCfg.pcTaskName = strdup(buffer);
    xCfg.xDeadline = pdMS_TO_TICKS(10);
    xStatus = xPTL_CreateTask(&xCfg, NULL);
    run_test("CreateTask fill slot", xStatus, PTL_OK);
  }

  /* Table should now be full (4/4): next creation must fail with NO_MEMORY */
  xCfg.pcTaskName = "Overflow";
  xStatus = xPTL_CreateTask(&xCfg, NULL);
  run_test("CreateTask table full", xStatus, PTL_NO_MEMORY);

  UART_printf("\n--- END ---\n");
  exit_qemu(0);
  return 0;
}
