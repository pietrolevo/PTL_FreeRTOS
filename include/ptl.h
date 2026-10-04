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


#ifndef PTL_H
#define PTL_H

#include "FreeRTOS.h"
#include "task.h"
#include "stdint.h"

#define PTL_BIT_RELEASE ( 1 << 0 )  /* Bit 0: a new job has been released, the task can start */
#define PTL_BIT_ABORT   ( 1 << 1 )  /* Bit 1: an overrun occurred, abort the job if policy is KILL */

/* Number of entries in the circular trace log buffer. */
#define LOG_BUFFER_SIZE 256


/**
 * @brief Return status codes for PTL API
 */
typedef enum {
    PTL_OK = 0,                 /* Operation successful */
    PTL_ERROR = 1,              /* Generic execution error */
    PTL_INVALID_PARAM = 2,      /* Null pointers or out-of-range values */
    PTL_NO_MEMORY = 3           /* Memory allocation failed or maxTasks reached */
} PTL_Status_t;


/**
 * @brief Strategies for handling tasks that miss their xDeadline
 */
typedef enum {
    PTL_OVERRUN_SKIP = 0,
    PTL_OVERRUN_KILL = 1,
    PTL_OVERRUN_CATCH_UP = 2
} PTL_Policy_t;


/**
 * @brief Configuration parameters for a periodic task.
 *
 * Defines all static attributes of a PTL-managed task, including
 * its name, job function, stack size, priority, timing parameters
 * (period, deadline, phase) and optional user argument.
 */
typedef struct {
    const char *pcTaskName;
    TaskFunction_t PTL_JobFunction_t;
    uint16_t usStack_size;
    UBaseType_t xPriority;
    TickType_t xPeriod;
    TickType_t xDeadline;
    TickType_t xPhase;
    void* pArg;
} PTL_TaskConfig_t;


/**
 * @brief Global scheduler configuration
 */
typedef struct {
    uint8_t maxTasks;       /* Maximum number of xPriority tasks allowed */
    PTL_Policy_t xPolicy;   /* Strategy for xDeadline misses */
    BaseType_t xTraceEnabled;
} SchedulerConfig_t;


/* Opaque pointer to hide internal Task Control Block from the user */
typedef struct PTL_TCB PTL_TaskHandle_t;


/**
 * @brief Initializes the PTL internal structures and allocates memory for the task table.
 * @param xCfg Pointer to scheduler configuration.
 * @return PTL_OK on success, PTL_INVALID_PARAM if xCfg is NULL or maxTasks is 0,
 *         PTL_NO_MEMORY if allocation fails, PTL_ERROR if already initialized.
 */
PTL_Status_t xPTL_Init( const SchedulerConfig_t *xCfg );


/**
 * @brief Starts periodic scheduling.
 * Defines t0 for all registered tasks (xNextRelease = now + phase),
 * resets their job counters and running flags, then starts the FreeRTOS scheduler.
 * 
 * @return PTL_ERROR if xPTL_Init was not called first; otherwise this
 *         function does not return under normal operation, since
 *         vTaskStartScheduler() only returns on insufficient RAM.
 */
PTL_Status_t xPTL_Start( void );


/**
 * @brief Registers a new periodic task and creates the underlying FreeRTOS task.
 * The task blocks immediately, waiting for its first release notification;
 * it does not run until xPTL_Start() defines t0 and releases begin.
 * @param xCfg Pointer to task requirements.
 * @param pxHandle Optional pointer to store the generated PTL handle.
 * @return PTL_OK on success.
 */
PTL_Status_t xPTL_CreateTask( const PTL_TaskConfig_t *xCfg, PTL_TaskHandle_t **pxHandle );


/**
 * @brief Enum of possible events to be logged
 */
typedef enum {
    PTL_EVENT_START,
    PTL_EVENT_DEADLINE_MISS,
    PTL_EVENT_OVERRUN,
    PTL_EVENT_RELEASE,
    PTL_EVENT_KILL,
    PTL_EVENT_SKIP,
    PTL_EVENT_CATCH_UP,
    PTL_EVENT_IDLE,
    PTL_EVENT_COMPLETE,
    PTL_EVENT_ABORTED,
} PTL_EventType_t;


/**
 * @brief Struct where log messages are packaged
 */
typedef struct {
    PTL_EventType_t xType;
    const char *pcTaskName;
    TickType_t xTimeStamp;
    uint32_t ulJobId;
} PTL_LogEvent_t;


/* --- UTILITIES FOR TEST PURPOSES */

/**
 * @brief struct where test purpouse statistics are packaged
 */
typedef struct {
    uint32_t ulJobCounter;
    uint32_t ulDeadlinesMisses;
    uint32_t ulOverruns;
    TickType_t xMaxReleaseJitter;
} PTL_TaskStats_t;


/**
 * @brief Retrieves runtime statistics for a periodic task.
 * @param pxHandle handle returned by xPTL_CreateTask.
 * @param pxStats pointer to struct to fill.
 * @return PTL_OK on success, PTL_INVALID_PARAM if either pointer is NULL.
 */
PTL_Status_t PTL_GetTaskStats( PTL_TaskHandle_t *pxHandle, PTL_TaskStats_t *pxStats );


/**
 * @brief Returns the total number of ticks the system has spent busy scheduling
 * @return Number of idle ticks accumulated so far.
 */
TickType_t PTL_GetOverheadTicks( void );



/**
 * @brief Returns the number of ticks elapsed since xPTL_Init() was called.
 * @return Total tick count since initialization.
 */
TickType_t PTL_GetTotalTicks( void );


/**
 * @brief Returns the maximum release jitter observed for a given task.
 * @param pxHandle handle returned by xPTL_CreateTask.
 * @return Maximum jitter in ticks, or 0 if pxHandle is NULL.
 */
TickType_t PTL_GetMaxJitter( PTL_TaskHandle_t *pxHandle );


/**
 * @brief Print information about the task
 * @param task Task to be printed
*/
void vPTL_PrintTaskParam( PTL_TaskConfig_t task );


/**
 * @brief Print information about the PTL configuration
 * @param sch_cfg PTL configuration to be printed
*/
void vPTL_PrintPTLCfg( SchedulerConfig_t sch_cfg );


/**
 * @brief ONLY FOR TESTING PURPOSE: deletes all registered tasks and releases
 * the internal task table, so a fresh xPTL_Init() can be called again.
 */
void PTL_TestCleanup( void );


/**
 * @brief Macro used by user to manage the task abortion
*/
#define PTL_CheckAbort() \
    do { \
        uint32_t _notifiedValue_ = 0; \
        xTaskNotifyWait( 0x00, PTL_BIT_ABORT, &_notifiedValue_, 0 ); \
        if ( _notifiedValue_ & PTL_BIT_ABORT ) { \
            return; \
        } \
    } while (0)


/**
 * @brief vTaskDelay implementation with awareness of abort signal
 * @param _time_ Amount of time to wait
*/
#define PTL_vTaskDelay( _time_ ) \
    do {\
        for( TickType_t _i_ = 0; _i_ < (_time_); _i_++ ) { \
            uint32_t _ptl_n_ = 0; \
            xTaskNotifyWait( 0x00, PTL_BIT_ABORT | PTL_BIT_RELEASE, &_ptl_n_, pdMS_TO_TICKS(1) ); \
            if( _ptl_n_ & PTL_BIT_ABORT ) { \
                if( _ptl_n_ & PTL_BIT_RELEASE ) { \
                    xTaskNotify( xTaskGetCurrentTaskHandle(), PTL_BIT_RELEASE, eSetBits ); \
                } \
                return; \
            } \
        } \
    } while (0)
#endif
