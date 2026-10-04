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


#include "ptl.h"
#include "portmacro.h"
#include "string.h"
#include "task.h"
#include "queue.h"
#include "uart.h"


/**
 * @brief Task Control Block for a periodic task managed by the PTL.
 * This Structure stores runtime data needed by the scheduler to manage the tasks.
 *
 * Fields in order:
 * - FreeRTOS task handle
 * - configuration of the task
 * - Tick at which the current job was released
 * - Tick at which the next job release is scheduled
 * - Absolute deadline for current job
 * - Maximum observed release jitter (ticks)
 * - Number of jobs released for this task
 * - Number of jobs still pending execution (queued releases)
 * - Number of deadline misses detected for this task
 * - Number of period overruns occurred
 * - Flag which indicates if a job is currently running (also used, together
 *   with ulJobCounter, to detect and discard stale/aborted jobs on completion)
 */
struct PTL_TCB
{
    TaskHandle_t xHandle;
    PTL_TaskConfig_t xCfg; 
    TickType_t xReleaseTime;
    TickType_t xNextRelease;
    TickType_t xAbsoluteDeadline;
    TickType_t xMaxReleaseJitter;
    uint32_t ulJobCounter;
    uint32_t ulPendingJobs;
    uint32_t ulDeadlinesMisses;
    uint32_t ulOverruns;
    volatile BaseType_t xJobRunning;
};

static SchedulerConfig_t xSchedulerCfg;
static struct PTL_TCB *pxTaskTable = NULL;
static uint8_t ucTaskCount = 0;
static BaseType_t xInitialized = pdFALSE; /* Flag to prevent double initialization and memory leaks */
static volatile BaseType_t xIsSystemIdle = pdFALSE;
static TickType_t xIdleTicks = 0;
static TickType_t xInitTickCount = 0; 

static volatile TickType_t xOverheadTicks = 0;

static PTL_LogEvent_t xLogBuffer[ LOG_BUFFER_SIZE ];
static volatile uint32_t ulLogHead = 0;
static volatile uint32_t ulLogTail = 0;
static volatile uint32_t ulLogDropped = 0;


/**
 * @brief Validates a PTL task configuration.
 * Checks that the configuration pointer, job function and task name are not
 * NULL, that the period is not zero, that the deadline does not exceed the
 * period, and that the requested priority is within configMAX_PRIORITIES.
 *
 * @param xCfg Pointer to task configuration.
 * @return pdTRUE if valid, pdFALSE otherwise.
 */
static BaseType_t prvPTL_ValidateTaskConfiguration( const PTL_TaskConfig_t *xCfg )
{
    if ( xCfg == NULL || xCfg->PTL_JobFunction_t == NULL || xCfg->pcTaskName == NULL )
    {
        return pdFALSE;
    }
    if ( xCfg->xPeriod == 0 )
    { 
        return pdFALSE;
    }
    
    /* Deadline must be <= xPeriod */
    if ( xCfg->xDeadline > xCfg->xPeriod )
    { 
        return pdFALSE;
    }

    if ( xCfg->xPriority >= configMAX_PRIORITIES)
    {
        return pdFALSE;
    }

    return pdTRUE;
}


/**
 * @brief Logs a PTL event from task context.
 * Stores event type, timestamp, task name and job id into a circular buffer.
 * 
 * @param xType Event type to log.
 * @param pxTCB task that generated the event(for IDLE it is NULL).
 * @param ulJobId job identifier associated with the event.
 */
static void prvPTL_Log( PTL_EventType_t xType, struct PTL_TCB *pxTCB, uint32_t ulJobId )
{
    if ( xSchedulerCfg.xTraceEnabled == pdFALSE )
    {
        return;
    }

    taskENTER_CRITICAL();

    /* check if buffer is full */
    if ( ( ulLogHead - ulLogTail ) >= LOG_BUFFER_SIZE )
    {
        ulLogDropped++;
        taskEXIT_CRITICAL();
        return;
    }

    uint32_t ulBufPos = ulLogHead % LOG_BUFFER_SIZE;   
    ulLogHead++;
        
    xLogBuffer[ ulBufPos ].xType = xType;
    xLogBuffer[ ulBufPos ].xTimeStamp = PTL_GetTotalTicks();

    if ( pxTCB != NULL )
    {
        xLogBuffer[ ulBufPos ].pcTaskName = pxTCB->xCfg.pcTaskName;
        xLogBuffer[ ulBufPos ].ulJobId = ulJobId;
    } 
    else 
    {
        xLogBuffer[ ulBufPos ].pcTaskName = "IDLE";
        xLogBuffer[ ulBufPos ].ulJobId = 0;
    }
    taskEXIT_CRITICAL();
}


/**
 * @brief Logs a PTL event from ISR context.
 * ISR-safe version of prvPTL_Log() with critical sections.
 * 
 * @param xType event type to log.
 * @param pxTCB task that generated the event(for IDLE it is NULL).
 * @param ulJobId job identifier associated with the event.
 */
static void prvPTL_LogFromISR( PTL_EventType_t xType, struct PTL_TCB *pxTCB, uint32_t ulJobId )
{
    if ( xSchedulerCfg.xTraceEnabled == pdFALSE )
    {
        return;
    }

    UBaseType_t xSavedITStatus = taskENTER_CRITICAL_FROM_ISR();

    /* check if buffer is full */
    if ( ( ulLogHead - ulLogTail ) >= LOG_BUFFER_SIZE )
    {
        ulLogDropped++;
        taskEXIT_CRITICAL_FROM_ISR( xSavedITStatus );
        return;
    }

    uint32_t ulBufPos = ulLogHead % LOG_BUFFER_SIZE;   
    ulLogHead++;
        
    xLogBuffer[ ulBufPos ].xType = xType;
    xLogBuffer[ ulBufPos ].xTimeStamp = PTL_GetTotalTicks();

    if ( pxTCB != NULL )
    {
        xLogBuffer[ ulBufPos ].pcTaskName = pxTCB->xCfg.pcTaskName;
        xLogBuffer[ ulBufPos ].ulJobId = ulJobId;
    } 
    else 
    {
        xLogBuffer[ ulBufPos ].pcTaskName = "IDLE";
        xLogBuffer[ ulBufPos ].ulJobId = 0;
    }
    taskEXIT_CRITICAL_FROM_ISR( xSavedITStatus );
}


/**
 * @brief Outputs all pending log events over UART.
 * flushes the circular log buffer and prints each event.
 */
static void prvPTL_FlushLogBuffer( void )
{
    while ( ulLogTail != ulLogHead )
    {
        uint32_t ulPos = ulLogTail % LOG_BUFFER_SIZE;
        PTL_LogEvent_t *pxEv = &xLogBuffer[ ulPos ];

        UART_printf( "[" );
        UART_printfvalue( pxEv->xTimeStamp );
        UART_printf( "] " );

        switch( pxEv->xType )
        {
            case PTL_EVENT_START:         UART_printf( "START " ); break;
            case PTL_EVENT_COMPLETE:      UART_printf( "COMPLETE " ); break;
            case PTL_EVENT_RELEASE:       UART_printf( "RELEASE " ); break;
            case PTL_EVENT_DEADLINE_MISS: UART_printf( "DEADLINE_MISS " ); break;
            case PTL_EVENT_OVERRUN:       UART_printf( "OVERRUN " ); break;
            case PTL_EVENT_SKIP:          UART_printf( "SKIP " ); break;
            case PTL_EVENT_KILL:          UART_printf( "KILL " ); break;
            case PTL_EVENT_CATCH_UP:      UART_printf( "CATCH_UP " ); break;
            case PTL_EVENT_IDLE:          UART_printf( "IDLE " ); break;
            case PTL_EVENT_ABORTED:       UART_printf( "ABORTED " ); break;
            default:                      UART_printf( "UNKNOWN " ); break;
        }

        UART_printf( pxEv->pcTaskName );
        UART_printf( " " );
        UART_printfvalue( pxEv->ulJobId );
        UART_printf( "\n" );
        ulLogTail++;
    }

    taskENTER_CRITICAL();
    uint32_t ulDroppedSnapshot = ulLogDropped;
    ulLogDropped = 0;
    taskEXIT_CRITICAL();

    if ( ulDroppedSnapshot > 0 )
    {
        UART_printf( "[TRACE] " );
        UART_printfvalue( ulDroppedSnapshot );
        UART_printf( " log events dropped" );
    }
}


/**
 * @brief Handles completion of a job.
 * Logs completion and, if the completing job is still the current one
 * checks whether its deadline was missed. If ulJobId does not match the TCB's current
 * job counter, the job is "stale": it belongs to a job that was already
 * aborted/superseded, so its completion is only logged (as ABORTED under
 * the KILL policy) and no deadline check or job-running flag update is
 * performed, since a newer job already owns that state.
 * 
 * @param pxTCB task whose job has completed.
 * @param ulJobId job identifier.
 */
static void prvPTL_JobComplete( struct PTL_TCB *pxTCB, uint32_t ulJobId )
{
    TickType_t xNow = xTaskGetTickCount() - xInitTickCount;
    BaseType_t xIsStale;

    taskENTER_CRITICAL();
    xIsStale = ( pxTCB->ulJobCounter != ulJobId );
    if ( !xIsStale )
    {
        pxTCB->xJobRunning = pdFALSE;
    }
    taskEXIT_CRITICAL();

    if ( xIsStale )
    {
        if ( xSchedulerCfg.xPolicy == PTL_OVERRUN_KILL )
        {
            prvPTL_Log( PTL_EVENT_ABORTED, pxTCB, ulJobId );
        }
        else
        {
            prvPTL_Log( PTL_EVENT_COMPLETE, pxTCB, ulJobId );
        }
        return;
    }

    prvPTL_Log( PTL_EVENT_COMPLETE, pxTCB, ulJobId );

    if ( xNow > pxTCB->xAbsoluteDeadline )
    {
        taskENTER_CRITICAL();
        pxTCB->ulDeadlinesMisses++;
        taskEXIT_CRITICAL();
        prvPTL_Log( PTL_EVENT_DEADLINE_MISS, pxTCB, ulJobId );
    }
}


/**
 * @brief Internal wrapper executed by each PTL task.
 * Implements the infinite loop around the user's job function: if jobs
 * are already queued (ulPendingJobs > 0, released while a previous
 * job was still running) it picks up the next one immediately, otherwise
 * it blocks on a notification wait until a release arrives.
 * After each execution of the user job function it reports completion and
 * decrements the pending job counter.
 * 
 * @param pvParameters pointer to the task's PTL_TCB.
 */
static void prvPTL_TaskWrap( void *pvParameters )
{
    struct PTL_TCB *pxTCB = ( struct PTL_TCB * )pvParameters;
    uint32_t ulNotifiedValue;
    /* counter of local jobs */
    uint32_t ulLocalJobId = 0;

    for( ;; ){
        /* check if old jobs present*/
        taskENTER_CRITICAL();
        BaseType_t xHasWork = ( pxTCB->ulPendingJobs > 0 );
        if ( xHasWork )
        {
            ulLocalJobId = pxTCB->ulJobCounter - ( pxTCB->ulPendingJobs - 1 );
        }
        taskEXIT_CRITICAL();

        if ( !xHasWork ) 
        {
            xTaskNotifyWait( 0, PTL_BIT_RELEASE | PTL_BIT_ABORT, &ulNotifiedValue, portMAX_DELAY );
            if ( ulNotifiedValue & PTL_BIT_ABORT ) {
                continue;
            }
            if ( ulNotifiedValue & PTL_BIT_RELEASE ) {
                taskENTER_CRITICAL();
                ulLocalJobId = pxTCB->ulJobCounter - ( pxTCB->ulPendingJobs - 1 );
                xHasWork = pdTRUE;
                taskEXIT_CRITICAL();
            }
        }

        if ( xHasWork )
        {

            TickType_t xStart = xTaskGetTickCount();
            prvPTL_Log( PTL_EVENT_START, pxTCB, ulLocalJobId );

            xOverheadTicks += ( xTaskGetTickCount() - xStart );

            pxTCB->xCfg.PTL_JobFunction_t( pxTCB->xCfg.pArg );
            
            xStart = xTaskGetTickCount();
            prvPTL_JobComplete( pxTCB, ulLocalJobId );

            taskENTER_CRITICAL();
            if ( pxTCB->ulPendingJobs > 0 ) {
                pxTCB->ulPendingJobs--;
            }
            taskEXIT_CRITICAL();
            xOverheadTicks += ( xTaskGetTickCount() - xStart );
        }
    }
}


/**
 * @brief Releases a new job for a periodic task.
 * Updates release time, deadline, job counter and notifies
 * the task to begin execution.
 * 
 * @param pxTCB task to release.
 * @param pxHigherWoken pdTRUE if an higher priority task is unblocked.
 */
static void prvPTL_ReleaseJob( struct PTL_TCB *pxTCB, BaseType_t *pxHigherWoken )
{
    TickType_t xNow = xTaskGetTickCountFromISR() - xInitTickCount;
    if ( xNow > pxTCB->xNextRelease )
    {
        TickType_t xReleaseJitter = xNow - pxTCB->xNextRelease;
        if ( xReleaseJitter > pxTCB->xMaxReleaseJitter )
        {
            pxTCB->xMaxReleaseJitter = xReleaseJitter;
        }
    }

    UBaseType_t xSavedITStatus = taskENTER_CRITICAL_FROM_ISR();
    pxTCB->ulJobCounter++;
    pxTCB->ulPendingJobs++;
    pxTCB->xReleaseTime = pxTCB->xNextRelease;
    pxTCB->xAbsoluteDeadline = pxTCB->xReleaseTime + pxTCB->xCfg.xDeadline;
    pxTCB->xNextRelease += pxTCB->xCfg.xPeriod;
    pxTCB->xJobRunning = pdTRUE;
    taskEXIT_CRITICAL_FROM_ISR( xSavedITStatus );

    prvPTL_LogFromISR( PTL_EVENT_RELEASE, pxTCB, pxTCB->ulJobCounter );
    xTaskNotifyFromISR( pxTCB->xHandle, PTL_BIT_RELEASE, eSetBits, pxHigherWoken );
}


/**
 * @brief Applies the configured overrun policy.
 * Handles SKIP, KILL and CATCH_UP when a task is still running
 * at its next release time.
 * 
 * @param pxTCB task under overrun.
 * @param pxHigherWoken pdTRUE if an higher priority task is unblocked.
 */
static void prvPTL_ApplyOverrunPolicy( struct PTL_TCB *pxTCB, BaseType_t *pxHigherWoken )
{
    pxTCB->ulOverruns++;
    prvPTL_LogFromISR( PTL_EVENT_OVERRUN, pxTCB, pxTCB->ulJobCounter );

    switch( xSchedulerCfg.xPolicy )
    {
        case PTL_OVERRUN_SKIP:
            /* Skip the new job; let the late one finish. */
            prvPTL_LogFromISR( PTL_EVENT_SKIP, pxTCB, pxTCB->ulJobCounter );
            UBaseType_t xSavedITStatus1 = taskENTER_CRITICAL_FROM_ISR();
            pxTCB->xNextRelease += pxTCB->xCfg.xPeriod;
            taskEXIT_CRITICAL_FROM_ISR( xSavedITStatus1 );
        break;
        case PTL_OVERRUN_KILL:
            /* Terminate/Suspend the running job immediately and release the new one. */
            prvPTL_LogFromISR( PTL_EVENT_KILL, pxTCB, pxTCB->ulJobCounter );
            xTaskNotifyFromISR( pxTCB->xHandle, PTL_BIT_ABORT, eSetBits, pxHigherWoken );
            prvPTL_ReleaseJob( pxTCB, pxHigherWoken );
        break;           
        case PTL_OVERRUN_CATCH_UP:
            /* release now, mark previous job missed, keep nominal cadence. */
            prvPTL_LogFromISR( PTL_EVENT_CATCH_UP, pxTCB, pxTCB->ulJobCounter );
            UBaseType_t xSavedITStatus2 = taskENTER_CRITICAL_FROM_ISR();
            pxTCB->ulDeadlinesMisses++;
            taskEXIT_CRITICAL_FROM_ISR( xSavedITStatus2 );
            prvPTL_LogFromISR( PTL_EVENT_DEADLINE_MISS, pxTCB, pxTCB->ulJobCounter );
            prvPTL_ReleaseJob( pxTCB, pxHigherWoken );
        break;
    }
}


/**
 * @brief PTL scheduler executed at every system tick.
 * Checks for job releases, detects overruns, applies policies
 * and triggers task notifications.
 */
static void prvPTL_SchedulerTick( void )
{
    TickType_t xStart = xTaskGetTickCountFromISR();
    TickType_t xNow = xTaskGetTickCountFromISR() - xInitTickCount;
    BaseType_t pxHigherWoken = pdFALSE;

    if ( xInitialized == pdFALSE ) {
        xOverheadTicks += ( xTaskGetTickCountFromISR() - xStart );
        return;
    }

    for ( uint8_t ux = 0; ux < ucTaskCount; ux++ )
    {
        struct PTL_TCB *pxTCB = &pxTaskTable[ ux ];

        if ( xNow >= pxTCB->xNextRelease )
        {
            xIsSystemIdle = pdFALSE;
            if ( pxTCB->xJobRunning == pdTRUE )
            {
                prvPTL_ApplyOverrunPolicy( pxTCB, &pxHigherWoken );
            }
            else
            {
                prvPTL_ReleaseJob( pxTCB, &pxHigherWoken );
            }
        }
    }

    xOverheadTicks += ( xTaskGetTickCountFromISR() - xStart );
    portYIELD_FROM_ISR( pxHigherWoken );
}


PTL_Status_t xPTL_Init( const SchedulerConfig_t *xCfg )
{
    
    if ( xCfg == NULL || xCfg->maxTasks == 0 )
    {
        return PTL_INVALID_PARAM;
    }

    /* Prevent double initialization */
    if ( xInitialized == pdTRUE )
    {
        return PTL_ERROR;
    }

    /* Dynamic allocation using FreeRTOS Heap */
    pxTaskTable = pvPortMalloc( sizeof( struct PTL_TCB ) * xCfg->maxTasks );

    if ( pxTaskTable == NULL )
    {
        return PTL_NO_MEMORY;
    }
    
    /* Function to clear memory to avoid garbage values */
    memset( pxTaskTable, 0, sizeof(struct PTL_TCB) * xCfg->maxTasks );

    xSchedulerCfg = *xCfg;
    ucTaskCount = 0;
    xInitialized = pdTRUE;

    memset( xLogBuffer, 0, sizeof( xLogBuffer ) );
    ulLogHead = 0;
    ulLogTail = 0;

    xInitTickCount = xTaskGetTickCount();

    return PTL_OK;
}


PTL_Status_t xPTL_Start( void )
{    
    xInitTickCount = xTaskGetTickCount();

    if ( xInitialized == pdFALSE )
    {
        return PTL_ERROR;
    }

    for( int i = 0; i < ucTaskCount; i++ )
    {
        struct PTL_TCB *pxTCB = &pxTaskTable[ i ];
        pxTCB->xNextRelease = pxTCB->xCfg.xPhase;
        pxTCB->ulJobCounter = 0;
        pxTCB->xJobRunning = pdFALSE;
    }

    vTaskStartScheduler();
    
    return PTL_OK; /* Code should never reach this point */
}


PTL_Status_t xPTL_CreateTask( const PTL_TaskConfig_t *xCfg, PTL_TaskHandle_t **pxHandle )
{
    BaseType_t xTaskStatus;
    struct PTL_TCB *pxTCB;

    /* Ensure PTL_Init was called first */
    if ( xInitialized == pdFALSE )
    {
        return PTL_ERROR;
    }

    /* Validate input parameters */
    if ( prvPTL_ValidateTaskConfiguration(xCfg) == pdFALSE )
    {
        return PTL_INVALID_PARAM;
    }

    /* Check if the pre-allocated table has space */
    if ( ucTaskCount >= xSchedulerCfg.maxTasks ) {
        return PTL_NO_MEMORY;
    }

    /* Pick the next available slot in the table */
    pxTCB = &pxTaskTable[ ucTaskCount ];

    /* Initialize internal TCB */
    pxTCB->xCfg = *xCfg;
    pxTCB->ulJobCounter = 0;
    pxTCB->ulDeadlinesMisses = 0;
    pxTCB->ulOverruns = 0;

    /* if deadline was not specified then Deadline = Period */
    if ( pxTCB->xCfg.xDeadline == 0 )
    {
        pxTCB->xCfg.xDeadline = pxTCB->xCfg.xPeriod;
    }

    /* Create the FreeRTOS task */
    xTaskStatus = xTaskCreate(
        prvPTL_TaskWrap,
        pxTCB->xCfg.pcTaskName,
        xCfg->usStack_size,
        pxTCB,
        xCfg->xPriority,
        &pxTCB->xHandle
    );

    pxTCB->xNextRelease = 0;

    if ( xTaskStatus != pdPASS )
    {
        return PTL_NO_MEMORY;
    }

    /* Export the xHandle as an opaque pointer */
    if ( pxHandle != NULL )
    {
        *pxHandle = ( PTL_TaskHandle_t * )pxTCB;
    }

    ucTaskCount++;
    return PTL_OK;
}


void vPTL_PrintPTLCfg( SchedulerConfig_t sch_cfg )
{
    UART_printf( "[CFG] maxTasks=");
    UART_printfvalue( sch_cfg.maxTasks );
    UART_printf( " policy=" );
    const char *pcPolicyString;
    switch( sch_cfg.xPolicy )
    {
        case PTL_OVERRUN_SKIP:
            pcPolicyString = "SKIP";
            break;
        case PTL_OVERRUN_KILL:
            pcPolicyString = "KILL";
            break;
        case PTL_OVERRUN_CATCH_UP:
            pcPolicyString = "CATCH_UP";
            break;
        default:
            pcPolicyString = "UNKNOWN";
            break;
    }
    UART_printf( pcPolicyString );
    UART_printf( " trace=" );
    UART_printfvalue( ( sch_cfg.xTraceEnabled == pdTRUE ) ? 1 : 0 );
    UART_printf("\n");
}


PTL_Status_t PTL_GetTaskStats( PTL_TaskHandle_t *pxHandle, PTL_TaskStats_t *pxStats )
{
    struct PTL_TCB *pxTCB = ( struct PTL_TCB * ) pxHandle;

    if ( pxTCB == NULL || pxStats == NULL )
    {
        return PTL_INVALID_PARAM;
    }

    taskENTER_CRITICAL();
    pxStats->ulJobCounter       = pxTCB->ulJobCounter;
    pxStats->ulDeadlinesMisses  = pxTCB->ulDeadlinesMisses;
    pxStats->ulOverruns         = pxTCB->ulOverruns;
    pxStats->xMaxReleaseJitter  = pxTCB->xMaxReleaseJitter;
    taskEXIT_CRITICAL();

    return PTL_OK;
}


void vPTL_PrintTaskParam( PTL_TaskConfig_t task )
{
    UART_printf( "[TASK] name=" );
    UART_printf( task.pcTaskName );
    
    UART_printf( " period=" );
    UART_printfvalue( ( uint32_t ) task.xPeriod );

    UART_printf( " deadline=" );
    UART_printfvalue( ( uint32_t ) task.xDeadline );

    UART_printf( " priority=" );
    UART_printfvalue( ( uint32_t ) task.xPriority );

    UART_printf( " phase=");
    UART_printfvalue( ( uint32_t ) task.xPhase );
    UART_printf( "\n" );   
}


/**
 * @brief FreeRTOS tick hook used by the PTL.
 * Invokes the internal PTL scheduler tick function.
 */
void vApplicationTickHook(void) {
    prvPTL_SchedulerTick();
}


/**
 * @brief FreeRTOS idle hook used by the PTL.
 *
 * Flushes the log buffer and gives IDLE events when
 * the system becomes idle.
 */
void vApplicationIdleHook( void )
{
    static TickType_t xLastTick = 0;
    TickType_t xNow = PTL_GetTotalTicks();


    if ( ulLogTail != ulLogHead )
    {
        prvPTL_FlushLogBuffer();
    }

    if ( xIsSystemIdle == pdFALSE )
    {
        prvPTL_Log( PTL_EVENT_IDLE, NULL, 0 );
        xIsSystemIdle = pdTRUE;
    }
    
    if ( xNow != xLastTick )
    {
        xIdleTicks += (xNow - xLastTick);
        xLastTick = xNow;
    }
}


/**
 * @brief FREERTOS stack overflow hook
 *
*/
void vApplicationStackOverflowHook ( TaskHandle_t xTask, char *pcTaskName )
{
    ( void ) xTask;
    UART_printf( "[FATAL] Stack Overflow: " );
    UART_printf( pcTaskName );
    UART_printf( "\n" );
    taskDISABLE_INTERRUPTS();
    for ( ;; );
}


/**
 * @brief FREERTOS malloc fail hook
 *
*/
void vApplicationMallocFailedHook( void )
{
    UART_printf( "[FATAL] Malloc Failed\n " );
    taskDISABLE_INTERRUPTS();
    for ( ;; );
}


/**
 * @brief Returns the total number of ticks the system has spent busy scheduling
 * since xPTL_Start() was called (t0).
*/
TickType_t PTL_GetOverheadTicks( void )
{
    return xOverheadTicks;
}


/**
 * @brief Returns the number of ticks elapsed since xPTL_Init() was called (t0).
*/
TickType_t PTL_GetTotalTicks( void )
{
    return ( TickType_t )( xTaskGetTickCount() - xInitTickCount );
}


/**
 * @brief ONLY FOR TESTING PURPOSES: allows to check jitter
 */
TickType_t PTL_GetMaxJitter( PTL_TaskHandle_t *pxHandle )
{
    struct PTL_TCB *pxTCB = ( struct PTL_TCB * ) pxHandle;
    if ( pxTCB == NULL )
    {
        return 0;
    }
    return pxTCB->xMaxReleaseJitter;
}

/**
 * @brief ONLY FOR TESTING PURPOSES: allows to shut down all the tasks
 */
void PTL_TestCleanup( void )
{
    xInitialized = pdFALSE;

    for ( uint8_t ux = 0; ux < ucTaskCount; ux++ )
    {
        if (pxTaskTable[ ux ].xHandle != NULL ) 
        {
            vTaskDelete( pxTaskTable[ ux ].xHandle );
        }
        pxTaskTable[ ux ].xHandle = NULL;
    }
        ucTaskCount = 0;


    if ( pxTaskTable != NULL )
    {
        vPortFree( pxTaskTable );
        pxTaskTable = NULL;
    }
}
