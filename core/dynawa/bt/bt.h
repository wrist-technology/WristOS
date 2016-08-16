#ifndef BT_H__
#define BT_H__

#include "task_param.h"
#include "lwbt/hci.h"
#include "lwbt/bd_addr.h"

#include "bt/bt_socket.h"
#include "types.h"

#define BT_HOST_WAKE_SLEEP_TIMEOUT  500

#ifdef CFG_DEEP_SLEEP

#define BT_HOST_WAKE_BREAK_LEN      1   // !!!deep sleep -> normal > 5ms
#define BT_HOST_WAKE_PAUSE_LEN      32

#else

#define BT_HOST_WAKE_BREAK_LEN      5   // !!!deep sleep -> normal > 5ms
#define BT_HOST_WAKE_PAUSE_LEN      10

#endif

#define BCBOOT0_MASK (1 << 23)  // BC4 PIO0
#define BCBOOT1_MASK (1 << 25)  // BC4 PIO1
#define BCBOOT2_MASK (1 << 29)  // BC4 PIO4
#define BCNRES_MASK (1 << 30)

#define BC_WAKEUP_MASK (1 << 23)  // BC4 PIO0

#define BC_STATE_STOPPED             0
#define BC_STATE_STOPPING            1
#define BC_STATE_STARTED             10
#define BC_STATE_STARTING            11
#define BC_STATE_ANAFREQ_SET         20
#define BC_STATE_BAUDRATE_SET        30
#define BC_STATE_UART_HOST_WAKE_SIGNAL      40
#define BC_STATE_UART_HOST_WAKE             41
#define BC_STATE_LC_MAX_TX_POWER            50
#define BC_STATE_LC_DEFAULT_TX_POWER        51
#define BC_STATE_LC_MAX_TX_POWER_NO_RSSI    52
#define BC_STATE_PS_SET          500
#define BC_STATE_RESTARTING          1000
#define BC_STATE_READY               10000

#define BT_COMMAND_STOP     1
#define BT_COMMAND_SEND     2
#define BT_COMMAND_SET_LINK_KEY 3
#define BT_COMMAND_INQUIRY 4
#define BT_COMMAND_FIND_SERVICE 5
#define BT_COMMAND_RFCOMM_CONNECT 6
#define BT_COMMAND_RFCOMM_LISTEN 7
#define BT_COMMAND_LINK_KEY_REQ_REPLY 8
#define BT_COMMAND_LINK_KEY_REQ_NEG_REPLY 9
#define BT_COMMAND_ADVERTISE_SERVICE 10
#define BT_COMMAND_REMOTE_NAME_REQ  11

#define BT_LED      1
#if 1
#define BT_LED_START        0x40
#define BT_LED_LOW          0x0
#define BT_LED_HIGH         0x0
#elif 0
#define BT_LED_START        0x0
#define BT_LED_LOW          0x0
#define BT_LED_HIGH         0x40
#else
#define BT_LED_START        0x40
#define BT_LED_LOW          0x40
#define BT_LED_HIGH         0xff
#endif

#define BT_OK                       0
#define BT_ERR_MEM                1
#define BT_ERR_ALREADY_STARTED    10

typedef struct {
    void *record;
    uint16_t len;
} sdp_service;

typedef struct {
    uint8_t id;
    bt_socket *sock;
    union {
        void *ptr;
        uint8_t cn;
        sdp_service service;
    } param; 
} bt_command;

typedef struct {
    uint8_t id;
    bt_socket *sock;
    uint8_t error;
    union {
        void *ptr;
        uint8_t cn;
    } data; 
} bt_command_result;

#define BT_LINK_KEY_LEN     16
#define BT_BDADDR_LEN      6

/*
struct bt_bd_addr {
    uint8_t addr[BT_BDADDR_LEN];
};
*/

struct bt_link_key {
    uint8_t key[BT_LINK_KEY_LEN];
};

struct bt_bdaddr_link_key {
    struct bd_addr bdaddr;
    struct bt_link_key link_key;
};

struct bt_bdaddr_cn {
    struct bd_addr bdaddr;
    uint8_t cn;
};

bool bt_set_command_result(bt_command_result *res);

#endif /* BT_H__ */
