/*****************************************************************************
*	FILENAME:		Writer.cpp
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Serial Com Port Write routines, including both the Writer Thread 
*						routine, and the Entry point	WriteBuffer for adding write requests.
*
*	NOTES:
*	
*	AUTHOR:			A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM: mtty.c code sample and subsequent gcpdll implementation by Mike O'Haire
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/WRITER.CPP-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:26:00   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:52   schuesla
 * Initial revision.
*
*****************************************************************************/

/****************************************************************************/
/*	Include Files ***********************************************************/

#include <windows.h>
#include "ssidll.h"
#include "CommThreads.h"
#include "comport.h"


static int WriterGeneric(LPVOID lpV, char * lpBuf, DWORD dwToWrite);
static BOOL HandleWriteRequests(LPVOID lpV);



/*****************************************************************************
*	SYNOPSIS:		BOOL CComThreads::WriteBuffer(BYTE *buffer, DWORD ByteCount)                         
*
*	DESCRIPTION:	Stores a write request then sets the write request event
*
*	PARAMETERS:		ByteCount:	number of data bytes to store
*						buffer:	the data to store in write request buffer	
*
*	RETURN VALUE:	TRUE if write request event was successfully serviced
*	INPUTS:			gcsWriterData
*	OUTPUTS:			gWriteRequest
*
*	NOTES:
*
*	PSEUDOCODE:		Enter the writer request critical section
*						If the packet queue is not empty - last request not serviced -
*							Leave the critical section and return false
*						Else
*							copy the input information to the write request data struct
*							leave the critical section
*							Set the request event
*							If the event was not successfully set,
*								Enter the critical section again and remove the request
*								Leave the critical section and return false 
*							Else
*								return TRUE
*
******************************************************************************/

BOOL CComThreads::WriteBuffer(BYTE *buffer, DWORD ByteCount)
{

   PWRITEREQUEST pWrite = &gWriteRequest;

	EnterCriticalSection(&gcsWriterData);
	if(PacketQInsertIndex) // not done sending last message - THIS SHOULD NEVER HAPPEN UNLESS WE GOT DISCONNECTED PHYSICALLY
	{
		LeaveCriticalSection(&gcsWriterData);
		return FALSE;
	}

	pWrite->dwSize = ByteCount;
   memcpy(pWrite->lpBuf,buffer, ByteCount);


	PacketQInsertIndex++;
	LeaveCriticalSection(&gcsWriterData);

	if (!SetEvent(ghWriterEvent))
	{
		EnterCriticalSection(&gcsWriterData);
		PacketQInsertIndex--;
		LeaveCriticalSection(&gcsWriterData);
		return FALSE;
	}
	return TRUE;
}

/*****************************************************************************
*	SYNOPSIS:		DWORD WINAPI WriterProc(LPVOID lpV)
*
*	DESCRIPTION:	Thread function controling write requests to com port
*
*	PARAMETERS:		lpV: void pointer that is cast to pointer to the CComPort object 
*						
*
*	RETURN VALUE:	Returns TRUE when the thread exits. 
*	INPUTS:
*	OUTPUTS:			pPortInfo
*
*	NOTES:			The SSI protocol only has 1 write request ever in the queue
*						since each transmission from us, the host, requires a
*						response from the scanner in the form of an ack, nak or
*						data satisfying our request for same.
*						
*
*	PSEUDOCODE:		Store the write request and exit event handles in hArray
*						Set the fDone flag to false and cast the input pointer to pointer to CComPort object
*						While not done
*							call the wait for multiple objects function
*							if the function returns indication of exit event
*								set not done to false to exit loop
*							if the function returns indication of write request event
*								call the function to handle the write request
*								if that function returns false, set fDone to true
*						Return TRUE
*						
*
******************************************************************************/
DWORD WINAPI WriterProc(LPVOID lpV)
{
	HANDLE hArray[2];
	DWORD dwRes;
	BOOL fDone = FALSE;

	CComPort *pSSI = (CComPort *)lpV;

	pSSI->ProtocolHandler->pPortInfo = &(pSSI->PortInfo);
			
	hArray[0] = pSSI->ProtocolHandler->ghWriterEvent;
	hArray[1] = pSSI->ProtocolHandler->ghThreadExitEvent;
   
    
	while ( !fDone ) 
	{
		dwRes = WaitForMultipleObjects(2, hArray, FALSE, WRITE_CHECK_TIMEOUT);
		switch(dwRes)
		{
			case WAIT_TIMEOUT:  /* ignore - should never happen*/
				break;

			case WAIT_FAILED:
				{  // may not be fatal - state machine should retry

				//pSSI->ProtocolHandler->OnFatalError(SSICOMM_WAITMOWRITER); // FOR DEBUGGING - should never happen

				break;
				}	//
					// write request event
					//
			case WAIT_OBJECT_0:
				EscapeCommFunction(pCOMDEV(pSSI->ProtocolHandler->pPortInfo),SETRTS); // from setrts to clrdtr
				{
					fDone = !(HandleWriteRequests(pSSI));  // returns false if exit event occurred during handling 
				}
				break;
					//
					// thread exit event
					//
			case WAIT_OBJECT_0 + 1:
				fDone = TRUE;
				break;
		}
	}

	return 1;
}

/*****************************************************************************
*	SYNOPSIS:		BOOL HandleWriteRequests(LPVOID lpV)
*
*	DESCRIPTION:	Disables or enables the response timer depending on the
*						value of the response timer interval then
*						calls the writer function with the stored data in the
*						write request, after which it clears the write request so a
*						new write request may be issued.
*
*	PARAMETERS:		lpVoid cast to CComPort pointer
*
*	RETURN VALUE:	TRUE if the write request was handled (even if an error 
*						occurred), FALSE if the exit thread event was detected
*						while doing the write operation.
*	INPUTS:			gcsWriterData
*						gWriteRequest
*	OUTPUTS:			PacketQInserIndex
*
*
******************************************************************************/
BOOL HandleWriteRequests(LPVOID lpV)
{
	PWRITEREQUEST pWrite;
   CComPort *pSSI = (CComPort *)lpV;


	if(pSSI->ProtocolHandler->ResponseTimer)  
	{
		pSSI->ProtocolHandler->ResponseTimer->Enable(FALSE);
		// if interval was set to zero - timer will not be enabled - no timeout for message we are sending out now
		pSSI->ProtocolHandler->ResponseTimer->Enable(TRUE);
	}


	pWrite = &(pSSI->ProtocolHandler->gWriteRequest);
	if(!WriterGeneric(pSSI, (char *)pWrite->lpBuf, pWrite->dwSize))
		return FALSE; // exit thread was encountered, no need to decrement packetq index

	EnterCriticalSection(&(pSSI->ProtocolHandler->gcsWriterData));
	pSSI->ProtocolHandler->PacketQInsertIndex--;
	LeaveCriticalSection(&(pSSI->ProtocolHandler->gcsWriterData));


	return TRUE;

}

/*****************************************************************************
*	SYNOPSIS:		int WriterGeneric(LPVOID lpV, char * lpBuf, DWORD dwToWrite)
*
*	DESCRIPTION:	Actually does the write to the comm port.
*
*	PARAMETERS:		lpV: cast here to point to the CComPort Object that wants to write.
*                 lpBuf:		data to write to the comm port
*						dwToWrite:	number of bytes to write 
*
*	RETURN VALUE:	False if the exit event was asserted, o/w true even on errors
*	INPUTS:
*	OUTPUTS:			Fatal error messages are written as WM_ERROR messages to user
*
*	NOTES:			If user receives a fatal error message, API disconnect function
*						should be called.  This function returns TRUE even if there are
*						errors detected.  It will only return FALSE if the exit thread event
*						is asserted.
*
*	PSEUDOCODE:		Set the ret_val to TRUE
*						Create the overlapped io write event
*						If unsuccessful,
*							send a fatal error message to user as WM_ERROR
*						Else
*							fill hArray with the write	and exit event handles
*							Issue the write file 
*							If the return value is false
*								Get the error from the system
*								If the error was io pending meaning write is delayed
*									Call the function to wait for multiple objects 
*									If the return value indicates a write event
*										Set the last error to success
*										Call the function to get the overalpped result
*										If the return value is false
*											Get the last error from the OS 
*											If it doesn't indicated write was aborted,
*												Send the user a fatal error message
*										If the number of chars written  is not the number desired
*											Get the last error code from the OS
*											If it's not success, then while debugging
*												Send a non fatal error messag to user - depend on scanner to nak resend
*									Else if the return value indicates a thread termination event
*										Set ret_val to false
*									Else if the return value indicates a wait timeout 
*										Ignore it since this should never happen on infinite wait
*									Else 
*										Send a fatal windows message to user
*								Else
*									Send fatal window message to user that read wasn't delayed, but write failed 
*							Else write returned immediately, so....
*								If	the number of bytes written is not the number desired then while debugging	
*									Send a non fatal error messag to user - depend on scanner to nak resend
*							Close the event handle 
*						Return ret_val							
*
******************************************************************************/
int WriterGeneric(LPVOID lpV, char * lpBuf, DWORD dwToWrite)
{
	OVERLAPPED osWrite = {0};
	HANDLE hArray[2];
	DWORD dwWritten = 0;
	DWORD dwRes;
	unsigned char pUSERNAKCANCELMSG[] = {0x05, 0xd1, 0x04, 0x00, 0x0a, 0xff, 0x1c};
	CComPort *pSSI = (CComPort *)lpV;


#define USER_CMD_NAKCANCEL_LEN 7

	int ret_val = TRUE;

	// debug stuff
	if(dwToWrite < 4)
		return 0;

	 //
    // create this writes overlapped structure hEvent
    //
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL)
		pSSI->ProtocolHandler->OnFatalError(SSITHREAD_CREATEWEVENT);  
															// we send fatal error to user - user should call disconnect 
	else
	{
		BOOL bImmediateReturn;

		hArray[0] = osWrite.hEvent;
		hArray[1] = pSSI->ProtocolHandler->ghThreadExitEvent;
    
    //
    // issue write
    //
		if(pSSI->ProtocolHandler->bFlagUserInterrupt) // see notes in state machine for this flag
			bImmediateReturn = WriteFile(pCOMDEV(pSSI->ProtocolHandler->pPortInfo), pUSERNAKCANCELMSG, USER_CMD_NAKCANCEL_LEN, &dwWritten, &osWrite);
		else
			bImmediateReturn = WriteFile(pCOMDEV(pSSI->ProtocolHandler->pPortInfo), lpBuf, dwToWrite, &dwWritten, &osWrite);
		
		if(!bImmediateReturn)
		{
			if (GetLastError() == ERROR_IO_PENDING) // write is delayed
			{ 
            
				dwRes = WaitForMultipleObjects(2, hArray, FALSE, INFINITE);
				switch(dwRes)
				{
                //
                // write event set
                //
					case WAIT_OBJECT_0:
						pSSI->SetLastError(ERROR_SUCCESS);  
						if (!GetOverlappedResult(pCOMDEV(pSSI->ProtocolHandler->pPortInfo), &osWrite, &dwWritten, FALSE)) 
						{
							if (GetLastError() != ERROR_OPERATION_ABORTED)
							{
								pSSI->ProtocolHandler->OnFatalError(SSITHREAD_OLRESW); 
								// we send fatal error to user - user should call disconnect 
							}
							
						}
                  // state machine should recover from this - uncomment for debugging         
						//if (dwWritten != dwToWrite) 
						//	if (GetLastError() != ERROR_SUCCESS)
						//		pSSI->ProtocolHandler->OnError(SSITHREAD_WRITEERR);
					
						break;

                //
                // thread exit event set
                //
					case WAIT_OBJECT_0 + 1:
						ret_val = FALSE;
						break;

                //                
                // wait timed out
                //
					case WAIT_TIMEOUT: // won't happen
						break;               

					case WAIT_FAILED:
					default:    
						pSSI->ProtocolHandler->OnFatalError(SSITHREAD_WMOW);
						// we send fatal error to user - user should call disconnect 
					break;
				}
			}
			else  // last error wasn't io pending    
			{
            //
            // writefile failed, but it isn't delayed
            //
				pSSI->ProtocolHandler->OnFatalError(SSITHREAD_WRITEFILEFAIL);  
			// we send fatal error to user - user should call disconnect 	
			}
		}
		else
		{
        //
        // writefile returned immediately
        //

			 // scanner will nak resend us - uncomment for debugging
		//	if (dwWritten != dwToWrite)
		//		pSSI->ProtocolHandler->OnError(SSITHREAD_WRITEERR);
		}
		CloseHandle(osWrite.hEvent);
	} // end else we have a good handle

	return(ret_val);
}


