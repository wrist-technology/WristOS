/* ----------------------------------------------------------------------------
 *         BTWWEV-1 Image linker script
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Wrist Technology Ltd
 * 
 * stack&irqhandlers: SRAM on-chip
 * program memory: eXtrnal SRAM
 * data memory: eXtrnal SRAM    
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

/*------------------------------------------------------------------------------
 *      Linker script for running in internal Flash on the AT91SAM7SE512.
 *----------------------------------------------------------------------------*/

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(entry)

MEMORY
{
    sram (W!RX) : ORIGIN = 0x0000, LENGTH = 0x8000
    xdatasram (W!RX) : ORIGIN = 0x10080000, LENGTH = 0x8000
    xcodesram (RX) : ORIGIN = 0x10000000, LENGTH = 0x70000
}

SECTIONS
{      
        
    .fixed :
    {
        . = ALIGN(4);
        _sfixed = .;
        *(.imageboot)                                    
        *(.text*)
        *(.rodata*)                
        . = ALIGN(4);
        _efixed = .;
        _efixedr = . + 0x200;
    } >xcodesram

    .relocate : AT (_efixedr)
    {
        . = ALIGN(4);
        _srelocate = .;
        *(.ramfunc)
        *(.data)
        . = ALIGN(4);
        _erelocate = .;        
    } >xdatasram

    .irqhandlers : AT (_efixed)
    {
        . = ALIGN(4);
        _sirqhandlers = .;
        *(.vectors)        
        . = ALIGN(4);
        _eirqhandlers = .;
    } >sram
    
    
    
    _sstack = 0x00008000;
}
end = .;

