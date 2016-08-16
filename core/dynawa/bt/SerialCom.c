/*******************************************************************************

				(C) COPYRIGHT Cambridge Silicon Radio

FILE
				SerialCom.h 

DESCRIPTION
				This module is a Win32 implementation of a serial communication module.
				The module is intended for use in combination with the ABCSP.

REVISION:		$Revision: 1.1.1.1 $ by $Author: ca01 $
*******************************************************************************/


//#include <Windows.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>

#include "chw.h" /* for bool_t, uint8_t, uint16_t etc. */
#include "uSched.h"
#include "SerialCom.h"
#include "abcsp.h"
#include "debug/trace.h"

//extern volatile portTickType xTickCount;

#define SERIAL_CHANNEL 0

extern void BgIntPump(void);

#define BT_DATA_POLL_INTERVAL    10

#define	TX_BUF_MAX_SIZE				((uint16_t) 4096)	/* the buffer size for incoming and outgoing characters */
#define RX_BUF_MAX_SIZE				((uint16_t) 4096)
//#define RX_BUF_MAX_SIZE				((uint16_t) 1024)

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
//static HANDLE	comHandle;	

/*	internal events used to signal the thread	*/
/*
static HANDLE	newTxDataEvent;
static HANDLE	txCloseDownEvent;
static HANDLE	txDownEvent;
static HANDLE	rxCloseDownEvent;
static HANDLE	rxDownEvent;
static HANDLE	dataReadEvent;
static HANDLE	rxEvents[NO_OF_RX_EVENTS];
static HANDLE	txEvents[NO_OF_TX_EVENTS];
*/
static xQueueHandle txEvents;
static xQueueHandle rxEvents;

/*	ensure mutual exclusion for write and read	*/
//static CRITICAL_SECTION	txMutex;
static xSemaphoreHandle txMutex;
//static CRITICAL_SECTION	rxMutex;
static xSemaphoreHandle rxMutex;

/*	the threads for rx and tx	*/
//static HANDLE txThread;
static xTaskHandle txThread;
//static HANDLE rxThread;
static xTaskHandle rxThread;

static uint16_t	txIn;
static uint16_t	txOut;
static uint16_t	rxIn;
static uint16_t	rxOut;
static uint16_t	txSize;
static uint16_t	rxSize;
static uint8_t	txBuf[TX_BUF_MAX_SIZE];
static uint8_t	rxBuf[RX_BUF_MAX_SIZE];
// MV Test - rxBuf in SRAM (32KB)
//static uint8_t *rxBuf = 0x8000 - RX_BUF_MAX_SIZE;

static unsigned long baudRate;
static char comPortString[128];

extern abcsp AbcspInstanceData;

static unsigned int last_activity_ticks = 0; 

void errorHandler(uint16_t	line, char *file, char *text)
{
#ifdef DEBUG_ENABLE
	printf("Serial com error %s in file: %s, line %i\n", text, file, line);
#endif /*DEBUG_ENABLE*/
    TRACE_ERROR("Serial com error %s in file: %s, line %i\r\n", text, file, line);
}

bool_t setComDefault(char *dcbInitString)
{
	/*	format of the init string must be: baud=1200 parity=N data=8 stop=1	*/
    return TRUE;
}

bool_t	handleRxData(uint16_t len, uint8_t	*data)
{
	uint16_t	theRxSize;

    //TRACE_BT("handleRxData %d %x %d %d\r\n", len, rxBuf, rxIn, rxSize);
	//EnterCriticalSection(&rxMutex);
    xSemaphoreTake(rxMutex, portMAX_DELAY);
	theRxSize = rxSize;
	rxSize = rxSize + (uint16_t) len;
	//LeaveCriticalSection(&rxMutex);
    xSemaphoreGive(rxMutex);

	if ((rxIn + len) == RX_BUF_MAX_SIZE)
	{
		rxIn = 0;
	}
	else
	{
		rxIn = rxIn + (uint16_t) len;
	}
    //TRACE_BT("handleRxData %d %d\r\n", rxIn, rxSize);

	if (memchr(data, 0xC0, len) != NULL)  // BCSP delimiter character (0xC0) received
	{
		bg_int1();
	}
	return TRUE;
}

void txThreadFunc(void)
{
	//OVERLAPPED	osWrote	= {0};
	//HANDLE		wroteDataEvent;
	uint16_t		bytesWritten;
	uint16_t		event;
	bool_t		txBusy;

/*
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
*/

	txBusy = FALSE;

    TRACE_INFO("txThreadFunc %x\r\n", xTaskGetCurrentTaskHandle());
	while (TRUE)
	{
		//event = WaitForMultipleObjects(NO_OF_TX_EVENTS, txEvents, FALSE, INFINITE);
        //TRACE_BT("txevent wait\r\n");
        //TRACE_INFO("txevent wait %d\r\n", xTickCount);
        //TRACE_INFO("txevent wait %d\r\n", Timer_tick_count());
        xQueueReceive(txEvents, &event, portMAX_DELAY); 
        //TRACE_BT("txevent\r\n");
        //event = NEW_TX_DATA_EVENT;
		switch (event)
		{
			//case WAIT_OBJECT_0 + CLOSE_DOWN_EVENT:
			case CLOSE_DOWN_EVENT:
			{
				/*	to do	*/
				//ResetEvent(txCloseDownEvent);
				break;
			}

			//case WAIT_OBJECT_0 + NEW_TX_DATA_EVENT:
			case NEW_TX_DATA_EVENT:
			{
				//ResetEvent(newTxDataEvent);

				/*	do not try to send while busy	*/
				if (!txBusy)
				{
                    bool keep_writing;
                    do {
                        keep_writing = FALSE;
                        uint16_t	size;
                        uint16_t	no2Send;

                        //EnterCriticalSection(&txMutex);
                        xSemaphoreTake(txMutex, portMAX_DELAY);
                        size = txSize;
                        //LeaveCriticalSection(&txMutex);
                        xSemaphoreGive(txMutex);

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

                        //if (!Serial_write(SERIAL_CHANNEL, &(txBuf[txOut]), no2Send, -1))
                        last_activity_ticks = Timer_tick_count();
                        if (!Serial_writeDMA(SERIAL_CHANNEL, &(txBuf[txOut]), no2Send, -1))
                        {
                            //TRACE_BT("data written\r\n");
                            //TRACE_INFO("data written\r\n");
                            bytesWritten = no2Send;
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
                            //EnterCriticalSection(&txMutex);
                            xSemaphoreTake(txMutex, portMAX_DELAY);
                            txSize = txSize - (uint16_t) bytesWritten;
                            //LeaveCriticalSection(&txMutex);
                            xSemaphoreGive(txMutex);
                            if (txSize > 0)
                            {
                                keep_writing = TRUE;
                                //SetEvent(newTxDataEvent);
                            }
                        }
                        else
                        {
    /*
                            if (GetLastError() != ERROR_IO_PENDING)
                            {
                                errorHandler(__LINE__, __FILE__, "Serious error in write file (new data event) in tx thread");
                                break;
                            }
                            else
                            {
                                txBusy = TRUE;
                            }
    */
                            errorHandler(__LINE__, __FILE__, "Serious error in write file (new data event) in tx thread");
                        }
                    } while(keep_writing);
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
		//if (event == WAIT_OBJECT_0 + CLOSE_DOWN_EVENT)
		if (event == CLOSE_DOWN_EVENT)
		{
			break;
		}
	}	/*	end while	*/

	//CloseHandle(osWrote.hEvent);
	//CloseHandle(wroteDataEvent);

	//SetEvent(txDownEvent);

	//ExitThread(0);
    vTaskDelete(NULL);
}

void rxThreadFunc_last_working(void)
//void rxThreadFunc(void)
{
	uint16_t		event;
	uint8_t		*rxData;
	uint16_t		bytesRead;
	uint16_t	theRxSize;
	//COMSTAT		comStat;
	uint16_t		errors;
	uint16_t		numberToRead;


	rxData = rxBuf;
	rxSize = 0;

    TRACE_INFO("rxThreadFunc %x\r\n", xTaskGetCurrentTaskHandle());
	while (TRUE)
	{
		//EnterCriticalSection(&rxMutex);
        //TRACE_BT("RxMutex take\r\n");
        xSemaphoreTake(rxMutex, portMAX_DELAY);
		theRxSize = rxSize;
		//LeaveCriticalSection(&rxMutex);
        xSemaphoreGive(rxMutex);
        //TRACE_BT("RxMutex given\r\n");

		/*	find next position for rx data in and if any space available in buffer	*/
		if (theRxSize == RX_BUF_MAX_SIZE)
		{
            //TRACE_BT("RXEVENT WAIT\r\n");
            xQueueReceive(rxEvents, &event, portMAX_DELAY); 
            //TRACE_BT("RXEVENT\r\n");
            switch (event)
            {
                case CLOSE_DOWN_EVENT:
                {
                    /*	to do	*/
                    //ResetEvent(rxCloseDownEvent);
                    break;
                }
                case DATA_READ_EVENT:
                {
                    /* sufficient data has been read from the rx buffer to restart the reader thread */
// TODO
                    //ResetEvent(dataReadEvent);
                    break;
                }
                default:
                {
                    /*	error occured	*/
                    errorHandler(__LINE__, __FILE__, "Default called");
                    break;
                }
            }
			if (event == CLOSE_DOWN_EVENT)
			{
				break;
			}
		}


        rxData = rxBuf + rxIn;

#if 0
        int bytesAvail = Serial_bytesAvailable(SERIAL_CHANNEL);
        //TRACE_BT("AVAIL %d\r\n", bytesAvail);
        numberToRead =  bytesAvail > 0 ? bytesAvail : 1;
        if (rxIn + numberToRead > RX_BUF_MAX_SIZE)
        {
            numberToRead = RX_BUF_MAX_SIZE - rxIn;
        }

        //TRACE_BT("READING\r\n");
        bytesRead = Serial_read(SERIAL_CHANNEL, rxData, numberToRead, -1);
        //TRACE_BT("DATA READ %d\r\n", bytesRead);
        if (bytesRead < 0)
        {
            errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
        }
        else
        {
            if (bytesRead > 0)
            {
                handleRxData(bytesRead, rxData);
            }
        }
#else

        bytesRead = 0;
        int count = 0;
        while(1) {

            int bytesAvail = Serial_bytesAvailable(SERIAL_CHANNEL);
            //TRACE_BT("AVAIL %d\r\n", bytesAvail);

            if (bytesRead) {
                if (!bytesAvail) 
                    break;
                numberToRead = bytesAvail;
            } else {
                numberToRead =  bytesAvail > 0 ? bytesAvail : 1;
            }
            if (rxIn + bytesRead + numberToRead > RX_BUF_MAX_SIZE)
            {
                numberToRead = RX_BUF_MAX_SIZE - rxIn - bytesRead;
            }
            if (!numberToRead)
                break;

            //TRACE_BT("READING %x %d\r\n", rxData, numberToRead);
            int n = Serial_read(SERIAL_CHANNEL, rxData + bytesRead, numberToRead, -1);
            //TRACE_BT("DATA READ %d\r\n", n);
            if (n < 0)
            {
                errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
                bytesRead = n;
                break;
            }
            bytesRead += n;
            count++;
        }
        if (bytesRead > 0) {
            handleRxData(bytesRead, rxData);
        }
#endif

	}	/*	end while	*/

    vTaskDelete(NULL);
}

void rxThreadFunc_new(void)
//void rxThreadFunc(void)
{
	uint16_t		event;
	uint8_t		*rxData;
	uint16_t		bytesRead;
	bool_t		readSuccess;
	bool_t		startRead;
	uint16_t	theRxSize;
	uint16_t		errors;
	uint16_t		numberToRead;


	rxData = rxBuf;
	rxSize = 0;

    TRACE_INFO("rxThreadFunc %x\r\n", xTaskGetCurrentTaskHandle());
	while (TRUE)
	{
        xSemaphoreTake(rxMutex, portMAX_DELAY);
		theRxSize = rxSize;
        xSemaphoreGive(rxMutex);

		/*	find next position for rx data in and if any space available in buffer	*/
		if (theRxSize < RX_BUF_MAX_SIZE)
		{
			startRead = TRUE;
		}
		else
		{
			/*	wait for available space in the buffer	*/
            readSuccess = FALSE;
			startRead = FALSE;
		}
		
		/*	kick the read process if allowed (space left in rx buffer)	*/
		if (startRead)
		{
			rxData = rxBuf + rxIn;
#if 0
            int bytesAvail = Serial_bytesAvailable(SERIAL_CHANNEL);
            //TRACE_BT("AVAIL %d\r\n", bytesAvail);
            numberToRead =  bytesAvail > 0 ? bytesAvail : 1;
            if (rxIn + numberToRead > RX_BUF_MAX_SIZE)
            {
                numberToRead = RX_BUF_MAX_SIZE - rxIn;
            }

            //TRACE_BT("READING\r\n");
            bytesRead = Serial_read(SERIAL_CHANNEL, rxData, numberToRead, -1);
            //TRACE_BT("DATA READ %d\r\n", bytesRead);
            if (bytesRead < 0)
            {
                readSuccess = FALSE;
                errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
            }
            else
            {
                readSuccess = TRUE;
                if (bytesRead > 0)
                {
                    handleRxData(bytesRead, rxData);
                }
            }
#else

            bytesRead = 0;
            int count = 0;
            while(1) {

                int bytesAvail = Serial_bytesAvailable(SERIAL_CHANNEL);
                //TRACE_BT("AVAIL %d\r\n", bytesAvail);

                if (bytesRead) {
                    if (!bytesAvail) 
                        break;
                    numberToRead = bytesAvail;
                } else {
                    numberToRead =  bytesAvail > 0 ? bytesAvail : 1;
                }
                if (rxIn + bytesRead + numberToRead > RX_BUF_MAX_SIZE)
                {
                    numberToRead = RX_BUF_MAX_SIZE - rxIn - bytesRead;
                }
                if (!numberToRead)
                    break;

                //TRACE_BT("READING %x %d\r\n", rxData, numberToRead);
                int n = Serial_read(SERIAL_CHANNEL, rxData + bytesRead, numberToRead, -1);
                //TRACE_BT("DATA READ %d\r\n", n);
                if (n < 0)
                {
                    readSuccess = FALSE;
                    errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
                    bytesRead = n;
                    break;
                }
                bytesRead += n;
                count++;
            }
            if (bytesRead >= 0) {
                readSuccess = TRUE;
                if (bytesRead > 0) {
                    handleRxData(bytesRead, rxData);
                }
            }
#endif
		}

		if (!readSuccess)
		{
            //TRACE_BT("rxEvent Wait\r\n");
            xQueueReceive(rxEvents, &event, portMAX_DELAY); 
            //TRACE_BT("rxEvent\r\n");
            switch (event)
            {
                case CLOSE_DOWN_EVENT:
                {
                    /*	to do	*/
                    break;
                }
                case DATA_READ_EVENT:
                {
                    /* sufficient data has been read from the rx buffer to restart the reader thread */
// TODO
                    break;
                }
                default:
                {
                    /*	error occured	*/
                    errorHandler(__LINE__, __FILE__, "Default called");
                    break;
                }
            }

			if (event == CLOSE_DOWN_EVENT)
			{
				break;
			}
		}
	}	/*	end while	*/

    vTaskDelete(NULL);
}

//void rxThreadFuncDMA(void)
void rxThreadFunc(void)
{
	uint16_t		event;
	uint8_t		*rxData;
	uint16_t		bytesRead;
	bool_t		readSuccess;
	bool_t		startRead;
	uint16_t	theRxSize;
	uint16_t		errors;
	uint16_t		numberToRead;
    int         ringBuffCycle = 0;

// TEST
    xSemaphoreHandle sleep_sem;
    vSemaphoreCreateBinary(sleep_sem);
    xSemaphoreTake(sleep_sem, -1);

	rxData = rxBuf;
	rxSize = 0;

    TRACE_INFO("rxThreadFunc %x\r\n", xTaskGetCurrentTaskHandle());

    pm_lock();

    Serial_setDMARxBuff(SERIAL_CHANNEL, rxBuf, RX_BUF_MAX_SIZE, rxBuf, RX_BUF_MAX_SIZE);
    Serial_DMARxStart(SERIAL_CHANNEL);
	while (TRUE)
	{
        xSemaphoreTake(rxMutex, portMAX_DELAY);
		theRxSize = rxSize;
        xSemaphoreGive(rxMutex);

		/*	find next position for rx data in and if any space available in buffer	*/
        //TRACE_INFO("rxSize %d rxIn %d\r\n", theRxSize, rxIn);
		if (theRxSize < RX_BUF_MAX_SIZE)
		{
            // ring buffer cycled 
            if (rxIn == 0) {

                //TRACE_INFO("\n---------------- Cycle %d -----------------\r\n\n", ringBuffCycle);
                if (ringBuffCycle) {
                    Serial_setDMARxBuff(SERIAL_CHANNEL, NULL, 0, rxBuf, RX_BUF_MAX_SIZE);
                }
                ringBuffCycle++;
            }
			startRead = TRUE;
		}
		else
		{
			/*	wait for available space in the buffer	*/
            readSuccess = FALSE;
			startRead = FALSE;
		}
		
		/*	kick the read process if allowed (space left in rx buffer)	*/
		if (startRead)
		{
			rxData = rxBuf + rxIn;

#if 0
            int rcr;
            uint8_t *rpr = Serial_getDMARxBuff(SERIAL_CHANNEL, &rcr);
            TRACE_INFO("RPR %x RCR %d\r\n", rpr, rcr);
            bytesRead = Serial_waitForDMARxData3(SERIAL_CHANNEL, rxBuf, RX_BUF_MAX_SIZE, rxData, 1, -1);
#elif 0                
            int rcr;
            uint8_t *currRxData = Serial_getDMARxBuff(SERIAL_CHANNEL, &rcr);

            int bytesAvail = 0;
            if (currRxData > rxData) {
                bytesAvail = currRxData - rxData;
            } else if (currRxData < rxData) {
                bytesAvail = rxBuf + RX_BUF_MAX_SIZE - rxData;
            }
     
            TRACE_BT("AVAIL %d\r\n", bytesAvail);
            if (bytesAvail) {
                TRACE_BT("READING\r\n");
                bytesRead = bytesAvail;
            } else {
                // TODO: Wait for DMA data
                TRACE_BT("WAITTING\r\n");
                bytesRead = Serial_waitForDMARxData3(SERIAL_CHANNEL, rxBuf, RX_BUF_MAX_SIZE, rxData, 1, -1);
            }
#else
            // Polling every 10ms
            //TRACE_SER("Read %d\r\n", xTickCount);
            //TRACE_SER("Read %d\r\n", Timer_tick_count());
            int waitCount = 0;
            while (1) {
                int rcr;
                uint8_t *currRxData = Serial_getDMARxBuff(SERIAL_CHANNEL, &rcr);

                int bytesAvail = 0;
                if (currRxData > rxData) {
                    bytesAvail = currRxData - rxData;
                } else if (currRxData < rxData) {
                    bytesAvail = rxBuf + RX_BUF_MAX_SIZE - rxData;
                }
         
                if (bytesAvail) {
                    TRACE_SER("AVAIL %d %d\r\n", bytesAvail, waitCount);
                    TRACE_BT("READING\r\n");
                    bytesRead = bytesAvail;
                    last_activity_ticks = Timer_tick_count();
                    break;
                }
                // TODO: Wait for DMA data
                //TRACE_BT("WAITTING\r\n");
                if (waitCount == 0) {
                    //TRACE_INFO("WAITTING %d\r\n", xTickCount);
                    //TRACE_INFO("WAITTING %d\r\n", Timer_tick_count());
                }
                //if (1) {
                //if (waitCount < 40) {
                if (Timer_tick_count() - last_activity_ticks < BT_HOST_WAKE_SLEEP_TIMEOUT - BT_DATA_POLL_INTERVAL) {
                    //Task_sleep(10);
                    sleep2(sleep_sem, BT_DATA_POLL_INTERVAL);
                    waitCount++;
                } else {
                    //Serial_waitForData(SERIAL_CHANNEL, -1);
                    bt_wait_for_data();
                    last_activity_ticks = Timer_tick_count();
                    waitCount = 0;
                }
            }
    
#endif
            //TRACE_INFO("bytesRead %d\r\n", bytesRead);
            if (bytesRead >= 0)
            {
                readSuccess = TRUE;
                if (bytesRead > 0) {
                    handleRxData(bytesRead, rxData);
                }
            }
		}

		if (!readSuccess)
		{
            //TRACE_BT("rxEvent Wait\r\n");
            xQueueReceive(rxEvents, &event, portMAX_DELAY); 
            //TRACE_BT("rxEvent\r\n");
            switch (event)
            {
                case CLOSE_DOWN_EVENT:
                {
                    /*	to do	*/
                    break;
                }
                case DATA_READ_EVENT:
                {
                    /* sufficient data has been read from the rx buffer to restart the reader thread */
// TODO
                    /*
                    if (rxIn == 0) {
                        Serial_setDMARxBuff(SERIAL_CHANNEL, NULL, 0, rxBuf, RX_BUF_MAX_SIZE);
                    }
                    */
                    break;
                }
                default:
                {
                    /*	error occured	*/
                    errorHandler(__LINE__, __FILE__, "Default called");
                    break;
                }
            }

			if (event == CLOSE_DOWN_EVENT)
			{
				break;
			}
		}
	}	/*	end while	*/

    pm_unlock();

    vQueueDelete(sleep_sem);
    vTaskDelete(NULL);
}

#if 0
void rxThreadFuncDMA_DoubleBuf(void)
//void rxThreadFunc(void)
{
	uint16_t		event;
	uint8_t		*rxData;
	uint16_t		bytesRead;
	bool_t		readSuccess;
	bool_t		startRead;
	uint16_t	theRxSize;
	uint16_t		errors;
	uint16_t		numberToRead;
    int         ringBuffCycle = 0;
    int         currRxBuffNo = 0;
    uint8_t     *currRxBuff;
    bool_t wait = 0;

    currRxBuf = &rxDMABuf[0];
	//rxData = currRxBuf;
	rxSize = 0;

    TRACE_INFO("rxThreadFunc %x\r\n", xTaskGetCurrentTaskHandle());

      
    Serial_setDMARxBuff(SERIAL_CHANNEL, &rxDMABuf[0], RX_BUF_MAX_SIZE, &rxDMABuf[1], RX_BUF_MAX_SIZE);
	while (TRUE)
	{
        xSemaphoreTake(rxMutex, portMAX_DELAY);
		theRxSize = rxSize;
        xSemaphoreGive(rxMutex);

		/*	find next position for rx data in and if any space available in buffer	*/
        TRACE_INFO("rxSize %d rxIn %d\r\n", theRxSize, rxIn);
		if (theRxSize < RX_BUF_MAX_SIZE)
		{
            // ring buffer cycled 
            if (rxIn == 0) {
                TRACE_INFO("\n---------------- Cycle %d -----------------\r\n\n", ringBuffCycle);
                if (ringBuffCycle) {
                    wait = TRUE;
                    waitForEmptyRxBuff = TRUE;
                    readSuccess = FALSE;
                    startRead = FALSE;
                } else {
                    startRead = TRUE;
                }
                ringBuffCycle++;
            }
		}
		else
		{
			/*	wait for available space in the buffer	*/
            wait = TRUE;
            readSuccess = FALSE;
			startRead = FALSE;
		}
		
		/*	kick the read process if allowed (space left in rx buffer)	*/
		if (startRead)
		{
			rxData = currRxBuf + rxIn;

#if 1
            int rcr;
            uint8_t *rpr = Serial_getDMARxBuff(SERIAL_CHANNEL, &rcr);
            TRACE_INFO("RPR %x RCR %d\r\n", rpr, rcr);
            bytesRead = Serial_waitForDMARxData3(SERIAL_CHANNEL, currRxBuf, RX_BUF_MAX_SIZE, rxData, 1, -1);
            TRACE_INFO("bytesRead %d\r\n", bytesRead);
#else                
            while (1) {
                Serial_DMARxStop(SERIAL_CHANNEL);
                uint8_t *currRxData = Serial_getDMARxBuff(SERIAL_CHANNEL);

                int bytesAvail = 0;
                if (currRxData > rxData) {
                    bytesAvail = currRxData - rxData;
                } else if (currRxData < rxData) {
                    bytesAvail = RX_BUF_MAX_SIZE - (int)rxData;
                }
         
                TRACE_BT("AVAIL %d\r\n", bytesAvail);
                if (bytesAvail) {
                    Serial_DMARxStart(SERIAL_CHANNEL);
                    TRACE_BT("READING\r\n");
                    bytesRead = bytesAvail;
                    break;
                } else {
                    // TODO: Wait for DMA data
                    TRACE_BT("WAITTING\r\n");
                    if (Serial_waitForDMARxData(SERIAL_CHANNEL, 1, -1) < 0) 
                    {
                        errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
                        bytesRead = -1;
                        break;
                    }
                }
            }
#endif
            if (bytesRead >= 0)
            {
                readSuccess = TRUE;
                if (bytesRead > 0) {
                    handleRxData(bytesRead, rxData);
                }
            } else {
                readSuccess = FALSE;
            }
		}

		if (wait)
		{
            do {
                //TRACE_BT("rxEvent Wait\r\n");
                xQueueReceive(rxEvents, &event, portMAX_DELAY); 
                //TRACE_BT("rxEvent\r\n");
                switch (event)
                {
                    case CLOSE_DOWN_EVENT:
                    {
                        /*	to do	*/
                        wait = FALSE;
                        break;
                    }
                    case DATA_READ2_EVENT:
                    {
                        /* sufficient data has been read from the rx buffer to restart the reader thread */
                        if (rxIn == 0) {
                            Serial_setDMARxBuff(SERIAL_CHANNEL, NULL, 0, currRxBuf, RX_BUF_MAX_SIZE);
                            currRxBuffNo = !currRxBuffNo;
                            currRxBuf = &rxDMABuf[currRxBuffNo];
                            startRead = TRUE;
                            wait = FALSE;
                        }
                    }
                    case DATA_READ_EVENT:
                    {
                        /* sufficient data has been read from the rx buffer to restart the reader thread */
                        wait = FALSE;
                        break;
                    }
                    default:
                    {
                        /*	error occured	*/
                        errorHandler(__LINE__, __FILE__, "Default called");
                        break;
                    }
                }
			} while (wait));

			if (event == CLOSE_DOWN_EVENT)
			{
				break;
			}
		}
	}	/*	end while	*/

    vTaskDelete(NULL);
}
#endif

void rxThreadFuncDMA_semiworking(void)
//void rxThreadFunc(void)
{
	uint16_t		event;
	uint8_t		*rxData;
	uint16_t		bytesRead;
	bool_t		readSuccess;
	bool_t		startRead;
	uint16_t	theRxSize;
	uint16_t		errors;
	uint16_t		numberToRead;
	uint16_t		lastCurrFreeSize;
	uint16_t		lastNextFreeSize;
	uint16_t		lastTheRxOut;
    int         ringBuffCycle = 0;

	rxData = rxBuf;
	rxSize = 0;
    lastTheRxOut = 0;

    TRACE_INFO("rxThreadFunc %x\r\n", xTaskGetCurrentTaskHandle());

    Serial_setDMARxBuff(SERIAL_CHANNEL, rxData, RX_BUF_MAX_SIZE, NULL, 0);
	while (TRUE)
	{
        xSemaphoreTake(rxMutex, portMAX_DELAY);
		theRxSize = rxSize;
        xSemaphoreGive(rxMutex);

		/*	find next position for rx data in and if any space available in buffer	*/
        // !!!! TODO: rxSize + rxIn sometimes > RX_BUF_MAX_SIZE !!!!
        int theRxOut = rxIn - rxSize;
        if (theRxOut < 0)
            theRxOut = RX_BUF_MAX_SIZE + theRxOut;

        TRACE_INFO("rxSize %d rxIn %d rxOut %d\r\n", theRxSize, rxIn, theRxOut);
		if (theRxSize < RX_BUF_MAX_SIZE)
		{
            // ring buffer cycled 
            if (rxIn == 0) {
                TRACE_INFO("\n---------------- Cycle %d -----------------\r\n\n", ringBuffCycle++);
                lastCurrFreeSize = RX_BUF_MAX_SIZE;
                lastNextFreeSize = 0;
            }

            // adjust DMA buffers

            int currFreeSize = RX_BUF_MAX_SIZE - theRxSize;
            int nextFreeSize = 0;

            // check if there is an empty space at the beginning of rxBuf
            if (theRxSize < rxIn) {
                // TODO 
                nextFreeSize = rxIn - theRxSize;
                currFreeSize -= nextFreeSize;
            }
            TRACE_INFO("Free C %d (%d) N %d (%d)\r\n", currFreeSize, lastCurrFreeSize, nextFreeSize, lastNextFreeSize);

            if (lastTheRxOut > theRxOut) {
                Serial_setDMARxBuff(SERIAL_CHANNEL, NULL, RX_BUF_MAX_SIZE - lastTheRxOut, NULL, 0);
            } else if (theRxOut > rxIn && theRxOut > lastTheRxOut) {
                Serial_setDMARxBuff(SERIAL_CHANNEL, NULL, theRxOut - lastTheRxOut, NULL, 0);
            }
            if (nextFreeSize > lastNextFreeSize) {
                Serial_setDMARxBuff(SERIAL_CHANNEL, NULL, 0, rxBuf, nextFreeSize);
            }
            lastCurrFreeSize = currFreeSize;
            lastNextFreeSize = nextFreeSize;
            lastTheRxOut = theRxOut;

			startRead = TRUE;
		}
		else
		{
            lastCurrFreeSize = 0;
            lastNextFreeSize = 0;

			/*	wait for available space in the buffer	*/
            readSuccess = FALSE;
			startRead = FALSE;
		}
		
		/*	kick the read process if allowed (space left in rx buffer)	*/
		if (startRead)
		{
			rxData = rxBuf + rxIn;

#if 1
            int rcr;
            uint8_t *rpr = Serial_getDMARxBuff(SERIAL_CHANNEL, &rcr);
            TRACE_INFO("RPR %x RCR %d\r\n", rpr, rcr);
            bytesRead = Serial_waitForDMARxData2(SERIAL_CHANNEL, rxBuf, RX_BUF_MAX_SIZE, rxData, 1, -1);
            TRACE_INFO("bytesRead %d\r\n", bytesRead);
#else                
            while (1) {
                Serial_DMARxStop(SERIAL_CHANNEL);
                uint8_t *currRxData = Serial_getDMARxBuff(SERIAL_CHANNEL);

                int bytesAvail = 0;
                if (currRxData > rxData) {
                    bytesAvail = currRxData - rxData;
                } else if (currRxData < rxData) {
                    bytesAvail = RX_BUF_MAX_SIZE - (int)rxData;
                }
         
                TRACE_BT("AVAIL %d\r\n", bytesAvail);
                if (bytesAvail) {
                    Serial_DMARxStart(SERIAL_CHANNEL);
                    TRACE_BT("READING\r\n");
                    bytesRead = bytesAvail;
                    break;
                } else {
                    // TODO: Wait for DMA data
                    TRACE_BT("WAITTING\r\n");
                    if (Serial_waitForDMARxData(SERIAL_CHANNEL, 1, -1) < 0) 
                    {
                        errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
                    }
                }
            }
#endif
            if (bytesRead >= 0)
            {
                readSuccess = TRUE;
                if (bytesRead > 0) {
                    handleRxData(bytesRead, rxData);
                }
            }
		}

		if (!readSuccess)
		{
            //TRACE_BT("rxEvent Wait\r\n");
            xQueueReceive(rxEvents, &event, portMAX_DELAY); 
            //TRACE_BT("rxEvent\r\n");
            switch (event)
            {
                case CLOSE_DOWN_EVENT:
                {
                    /*	to do	*/
                    break;
                }
                case DATA_READ_EVENT:
                {
                    /* sufficient data has been read from the rx buffer to restart the reader thread */
// TODO
                    break;
                }
                default:
                {
                    /*	error occured	*/
                    errorHandler(__LINE__, __FILE__, "Default called");
                    break;
                }
            }

			if (event == CLOSE_DOWN_EVENT)
			{
				break;
			}
		}
	}	/*	end while	*/

    vTaskDelete(NULL);
}

void rxThreadFunc_old(void)
{
	//OVERLAPPED	osRead	= {0};
	uint16_t		event;
	uint8_t		*rxData;
	uint16_t		bytesRead;
	bool_t		readSuccess;
	bool_t		startRead;
	uint16_t	theRxSize;
	//COMSTAT		comStat;
	uint16_t		errors;
	uint16_t		numberToRead;


/*
	osRead.hEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
	if ( (osRead.hEvent == NULL) )
	{
		errorHandler(__LINE__, __FILE__, "Create event failed in rx thread");
		exit(0);
	}

	rxEvents[CLOSE_DOWN_EVENT]	= rxCloseDownEvent;
	rxEvents[RX_DATA_EVENT]		= osRead.hEvent;
	rxEvents[DATA_READ_EVENT]	= dataReadEvent;
*/

	rxData = rxBuf;
	rxSize = 0;

    TRACE_INFO("rxThreadFunc %x\r\n", xTaskGetCurrentTaskHandle());
	while (TRUE)
	{
		//EnterCriticalSection(&rxMutex);
        xSemaphoreTake(rxMutex, portMAX_DELAY);
		theRxSize = rxSize;
		//LeaveCriticalSection(&rxMutex);
        xSemaphoreGive(rxMutex);

		/*	find next position for rx data in and if any space available in buffer	*/
		if (theRxSize < RX_BUF_MAX_SIZE)
		{
			startRead = TRUE;
		}
		else
		{
			/*	wait for available space in the buffer	*/
            readSuccess = FALSE;
			startRead = FALSE;
		}
		
		/*	kick the read process if allowed (space left in rx buffer)	*/
		if (startRead)
		{
			rxData = rxBuf + rxIn;

			/* find out how many to read */
/*
			if (!ClearCommError(comHandle, &errors, &comStat))
			{
				errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
			}
			numberToRead = comStat.cbInQue > 0 ? comStat.cbInQue : 1;
*/
            int bytesAvail = Serial_bytesAvailable(SERIAL_CHANNEL);
            //TRACE_BT("avail %d\r\n", bytesAvail);
			numberToRead =  bytesAvail > 0 ? bytesAvail : 1;
			if (rxIn + numberToRead > RX_BUF_MAX_SIZE)
			{
				numberToRead = RX_BUF_MAX_SIZE - rxIn;
			}

			//readSuccess = ReadFile(comHandle, rxData, numberToRead, &bytesRead, &osRead);
            //TRACE_BT("Reading\r\n");
			bytesRead = Serial_read(SERIAL_CHANNEL, rxData, numberToRead, -1);
			//if (!readSuccess)
			if (bytesRead < 0)
			{
                //TRACE_BT("Data read\r\n");
/*
				if (GetLastError() != ERROR_IO_PENDING)
				{
					errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
				}
*/
                errorHandler(__LINE__, __FILE__, "Serious error in read file in rx thread");
			}
			else
			{
                readSuccess = TRUE;
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
				//event = WaitForMultipleObjects(NO_OF_RX_EVENTS, rxEvents, FALSE, INFINITE);
                //TRACE_BT("rxEvent Wait\r\n");
                xQueueReceive(rxEvents, &event, portMAX_DELAY); 
                //TRACE_BT("rxEvent\r\n");
                //event = DATA_READ_EVENT;
				switch (event)
				{
					case CLOSE_DOWN_EVENT:
					{
						/*	to do	*/
						//ResetEvent(rxCloseDownEvent);
						break;
					}
/*
					case RX_DATA_EVENT:
					{
						bool_t	success;

						//	receive complete, store the dat
						success = GetOverlappedResult(comHandle, &osRead, &bytesRead, FALSE);

						if ((bytesRead > 0) && success)
						{
							handleRxData(bytesRead, rxData);
						}

						startRead = FALSE;
						//ResetEvent(osRead.hEvent);
						break;
					}
*/
					case DATA_READ_EVENT:
					{
						/* sufficient data has been read from the rx buffer to restart the reader thread */
// TODO
						//ResetEvent(dataReadEvent);
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

	//CloseHandle(osRead.hEvent);

	//SetEvent(rxDownEvent);

	//ExitThread(0);
    vTaskDelete(NULL);
}

void clearBuffer(void)
{
    //TRACE_BT("clearBuffer\r\n");
	/* flush the port for any operations waiting and any data	*/
	//PurgeComm(comHandle, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

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

    //TRACE_BT("init\r\n");
	clearBuffer();

	/*	mutex for sync of write and read	*/
	//InitializeCriticalSection(&rxMutex);
    rxMutex = xSemaphoreCreateMutex();
	//InitializeCriticalSection(&txMutex);
    txMutex = xSemaphoreCreateMutex();

	/*	event for internal communication	*/
/*
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
*/
    
    rxEvents = xQueueCreate(1, sizeof(uint16_t)); 
    txEvents = xQueueCreate(1, sizeof(uint16_t)); 

	/*	create the file handle	*/
/*
    
	comHandle = CreateFile(comPortString, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
	if (comHandle == INVALID_HANDLE_VALUE)
	{
		errorHandler(__LINE__, __FILE__, "Create file handle failed");
		return FALSE;	
	}
*/
    Serial_open(SERIAL_CHANNEL, 1024);

/*
	sprintf(configurationString, "baud=%lu parity=E data=8 stop=1", baudRate);
	if (!setComDefault(configurationString))
	{
		return FALSE;
	}
*/
    Serial_setBaud(SERIAL_CHANNEL, baudRate);
    Serial_setParity(SERIAL_CHANNEL, 1);

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
void UartDrv_Configure(unsigned long theBaudRate)
{
    //TRACE_BT("UartDrv_Configure %d\r\n", theBaudRate);
	baudRate = theBaudRate;
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
	//uint16_t	threadId;

    //TRACE_BT("UartDrv_Start\r\n");
	if (!init())
	{
		return FALSE;
	}

	/*	tx and rx threads		*/
	//txThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) txThreadFunc, NULL, 0, &threadId);
	//if(txThread == INVALID_HANDLE_VALUE) 
    if(xTaskCreate(txThreadFunc, "bt_tx", TASK_STACK_SIZE(TASK_BT_TX_STACK), NULL, TASK_BT_TX_PRI, &txThread) != pdPASS ) 
	{
		errorHandler(__LINE__, __FILE__, "Thread create failure");
		return(FALSE);
	}
	//rxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) rxThreadFunc, NULL, 0, &threadId);
	//if(rxThread == INVALID_HANDLE_VALUE) 
    if(xTaskCreate(rxThreadFunc, "bt_rx", TASK_STACK_SIZE(TASK_BT_RX_STACK), NULL, TASK_BT_RX_PRI, &rxThread) != pdPASS ) 
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
    //TRACE_BT("UartDrv_Stop\r\n");

    vTaskDelete(txThread);
    vTaskDelete(rxThread);

    vQueueDelete(txMutex);
    vQueueDelete(rxMutex);

    vQueueDelete(txEvents);
    vQueueDelete(rxEvents);

    Serial_close(SERIAL_CHANNEL);
/*
	uint16_t threadExitCode;

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
*/
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
    //TRACE_BT("UartDrv_Tx %d %x %d %d\r\n", len, txBuf, txIn, txSize);
	//EnterCriticalSection(&txMutex);
    xSemaphoreTake(txMutex, portMAX_DELAY);
	size = txSize;
	//LeaveCriticalSection(&txMutex);
    xSemaphoreGive(txMutex);

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
    //TRACE_BT("UartDrv_Tx %d %d %d\r\n", len, txIn, txSize);

	//EnterCriticalSection(&txMutex);
    xSemaphoreTake(txMutex, portMAX_DELAY);
	txSize = txSize + (*numSend);
	//LeaveCriticalSection(&txMutex);
    xSemaphoreGive(txMutex);

	/*	signal tx thread that new data has arrived	*/
	//SetEvent(newTxDataEvent);
    uint16_t event = NEW_TX_DATA_EVENT;
    //xQueueSend(txEvents, &event, portMAX_DELAY);
    xQueueSend(txEvents, &event, 0);
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
    //TRACE_BT("UartDrv_GetTxSpace\r\n");

	//EnterCriticalSection(&txMutex);
    xSemaphoreTake(txMutex, portMAX_DELAY);
	size = TX_BUF_MAX_SIZE - txSize;
	//LeaveCriticalSection(&txMutex);
    xSemaphoreGive(txMutex);

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
    //TRACE_BT("UartDrv_GetRxAvailable\r\n");

	//EnterCriticalSection(&rxMutex);
    xSemaphoreTake(rxMutex, portMAX_DELAY);
	theSize = rxSize;
	//LeaveCriticalSection(&rxMutex);
    xSemaphoreGive(rxMutex);

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
    //TRACE_BT("UartDrv_Reset\r\n");
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

    //TRACE_BT("UartDrv_Rx\r\n");
	//EnterCriticalSection(&rxMutex);
    xSemaphoreTake(rxMutex, portMAX_DELAY);
	noOfBytes = rxSize;
	//LeaveCriticalSection(&rxMutex);
    xSemaphoreGive(rxMutex);

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
    //TRACE_BT("abcsp_uart_deliverbytes %d\r\n", bytesConsumed);
	rxOut = rxOut + bytesConsumed;

	if (rxOut >= RX_BUF_MAX_SIZE)
	{
		rxOut = 0x00; /* Buffer start from beginning*/
	}

	//EnterCriticalSection(&rxMutex);
    xSemaphoreTake(rxMutex, portMAX_DELAY);
	rxSize = rxSize - bytesConsumed;
	//LeaveCriticalSection(&rxMutex);
    xSemaphoreGive(rxMutex);

	if (bytesConsumed > 0)
	{
		//SetEvent(dataReadEvent);
        uint16_t event = DATA_READ_EVENT;
        //xQueueSend(rxEvents, &event, portMAX_DELAY);
        xQueueSend(rxEvents, &event, 0);
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
    //TRACE_BT("UartDrv_RegisterHandlers\r\n");
	register_bg_int(1, UartDrv_Rx);
	register_bg_int(2, BgIntPump); /*The bg int needed for abcsp pumps*/
}
