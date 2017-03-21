/*****************************************************************************
*	FILENAME:		comPort.h
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	CComPort Class for Serial Communication
*
*	NOTES:
*	
*	AUTHOR:			A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM:	New File 
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/COMPORT.H-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:44   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:46   schuesla
 * Initial revision.
*
*****************************************************************************/

#ifndef __COMPORT_H__
#define __COMPORT_H__

/****************************************************************************/
/*	Nested Include Files ****************************************************/

#include "CommThreads.h"		// for reader and writer threads  
#include "PortInfoStruct.h"	// for storing serial communication settings

/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/
class CComPort
{
	public:
        					CComPort(HWND Hwnd);
        virtual 			~CComPort();
        BOOL                SetComPort(long Baud, int port);
        BOOL                Disconnect();
        int 				LastErrorCode();
        void 				SetLastError(int code);

        /* data */
        TTYInfo 			PortInfo;
        CComThreads*		ProtocolHandler;

        HWND 				g_hwnd;

	private:
        int 				g_commErrorCode;
        OSVERSIONINFO 		m_OSVersion;



        BOOL             	VersionCheck(void);
        void             	InitTTYInfo(void);
        BOOL             	UpdateConnection(void);
        BOOL             	BreakDownCommPort();
        int 				Connect();
        void 				CheckComStat(void);
};

#endif
