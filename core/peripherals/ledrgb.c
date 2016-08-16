#include "ledrgb.h"
#include "i2c.h"
#include "rtos.h"
#include "debug/trace.h"

void ledrgb_set(uint8_t rgb_mask, uint8_t r, uint8_t g, uint8_t b)
{
    //TRACE_INFO("ledrgb_set %x %x %x\r\n", r, g, b);
    i2c_open();

    if (rgb_mask & 0x1)
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_RPWM, r);
    if (rgb_mask & 0x2)
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_GPWM, g);
    if (rgb_mask & 0x4)
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_BPWM, b);

    i2c_close();
}

// MV

// TODO: add mutex
static unsigned int ledrgb_open_count = 0;

int ledrgb_open () {
    if(!ledrgb_open_count++) {
        i2c_open();

        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_ENABLE, LEDRGB_CHIPEN);
        //delay(500);
        Task_sleep(1);
        //i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_CONFIG, LEDRGB_CPMODE_AUTO|LEDRGB_PWM_HF|LEDRGB_INT_CLK_EN|LEDRGB_R_TO_BATT);
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_CONFIG, LEDRGB_CPMODE_AUTO|LEDRGB_PWM_HF|LEDRGB_INT_CLK_EN|LEDRGB_R_TO_BATT|LEDRGB_PWRSAVE_EN);
        //delay(20);
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_OPMODE, LEDRGB_RMODE_DC|LEDRGB_GMODE_DC|LEDRGB_BMODE_DC);
        //config lights:      
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_RCURRENT, 0x20);
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_GCURRENT, 0x20);
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_BCURRENT, 0x20);

        i2c_close();
    }
}

int ledrgb_close () {
    if(ledrgb_open_count && --ledrgb_open_count == 0) {
        i2c_open();

        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_CONFIG, 0);
        i2cMasterWrite(LEDRGB_PHY_ADDR, 1, LEDRGB_REG_ENABLE, 0);

        i2c_close();
    }
}
