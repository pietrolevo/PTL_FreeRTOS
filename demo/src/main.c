/*
 * DEMO - PTL
 * Group3 - 2026
 *
 * main.c DEMO
 */

#include "FreeRTOS.h"
#include "task.h"
#include "ptl.h"
#include "uart.h"
#include "demo_tasks.h"

/* Sets up demo resources, registers the four periodic tasks and starts the PTL scheduler. */
int main( void )
{
    UART_init();

    SchedulerConfig_t xSchedCfg = {
        .maxTasks = 4,
        .xPolicy = PTL_OVERRUN_SKIP,
        .xTraceEnabled = pdTRUE
    };

    PTL_Status_t xStatus;

    g_xSpeedQueue = xQueueCreate( 1, sizeof( uint16_t ) );
    g_xBrakeSemaphore = xSemaphoreCreateBinary();

    if ( ( g_xSpeedQueue == NULL ) || ( g_xBrakeSemaphore == NULL ) )
    {
        UART_printf( "[FATAL] Resource creation failed\n" );
        for ( ;; );
    }

    xStatus = xPTL_Init( &xSchedCfg );
    if ( xStatus != PTL_OK )
    {
        UART_printf( "[FATAL] PTL_Init failed\n" );
        for ( ;; );
    }

    PTL_TaskConfig_t xEngineCfg = {
        .pcTaskName = "EngineSensor",
        .PTL_JobFunction_t = vEngineSensorTask,
        .usStack_size = 256,
        .xPriority = 3,
        .xPeriod = pdMS_TO_TICKS( 100 ),
        .xDeadline = pdMS_TO_TICKS( 50 ),
        .pArg = NULL,
        .xPhase = 0
    };

    PTL_TaskConfig_t xDashboardCfg = {
        .pcTaskName = "Dashboard",
        .PTL_JobFunction_t = vDashboardTask,
        .usStack_size = 256,
        .xPriority = 2,
        .xPeriod = pdMS_TO_TICKS( 200 ),
        .xDeadline = pdMS_TO_TICKS( 150 ),
        .pArg = NULL,
        .xPhase = 0
    };

    PTL_TaskConfig_t xBrakeSimCfg = {
        .pcTaskName = "BrakePedalSim",
        .PTL_JobFunction_t = vBrakePedalSimTask,
        .usStack_size = 256,
        .xPriority = 1,
        .xPeriod = pdMS_TO_TICKS( 500 ),
        .xDeadline = pdMS_TO_TICKS( 500 ),
        .pArg = NULL,
        .xPhase = 0
    };

    PTL_TaskConfig_t xBrakeMonCfg = {
        .pcTaskName = "BrakeMonitor",
        .PTL_JobFunction_t = vBrakeMonitorTask,
        .usStack_size = 256,
        .xPriority = 4,
        .xPeriod = pdMS_TO_TICKS( 50 ),
        .xDeadline = pdMS_TO_TICKS( 50 ),
        .pArg = NULL,
        .xPhase = 0
    };

    xPTL_CreateTask( &xEngineCfg, NULL );
    xPTL_CreateTask( &xDashboardCfg, NULL );
    xPTL_CreateTask( &xBrakeSimCfg, NULL );
    xPTL_CreateTask( &xBrakeMonCfg, NULL );

    xPTL_Start();

    UART_printf( "[FATAL] Scheduler returned\n" );
    for ( ;; );
}