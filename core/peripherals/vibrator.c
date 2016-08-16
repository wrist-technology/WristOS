#include "io.h"

void vibrator_set(bool on) {
#if 0
    //uint32_t mask = (1 << 15); 
    uint32_t mask = AT91C_PIO_PA1;
    AT91C_BASE_PIOA->PIO_PER = mask;
    AT91C_BASE_PIOA->PIO_OER = mask;

    if (on) {
        AT91C_BASE_PIOA->PIO_SODR = mask;
    } else {
        AT91C_BASE_PIOA->PIO_CODR = mask;
    }
#else
    Io vibrator;
    Io_init(&vibrator, IO_PA01, IO_GPIO, OUTPUT);
    Io_setValue(&vibrator, on);
#endif
}
