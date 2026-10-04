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


/*
 * test_utils.c
 *
 */

#include "include/test_utils.h"
#include <stdio.h>
#include "uart.h"
#include "timers.h"
#include "ptl.h"


/**
 * @brief Helper function to execute and report test results
 * @param test_name Description of the test case
 * @param result The status returned by the PTL function under test
 * @param expected The status we expect to receive if the library behaves correctly
 */
void run_test(const char* test_name, PTL_Status_t result, PTL_Status_t expected) {
    char buf[128];
    if (result == expected) {
        sprintf(buf, "[PASS] %s\n", test_name);
    } else {
        /* In case of failure, report both the expected and actual error codes */
        sprintf(buf, "[FAIL] %s (Expected %d, got %d)\n", test_name, expected, result);
    }
    UART_printf(buf);
}


void exit_qemu(int code) {
    static volatile int block[2];
    block[0] = 0x20026;
    block[1] = code;
    asm volatile (
        "mov r0, #0x20\n"
        "mov r1, %0\n"
        "bkpt 0xAB\n"
        :
        : "r" (block)
        : "r0", "r1", "memory"
    );
}


void vPrintTaskStats( const char* pcName, const PTL_TaskStats_t *pxStats )
{
    char buf[160];
    snprintf(buf, sizeof(buf),
        "[STATS] task=%s jobs=%lu misses=%lu overruns=%lu maxJitter=%lu\n",
        pcName,
        (unsigned long) pxStats->ulJobCounter,
        (unsigned long) pxStats->ulDeadlinesMisses,
        (unsigned long) pxStats->ulOverruns,
        (unsigned long) pxStats->xMaxReleaseJitter);
    UART_printf(buf);
}


BaseType_t xCheckJitter( const PTL_TaskStats_t *pxStats )
{
    return ( pxStats->xMaxReleaseJitter <= 1 ) ? pdTRUE : pdFALSE;
}

float xComputeOverheadPercent( void )
{
    uint32_t xTotal    = PTL_GetTotalTicks();
    uint32_t xOverhead = PTL_GetOverheadTicks();

    if ( xTotal == 0 )
    {
        return 0.0f;
    }

    return ( (float) xOverhead * 100.0f ) / (float) xTotal;
}

void vFinishTest( const char* pcTestName, BaseType_t xPassed )
{
    TickType_t xTotal = PTL_GetTotalTicks();
    TickType_t xOverheadTicks = PTL_GetOverheadTicks();
    char buf[128];

    snprintf(buf, sizeof(buf), "[STATS] busy_ticks=%lu total_ticks=%lu\n",
        (unsigned long) xOverheadTicks, (unsigned long) xTotal);
    UART_printf(buf);

    snprintf(buf, sizeof(buf), "\n[RESULT] %s: %s\n",
        pcTestName, xPassed ? "PASSED" : "FAILED");
    UART_printf(buf);

    // Shut all the tasks
    PTL_TestCleanup();
    PTL_vTaskDelay( pdMS_TO_TICKS( 10 ) );
    UART_printf("\n--- END ---\n");

    exit_qemu( xPassed ? 0 : 1 );
}
