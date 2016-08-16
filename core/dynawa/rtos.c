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

#include "rtos.h"
#include "string.h"

#define CFG_PM

/**********************************************************************************

  Task

 **********************************************************************************/

/**
  Create a new Task.

  A task is the basic unit that runs a program on the Make Controller.  

  @param loop The loop that will run this task.
  @param name A name for your task.
  @param stackDepth How many bytes of memory this task can use.
  @param priority This task's priority, relative to other tasks.  Valid options are from 1-9, 9 being highest priority.
  @param params (optional) A value that will be passed into your task when it's created.

  \b Example
  \code

  \endcode
  */
Task Task_create( TaskLoop loop, const char* name, int stackDepth, int priority, void* params )
{
    Task task;
    if( xTaskCreate( loop, (const signed char*)name, ( stackDepth >> 2 ), params, priority, &task ) != 1 ) {
        return NULL; 
    } else {
        //xTaskSetContext( task, task );
    }
    return task;
}

void Task_delete( Task task )
{
    //if(task)
    vTaskDelete( task );
}

/**
  Delay your task for a number of milliseconds.

  It's important to always include some sleep, or a yield() in your
  task so that other tasks in the system have a chance to run.

  @param ms How many milliseconds to sleep.
  @see yield()

  \b Example
  \code
  void myLoop(void* p)
  {
  while(1) // looping forever
  {
// ...some other processing here...

Task::sleep(10); // wait 10 millis each time through the loop
}
}
\endcode
*/
void Task_sleep( int ms ) // static
{
#ifdef CFG_PM
    sleep(ms);
#else
    vTaskDelay( ms / portTICK_RATE_MS );
#endif
}

/**
  Stop processing the current task, giving time to other tasks.

  \b Example
  \code
  void myLoop(void* p)
  {
  while(1) // looping forever
  {
// ...some other processing here...

Task::yield(); // wait 10 millis each time through the loop
}
}
\endcode
*/
void Task_yield( ) // static
{
    taskYIELD(); 
}

/**
  Start a section that prevents other tasks from interrupting you.

  If you have some very time sensitive operations that all need to be
  performed at once, you may want to make sure that no other tasks
  interrupt you.  You can place this code in a 'critical' section enterCritical().

  Make sure to always match your calls to enterCritical() with a follow
  up call to exitCritical(), otherwise nothing else in the system will
  be able to run.

  @see exitCritical()

  \b Example
  \code
  void myLoop(void* p)
  {
  while(1) // looping forever
  {
  Task::enterCritical()
// ...some very time sensitive processing here...
Task::exitCritical() // now we're done
}
}
\endcode
*/
void Task_enterCritical( ) // static
{
    taskENTER_CRITICAL();
}

/**
  Complete a section that prevents others from interrupting you.

  If you're in a time sensitive section of code that that uses
  enterCritical(), be sure to call this when you're done so that
  other tasks can continue to run.

  @see enterCritical()

  \b Example
  \code
  void myLoop(void* p)
  {
  while(1) // looping forever
  {
  Task::enterCritical()
// ...some very time sensitive processing here...
Task::exitCritical() // now we're done
}
}
\endcode
*/
void Task_exitCritical( ) // static
{
    taskEXIT_CRITICAL();
}

/**
  How much memory is available on this task's stack.

  When you created your task, you specified the amount of memory available
  to it.  You can check how much of that memory is remaining at any time
  with a call to remainingStack().

  @return How many bytes of free memory are remaining.

  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  int stack = myTask->remainingStack();
  \endcode
  */
/* MV
int Task_remainingStack( Task task )
{
    if( ! task )
        return -1;
    else
        return usVoidTaskCheckFreeStackSpace( task );
}
*/

/**
  Get a task's priority

  Each task is assigned a priority, relative to other tasks, when it's created.  
  You can check a task's priority with this method.

  @return The task's priority, from 1-9

  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  int priority = myTask->priority();
  \endcode
  */
/*
int Task_priority( Task task )
{
    return xTaskGetPriority( task );
}
*/

/**
  Set a task's priority.

  Each task is assigned a priority, relative to other tasks, when it's created.  
  You can modify an existing task's priority with this method.

  @param priority The new priority for this task.

  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  void myLoop(void* p)
  {
  while(1) // looping forever
  {
// ...normal task processing...
if(need_to_be_more_important)
myTask->setPriority(9);
}
}
\endcode
*/
void Task_setPriority( Task task, int priority )
{
    vTaskPrioritySet( task, priority );
}

/**
  Get a task's ID number.

  Each task is internally assigned an ID number by the operating system.
  You can read a task's ID number by calling its id() method.

  @return The task's ID number

  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  int some_id = myTask->id();
  \endcode
  */
/* MV
int Task_id( Task task )
{
    return xTaskGetIDNumber( task );
}
*/

/**
  Get a task's name.

  Each task is given a name when it's created.  You can check a 
  task's name with this method.

  @return The task's name

  \b Example
  \code
  Task* myTask = new Task( myLoop, "Me!", 1000, 1);
  char* name = myTask->name();
// now name should be "Me!"
\endcode
*/
/*
char* Task_name( Task task )
{
    return (char*)xTaskGetName( task );
}
*/

/**
  Get the next task in the system.

  You can loop through all the tasks in the system using the nextTask()
  method.  Once it has reached the last task, it will wrap back around
  and return the first task in the list.

  @return The next Task

  \b Example
  \code
// find the task with the highest priority
Task* t = RTOS::currentTask();
int tasks_to_find = 5; // adjust for the number of tasks you want to check
int highest = 0;
while(tasks_to_find--)
{
t = t->nextTask();
if(t->priority() > highest)
highest = t->priority();
}

\endcode
*/

/////////////MV


#if 0
Task Task_nextTask( Task task )
{
    void* tcb = NULL;
    vTaskSuspendAll();
    {
        tcb = RTOS_getNextTaskControlBlock( task );
    }
    xTaskResumeAll( );
    return (tcb) ? (Task)xTaskGetContext(tcb) : NULL;
}

/**********************************************************************************

  RTOS

 **********************************************************************************/

/**
  Find a Task by its name.

  Each Task is given a name when it's created.  If you have the name, but
  not a pointer to the Task, use this method to look it up.

  @param name The name of the Task.
  @return The Task, or NULL if not found.

  \b Example
  \code
  Task* t = RTOS::findTaskByName("myGreatTask");
  if(t != NULL)
  {
// then we found the task successfully.
}
\endcode
*/
Task RTOS_findTaskByName( const char* name ) // static
{
    void *tcb = NULL;
    vTaskSuspendAll();
    {
        tcb = RTOS_findTask( (char*)name, -1 );
    }
    xTaskResumeAll();
    return (tcb) ? (Task)xTaskGetContext(tcb) : NULL;
}

/**
  Find a Task by its ID.

  Each Task is assigned an ID number by the RTOS.  If you have an ID,
  but not a pointer to the Task, use this method to look it up.

  @param id The ID of the Task you're looking for.
  @return A pointer to the Task, or NULL if not found.

  \b Example
  \code
// get the ID for the current task so we can look it up later if we want to
Task* current = RTOS::currentTask();
int id = current->id();

// ... so some other things for a while...

Task* earlier = RTOS::findTaskByID(id);
\endcode
*/
Task RTOS_findTaskByID( int id ) // static
{
    void* tcb = NULL;
    vTaskSuspendAll();
    {
        tcb = RTOS_findTask( NULL, id );
    }
    xTaskResumeAll();
    return (tcb) ? (Task)xTaskGetContext(tcb) : NULL;
}

/**
  Get the task that's currently running.

  @return A pointer to the currently running Task

  \b Example
  \code
  Task* current = RTOS::currentTask();
  \endcode
  */
Task RTOS_currentTask( ) // static
{
    return (Task)xTaskGetContext(xTaskGetCurrentTaskHandle());
}

/**
  Get the number of tasks that have been created.

  @return The number of tasks.

  \b Example
  \code
  int total_tasks = RTOS::numberOfTasks( );
  \endcode
  */
int RTOS_numberOfTasks( ) // static
{
    return uxTaskGetNumberOfTasks( );
}

/**
  Check the highest priority assigned to any task.

  @return The highest priority of any task, 1-9.

  \b Example
  \code
  int top_dog = RTOS::topTaskPriority( );
  \endcode
  */
int RTOS_topTaskPriority( ) // static
{
    return xTaskGetTopUsedPriority( );
}

/**
  The number of ticks that have happened since the board last booted.

  A tick in the RTOS is roughly equivalent to 1 millisecond.  This value
  will wrap once it exceeds 4,294,967,296 (2^32).

  @return The number of ticks since boot. 

  \b Example
  \code
// record how long it takes to perform some task
int start_time = RTOS::ticksSinceBoot( );
while(doingsometask)
{
// do something in here you want to measure...
}
int elapsed_time = RTOS::ticksSinceBoot( ) - start_time;
\endcode
*/
int RTOS_ticksSinceBoot( ) // static
{
    return xTaskGetTickCount( );
}

void* RTOS_getNextTaskControlBlock( void* task )
{
    int currentID;
    int lowID = 1024; // some large number that will be greater than the number of tasks...
    int highID = 1024;
    void* lowTask = NULL;
    void* highTask = NULL;
    void* tcb = NULL;
    //Task* t;
    unsigned portBASE_TYPE uxQueue = RTOS_topTaskPriority( ) + 1;

    if( task == NULL )
        currentID = 1;
    else
    {
        //t = (Task*)xTaskGetContext(task);
        currentID = Task_id(task);
    }

    // look through all the possible lists of tasks
    do
    {
        uxQueue--;
        if( !listLIST_IS_EMPTY( GetReadyTaskByPriority( uxQueue ) ) )
        {
            tcb = RTOS_iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetReadyTaskByPriority( uxQueue ) );
            if( tcb != NULL )
                return tcb;
        }   
    }while( uxQueue > ( unsigned portSHORT ) tskIDLE_PRIORITY );

    // check the list of delayed tasks
    if( !listLIST_IS_EMPTY( GetDelayedTaskList( ) ) )
    {
        tcb = RTOS_iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetDelayedTaskList( ) );
        if( tcb != NULL )
            return tcb;
    }
    // check the list of overflow delayed tasks
    if( !listLIST_IS_EMPTY( GetOverflowDelayedTaskList( ) ) )
    {
        tcb = RTOS_iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetOverflowDelayedTaskList( ) );
        if( tcb != NULL )
            return tcb;
    }
    // check the list of tasks about to die
    if( !listLIST_IS_EMPTY( GetListOfTasksWaitingTermination( ) ) )
    {
        tcb = RTOS_iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetListOfTasksWaitingTermination( ) );
        if( tcb != NULL )
            return tcb;
    }
    // check the list of suspended tasks
    if( !listLIST_IS_EMPTY( GetSuspendedTaskList( ) ) )
        tcb = RTOS_iterateForNextTask( &lowTask, &lowID, &highTask, &highID, currentID, GetSuspendedTaskList( ) );


    if( highTask )
        return highTask;
    else
        return lowTask;
}

// one function to search either by name or ID
// pass in a garbage value for the parameter you're not interested in
void* RTOS_findTask( char *taskName, int taskID )
{
    bool byName;
    if( taskName == NULL && taskID > 0 )
        byName = false;
    if( taskName != NULL && taskID < 0 )
        byName = true;

    unsigned portBASE_TYPE uxQueue = RTOS_topTaskPriority( ) + 1;
    void *tcb = NULL;

    // look through all the possible lists of tasks
    do
    {
        uxQueue--;
        if( !listLIST_IS_EMPTY( GetReadyTaskByPriority( uxQueue ) ) )
        {
            if( byName )
                tcb = RTOS_iterateByName( taskName, GetReadyTaskByPriority( uxQueue ) );
            else
                tcb = RTOS_iterateByID( taskID, GetReadyTaskByPriority( uxQueue ) );

            if( tcb != NULL )
                return tcb;
        }   
    }while( uxQueue > ( unsigned portSHORT ) tskIDLE_PRIORITY );

    // check the list of delayed tasks
    if( !listLIST_IS_EMPTY( GetDelayedTaskList( ) ) )
    {
        if( byName)
            tcb = RTOS_iterateByName( taskName, GetDelayedTaskList( ) );
        else
            tcb = RTOS_iterateByID( taskID, GetDelayedTaskList( ) );

        if( tcb != NULL )
            return tcb;
    }
    // check the list of overflow delayed tasks
    if( !listLIST_IS_EMPTY( GetOverflowDelayedTaskList( ) ) )
    {
        if( byName )
            tcb = RTOS_iterateByName( taskName, GetOverflowDelayedTaskList( ) );
        else
            tcb = RTOS_iterateByID( taskID, GetOverflowDelayedTaskList( ) );
        if( tcb != NULL )
            return tcb;
    }
    // check the list of tasks about to die
    if( !listLIST_IS_EMPTY( GetListOfTasksWaitingTermination( ) ) )
    {
        if( byName )
            tcb = RTOS_iterateByName( taskName, GetListOfTasksWaitingTermination( ) );
        else
            tcb = RTOS_iterateByID( taskID, GetListOfTasksWaitingTermination( ) );

        if( tcb != NULL )
            return tcb;
    }
    // check the list of suspended tasks
    if( !listLIST_IS_EMPTY( GetSuspendedTaskList( ) ) )
    {
        if( byName )
            tcb = RTOS_iterateByName( taskName, GetSuspendedTaskList( ) );
        else
            tcb = RTOS_iterateByID( taskID, GetSuspendedTaskList( ) );
    }
    return tcb;
}

void* RTOS_iterateByID( int id, xList* pxList )
{
    void *pxNextTCB, *pxFirstTCB;
    //Task* t;
    listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
    do
    {
        listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
        //t = (Task*)xTaskGetContext( pxNextTCB );
        //if( t->id() == id )
        if( Task_id( pxNextTCB ) == id )
            return pxNextTCB;

    } while( pxNextTCB != pxFirstTCB );
    // if we get down here, we didn't find it.
    return NULL;
}

void* RTOS_iterateByName( char* taskName, xList* pxList )
{
    void *pxNextTCB, *pxFirstTCB;
    //Task* t;
    listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
    do
    {
        listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
        // t = (Task*)xTaskGetContext( pxNextTCB );
        //if( strcmp( t->name(), taskName ) == 0 )
        if( strcmp( Task_name( pxNextTCB ), taskName ) == 0 )
            return pxNextTCB;

    } while( pxNextTCB != pxFirstTCB );
    // if we get down here, we didn't find it.
    return NULL;
}

void* RTOS_iterateForNextTask( void** lowTask, int* lowID, void** highTask, int* highID, int currentID, xList* pxList )
{
    void *pxNextTCB, *pxFirstTCB;
    //Task* t;
    listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
    do
    {
        listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
        //t = (Task*)xTaskGetContext( pxNextTCB );
        //int id = t->id();
        int id = Task_id( pxNextTCB );
        if( id == (currentID + 1) ) // if it's the ID we're looking for plus one, we got it and we're out.
            return pxNextTCB;
        if( id > currentID && id < *highID )
        {
            *highID = id;
            *highTask = pxNextTCB;
        }
        if( id < currentID && id < *lowID && id != 1 ) // we don't want id 1 since that's IDLE and there's no real task there.
        {
            *lowID = id;
            *lowTask = pxNextTCB;
        }
    } while( pxNextTCB != pxFirstTCB );
    // if we get down here, we didn't find it.
    return NULL;
}
#endif

/**********************************************************************************

  Queue

 **********************************************************************************/

/**
  Create a new Queue.

  When you create a Queue, you specify both the size of the items that it will
  contain, and how many of them to make room for.  This means that all the items
  in the list should be the same kind, or the same size at the very least.  
  It's usually most convenient to use the sizeof() function to determine the 
  size of the items you want to keep in the Queue.  

  @param length How many items to make room for.
  @param itemSize The size of each item.

  \b Example
  \code
// create a Queue for 100 ints
Queue* q = new Queue( 100, sizeof(int) );
\endcode
*/
Queue Queue_create( uint length, uint itemSize )
{
    return xQueueCreate( length, itemSize );
}

void Queue_delete( Queue queue )
{
    vQueueDelete( queue );
}

/**
  Add an item to the Queue.

  If the Queue is full, this method will wait as long as you specify
  in the \b timeout parameter for it to become available.  The size of
  the item that you add must match the size that you specified when
  you created the Queue.

  If you want to do this from within an ISR, use sendFromISR() instead.

  @param itemToQueue
  @param timeout (optional) The number of milliseconds to wait if the queue is full 
  or not available immediately.  If this is omitted or less than 0, it will wait indefinitely.
  @return True if the item was successfully sent, false if not.

  \b Example
  \code
// add an integer to this Queue
Queue q( 50, sizeof(int) );
int to_be_queued = 512;
q.send( &to_be_queued );
\endcode
*/
bool Queue_send( Queue queue, void* itemToQueue, int timeout )
{
    timeout = (timeout < 0) ? portMAX_DELAY : (timeout/portTICK_RATE_MS);
    return xQueueSend( queue, itemToQueue, timeout ) == pdTRUE;
}

/**
  Receive an item from the Queue.

  If the Queue is empty, this method will wait as long as you specify 
  in the \b timeout parameter for some data to arrive.  The size of the item
  will be what you specified when you created the Queue.

  If you want to do this from within an ISR, use receiveFromISR() instead.

  @param buffer Where to place the received item.
  @param timeout (optional) The number of milliseconds to wait if the queue is empty.
  If this is omitted or less than 0, it will wait indefinitely.
  @return True if an item was received, false if not.

  \b Example
  \code
// pull an integer off the Queue
// assume our Queue q was already created...
int to_be_received;
if( q.receive(&to_be_received) )
{
// then we received successfully
}
\endcode
*/
bool Queue_receive( Queue queue, void* buffer, int timeout )
{
    timeout = (timeout < 0) ? portMAX_DELAY : (timeout/portTICK_RATE_MS);
    return xQueueReceive( queue, buffer, timeout ) == pdTRUE;
}

/**
  Check the number of messages available in a Queue.

  @return The number of messages currently in the Queue.

  \b Example
  \code
  Queue q( 50, sizeof(int) );
  int msgs = q.msgsAvailable();
  \endcode
  */
int Queue_msgsAvailable( Queue queue )
{
    return uxQueueMessagesWaiting( queue );
}

/**
  Put an item on the Queue from within an interrupt handler.

  Same as send() except suitable for use within an interrupt handler.  Because
  this is used in an interrupt handler, there can be no timeout parameter.

  @param itemToSend The item to send on the Queue.
  @param taskWoken Did this send cause another task to be woken up?  This is required
  in cases where multiple calls to sendFromISR() might happen in the same interrupt.
  @return True if the item was sent successfully, false if not.

  \b Example
  \code
  Queue q( 50, sizeof(int) );

  void myISRHandler()
  {
  int woken = 0;
  q.sendFromISR( 34, &woken );
  }
  \endcode
  */
bool Queue_sendFromISR( Queue queue, void* itemToSend, int* taskWoken )
{
    return xQueueSendFromISR( queue, itemToSend, (long int*)taskWoken ) == pdTRUE;
}

/**
  Receive an item from a Queue within an interrupt handler.

  Same as receive() except suitable for use within an interrupt handler.  Because
  this is used in an interrupt handler, there can be no timeout parameter.

  @param buffer Where to place the received item
  @param taskWoken Did this receive cause another task to be woken up?  This is required
  in cases where multiple calls to receiveFromISR() might happen in the same interrupt.
  @return True if the item was received successfully, false if not.

  \b Example
  \code
  Queue q(50, sizeof(int));

  void myISRHandler()
  {
  int woken = 0;
  int to_be_received;
  q.receiveFromISR( &to_be_received, &woken );
  }
  \endcode
  */
bool Queue_receiveFromISR( Queue queue, void* buffer, int* taskWoken )
{
    return xQueueReceiveFromISR( queue, buffer, (long int*)taskWoken ) == pdTRUE;
}

/**********************************************************************************

  Semaphore

 **********************************************************************************/

/**
  Create a new Semaphore.

  \b Example
  \code
  Semaphore sem;
// or allocate from the heap
Semaphore* sem = new Semaphore();
\endcode
*/
Semaphore Semaphore_create( )
{
    Semaphore semaphore;
    vSemaphoreCreateBinary( semaphore );
    return semaphore;
}

void Semaphore_delete( Semaphore semaphore )
{
    vQueueDelete( semaphore );
}

/**
  Lock a semaphore.

  If a semaphore is already locked by something else, this method will wait
  until it's released.

  @param timeout (optional) How long to wait if the Semaphore is already locked.
  If this is omitted or less than 0, it will wait indefinitely.
  @return True if it was successfully taken in the timeout specified, false if not.

  \b Example
  \code
  Semaphore s;
  if( s.take() )
  {
// then we successfully got the lock
}
\endcode
*/
bool Semaphore_take( Semaphore semaphore, int timeout )
{
    timeout = ( timeout < 0 ) ? portMAX_DELAY : timeout / portTICK_RATE_MS;
    return xSemaphoreTake( semaphore, timeout ) == pdTRUE;
}

/**
  Release a Sempahore.

  If you want to release the semaphore from within an interrupt handler, use
  giveFromISR() instead.

  @return True if it was successfully given, false if not.

  \b Example
  \code
  Semaphore s;
  if( s.give() )
  {
// then we successfully released the lock
}
\endcode
*/
bool Semaphore_give( Semaphore semaphore )
{
    return xSemaphoreGive( semaphore ) == pdTRUE;
}

/**
  Release a Semaphore from within an interrupt handler.

  Same as give() but suitable for use within an interrupt handler.  Because of this,
  no timeout can be specified.

  @param taskWoken Did this receive cause another task to be woken up?  This is required
  in cases where multiple calls to giveFromISR() might happen in the same interrupt.
  @return True if it was released successfully, false if not.

  \b Example
  \code
  Semaphore sem;

  void myISRHandler()
  {
  int woken = 0;
  if( ready_to_release )
  sem.giveFromISR(&woken);
  }
  \endcode
  */
bool Semaphore_giveFromISR( Semaphore semaphore, int* taskWoken )
{
    return xSemaphoreGiveFromISR( semaphore, (long int*)taskWoken ) == pdTRUE;
}





