/*****************************************************************************
  Exception handlers and startup code for ATMEL AT91SAM7.

  Copyright (c) 2004 Rowley Associates Limited.

  This file may be distributed under the terms of the License Agreement
  provided with this software. 
 
  THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE
  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *****************************************************************************/

#define ARM_MODE_ABT     0x17
#define ARM_MODE_FIQ     0x11
#define ARM_MODE_IRQ     0x12
#define ARM_MODE_SVC     0x13

#define I_BIT            0x80
#define F_BIT            0x40

#define REG_BASE 0xFFFFF000
#define CKGR_MOR_OFFSET 0xC20
#define CKGR_PLLR_OFFSET 0xC2C
#define PMC_MCKR_OFFSET 0xC30
#define PMC_SR_OFFSET 0xC68
#define WDT_MR_OFFSET 0xD44
#define MC_RCR_OFFSET 0xF00
#define MC_FMR_OFFSET 0xF60

#define CKGR_MOR_MOSCEN (1 << 0)
#define CKGR_MOR_OSCBYPASS (1 << 1)
#define CKGR_MOR_OSCOUNT_BIT_OFFSET (8)

#define CKGR_PLLR_DIV_BIT_OFFSET (0)
#define CKGR_PLLR_PLLCOUNT_BIT_OFFSET (8)
#define CKGR_PLLR_OUT_BIT_OFFSET (14)
#define CKGR_PLLR_MUL_BIT_OFFSET (16)
#define CKGR_PLLR_USBDIV_BIT_OFFSET (28)

#define PMC_MCKR_CSS_MAIN_CLOCK (0x1)
#define PMC_MCKR_CSS_PLL_CLOCK (0x3)
#define PMC_MCKR_PRES_CLK (0)
#define PMC_MCKR_PRES_CLK_2 (1 << 2)
#define PMC_MCKR_PRES_CLK_4 (2 << 2)
#define PMC_MCKR_PRES_CLK_8 (3 << 2)
#define PMC_MCKR_PRES_CLK_16 (4 << 2)
#define PMC_MCKR_PRES_CLK_32 (5 << 2)
#define PMC_MCKR_PRES_CLK_64 (6 << 2)

#define PMC_SR_MOSCS (1 << 0)
#define PMC_SR_LOCK (1 << 2)
#define PMC_SR_MCKRDY (1 << 3)
#define PMC_SR_PCKRDY0 (1 << 8)
#define PMC_SR_PCKRDY1 (1 << 9)
#define PMC_SR_PCKRDY2 (1 << 10)

#define MC_RCR_RCB (1 << 0)

#define MC_FMR_FWS_0FWS (0)
#define MC_FMR_FWS_1FWS (1 << 8)
#define MC_FMR_FWS_2FWS (2 << 8)
#define MC_FMR_FWS_3FWS (3 << 8)
#define MC_FMR_FMCN_BIT_OFFSET 16

#define WDT_MR_WDDIS (1 << 15)

  .section .imageboot
  .code 32
  .align 0
_boot:
  //ldr     pc, =_start        /* Boot image at 0x1000000... */
  ldr pc, [pc, #boot_handler_address - . - 8]  /* boot */

boot_handler_address:
  .word boot_handler

  .section .vectors, "ax"
  .code 32
  .align 0
  
/*****************************************************************************
  Exception Vectors
 *****************************************************************************/
_vectors:
  ldr pc, [pc, #undef_handler_address - . - 8]  /* reset */
  ldr pc, [pc, #undef_handler_address - . - 8]  /* undefined instruction */
  ldr pc, [pc, #swi_handler_address - . - 8]    /* swi handler */
  ldr pc, [pc, #pabort_handler_address - . - 8] /* abort prefetch */
  ldr pc, [pc, #dabort_handler_address - . - 8] /* abort data */
  nop
  //ldr pc, [PC, #-0xF20]    /* irq */
  ldr pc, [pc, #irq_handler_address - . - 8] /* irq handler */
  /* MAKINGTHINGS: Add */
  ldr pc, [pc, #-0xF20]   /* fiq */

  /* MAKINGTHINGS: remove */
  // ldr pc, [pc, #fiq_handler_address - . - 8]    /* fiq */

undef_handler_address:
  .word undef_handler
swi_handler_address:
  .word swi_handler
pabort_handler_address:
  .word pabort_handler
dabort_handler_address:
  .word dabort_handler
irq_handler_address:
  .word irq_handler
fiq_handler_address:
  .word fiq_handler

  .section .init, "ax"
  .code 32
  .align 0

  .global __boot_handler

/******************************************************************************
  boot handler
 ******************************************************************************/
boot_handler:
__boot_handler:

  /* Copy exception vectors into Internal SRAM */
  /* 
  *  MakingThings
  *  mov r8, #0x00200000
  *  ldr r9, =_vectors
  */
  ldr r8, =__vectors_ram_start__
  ldr r9, =__vectors_load_start__
  /* end MakingThings */
/*
  ldmia r9!, {r0-r7}
  stmia r8!, {r0-r7}
  ldmia r9!, {r0-r6}
  stmia r8!, {r0-r6}
*/

  ldr     r0, =__vectors_load_start__
  ldr     r1, =__vectors_ram_start__
  ldr     r2, =__vectors_ram_end__
  1:
  cmp     r1, r2
  ldrcc   r3, [r0], #4
  strcc   r3, [r1], #4
  bcc     1b


  /* Jump to the default C runtime startup code. */
  b _start

/******************************************************************************
  Default exception handlers
  (These are declared weak symbols so they can be redefined in user code)
 ******************************************************************************/
undef_handler:
//  b undef_handler
  b gen_handler
  
swi_handler:
//  b swi_handler
  b gen_handler
  
pabort_handler:
//  b pabort_handler
  b gen_handler
  
dabort_handler:
//  b dabort_handler
  b gen_handler
  
irq_handler:
//  b irq_handler
  b gen_handler
  
fiq_handler:
//  b fiq_handler
  b gen_handler

  .weak undef_handler, swi_handler, pabort_handler, dabort_handler, irq_handler, fiq_handler

  .extern kill

gen_handler:
//  b reset_handler
  b kill

