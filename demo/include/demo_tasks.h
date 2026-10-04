/*
 * DEMO - PTL
 * Group3 - 2026
 *
 * demo_tasks.h DEMO
 */

#ifndef DEMO_TASKS_H
#define DEMO_TASKS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

extern QueueHandle_t g_xSpeedQueue;
extern SemaphoreHandle_t g_xBrakeSemaphore;

void vEngineSensorTask( void *pArg );

void vDashboardTask( void *pArg );

void vBrakePedalSimTask( void *pArg );

void vBrakeMonitorTask( void *pArg );

#endif /* DEMO_TASKS_H */