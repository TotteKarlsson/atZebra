
#pragma once

#define PARAM_RETURN_DATA_LEN 2048
#define PARAM_SEND_DATA_LEN 32
#define DECODE_DATA_LEN 8192

#define BARCODE_COM_PORT 3


#define PARM_CODE_128           0x08
#define PARM_XMIT_NO_READ_MSG   0x5E
#define PARM_POWER_MODE         0x80
#define PARM_TRIGGER_MODE       0x8A
#define PARM_HOST_RESP_TIMEOUT  0x9B
#define PARM_BAUD_RATE          0x9C
#define PARM_PARITY             0x9E
#define PARM_SOFT_HANDSHAKE     0x9F
#define PARM_PARAM_SCAN         0xEC
#define PARM_DECODE_FORMAT      0xEE
#define PARM_HOST_CHAR_TIMEOUT  0xEF

#define PARM_EX_F0              0xF0
#define PARM_EX_DECODE_EVENT    0x00
#define PARM_EX_BOOTUP_EVENT    0x02
#define PARM_EX_PARAM_EVENT     0x03



void InitializeScannerThread(HWND myWindow);
void ScanBarcodeThread(void);
