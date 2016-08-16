#include "AT91SAM7SE512.h"
#include "debug/trace.h"

extern unsigned int __abort_mem;
extern unsigned int __abort_typ;

void abort_dump() {
/*
    uint32_t *abort_type = *(uint32_t *)__abort_typ;
    uint32_t *abort_mem = *(uint32_t *)__abort_mem;
    //TRACE_ERROR("\r\nABORT %d %x\r\n", abort_type, abort_mem[0]);
    //TRACE_ERROR("\r\nABORT %x %x\r\n", abort_type, abort_mem);
*/
    //TRACE_ERROR("\r\nABORT %x %x\r\n", &__abort_typ, &__abort_mem);
    AT91C_BASE_AIC->AIC_IDCR = 0xffffffff;
    //TRACE_ERROR("\r\nABORT %x %x\r\n", __abort_typ, __abort_mem);
    panic_abort(__abort_typ, __abort_mem);
}

