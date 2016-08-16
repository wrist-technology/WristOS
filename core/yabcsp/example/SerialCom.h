#ifndef _SERIAL_COM_H    /* Once is enough */
#define _SERIAL_COM_H
/*******************************************************************************

				(C) COPYRIGHT Cambridge Silicon Radio

FILE
				SerialCom.h 

DESCRIPTION:
				Header file for the PC serial communication

REVISION:		$Revision: 1.1.1.1 $ by $Author: ca01 $
*******************************************************************************/


/*============================================================================*
Public Defines
*============================================================================*/
#include "chw.h"

typedef enum
{
	parityNone,
	parityEven,
	parityOdd
}
ParityMode_t;

typedef struct
{
	ParityMode_t	parityMode;
	uint16_t		baudRate;
	uint8_t			byteSize;
	uint16_t		stopBits;
}
uartConfig_t;

typedef struct
{
	char			comPortName[10];
	uartConfig_t	uartConfig;
}
ComConfig_t;

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_Configure
* 
* DESCRIPTION
*
*   This function is called to initialise the UART driver.
*	It is passed a pointer to a structure defining the UART configuration.
*
*	If this function is not called, the UART may be started with the default
*	configuration.
*	If this function is called, it should be called before Uartdrv_Start is 
*	called to initialise the driver.
*
*   To change the configuration of the UART, use the following sequence:
*       UartDrv_Stop();
*       UartDrv_Configure(new_configuration);
*       UartDrv_Start();
* 
* RETURNS
*	None
*
*/
void UartDrv_Configure(unsigned long theBaudRate, char * theComPortString);


/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_RegisterHandlers
* 
* DESCRIPTION
*
*   This function is used to initialise the UART driver. 
*	MUST be called before start.
* 
* RETURNS
*	None
*
*/
void UartDrv_RegisterHandlers(void);


/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_Start
* 
* DESCRIPTION
*
*   This function is called to start (open) the UART driver.
*	Comms over the UART is started using the configuration previously 
*	provided.
*
*	This function MUST be called AFTER calling UartDrv_Configure.
*
*   Errors with the configuration are indicated by this function (rather
*   than UartDrv_Configure) because these errors are more likely to be
*   found when opening/starting the UART driver.
*   If detailed information about configuration errors is available eg.
*   which part of the configuration caused the error, it should be reported
*   either using STRLOG or to the error handler.
*
* RETURNS
*	TRUE if the UART was successfully started with the configuration
*   previously provided by UartDrv_Configure;
*   FALSE if there was an error with part(s) of the previously provided
*   configuration. In this case, the state of the UART will be the same
*   as before this function was called.
*
*/
bool_t UartDrv_Start(void);


/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_Stop
* 
* DESCRIPTION
*
*   This function is called to stop (close) the UART driver.  It un-does
*   all actions performed by UartDrv_Start eg. frees any allocated memory
*
*   To change the configuration of the UART, use the following sequence:
*       UartDrv_Stop();
*       UartDrv_Configure(new_configuration);
*       UartDrv_Start();
*
* RETURNS
*   None
*
*/
void UartDrv_Stop(void);

/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_Reset
* 
* DESCRIPTION
*
*   This function is called to reset the UART driver.  The transmit and
*   receive buffers are flushed, and the "transmit complete" handler
*   function is triggered.
*
*   NOTE: the UART configuration does NOT change.
*
* RETURNS
*   None
*
*/
void UartDrv_Reset(void);

/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_GetTxSpace
* 
* DESCRIPTION
*	Obtain amount of space available in UART.  Should be able to write at
*	least this number of chars in a call to UartDrv_Tx().
* 
* RETURNS
*	The number of characters that can be written to the UART.
*
*/
uint16_t UartDrv_GetTxSpace(void);

/***********************************************************************
*	bool_t UartDrv_Tx(char *buf, uint16_t num_to_send, uint16_t *num_send)
*
*  FUNCTION
*	Takes the bytes from calling queue pointer and places the data in the 
*    queue. If the queue is full return an error flag
*
*  RETURN
*	Return FALSE if Tx buffer overflows
*
*/
bool_t UartDrv_Tx(uint8_t *theData, unsigned len, uint16_t *numSend);

/***********************************************************************
* NAME
*     UartDrv_GetRxAvailable
* 
* DESCRIPTION
*	Obtain the number of received characters available in UART.  Should be
*   able to read at least this number of chars in a call to UartDrv_Rx().
* 
* RETURNS
*	The number of characters that can be read from the UART.
*
*/
uint16_t UartDrv_GetRxAvailable(void);

/***********************************************************************
	void UartDrv_Rx(void)

  FUNCTION (a bg_int function )
	Takes the bytes from the input queue ( max BytesToAbcsp numbers)
	send it to the ABCSP. ABCSP returns the number consumed this is used
	for updating the pointers / number of bytes in queue.

RETURN
	NON
*/
void UartDrv_Rx(void);

/********************************************************************
	void UartDrv_ReadRx(void)

	Takes the bytes from the input queue.
	To avoid copying, not all chars is necessarily returned. The caller must
	check this on return and maybe issue another read.

  RETURN
	TRUE if any data transfered.
	Return false if no data in buffer; charsRead is invalid in this case.
********************************************************************/
bool_t	UartDrv_ReadRx(uint16_t *noOfChars2Read, uint8_t **charsRead);


#ifdef __cplusplus
}
#endif


#endif /* ndef _SERIAL_COM_H */
