/**
 * \file delay.c
 * Simple delay functions
 * 
 * Simple delay functions for micro and milli seconds.
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

#include "hardware_conf.h"
#include "firmware_conf.h"
#include "delay.h"

/**
 * \def STEPS
 * Precalculate the number of steps per uS
 * 
 * This will normally be in the range 4-16
 * 
 * We're using TIMER_CLOCK2 for the timer (MCK/8)
 * 
 */
#define STEPS   ((uint32_t) ((MCK/8) + 500000) / 1000000L)

/**

 * Microseconds delay
 * 
 * Delay the specified number of microseconds. This is a busy wait
 * and the CPU will stay in the loop until the delay is expired.
 * \param   us  The number of microseconds to delay
 * 
*/
void delay(uint32_t us)
{
    *AT91C_PMC_PCER = (1 << AT91C_ID_TC2);  // Enable Clock for TC2
    *AT91C_TC2_CMR = 1;                     // select MCK/8 as clock
    *AT91C_TC2_RC = us * STEPS;             // set compare register C 
    *AT91C_TC2_CCR = 5;                     // enable clock, reset and start timer

    // wait for timer to complete
    // when the compace C flag is set, we're done
    while ((*AT91C_TC2_SR & AT91C_TC_CPCS) == 0) asm volatile ("nop");
    
    *AT91C_PMC_PCDR = (1 << AT91C_ID_TC2);  // Disable Clock for TC2
}

/**

 * Millisecond Delay
 * 
 * Delay the specified number of milliseconds. This is a busy wait
 * and the CPU will stay in the loop until the delay is expired.
 * \param   ms  The number of milliseconds to delay
 * 
*/
void delayms(uint32_t ms)
{
    while (ms--) delay(1000);
}
