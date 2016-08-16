/**
 * \file rprintf.h
 * Header: printf functionality
 * 
 * Printf functionality
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
#ifndef PRINTF_H_
#define PRINTF_H_

void rprintf(void (*stream)(char), const char *fmt, ...);
void spucharInit(char*s);
void sputchar(char c);

#define srprintf(s,...)      \
 spucharInit(s); \
rprintf(sputchar,__VA_ARGS__)

#endif
