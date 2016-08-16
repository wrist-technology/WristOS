/**
 * \file abort.c
 * Data and Program Abort handlers.
 *
 * These two functions handle the data and program abort exceptions.
 * Currently, they simpy display a blue screen (data abort), or a red
 * screen (program abort) to notify the user. They then lockup so the
 * watchdog can reset the player.
 * \remarks To test the Data Abort the following code will trigger it:
 * \code
 * {
 *      //
 *      // test data abort
 *      //
 *      uint32_t * ptr = 0x2000000;
 *      *(ptr+1) = 0;
 * }
 * \endcode
 *
 * \remarks To test the Program Abort the following code will trigger it:
 * \code
 * {
 *      //
 *      // test program abort
 *      //
 *      void (*ptr)() = 0x1000003;
 *      ptr();
 * }
 * \endcode
 *
 * AT91SAM7S-128 USB Mass Storage Device with SD Card by Michael Wolf\n
 * Copyright (C) 2008 Michael Wolf\n\n
 *
 * This program is free software: you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation, either version 3 of the License, or\n
 * any later version.\n\n
 *
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 * GNU General Public License for more details.\n\n
 *
 * You should have received a copy of the GNU General Public License\n
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 *
*/

#include "AT91SAM7SE512.h"
#include "trace.h"

/**
 * Program Abort Handler.
 *
 * Exception handler for program aborts.
 * The pointer to this funtion is setup in startup_SAM7.S
 *
 */
void myPAbt_HandlerR(void)
{
    /** \todo Add code for program abort handling here */
    TRACE("\n\n * * Program Abort * * \n\n");
	for(;;);
}

/**
 * Data Abort Handler.
 * 
 * Exception handler for data aborts.
 * The pointer to this funtion is setup in startup_SAM7.S
 * 
 */
void myDAbt_HandlerR(void)
{
    register unsigned long *lnk_ptr;

    __asm__ __volatile__ (
        "sub lr, lr, #8\n"
        "mov %0, lr" : "=r" (lnk_ptr)
    );
    TRACE("\n\n * * Data Abort at %p 0x%08lX * * \n\n", lnk_ptr, *(lnk_ptr));
	for(;;);
}
