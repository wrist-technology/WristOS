#ifndef IRQ_PARAM_H
#define IRQ_PARAM_H

#include "hardware_conf.h"

#define IRQ_SPI_PRI         4
#define IRQ_I2C_PRI         4
#define IRQ_ADC_PRI         4
#define IRQ_SERIAL_PRI      4
#define IRQ_TIMER_PRI       4
#define IRQ_SYS_PRI         AT91C_AIC_PRIOR_HIGHEST
#define IRQ_IO_PRI          3
#define IRQ_USB_PRI         3
#define IRQ_AUDIO_PRI       0

#endif // IRQ_PARAM_H
