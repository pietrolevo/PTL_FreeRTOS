/*
 * DEMO - PTL
 * Group3 - 2026
 *
 * demo_Tasks.c DEMO
 */

#include "demo_tasks.h"
#include "ptl.h"
#include "task.h"
#include "uart.h"

QueueHandle_t g_xSpeedQueue = NULL;
SemaphoreHandle_t g_xBrakeSemaphore = NULL;

#define ENGINE_RPM_MIN 800U
#define ENGINE_RPM_MAX 4000U
#define BRAKE_RPM_DECREMENT 300U


static uint16_t s_usEngineRpm = ENGINE_RPM_MIN;


/* Generates a pseudo-random value in [0, usMax) using a simple linear congruential generator. */
static uint16_t prvRandom( uint16_t usMax )
{
    static uint32_t ulState = 0;
    static BaseType_t xSeeded = pdFALSE;

    if ( xSeeded == pdFALSE )
    {
        ulState = ( uint32_t ) xTaskGetTickCount() + 1U;
        xSeeded = pdTRUE;
    }

    ulState = ( ulState * 1103515245UL ) + 12345UL;
    return ( uint16_t ) ( ( ulState >> 16 ) % usMax );
}


/* Simulates the engine RPM ramping up/down with noise and publishes it to the speed queue. */
void vEngineSensorTask( void *pArg )
{
    ( void ) pArg;
    static int8_t cStep = 200;
    int16_t sNoise;
    uint16_t usRpmSnapshot;

    taskENTER_CRITICAL();

    s_usEngineRpm = ( uint16_t ) ( s_usEngineRpm + cStep );
    if ( s_usEngineRpm >= ENGINE_RPM_MAX || s_usEngineRpm <= ENGINE_RPM_MIN )
    {
        cStep = -cStep;
    }

    sNoise = ( int16_t ) prvRandom( 101 ) - 50;
    if ( ( sNoise < 0 ) && ( ( uint16_t ) ( -sNoise ) > s_usEngineRpm ) )
    {
        sNoise = 0;
    }
    s_usEngineRpm = ( uint16_t ) ( s_usEngineRpm + sNoise );

    usRpmSnapshot = s_usEngineRpm;

    taskEXIT_CRITICAL();

    PTL_CheckAbort();

    xQueueOverwrite( g_xSpeedQueue, &usRpmSnapshot );
    PTL_vTaskDelay(50);
}


/* Reads the latest RPM value from the speed queue and prints it to the dashboard over UART. */
void vDashboardTask( void *pArg )
{
    ( void ) pArg;
    uint16_t usRpm = 0;

    if ( xQueuePeek( g_xSpeedQueue, &usRpm, 0 ) == pdPASS )
    {
        UART_printf( "[DASH] RPM=" );
        UART_printfvalue( usRpm );
        UART_printf( "\n" );
    }

    PTL_CheckAbort();
    PTL_vTaskDelay(200);
}


/* Randomly simulates the driver pressing the brake pedal by signaling the brake semaphore. */
void vBrakePedalSimTask( void *pArg )
{
    ( void ) pArg;

    if ( prvRandom( 5 ) == 0 )
    {
        xSemaphoreGive( g_xBrakeSemaphore );
    }

    PTL_CheckAbort();
    PTL_vTaskDelay(100);
}


/* Reacts to a pending brake signal by dropping the RPM and reporting the event over UART. */
void vBrakeMonitorTask( void *pArg )
{
    ( void ) pArg;
    uint16_t usRpmAfterBrake;

    if ( xSemaphoreTake( g_xBrakeSemaphore, 0 ) == pdPASS )
    {
        taskENTER_CRITICAL();
        if ( s_usEngineRpm > ( ENGINE_RPM_MIN + BRAKE_RPM_DECREMENT ) )
        {
            s_usEngineRpm = ( uint16_t ) ( s_usEngineRpm - BRAKE_RPM_DECREMENT );
        }
        else
        {
            s_usEngineRpm = ENGINE_RPM_MIN;
        }
        usRpmAfterBrake = s_usEngineRpm;
        taskEXIT_CRITICAL();

        PTL_CheckAbort();

        xQueueOverwrite( g_xSpeedQueue, &usRpmAfterBrake );

        UART_printf( "[ALERT] Brake pedal pressed! RPM dropped to " );
        UART_printfvalue( usRpmAfterBrake );
        UART_printf( "\n" );
    }

    PTL_vTaskDelay(50);
}