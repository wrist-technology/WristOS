/**
 * \file interrupt_utils.c
 * Interrupt utilities
 * 
 * Provides the interface routines for setting up and
 * controlling the various interrupt modes.
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
 * This module provides the interface routines for setting up and
 * controlling the various interrupt modes present on the ARM processor.
 * Copyright 2004, R O SoftWare
 * No guarantees, warrantees, or promises, implied or otherwise.
 * May be used for hobby or commercial purposes provided copyright
 * notice remains intact.
 *
 */
#include "interrupt_utils.h"

/**
 * \def IRQ_MASK
 * Normal interrupt mask
 * 
 */ 
#define IRQ_MASK 0x00000080
/**
 * \def FIQ_MASK
 * Fast interrupt mask
 * 
 */ 
#define FIQ_MASK 0x00000040

/**
 * \def INT_MASK
 * All interrupt mask
 * 
 */ 
#define INT_MASK (IRQ_MASK | FIQ_MASK)

/**

 * Get CPSR
 * 
 * Get content of Current Program Status Register
 * \return CPSR content
 * 
 */
static inline unsigned __get_cpsr(void)
{
  unsigned long retval;
  asm volatile (" mrs  %0, cpsr" : "=r" (retval) : /* no inputs */  );
  return retval;
}

/**

 * Set CPSR
 * 
 * Set Current Program Status Register
 * \param val CPSR content
 * 
 */
static inline void __set_cpsr(unsigned val)
{
  asm volatile (" msr  cpsr, %0" : /* no outputs */ : "r" (val)  );
}

/**

 * Disable interrupts
 * 
 * \return CPSR
 * 
 */
unsigned disableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr(_cpsr | IRQ_MASK);
  return _cpsr;
}

/**

 * Restore interrupts
 * 
 * \param oldCPSR Old interrupt mask
 * \return CPSR
 * 
 */ 
unsigned restoreIRQ(unsigned oldCPSR)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr((_cpsr & ~IRQ_MASK) | (oldCPSR & IRQ_MASK));
  return _cpsr;
}

/**

 * Enable interrupts
 * 
 * \return CPSR
 * 
 */
unsigned enableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr(_cpsr & ~IRQ_MASK);
  return _cpsr;
}

/**

 * Disable fast interrupts
 * 
 * \return CPSR
 * 
 */
unsigned disableFIQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr(_cpsr | FIQ_MASK);
  return _cpsr;
}

/**

 * Restore fast interrupts.
 * 
 * \param oldCPSR Old fast interrupt mask.
 * \return CPSR
 * 
 */
unsigned restoreFIQ(unsigned oldCPSR)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr((_cpsr & ~FIQ_MASK) | (oldCPSR & FIQ_MASK));
  return _cpsr;
}

/**

 * Enable fast interrupts
 * 
 * \return CPSR
 * 
 */
unsigned enableFIQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr(_cpsr & ~FIQ_MASK);
  return _cpsr;
}
