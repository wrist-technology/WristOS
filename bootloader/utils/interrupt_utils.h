/**
 * \file interrupt_utils.h
 * Header: Interrupt utilities
 * 
 * Defines and Macros for Interrupt-Service-Routines
 * 
 * AT91SAM7S-128 USB Mass Storage Device with SD Card by Michael Wolf\n
 * Copyright (C) 2008 Michael Wolf\n\n
 * 
 * This program is free software: you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation, either version 3 of the License, or\n
 * any later version.\n\n
 * 
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 * GNU General Public License for more details.\n\n
 * 
 * You should have received a copy of the GNU General Public License\n
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 * 
 */


/*
 *  Defines and Macros for Interrupt-Service-Routines
 *  collected and partly created by
 *  Martin Thomas <mthomas@rhrk.uni-kl.de>
 *
 *  Copyright 2005 M. Thomas
 *  No guarantees, warrantees, or promises, implied or otherwise.
 *  May be used for hobby or commercial purposes provided copyright
 *  notice remains intact.
 */

#ifndef INTERRUPT_UTILS_H_
#define INTERRUPT_UTILS_H_

/*
 * The following defines are usefull for 
 * interrupt service routine declarations.
 * 
*/

/**
 * \def RAMFUNC
 * Attribute which defines a function to be located
 * in memory section .fastrun and called via "long calls".
 * See linker-skript and startup-code to see how the
 * .fastrun-section is handled.
 * The definition is not only useful for ISRs but since
 * ISRs should be executed fast the macro is defined in
 * this header.
 * 
*/
#define RAMFUNC __attribute__ ((long_call, section (".fastrun")))


/**
 * \def INTFUNC
 * standard attribute for arm-elf-gcc which marks
 * a function as ISR (for the VIC). Since gcc seems
 * to produce wrong code if this attribute is used in
 * thumb/thumb-interwork the attribute should only be
 * used for "pure ARM-mode" binaries.
 * 
*/
#define INTFUNC __attribute__ ((interrupt("IRQ"))) 


/**
 * \def NAKEDFUNC
 * gcc will not add any code to a function declared
 * "nacked". The user has to take care to save registers
 * and add the needed code for ISR functions. Some
 * macros for this tasks are provided below.
 * 
*/
#define NAKEDFUNC __attribute__((naked))


/**
 * \def ISR_STORE
 * This MACRO is used upon entry to an ISR with interrupt nesting.  
 * Should be used together with ISR_ENABLE_NEST(). The MACRO
 * performs the following steps:
 *
 */
#define ISR_STORE() asm volatile( \
 "STMDB SP!,{R0-R12,LR}\n" )
 
/**
 * \def ISR_RESTORE
 * This MACRO is used upon exit from an ISR with interrupt nesting.  
 * Should be used together with ISR_DISABLE_NEST(). The MACRO
 * performs the following steps:
 *
 */
#define ISR_RESTORE()  asm volatile( \
 "LDMIA SP!,{R0-R12,LR}\n" \
 "SUBS  R15,R14,#0x0004\n" ) 

/**
 * \def ISR_ENABLE_NEST
 * This MACRO is used upon entry from an ISR with interrupt nesting.  
 * Should be used after ISR_STORE.
 * 
 */
#define ISR_ENABLE_NEST() asm volatile( \
 "MRS     LR, SPSR \n"  \
 "STMFD   SP!, {LR} \n" \
 "MSR     CPSR_c, #0x1F \n" \
 "STMFD   SP!, {LR} " )

/**
 * \def ISR_DISABLE_NEST
 * This MACRO is used upon entry from an ISR with interrupt nesting.  
 * Should be used before ISR_RESTORE.
 *
 */
#define ISR_DISABLE_NEST() asm volatile(  \
 "LDMFD   SP!, {LR} \n" \
 "MSR     CPSR_c, #0x92 \n" \
 "LDMFD   SP!, {LR} \n" \
 "MSR     SPSR_cxsf, LR \n" )
 
/*
 * The following marcos are from the file "armVIC.h" by:
 *
 * Copyright 2004, R O SoftWare
 * No guarantees, warrantees, or promises, implied or otherwise.
 * May be used for hobby or commercial purposes provided copyright
 * notice remains intact.
 * 
 */ 
 
/**
 * \def ISR_ENTRY
 * This MACRO is used upon entry to an ISR.  The current version of
 * the gcc compiler for ARM does not produce correct code for
 * interrupt routines to operate properly with THUMB code.  The MACRO
 * performs the following steps.
 * 
 */
#define ISR_ENTRY() asm volatile(" sub   lr, lr,#4\n" \
                                 " stmfd sp!,{r0-r12,lr}\n" \
                                 " mrs   r1, spsr\n" \
                                 " stmfd sp!,{r1}")

/**
 * \def ISR_EXIT
 * This MACRO is used to exit an ISR.  The current version of the gcc
 * compiler for ARM does not produce correct code for interrupt
 * routines to operate properly with THUMB code.  The MACRO performs
 * the following steps.
 * 
 */
#define ISR_EXIT()  asm volatile(" ldmfd sp!,{r1}\n" \
                                 " msr   spsr_c,r1\n" \
                                 " ldmfd sp!,{r0-r12,pc}^") 
  
unsigned disableIRQ(void);
unsigned enableIRQ(void);
unsigned restoreIRQ(unsigned oldCPSR);
unsigned disableFIQ(void);
unsigned enableFIQ(void);
unsigned restoreFIQ(unsigned oldCPSR);

#endif  /*INTERRUPT_UTILS_H_*/

