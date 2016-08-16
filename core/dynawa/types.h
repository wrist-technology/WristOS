/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef TYPES_H
#define TYPES_H

// MV
#include <inttypes.h>

/* some types */
#ifndef __cplusplus

#undef bool

//MV yabcsp/src/commonc.h
// ?typedef unsigned bool;
typedef unsigned char bool;

#endif

typedef unsigned char uchar;
typedef unsigned char uint8;
typedef unsigned int uint;
typedef unsigned short int uint16;
typedef unsigned long long int uint64;
typedef long long int llong;
typedef long long int longlong;
typedef unsigned long long int ullong;
typedef unsigned long long int ulonglong;

// MV yabcsp
typedef unsigned char   bool_t;

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef NULL
#define NULL 0
#endif

#endif
