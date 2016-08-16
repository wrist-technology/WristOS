/**
 * \file trace.h
 * Header: Trace functions
 * 
 * Trace functions for debugging
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
#ifndef _TRACE_H
#define _TRACE_H

#if !defined(NO_TRACES)

#include <peripherals/serial.h>
#include <screen/font.h>
#include <utils/rprintf.h> 

/* Enable/Disable tracer levels here */

#define TR_INFO         
#define TR_WARNING      
#define TR_ERROR        
#define TR_SCR  //screen
//#define TR_FAT          
#define TR_AUDIO
//#define TR_BT          
//#define TR_LUA          
//#define TR_BMP          
//#define TR_SYS          
//#define TR_TMR          
//#define TR_RTC          
//#define TR_ADC
//#define TR_SER          
//#define TR_SD           
//#define TR_SPI
//#define TR_I2C
#define TR_ACCEL
//#define TR_USB          
//#define TR_BOT          
//#define TR_SBC          
//#define TR_LUN          

#endif // !defined(NOTRACES)

//------------------------------------------------------------------------------
//      Macro
//------------------------------------------------------------------------------

#if !defined(NOTRACES)
    #define TRACE_INIT()    void dbg_usart_init(void)         
#else
    #define TRACE_INIT(...) 
#endif // !defined(NOTRACES)

#if !defined(NOTRACES)
    #define TRACE_ALL(...)   rprintf(DBG,__VA_ARGS__)
#else
    #define TRACE_ALL(...)
#endif // !defined(NOTRACES)


#if defined(TR_DEBUG)
    #define TRACE(...)      rprintf(DBG,__VA_ARGS__)
#else
    #define TRACE(...)      
#endif // TR_DEBUG

#if defined(TR_SCR)
    #define TRACE_SCR(...)      rprintf(SCR,__VA_ARGS__)        
#else
    #define TRACE_SCR(...)      
#endif // TR_DEBUG

#if defined(TR_FAT)
    #define TRACE_FAT(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_FAT(...)      
#endif // TR_FAT

#if defined(TR_AUDIO)
    #define TRACE_AUDIO(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_AUDIO(...)      
#endif // TR_AUDIO

#if defined(TR_BT)
    //#define TRACE_BT(...)      rprintf(DBG, __VA_ARGS__)    
    #define TRACE_BT(...)      usart_rprintf(__VA_ARGS__)    
#else
    #define TRACE_BT(...)      
#endif // TR_BT

#if defined(TR_LUA)
    #define TRACE_LUA(...)      rprintf(DBG, __VA_ARGS__)    
#else
    #define TRACE_LUA(...)      
#endif // TR_LUA

#if defined(TR_BMP)
    #define TRACE_BMP(...)      rprintf(DBG, __VA_ARGS__)    
#else
    #define TRACE_BMP(...)      
#endif // TR_BMP

#if defined(TR_SYS)
    #define TRACE_SYS(...)      rprintf(DBG, __VA_ARGS__)    
#else
    #define TRACE_SYS(...)      
#endif // TR_SYS

#if defined(TR_TMR)
    #define TRACE_TMR(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_TMR(...)      
#endif // TR_TMR

#if defined(TR_RTC)
    #define TRACE_RTC(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_RTC(...)      
#endif // TR_RTC

#if defined(TR_ADC)
    #define TRACE_ADC(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_ADC(...)      
#endif // TR_ADC

#if defined(TR_SER)
    #define TRACE_SER(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_SER(...)      
#endif // TR_SER

#if defined(TR_SD)
    #define TRACE_SD(...)       rprintf(DBG,__VA_ARGS__)     
#else
    #define TRACE_SD(...)       
#endif // TR_SD

#if defined(TR_SPI)
    #define TRACE_SPI(...)       rprintf(DBG,__VA_ARGS__)     
#else
    #define TRACE_SPI(...)       
#endif // TR_SPI

#if defined(TR_I2C)
    #define TRACE_I2C(...)       rprintf(DBG,__VA_ARGS__)     
#else
    #define TRACE_I2C(...)       
#endif // TR_I2C

#if defined(TR_ACCEL)
    #define TRACE_ACCEL(...)       rprintf(DBG,__VA_ARGS__)     
#else
    #define TRACE_ACCEL(...)       
#endif // TR_ACCEL

#if defined(TR_USB)
    #define TRACE_USB(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_USB(...)      
#endif // TR_USB

#if defined(TR_BOT)
    #define TRACE_BOT(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_BOT(...)      
#endif // TR_BOT

#if defined(TR_SBC)
    #define TRACE_SBC(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_SBC(...)      
#endif // TR_SBC

#if defined(TR_LUN)
    #define TRACE_LUN(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_LUN(...)      
#endif // TR_LUN

#ifdef TR_INFO
    #define TRACE_INFO(...)         rprintf(DBG,__VA_ARGS__)    
    #define TRACE_X(...)         rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_INFO(...)         
#endif // TR_INFO

#ifdef TR_WARNING
    #define TRACE_WARNING(...)      rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_WARNING(...)      
#endif // TR_WARNING

#ifdef TR_ERROR
    #define TRACE_ERROR(...)        rprintf(DBG,__VA_ARGS__)    
#else
    #define TRACE_ERROR(...)        
#endif // TR_ERROR

#endif // _TRACE_H
