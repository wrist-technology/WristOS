/*****************************************************************************
  Exception handlers and startup code for ATMEL AT91SAM7.

  Copyright (c) 2004 Rowley Associates Limited.

  This file may be distributed under the terms of the License Agreement
  provided with this software. 
 
  THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE
  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *****************************************************************************/

// MV
#define AIC_IVR		0x00000100
#define AIC_EOICR	0x00000130
#define AT91C_BASE_AIC	0xFFFFF000

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
  ldr pc, [PC, #-0xF20]    /* irq */
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
 //   b gen_handler
    stmfd sp!, {r0-r12, lr}
    ldr     sp, =(__abort_mem+5*4)  @ Set sp_abt to data array with offset (restore later)
    stmia   sp, {r0-r12}            @ Save first dataset in r0-r12 to array
    sub     r0, lr, #4              @ Calculate PC value of undef instruction
    mov     r1, #0                  @ Abort type
    b       .abtstore               @ Save info, reset system
  
swi_handler:
//  b swi_handler
  b gen_handler

  
pabort_handler:
//  b pabort_handler
//    b gen_handler
    stmfd sp!, {r0-r12, lr}
    ldr     sp, =(__abort_mem+5*4)  @ Set sp_abt to data array with offset (restore later)
    stmia   sp, {r0-r12}            @ Save first dataset in r0-r12 to array
    sub     r0, lr, #4              @ Calculate PC value of undef instruction
    mov     r1, #1                  @ Abort type
    b       .abtstore               @ Save info, reset system

dabort_handler:
//  b dabort_handler
//    b gen_handler
/*
stmfd sp!, {r0-r12, lr}
    ldr     sp, =(__abort_mem+5*4)  @ Set sp_abt to data array with offset (restore later)
    stmia   sp, {r0-r12}            @ Save first dataset in r0-r12 to array
*/
    //sub     r0, lr, #4              @ Calculate PC value of undef instruction
sub     r0, lr, #8              @ Calculate PC value of undef instruction
    mov     r1, #2                  @ Abort type
    b       .abtstore               @ Save info, reset system
  
_irq_handler:
//  b irq_handler
  b gen_handler
  
fiq_handler:
//  b fiq_handler
  b gen_handler

@
@  Store the abort type.  Then see if the sigil value is set, and if not,
@  reset the abort counter to 0.
@
.abtstore:
        ldr     r2, =__abort_typ        @ Abort type
        str     r1, [r2]                @ Store it
ldr     r2, =__abort_mem        @ Abort mem
str     r0, [r2]                @ Store it

//ldr     sp, =__stack_abt_end__
bl      abort_dump

        ldr     r2, =__abort_sig        @ Get the sigil address
        ldr     r4, =ABORT_SIGIL        @ Load sigil value
        ldr     r3, [r2]                @ Get sigil contents
        cmp     r3, r4                  @ Sigil set?

        strne   r4, [r2]                @ No, store sigil value
        ldrne   r2, =__abort_cnt        @ No, load address of abort counter
        movne   r4, #0                  @ No, Zero for store
        strne   r4, [r2]                @ No, Clear counter

@
@  Now build up structure of registers and stack (r0 = abort address, r1 = 
@  abort type).  This code is based heavily on the work of Roger Lynx, from 
@  http://www.embedded.com/shared/printableArticle.jhtml?articleID=192202641
@
        mrs     r5, cpsr                @ Save current mode to R5 for mode switching
        mrs     r6, spsr                @ spsr_abt = CPSR of dabt originating mode, save to r6 for mode switching
        mov     r2, r6                  @ Building second dataset: r2 = CPSR of exception
        tst     r6, #0x0f               @ Test mode of the raised exception
        orreq   r6, r6, #0x0f           @ If 0, elevate from user mode to system mode
        msr     cpsr_c, r6              @ Switch out from mode 0x17 (abort) to ...
        mov     r3, lr                  @ ... dabt generating mode and state
        mov     r4, sp                  @ ... Get lr (=r3) and sp (=r4)
        msr     cpsr_c, r5              @ Switch back to mode 0x17 (abort)
        cmp     r1, #1                  @ Test for prefetch abort
        moveq   r1, #0                  @ Can't fetch instruction at the abort address
        ldrne   r1, [r0]                @ r1 = [pc] (dabt)
        ldr     sp, =__abort_mem        @ Reset sp to arrays starting address
        stmia   sp, {r0-r4}             @ Save second dataset from r0 to r4

        ldr     r1, =__abort_stk        @ Space where we'll store abort stack
        mov     r2,#8                   @ Copy 8 stack entries
.abtcopy:
        ldr     r0, [r4], #4            @ Get byte from source, r4 += 4
        str     r0, [r1], #4            @ Store byte to destination, r1 += 4
        subs    r2, r2, #1              @ Decrement loop counter
        bgt     .abtcopy                @ >= 0, go again

        //b       .sysreset               @ And reset
        ldr     sp, =__stack_abt_end__
        bl      abort_dump

@
@  Force a system reset with ye olde watch dogge
@
        .set    SCB_RSIR_MASK, 0x0000000f
        .set    SCB_RSIR,      0xe01fc180
        .set    WD_MOD,        0xe0000000
        .set    WD_TC,         0xe0000004
        .set    WD_FEED,       0xe0000008
        .set    WD_MOD_WDEN,   0x00000001
        .set    WD_MOD_RESET,  0x00000002
        .set    WD_MOD_TOF,    0x00000004
        .set    WD_MOD_INT,    0x00000008
        .set    WD_MOD_MASK,   0x0000000f
        .set    WD_FEED_FEED1, 0x000000aa
        .set    WD_FEED_FEED2, 0x00000055
        .set    ABORT_SIGIL,   0xdeadc0de

.sysreset:
        ldr     r1, =__abort_cnt        @ Get the abort counter address
        ldr     r0, [r1]                @ Load it
        add     r0, r0, #1              @ Add 1
        str     r0, [r1]                @ Store it back

@
@  Now enable the watch dog, and go into a loop waiting for a timeout
@
        ldr     r0, =SCB_RSIR_MASK
        ldr     r1, =SCB_RSIR
        str     r0, [r1]
        ldr     r0, =WD_MOD_WDEN | WD_MOD_RESET
        ldr     r1, =WD_MOD
        str     r0, [r1]
        ldr     r0, =120000
        ldr     r1, =WD_TC
        str     r0, [r1]
        ldr     r0, =WD_FEED_FEED1
        ldr     r1, =WD_FEED
        str     r0, [r1]
        ldr     r0, =WD_FEED_FEED2
        ldr     r1, =WD_FEED
        str     r0, [r1]
        b       .
  .weak undef_handler, swi_handler, pabort_handler, dabort_handler, irq_handler, fiq_handler

  .extern kill

gen_handler:
//  b reset_handler
//  b kill
    b abort_dump
  b gen_handler

    .global __abort_dat
    .global __abort_mem
    .global __abort_typ
    //MV .section .protected
    .section .data
    .align  0

__abort_dat:  .word 0                   @ Dummy, not used
__abort_sig:  .word 0                   @ Sigil to indicate data validity
__abort_cnt:  .word 0                   @ Number of times we've aborted
__abort_typ:  .word 0                   @ Type of abort (0=undef,1=pabort,2=dabort)
__abort_mem:  .space (18 * 4), 0        @ Registers from abort state
__abort_stk:  .space (8 * 4), 0         @ 8 stack entries from abort state


