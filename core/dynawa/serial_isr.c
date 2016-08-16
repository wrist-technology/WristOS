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

#include "serial.h"
#include "rtos.h"
#include "debug/trace.h"

extern bool in_panic_handler;

// The interrupt entry point is naked so we can control the context saving.
void Serial0Isr_Wrapper( void ) __attribute__ ((naked));
void Serial1Isr_Wrapper( void ) __attribute__ ((naked));

/* The interrupt handler function must be separate from the entry function
   to ensure the correct stack frame is set up. */
void SerialIsr_Handler( int index );

void SerialIsr_Handler( int index )
{
    signed portCHAR cChar;
    int xTaskWokenByTx = false;
    int xTaskWokenByPost = false;
    long xTaskWokenByTxThis = false;
    long xTaskWokenByPostThis = false;
    Serial_Internal* sp = &Serial_internals[index];

#ifdef CFG_DEEP_SLEEP
    check_power_mode();
#endif

    //TRACE_INFO("SER ISR %d %x %x\r\n", index, sp->uart->US_CSR, sp->uart->US_IMR);
    TRACE_SER("SER ISR %d %x %x\r\n", index, sp->uart->US_CSR, sp->uart->US_IMR);
    unsigned int status = ( sp->uart->US_CSR ) & ( sp->uart->US_IMR ); // What caused the interrupt?
    if( status & AT91C_US_TXRDY ) 
    { 
        //TRACE_SER("Tx\r\n");
        /* The interrupt was caused by the THR becoming empty. Are there any 
           more characters to transmit? */ 
        if( Queue_receiveFromISR(sp->txQueue, &cChar, &xTaskWokenByTx ) == pdTRUE ) 
        { 
            // A character was retrieved from the queue so can be sent to the THR now.
            sp->uart->US_THR = cChar; 
        } 
        else // Queue empty, nothing to send so turn off the Tx interrupt.
            sp->uart->US_IDR = AT91C_US_TXRDY; 
    } 

    if( status & AT91C_US_RXRDY ) 
    { 
        /*
           if (in_panic_handler)
           TRACE_SER("SER ISR RX %d\r\n", index);
           */
        /* The interrupt was caused by a character being received. Grab the 
           character from the RHR and place it in the queue or received  
           characters. */ 
#if DMA
        sp->uart->US_IDR = AT91C_US_RXRDY;
/* test mv
        sp->uart->US_PTCR = AT91C_PDC_RXTEN;
        //int t = sp->uart->US_RHR;
        //cChar = t & 0xFF; 
        //char *rpr = (char*)sp->uart->US_RPR;
        //*sp->rpr = (char)(sp->uart->US_RHR & 0xff);
        //sp->uart->US_RPR++;
        //sp->uart->US_RCR--;
        //Serial_DMARxStart(index);        
        sp->uart->US_IDR = AT91C_US_RXRDY;
        Semaphore_giveFromISR(sp->rxSem, &xTaskWokenByPost);
*/
#else
        int t = sp->uart->US_RHR;
        cChar = t & 0xFF; 
        //if (!in_panic_handler)
        Queue_sendFromISR(sp->rxQueue, &cChar, &xTaskWokenByPost );
#endif
    }

//#if DMA
    // DMA
/*
    if( status & AT91C_US_ENDTX ) {
        TRACE_SER("IRQ ENDTX\r\n");
        Semaphore_giveFromISR(sp->txSem, &xTaskWokenByTx);
        sp->uart->US_IDR = AT91C_US_ENDTX; 
    }
    if( status & AT91C_US_ENDRX ) {
        //TRACE_SER("IRQ ENDRX\r\n");
        Semaphore_giveFromISR(sp->rxSem, &xTaskWokenByPost);
        sp->uart->US_IDR = AT91C_US_ENDRX; 
    }
*/
    if( status & AT91C_US_TXBUFE ) {
        //TRACE_SER("IRQ TXBUFE\r\n");
        Semaphore_giveFromISR(sp->txSem, &xTaskWokenByTx);
        sp->uart->US_IDR = AT91C_US_TXBUFE; 
    }
/*
    if( status & AT91C_US_RXBUFF ) {
        //TRACE_SER("IRQ RXBUFE\r\n");
        Semaphore_giveFromISR(sp->rxSem, &xTaskWokenByPost);
        sp->uart->US_IDR = AT91C_US_RXBUFF; 
    }
*/
//#endif
    if( status & AT91C_US_RXBRK ) {
        //Semaphore_giveFromISR(sp->txSem, &xTaskWokenByTx);
        //sp->uart->US_IDR = AT91C_US_TXBUFE; 
        sp->uart->US_CR = AT91C_US_RSTSTA; 
        if (sp->rxBreak) {
            Semaphore_giveFromISR(sp->rxSem, &xTaskWokenByPost);
            //sp->uart->US_IDR = AT91C_US_RXBRK;
        } else {
            Serial_DMARxStop(index);
        }
        sp->rxBreak = !sp->rxBreak;
    }

    xTaskWokenByTx = xTaskWokenByTx || xTaskWokenByTxThis; 
    xTaskWokenByPost = xTaskWokenByPost || xTaskWokenByPostThis; 

    /* End the interrupt in the AIC. */ 
    AT91C_BASE_AIC->AIC_EOICR = 0;

    /* If a task was woken by either a frame being received then we may need to 
       switch to another task.  If the unblocked task was of higher priority then
       the interrupted task it will then execute immediately that the ISR
       completes. */
    if( xTaskWokenByPost || xTaskWokenByTx )
    {
        portYIELD_FROM_ISR();
    }
}

void Serial0Isr_Wrapper( void )
{
    portSAVE_CONTEXT(); // Save the context of the interrupted task.
    /* Call the handler to do the work.  This must be a separate
       function to ensure the stack frame is set up correctly. */
    SerialIsr_Handler(0);
    portRESTORE_CONTEXT(); // Restore the context of whichever task will execute next.
}

void Serial1Isr_Wrapper( void )
{
    portSAVE_CONTEXT(); // Save the context of the interrupted task.
    /* Call the handler to do the work.  This must be a separate
       function to ensure the stack frame is set up correctly. */
    SerialIsr_Handler(1);
    portRESTORE_CONTEXT(); // Restore the context of whichever task will execute next.
}


