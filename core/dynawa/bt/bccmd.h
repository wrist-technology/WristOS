#ifndef BCCMD_H__
#define BCCMD_H__

#define BCCMD_CHANNEL	2

#define BCCMDPDU_STAT_OK    0

#define BCCMDPDU_GETREQ             0
#define BCCMDPDU_GETRESP            1
#define BCCMDPDU_SETREQ             2

#define BCCMDVARID_CHIPVER          0x281a
#define BCCMDVARID_COLD_RESET       0x4001
#define BCCMDVARID_WARM_RESET       0x4002
#define BCCMDVARID_PS               0x7003

#define PSKEY_BDADDR                0x1
#define PSKEY_ANAFREQ               0x1fe
#define PSKEY_BAUDRATE              0x1be
#define PSKEY_UART_CONFIG_BCSP      0x1bf
#define PSKEY_UART_HOST_WAKE_SIGNAL     0x01ca
#define PSKEY_UART_HOST_WAKE            0x01c7
#define PSKEY_LC_MAX_TX_POWER           0x17  
#define PSKEY_LC_DEFAULT_TX_POWER       0x21
#define PSKEY_LC_MAX_TX_POWER_NO_RSSI   0x2d

#endif /* BCCMD_H__ */
