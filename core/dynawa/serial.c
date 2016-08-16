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

#include "serial.h"
#include "io.h"
#include "board.h"
#include "irq_param.h"
#include "error.h"
#include "debug/trace.h"

extern volatile portTickType xTickCount;

Serial_Internal Serial_internals[SERIAL_PORTS];
extern void (Serial0Isr_Wrapper)(void);
extern void (Serial1Isr_Wrapper)(void);

/**
  Create a new serial port.

  \b Example
  \code
  Serial ser(0);
  \endcode
  */
void Serial_open( int channel, int q_size )
{
    Io rx;
    Io tx;
    TRACE_SER("Serial_init %d %d\r\n", channel, q_size);
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[channel];
    //if( sp->rxQueue == NULL )
        sp->rxQueue = Queue_create( q_size, 1 );
    //if( sp->txQueue == NULL )
        sp->txQueue = Queue_create( q_size, 1 );

    sp->rxSem = Semaphore_create();
    sp->txSem = Semaphore_create();

    // default to SERIAL_0 values
    int id = AT91C_ID_US0;
    int rxPin = IO_PA05;
    int txPin = IO_PA06;
    //long rxPinBit = IO_PA05_BIT;
    //long txPinBit = IO_PA06_BIT;
    switch( channel )
    {
        case 0:
            // values already set for this above
            sp->uart = AT91C_BASE_US0;
            break;
        case 1:
            id = AT91C_ID_US1;
            //rxPinBit = IO_PA21_BIT;
            //txPinBit = IO_PA22_BIT;
            rxPin = IO_PA21;
            txPin = IO_PA22;
            sp->uart = AT91C_BASE_US1;
            break;
    }

    unsigned int mask = 0x1 << id;                      

    AT91C_BASE_PMC->PMC_PCER = mask;   // Enable the peripheral clock
    sp->baud = SERIAL_DEFAULT_BAUD;
    sp->bits = SERIAL_DEFAULT_BITS;
    sp->stopBits = SERIAL_DEFAULT_STOPBITS;
    sp->parity = SERIAL_DEFAULT_PARITY;
    sp->handshaking = SERIAL_DEFAULT_HANDSHAKING;
    sp->uart->US_IDR = (unsigned int) -1; // Disable interrupts
    sp->uart->US_TTGR = 0;                // Timeguard disabled
    Io_init( &rx, rxPin, IO_A, false );
    Io_init( &tx, txPin, IO_A, true );

    Serial_setDetails( channel );

    // Disable the interrupt on the interrupt controller
    AT91C_BASE_AIC->AIC_IDCR = mask;
#if 1
    // Save the interrupt handler routine pointer and the interrupt priority
    if(channel == 0)
        AT91C_BASE_AIC->AIC_SVR[ id ] = (unsigned int)Serial0Isr_Wrapper;
    else
        AT91C_BASE_AIC->AIC_SVR[ id ] = (unsigned int)Serial1Isr_Wrapper;
    // Store the Source Mode Register
    AT91C_BASE_AIC->AIC_SMR[ id ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | IRQ_SERIAL_PRI;

#else
// test FIQ
    //AT91C_BASE_AIC->AIC_SMR[ 0 ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | IRQ_SERIAL_PRI;
    AT91C_BASE_AIC->AIC_SMR[ id ] = AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE | IRQ_SERIAL_PRI;
    AT91C_BASE_AIC->AIC_SVR[ 0 ] = (unsigned int)Serial0Isr_Wrapper;
    AT91C_BASE_AIC->AIC_FFER = mask;
#endif

    //AT91C_BASE_AIC->AIC_SMR[ id ] = AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE | IRQ_SERIAL_PRI;
    // Clear the interrupt on the interrupt controller
    AT91C_BASE_AIC->AIC_ICCR = mask;
    AT91C_BASE_AIC->AIC_IECR = mask;

    // MV
    sp->rxBreak = 0;
    sp->uart->US_IER = AT91C_US_RXBRK;
#if DMA
/*
    sp->rxCurBuf = 0;
    sp->rxCurBufPos = 0;
    sp->uart->US_RPR = &sp->rxBuf[0];
    sp->uart->US_RCR = SERIAL_RX_BUF_SIZE;

    sp->uart->US_RNPR = &sp->rxBuf[1];
    sp->uart->US_RNCR = SERIAL_RX_BUF_SIZE;

    sp->uart->US_PTCR = AT91C_PDC_RXTEN;

    //sp->uart->US_IER = AT91C_US_RXBUFF;  // both current & next buffer
    sp->uart->US_IER = AT91C_US_ENDRX; // current buffer
*/
#else
    sp->uart->US_IER = AT91C_US_RXRDY;
#endif
}

void Serial_close (int channel) {
    Serial_Internal* sp = &Serial_internals[channel];

    int id = AT91C_ID_US0;
    switch( channel )
    {
        case 0:
            break;
        case 1:
            id = AT91C_ID_US1;
            break;
    }

    unsigned int mask = 0x1 << id;                      

    // Disable the interrupt on the interrupt controller
    AT91C_BASE_AIC->AIC_IDCR = mask;
    AT91C_BASE_PMC->PMC_PCDR = mask;   // Disable the peripheral clock

    if (sp->rxQueue) {
        Queue_delete(sp->rxQueue);
    }
    if (sp->txQueue) {
        Queue_delete(sp->txQueue);
    }
    if (sp->rxSem) {
        Semaphore_delete(sp->rxSem);
    }
    if (sp->txSem) {
        Semaphore_delete(sp->txSem);
    }
}

void Serial_setDetails( int channel )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;

    Serial_Internal* sp = &Serial_internals[ channel ];
    // Reset receiver and transmitter
    sp->uart->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS; 


/*
    // MCK is 47923200 for the Make Controller Kit
    // Calculate ( * 10 )
    int baudValue = ( MCK * 10 ) / ( sp->baud * 16 );
    // Round (and / 10)
    if ( ( baudValue % 10 ) >= 5 ) 
        baudValue = ( baudValue / 10 ) + 1; 
    else 
        baudValue /= 10;

    sp->uart->US_BRGR = baudValue;
*/

    int cd = ((MCK / sp->baud) / 16);
    int min_baudrate_diff;
    int best_fd;
    int fd;
    for (fd = 0; fd < 8; fd++) {
        int baudrate = MCK / (16 * (cd + fd / 8.0));

        int baudrate_diff = abs(baudrate - sp->baud);
        if (!fd || baudrate_diff < min_baudrate_diff) {
            min_baudrate_diff = baudrate_diff;
            best_fd = fd;
        }
        TRACE_INFO("fd %d br %d\r\n", fd, baudrate);
    }
    if (best_fd) {
        cd |= (best_fd << 16);
    }
    sp->uart->US_BRGR = cd;

    TRACE_INFO("US_BRGR %x\r\n", sp->uart->US_BRGR);

    sp->uart->US_MR = ( AT91C_US_CHMODE_NORMAL ) |
        ( ( sp->handshaking ) ? AT91C_US_USMODE_HWHSH : AT91C_US_USMODE_NORMAL ) |
        ( AT91C_US_CLKS_CLOCK ) |
        ( ( ( sp->bits - 5 ) << 6 ) & AT91C_US_CHRL ) |
        ( ( sp->stopBits == 2 ) ? AT91C_US_NBSTOP_2_BIT : AT91C_US_NBSTOP_1_BIT ) |
        ( ( sp->parity == 0 ) ? AT91C_US_PAR_NONE : ( ( sp->parity == -1 ) ? AT91C_US_PAR_ODD : AT91C_US_PAR_EVEN ) );
    // 2 << 14; // this last thing puts it in loopback mode

    sp->uart->US_CR = AT91C_US_RXEN | AT91C_US_TXEN;
}

/**
  Set the baud rate of a serial port.

  \b Example
  \code
  Serial ser(0);
  ser.setBaud(115200);
  \endcode
  */
void Serial_setBaud( int channel, int rate )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];

    sp->baud = rate;
    Serial_setDetails( channel );
}

/**
  Returns the current baud rate.
  @return The current baud rate.

  \b Example
  \code
  Serial ser(0);
  int baudrate = ser.getBaud();
  \endcode
  */
int Serial_baud( int channel )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return -1;
    Serial_Internal* sp = &Serial_internals[ channel ];

    return sp->baud;
}

/**
  Sets the number of bits per character.
  5 through 8 are legal values - 8 is the default.
  @param bits bits per character

  \b Example
  \code
  Serial ser(0);
  ser.setDataBits(5);
  \endcode
  */
void Serial_setDataBits( int channel, int bits )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];

    if ( bits >= 5 && bits <= 8 )
        sp->bits = bits;
    else
        sp->bits = 8;
    Serial_setDetails( channel );
}

/**
  Returns the number of bits for each character.
  @return The current data bits setting.

  \b Example
  \code
  Serial ser(0);
  int dbits = ser.getDataBits();
  \endcode
  */
int Serial_dataBits( int channel )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];
    return sp->bits;
}

/**
  Sets the parity.
  -1 is odd, 0 is none, 1 is even.  The default is none - 0.
  @param parity -1, 0 or 1.

  \b Example
  \code
  Serial ser(0);
  ser.setParity(-1); // set to odd parity
  \endcode
  */
void Serial_setParity( int channel, int parity )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];
    if ( parity >= -1 && parity <= 1 )
        sp->parity = parity;
    else
        sp->parity = 1;
    Serial_setDetails( channel );
}

/**
  Returns the current parity.
  -1 is odd, 0 is none, 1 is even.  The default is none - 0.
  @return The current parity setting.

  \b Example
  \code
  Serial ser(0);
  int par = getParity();
  \endcode
  */
int Serial_parity( int channel )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];
    return sp->parity;
}

/**
  Sets the stop bits per character.
  1 or 2 are legal values.  1 is the default.
  @param bits stop bits per character

  \b Example
  \code
  Serial ser(0);
  ser.setStopBits(2);
  \endcode
  */
void Serial_setStopBits( int channel, int bits )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];
    if ( bits == 1 || bits == 2 )
        sp->stopBits = bits;
    else
        sp->stopBits = 1;
    Serial_setDetails( channel );
}

/**
  Returns the number of stop bits.
  @return The number of stop bits.

  \b Example
  \code
  Serial ser(0);
  int sbits = ser.getStopBits();
  \endcode
  */
int Serial_stopBits( int channel )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];
    return sp->stopBits;
}

/**
  Sets whether hardware handshaking is being used.
  @param enable Whether to use handshaking - true or false.

  \b Example
  \code
  Serial ser(0);
  ser.setHandshaking(true); // enable hardware handshaking
  \endcode
  */
void Serial_setHandshaking( int channel, bool enable )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];
    sp->handshaking = enable;
    Serial_setDetails( channel );
}

/**
  Returns whether hardware handshaking is enabled or not.
  @return Wheter handshaking is currently enabled - true or false.

  \b Example
  \code
  Serial ser(0);
  if( ser.getHandshaking() )
  {
// then handshaking is enabled
}
\endcode
*/
bool Serial_handshaking( int channel )
{
    if( channel < 0 || channel >= 2 ) // make sure channel is valid
        return;
    Serial_Internal* sp = &Serial_internals[ channel ];
    return sp->handshaking;
}

/**
  Write a single character
  @param character The character to write.
  */
int Serial_writeChar( int channel, char character )
{
    return 0;
}

/**
  Write a block of data
  @param data The data to send.
  @param length How many bytes of data to send.
  @param timeout How long to wait to make sure it goes through.
  */
int Serial_write( int channel, char* data, int length, int timeout )
{
    Serial_Internal* sp = &Serial_internals[ channel ];

    TRACE_SER("Serial_write %d %d\r\n", channel, length);
    while ( length ) // Do the business
    {
        if( Queue_send(sp->txQueue, data++, timeout ) == 0 ) 
            return CONTROLLER_ERROR_QUEUE_ERROR;
        length--;
    }

    /* Turn on the Tx interrupt so the ISR will remove the character from the 
       queue and send it. This does not need to be in a critical section as 
       if the interrupt has already removed the character the next interrupt 
       will simply turn off the Tx interrupt again. */ 
    sp->uart->US_IER = AT91C_US_TXRDY; 
    return CONTROLLER_OK;
}

int Serial_writeDMA(int channel, void *data, int length, int timeout)
{
    Serial_Internal* sp = &Serial_internals[ channel ];
    // Check if the first PDC bank is free
    //TRACE_SER("Serial_writeDMA %d %x %d\r\n", channel, data, length);
    if ((sp->uart->US_TCR == 0) && (sp->uart->US_TNCR == 0))
    {
        //TRACE_SER("TPR\r\n");
        sp->uart->US_TPR = (unsigned int) data;
        sp->uart->US_TCR = length;
        sp->uart->US_PTCR = AT91C_PDC_TXTEN;
    }
    // Check if the second PDC bank is free
    else if (sp->uart->US_TNCR == 0)
    {
        //TRACE_SER("TNPR\r\n");
        sp->uart->US_TNPR = (unsigned int) data;
        sp->uart->US_TNCR = length;
    } else {
        return CONTROLLER_ERROR_NO_SPACE;
    }

    sp->uart->US_IER = AT91C_US_TXBUFE;     // both current & next buffer
    //sp->uart->US_IER = AT91C_US_ENDTX;      // current buffer

    //TRACE_SER("WDMA WAIT\r\n");
    Semaphore_take(sp->txSem, -1); // wait until we get this back from the interrupt
    //TRACE_SER("WDMA OK\r\n");
    return CONTROLLER_OK;
}

int Serial_bytesAvailable( int channel )
{
    Serial_Internal* sp = &Serial_internals[ channel ];
    int n = Queue_msgsAvailable(sp->rxQueue);
    TRACE_SER("Serial_bytesAvailable %d %d\r\n", channel, n);
    return n;
}

bool Serial_anyBytesAvailable( int channel )
{
    Serial_Internal* sp = &Serial_internals[ channel ];
    return ((sp->uart->US_CSR & AT91C_US_RXRDY) != 0);
}

int Serial_read( int channel, char* data, int length, int timeout )
{
    Serial_Internal* sp = &Serial_internals[ channel ];

    TRACE_SER(">>>Serial_read %d %x %d\r\n", channel, data, length);
    // Do the business
    int count = 0;
    while ( count < length )
    {
        /* Place the character in the queue of characters to be transmitted. */ 
        if( Queue_receive(sp->rxQueue, data++, timeout ) == 0 )
            break;
        count++;
    }
    TRACE_SER("<<<\r\n");
    return count;
}

void Serial_DMARxStop(int channel) {
    Serial_Internal* sp = &Serial_internals[ channel ];

    //if (!sp->dma_stop_count++) {
    sp->uart->US_PTCR = AT91C_PDC_RXTDIS;
    //}
}

void Serial_DMARxStart(int channel) {
    Serial_Internal* sp = &Serial_internals[ channel ];

    //if (sp->dma_stop_count && --sp->dma_stop_count == 0) {
    sp->uart->US_PTCR = AT91C_PDC_RXTEN;
    //}
}

int Serial_waitForData( int channel, int timeout ) {
    Serial_Internal* sp = &Serial_internals[ channel ];

    TRACE_SER("waitForData %d\r\n", xTickCount);
    //sp->uart->US_IER = AT91C_US_RXBRK;

    Serial_DMARxStop(channel);
    //sp->rpr = sp->uart->US_RPR++;
    //sp->uart->US_RCR--;

    sp->uart->US_IER = AT91C_US_RXRDY; // one char
    // TODO: timeout
    Semaphore_take(sp->rxSem, timeout); // wait until we get this back from the interrupt
    //Serial_DMARxStart(channel);
    TRACE_SER("done %d\r\n", xTickCount);
}

int Serial_waitForDMARxData( int channel, int length, int timeout ) {
    Serial_Internal* sp = &Serial_internals[ channel ];

    char *rnpr_saved;
    int rncr_saved;

    Serial_DMARxStop(channel);

    int rcr = sp->uart->US_RCR;

    if (rcr < length) {
        Serial_DMARxStart(channel);
        return -1;
    }

    unsigned int rnpr = 0;

    if (rcr > length) {
        rnpr = sp->uart->US_RNPR;

        if (rnpr) {
            rnpr_saved = rnpr;
            rncr_saved = sp->uart->US_RNCR;
        }

        unsigned int rpr = sp->uart->US_RPR;
        sp->uart->US_RNPR = rpr + length;
        sp->uart->US_RNCR = rcr - length;

        sp->uart->US_RCR = length;
    }

    Serial_DMARxStart(channel);


    sp->uart->US_IER = AT91C_US_ENDRX; // current buffer
    // TODO: timeout
    Semaphore_take(sp->rxSem, -1); // wait until we get this back from the interrupt

    if (rnpr) {
        Serial_DMARxStop(channel);

        sp->uart->US_RNPR = rnpr_saved;
        sp->uart->US_RNCR = rncr_saved;

        Serial_DMARxStart(channel);
    }
    return 0;
}

static int _getAvailDMARxData( Serial_Internal* sp, char *last_addr, char *buff_top) {
    unsigned int rpr = sp->uart->US_RPR;

    // (TRACE doesn't work, too slow) TRACE_INFO("avail %x %x\r\n", rpr, last_addr);
    int avail = 0;
    if (rpr > last_addr) {
        avail = (char*)rpr - last_addr;
    } else if (rpr < last_addr) {
        avail = buff_top - last_addr;
    }
    return avail;
}

int Serial_waitForDMARxData2( int channel, char *buff, int buff_size, char *last_addr, int length, int timeout ) {
    Serial_Internal* sp = &Serial_internals[ channel ];

    char *rnpr_saved;
    int rncr_saved;
    char *buff_top = buff + buff_size;

    TRACE_SER("Serial_waitForDMARxData2 %d %x %d %d\r\n", channel, last_addr, buff_size, length);

    
    Serial_DMARxStop(channel);

    int avail = _getAvailDMARxData(sp, last_addr, buff_top);

    if (avail) {
        Serial_DMARxStart(channel); 
        TRACE_SER("Avail %d\r\n", avail);
        return avail;
    }

    int rcr = sp->uart->US_RCR;

    if (rcr < length) {
        Serial_DMARxStart(channel);
        return -1;
    }

    unsigned int rnpr = 0;

    if (rcr > length) {
        rnpr = sp->uart->US_RNPR;

        if (rnpr) {
            rnpr_saved = rnpr;
            rncr_saved = sp->uart->US_RNCR;
        }

        unsigned int rpr = sp->uart->US_RPR;
        sp->uart->US_RNPR = rpr + length;
        sp->uart->US_RNCR = rcr - length;

        sp->uart->US_RCR = length;
    }

    Serial_DMARxStart(channel);


    sp->uart->US_IER = AT91C_US_ENDRX; // current buffer
    // TODO: timeout
    Semaphore_take(sp->rxSem, -1); // wait until we get this back from the interrupt

    Serial_DMARxStop(channel);
    avail = _getAvailDMARxData(sp, last_addr, buff_top);

    if (rnpr) {

        sp->uart->US_RNPR = rnpr_saved;
        sp->uart->US_RNCR = rncr_saved;

    }
    Serial_DMARxStart(channel);
    return avail;
}

int Serial_waitForDMARxData3( int channel, char *buff, int buff_size, char *last_addr, int length, int timeout ) {
    Serial_Internal* sp = &Serial_internals[ channel ];

    char *buff_top = buff + buff_size;

    TRACE_SER("Serial_waitForDMARxData3 %d %x %d %d\r\n", channel, last_addr, buff_size, length);

    
    Serial_DMARxStop(channel);

    int avail = _getAvailDMARxData(sp, last_addr, buff_top);

    if (avail) {
        Serial_DMARxStart(channel); 
        TRACE_SER("Avail %d\r\n", avail);
        return avail;
    }

    int rcr = sp->uart->US_RCR;

    if (rcr < length) {
        Serial_DMARxStart(channel);
        return -1;
    }

    sp->uart->US_IER = AT91C_US_RXRDY; // one char
    Serial_DMARxStart(channel);

    // TODO: timeout
    Semaphore_take(sp->rxSem, -1); // wait until we get this back from the interrupt

    Serial_DMARxStop(channel);
    avail = _getAvailDMARxData(sp, last_addr, buff_top);

    Serial_DMARxStart(channel);
    return avail;
}

char* Serial_getDMARxBuff( int channel, int *count ) {
    Serial_Internal* sp = &Serial_internals[ channel ];
    
    *count = sp->uart->US_RCR;
    return (char*)sp->uart->US_RPR;
}

int Serial_setDMARxBuff( int channel, char *buff, int length, char *buff_next, int length_next ) {
    Serial_Internal* sp = &Serial_internals[ channel ];

    TRACE_SER("Serial_setDMARxBuff %d %x %d %x %d\r\n", channel, buff, length, buff_next, length_next);
    //Serial_DMARxStop(channel);

    if (buff) {
        sp->uart->US_RPR = (unsigned int)buff;
        sp->uart->US_RCR = length;
    } else if (length) {
        sp->uart->US_RCR += length;
    }

    if (buff_next) {
        sp->uart->US_RNPR = (unsigned int)buff_next;
        sp->uart->US_RNCR = length_next;
    }

    //Serial_DMARxStart(channel);

    return 0;
}

/*
int Serial_bytesAvailableDMA( int channel )
{
    Serial_Internal* sp = &Serial_internals[ channel ];
    int n = (char*)sp->uart->US_RCR - &sp->rxBuf[sp->rxPos];
    TRACE_SER("Serial_bytesAvailable %d %d\r\n", channel, n);
    return n;
}

int Serial_readDMA2( int channel, char* data, int length, int timeout )
{
    Serial_Internal* sp = &Serial_internals[ channel ];

    int avail = Serial_bytesAvailableDMA(channel);
    if (avail >= length) {
        memcpy(data, &sp->rxBuf[sp->rxPos], length);
        sp->rxPos += length;
        return length;
    }
    int data_offset = 0;
    int rem = length;
    while (rem) {
        int free = SERIAL_RX_BUF_SIZE - sp->rxPos;

        int chunk;
        if (rem > free)
            chunk = free;
        else 
            chunk = rem;

        int n = Serial_waitForDMARxData2(channel, sp->rxBuf, SERIAL_RX_BUF_SIZE, &sp->rxBuf[sp->rxPos], chunk, -1);
        if (n == chunk) {
                
        }

        memcpy(data + data_offset, &sp->rxBuf[sp->rxPos], n);
        data_offset += n;
        sp->rxPos += n;
        rem -= n;
    }
    return length;
}
*/

int Serial_readDMA( int channel, char* data, int length, int timeout )
{
    Serial_Internal* sp = &Serial_internals[ channel ];

    // Check if the first PDC bank is free
    if ((sp->uart->US_RCR == 0) && (sp->uart->US_RNCR == 0))
    {
        sp->uart->US_RPR = (unsigned int) data;
        sp->uart->US_RCR = length;
        sp->uart->US_PTCR = AT91C_PDC_RXTEN;
    }
    // Check if the second PDC bank is free
    else if (sp->uart->US_RNCR == 0)
    {
        sp->uart->US_RNPR = (unsigned int) data;
        sp->uart->US_RNCR = length;
    }
    else 
    {
        return CONTROLLER_ERROR_NO_SPACE;
    }

    if (timeout) {
        sp->uart->US_IER = AT91C_US_RXBUFF;  // both current & next buffer
        //sp->uart->US_IER = AT91C_US_ENDRX; // current buffer

        Semaphore_take(sp->rxSem, -1); // wait until we get this back from the interrupt
        return length;
    } else {   
        return 0;
    }
}

char Serial_readChar( int channel, int timeout )
{
    Serial_Internal* sp = &Serial_internals[ channel ];
    char c;
    // Do the business
    if( Queue_receive(sp->rxQueue, &c, timeout ) == 0 )
        return 0;
    else
        return c;
}

/**
  Clear out the serial port.
  Ensures that there are no bytes in the incoming buffer.

  \b Example
  \code
  Serial ser(1);
  ser.flush( ); // after starting up, make sure there's no junk in there
  \endcode
  */
void Serial_flush( int channel )
{
    while( Serial_bytesAvailable( channel ) )
        Serial_readChar( channel, 0 );
}

/**
  Reset the error flags in the serial system.
  In the normal course of operation, the serial system may experience
  a variety of different error modes, including buffer overruns, framing 
  and parity errors, and more.  You'll usually only want to call this
  after you've determined that errors exist with getErrors().
  If there aren't any errors, this has no effect.

  \b Example

  \code 
  Serial ser(1);
  if( ser.getErrors() )
  {
// handle errors...
ser.clearErors();
}
\endcode
*/
void Serial_clearErrors( int channel )
{
    Serial_Internal* sp = &Serial_internals[ channel ];
    if( sp->uart->US_CSR & (AT91C_US_OVRE | AT91C_US_FRAME | AT91C_US_PARE) )
        sp->uart->US_CR = AT91C_US_RSTSTA; // clear all errors
}

/**
  Read whether there are any errors.
  We can check for three kinds of errors in the serial system:
  - buffer overrun
  - framing error
  - parity error

  Each parameter will be set with a true or a false, given the current
  error state.  If you don't care to check one of the parameters, just
  pass in 0.

  @param overrun (optional) Will be set with the overrun error state.
  @param frame (optional) Will be set with the frame error state.
  @param parity (optional) Will be set with the parity error state.
  @return True if there were any errors, false if there were no errors.

  \b Example
  \code
  Serial ser(1);
  bool over, fr, par;
  if( ser.errors( &over, &fr, &par ) )
  {
// if we wanted, we could just clear them all right here with clearErrors()
// but here we'll check to see what kind of errors we got for the sake of the example
if(over)
{
// then we have an overrun error
}
if(fr)
{
// then we have a framing error
}
if(par)
{
// then we have a parity error
}
}
else
{
// there were no errors
}
\endcode
*/
bool Serial_errors( int channel, bool* overrun, bool* frame, bool* parity )
{
    bool retval = false;
    Serial_Internal* sp = &Serial_internals[ channel ];

    bool ovre = sp->uart->US_CSR & AT91C_US_OVRE;
    if(ovre)
        retval = true;
    if(overrun)
        *overrun = ovre;

    bool fr = sp->uart->US_CSR & AT91C_US_FRAME;
    if(fr)
        retval = true;
    if(frame)
        *frame = fr;

    bool par = sp->uart->US_CSR & AT91C_US_PARE;
    if(par)
        retval = true;
    if(parity)
        *parity = par;

    return retval;
}

/**
  Start the transmission of a break.
  This has no effect if a break is already in progress.

  \b Example
  \code 
  Serial ser(1);
  ser.startBreak();
  \endcode
  */
void Serial_startBreak( int channel )
{
    Serial_internals[ channel ].uart->US_CR = AT91C_US_STTBRK;
}

/**
  Stop the transmission of a break.
  This has no effect if there's not a break already in progress.

  \b Example
  \code
  Serial ser(1);
  ser.stopBreak();
  \endcode
  */
void Serial_stopBreak( int channel )
{
    Serial_internals[ channel ].uart->US_CR = AT91C_US_STPBRK;
}



