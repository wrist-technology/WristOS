#include "hardware_conf.h"
#include "spi.h"
#include "rtos.h"
#include "debug/trace.h"

// The interrupt entry point is naked so we can control the context saving.
void SPIIsr_Wrapper( void ) __attribute__ ((naked));

/* The interrupt handler function must be separate from the entry function
   to ensure the correct stack frame is set up. */
void SPIIsr_Handler( );

extern xSemaphoreHandle spi_semaphore;

void SPIIsr_Handler( )
{
    SPI pSPI = SPI_BASE;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    unsigned int status = (pSPI->SPI_SR & pSPI->SPI_IMR);

    TRACE_SPI("SPI IRQ\r\n");
    //if( status & (AT91C_SPI_ENDRX | AT91C_SPI_ENDTX)) {
    if( status & AT91C_SPI_ENDRX) {
        xSemaphoreGiveFromISR(spi_semaphore, &xHigherPriorityTaskWoken);

        pSPI->SPI_IDR = AT91C_SPI_ENDRX;
        pSPI->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;
    }


    /* End the interrupt in the AIC. */ 
    AT91C_BASE_AIC->AIC_EOICR = 0;

    /* If a task was woken by either a frame being received then we may need to 
       switch to another task.  If the unblocked task was of higher priority then
       the interrupted task it will then execute immediately that the ISR
       completes. */
    if( xHigherPriorityTaskWoken )
    {
        portYIELD_FROM_ISR();
    }
}

void SPIIsr_Wrapper( void )
{
    portSAVE_CONTEXT(); // Save the context of the interrupted task.
    
#ifdef CFG_DEEP_SLEEP
    check_power_mode();
#endif

    /* Call the handler to do the work.  This must be a separate
       function to ensure the stack frame is set up correctly. */
    SPIIsr_Handler();
    portRESTORE_CONTEXT(); // Restore the context of whichever task will execute next.
}
