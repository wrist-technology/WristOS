/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef RTOS_H
#define RTOS_H

#include "types.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "portmacro.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

/**
  A run loop.
*/
typedef void (TaskLoop)(void*);

/**
  A run loop, or thread, that can run simultaneously with many others.
  
  \section Usage
  Tasks are usually...
  
  \section Memory
  Memory is a pain...
  
  \section Priority
  A few words on priority...
  
  \section Synchronization
  To synchronize...
  
  \ingroup rtos
*/

typedef void* Task;

Task Task_create( TaskLoop loop, const char* name, int stackDepth, int priority, void* params );
void Task_delete( Task task );
void Task_sleep( int ms );
void Task_yield( void );
void Task_enterCritical( void );
void Task_exitCritical( void );
int Task_remainingStack( Task task );
int Task_priority( Task task );
void Task_setPriority( Task task, int priority );
int Task_id( Task task );
char* Task_name( Task task );
Task Task_nextTask( Task task );

/**
  The Real Time Operating System at the heart of the Make Controller.
  
  \ingroup rtos
*/
Task RTOS_findTaskByName( const char* name );
Task RTOS_findTaskByID( int id );
Task RTOS_currentTask( void );
int RTOS_numberOfTasks( void );
int RTOS_topTaskPriority( void );
int RTOS_ticksSinceBoot( void );
void* RTOS_getNextTaskControlBlock( void* task );
void* RTOS_findTask( char *taskName, int taskID );
void* RTOS_iterateByID( int id, xList* pxList );
void* RTOS_iterateByName( char* taskName, xList* pxList );
void* RTOS_iterateForNextTask( void** lowTask, int* lowID, void** highTask, int* highID, int currentID, xList* pxList );

/**
  An inter-task mechanism to pass data.
  
  \ingroup rtos
*/
typedef void *Queue;

Queue Queue_create( uint length, uint itemSize );
void Queue_delete( Queue queue );

bool Queue_send( Queue queue, void* itemToQueue, int timeout );
bool Queue_receive( Queue queue, void* buffer, int timeout );
int Queue_msgsAvailable( Queue queue );
bool Queue_sendFromISR( Queue queue, void* itemToSend, int* taskWoken );
bool Queue_receiveFromISR( Queue queue, void* buffer, int* taskWoken );

/**
  A way to synchronize between different tasks.
  
  \ingroup rtos
*/
typedef void *Semaphore;

Semaphore Semaphore_create( void );
void Semaphore_delete( Semaphore semaphore );
bool Semaphore_take( Semaphore semaphore, int timeout );
bool Semaphore_give( Semaphore semaphore);
bool Semaphore_giveFromISR( Semaphore semaphore, int* taskWoken );

#endif // RTOS__H
