#include "gasgauge.h"
#include "hardware_conf.h"
#include "utils/macros.h"
#include "i2c.h"
#include "debug/trace.h"

int gasgauge_init () {
    // TBD
    /*
       i2cMasterConf(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_OPmode<<10)|(I2CGG_REG_OPmode), I2CMASTER_WRITE);
       i2cWriteByte(I2CGG_POR_BITMASK|I2CGG_SHELF_BITMASK);
       i2cMasterConf(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_OPmode<<10)|(I2CGG_REG_OPmode), I2CMASTER_WRITE);
       i2cWriteByte(I2CGG_SHELF_BITMASK);
       */
    AT91C_BASE_PIOA->PIO_PER = PIN_CHARGING;
    AT91C_BASE_PIOA->PIO_ODR = PIN_CHARGING;
    AT91C_BASE_PIOA->PIO_PPUER = PIN_CHARGING; // pull-up enabled

    AT91C_BASE_PIOA->PIO_PER = PIN_CHARGEDONE;
    AT91C_BASE_PIOA->PIO_ODR = PIN_CHARGEDONE;
    AT91C_BASE_PIOA->PIO_PPUER = PIN_CHARGEDONE; // pull-up enabled

    AT91C_BASE_PIOA->PIO_PER = CHARGEEN_PIN;
    AT91C_BASE_PIOA->PIO_OER = CHARGEEN_PIN;
    AT91C_BASE_PIOA->PIO_SODR = CHARGEEN_PIN; //set to log1

    // USBPEN2_PIN high => 500mA charging
   AT91C_BASE_PIOA->PIO_PER = USBPEN2_PIN;                          // PIO Enable Register - allow PIO to control pin PP3
   AT91C_BASE_PIOA->PIO_OER = USBPEN2_PIN;                          // PIO Output Enable Register - sets pin P3 to outputs
   AT91C_BASE_PIOA->PIO_SODR = USBPEN2_PIN;
   //AT91C_BASE_PIOA->PIO_CODR = USBPEN2_PIN;

#if 0 
    //measure pack voltage:
    i2cMasterWrite(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_OPmode<<10)|(I2CGG_REG_OPmode), 1);
    i2cMasterWrite(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_OPmode<<10)|(I2CGG_REG_OPmode), 2);
    i2cMasterWrite(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_OPmode<<10)|(I2CGG_REG_OPmode), 0);

    i2cMasterWrite(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_ADconfig<<10)|(I2CGG_REG_ADconfig), 0x80);

    //i2cMasterWrite(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_ITctrl<<10)|(I2CGG_REG_ITctrl), I2CGG_DEFVAL_ITctrl);

    i2cMasterWrite(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_VPctrl<<10)|(I2CGG_REG_VPctrl), I2CGG_DEFVAL_VPctrl);

    //b = i2cMasterRead(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_VPctrl<<10)|(I2CGG_REG_VPctrl), I2CMASTER_READ);

    //TRACE_ALL("gas gauge ADC Vpack ON:VPctrl=%x\n\r",b); 
    //delayms(300);
#endif
}

int gasgauge_charge(bool on) {
    if (on) {
        AT91C_BASE_PIOA->PIO_CODR = CHARGEEN_PIN; //enable
        AT91C_BASE_PIOA->PIO_SODR = USBPEN2_PIN;
    } else {
        AT91C_BASE_PIOA->PIO_SODR = CHARGEEN_PIN; //disable
        AT91C_BASE_PIOA->PIO_CODR = USBPEN2_PIN;
    }
}

int gasgauge_get_stats (gasgauge_stats *stats) {
    uint16_t t;
    uint8_t b;

    TRACE_INFO("gasgauge_stats\r\n");
    i2c_open();
    //i2c_lock();

    if(i2cMasterRead(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_VPres<<10)|(I2CGG_REG_VPres), &b)) {
        //i2c_unlock();
        i2c_close();
        return -1;
    }
  
    t = (uint16_t)b;
    if(i2cMasterRead(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_VPres<<10)|(I2CGG_REG_VPres+1), &b)) {
        panic("gasgauge_get_stats1");
        goto exit_error;
    }
    t |= (((uint16_t)b)<<8);

    stats->voltage = ((t>>6)*199)/10;

    if(i2cMasterRead(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_Ires<<10)|(I2CGG_REG_Ires), &b)) {
        panic("gasgauge_get_stats2");
        goto exit_error;
    }
    t = (uint16_t)b;
    if(i2cMasterRead(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_Ires<<10)|(I2CGG_REG_Ires+1), &b)) {
        panic("gasgauge_get_stats3");
        goto exit_error;
    }
    //i2c_unlock();
    i2c_close();

    t |= (((uint16_t)b)<<8);

    if (t&0x8000)
        stats->current = -(((int32_t)(t&0x7fff)*157)/10000);//mAmps
    else
        stats->current = ((int32_t)(t&0x7fff)*157)/10000;//mAmps

    TRACE_INFO("gasgauge U: %d I: %d ", stats->voltage, stats->current);


    if (ISCLEARED(AT91C_BASE_PIOA->PIO_PDSR, PIN_CHARGING)) {
        TRACE_INFO("Charging.\n\r");
        stats->state = GASGAUGE_STATE_CHARGING;
    } else {
        if (ISCLEARED(AT91C_BASE_PIOA->PIO_PDSR, PIN_CHARGEDONE))
        {
            TRACE_INFO("Charged.\n\r");
            //chargedone=1;
            //AT91C_BASE_PIOA->PIO_SODR = CHARGEEN_PIN; //disable
            stats->state = GASGAUGE_STATE_CHARGED;
        } else {

            TRACE_INFO("No charge.\n\r");
            stats->state = GASGAUGE_STATE_NO_CHARGE;
        }
    }
    return 0;

exit_error:
    //i2c_unlock();
    i2c_close();
    panic("gasgauge_get_stats");
    return -1;
}

/*
void volt_meas(void)
{
  // divider 0,18032786885245901639344262295082
  // ref: 3.3V
  // k= 17,87109375
  //volatile AT91PS_PIO  pPIO = AT91C_BASE_PIOA; 
  //volatile AT91PS_TC pTC = AT91C_BASE_TC0;
  volatile AT91PS_ADC pADC = AT91C_BASE_ADC;
  uint32_t result;
  static uint32_t filt;
  static char buf[32];
  
  pADC->ADC_CR = 0x1;//rst
  pADC->ADC_MR =  0x0f1f3f10; //mck/30
  pADC->ADC_CHDR=0xffffffff;
  //pADC->ADC_CHER= AT91C_ADC_CH5;
  //pADC->ADC_CHER= AT91C_ADC_CH4;//usb sense
  pADC->ADC_CHER= AT91C_ADC_CH7;//current sense
  
  pADC->ADC_IDR = 0xffffffff;
  
  
    pADC->ADC_CR = 0x2;//start conversion
    
    while (!(pADC->ADC_SR&AT91C_ADC_DRDY)) asm volatile ("nop");
    
    result = (pADC->ADC_LCDR);
    result = (result*8);
    filt = (7*result + filt)/8;
    
    
    //srprintf(buf,"Volt:%d ",filt);
    fontSetCharPos(0,8);
    TRACE_SCR("%d  ",filt);
        
  
}  
*/
