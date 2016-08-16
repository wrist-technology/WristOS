#include "amblightsensor.h"
#include <stdint.h>
#include <hardware_conf.h>

void ADCinit(void)
{
  volatile AT91PS_ADC   pADC = AT91C_BASE_ADC;
  pADC->ADC_CR = AT91C_ADC_SWRST;
  pADC->ADC_CR = AT91C_ADC_SWRST; //maybe unused
  pADC->ADC_CR = 0;
  pADC->ADC_CR = 0;
  pADC->ADC_MR = (64<<8)|(16<<16)|(8<<24);

  pADC->ADC_CHDR = 0xff;
  pADC->ADC_CHER = 0x80; //adc 7 input
}

uint16_t ambLightADC(void)
{
  volatile AT91PS_ADC   pADC = AT91C_BASE_ADC;
  pADC->ADC_CHER = 0x80; //adc 7 input
  //start
  pADC->ADC_CR = AT91C_ADC_START;

  while (!(pADC->ADC_SR&AT91C_ADC_EOC7)) ;

  return pADC->ADC_CDR7;
}
