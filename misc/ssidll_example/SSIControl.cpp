
#include "stdafx.h"
#include "SSIControl.h"
#include "SSIDLL.H"


extern unsigned char DecodeData[DECODE_DATA_LEN];
extern unsigned char ParamReturnData[PARAM_RETURN_DATA_LEN];
extern unsigned char ParamSendData[PARAM_SEND_DATA_LEN];
extern HANDLE  g_hCmdComplete;
extern HANDLE  g_hParams;

static int result;
static TCHAR SSImessage[128];


//-----------------------------------------------------------------------------
// Read and Write configuration parameters into the barcode scanner
//-----------------------------------------------------------------------------
void InitializeScannerThread(HWND myWindow)
{
    // Prepare the parameter return buffer
    memset(ParamReturnData,0,PARAM_RETURN_DATA_LEN);
    result = SetParameterBuffer(BARCODE_COM_PORT, ParamReturnData, PARAM_RETURN_DATA_LEN);
    if(result)
    {   // failed, give up
        wsprintf(SSImessage, L"SetParameterBuffer ERROR number %d", result);
        AfxMessageBox(SSImessage);    
        return;
    }

    //-----------------------------------------------------------------
    // load some default configurations
    //-----------------------------------------------------------------
    // put the parameter and its data into the array to ship it out
    memset(ParamSendData,0,PARAM_SEND_DATA_LEN);
    ParamSendData[0] = PARM_SOFT_HANDSHAKE;
    ParamSendData[1] = 0x01;
    ParamSendData[2] = PARM_DECODE_FORMAT;
    ParamSendData[3] = 0x01;
    ParamSendData[4] = PARM_POWER_MODE;
    ParamSendData[5] = 0x01;
    ParamSendData[6] = PARM_TRIGGER_MODE;
    ParamSendData[7] = 0x08;
    ParamSendData[8] = PARM_XMIT_NO_READ_MSG;
    ParamSendData[9] = 0x01;
    ParamSendData[10] = PARM_PARAM_SCAN;
    ParamSendData[11] = 0x01;
    ParamSendData[12] = PARM_HOST_CHAR_TIMEOUT;
    ParamSendData[13] = 0x14;
    ParamSendData[14] = PARM_HOST_RESP_TIMEOUT;
    ParamSendData[15] = 0x14;
    ParamSendData[16] = PARM_EX_F0;
    ParamSendData[17] = PARM_EX_DECODE_EVENT;
    ParamSendData[18] = 0x00;
    ParamSendData[19] = PARM_EX_F0;
    ParamSendData[20] = PARM_EX_BOOTUP_EVENT;
    ParamSendData[21] = 0x00;
    ParamSendData[22] = PARM_EX_F0;
    ParamSendData[23] = PARM_EX_PARAM_EVENT;
    ParamSendData[24] = 0x00;

    // send the parameters
    result = SetParameters( ParamSendData, 25, BARCODE_COM_PORT);
    if(result)
    {   // failed, give up
        wsprintf(SSImessage, L"SetParameters ERROR number %d", result);
        AfxMessageBox(SSImessage);    
        return;
    }

    // wait for message to know when this has completed
    WaitForSingleObject(g_hCmdComplete, INFINITE);

    //-----------------------------------------------------------------
    // read some configurations back to see what they are
    //-----------------------------------------------------------------
    // Morpheus Hardware takes a bit more time before it is ready
    Sleep(500);

    // prepare the list of parameters to read
    memset(ParamReturnData,0,PARAM_RETURN_DATA_LEN);
    memset(ParamSendData,0,PARAM_SEND_DATA_LEN);
    ParamSendData[0] = PARM_SOFT_HANDSHAKE;
    ParamSendData[1] = PARM_DECODE_FORMAT;
    ParamSendData[2] = PARM_POWER_MODE;
    ParamSendData[3] = PARM_TRIGGER_MODE;
    ParamSendData[4] = PARM_XMIT_NO_READ_MSG;
    ParamSendData[5] = PARM_PARAM_SCAN;
    ParamSendData[6] = PARM_BAUD_RATE;
    ParamSendData[7] = PARM_PARITY;
    ParamSendData[8] = PARM_HOST_CHAR_TIMEOUT;
    ParamSendData[9] = PARM_HOST_RESP_TIMEOUT;
    ParamSendData[10] = PARM_CODE_128;
    //ParamSendData[11] = PARM_EX_F0;
    //ParamSendData[12] = PARM_EX_DECODE_EVENT;
    //ParamSendData[13] = PARM_EX_F0;
    //ParamSendData[14] = PARM_EX_BOOTUP_EVENT;
    //ParamSendData[15] = PARM_EX_F0;
    //ParamSendData[16] = PARM_EX_PARAM_EVENT;

    // Can't do anymore than 16 bytes!!! it just hangs up for some reason

    // send the command
    result = RequestParameters( ParamSendData, 11, BARCODE_COM_PORT);
    if(result)
    {   // failed, give up
        wsprintf(SSImessage, L"RequestParameters ERROR number %d", result);
        AfxMessageBox(SSImessage);    
        return;
    }

    // wait for the signal to know when it is finished
    WaitForSingleObject(g_hParams, INFINITE);

    // let the user know the scanner is done booting up
    wsprintf(SSImessage, L"Scanner is ready");
    AfxMessageBox(SSImessage);    
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Perform the commands needed to scan a barcode and retrieve its data
//-----------------------------------------------------------------------------
void ScanBarcodeThread(void)
{
    // Need to enable scan mode in the module
    result = ScanEnable(BARCODE_COM_PORT);
    if(result)
    {   // failed, give up
        wsprintf(SSImessage, L"ScanEnable ERROR number %d", result);
        AfxMessageBox(SSImessage);    
        return;
    }
    WaitForSingleObject(g_hCmdComplete, INFINITE);

    // Prepare the decode buffer that will eventually hold the symbol data
    memset(DecodeData, 0, DECODE_DATA_LEN);
    result = SetDecodeBuffer(BARCODE_COM_PORT, DecodeData, DECODE_DATA_LEN);
    if(result)
    {   // failed, give up
        wsprintf(SSImessage, L"SetDecodeBuffer ERROR number %d", result);
        AfxMessageBox(SSImessage);    
        return;
    }

    // Send the command to pull the trigger
    result = PullTrigger(BARCODE_COM_PORT);
    if(result)
    {   // failed, give up
        wsprintf(SSImessage, L"PullTrigger ERROR number %d", result);
        AfxMessageBox(SSImessage);    
        return;
    }
    WaitForSingleObject(g_hCmdComplete, INFINITE);
}
//-----------------------------------------------------------------------------
