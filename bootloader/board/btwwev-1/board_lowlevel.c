/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

//------------------------------------------------------------------------------
/// \unit
///
/// !Purpose
///
/// Provides the low-level initialization function that gets called on chip
/// startup.
///
/// !Usage
///
/// LowLevelInit() is called in #board_cstartup.S#.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "hardware_conf.h"
//#include "board_memories.h"
#include <peripherals/pmc/pmc.h>

//------------------------------------------------------------------------------
//         Internal definitions
//------------------------------------------------------------------------------
// Startup time of main oscillator (in number of slow clock ticks).
#define BOARD_OSCOUNT           (AT91C_CKGR_OSCOUNT & (0x40 << 8))

// USB PLL divisor value to obtain a 48MHz clock. DIV 1=/2
#define BOARD_USBDIV            AT91C_CKGR_USBDIV_1  

// PLL frequency range.
#define BOARD_CKGR_PLL          AT91C_CKGR_OUT_0

// PLL startup time (in number of slow clock ticks).
#define BOARD_PLLCOUNT          (16 << 8)

// PLL MUL value.
#define BOARD_MUL               (AT91C_CKGR_MUL & (52 << 16))

// PLL DIV value.
#define BOARD_DIV               (AT91C_CKGR_DIV & 10)

// Master clock prescaler value.
#define BOARD_PRESCALER         AT91C_PMC_PRES_CLK_2

//------------------------------------------------------------------------------
//         Internal functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// Default spurious interrupt handler. Infinite loop.
//------------------------------------------------------------------------------
void defaultSpuriousHandler( void )
{
    //while (1);
}

//------------------------------------------------------------------------------
/// Default handler for fast interrupt requests. Infinite loop.
//------------------------------------------------------------------------------
void defaultFiqHandler( void )
{
    //while (1);
}

//------------------------------------------------------------------------------
/// Default handler for standard interrupt requests. Infinite loop.
//------------------------------------------------------------------------------
void defaultIrqHandler( void )
{
    //while (1);
}

/*
    Function: BOARD_RemapRam
        Changes the mapping of the chip so that the remap area mirrors the
        internal RAM.
*/
void BOARD_RemapRam( void )
{
    //if (BOARD_GetRemap() != BOARD_RAM) {

        AT91C_BASE_MC->MC_RCR = AT91C_MC_RCB;
    //}
}



//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Performs the low-level initialization of the chip. This includes EFC, master
/// clock, AIC & watchdog configuration, as well as memory remapping.
//------------------------------------------------------------------------------
void LowLevelInit( void )
{
    unsigned char i;

    //conf flash controller:
    AT91C_BASE_MC->MC0_FMR = ((AT91C_MC_FMCN)&(50 <<16)) | AT91C_MC_FWS_1FWS;

    //  Watchdog Disable
    //
    // result: 0xFFFFFD44 = 0x00008000  (AT91C_BASE_WDTC->WDTC_WDMR = Watchdog Mode Register)
  	AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;
	
  

//#if !defined(sdram)
    /* Initialize main oscillator
     ****************************/
    AT91C_BASE_PMC->PMC_MOR = BOARD_OSCOUNT | AT91C_CKGR_MOSCEN;
    while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MOSCS));

    /* Initialize PLL at 96MHz (96.109) and USB clock to 48MHz */
    AT91C_BASE_PMC->PMC_PLLR = BOARD_USBDIV | BOARD_CKGR_PLL | BOARD_PLLCOUNT
                               | BOARD_MUL | BOARD_DIV;
    while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_LOCK));

    /* Wait for the master clock if it was already initialized */
    while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY));

    /* Switch to fast clock
     **********************/
    /* Switch to slow clock + prescaler */
    AT91C_BASE_PMC->PMC_MCKR = BOARD_PRESCALER;
    while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY));

    /* Switch to fast clock + prescaler */
    AT91C_BASE_PMC->PMC_MCKR |= AT91C_PMC_CSS_PLL_CLK;
    while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY));
//#endif //#if !defined(sdram)
    
    /* Initialize AIC
     ****************/
    AT91C_BASE_AIC->AIC_IDCR = 0xFFFFFFFF;
    AT91C_BASE_AIC->AIC_SVR[0] = (unsigned int) defaultFiqHandler;
    for (i = 1; i < 31; i++) {

        AT91C_BASE_AIC->AIC_SVR[i] = (unsigned int) defaultIrqHandler;
    }
    AT91C_BASE_AIC->AIC_SPU = (unsigned int) defaultSpuriousHandler;

    // Unstack nested interrupts
    for (i = 0; i < 8 ; i++) {

        AT91C_BASE_AIC->AIC_EOICR = 0;
    }

    // Enable Debug mode
    //AT91C_BASE_AIC->AIC_DCR = AT91C_AIC_DCR_PROT;

    /* Watchdog initialization
     *************************/
    //AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;

    /* Remap
     *******/
    BOARD_RemapRam();

    // Disable RTT and PIT interrupts (potential problem when program A
    // configures RTT, then program B wants to use PIT only, interrupts
    // from the RTT will still occur since they both use AT91C_ID_SYS)
    AT91C_BASE_RTTC->RTTC_RTMR &= ~(AT91C_RTTC_ALMIEN | AT91C_RTTC_RTTINCIEN);
    AT91C_BASE_PITC->PITC_PIMR &= ~AT91C_PITC_PITIEN;
}


void pioInit(void)
{
   volatile AT91PS_PIO	pPIOA = AT91C_BASE_PIOA;	
   volatile AT91PS_PIO	pPIOB = AT91C_BASE_PIOB;
   volatile AT91PS_PIO	pPIOC = AT91C_BASE_PIOC;
   volatile AT91PS_EBI  pEBI = AT91C_BASE_EBI;
   
   pEBI->EBI_CSA = 0x0;
   //assign controller functions:
   //Peripheral A/B:
   pPIOA->PIO_ASR = 0x80007818; 
   pPIOA->PIO_BSR = 0x04800000;
   pPIOA->PIO_OWDR = 0x84807818;
   pPIOA->PIO_PDR =  0x84807818; 
   pPIOA->PIO_MDER = 0x00000018; //SCL and SDA open drain only 
   
   pPIOA->PIO_IFDR= 0xFFFFFFFF; //disable filters
   pPIOA->PIO_IDR = 0xFFFFFFFF; //disable ints
   pPIOA->PIO_PPUDR = 0xFFFFFFFF; //disable pullups
   
   //set #ZZ of PSRAM:
   pPIOB->PIO_PER = NSRZZ_MASK;							// PIO Enable Register - allow PIO to control pin PP3
	 pPIOB->PIO_OER = NSRZZ_MASK;							// PIO Output Enable Register - sets pin P3 to outputs
	 pPIOB->PIO_SODR = NSRZZ_MASK; //set to log1
   
   
	 
	 //global battery ON - temporary - should be replaced with GAS GAUGE Gpio:
	 //pPIOA->PIO_PER = GBON_MASK;							
	 //pPIOA->PIO_OER = GBON_MASK;							
	 //pPIOA->PIO_SODR = GBON_MASK; //set to log1 (power battery on)
   
   //microSD on:
	 //pPIOA->PIO_PER = SDON_MASK;							
	 //pPIOA->PIO_OER = SDON_MASK;							
	 //pPIOA->PIO_CODR = SDON_MASK; //set to log0 (power off)
	 
	 
	 //pPIOA->PIO_PER = LED2_MASK;							
	 //pPIOA->PIO_OER = LED2_MASK;							
	 //pPIOA->PIO_CODR = LED2_MASK; //set to log0 (power off)
	 
	 //BC boot config
	 pPIOA->PIO_ODR = 0x00000060;
	 pPIOA->PIO_PER = 0x00000060;
	 
	 
   
   
	 
	 
   //Peripheral B:
   pPIOB->PIO_ASR = 0x0000; 
   pPIOB->PIO_BSR = 0x0003FFFF;
   pPIOB->PIO_OWDR = 0x0003FFFF;
   pPIOB->PIO_PDR = 0x0003FFFF;
   
   pPIOB->PIO_IFDR= 0xFFFFFFFF; //disable filters
   pPIOB->PIO_IDR = 0xFFFFFFFF; //disable ints
   pPIOB->PIO_PPUDR = 0xFFFFFFFF; //disable pullups
   
   
   //
   
   pPIOB->PIO_PER = BUT0_MASK | BUT1_MASK | BUT2_MASK | BUT3_MASK | BUT4_MASK;	
   pPIOB->PIO_ODR = BUT0_MASK | BUT1_MASK | BUT2_MASK | BUT3_MASK | BUT4_MASK;   
   pPIOB->PIO_IFER =  BUT0_MASK | BUT1_MASK | BUT2_MASK | BUT3_MASK | BUT4_MASK;
   pPIOB->PIO_PPUER = BUT0_MASK | BUT1_MASK | BUT2_MASK | BUT3_MASK | BUT4_MASK;
   /*   
   pPIOB->PIO_PER = BCBOOT0_MASK;							
	 pPIOB->PIO_OER = BCBOOT0_MASK;							
	 pPIOB->PIO_CODR = BCBOOT0_MASK; //set to log0
   
   pPIOB->PIO_PER = BCBOOT1_MASK;							
	 pPIOB->PIO_OER = BCBOOT1_MASK;							
	 pPIOB->PIO_CODR = BCBOOT1_MASK; //set to log0
	 
	 pPIOB->PIO_PER = BCBOOT2_MASK;							
	 pPIOB->PIO_OER = BCBOOT2_MASK;							
	 pPIOB->PIO_CODR = BCBOOT2_MASK; //set to log0
	 
	 pPIOB->PIO_PER = BCNRES_MASK;							
	 pPIOB->PIO_OER = BCNRES_MASK;							
	 pPIOB->PIO_CODR = BCNRES_MASK; //set to log0
   */
   
   
   //Peripheral C:
   pPIOC->PIO_ASR = 0x001FFFFF; 
   pPIOC->PIO_BSR = 0x00E00000;
   pPIOC->PIO_OWDR =0x00FFFFFF;
   pPIOC->PIO_PDR = 0x00FFFFFF;
   
   pPIOC->PIO_IFDR= 0xFFFFFFFF; //disable filters
   pPIOC->PIO_IDR = 0xFFFFFFFF; //disable ints
   pPIOC->PIO_PPUDR = 0xFFFFFFFF; //disable pullups
   
   
   
}


