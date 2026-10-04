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

/**
 * test_utils.h
*/
#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "ptl.h"
#include "timers.h"

// -- TEST PRE DIRECTIVES

// #define LONG_TEST
// #define MID_TEST
#define SHORT_TEST

#ifdef LONG_TEST
#define TEST_LENGTH 5000 // 5 s
#elif MID_TEST
#define TEST_LENGTH 1000 // 1 s
#else
#define TEST_LENGTH 500 // 0.5 s
#endif

#define RANDOM 0 // RANDOM SEED ? SET TO 0 IF YOU WANT A CLEAR COMPARISON BETWEEN POLICIES
#define PERIOD 20 // ms
#define DEADLINE 20 // ms
#define N 100 // HOW MUCH IT TAKES A TASK TO COMPLETE IN THE WORST CASE
#define N_TASK 4 // NUMBER OF TASKS => DO NOT GO OVER 8

void run_test(const char* test_name, PTL_Status_t result, PTL_Status_t expected);
void vTestTimeoutCallback(TimerHandle_t xTimer);
void exit_qemu(int code);

void vPrintTaskStats( const char* pcName, const PTL_TaskStats_t *pxStats );
BaseType_t xCheckJitter( const PTL_TaskStats_t *pxStats );
float xComputeOverheadPercent( void );
void vFinishTest( const char* pcTestName, BaseType_t xPassed );

#endif // TEST_UTILS_H
