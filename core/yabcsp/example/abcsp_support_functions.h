#ifndef ABCSP_SUPPORT_FUNCTIONS_H__
#define ABCSP_SUPPORT_FUNCTIONS_H__

#include "abcsp.h"

typedef struct
{
	unsigned length;
	unsigned index;
	unsigned char * buffer;
} MessageBuffer;

extern abcsp AbcspInstanceData;

extern void abcsp_delivermsg(abcsp * thisInstance, ABCSP_RXMSG * message, unsigned channel, unsigned reliableFlag);
extern ABCSP_RXMSG * abcsp_rxmsg_create(abcsp * thisInstance, unsigned length);
extern char * abcsp_rxmsg_getbuf(ABCSP_RXMSG * message, unsigned * bufferSize);
extern void abcsp_rxmsg_write(ABCSP_RXMSG * message, char * buffer, unsigned number);
extern void abcsp_rxmsg_complete(abcsp * thisInstance, ABCSP_RXMSG * message);
extern void abcsp_rxmsg_destroy(abcsp * thisInstance, ABCSP_RXMSG * message);

extern void abcsp_event(abcsp * thisInstance, unsigned event);
extern void abcsp_req_pumptxmsgs(abcsp * thisInstance);

extern void abcsp_start_bcsp_timer(abcsp * thisInstance);
extern void abcsp_start_tshy_timer(abcsp * thisInstance);
extern void abcsp_start_tconf_timer(abcsp * thisInstance);
extern void abcsp_cancel_bcsp_timer(abcsp * thisInstance);
extern void abcsp_cancel_tshy_timer(abcsp * thisInstance);
extern void abcsp_cancel_tconf_timer(abcsp * thisInstance);

extern void abcsp_panic(abcsp * thisInstance, unsigned panicCode);

extern void abcsp_uart_sendbytes(abcsp * thisInstance, unsigned number);
extern void abcsp_txmsg_init_read(ABCSP_TXMSG * message);
extern unsigned abcsp_txmsg_length(ABCSP_TXMSG * message);
extern char * abcsp_txmsg_getbuf(ABCSP_TXMSG * message, unsigned * bufferLength);
extern void abcsp_txmsg_taken(ABCSP_TXMSG * message, unsigned numberTaken);
extern void abcsp_txmsg_done(abcsp * thisInstance, ABCSP_TXMSG * message);

#endif /* ABCSP_SUPPORT_FUNCTIONS_H__ */
