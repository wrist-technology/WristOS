/*
	FreeRTOS V4.0.1 - Copyright (C) 2003-2006 Richard Barry.

	This file is part of the FreeRTOS distribution.

	FreeRTOS is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS, without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************
	See http://www.FreeRTOS.org for documentation, latest information, license
	and contact details.  Please ensure to read the configuration and relevant
	port sections of the online documentation.
	***************************************************************************
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *----------------------------------------------------------*/

/* The SWI is used by the scheduler. */
#define vPortYieldProcessor swi_handler

#include "config.h"

// MV
#define configUSE_NEWLIB_REENTRANT	1
#define configUSE_MUTEXES           1
#define configUSE_RECURSIVE_MUTEXES 1
#define configCHECK_FOR_STACK_OVERFLOW      2
// end MV
#define configUSE_PREEMPTION		1

#if CFG_PM
#define configUSE_IDLE_HOOK			1
#else
#define configUSE_IDLE_HOOK			0
#endif

#define configUSE_TICK_HOOK			1
#define configCPU_CLOCK_HZ			( ( unsigned portLONG ) 47923200 )
#define configTICK_RATE_HZ			( ( portTickType ) 1000 )
#define configMAX_PRIORITIES		( ( unsigned portBASE_TYPE ) 8 )
#define configMINIMAL_STACK_SIZE	( ( unsigned portSHORT ) 110 )
//MV#define configTOTAL_HEAP_SIZE		( ( size_t ) CONTROLLER_HEAPSIZE)
//MV#define configMAX_TASK_NAME_LEN		( 16 )
#define configMAX_TASK_NAME_LEN		( 32 )
#define configUSE_TRACE_FACILITY	0
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_uxTaskPriorityGet			1
#define INCLUDE_vTaskDelete					1
#define INCLUDE_vTaskCleanUpResources		1
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay					1
#define INCLUDE_xTaskGetCurrentTaskHandle	1

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_CONFIG_H */
