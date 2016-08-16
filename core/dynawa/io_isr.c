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

#include "io.h"
#include "rtos.h"
#include "debug/trace.h"

//extern Io_InterruptSource Io_isrSources[MAX_INTERRUPT_SOURCES];
extern Io_InterruptSource Io_isrSources[];
extern unsigned int Io_isrSourceCount;

void IoAIsr_Wrapper( ) __attribute__ ((naked));
void IoBIsr_Wrapper( ) __attribute__ ((naked));
void IoCIsr_Wrapper( ) __attribute__ ((naked));

void Io_Isr( AT91S_PIO* basePio );

unsigned int status;

void Io_Isr( AT91S_PIO* basePio )
{
#ifdef CFG_DEEP_SLEEP
    check_power_mode();
#endif

    TRACE_INFO("Io_Isr\r\n");
    status = basePio->PIO_ISR;
    status &= basePio->PIO_IMR;

    // Check pending events
    if(status)
    {
        unsigned int i = 0;
        Io_InterruptSource* is;
        //while( status != 0  && i < Io_isrSourceCount )
        while( status != 0  && i < MAX_INTERRUPT_SOURCES)
        {
            is = &(Io_isrSources[i]);
            if( is->port == basePio) // Source is configured on the same controller
            {
                if ((status & is->mask) != 0) // Source has PIOs whose statuses have changed
                {
                    if (is->handler) {
                        is->handler(is->context); // callback the handler
                    }
                    status &= ~(is->mask);    // mark this channel as serviced
                }
            }
            i++;
        }
    }
    AT91C_BASE_AIC->AIC_EOICR = 0; // Clear AIC to complete ISR processing
}

void IoAIsr_Wrapper( )
{
    portSAVE_CONTEXT();        // Save the context of the interrupted task.
    Io_Isr( AT91C_BASE_PIOA ); // execute the handler
    portRESTORE_CONTEXT();     // Restore the context of whichever task will execute next.
}

void IoBIsr_Wrapper( )
{
    portSAVE_CONTEXT();        // Save the context of the interrupted task.
    Io_Isr( AT91C_BASE_PIOB ); // execute the handler
    portRESTORE_CONTEXT();     // Restore the context of whichever task will execute next.
}

void IoCIsr_Wrapper( )
{
    portSAVE_CONTEXT();        // Save the context of the interrupted task.
    Io_Isr( AT91C_BASE_PIOC ); // execute the handler
    portRESTORE_CONTEXT();     // Restore the context of whichever task will execute next.
}


