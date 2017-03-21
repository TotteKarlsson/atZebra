/*****************************************************************************
*	FILENAME:		PortInfoStruct.h
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Typedef for TTYInfo data structure and macros for field access
*
*	NOTES:
*	
*	AUTHOR:			A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM:	New file, data strurcture and macros are based on mtty.c code sample
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/PortInfoStruct.h-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:50   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:48   schuesla
 * Initial revision.
*
*****************************************************************************/

#ifndef __TTYInfo_H_
#define __TTYInfo_H_


/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/

typedef struct TTYInfo
{
    HANDLE hCommPort;
    HANDLE hReaderStatus;
    HANDLE hWriter ;
    DWORD  dwEventFlags;

    CHAR   chFlag, chXON, chXOFF;
    WORD   wXONLimit, wXOFFLimit;
    DWORD  fRtsControl;
    DWORD  fDtrControl;
    BOOL   fConnected,
           fCTSOutFlow, fDSROutFlow, fDSRInFlow,
           fXonXoffOutFlow, fXonXoffInFlow,
           fTXafterXoffSent;

    BYTE   bPort, bByteSize, bParity, bStopBits ;
    DWORD  dwBaudRate ;

    COMMTIMEOUTS timeoutsorig;
    COMMTIMEOUTS timeoutsnew;
} TTYInfo;


// macros for field access

#define COMDEV( x )         (x.hCommPort)
#define pCOMDEV( x )         (x->hCommPort)
#define PORT( x )           (x.bPort)
#define CONNECTED( x )      (x.fConnected)


#define BYTESIZE( x )       (x.bByteSize)
#define PARITY( x )         (x.bParity)
#define STOPBITS( x )       (x.bStopBits)
#define BAUDRATE( x )       (x.dwBaudRate)

#define TIMEOUTSORIG( x )   (x.timeoutsorig)
#define TIMEOUTSNEW( x )    (x.timeoutsnew)

#define READSTATTHREAD( x ) (x.hReaderStatus)
#define WRITERTHREAD( x )   (x.hWriter)

#define pREADSTATTHREAD( x ) (x->hReaderStatus)
#define pWRITERTHREAD( x )   (x->hWriter)

#define EVENTFLAGS( x )     (x.dwEventFlags)
#define pEVENTFLAGS( x )	 (x->dwEventFlags)

#define FLAGCHAR( x )       (x.chFlag)
#define DTRCONTROL( x )     (x.fDtrControl)
#define RTSCONTROL( x )     (x.fRtsControl)
#define XONCHAR( x )        (x.chXON)
#define XOFFCHAR( x )       (x.chXOFF)
#define XONLIMIT( x )       (x.wXONLimit)
#define XOFFLIMIT( x )      (x.wXOFFLimit)
#define CTSOUTFLOW( x )     (x.fCTSOutFlow)
#define DSROUTFLOW( x )     (x.fDSROutFlow)
#define DSRINFLOW( x )      (x.fDSRInFlow)
#define XONXOFFOUTFLOW( x ) (x.fXonXoffOutFlow)
#define XONXOFFINFLOW( x )  (x.fXonXoffInFlow)
#define TXAFTERXOFFSENT(x)  (x.fTXafterXoffSent)

// for call to PurgeComm

#define PURGE_FLAGS             (PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR) 

#endif  // __TTYInfo_H_

