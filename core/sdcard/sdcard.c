/**
 * \file sdcard.c
 * SD card handler
 *
 * Code for SD card interface and communication handling
 *
 * FIXIES, UPDATES, PORTS, ANSI-C: Wrist Technology Ltd 
 *
 *  original:  
 * AT91SAM7S-128 USB Mass Storage Device with SD Card by Michael Wolf\n
 * Copyright (C) 2008 Michael Wolf\n\n
 * 
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
#include <string.h>
#include "hardware_conf.h"
#include "firmware_conf.h"
#include <debug/trace.h>
#include <utils/macros.h>
#include <peripherals/spi.h>
#include <utils/delay.h>
#include <utils/time.h>
#include "sdcard.h"
#include "timer.h"
#include "rtos.h"
#include "task.h"
#include "queue.h"
#include "task_param.h"
#include "io.h"

#define SD_PM       1
#define SPI_DMA     1

#define SPI_SD_CHANNEL  1

#define PIO_SDPOWER  IO_PA19

#define WAKEUP_EVENT_CLOSE          1
#define WAKEUP_EVENT_TIMED_EVENT    2

#if SD_PM
#define SD_PM_TIMEOUT    2000

static Io sd_io;
static xTaskHandle sd_task_handle;
static xSemaphoreHandle sd_mutex;
static xQueueHandle sd_queue;
static Timer sd_timer;
static bool sd_ready;
static bool sd_timeout;
static bool sd_pm = true;

#endif

static uint8_t sd_ver2_card = false;  //!< Flag to indicate version 2.0 SD card
static uint8_t sd_sdhc_card = false;  //!< Flag to indicate version SDHC card
static uint32_t sd_numsectors; //!< Total number of sectors on card
static uint64_t sd_size = 0; //!< Total number of sectors on card
static uint32_t sd_blocksize; //!< Total number of sectors on card
static uint8_t sd_sectorbuffer[512];   //!< buffer to hold one sector of card
static uint32_t sd_clock; //!< Total number of sectors on card

#if SPI_DMA
static uint8_t spi_dummy_buff_in[512];
static uint8_t spi_dummy_buff_out[512];
#endif

uint64_t sd_get_size(void) {
    return sd_size;
}

/**
 * Return SD card present status.
 * 
 * Indicate the presence of a memory card in the SD slot.
 * \return  Card Status
 * 
 */
uint8_t sd_card_detect(void)
{
    /*        
    pPIO->PIO_PER = PIN_CARD_DETECT;
    pPIO->PIO_ODR = PIN_CARD_DETECT;
    pPIO->PIO_PPUER = PIN_CARD_DETECT;
    
    if (ISSET(pPIO->PIO_PDSR,PIN_CARD_DETECT))
        return SD_OK;            
    else
        return SD_NOCARD;
    */
    return SD_OK;    
}

/**
 * SD command
 * 
 * Send SD command to SD card
 * \param   cmd     Card command
 * \param   arg     Argument
 * 
 */
void sd_command(uint8_t cmd, uint32_t arg)
{
    //TRACE_SD("SDCMD[%d]",cmd);
    //spi_lock();

    spi_byte(SPI_SD_CHANNEL, 0xff,0);        // dummy byte
    spi_byte(SPI_SD_CHANNEL, cmd | 0x40,0);  // send command
    spi_byte(SPI_SD_CHANNEL, arg>>24,0);     // send argument
    spi_byte(SPI_SD_CHANNEL, arg>>16,0);
    spi_byte(SPI_SD_CHANNEL, arg>>8,0);
    spi_byte(SPI_SD_CHANNEL, arg,0);
    
    switch(cmd)
    {
    	case SD_GO_IDLE_STATE:
    	    spi_byte(SPI_SD_CHANNEL, 0x95,1); // CRC for CMD0
    	    break;
    	/*
    	 * CRC for CMD8 always enabled see:
    	 * Physical Layer Simplified Specification Version 2.00
    	 * chapter 7.2.2 Bus Transfer Protection
    	 */
    	case SD_SEND_IF_COND:
    		spi_byte(SPI_SD_CHANNEL, 0x87,1); // CRC for CMD8, argument 0x000001AA, see sd_init
    		break;
    		
    	default:
    		spi_byte(SPI_SD_CHANNEL, 0xFF,1); // send dummy CRC for all other commands
    }
    //spi_unlock();
}

/**
 * Send dummys
 * 
 * Send 10 dummy bytes to card
 * 
 */
void sd_send_dummys(void)
{
    uint8_t i;
    
    //spi_lock();

    for(i=0; i < 9; i++)
        spi_byte(SPI_SD_CHANNEL, 0xff,0);
    
    spi_byte(SPI_SD_CHANNEL, 0xff,1);

    //spi_unlock();
}

/**
 * Get response
 * 
 * Get card response tokens
 * \return Response token
 * 
 */
uint8_t sd_get_response(void)
{
    //uint32_t tmout = timeval + 1000;    // 1 second timeout
    uint32_t tmout = Timer_tick_count() + 1000;    // 1 second timeout
    uint8_t b = 0xff;

    //spi_lock();

    //while ((b == 0xff) && (timeval < tmout)) 
    while ((b == 0xff) && (Timer_tick_count() < tmout)) 
    {
        b = spi_byte(SPI_SD_CHANNEL, 0xff,0); //TRACE_SD("x");
    }
    //spi_unlock();

    return b;
}

/**
 * Get data token
 * 
 * Get card data response tokens
 * \return Data token
 * 
 */
uint8_t sd_get_datatoken(void)
{
    //uint32_t tmout = timeval + 1000;    // 1 second timeout
    uint32_t tmout = Timer_tick_count() + 1000;    // 1 second timeout
    uint8_t b = 0xff;

    //spi_lock();

    //while ((b != SD_STARTBLOCK_READ) && (timeval < tmout)) 
    while ((b != SD_STARTBLOCK_READ) && (Timer_tick_count() < tmout)) 
    {
        b = spi_byte(SPI_SD_CHANNEL, 0xff,0);
    }

    //spi_unlock();

    return b;
}

/**
 * Init SD card
 * 
 * Init SD card
 * \return  Error code
 * 
 */
#if SD_PM
int8_t _sd_init(void)
#else
int8_t sd_init(void)
#endif
{
    // Card not initalized
    
    // setup card detect on PA18
    /*
    pPIO->PIO_PER   = PIN_CARD_DETECT;          // Enable PIO pin
    pPIO->PIO_ODR   = PIN_CARD_DETECT;          // Enable input
    pPIO->PIO_PPUER = PIN_CARD_DETECT;         // Enable pullup
    */
    uint8_t retries;
    uint8_t resp;

    TRACE_SD("Init SD card\r\n");
    
    int ticks = Timer_tick_count();
#if SPI_DMA
    memset(spi_dummy_buff_out, 0xff, 512);
#endif

    spi_lock();

    for(retries = 0, resp = 0; (retries < 5) && (resp != SD_R1_IDLE_STATE) ; retries++)
    {
        // send CMD0 to reset card
    	sd_command(SD_GO_IDLE_STATE,0);
        resp = sd_get_response();
 
        TRACE_SD("go idle resp: %X\n\r", resp);
        //delayms(100);
        Task_sleep(100);
    }
    
    if(resp != SD_R1_IDLE_STATE) return SD_E_IDLE;

    TRACE_SD("SD idle %d\r\n", Timer_tick_count() - ticks);

    // send CMD8 to check voltage range
    // this also determines if the card is a 2.0 (or later) card
    sd_command(SD_SEND_IF_COND,0x000001AA);
    resp = sd_get_response();

    TRACE_SD("CMD8resp: %02X\n\r",resp);    
    
    if ((resp & SD_R1_ILLEGAL_COM) != SD_R1_ILLEGAL_COM)
    {
    	TRACE_SD("2.0 card\n\r");
        uint32_t r7reply;
        sd_ver2_card = true;  // mark this as a version2 card
        r7reply = sd_get_response();     
        r7reply <<= 8;
        r7reply |= sd_get_response();    
        r7reply <<= 8;
        r7reply |= sd_get_response();    
        r7reply <<= 8;
        r7reply |= sd_get_response();

        TRACE_SD("CMD8REPLY: %08x\n\r",r7reply);
        
        // verify that we're compatible
        if ( (r7reply & 0x00000fff) != 0x01AA )
        {
            spi_unlock();
            TRACE_SD("Voltage range mismatch\n\r");
            return SD_E_VOLT;  // voltage range mismatch, unsuable card
        }
    }
    else
    {
         TRACE_SD("Not a 2.0 card\n\r");
    }   

    sd_send_dummys();

    /*
     * send ACMD41 until we get a 0 back, indicating card is done initializing
     * wait for max 5 seconds
     * 
     */
    for (retries=0,resp=0; !resp && retries<50; retries++)
    {
        uint8_t i;
        // send CMD55
        sd_command(SD_APP_CMD, 0);    // CMD55, prepare for APP cmd

        TRACE_SD("Sending CMD55\n\r");
        
        if ((sd_get_response() & 0xFE) != 0)
        {
             TRACE_SD("CMD55 failed\n\r");
        }
        // send ACMD41
        TRACE_SD("Sending ACMD41\n\r");
        
        if(sd_ver2_card)
        	sd_command(SD_ACMD_SEND_OP_COND, 1UL << 30); // ACMD41, HCS bit 1
        else
        	sd_command(SD_ACMD_SEND_OP_COND, 0); // ACMD41, HCS bit 0
        
        i = sd_get_response();
        
        TRACE_SD("response = %02x\n\r",i);
        
        if (i != 0)
        {
            sd_send_dummys();
            //delayms(100);
            Task_sleep(100);
        }
        else    
            resp = 1;
        
        //delayms(500);
        //Task_sleep(500);
    }

    if (!resp)
    {
        spi_unlock();
        TRACE_SD("not valid\n\r");
        return SD_E_INIT;          // init failure
    }
    sd_send_dummys();     // clean up

    if (sd_ver2_card)
    {
        uint32_t ocr;
        // check for High Cap etc
        
        // send CMD58
        TRACE_SD("sending CMD58\n\r");

        sd_command(SD_READ_OCR,0);    // CMD58, get OCR
        TRACE_SD(".resp.");
        if (sd_get_response() != 0)     // MV blocks upon sd_init() call
        {
            TRACE_SD("CMD58 failed\n\r");
        }
        else
        {
            // 0x80, 0xff, 0x80, 0x00 would be expected normally
            // 0xC0 if high cap
            
            ocr = sd_get_response();
            ocr <<= 8;
            ocr |= sd_get_response();
            ocr <<= 8;
            ocr |= sd_get_response();
            ocr <<= 8;
            ocr |= sd_get_response();
             
            TRACE_SD("OCR = %08x\n\r", ocr);
            
            if((ocr & 0xC0000000) == 0xC0000000)
            {
            	TRACE_SD("SDHC card.\n\r");
            	sd_sdhc_card = true; // Set HC flag.
            }
        }
    }
    sd_send_dummys();     // clean up

    if (sd_size == 0) {
        sd_size = sd_info();
    }

    spi_unlock();
    TRACE_SD("Init SD card OK %d\n\r", Timer_tick_count() - ticks);
    return SD_OK;   
}

#define KBPS 1
#define MBPS 1000

static uint32_t ts_exp[] = { 100*KBPS, 1*MBPS, 10*MBPS, 100*MBPS, 0, 0, 0, 0 };
static uint32_t ts_mul[] = { 0,    1000, 1200, 1300, 1500, 2000, 2500, 3000, 
                  3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000 };

static uint32_t sd_tran_speed(uint8_t ts)
{
      uint32_t clock = ts_exp[(ts & 0x7)] * ts_mul[(ts & 0x78) >> 3];

      TRACE_SD("clock :%d\r\n", clock);
      return clock;
}


/**
 * Get Card Information.
 * 
 * Read and print some card information and return card size in bytes
 * 
 * \return     Cardsize in bytes
 * \todo This return value should be changed to allow cards > 4GB
 * 
*/
//uint32_t sd_info(void)
uint64_t sd_info(void)
{
    int i;
    uint32_t l;
    uint16_t w;
    uint8_t b;

    uint16_t csize;
    uint8_t csize_mult;
    uint32_t blockno;
    uint16_t mult;
    uint16_t block_len; //Wrist Technology Ltd

#ifdef TR_SD // CID only needed for tracing
    sd_send_dummys();     // cleanup  
    
    sd_command(SD_SEND_CID,0);
    if (sd_get_datatoken() != SD_STARTBLOCK_READ) 
         TRACE_SD("Error during CID read\n");
    else 
    {
        TRACE_SD("CID read\n");

        TRACE_SD("Manufacturer ID: %02x\r\n",spi_byte(SPI_SD_CHANNEL, 0xff,0));

        //spi_lock();

        w = spi_byte(SPI_SD_CHANNEL, 0xff,0);
        w <<= 8;
        w |= spi_byte(SPI_SD_CHANNEL, 0xff,0);
        TRACE_SD("OEM/Application ID: %02x\n",w);

        TRACE_SD("Product Name: ");
        for (i=0;i<6;i++) 
            TRACE_SD("%c",spi_byte(SPI_SD_CHANNEL, 0xff,0));

        TRACE_SD("\nProduct Revision: %02x\n",spi_byte(SPI_SD_CHANNEL, 0xff,0));

        l = spi_byte(SPI_SD_CHANNEL, 0xff,0);
        l <<= 8;
        l |= spi_byte(SPI_SD_CHANNEL, 0xff,0);
        l <<= 8;
        l |= spi_byte(SPI_SD_CHANNEL, 0xff,0);
        l <<= 8;
        l |= spi_byte(SPI_SD_CHANNEL, 0xff,0);
        TRACE_SD("Serial Number: %08lx (%ld)\n",l,l);
        TRACE_SD("Manuf. Date Code: %02x\n",spi_byte(SPI_SD_CHANNEL, 0xff,0));

        //spi_unlock();
    }

    //spi_lock();
    spi_byte(SPI_SD_CHANNEL, 0xff,0);    // skip checksum
    //spi_unlock();

#endif
    sd_send_dummys();
    
    sd_command(SD_SEND_CSD,0);
    if ((b = sd_get_datatoken()) != SD_STARTBLOCK_READ) 
    {
        TRACE_SD("Error during CSD read, token was %02x\n",b);
        sd_send_dummys();
        return 0;
    }
    else 
    {
        TRACE_SD("CSD read\n");
    }

    //spi_lock();

    // we need C_SIZE (bits 62-73 (bytes ) and C_SIZE_MULT (bits 47-49)
    //  0,  8 , 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96,104,112,120
    // 16  15   14  13  12  11  10   9   8   7   6   5   4   3   2   1

    // read 1 byte [bits 127:120]
    l = spi_byte(SPI_SD_CHANNEL, 0xff,0);

// MV b
    spi_byte(SPI_SD_CHANNEL, 0xff,0);   // taac
    spi_byte(SPI_SD_CHANNEL, 0xff,0);   // nsac
    uint8_t trans_speed = spi_byte(SPI_SD_CHANNEL, 0xff,0);

    //spi_unlock();

    sd_clock = sd_tran_speed(trans_speed);
    TRACE_SD("TRANS_SPEED %x %d\r\n", trans_speed, sd_clock);
    spi_set_clock(SPI_SD_CHANNEL, sd_clock);  
// MV e
    
    //orig. if(l == 0) // CSD 1.0 structure
    //correction Wrist Technology Ltd:
    if((l&0xC0) == 0) // CSD 1.0 structure
    {
    	TRACE_SD("CSD 1.0\n");

        //spi_lock();
    	   //orig skip next 5 bytes [bits 119:80]
        //orig  for (i=0;i<5;i++) 
        //    spi_byte(SPI_SD_CHANNEL, 0xff,0);  //last con
        
        //Wrist Technology Ltd:
        //skip next 4 bytes [bits 119:88]
    	  //MV for (i=0;i<4;i++) 

        for (i=0;i<1;i++) 
            spi_byte(SPI_SD_CHANNEL, 0xff,0);  
        w=spi_byte(SPI_SD_CHANNEL, 0xff,0);  //bits [87:80]
        // [83:80] = block len
        w &= 0x0f;
        if (w<9) { TRACE_ERROR("Block len error %d", w); }
        if (w>11) { TRACE_ERROR("Block len error %d", w); }  
        block_len = (1 << w);   
        
        // get dword from [bits 79:56] 
        l = spi_byte(SPI_SD_CHANNEL, 0xff,0);
        l <<= 8;
        l |= spi_byte(SPI_SD_CHANNEL, 0xff,0);
        l <<= 8;
        l |= spi_byte(SPI_SD_CHANNEL, 0xff,0);

        // shift down to to access [bits 73:62]
        l >>= 6;
        csize = (uint16_t) (l & 0x0fff);
        TRACE_SD("C_SIZE = %04x\n",csize);

        // get word from [bits 55:40]
        w = spi_byte(SPI_SD_CHANNEL, 0xff,0);
        w <<= 8;
        w |= spi_byte(SPI_SD_CHANNEL, 0xff,0);

        //spi_unlock();

        // shift down to to access [bits 49:47]
        w >>= 7;
        csize_mult = (uint16_t) (w & 0x07);
        TRACE_SD("C_SIZE_MULT = %02x\n",csize_mult);

        mult = 1 << (csize_mult+2);
        blockno = (uint32_t) ((uint16_t)csize+1) * mult;
        TRACE_SD("mult = %0d\n",mult);
        TRACE_SD("blockno = %ld\n",blockno);
        TRACE_SD("block len = %ld\n",block_len);
        //TRACE_SD("card size = %lu / (%lu MByte)\n\n", blockno * (uint32_t)block_len,blockno / 2048L);
        TRACE_SD("card size = %lu / (%lu MByte)\n\n", blockno * (uint32_t)block_len,blockno / 1024L);

        sd_send_dummys();
        sd_numsectors = blockno;
        sd_blocksize = block_len;
        uint32_t byte_size = blockno * (uint32_t)block_len;
        return byte_size;
    }
    else
    //orig. if( l == 0x40) // CSD 2.0 structure
    //correction Wrist Technology Ltd:
    //if((l&0xC0) == 0) // CSD 1.0 structure
    if((l&0xC0) == 0x40) // CSD 2.0 structure
    {
    	TRACE_SD("CSD 2.0\n");
    	
        //spi_lock();
        // skip next 6 bytes [bits 119:72]
        //MV for (i=0;i<6;i++) 
        for (i=0;i<3;i++) 
            spi_byte(SPI_SD_CHANNEL, 0xff,0);
        
        // get dword from [bits 71:48] 
        l = spi_byte(SPI_SD_CHANNEL, 0xff,0);
        l <<= 8;
        l |= spi_byte(SPI_SD_CHANNEL, 0xff,0);
        l <<= 8;
        l |= spi_byte(SPI_SD_CHANNEL, 0xff,0);
        //spi_unlock();

        l &= 0x0000ffff; // mask c_size field
        
        //uint32_t byte_size = ((l+1) * 524288L);
        uint64_t byte_size = ((l+1) * 524288LL);
        
        TRACE_SD("C_SIZE = %08x\n",l);
        //TRACE_SD("card size = %lu / (%lu MByte)\n\n",byte_size , ((l+1)>>1));
        TRACE_SD("card size = %lu * 4G + %lu / (%lu MByte)\n\n", (uint32_t)(byte_size >> 32), (uint32_t)(byte_size & 0xffffffff) , ((l+1)>>1));

        sd_send_dummys();
        sd_numsectors = (byte_size / 512)-1;
// TODO
        sd_blocksize = 512;

        return byte_size;
    }
    else
    {
    	TRACE_SD("Invalid CSD structure!\n");
    	sd_numsectors = 0;
    	return 0;    	
    }
}

/**
 * Read SD sector.
 * 
 * Read a single 512 byte sector from the SD card
 * \param   lba         Logical sectornumber to read from
 * \param   buffer      Pointer to buffer for received data
 * \param   fCallback   Callback function name
 * \param   pArgument   Callback argument
 * \return 0 on success, -1 on error
 * 
*/  
static int c = 0;
int8_t sd_readsector(uint32_t lba,
                     uint8_t *buffer,
                     Callback_f   fCallback,
                     void *pArgument)
{
    uint16_t i;
    
    TRACE_SD("SDrd%ld   ", lba);

    //spi_lock();
    sd_lock();
    if(sd_sdhc_card)
    	 // on new High Capacity cards, the lba is sent
    	sd_command(SD_READ_SINGLE_BLOCK,lba);
    else
    	sd_command(SD_READ_SINGLE_BLOCK,lba<<9);
    	// send read command and logical sector address
    	// the address sent to the card is the BYTE address
    	// so the lba needs to be multiplied by 512
    
    if (sd_get_response() != 0) // if no valid token
    {
        sd_send_dummys(); // cleanup and  
        //spi_unlock();
        sd_unlock();
        TRACE_ERROR("sd_readsector() error: cmd response timeout\r\n");
        return SD_ERROR;   // return error code
    }

    if (sd_get_datatoken() != SD_STARTBLOCK_READ) // if no valid token
    {
        sd_send_dummys(); // cleanup and  
        //spi_unlock();
        sd_unlock();
        TRACE_ERROR("sd_readsector() error: data packet timeout\r\n");
        return SD_ERROR;   // return error code
    }

    uint8_t *b = buffer;
    //spi_lock();
#if SPI_DMA
    spi_rw_bytes(SPI_SD_CHANNEL, spi_dummy_buff_out, buffer, 512, 0);
#else
    for (i=0; i<512 ; i++)             // read sector data
        *buffer++ = spi_byte(SPI_SD_CHANNEL, 0xff,0);
#endif
        
    spi_byte(SPI_SD_CHANNEL, 0xff,0);    // ignore dummy checksum
    spi_byte(SPI_SD_CHANNEL, 0xff,0);    // ignore dummy checksum
    //spi_unlock();

#if 0
    int s = 0;
    for (i=0; i<512 ; i++) {
        if (b[i]) {
            TRACE_SD("spi %d %x\r\n", i, b[i]);
            s += b[i];
        }
    }
    if (s) {
        //TRACE_SD("spi %d %x\r\n", i, b[i]);
    }
    TRACE_SD("sum %x\r\n", s);
    if (c++ == 1) {
        while(1);
    }
#endif

    sd_send_dummys();     // cleanup
    //spi_unlock();
    sd_unlock();

    // Invoke callback
    if (fCallback != 0) {

        fCallback((unsigned int) pArgument, SD_OK, 0, 0);
    }

    return SD_OK;                       // return success       
}


/** 
 * Read part of a SD sector.
 * 
 * Read part of a single 512 byte sector from the SD card
 * \param  block   Logical sectornumber to read from
 * \param  loffset Offset to first byte we should read
 * \param  nbytes  Number of bytes to read
 * \param  buffer  Pointer to buffer for received data
 * \return 1 on success, 0 on error
 * 
*/ 
uint8_t sd_read_n(uint32_t block,uint16_t loffset, uint16_t nbytes, uint8_t * buffer)
{
    sd_readsector(block,sd_sectorbuffer, 0, 0);
    memcpy(buffer,&sd_sectorbuffer[loffset],nbytes);
    return 0;
}


/**
 * Write SD sector.
 * 
 * Write a single 512 byte sector to the SD card
 * 
 * \param   lba         Logical sectornumber to write to
 * \param   buffer      Pointer to buffer with data to send
 * \param   fCallback   Callback function name
 * \param   pArgument   Callback argument
 * \return 0 on success, -1 on error
*/  
int8_t sd_writesector(uint32_t lba,
                      uint8_t *buffer,
                      Callback_f   fCallback,
                      void *pArgument)
{
    uint16_t i;
    uint32_t tmout;

    TRACE_SD("SDwr%ld   ", lba);

    //spi_lock();
    sd_lock();
    if(sd_sdhc_card)
    	 // on new High Capacity cards, the lba is sent
    	sd_command(SD_WRITE_BLOCK,lba);
    else
    	sd_command(SD_WRITE_BLOCK,lba<<9);
    	// send read command and logical sector address
    	// the address sent to the card is the BYTE address
    	// so the lba needs to be multiplied by 512
    
    if (sd_get_response() != 0) // if no valid token
    {
        sd_send_dummys(); // cleanup and
        //spi_unlock();
        sd_unlock();
        TRACE_ERROR("sd_writesector() error: cmd response timeout\r\n");
        return SD_ERROR;   // return error code
    }

    //spi_lock();
    spi_byte(SPI_SD_CHANNEL, 0xfe,0);    // send data token

#if SPI_DMA
    spi_rw_bytes(SPI_SD_CHANNEL, buffer, spi_dummy_buff_in, 512, 0);
#else
    for (i=0;i<512;i++)             // write sector data
    {
        spi_byte(SPI_SD_CHANNEL, *buffer++,0);
    }
#endif

    spi_byte(SPI_SD_CHANNEL, 0xff,0);    // send dummy checksum
    spi_byte(SPI_SD_CHANNEL, 0xff,0);    // send dummy checksum
    //spi_unlock();

    if ( (sd_get_response()&0x0F) != 0x05) // if no valid token
    {
        sd_send_dummys(); // cleanup and
        //spi_unlock();
        sd_unlock();
        TRACE_ERROR("sd_writesector() error: data response timeout\r\n");
        return SD_ERROR;   // return error code
    }

    //spi_lock();
    //
    // wait while the card is busy
    // writing the data
    //
    //tmout = timeval + 1000;
    tmout = Timer_tick_count() + 1000;

    // wait for the SO pin to go high
    while (1)
    {
        uint8_t b = spi_byte(SPI_SD_CHANNEL, 0xff,0);

        if (b == 0xff) break;   // check SO high
        
        //if (timeval > tmout)    // if timeout
        if (Timer_tick_count() > tmout)    // if timeout
        {
            //spi_unlock();
            sd_send_dummys();   // cleanup and
            //spi_unlock();
            sd_unlock();
            TRACE_ERROR("sd_writesector() error: busy timeout\r\n");
            return SD_ERROR;    // return failure
        }

    }
    //spi_unlock();

    sd_send_dummys(); // cleanup  
    //spi_unlock();
    sd_unlock();
    
    // Invoke callback
    if (fCallback != 0) {

        fCallback((unsigned int) pArgument, SD_OK, 0, 0);
    }
   
    return SD_OK;   // return success
}

int8_t sd_lock() {
#if SD_PM
    if (sd_pm || !sd_ready) {
        xSemaphoreTake(sd_mutex, -1);

        if (sd_timeout) {
            Timer_stop(&sd_timer);
            sd_timeout = false;
        }
        if (!sd_ready) {
            // PA19 high
            Io_setValue(&sd_io, 1);
            //Task_sleep(10);
            _sd_init();
            sd_ready = true;
            TRACE_INFO("sd powered on\r\n");
        }

        xSemaphoreGive(sd_mutex);
    }
#endif
    spi_lock();
    return SD_OK;
}

#if SD_PM
void sd_start_timer(void) {
    xSemaphoreTake(sd_mutex, -1);

    Timer_start(&sd_timer, SD_PM_TIMEOUT, false, false);
    sd_timeout = true;

    xSemaphoreGive(sd_mutex);
}
#endif

int8_t sd_unlock() {
    spi_unlock();
#if SD_PM
    if (sd_pm)
        sd_start_timer();
#endif
    return SD_OK;
}

#if SD_PM
void sd_set_pm(bool on) {
    if (sd_pm == on) {
        return;
    }
    sd_pm = on;
    if (on) {
        sd_start_timer();
    }
}

void sd_timer_handler(void* context) {
    //TRACE_INFO("sd_timer_handler\r\n");

    portBASE_TYPE xHigherPriorityTaskWoken;
    uint8_t event = WAKEUP_EVENT_TIMED_EVENT;

    xQueueSendFromISR(sd_queue, &event, &xHigherPriorityTaskWoken);

    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void sd_task( void* p ) {
    TRACE_INFO("sd task %x\r\n", xTaskGetCurrentTaskHandle());

    while (true) {
        uint8_t sd_event;
        xQueueReceive(sd_queue, &sd_event, -1);

        if (sd_event == WAKEUP_EVENT_CLOSE) {
            Timer_stop(&sd_timer);
            break;
        } 

        xSemaphoreTake(sd_mutex, -1);

        sd_timeout = false;
        if (sd_ready) {
            // PA19 low
            Io_setValue(&sd_io, 0);
            sd_ready = false;
            TRACE_INFO("sd powered off\r\n");
        }
        xSemaphoreGive(sd_mutex);
    }
    Timer_close(&sd_timer);
    vQueueDelete(sd_queue);
    vTaskDelete(NULL);
}

int8_t sd_init () {

    Io_init(&sd_io, PIO_SDPOWER, IO_GPIO, OUTPUT);
    Io_setValue(&sd_io, 1);

    _sd_init();

    Io_setValue(&sd_io, 0);
    TRACE_INFO("sd powered off\r\n");

    sd_ready = false;
    sd_timeout = false;

    Timer_init(&sd_timer, 0);
    Timer_setHandler(&sd_timer, sd_timer_handler, NULL);

    sd_mutex = xSemaphoreCreateMutex();

    if (sd_mutex == NULL) {
        panic("sd_init mutex");
        return SD_E_INIT;
    }
    sd_queue = xQueueCreate(1, sizeof(uint8_t));
    if (sd_queue == NULL) {
        panic("sd_init queue");
        return SD_E_INIT;
    }

    if (xTaskCreate( sd_task, "sd", TASK_STACK_SIZE(TASK_SD_STACK), NULL, TASK_SD_PRI, &sd_task_handle ) != 1 ) {
        return SD_E_INIT;
    }

    return SD_OK;
}
#endif

int8_t sd_close() {
#if SD_PM
    uint8_t ev = WAKEUP_EVENT_CLOSE;
    xQueueSend(sd_queue, &ev, 0);
    vQueueDelete(sd_mutex);
#endif
    return SD_OK;
}

