#include "AT91SAM7SE512.h"
#include "FreeRTOS.h"
#include "debug/trace.h"

/// Default spurious interrupt handler. Infinite loop.
//------------------------------------------------------------------------------
void SpuriousIsr_Handler( void )
{
    TRACE_ERROR("defaultSpuriousHandler\r\n");
    //panic("SpuriousIsr_Handler");
    //while (1);
    AT91C_BASE_AIC->AIC_EOICR = 0;
}

void defaultSpuriousHandler( void )
{
    portSAVE_CONTEXT(); // Save the context of the interrupted task.
    /* Call the handler to do the work.  This must be a separate
       function to ensure the stack frame is set up correctly. */
    SpuriousIsr_Handler();
    portRESTORE_CONTEXT(); // Restore the context of whichever task will execute next.
}

//------------------------------------------------------------------------------
/// Default handler for fast interrupt requests. Infinite loop.
//------------------------------------------------------------------------------
void defaultFiqHandler( void )
{
    TRACE_ERROR("defaultFiqHandler\r\n");
    panic("defaultFiqHandler");
    //while (1);
}

//------------------------------------------------------------------------------
/// Default handler for standard interrupt requests. Infinite loop.
//------------------------------------------------------------------------------
void defaultIrqHandler( void )
{
    TRACE_ERROR("defaultIrqHandler\r\n");
    panic("defaultIrqHandler");
    //while (1);
}
