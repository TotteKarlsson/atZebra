/*****************************************************************************
*	FILENAME:
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:   Com Port Data Structure Initialization, Connection
*                 and Disconnection of Com Port.
*
*	NOTES:
*	
*	AUTHOR:
*	CREATION DATE: 10/28/02
*	DERIVED FROM: mtty.c code sample and CommThread.cpp by M.O'Haire
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/COMPORT.CPP-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:44   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:46   schuesla
 * Initial revision.
*
*****************************************************************************/


/****************************************************************************/
/*	Include Files ***********************************************************/
#include "windows.h"
#include "ssidll.h"
#include "comport.h"
#include <string>
using namespace std;
/****************************************************************************/
/*	Defines, typedefs, etc. **************************************************/

#define DEFAULT_READINTERVAL_T_O	1 // max time to elapse between arrival of 
#define WIN32_READINTERVAL_T_O		5 // 2 chars beginning with 1st bit of 1st char

#define EVENTFLAGS_DEFAULT      	(EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD)
#define FLAGCHAR_DEFAULT        	0

/*****************************************************************************
*	SYNOPSIS:      CComPort::CComPort(HWND hwnd)
*
*	DESCRIPTION:	Constructor for CComPort Object
*
*	PARAMETERS:		hwnd:	handle of the window that should receive any
*								Windows messages from the dll.
*
*	RETURN VALUE:	none
*	INPUTS:			none
*	OUTPUTS:			Initialized CComPort Object
*
*	NOTES:         Initializes but does not connect the comm port 
*                 Connect() must be called to do the connection.
*
*	PSEUDOCODE:		Initialize the g_hwnd member with the input hwnd value
*						Initialize the g_commErrorCode to no errors
*						Call the function to check if this version can support the
*							dll
*						if not, 
*							set the error code to version error 
*							call the Init function for the com data structure so it 
*							will be set to not connected and return
*						if the version can support the dll, Init the data structure
*							for the com port then create the protocol handler 
*
******************************************************************************/
CComPort::CComPort(HWND hwnd)
{
	g_commErrorCode = SSICOMM_NOERROR;
   	g_hwnd = hwnd;

   	if (!VersionCheck())  // if fails, sets windows version to ver 3.1 which doesn't allow the dll to run
	{
        // can't run on this version of Windows */
      g_commErrorCode = SSICOMM_ERRVERSION;
      InitTTYInfo();  // no error codes set, port is NOT CONNECTED
      return;
	}

   	InitTTYInfo();
	ProtocolHandler = new CComThreads();  // null constructor
}


/*****************************************************************************
*	SYNOPSIS:		CComPort::~CComPort()
*
*	DESCRIPTION:	Call the Protocol Handler's cleanup function then delete it.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	none
*	INPUTS:			m_OSVersion
*						ProtocolHandler
*	OUTPUTS:			ProtocolHandler
*
*	NOTES:			If the OS version is not supported by the dll, then 
*						there is nothing to clean up.
*
******************************************************************************/
CComPort::~CComPort()
{

   if(m_OSVersion.dwPlatformId != VER_PLATFORM_WIN32s) 
	{
	   ProtocolHandler->GlobalCleanup(PORT( PortInfo ) ); 
		delete (ProtocolHandler); 
	
	}
}



/*****************************************************************************
*	SYNOPSIS:		BOOL CComPort::VersionCheck(void)
*
*	DESCRIPTION:	Verifies that the correct version of Windows is running
*                 	otherwise the dll is not suppported.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if version supports the dll, o/w FALSE
*
*	NOTES:			If we are unable to obtain the version, the value is set to
*					an unsupported version.
*
******************************************************************************/
/* verifies that the correct version of Windows is running */

BOOL CComPort::VersionCheck(void)
{
	m_OSVersion.dwOSVersionInfoSize = sizeof(m_OSVersion);
	if (!GetVersionEx(&m_OSVersion))
		m_OSVersion.dwPlatformId = VER_PLATFORM_WIN32s;

   if (m_OSVersion.dwPlatformId == VER_PLATFORM_WIN32s)
      return FALSE;

    return TRUE ;
}

/*****************************************************************************
*	SYNOPSIS:		void CComPort::InitTTYInfo(void)
*
*	DESCRIPTION:	Initialize the data structure for all com port values
*						leaving it in "UNCONNECTED" state with thread handles
*                 	NULL
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	none
*	INPUTS:
*	OUTPUTS:		TTYInfo
*
******************************************************************************/
void CComPort::InitTTYInfo(void)
{
    COMMTIMEOUTS TimeoutsDefault;

    TimeoutsDefault.ReadIntervalTimeout = DEFAULT_READINTERVAL_T_O;
    TimeoutsDefault.ReadTotalTimeoutMultiplier = 0;
    TimeoutsDefault.ReadTotalTimeoutConstant = 0;
    TimeoutsDefault.WriteTotalTimeoutMultiplier = 0;
    TimeoutsDefault.WriteTotalTimeoutConstant = 0;

    COMDEV( PortInfo )        = NULL ;
    CONNECTED( PortInfo )     = FALSE ;

    PORT( PortInfo )          = 0 ; // invalid value
    BAUDRATE( PortInfo )      = CBR_115200;
    BYTESIZE( PortInfo )      = DATABITS_8 ;
    PARITY( PortInfo )        = NOPARITY ;
	STOPBITS( PortInfo )      = ONESTOPBIT ;

    //
    // timeouts
    //
	//for win95,98, etc... delay read interval as per GCP DLL
	if(m_OSVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		TimeoutsDefault.ReadIntervalTimeout = WIN32_READINTERVAL_T_O;
	};

   	TIMEOUTSNEW( PortInfo )   = TimeoutsDefault;

    //
    // read state and status events
    //

   	EVENTFLAGS( PortInfo )    = EVENTFLAGS_DEFAULT;

	FLAGCHAR( PortInfo )      = FLAGCHAR_DEFAULT;

    //
    // Flow Control Settings
    //

	DTRCONTROL( PortInfo )    = DTR_CONTROL_DISABLE;
   	RTSCONTROL( PortInfo )    = RTS_CONTROL_DISABLE;

   	XONLIMIT( PortInfo )      = 0;
   	XOFFLIMIT( PortInfo )     = 0;


	CTSOUTFLOW( PortInfo )    = TRUE;

   	DSROUTFLOW( PortInfo )    = FALSE;
   	DSRINFLOW( PortInfo )     = FALSE;
   	XONXOFFOUTFLOW(PortInfo)  = FALSE;
   	XONXOFFINFLOW(PortInfo)   = FALSE;
   	TXAFTERXOFFSENT(PortInfo) = FALSE;



	WRITERTHREAD(PortInfo) = NULL;
	READSTATTHREAD(PortInfo) = NULL;
}

/*****************************************************************************
*	SYNOPSIS:      BOOL CComPort::SetComPort(long baud, int port )
*
*	DESCRIPTION:   This is never called for an open com port.
*                 Sets the input port and baud rate values and calls
*                 the function to make a new serial connection.
*
*	PARAMETERS:    baud - the baud rate in bits/sec
*                 port - port number to open eg. 1 for COM1:
*
*	RETURN VALUE:  TRUE if successful, o/w FALSE
*	INPUTS:        m_OSVersion
*                 PortInfo
*
*	OUTPUTS:       PortInfo
*                 g_commErrorCode
*
*	NOTES:         This function always returns FALSE under Windows 3.1, and 
*                 does not attempt to disconnect or connect in that case.                
*
******************************************************************************/
BOOL CComPort::SetComPort(long baud, int port )
{

	if(m_OSVersion.dwPlatformId == VER_PLATFORM_WIN32s)
	{
		g_commErrorCode = SSICOMM_ERRVERSION;
		return FALSE;
	}


   PORT( PortInfo )          = (BYTE)port;
   BAUDRATE( PortInfo )      = baud;

	if(!Connect())
		return FALSE;        // no connection, no threads running

	return TRUE;
}

/*****************************************************************************
*	SYNOPSIS:      BOOL CComPort::Connect()
*
*	DESCRIPTION:	Calls CreateFile to open the com port
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if connected
*	INPUTS:			PortInfo
*	OUTPUTS:			g_commErrorCode			
*
*	NOTES:			If there was a problem starting the threads and the com port 
*						was closed again, then this will return False.
*
*	PSEUDOCODE:		Build the comport name from the portnumber 
*						Call CreateFile to open the comport for overlapped r/w
*						If an invalid handle is returned, set the error code and return FALSE, non-fatal
*						Call GetCommTimeouts and save the values in the PortInfo struct 
*						If unsuccessful
*							set the error code, close the comport, set the handle to null and return FALSE
*						Call the function to update the connection with the values in PortInfo for baud
*						parity, timeouts etc.
*						If unsuccessful
*							set the error code, close the comport, set the handle to null and return FALSE
*						Else set connected to true in PortInfo				
*						Call the function to clear DTR and all port error status codes
*						Call the function to start the read and write threads
*						If the return is not successful,
*							Call the function to break down the comm port and return false
*						Else call the function to set DTR and return TRUE 
*												
******************************************************************************/
BOOL CComPort::Connect()
{

    int pNr = PORT( PortInfo );

    char portNr[25];//("COM" + toString(pNr));
    string pNrStr;
	itoa(pNr, portNr, 10);
	if(pNr > 10)
    {
    	pNrStr = string("\\\\.\\COM") + string(portNr);
    }
    else
    {
	    pNrStr = "COM" + string(portNr);
    }

    //
    // open communication port handle
    //
    COMDEV( PortInfo ) = CreateFile( pNrStr.c_str(), //commport,
                                      GENERIC_READ | GENERIC_WRITE, 
                                      0, 
                                      0, 
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                      0);

	if (COMDEV(PortInfo) == INVALID_HANDLE_VALUE)
	{
		g_commErrorCode = SSICOMM_BAD_CREATEFILE; 
		COMDEV(PortInfo) = NULL;
		return FALSE;
	}

    //
    // Save original comm timeouts and set new ones
    //
    if (!GetCommTimeouts( COMDEV(PortInfo), &(TIMEOUTSORIG(PortInfo))))
	 {
		 g_commErrorCode = SSICOMM_BAD_GETTIMEOUTS;
		 CloseHandle(COMDEV(PortInfo));
		 COMDEV(PortInfo) = NULL;
		 return FALSE;
    }

    //
    // Set port state
    //
    if(!UpdateConnection())  // PortInfo values stored in DCB then SetCommState called
	 {
		 CloseHandle(COMDEV(PortInfo));
		 COMDEV(PortInfo) = NULL;
		 return FALSE;
    }

    //
    // set comm buffer sizes
    //
   if(!SetupComm(COMDEV(PortInfo), MAX_READ_BUFFER, MAX_WRITE_BUFFER))
	{
		SetCommTimeouts(COMDEV(PortInfo),  &(TIMEOUTSORIG(PortInfo))); // try to reset original timeouts
		CloseHandle(COMDEV(PortInfo));
		COMDEV(PortInfo) = NULL;
		return FALSE;
   }
	CONNECTED( PortInfo) = TRUE ;
    //
    // start threads and set initial thread state to not done
    //

	EscapeCommFunction(COMDEV(PortInfo), CLRDTR);
	CheckComStat();

   if((g_commErrorCode = ProtocolHandler->StartThreads(this)) != SSICOMM_NOERROR)                      //
   {
      BreakDownCommPort(); // undo the connection we just did above - 
      return FALSE;        // ...just return the false from start threads.  There is no connection.
   }


	EscapeCommFunction(COMDEV(PortInfo), SETDTR);

   return TRUE;
}

/*****************************************************************************
*	SYNOPSIS:		BOOL CComPort::UpdateConnection(void)
*
*	DESCRIPTION:	Gets the current state of the com port DCB structure,
*						saves it, then changes the values for baud, byte size
*						parity, stop bits, EvtChar, flow control settings, fParity
*						and calls the function to set the state of the com port
*						according to the new settings, then sets the timeout 
*						values to the new timeout values.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if sucssfully gets and sets the DCB values and timouts.
*	INPUTS:			PortInfo
*	OUTPUTS:			the global error code may be set to an error code
*
*	NOTES:
*
*	PSEUDOCODE:		Get the current DCB settings
*						If unsuccessful, set the error code and return false
*						Reset the values for baud, byte size, parity, stop bits, EvtChar
*						flow control settings and fParity according to the values in PortInfo
*						Call SetCommState to set the new DCB settings
*						If unsuccessful, set the error code and return false
*						Call the function to set the timeout values to the new ones in PortInfo
*						If unsuccessful, 
*							call SetCommState with the saved DCB settings, set the error code and return false
*						Else return true
*
******************************************************************************/
BOOL CComPort::UpdateConnection(void)
{
	DCB dcb = {0};
	DCB old_dcb;
    
   dcb.DCBlength = sizeof(dcb);

    //
    // get current DCB settings
    //
   if (!GetCommState(COMDEV(PortInfo), &dcb))
	{
		g_commErrorCode = SSICOMM_BAD_GETCOMSTATE;
		return(FALSE);
	}
	old_dcb = dcb;

   //
   // update DCB rate, byte size, parity, and stop bits size
   //
   dcb.BaudRate = BAUDRATE(PortInfo);
   dcb.ByteSize = BYTESIZE(PortInfo);
   dcb.Parity   = PARITY(PortInfo);
   dcb.StopBits = STOPBITS(PortInfo);
   //
   // update event flags
   //
   if (EVENTFLAGS(PortInfo) & EV_RXFLAG)
		dcb.EvtChar = FLAGCHAR(PortInfo);
   else
		dcb.EvtChar = '\0';

   //
   // update flow control settings
   //
   dcb.fDtrControl     = DTRCONTROL(PortInfo);
   dcb.fRtsControl     = RTSCONTROL(PortInfo);
   dcb.fOutxCtsFlow    = CTSOUTFLOW(PortInfo);
   dcb.fOutxDsrFlow    = DSROUTFLOW(PortInfo);
   dcb.fDsrSensitivity = DSRINFLOW(PortInfo);
   dcb.fOutX           = XONXOFFOUTFLOW(PortInfo);
   dcb.fInX            = XONXOFFINFLOW(PortInfo);
   dcb.fTXContinueOnXoff = TXAFTERXOFFSENT(PortInfo);
	// let XonChar and XoffChar  keep their old values since we're not using xon, xoff
   dcb.XonLim          = XONLIMIT(PortInfo);
   dcb.XoffLim         = XOFFLIMIT(PortInfo);

   dcb.fParity = TRUE;
	dcb.fBinary = TRUE;  // this must be true - only allowed value for windows
	dcb.fErrorChar = FALSE;
	dcb.fAbortOnError = FALSE;
	dcb.fNull = FALSE;
	dcb.EofChar = 0;
	
   
	//
   // set new state
   //
   if (!SetCommState(COMDEV(PortInfo), &dcb))
	{
		g_commErrorCode = SSICOMM_BAD_SETSTATE;
		return(FALSE);
	}

    //
    // set new timeouts
    //
	if (!SetCommTimeouts(COMDEV(PortInfo),  &(TIMEOUTSNEW(PortInfo))))
	{
		g_commErrorCode = SSICOMM_BAD_SETTIMEOUTS;
		SetCommState(COMDEV(PortInfo), &old_dcb); // restore orig settings
		return(FALSE);
	}
   return TRUE;
}


/*****************************************************************************
*	SYNOPSIS:      BOOL CComPort::Disconnect()
*
*	DESCRIPTION:   Checks the device field of the PortInfo record
*                 before calling BreakDownCommPort function.
*
*	PARAMETERS:    none
*
*	RETURN VALUE:  If there is a null serial device, the function returns 
*                 true - nothing to disconnect. 
*                 Otherwise,the return value of BreakDownCommPort is
*                 returned.
*
******************************************************************************/
BOOL CComPort::Disconnect()
{
	if(COMDEV(PortInfo))  
		return BreakDownCommPort();

	return(true);
}




/*****************************************************************************
*	SYNOPSIS:      BOOL CComPort::BreakDownCommPort()
*
*	DESCRIPTION:   Returns true if there was no connection to close, o/w
*                 calls PurgeComm before calling 
*                 WaitForThreadsToExit(). After that, deasserts DTR, 
*                 restores the original COMM timeouts, then purges io again.
*                 Closes the handle to the port and both read and writer 
*                 threads, setting them to NULL in the PortInfo sturcture.
*                 Returns TRUE unless the threads were unable to exit.
*
*	PARAMETERS:    none
*
*	RETURN VALUE:  Returns false on fatal error - threads were unable to exit
*	INPUTS:        PortInfo
*	OUTPUTS:       g_commErrorCode
*                 PortInfo
*
*	NOTES:         Returns false on fatal error - threads were unable to exit
*                 It is possible for this function to return TRUE with a 
*                 non-fatal error having occured.
*
*	PSEUDOCODE:    Check that the com port is connected and if not, return true.
*                 O/W  purge all i/o and read/write 
*                 buffers before calling WaitForThreadsToExit(). 
*                 Call function to deassert DTR then the function to restore 
*                 the comm timeouts. 
*						Purge again, then call the function to
*                 close the port handle, read handle and write handle and set
*                 each of them to NULL.  
*						If at any point an unsuccessful value was returned, 
*							set the appropriate error code in g_commErrorCode.
*                 Return FALSE if the call to WaitForThreadsToExit returned an 
*                 unsuccessful value, o/w return true. 
*
******************************************************************************/
BOOL CComPort::BreakDownCommPort()
{
	if(!CONNECTED(PortInfo))
	{
		g_commErrorCode = SSICOMM_ALREADY_CLOSED;
		return TRUE; 
	}


   if (!PurgeComm(COMDEV(PortInfo), PURGE_FLAGS))
	{
      if(g_commErrorCode == SSICOMM_NOERROR)  // don't overwrite a previous error
		   g_commErrorCode = SSICOMM_UNABLE_PURGE;
      // we won't return false here because it's a non fatal error 
	};

   if (ProtocolHandler->WaitForThreadsToExit( 20000) != WAIT_OBJECT_0)
   {
      /*
      **    If threads haven't exited, then they will
      **    interfere with a new connection.  I must abort
      **    the entire program.
      **    Set a fatal error, but we'll continue closing the com port
      */
	   g_commErrorCode = SSICOMM_THREADS_BADEXIT;
     
   }

	// if threads exited correctly, then they deallocated the writer heap 
   if (!EscapeCommFunction(COMDEV(PortInfo), CLRDTR)) // deassert DTR
	{
      // don't overwrite a previous error 
      if(g_commErrorCode == SSICOMM_NOERROR)  
		   g_commErrorCode = SSICOMM_ERROR_CLRDTR;  // not fatal
	}
	
   // restore comm timeouts
   if (!SetCommTimeouts(COMDEV(PortInfo),  &(TIMEOUTSORIG(PortInfo)) ))
	{
      if(g_commErrorCode == SSICOMM_NOERROR)  // don't overwrite a previous error
		   g_commErrorCode = SSICOMM_BAD_SETTIMEOUTS;  // not fatal
	}
	
    // Purge reads/writes, input buffer and output buffer
   if (!PurgeComm(COMDEV(PortInfo), PURGE_FLAGS))
	{
      if(g_commErrorCode == SSICOMM_NOERROR)  // don't overwrite a previous error 
		   g_commErrorCode = SSICOMM_UNABLE_PURGE; // not fatal
	}


   	CloseHandle(COMDEV(PortInfo));
	COMDEV(PortInfo) = NULL;
   	CONNECTED( PortInfo ) = FALSE;

	if(READSTATTHREAD(PortInfo))
		CloseHandle(READSTATTHREAD(PortInfo));
	READSTATTHREAD(PortInfo) = NULL;

	if(WRITERTHREAD(PortInfo))
		CloseHandle(WRITERTHREAD(PortInfo));
	WRITERTHREAD(PortInfo) = NULL;

	if(g_commErrorCode == SSICOMM_THREADS_BADEXIT)
		return FALSE;

   return TRUE;
}

/*****************************************************************************
*	SYNOPSIS:      void CComPort::CheckComStat(void)
*
*	DESCRIPTION:   Checks for a non-null com port device before calling the
*                 ClearCommError function.
*
*	RETURN VALUE:  none
*
******************************************************************************/
void CComPort::CheckComStat(void)
{
   COMSTAT ComStatNew;
   DWORD dwErrors;

	if(COMDEV(PortInfo))
		ClearCommError(COMDEV(PortInfo), &dwErrors, &ComStatNew);

    return;
}


/*****************************************************************************
*	SYNOPSIS:      int CComPort::LastErrorCode(void )
*
*	DESCRIPTION:   Returns the current value of the error code for the dll.
*
*	NOTES:         The error code is reset during dll execution whenever an
*                 error is detected.
*
******************************************************************************/
int CComPort::LastErrorCode(void )
{
	return g_commErrorCode;
}

/*****************************************************************************
*	SYNOPSIS:      void CComPort::SetLastError(int code)
*	DESCRIPTION:   Sets the current value of the error code for the dll.
*
*	PARAMETERS:    code:  The error code to be stored in g_commErrorCode.
*
*	NOTES:         The error code may be set at any point during dll execution.
*
******************************************************************************/
void CComPort::SetLastError(int code)
{
	g_commErrorCode = code;
}
