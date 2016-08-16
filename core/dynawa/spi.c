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

#include "spi.h"
#include "error.h"
//#include "AT91SAM7X256.h"
#include "hardware_conf.h"
#include "AT91SAM7SE512.h"


// Define the SPI select lines 
// and which peripheral they are on that line
#if ( CONTROLLER_VERSION == 50 )
#define SPI_SEL0_IO           IO_PA12
#define SPI_SEL0_PERIPHERAL_A 1 
#define SPI_SEL1_IO           IO_PA13
#define SPI_SEL1_PERIPHERAL_A 1 
#define SPI_SEL2_IO           IO_PA08
#define SPI_SEL2_PERIPHERAL_A 0 
#define SPI_SEL3_IO           IO_PA09
#define SPI_SEL3_PERIPHERAL_A 0
#elif ( CONTROLLER_VERSION == 90 )
#define SPI_SEL0_IO           IO_PA12
#define SPI_SEL0_PERIPHERAL_A 1 
#define SPI_SEL1_IO           IO_PA13
#define SPI_SEL1_PERIPHERAL_A 1 
#define SPI_SEL2_IO           IO_PB14
#define SPI_SEL2_PERIPHERAL_A 0 
#define SPI_SEL3_IO           IO_PB17
#define SPI_SEL3_PERIPHERAL_A 0
#elif ( CONTROLLER_VERSION >= 95 )
#define SPI_SEL0_IO           IO_PA12
#define SPI_SEL0_PERIPHERAL_A 1 
#define SPI_SEL1_IO           IO_PA13
#define SPI_SEL1_PERIPHERAL_A 1 
#define SPI_SEL2_IO           IO_PA08
#define SPI_SEL2_PERIPHERAL_A 0 
#define SPI_SEL3_IO           IO_PA09
#define SPI_SEL3_PERIPHERAL_A 0
#endif

// Dynawa
// Accelerometer
#define SPI_SEL0_IO           IO_PA11
#define SPI_SEL0_PERIPHERAL_A 1 
// SD Card
#define SPI_SEL1_IO           IO_PA31
#define SPI_SEL1_PERIPHERAL_A 1 
// ?
#define SPI_SEL2_IO           IO_PA08
#define SPI_SEL2_PERIPHERAL_A 0 
// ?
#define SPI_SEL3_IO           IO_PA09
#define SPI_SEL3_PERIPHERAL_A 0

// static
int spi_refcount = 0;

void Spi_open( Spi *spi, int channel ) {
    if( channel < 0 || channel > 3 )
        return;

    spi->_lock = Semaphore_create();

    spi->_channel = channel;
    if(spi_refcount++ == 0)
        Spi_init();

    Io_Peripheral io_type = Spi_getChannelPeripheralA( channel ) ? IO_A : IO_B;
    Io_init(&spi->chan, Spi_getIO(channel), io_type, OUTPUT );
}

void Spi_init( )
{
    // Reset it
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SWRST;

    // Must confirm the peripheral clock is running
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SPI;

    // DON'T USE FDIV FLAG - it makes the SPI unit fail!!

    AT91C_BASE_SPI->SPI_MR = 
        AT91C_SPI_MSTR | // Select the master
        AT91C_SPI_PS_VARIABLE |
        AT91C_SPI_PCS | // Variable Addressing - no address here
        // AT91C_SPI_PCSDEC | // Select address decode
        // AT91C_SPI_FDIV | // Select Master Clock / 32 - PS DON'T EVER SET THIS>>>>  SAM7 BUG
        AT91C_SPI_MODFDIS | // Disable fault detect
        // AT91C_SPI_LLB | // Enable loop back test
        //MV( ( 0x0 << 24 ) & AT91C_SPI_DLYBCS ) ;  // Delay between chip selects
        ( ( 20 << 24 ) & AT91C_SPI_DLYBCS ) ;  // Delay between chip selects

    AT91C_BASE_SPI->SPI_IDR = 0x3FF; // All interupts are off

/*
    // Set up the IO lines for the peripheral
    // Disable their peripherality
    AT91C_BASE_PIOA->PIO_PDR = 
        AT91C_PA16_SPI_MISO | 
        AT91C_PA17_SPI_MOSI | 
        AT91C_PA18_SPI_SPCK;

    // Kill the pull up on the Input
    AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA16_SPI_MISO;

    // Make sure the input isn't an output
    AT91C_BASE_PIOA->PIO_ODR = AT91C_PA16_SPI_MISO;

    // Select the correct Devices
    AT91C_BASE_PIOA->PIO_ASR = 
        AT91C_PA16_SPI_MISO | 
        AT91C_PA17_SPI_MOSI | 
        AT91C_PA18_SPI_SPCK;
*/

    // Set up the IO lines for the peripheral
    // Disable their peripherality
    AT91C_BASE_PIOA->PIO_PDR = 
        SPI_PIO_MISO | 
        SPI_PIO_MOSI | 
        SPI_PIO_SPCK |
        SPI_PIO_NPCS;

    AT91C_BASE_PIOA->PIO_OER = 
        SPI_PIO_MOSI | 
        SPI_PIO_SPCK |
        SPI_PIO_NPCS;

    // Kill the pull up on the Input
    AT91C_BASE_PIOA->PIO_PPUDR = SPI_PIO_MISO;

    // Make sure the input isn't an output
    AT91C_BASE_PIOA->PIO_ODR = SPI_PIO_MISO;

    // Select the correct Devices
    AT91C_BASE_PIOA->PIO_ASR = 
        SPI_PIO_MISO | 
        SPI_PIO_MOSI | 
        SPI_PIO_SPCK |
        SPI_PIO_NPCS;

    // Elsewhere need to do this for the select lines
    // AT91C_BASE_PIOB->PIO_BSR = 
    //    AT91C_PB17_NPCS03; 

    // Fire it up
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SPIEN;
    return;
}

void Spi_close ( Spi *spi)
{
    Semaphore_delete(spi->_lock);

    if(--spi_refcount <= 0)
        AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SPIDIS;
}

int Spi_configure( Spi *spi, int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers )
{
    if( !Spi_valid(spi))
        return 0;
    // Check parameters
    if ( bits < 8 || bits > 16 )
        return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

    if ( clockDivider < 0 || clockDivider > 255 )
        return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

    if ( delayBeforeSPCK < 0 || delayBeforeSPCK > 255 )
        return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

    if ( delayBetweenTransfers < 0 || delayBetweenTransfers > 255 )
        return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;

    // Set the values
    AT91C_BASE_SPI->SPI_CSR[ spi->_channel ] = 
        AT91C_SPI_NCPHA | // Clock Phase TRUE

// MV
        //AT91C_SPI_CPOL |


        ( ( ( bits - 8 ) << 4 ) & AT91C_SPI_BITS ) | // Transfer bits
        ( ( clockDivider << 8 ) & AT91C_SPI_SCBR ) | // Serial Clock Baud Rate Divider (255 = slow)
        ( ( delayBeforeSPCK << 16 ) & AT91C_SPI_DLYBS ) | // Delay before SPCK
        ( ( delayBetweenTransfers << 24 ) & AT91C_SPI_DLYBCT ); // Delay between transfers

    return CONTROLLER_OK;
}


int Spi_readWriteBlock( Spi *spi, unsigned char* buffer, int count )
{
    if( !Spi_valid(spi))
        return 0;
    int r;
    int address = ~( 1 << spi->_channel );

    if ( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY ) ) // Make sure the unit is at rest before we re-begin
    {
        while( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY ) );
        while( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF ) )
            r = AT91C_BASE_SPI->SPI_SR;
        r = AT91C_BASE_SPI->SPI_RDR;
    }

    if ( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF )
        r = AT91C_BASE_SPI->SPI_RDR;

    //AT91C_BASE_SPI->SPI_CSR[ spi->_channel ] |= AT91C_SPI_CSAAT; // Make the CS line hang around

    int writeIndex = 0;
    unsigned char* writeP = buffer;
    unsigned char* readP = buffer;

    while ( writeIndex < count ) // Do the read write
    {
        writeIndex++;
        AT91C_BASE_SPI->SPI_TDR = ( *writeP++ & 0xFF ) | 
            ( ( address << 16 ) &  AT91C_SPI_TPCS ) | 
            (int)( ( writeIndex == count ) ? AT91C_SPI_LASTXFER : 0 );

        while ( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF ) );
        *readP++ = (unsigned char)( AT91C_BASE_SPI->SPI_RDR & 0xFF );
    }

    //AT91C_BASE_SPI->SPI_CSR[ spi->_channel ] &= ~AT91C_SPI_CSAAT;

    return 0;
}
int Spi_readWriteBlock16( Spi *spi, uint16_t *buffer, int count )
{
    if( !Spi_valid(spi))
        return 0;
    int r;
    int address = ~( 1 << spi->_channel );

    if ( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY ) ) // Make sure the unit is at rest before we re-begin
    {
        while( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY ) );
        while( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF ) )
            r = AT91C_BASE_SPI->SPI_SR;
        r = AT91C_BASE_SPI->SPI_RDR;
    }

    if ( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF )
        r = AT91C_BASE_SPI->SPI_RDR;

    //AT91C_BASE_SPI->SPI_CSR[ spi->_channel ] |= AT91C_SPI_CSAAT; // Make the CS line hang around

    int writeIndex = 0;
    uint16_t* writeP = buffer;
    uint16_t* readP = buffer;

    while ( writeIndex < count ) // Do the read write
    {
        writeIndex++;
        AT91C_BASE_SPI->SPI_TDR = ( *writeP++ & 0xFFFF ) | 
            ( ( address << 16 ) &  AT91C_SPI_TPCS ) | 
            (int)( ( writeIndex == count ) ? AT91C_SPI_LASTXFER : 0 );

        while ( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF ) );
        *readP++ = (uint16_t)( AT91C_BASE_SPI->SPI_RDR & 0xFFFF );
    }

    //AT91C_BASE_SPI->SPI_CSR[ spi->_channel ] &= ~AT91C_SPI_CSAAT;

    return 0;
}

void Spi_lock(Spi *spi) {
    Semaphore_take(spi->_lock, -1);
}

void Spi_unlock(Spi *spi) {
    Semaphore_give(spi->_lock);
}

bool Spi_valid( Spi *spi ) {
    //return spi->chan != NULL;
    return true;
}

int Spi_getIO( int channel )
{
    switch ( channel )
    {
        case 0: return SPI_SEL0_IO;
        case 1: return SPI_SEL1_IO;
        case 2: return SPI_SEL2_IO;
        case 3: return SPI_SEL3_IO;
        default: return 0;
    }
}

int Spi_getChannelPeripheralA( int channel )
{  
    switch ( channel )
    {
        case 0: return SPI_SEL0_PERIPHERAL_A;
        case 1: return SPI_SEL1_PERIPHERAL_A;
        case 2: return SPI_SEL2_PERIPHERAL_A;
        case 3: return SPI_SEL3_PERIPHERAL_A;
        default: return -1;
    }
}
