
#include <stdio.h>
#include <stdlib.h>
#include "uSched.h"
#include "SerialCom.h"
#include "abcsp_support_functions.h"
#include "hci.h"

abcsp AbcspInstanceData;

extern int NumberOfHciCommands;

/* Used as defines in config_rxmsg.h */

static unsigned char cmdCompleteCount = 0;

void abcsp_delivermsg(abcsp * thisInstance, ABCSP_RXMSG * message, unsigned channel, unsigned reliableFlag)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;
	if (channel == HCI_EVENT_CHANNEL)
	{
		if (messageBuffer->buffer[0] == HCI_COMMAND_COMPLETE_EVENT)
		{
			NumberOfHciCommands = messageBuffer->buffer[HCI_CCE_NUM_HCI_COMMAND_PACKETS_OFFSET];
			if (((((uint16) messageBuffer->buffer[HCI_CCE_COMMAND_OPCODE_HIGH_BYTE]) << 8)
				 | ((uint16) messageBuffer->buffer[HCI_CCE_COMMAND_OPCODE_LOW_BYTE]))
				== HCI_COMMAND_READ_BD_ADDR)
			{
				cmdCompleteCount++;
				if (cmdCompleteCount >= 100)
				{
					printf("*");
					cmdCompleteCount = 0;
				}
			}
		}
		else if (messageBuffer->buffer[0] == HCI_COMMAND_STATUS_EVENT)
		{
			NumberOfHciCommands = messageBuffer->buffer[HCI_CSE_NUM_HCI_COMMAND_PACKETS_OFFSET];
		}

		if (NumberOfHciCommands)
		{
			ScheduleTaskToRun();
		}
	}
	free(messageBuffer->buffer);
	free(messageBuffer);
}

ABCSP_RXMSG * abcsp_rxmsg_create(abcsp * thisInstance, unsigned length)
{
	MessageBuffer * messageBuffer;

	messageBuffer = malloc(sizeof(MessageBuffer));
	messageBuffer->length = length;
	messageBuffer->index = 0;
	messageBuffer->buffer = (unsigned char *) malloc(length);

	return (ABCSP_RXMSG *) messageBuffer;
}

char * abcsp_rxmsg_getbuf(ABCSP_RXMSG * message, unsigned * bufferSize)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;

	return (char *) &messageBuffer->buffer[messageBuffer->index];
}

void abcsp_rxmsg_write(ABCSP_RXMSG * message, char * buffer, unsigned number)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;
	messageBuffer->index += number;
}

void abcsp_rxmsg_complete(abcsp * thisInstance, ABCSP_RXMSG * message)
{
}

void abcsp_rxmsg_destroy(abcsp * thisInstance, ABCSP_RXMSG * message)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;

	free(messageBuffer->buffer);
	free(messageBuffer);
}


/* Used as defines in config_event.h */

static char eventStrings[][48] =
{
	"ABCSP_EVT_NULL?!?!",
	"ABCSP_EVT_START",
	"ABCSP_EVT_INITED",
	"ABCSP_EVT_LE_SYNC",
	"ABCSP_EVT_LE_CONF",
	"ABCSP_EVT_LE_SYNC_LOST",
	"ABCSP_EVT_UNINITED",
	"ABCSP_EVT_SLIP_SYNC",
	"ABCSP_EVT_SLIP_SYNC_LOST",
	"ABCSP_EVT_RX_CHOKE_DISCARD",
	"ABCSP_EVT_TX_CHOKE_DISCARD",
	"ABCSP_EVT_TX_WINDOW_FULL_DISCARD",
	"ABCSP_EVT_OVERSIZE_DISCARD",
	"ABCSP_EVT_MISSEQ_DISCARD",
	"ABCSP_EVT_CHECKSUM",
	"ABCSP_EVT_SHORT_PAYLOAD",
	"ABCSP_EVT_OVERRUN",
	"ABCSP_EVT_CRC_FAIL",
	"ABCSP_EVT_MALLOC"
};

void abcsp_event(abcsp * thisInstance, unsigned event)
{
	if (event == ABCSP_EVT_LE_SYNC_LOST)
	{
		printf("\nabcsp_event: %s\n\n", eventStrings[event]);
		printf("The BCSP Link Establishment engine has detected that the peer BCSP-LE\n");
		printf("engine has restarted.  This presumably means that the peer BCSP stack\n");
		printf("(or system) has been restarted.  A common local response would be to\n");
		printf("restart (abcsp_init()) the local BCSP stack.\n");
		printf("\nThis example exits the program! Please, wait...\n");

		TerminateMicroSched();
	}
}

void abcsp_req_pumptxmsgs(abcsp * thisInstance)
{
	bg_int2();
}


/* Used as defines in config_timer.h */

static void * bcspTimerHandle = NULL;
static void * tshyTimerHandle = NULL;
static void * confTimerHandle = NULL;

static void bcspTimeout(void)
{
	bcspTimerHandle = NULL;
	abcsp_bcsp_timed_event(&AbcspInstanceData);
}

static void tshyTimeout(void)
{
	tshyTimerHandle = NULL;
	abcsp_tshy_timed_event(&AbcspInstanceData);
}

static void confTimeout(void)
{
	confTimerHandle = NULL;
	abcsp_tconf_timed_event(&AbcspInstanceData);
}

void abcsp_start_bcsp_timer(abcsp * thisInstance)
{
	if (bcspTimerHandle != NULL)
	{
		StopTimer(bcspTimerHandle);
	}
	bcspTimerHandle = StartTimer(250000, bcspTimeout);
}

void abcsp_start_tshy_timer(abcsp * thisInstance)
{
	if (tshyTimerHandle != NULL)
	{
		StopTimer(tshyTimerHandle);
	}
	tshyTimerHandle = StartTimer(250000, tshyTimeout);
}

void abcsp_start_tconf_timer(abcsp * thisInstance)
{
	if (confTimerHandle != NULL)
	{
		StopTimer(confTimerHandle);
	}
	confTimerHandle = StartTimer(250000, confTimeout);
}

void abcsp_cancel_bcsp_timer(abcsp * thisInstance)
{
	StopTimer(bcspTimerHandle);
	bcspTimerHandle = NULL;
}

void abcsp_cancel_tshy_timer(abcsp * thisInstance)
{
	StopTimer(tshyTimerHandle);
	tshyTimerHandle = NULL;
}

void abcsp_cancel_tconf_timer(abcsp * thisInstance)
{
	StopTimer(confTimerHandle);
	confTimerHandle = NULL;
}


/* Used as defines in config_panic.h */

void abcsp_panic(abcsp * thisInstance, unsigned panicCode)
{
	printf("abcsp_panic: %u\n", panicCode);
}


/* Used as defines in config_txmsg.h */

void abcsp_uart_sendbytes(abcsp * thisInstance, unsigned number)
{
	uint16_t num_send;
	uint8_t * buffer = thisInstance->txslip.ubuf;

	if (!UartDrv_Tx(buffer, number , &num_send))
	{
		printf("Tx buffer overrun");
	}
}

void abcsp_txmsg_init_read(ABCSP_TXMSG * message)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;
	messageBuffer->index = 0;
}

unsigned abcsp_txmsg_length(ABCSP_TXMSG * message)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;
	return messageBuffer->length;
}

char * abcsp_txmsg_getbuf(ABCSP_TXMSG * message, unsigned * bufferLength)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;
	if (messageBuffer->index >= messageBuffer->length)
	{
		*bufferLength = 0;
		return NULL;
	}
	else
	{
		*bufferLength = messageBuffer->length - messageBuffer->index;
		return &messageBuffer->buffer[messageBuffer->index];
	}
}

void abcsp_txmsg_taken(ABCSP_TXMSG * message, unsigned numberTaken)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;
	messageBuffer->index += numberTaken;
}

void abcsp_txmsg_done(abcsp * thisInstance, ABCSP_TXMSG * message)
{
	MessageBuffer * messageBuffer;

	messageBuffer = (MessageBuffer *) message;

	free(messageBuffer->buffer);
	free(messageBuffer);
}
