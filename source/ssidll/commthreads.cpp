/*****************************************************************************
*	FILENAME:      CommThreads.cpp
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Starting and Ending the CComThreads Object including:
*						Construction, Initialization, and Starting Threads, then
*						Waiting for Threads to Exit, Cleanup and Destruction.
*
*	NOTES:
*	
*	AUTHOR:        A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM:  New File
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/commthreads.cpp-arc  $
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
#include <windows.h>
#include <commctrl.h>
#include "ssidll.h"
#include "commthreads.h"
#include "comport.h"

/****************************************************************************/
/*	Local Variables **********************************************************/

/*****************************************************************************
*	SYNOPSIS:		CComThreads::CComThreads()
*
*	DESCRIPTION:	Default Constructor for the CComThreads object
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	none
*
******************************************************************************/
CComThreads::CComThreads()
{
}

/*****************************************************************************
*	SYNOPSIS:		CComThreads::CComThreads()
*
*	DESCRIPTION:	Default Destructor for the CComThreads object
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	none
*
******************************************************************************/
CComThreads::~CComThreads()
{
}

/*****************************************************************************
*	SYNOPSIS:		int CComThreads::GlobalInitialize(HWND hwnd, int nPort)
*
*	DESCRIPTION:	Sets up the state machine for the protcol and initializes
*						critical sections and events.	
*
*	PARAMETERS:		hwnd:	handle to the window that is to receive any windows 
*								messages.
*                 nPort: the com port number 
*
*	RETURN VALUE:	Error code if any error during initialization.
*	INPUTS:
*	OUTPUTS:			CComThreads object with intialized member variables.
*						
*
*	NOTES:			Critical section is initialized here even if event can't be
*						created.  GlobalCleanup() will delete them.  These two functions
*						are called only once - immediately after CComThreads construction and 
*						immediately before CComThreads destruction, in the CComPort constructor
*						and destructor.
*
*	PSEUDOCODE:		Initialize ret_val with status of no error and the hWnd data member
*						Call the function to Setup the State machine which sets the
*                 state to Idle and initialzes the lengths of all scanner input
*						storage vars to zero ( video data, barcode data, event data etc.)
*						Initialze the critical sections for access to the Writer data and state machine
*						Create the thread exit event and writer event
*						if unsuccessful,
*							Close the status message event handle and set the error code
*						Initialize the remainder of the protocol handling variables and 
*						create the packet and response timers by calling the setup protocol function.
*                 Return the status in ret_val  
*
******************************************************************************/
int CComThreads::GlobalInitialize(HWND hwnd, int nPort)
{
	int ret_val = SSICOMM_NOERROR;

	
	hWnd = hwnd;			// copy handle of window to get messages so we have it for WM_ messages 
 
	SetupStateMachine(); // no global allocation that needs to be cleaned up, no msgs sent to user

	bFlagUserInterrupt = FALSE;
   
   InitializeCriticalSection(&gcsWriterData);
   InitializeCriticalSection(&gcsStateMachine);
    //
    // thread exit event
    //
	ghThreadExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ghThreadExitEvent == NULL)
		ret_val = SSICOMM_BAD_CREATEEVENT; // don't return yet

	// write request event
	ghWriterEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (ghWriterEvent == NULL)
			ret_val = SSICOMM_BAD_CREATEEVENT; // don't return yet


	pPortInfo = NULL;

		//
		//   for ProtocolHandling Initialization
		//

	SetupProtocolHandler(nPort);  // create timers and init the array of pointers for this port
											// so timer callbacks will know which CComPort Object the timeout is for.

   return (ret_val);
}

/*****************************************************************************
*	SYNOPSIS:		void CComThreads::GlobalCleanup()
*
*	DESCRIPTION:	Deletes Timers, Critical Section and closes Exit
*						event handle.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	none
*	INPUTS:
*	OUTPUTS:			CComThreads ready for deletion
*
*	NOTES:			See notes for GlobalInitialize()
*
*	PSEUDOCODE:		Delete the ResponseTimer and PacketTimer in call to tear down protocol handler
*						Delete the critical section and close ExitEvent handle
*						Call function to Clean up the state machine which deallocates
*							storage for image data if necessary.
*
******************************************************************************/
void CComThreads::GlobalCleanup(int nPort)
{
	// global cleanup for protocolhandler - deletes timers
	TearDownProtocolHandler(nPort);
	DeleteCriticalSection(&gcsStateMachine);
   	DeleteCriticalSection(&gcsWriterData);


	if(ghThreadExitEvent)
		CloseHandle(ghThreadExitEvent);
	ghThreadExitEvent = NULL;

	if(ghWriterEvent)
		CloseHandle(ghWriterEvent);
	ghWriterEvent = NULL;

	CleanupStateMachine();	// deletes storage for image

	hWnd = NULL;


   return;
}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::StartThreads(LPVOID lpV)
*
*	DESCRIPTION:	Creates reader and writer threads
*
*	PARAMETERS:		lpV: really pointer to this for use in thread callback functions
*
*	RETURN VALUE:	status value indicating success or what went wrong
*	INPUTS:			ReaderProc, WriterProc and PortInfo
*	OUTPUTS:			PortInfo reader and writer handle members
*
*	NOTES:			This function is never called unless PortInfo has an open
*						com port. IF THE RETURN VALUE IS SSICOMM_BAD_WRITETHREAD,
*                 the reader thread is left running.  The caller should call
*						the function to break down the com port which will call
*						wait for threads to exit and will wait for the read thread
*						to exit only.
*
*	PSEUDOCODE:		Initialize the return status to no error
*						Call the function to create the reader thread and store
*							read handle in PortInfo
*						If a null value is returned, set the error code and set the
*							writer thread to null in PortInfo
*						Else
*							Call the function to create the writer thread and store
*								the handle in PortInfo
*							If a non null value is returned sleep a little bit
*							Else set the error code
*						Return the error code
*
******************************************************************************/
int CComThreads::StartThreads(LPVOID lpV)
{
	DWORD dwReadStatId;
   DWORD dwWriterId;
	int return_val = SSICOMM_NOERROR;
	CComPort *pSSI = (CComPort *)lpV;

	pPortInfo = &(pSSI->PortInfo);

	// 257 is max byes per packet - we assume no interchar delay and expect to see all the chars within this timeout
	// ... We allow some extra time also.
	nPACKETSTARTTIMEOUT =  (DWORD)(  ((280 * 1000) + (((double)pPortInfo->dwBaudRate/10) - 1)) / ((double)pPortInfo->dwBaudRate/10)) + 100;

	if(nPACKETSTARTTIMEOUT < 500)
		nPACKETSTARTTIMEOUT = 500;


   pREADSTATTHREAD(pPortInfo) = CreateThread( NULL,
                          0,
								  ReaderProc,
								  (LPVOID) pSSI,
                          0,
                          &dwReadStatId);

   if (pREADSTATTHREAD(pPortInfo) == NULL)
	{
		pWRITERTHREAD(pPortInfo) = NULL;
		return_val = SSICOMM_BAD_READTHREAD;
	}
	else
	{
		pWRITERTHREAD(pPortInfo) = CreateThread( NULL, 
                          0, 
								  WriterProc,
								  (LPVOID)pSSI,
                          0, 
                          &dwWriterId );
                   
		if (pWRITERTHREAD(pPortInfo))
		{

			Sleep(100);
			
		}
		else
		{
			return_val = SSICOMM_BAD_WRITETHREAD;  
		}
	}
   return return_val;
}



/*****************************************************************************
*	SYNOPSIS:		DWORD CComThreads::WaitForThreadsToExit( DWORD dwTimeout)  
*
*	DESCRIPTION:	Waits for thread exit event or timeout
*
*	PARAMETERS:		dwTimeout:	number of ms to wait
*
*	RETURN VALUE:	result of the wait event function
*	INPUTS:			PortInfo
*	OUTPUTS:
*
*	NOTES:			TerminateThread will be called if the event has timed out which 
*						should be treated as a fatal error by the application program.
*
*	PSEUDOCODE:		Set up the thread handle array with reader and writer thread handles
*						If both are null, return indicator that no threads are running
*						Disable the ResponseTimer and the PacketTimer so that no thread events
*							will be generated by the state machine as a result of their timing out.
*						Set the thread exit event
*						If either the reader or writer thread is null, call wait for 
*						single object for the non null thread
*						Else call wait for multiple objects for both threads
*						If the return value is not successful, 
*							set the error code
*							call the wait function for each non null thread and 
*							if the return value/s is not signaled 
*								call TerminateThread to terminate the thread/s
*						Reset the event 
*						Return the error code
*
******************************************************************************/
DWORD CComThreads::WaitForThreadsToExit( DWORD dwTimeout)  
{	
    HANDLE hThreads[2];
    DWORD  dwRes;

    hThreads[0] = pREADSTATTHREAD(pPortInfo);  
    hThreads[1] = pWRITERTHREAD(pPortInfo);

    if((hThreads[0] == NULL) && (hThreads[1] == NULL))
       return WAIT_OBJECT_0; // no threads were started, so return indicator that all threads are done


	if(ResponseTimer)
	{
		ResponseTimer->Enable(FALSE);
		ResponseTimer->SetInterval(0);
	}
	if(PacketTimer)
	{
		PacketTimer->Enable(FALSE);
		PacketTimer->SetInterval(0);
	}
	

	
	Sleep(100);

	SetEvent(ghThreadExitEvent);

   if(hThreads[0] == NULL) 
	{
      dwRes = WaitForSingleObject(pWRITERTHREAD(pPortInfo),dwTimeout);
	}
   else if (hThreads[1] == NULL) 
	{
      dwRes = WaitForSingleObject(pREADSTATTHREAD(pPortInfo), dwTimeout);
	}
   else
	{
      dwRes = WaitForMultipleObjects(2, hThreads, TRUE, dwTimeout);
	}

   switch(dwRes)
   {
		case WAIT_OBJECT_0:  // fall thru
		case WAIT_OBJECT_0 + 1: 
			dwRes = WAIT_OBJECT_0;
			break;

        
		default: // wait timeout
			if(pREADSTATTHREAD(pPortInfo))
			{
				if (WaitForSingleObject(pREADSTATTHREAD(pPortInfo), 0) == WAIT_TIMEOUT)
				{
					// Fatal Error occurred try to terminat the thread
					TerminateThread(pREADSTATTHREAD(pPortInfo),0); 
			                                              
					// This function is dangerous if thread owns a critical section, it won't be released, 
					// if thread is executing kernel32 calls when we call release, the kernel32
					// state of the thread's process may be inconsistent, 
					// if thread is manipulating the global state of a shared dll, 
					// ...state of the dll could be destroyed, affecting other users of the dll
				}
			}
			if(pWRITERTHREAD(pPortInfo))
			{
				if (WaitForSingleObject(pWRITERTHREAD(pPortInfo), 0) == WAIT_TIMEOUT)
				{
					//Fatal Error occured, try to terminate the thread
					TerminateThread(pWRITERTHREAD(pPortInfo),0);
				}
			}
         break;

    }

   ResetEvent(ghThreadExitEvent);


    return dwRes;  // error message is handled in calling function on receipt of this value
}


