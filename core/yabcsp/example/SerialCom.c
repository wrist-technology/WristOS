/*******************************************************************************

				(C) COPYRIGHT Cambridge Silicon Radio

FILE
				SerialCom.h 

DESCRIPTION
				This module is a Win32 implementation of a serial communication module.
				The module is intended for use in combination with the ABCSP.

REVISION:		$Revision: 1.1.1.1 $ by $Author: ca01 $
*******************************************************************************/


#include <Windows.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>

#include "chw.h" /* for bool_t, uint8_t, uint16_t etc. */
#include "uSched.h"
#include "SerialCom.h"
#include "abcsp.h"

extern void BgIntPump(void);

#define	TX_BUF_MAX_SIZE				((uint16_t) 4000)	/* the buffer size for incoming and outgoing characters */
#define RX_BUF_MAX_SIZE				((uint16_t) 4000)

/* the following number can be used to limit the number of bytes send at the time to the YABCSP libray.
If not defined as many bytes as possible are send to the library */
/*#define	MAX_BYTES_TO_ABCSP			((uint16_t) 32)*/

#define CLOSE_DOWN_EVENT		0
#define	WROTE_DATA_EVENT		1
#define	NEW_TX_DATA_EVENT		2
#define	NO_OF_TX_EVENTS			3
#define	RX_DATA_EVENT			1
#define	DATA_READ_EVENT			2
#define	NO_OF_RX_EVENTS			3

/*	the UART file handler	*/
static HANDLE	comHandle;	

/*	internal events used to signal the thread	*/
static HANDLE	newTxDataEvent;
static HANDLE	txCloseDownEvent;
static HANDLE	txDownEvent;
static HANDLE	rxCloseDownEvent;
static HANDLE	rxDownEvent;
static HANDLE	dataReadEvent;
static HANDLE	rxEvents[NO_OF_RX_EVENTS];
static HANDLE	txEvents[NO_OF_TX_EVENTS];

/*	ensure mutual exclusion for write and read	*/
static CRITICAL_SECTION	txMutex;
static CRITICAL_SECTION	rxMutex;

/*	the threads for rx and tx	*/
static HANDLE txThread;
static HANDLE rxThread;

static uint16_t	txIn;
static uint16_t	txOut;
static uint16_t	rxIn;
static uint16_t	rxOut;
static uint16_t	txSize;
static uint16_t	rxSize;
static uint8_t	txBuf[TX_BUF_MAX_SIZE];
static uint8_t	rxBuf[RX_BUF_MAX_SIZE];

static unsigned long baudRate;
static char comPortString[128];

extern abcsp AbcspInstanceData;

void errorHandler(DWORD	line, char *file, char *text)
{
#ifdef DEBUG_ENABLE
	printf("Serial com error %s in file: %s, line %i\n", text, file, line);
#endif /*DEBUG_ENABLE*/
}

bool_t setComDefault(char *dcbInitString)
{
	/*	format of the init string must be: baud=1200 parity=N data=8 stop=1	*/
	bool_t			success;
	DCB				dcb;
	COMMTIMEOUTS	comTimeouts;

	success = TRUE;

    if(!GetCommState(comHandle, &dcb))
    {
		success = FALSE;
		errorHandler(__LINE__, __FILE__, "Can not read current DCB");
    }

	dcb.fOutxCtsFlow    = FALSE;
	dcb.fOutxDsrFlow    = FALSE;
	dcb.fDtrControl     = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = 0;
	dcb.fRtsControl     = RTS_CONTROL_DISABLE;

	/* then set baud rate, parity, data size and stop bit */
	if (!BuildCommDCB(dcbInitString, &dcb))
	{
		DWORD lastError;

		success = FALSE;
		lastError = GetLastError();
		printf("BuildCommDCB failed with error code: %ld!\n(device control string: %s)\n", lastError, dcbInitString);
	}

    comTimeouts.ReadIntervalTimeout = MAXDWORD;
    comTimeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    comTimeouts.ReadTotalTimeoutConstant = 300000;
    comTimeouts.WriteTotalTimeoutMultiplier = 0;
    comTimeouts.WriteTotalTimeoutConstant = 0;

	if (!SetCommMask(comHandle, 0))
	{
		success = FALSE;
		errorHandler(__LINE__, __FILE__, "SetCommMask failed during initialisation");
	}

	if(!SetCommTimeouts(comHandle, &comTimeouts))
	{
		success = FALSE;
		errorHandler(__LINE__, __FILE__, "SetCommTimeouts failed during initialisation");
	}

	if(!SetCommState(comHandle, &dcb))
	{
		success = FALSE;
		errorHandler(__LINE__, __FILE__, "SetCommState failed during initialisation");
	}

	if (success == FALSE)
	{
		CloseHandle(comHandle);
		comHandle = INVALID_HANDLE_VALUE;
	}

	return(success);
}

bool_t	handleRxData(DWORD len, uint8_t	*data)
{
	uint16_t	theRxSize;

	EnterCriticalSection(&rxMutex);
	theRxSize = rxSize;
	rxSize = rxSize + (uint16_t) len;
	LeaveCriticalSection(&rxMutex);

	if ((rxIn + len) == RX_BUF_MAX_SIZE)
	{
		rxIn = 0;
	}
	else
	{
		rxIn = rxIn + (uint16_t) len;
	}

	if (memchr(data, 0xC0, len) != NULL)
	{
		bg_int1();
	}
	return TRUE;
}

void txThreadFunc(void)
{
	OVERLAPPED	osWrote	= {0};
	HANDLE		wroteDataEvent;
	DWORD		bytesWritten;
	DWORD		event;
	bool_t		txBusy;

	osWrote.hEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	wroteDataEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	if ( (osWrote.hEvent == NULL) || (wroteDataEvent == NULL) )
	{
		errorHandler(__LINE__, __FILE__, "Create event failed in tx thread");
		exit(0);
	}

	txEvents[CLOSE_DOWN_EVENT]	= txCloseDownEvent;
	txEvents[WROTE_DATA_EVENT]	= osWrote.hEvent;
	txEvents[NEW_TX_DATA_EVENT]	= newTxDataEvent;

	txBusy = FALSE;

	while (TRUE)
	{
		event = WaitForMultipleObjects(NO_OF_TX_EVENTS, txEvents, FALSE, INFINITE);
		switch (event)
		{
			case WAIT_OBJECT_0 + CLOSE_DOWN_EVENT:
			{
				/*	to do	*/
				ResetEvent(txCloseDownEvent);
				break;
			}
			case WAIT_OBJECT_0 + WROTE_DATA_EVENT:
			{
				BOOL	success;

				ResetEvent(osWrote.hEvent);

				txBusy = FALSE;
				success = GetOverlappedResult(comHandle, &osWrote, &bytesWritten, FALSE);
				if (success)
				{
					if (bytesWritten > 0)
					{
						txOut = txOut + (uint16_t) bytesWritten;
						if (txOut >= TX_BUF_MAX_SIZE)
						{
							txOut = 0;
						}
					}
					else
					{
						bytesWritten = 0;
					}
					EnterCriticalSection(&txMutex);
					txSize = txSize - (uint16_t) bytesWritten;
					LeaveCriticalSection(&txMutex);
				}
				else
				{
					errorHandler(__LINE__, __FILE__, "Error when sending data (wrote complete error - data packet lost)");
				}

				if (txSize > 0)
				{
					SetEvent(newTxDataEvent);
				}
				break;
			}

			case WAIT_OBJECT_0 + NEW_TX_DATA_EVENT:
			{
				ResetEvent(newTxDataEvent);

				/*	do not try to send while busy	*/
				if (!txBusy)
				{
					uint16_t	size;
					uint16_t	no2Send;

					EnterCriticalSection(&txMutex);
					size = txSize;
					LeaveCriticalSection(&txMutex);

					if (size == 0)
					{
						/*	no data so take another loop	*/
						break;
					}

					no2Send = size;
					if (size + txOut > TX_BUF_MAX_SIZE)
					{
						no2Send = TX_BUF_MAX_SIZE - txOut;
					}

					if (WriteFile(comHandle, &(txBuf[txOut]), no2Send, &bytesWritten, &osWrote))
					{
						if (bytesWritten > 0)
						{
							txOut = txOut + (uint16_t) bytesWritten;
							if (txOut >= TX_BUF_MAX_SIZE)
							{
								txOut = 0;
							}
						}
						else
						{
							bytesWritten = 0;
						}
						EnterCriticalSection(&txMutex);
						txSize = txSize - (uint16_t) bytesWritten;
						LeaveCriticalSection(&txMutex);
						if (txSize > 0)
						{
							SetEvent(newTxDataEvent);
						}
					}
					else
					{
						if (GetLastError() != ERROR_IO_PENDING)
						{
							errorHandler(__LINE__, __FILE__, "Serious error in write file (new data event) in tx thread");
							break;
						}
						else
						{
							/*	write pending	*/
							txBusy = TRUE;
						}
					}
				}
				break;
			}

			default:
			{
				/*	error occured	*/
				errorHandler(__LINE__, __FILE__, "Default called");
				break;
			}
		}	/*	end switch	*/
		if (event == WAIT_OBJECT_0 + CLOSE_DOWN_EVENT)
		{
			break;
		}
	}	/*	end while	*/

	CloseHandle(osWrote.hEvent);
	CloseHandle(wroteDataEvent);

	SetEvent(txDownEvent);

	ExitThread(0);
}

void rxThreadFunc(void)
{
	OVERLAPPED	osRead	= {0};
	DWORD		event;
	uint8_t		*rxData;
	DWORD		bytesRead;
	bool_t		readSuccess;
	bool_t		startRead;
	uint16_t	theRxSize;
	COMSTAT		comStat;
	DWORD		errors;
	DWORD		numberToRead;


	osRead.hEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	if ( (osRead.hEvent == NULL) )
	{
		errorHandler(__LINE__, __FILE__, "Create event failed in rx thread");
		exit(0);
	}

	rxEvents[CLOSE_DOWN_EVENT]	= rxCloseDownEvent;
	rxEvents[RX_DATA_EVENT]		= osRead.hEvent;
	rxEvents[DATA_READ_EVENT]	= dataReadEvent;

	rxData = rxBuf;
	rxSize = 0;

	while (TRUE)
	{
		EnterCriticalSection(&rxMutex);
		theRxSize = rxSize;
		LeaveCriticalSection(&rxMutex);

		/*	find next position for rx data in and if any space available in buffer	*/
		if (theRxSize < RX_BUF_MAX_SIZE)
		{
			startRead = TRUE;
		}
		else
		{
			/*	wait for available space in the buffer	*/
			startRead = FALSE;
		}
		
		/*	kick the read process if allowed (space left in rx buffer)	*/
		if (startRead)
		{
			rxData = rxBuf + rxIn;

			/* find out how many to read */
			if (!ClearCommError(comHandle, &errors, &comStat))
			{
				/*	serious error	*/
				errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
			}
			numberToRead = comStat.cbInQue > 0 ? comStat.cbInQue : 1;
			if (rxIn + numberToRead > RX_BUF_MAX_SIZE)
			{
				numberToRead = RX_BUF_MAX_SIZE - rxIn;
			}

			readSuccess = ReadFile(comHandle, rxData, numberToRead, &bytesRead, &osRead);
			if (!readSuccess)
			{
				if (GetLastError() != ERROR_IO_PENDING)
				{
					/*	serious error	*/
					errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
				}
			}
			else
			{
				if (bytesRead > 0)
				{
					handleRxData(bytesRead, rxData);
				}
			}
		}

		if (!readSuccess)
		{
			do
			{
				event = WaitForMultipleObjects(NO_OF_RX_EVENTS, rxEvents, FALSE, INFINITE);
				switch (event)
				{
					case CLOSE_DOWN_EVENT:
					{
						/*	to do	*/
						ResetEvent(rxCloseDownEvent);
						break;
					}
					case RX_DATA_EVENT:
					{
						bool_t	success;

						/*	receive complete, store the data	*/
						success = GetOverlappedResult(comHandle, &osRead, &bytesRead, FALSE);

						if ((bytesRead > 0) && success)
						{
							handleRxData(bytesRead, rxData);
						}

						startRead = FALSE;
						ResetEvent(osRead.hEvent);
						break;
					}
					case DATA_READ_EVENT:
					{
						/* sufficient data has been read from the rx buffer to restart the reader thread */
						ResetEvent(dataReadEvent);
						break;
					}
					default:
					{
						/*	error occured	*/
						errorHandler(__LINE__, __FILE__, "Default called");
						break;
					}
				}
			} while (startRead && (event != CLOSE_DOWN_EVENT));

			if (event == CLOSE_DOWN_EVENT)
			{
				break;
			}
		}
	}	/*	end while	*/

	CloseHandle(osRead.hEvent);

	SetEvent(rxDownEvent);

	ExitThread(0);
}

void clearBuffer(void)
{
	/* flush the port for any operations waiting and any data	*/
	PurgeComm(comHandle, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/*	the internal queue pointers	*/
	txSize = 0;
	rxSize = 0;
	rxIn = 0;
	rxOut = 0;
	txIn = 0;
	txOut = 0;
}

static bool_t init(void)
{
	char configurationString[128];

	clearBuffer();

	/*	mutex for sync of write and read	*/
	InitializeCriticalSection(&rxMutex);
	InitializeCriticalSection(&txMutex);

	/*	event for internal communication	*/
	newTxDataEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	txCloseDownEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	txDownEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	rxCloseDownEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	rxDownEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	dataReadEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	if (newTxDataEvent == NULL || txCloseDownEvent == NULL || rxCloseDownEvent == NULL || txDownEvent == NULL || rxDownEvent == NULL || dataReadEvent == NULL)
	{
		errorHandler(__LINE__, __FILE__,"Create event failure in init");
		return FALSE;
	}

	/*	create the file handle	*/
	comHandle = CreateFile(comPortString, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
	if (comHandle == INVALID_HANDLE_VALUE)
	{
		errorHandler(__LINE__, __FILE__, "Create file handle failed");
		return FALSE;	
	}

	sprintf(configurationString, "baud=%lu parity=E data=8 stop=1", baudRate);
	if (!setComDefault(configurationString))
	{
		return FALSE;
	}

	return TRUE;
}


/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_Configure
* 
* DESCRIPTION
*
*   This function can be used to configure the UART driver.
*
*	If this function is not called, the UART may be started with the default
*	configuration.
*	If this function is called, it should be called before Uartdrv_Start is 
*	called to initialise the driver.
*
* RETURNS
*	TRUE if the configuration was successful.
*
*/
void UartDrv_Configure(unsigned long theBaudRate, char * theComPortString)
{
	baudRate = theBaudRate;
	strcpy(comPortString, theComPortString);
}


/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_Start
* 
* DESCRIPTION
*
*   This function is called to start the UART driver.
*
* RETURNS
*	TRUE: the UART was successfully started, otherwise FALSE
*
*******************************************************************************/
bool_t UartDrv_Start(void)
{
	DWORD	threadId;

	if (!init())
	{
		return FALSE;
	}

	/*	tx and rx threads		*/
	txThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) txThreadFunc, NULL, 0, &threadId);
	if(txThread == INVALID_HANDLE_VALUE) 
	{
		errorHandler(__LINE__, __FILE__, "Thread create failure");
		return(FALSE);
	}
	rxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) rxThreadFunc, NULL, 0, &threadId);
	if(rxThread == INVALID_HANDLE_VALUE) 
	{
		errorHandler(__LINE__, __FILE__, "Thread create failure");
		return(FALSE);
	}

	return TRUE;
}


/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_Stop
* 
* DESCRIPTION
*
*   This function is called to stop the UART driver.
*
* RETURNS
*   None
*
*/
void UartDrv_Stop(void)
{
	DWORD threadExitCode;

	SetEvent(txCloseDownEvent);
	SetEvent(rxCloseDownEvent);

	while (WaitForSingleObject(txDownEvent, INFINITE) != WAIT_OBJECT_0)
	{
		printf("WaitForSingleObject(txDownEvent, INFINITE) NOT SIGNALED????\n");
	}
	do
	{
		if (!GetExitCodeThread(txThread, &threadExitCode))
		{
			printf("GetExitCodeThread failed!\n");
			break;
		}
		else
		{
			if (threadExitCode == STILL_ACTIVE)
			{
				Sleep(0);
			}
		}
	} while (threadExitCode == STILL_ACTIVE);
	CloseHandle(newTxDataEvent);
	CloseHandle(txCloseDownEvent);
	CloseHandle(txDownEvent);
	CloseHandle(txThread);

	while (WaitForSingleObject(rxDownEvent, INFINITE) != WAIT_OBJECT_0)
	{
		printf("WaitForSingleObject(rxDownEvent, INFINITE) NOT SIGNALED????\n");
	}
	do
	{
		if (!GetExitCodeThread(rxThread, &threadExitCode))
		{
			printf("GetExitCodeThread failed!\n");
			break;
		}
		else
		{
			if (threadExitCode == STILL_ACTIVE)
			{
				Sleep(0);
			}
		}
	} while (threadExitCode == STILL_ACTIVE);
	CloseHandle(rxCloseDownEvent);
	CloseHandle(rxDownEvent);
	CloseHandle(dataReadEvent);
	CloseHandle(rxThread);

	CloseHandle(comHandle);
	
	DeleteCriticalSection(&rxMutex);
	DeleteCriticalSection(&txMutex);
}


/***********************************************************************
*	bool_t UartDrv_Tx(char *buf, uint16_t num_to_send, uint16_t *num_send)
*
*  FUNCTION
*	Takes the bytes from calling queue pointer and places the data in the 
*   queue. If the queue is full return an error flag
*	copy the data into a new buffer, i.e. the caller must free the data in 
*	the call 
*
*  RETURN
*	Return FALSE if Tx buffer overflow (and *num_send = 0).
*
*/
bool_t UartDrv_Tx(uint8_t *theData, unsigned len, uint16_t *numSend)
{
	uint16_t	size;

	*numSend = 0;
	EnterCriticalSection(&txMutex);
	size = txSize;
	LeaveCriticalSection(&txMutex);

	/* check if enough space in buffer for new data - if not return FALSE */
	if (len + size > TX_BUF_MAX_SIZE)
	{
		errorHandler(__LINE__, __FILE__, "buffer full in uart tx");
		return FALSE;
	}

	if (txIn + len > TX_BUF_MAX_SIZE)
	{
		uint16_t remainingLength;

		memcpy(&txBuf[txIn], theData, TX_BUF_MAX_SIZE - txIn);
		remainingLength = len - (TX_BUF_MAX_SIZE - txIn);
		memcpy(txBuf, &theData[TX_BUF_MAX_SIZE - txIn], remainingLength);
		txIn = remainingLength;
	}
	else
	{
		memcpy(&txBuf[txIn], theData, len);
		if (txIn + len == TX_BUF_MAX_SIZE)
		{
			txIn = 0;
		}
		else
		{
			txIn += (uint16_t) len;
		}
	}
	*numSend = len;

	EnterCriticalSection(&txMutex);
	txSize = txSize + (*numSend);
	LeaveCriticalSection(&txMutex);

	/*	signal tx thread that new data has arrived	*/
	SetEvent(newTxDataEvent);
	return TRUE;
}


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
uint16_t UartDrv_GetTxSpace(void)
{
	uint16_t	size;

	EnterCriticalSection(&txMutex);
	size = TX_BUF_MAX_SIZE - txSize;
	LeaveCriticalSection(&txMutex);

	return size;
}


/***********************************************************************
* NAME
*     UartDrv_GetRxAvailable
* 
* DESCRIPTION
*	Obtain the number of received characters available in UART.  Should be
*   able to read at least this number of chars in a call to UartDrv_Rx().
*
*	NOTE:	if there is a 'wrap around' only the number of bytes up to the limit can
*			be read but the number of bytes available may be higher
* 
* RETURNS
*	The number of characters that can be read from the UART.
*
*/
uint16_t UartDrv_GetRxAvailable(void)
{
	uint16_t	theSize;

	EnterCriticalSection(&rxMutex);
	theSize = rxSize;
	LeaveCriticalSection(&rxMutex);

	return theSize;
}


/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_Reset
* 
* DESCRIPTION
*
*   This function is called to reset the UART driver.  The transmit and
*   receive buffers are flushed.
*
*   NOTE: the UART configuration does NOT change.
*
* RETURNS
*   None
*
*/
void UartDrv_Reset(void)
{
	/*	clear any data in the in/out buffers	*/
	clearBuffer();
}

/*************************************************************************
	void UartDrv_Rx(void)

	Takes the bytes from the input queue ( max BytesToAbcsp numbers)
	send it to the ABCSP. ABCSP returns the number consumed this is used
	for updating the pointers / number of bytes in queue.

  RETURN
	None
*************************************************************************/
void UartDrv_Rx(void)
{
	unsigned	bytesConsumed;
	uint16_t	noOfBytes;

	EnterCriticalSection(&rxMutex);
	noOfBytes = rxSize;
	LeaveCriticalSection(&rxMutex);

#ifdef MAX_BYTES_TO_ABCSP
	if (noOfBytes > MAX_BYTES_TO_ABCSP)
	{
		noOfBytes = MAX_BYTES_TO_ABCSP;
	}
#endif

	if (noOfBytes > (RX_BUF_MAX_SIZE - rxOut))
	{
		noOfBytes = (RX_BUF_MAX_SIZE - rxOut);
	}
	bytesConsumed = abcsp_uart_deliverbytes(&AbcspInstanceData, &rxBuf[rxOut], noOfBytes);
	rxOut = rxOut + bytesConsumed;

	if (rxOut >= RX_BUF_MAX_SIZE)
	{
		rxOut = 0x00; /* Buffer start from beginning*/
	}

	EnterCriticalSection(&rxMutex);
	rxSize = rxSize - bytesConsumed;
	LeaveCriticalSection(&rxMutex);

	if (bytesConsumed > 0)
	{
		SetEvent(dataReadEvent);
	}

	if (rxSize)
	{
		bg_int1();
	}
}

/*----------------------------------------------------------------------------*
* NAME
*     UartDrv_RegisterHandlers
* 
* DESCRIPTION
*
*   This function is called to register bg_int for uart 0
*   and for uart 1. driver.
*
*	This function MUST be called before UartDrv_Start.
*
* RETURNS
*	None
*
*/
void UartDrv_RegisterHandlers(void)
{
	register_bg_int(1, UartDrv_Rx);
	register_bg_int(2, BgIntPump); /*The bg int needed for abcsp pumps*/
}
