#ifndef BT_SOCKET_H_
#define BT_SOCKET_H_

#include "lwbt/rfcomm.h"

#define BT_SOCKET_STATE_INITIALIZED         1
#define BT_SOCKET_STATE_CONNECTING          2
#define BT_SOCKET_STATE_CONNECTED           3
//#define BT_SOCKET_STATE_DISCONNECTING     4
#define BT_SOCKET_STATE_DISCONNECTED        5
#define BT_SOCKET_STATE_RFCOMM_LISTENING    6

#define BT_SOCKET_ERR_OK                1

typedef struct {
    uint8_t proto;
    uint16_t state;
    uint8_t current_cmd;
    struct rfcomm_pcb *pcb;
    uint8_t cn;
    uint8_t sdp_record;
} bt_socket;

#endif /* BT_SOCKET_H_ */
