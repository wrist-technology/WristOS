#include "board/hardware_conf.h"
#include "dac7311.h"
#include "ssc.h"
#include "rtos.h"
#include "event.h"
#include "audio.h"
#include "debug/trace.h"

extern uint32_t audio_start;

extern audio_sample *audio_current_sample;
extern uint32_t audio_current_sample_loop;
extern uint32_t audio_current_sample_remaining;
extern uint32_t audio_current_sample_transmitted;
extern void* (*audio_current_sample_stop_callback)(void *arg);
extern void* audio_current_sample_stop_callback_arg;

void audioIsr_Wrapper( void ) __attribute__ ((naked));

//------------------------------------------------------------------------------
/// Interrupt handler for the SSC. Loads the PDC with the audio data to stream.
//------------------------------------------------------------------------------
static void audio_isr(void)
{
    unsigned int status = BOARD_DAC7311_SSC->SSC_SR;
    unsigned int size;

    TRACE_INFO("audio_isr %d %x\r\n", Timer_tick_count_nonblock(), status);

    // Last buffer sent
    if ((status & AT91C_SSC_TXBUFE) != 0) {

        audio_stop_isr();
    // One buffer sent & more buffers to send
    } else {
        if (audio_current_sample_remaining == 0 && audio_current_sample_loop) {
            audio_current_sample_remaining = audio_current_sample->length;
            audio_current_sample_transmitted = 0;
        }
        if (audio_current_sample_remaining > 0) {
            uint32_t size = min(audio_current_sample_remaining, 0xffff);

            SSC_WriteBuffer(BOARD_DAC7311_SSC, (void *) &AUDIO_SAMPLE_DATA(audio_current_sample)[audio_current_sample_transmitted], size); // 2nd DMA buffer
            audio_current_sample_remaining -= size;
            audio_current_sample_transmitted += size;
        } else {
            SSC_DisableInterrupts(BOARD_DAC7311_SSC, AT91C_SSC_ENDTX);
        }
    }
    AT91C_BASE_AIC->AIC_EOICR = 0; // Clear AIC to complete ISR processing
}

void audioIsr_Wrapper( void )
{
    /* Save the context of the interrupted task. */
    portSAVE_CONTEXT();

#ifdef CFG_DEEP_SLEEP
    check_power_mode();
#endif

    /* Call the handler to do the work.  This must be a separate
       function to ensure the stack frame is set up correctly. */
    audio_isr();

    /* Restore the context of whichever task will execute next. */
    portRESTORE_CONTEXT();
}
