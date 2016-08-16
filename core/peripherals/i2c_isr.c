#if I2C_IRQ

#include "hardware_conf.h"
#include "spi.h"
#include "rtos.h"
#include "debug/trace.h"

// The interrupt entry point is naked so we can control the context saving.
void i2c_Isr_Wrapper( void ) __attribute__ ((naked));

/* The interrupt handler function must be separate from the entry function
   to ensure the correct stack frame is set up. */
void i2c_Isr_Handler( );

extern xSemaphoreHandle i2c_semaphore;

void i2c_Isr_Handler( )
{
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;

    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(i2c_semaphore, &xHigherPriorityTaskWoken);

    pTWI->TWI_IDR = 0xffffffff;

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

void i2c_Isr_Wrapper( void )
{
    portSAVE_CONTEXT(); // Save the context of the interrupted task.
    
#ifdef CFG_DEEP_SLEEP
    check_power_mode();
#endif

    /* Call the handler to do the work.  This must be a separate
       function to ensure the stack frame is set up correctly. */
    i2c_Isr_Handler();
    portRESTORE_CONTEXT(); // Restore the context of whichever task will execute next.
}

#endif // I2C_IRQ
