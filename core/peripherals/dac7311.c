#include "dac7311.h"
#include "ssc.h"
#include "board/hardware_conf.h"

/*
   Dynawa DAC
   musi se tam ale zapnout i ten zesilovac  LED2 = MUTE  AUDIOSD=shutdown

   AUDIOSD = PA25
   LED2 = PA0
   */

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
/// \internal Calculates the SSC Transmit Clock Mode Register value given the
/// desired sample size (in bytes).
/*
#define AT73C213_TCMR(sampleSize, numChannels) \
           (AT91C_SSC_CKS_DIV \
            | AT91C_SSC_CKO_DATA_TX \
            | AT91C_SSC_START_FALL_RF \
            | SSC_STTDLY(1) \
            | SSC_PERIOD(sampleSize * 8 * numChannels))
*/

// ok (1)
#define _DAC7311_TCMR(sampleSize) \
    (AT91C_SSC_CKS_DIV \
     | AT91C_SSC_CKO_CONTINOUS \
     | AT91C_SSC_CKI \
     | SSC_STTDLY(0) \
     | AT91C_SSC_START_CONTINOUS)

// test (ok) ?AT91C_SSC_CKI
#define DAC7311_TCMR(sampleSize) \
    (AT91C_SSC_CKS_DIV \
     | AT91C_SSC_CKO_CONTINOUS \
     | AT91C_SSC_CKI \
     | SSC_STTDLY(0) \
     | AT91C_SSC_START_FALL_RF \
     | SSC_PERIOD(sampleSize * 8 + 2))

// test (ok)
#define _DAC7311_TCMR(sampleSize) \
    (AT91C_SSC_CKS_DIV \
     | AT91C_SSC_CKO_CONTINOUS \
     | AT91C_SSC_CKI \
     | SSC_STTDLY(0) \
     | AT91C_SSC_START_FALL_RF \
     | SSC_PERIOD(sampleSize * 8 + 2))

// test (nok)
#define _DAC7311_TCMR(sampleSize) \
    (AT91C_SSC_CKS_DIV \
     | AT91C_SSC_CKO_CONTINOUS \
     | SSC_STTDLY(1) \
     | AT91C_SSC_START_FALL_RF)


/// \internal Calculates the SSC Transmit Frame Mode Register value given the
/// desired sample size (in bytes).
/*
#define AT73C213_TFMR(sampleSize, numChannels) \
           (SSC_DATLEN(sampleSize * 8) \
            | AT91C_SSC_MSBF \
            | SSC_DATNB(numChannels) \
            | SSC_FSLEN(sampleSize * 8) \
            | AT91C_SSC_FSOS_NEGATIVE)
*/

// ok (1)
#define _DAC7311_TFMR(sampleSize) \
    (SSC_DATLEN(sampleSize * 8) \
     | AT91C_SSC_MSBF \
     | SSC_DATNB(1) \
     | AT91C_SSC_FSOS_LOW)

// test (ok)
#define DAC7311_TFMR(sampleSize) \
    (SSC_DATLEN(sampleSize * 8) \
     | AT91C_SSC_MSBF \
     | SSC_DATNB(1) \
     | SSC_FSLEN(sampleSize * 8) \
     | AT91C_SSC_FSOS_NEGATIVE)

// test (nok)
#define _DAC7311_TFMR(sampleSize) \
    (SSC_DATLEN(sampleSize * 8) \
     | AT91C_SSC_MSBF \
     | SSC_DATNB(1) \
     | AT91C_SSC_FSOS_LOW)

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Enables the DAC7311 DAC to operate using the specified sample frequency (in
/// Hz) and sample size (in bytes). 
/// \param Fs  Desired sampling frequency in Hz.
/// \param sampleSize  Size of one sample in bytes.
/// \param masterClock  Frequency of the system master clock.
//------------------------------------------------------------------------------
void DAC7311_Enable(unsigned int  Fs,
        unsigned int  sampleSize,
        unsigned int  masterClock)
{
/*
    AT91C_BASE_PIOA->PIO_PDR = AT91C_PA15_TF | AT91C_PA16_TK | AT91C_PA17_TD; 
    AT91C_BASE_PIOA->PIO_ASR = AT91C_PA15_TF | AT91C_PA16_TK | AT91C_PA17_TD; 

    DAC7311_PIO_BASE->PIO_PER = DAC7311_PIN_AUDIOSD;
    DAC7311_PIO_BASE->PIO_OER = DAC7311_PIN_AUDIOSD;
    DAC7311_PIO_BASE->PIO_SODR = DAC7311_PIN_AUDIOSD;

    DAC7311_PIO_BASE->PIO_PER = DAC7311_PIN_MUTE;
    DAC7311_PIO_BASE->PIO_OER = DAC7311_PIN_MUTE;
*/
    DAC7311_PIO_BASE->PIO_SODR = DAC7311_PIN_AUDIOSD;

    // Unmute channels
    DAC7311_SetMuteStatus(0);

    SSC_Open(BOARD_DAC7311_SSC, BOARD_DAC7311_SSC_ID);

    // Configure the SSC
    SSC_Configure(BOARD_DAC7311_SSC,
            BOARD_DAC7311_SSC_ID,
            //Fs * sampleSize * 8,
            Fs * (sampleSize * 8 + 2),
            masterClock);
    SSC_ConfigureTransmitter(BOARD_DAC7311_SSC,
            DAC7311_TCMR(sampleSize),
            DAC7311_TFMR(sampleSize));
    SSC_EnableTransmitter(BOARD_DAC7311_SSC);
}

//------------------------------------------------------------------------------
/// Disables the DAC7311 DAC.
//------------------------------------------------------------------------------
void DAC7311_Disable()
{
    SSC_Close(BOARD_DAC7311_SSC, BOARD_DAC7311_SSC_ID);
    DAC7311_PIO_BASE->PIO_CODR = DAC7311_PIN_AUDIOSD;
}

//------------------------------------------------------------------------------
/// Mutes or unmutes the left and/or right channel.
/// \param muted  Indicates the new status of the audio channel.
//------------------------------------------------------------------------------
void DAC7311_SetMuteStatus(unsigned char muted)
{
    // Update channel status
    if (muted) {
        DAC7311_PIO_BASE->PIO_CODR = DAC7311_PIN_MUTE;
    }
    else {
        DAC7311_PIO_BASE->PIO_SODR = DAC7311_PIN_MUTE;
    }
}

void DAC7311_Init() {
    AT91C_BASE_PIOA->PIO_PDR = AT91C_PA15_TF | AT91C_PA16_TK | AT91C_PA17_TD; 
    AT91C_BASE_PIOA->PIO_ASR = AT91C_PA15_TF | AT91C_PA16_TK | AT91C_PA17_TD; 

    DAC7311_PIO_BASE->PIO_PER = DAC7311_PIN_AUDIOSD;
    DAC7311_PIO_BASE->PIO_OER = DAC7311_PIN_AUDIOSD;
    DAC7311_PIO_BASE->PIO_CODR = DAC7311_PIN_AUDIOSD;

    DAC7311_PIO_BASE->PIO_PER = DAC7311_PIN_MUTE;
    DAC7311_PIO_BASE->PIO_OER = DAC7311_PIN_MUTE;
    DAC7311_PIO_BASE->PIO_CODR = DAC7311_PIN_MUTE;
}
