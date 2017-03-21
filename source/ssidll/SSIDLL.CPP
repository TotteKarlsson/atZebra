/*****************************************************************************
*	FILENAME:
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Defines the entry points for the DLL application.
*
*	NOTES:         The API functions in this file use the SSI protocol.
*	
*	AUTHOR:        A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM: gcpdll.cpp
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/SSIDLL.CPP-arc  $
 *
 *    Rev 1.0   Nov 22 2002 13:25:56   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:50   schuesla
 * Initial revision.
*
*****************************************************************************/

/****************************************************************************/
/*	Include Files ************************************************************/

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "ssidll.h"
#include "comPort.h"

/****************************************************************************/
/*	Local Variables *********************************************************/

/* for handling multiple com ports per process */
static CRITICAL_SECTION 	csPort = {0};						/* values within SSI_Ports[] are changed within this critical section */
static CComPort*			SSI_Ports[MAX_COM_PORTS] = {0};
static int 					nProcesses = 0;						/* the critical section csPort is initialized when this goes from 0 to 1 */

#define VER_MAJOR 1
#define VER_MINOR 0

static g_nVersion = (unsigned)((VER_MAJOR <<8) + VER_MINOR);
/*****************************************************************************
*	SYNOPSIS:		static CComPort *GetSSIComHandler(int nComPort)	
*
*	DESCRIPTION:	Returns a pointer to the Com port handler object for the given com port
*
*	RETURN VALUE:	NULL if there is no handler for the com port o/w pointer to the CComPort Object.
*
*	NOTES:			The input com port number is 1..MAX_COM_PORTS but objects are stored
*                 with zero offset - ie 0..MAX_COM_PORTS -1		
*
******************************************************************************/
static CComPort *GetSSIComHandler(int nComPort)
{
	if(nComPort < 1)
		return NULL;
	if(nComPort > MAX_COM_PORTS)
		return NULL;

	return SSI_Ports[nComPort-1]; 
}

/*****************************************************************************
*	SYNOPSIS:		static BOOL AddSSIComHandler(CComPort *pSSI, int nComPort)		
*
*	DESCRIPTION:	Range checks the value of the input nComPort	and returns FALSE
*                 if invalid.  If valid, but there is already a CComPort object
*                 pointer instantiated for nComPort, FALSE is returned. Otherwise,
*                 The input pointer to the CComPort Object is stored in the 
*                 SSI_Ports array at the index nComPort -1 and TRUE is returned.
*
*	RETURN VALUE:	TRUE if the given CComPort Object pointer was successfully 
*                 stored in the SSI_Ports array.  FALSE if invalid com port number
*                 or if a pointer was already stored for nComPort.
*
*	NOTES:			The input com port number is 1..MAX_COM_PORTS but objects are stored
*                 with zero offset - ie 0..MAX_COM_PORTS -1				
*
******************************************************************************/
static BOOL AddSSIComHandler(CComPort *pSSI, int nComPort)
{
	if(nComPort < 1)
		return FALSE;
	if(nComPort > MAX_COM_PORTS)
		return FALSE;

	if(SSI_Ports[nComPort-1] != NULL)
		return FALSE;
	EnterCriticalSection(&csPort);
	SSI_Ports[nComPort-1] = pSSI;
	LeaveCriticalSection(&csPort);
	return TRUE;
}

/*****************************************************************************
*	SYNOPSIS:		static BOOL RemoveSSIComHandler( int nComPort)		
*
*	DESCRIPTION:	Range checks the value of the input nComPort	and returns FALSE
*                 if invalid.  If valid, but there is no CComPort object
*                 pointer instantiated for nComPort, FALSE is returned. Otherwise,
*                 NULL is stored in the SSI_Ports array at the index nComPort -1 and 
*                 TRUE is returned.
*
*	RETURN VALUE:	TRUE if the given CComPort Object pointer was successfully 
*                 removed from the SSI_Ports array.  FALSE if invalid com port number
*                 or if no pointer was previously stored for nComPort.
*
*	NOTES:			The input com port number is 1..MAX_COM_PORTS but objects are stored
*                 with zero offset - ie 0..MAX_COM_PORTS -1. 
*
******************************************************************************/
static BOOL RemoveSSIComHandler( int nComPort)
{
	if(nComPort < 1)
		return FALSE;
	if(nComPort > MAX_COM_PORTS)
		return FALSE;

	if(SSI_Ports[nComPort-1] == NULL)
		return FALSE;
	EnterCriticalSection(&csPort);
	SSI_Ports[nComPort-1] = NULL;
	LeaveCriticalSection(&csPort);
	return TRUE;
}

/*****************************************************************************
*	SYNOPSIS:		static int __stdcall GetLastProtocolError()
*
*	DESCRIPTION:	When a member function of the CComPort object returns FALSE, call this
*						function to get the last error set within the dll.
*
*	RETURN VALUE:	See error codes in the API include file.
*
*	NOTES:			Error codes are valid only temporarily (until the next error)
*
******************************************************************************/
static int __stdcall GetLastProtocolError(int nComPort)
{

	CComPort * SSI = GetSSIComHandler(nComPort);

	if(SSI)
		return (SSI->LastErrorCode());
	else
		return ERR_SSI_NOOBJECT;
}

/*****************************************************************************
*	SYNOPSIS:		BOOL APIENTRY DllMain( HANDLE hModule, 
*                       DWORD  ul_reason_for_call, 
*                       LPVOID lpReserved)
*					 
*	DESCRIPTION:	Generic function generated by Visual Studio.
*
******************************************************************************/
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			if(nProcesses == 0)
			{
				InitializeCriticalSection(&csPort);
				nProcesses = 1;
			}
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			{
				int i;
				CComPort *SSI;
				for(i = 1; i <= MAX_COM_PORTS; i++)
				{
				
					SSI = GetSSIComHandler(i);
					if(SSI)
					{
						SSI->g_hwnd = NULL; 
						
						SSI->ProtocolHandler->hWnd = NULL; // STOP ANY WINDOWS MESSAGES

						SSI->Disconnect(); // if already disconnected, this will do nothing
						RemoveSSIComHandler(i);
						delete SSI;
					
					}					
				}
				DeleteCriticalSection(&csPort);
				nProcesses = 0;
			}
			break;
    }
    return TRUE;
}





/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall SSIConnect(int hwnd, long Baud, int Port)
*
*	DESCRIPTION:	Instantiates the SSI Object and establishes a comm connection,
*						including starting the read/status thread and write thread.
*
*	PARAMETERS:		hwnd: handle to the window to receive WM_XXXX messages from
*								the dll.
*						Baud:	baud rate to be used for the com port, eg 9600  
*						Port: number of the com port to be used. eg for COM1: use 1
*
*	RETURN VALUE:	Zero if successful connection is created, o/w the error code
*	INPUTS:			SSI 
*	OUTPUTS:			SSI 
*
*
*	PSEUDOCODE:		Set the return value to FALSE.
*                 Call the function to see if there is alreay a CComPortObject for the port
*                 If previously connected, call the function to set the dll error code
*						Else 
*                   If the user has sent window handle 
*						      instantiate the SSI com port object, 
*						      If the object was successfully created
*                          Call the function to initialize the protocol handler object
*                          If an error code was set, 
*                             delete the CComPort object and set it's pointer to NULL
*                   If there is a CComPort Object
*						    Call the SetComPort function with the baud and port. 
*						    If the function returned true, 
*                       Call the function to set the pointer for this com port in the global com port array.
*                       Set the ret_val to TRUE and sleep for a little while for the threads to get going.
*                     Else
*                       delete the SSI object and set it's pointer to null.
*                 If the ret_val is false, call the function to get the error code and return the code 
*                 Else return zero for success.    
*                  
******************************************************************************/
SSIDLL_API int __stdcall SSIConnect( HWND hwnd, long Baud, int Port)
{
	int ret_val = FALSE;
	int error_code;

	CComPort * SSI = GetSSIComHandler(Port);


	if(SSI)		// already connected 
		SSI->SetLastError(SSICOMM_ALREADYCONNECTED);	
	else
	{
		if(hwnd) // we have valid input
		{
			SSI = new CComPort( hwnd);  // global error code may be set but no messages sent to user
			if(SSI)
			{
				SSI->SetLastError(SSI->ProtocolHandler->GlobalInitialize(hwnd, Port));
				if(SSI->LastErrorCode() != SSICOMM_NOERROR)
				{
					error_code = GetLastProtocolError(Port);
					delete  SSI;
					SSI = NULL;
				}
			}
		}

		if(SSI)	// we have a valid object
		{
         SSI->SetLastError(SSICOMM_NOERROR);	

			if(SSI->SetComPort(Baud,Port))  
			{	
				AddSSIComHandler(SSI,Port);
				Sleep(50);
				ret_val = TRUE;
			}
			else  // if SetComPort fails, there is no connection
			{
				error_code = GetLastProtocolError(Port);
				delete SSI;
				SSI = NULL;
			}
		}
	}

	if(!ret_val)
	{
		if(SSI)
			return GetLastProtocolError(Port);
		else
			return error_code;
	}
	else
		return 0; // success
	
}


/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall SSIDisconnect(HWND hwnd, int nComPort)
*
*	DESCRIPTION:	Sends the disconnect commands to the scanner and deletes the SSI object.			
*
*	PARAMETERS:		hwnd:	handle to window that the object was created with.
*                 nComPort: the com port number
*
*	RETURN VALUE:	Zero if connection was terminated successfully o/w the erro code
*	INPUTS:			SSI 
*	OUTPUTS:			SSI (global error code is set if unsuccessful)
*
*	NOTES:			If a fatal system error occurs (threads unable to be stopped), 
*						the SSI Object will not be deleted and the window handle
*						will be null. A new connection will not be possible.  The user
*						should terminate their program, and when the dll terminates, the
*						SSI object will be deleted.
*
*	PSEUDOCODE:		Set the ret_val to FALSE
*                 Get the pointer to the object for this com port from the global com port array.
*                 If there is SSI an SSI object connected
*						  If the input handle isn't null and matches the window handle for the com port object
*							  Initialize the error code and set object's window handle to null
*							  Call Disconnect function 
*							  if the disconnect function returned true	
*								  delete the SSI com object and call the function to remove it from the global com port array.
*								  set ret_val to true
*                   Else if the the input handle dosen't match the objects handle or if it's NULL, set an error code  
*                 If ret_val is TRUE return zero o/w return the error code 
******************************************************************************/
SSIDLL_API int __stdcall SSIDisconnect( HWND   hwnd, int nComPort)
{
   int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);

	if(SSI)
	{
		if(SSI->g_hwnd && (SSI->g_hwnd == (HWND)hwnd) )  
		{
         SSI->SetLastError(SSICOMM_NOERROR);	
	
			SSI->g_hwnd = NULL;  

			SSI->ProtocolHandler->hWnd = NULL; // STOP ANY WINDOWS MESSAGES


		   SSI->Disconnect(); 
         delete SSI; 
			RemoveSSIComHandler(nComPort);
			ret_val = TRUE;	
		}
		else if(SSI->g_hwnd != (HWND)hwnd)  
			SSI->SetLastError(ERR_SSI_MISMATCHHWND);  
      else if(!SSI->g_hwnd)  
         SSI->SetLastError(ERR_SSI_HWND);

	}
	
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}


/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall TransmitVideo(nComPort)
*
*	DESCRIPTION:	Sends the SSI Command to the scanner to set the imager mode
*						to video mode ( as opposed to decode or image capture mode).
*						On the next trigger pull, the imager will send video frames
*						on the com port.
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent successfully, otherwise, the error code
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if error occurred
*
*	NOTES:			
*
*	PSEUDOCODE:		Set ret_val to false and get the SSI Object for the port
*                 If there is an SSI Object 
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   ...seting the return value to that function's return value 
*                       If ret_val is zero, set the error code to busy
*                     Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
******************************************************************************/
SSIDLL_API int __stdcall TransmitVideo(int nComPort)
{
	int ret_val = FALSE;
	unsigned char operational_mode = VIDEO_OPERATION;
	CComPort * SSI = GetSSIComHandler(nComPort);

	if(SSI)
	{
		if(SSI->g_hwnd && COMDEV(SSI->PortInfo)) 
		{      
			// When this is issued, video mode is enabled.
			// On trigger pulled, video stream starts.
			// On Trigger Release, video stops.
			// If trigger is not pulled withing scanner timeout, video mode will revert back to barcode scanning mode

			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,IMAGER_MODE, &operational_mode,1);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
   }
  
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success
}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall SetParameters(unsigned char *Params, int ParamBytes, int nComPort)
*
*	DESCRIPTION:	Sends the sent paramter string to the scanner.
*
*	PARAMETERS:		Params:	buffer of user data in paramnum/value pairs
*						ParamBytes:	length of Params in number's of bytes.
*                 nComPort: the com port number
*
*	RETURN VALUE:	TRUE if parameter command was sent successfully to scanner.
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*	NOTES:			The maximum number of parameters that may be changed is
*						limited by as many as fits in a max length SSI packet.
*						WORD values are stored as hi byte, lo byte.
*
*	PSEUDOCODE:		Set the ret_val to FALSE
*                 Check the input buffer for correct format while copying
*						the data to a temp buffer.
*						If there is an SSI Object
*						   if there is a valid com device and window handle
*                       if the data was bad
*                          Set the error code
*                       Else
*							      Clear the error code 
*							      Call the function to send the command to the scanner
*							      ...seting the return value to that function's return value 
*                          If ret_val is zero, set the error code to busy
*                     Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
******************************************************************************/
SSIDLL_API int __stdcall SetParameters(unsigned char *Params, int ParamBytes, int nComPort)
{
   int ret_val = FALSE;
	unsigned char tempbuf[255]; // reformatted data for SSI
	int i = 0;
	int j = 0;
	int bWordIndicator;	//  indicator that a param has a WORD value 
	CComPort * SSI = GetSSIComHandler(nComPort);


	// no beep indicator used but it is first data byte for SSI param send packet
	tempbuf[i++] = 0xff;  

	if(ParamBytes <= 0)	// bad format
		j = ParamBytes;

	while(j < ParamBytes)
	{

		bWordIndicator = *Params == 0xf4;
		// indicator that a param has a WORD value 
		if(bWordIndicator) 
		{
			
			j++;
			if( j == ParamBytes ) // bad format
			{
				j = 0;
				break;
			}
			tempbuf[i++] = *Params++; // copy the word indicator
		}

		// check for extended parameter code -=- 2 bytes
		if((*Params == 0xF0) || (*Params == 0xF1) || (*Params == 0xF2))
		{
			
			j++;
			if( j == ParamBytes ) // bad format
			{
				j = 0;
				break;
			}
			tempbuf[i++] = *Params++; // copy over the extended param number

		}
		
		j++;
		if( j == ParamBytes ) // bad format
		{
			j = 0;
			break;
		}
		tempbuf[i++] = *Params++; // copy offset or a non-extended param number

		// copy the parameter value
		if(bWordIndicator) 
		{
			
			j++;
			if( j == ParamBytes ) // bad format
			{
				j = 0;
				break;
			}
			tempbuf[i++] = *Params++; 

		}
		j++;
		tempbuf[i++] = *Params++; 
		
	}
	

   if(SSI)
   {
      if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
      {
			if((j <= 0) || (j != ParamBytes))
				SSI->SetLastError(SSICOMM_BADDATA);
			else if(ParamBytes < (SSI_MAX_PACKET_LENGTH - SSI_HEADER_LEN - SSI_CHECKSUM_LEN))
			{
				SSI->SetLastError(SSICOMM_NOERROR);	
				
				ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,PARAM_SEND, tempbuf,i);
				if(ret_val == 0)
					SSI->SetLastError(SSICOMM_BUSY);
			}
         else
            SSI->SetLastError(SSICOMM_TOOMUCHDATA);	
      }
      else
        SSI->SetLastError(ERR_SSI_HWND);
   }
	

	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success


}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall RequestParameters(unsigned char *Params, int ParamBytes, int nComPort)
*
*	DESCRIPTION:	Checks input length for length overrun then calls function
*						to send the request for the given parameters to the scanner.
*
*	PARAMETERS:		Params: Sequence of parameter numbers whose values are desired.
*						ParamBytes:	Length of Params in bytes.
*                 nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*	NOTES:			The maximum length of Param bytes is determined by
*						the max length of the SSI packet.  No check is done
*						on input data except for max length check.
*						Command is only sent here.  When info comes back from scanner
*						a windows message is sent to the application.			
*
*	PSEUDOCODE:		Set the ret_val to FALSE
*						If there is an SSI Object
*						   if there is a valid com device and window handle
*                       if the data was good length
*							      Clear the error code 
*							      Call the function to send the command to the scanner
*							      ...seting the return value to that function's return value 
*                          If ret_val is zero, set the error code to busy
*                        Else set the error code
*                     Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success	
*
******************************************************************************/
SSIDLL_API int __stdcall RequestParameters(unsigned char *Params, int ParamBytes, int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd && COMDEV(SSI->PortInfo)) 
		{
			if(ParamBytes < (SSI_MAX_PACKET_LENGTH - SSI_HEADER_LEN - SSI_CHECKSUM_LEN))
			{
				SSI->SetLastError(SSICOMM_NOERROR);	

				ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_PARAMDATAREQ_EVENT, PARAM_REQUEST, Params, ParamBytes );				
				if(ret_val == 0)
					SSI->SetLastError(SSICOMM_BUSY);
			}
         else
            SSI->SetLastError(SSICOMM_TOOMUCHDATA);	
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}


/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall RequestAllParameters(int nComPort)
*
*	DESCRIPTION:	Sends command to request the scanner to send
*						all parameters with their values.
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*	NOTES:			Command is only sent here.  When info comes back from scanner
*						a windows message is sent to the application.
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
******************************************************************************/
SSIDLL_API int __stdcall RequestAllParameters(int nComPort)
{
	int ret_val = FALSE;
   unsigned char cmd = SSI_PARAM_ALL;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	

			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_PARAMDATAREQ_EVENT, PARAM_REQUEST, &cmd, 1);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
   }
   if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}




/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall AbortImageXfer(int nComPort) 
*
*	DESCRIPTION:	Commands the scanner to cancel the current image data (not video) transfer
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if the command was sent to scanner successfully o/w the error codde
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
**	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false but no error code was set during processing, 
*                          Set ret_val to true since there was nothing to abort
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
******************************************************************************/
SSIDLL_API int __stdcall AbortImageXfer(int nComPort)  
{
   int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
   {
      if(SSI->g_hwnd  && COMDEV(SSI->PortInfo))
      {
         SSI->SetLastError(SSICOMM_NOERROR);	

         ret_val = SSI->ProtocolHandler->SendAbortCommands();
			if((!ret_val) && (SSI->LastErrorCode() == SSICOMM_NOERROR ))
				ret_val = TRUE; // if the command was not handled, it can only mean that there was nothing to abort
      }
      else
        SSI->SetLastError(ERR_SSI_HWND);

	}
	// else a call to get the error code will return no object or no hwnd error
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success


}



/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall PullTrigger(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to start a session which
*						will act like a software trigger.
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
**	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to input queue full
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
******************************************************************************/
SSIDLL_API int __stdcall PullTrigger(int nComPort)
{
	int ret_val = FALSE;
   CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
      {
			SSI->SetLastError(SSICOMM_NOERROR);	
			
			ret_val = SSI->ProtocolHandler->ServiceTriggerPull();
			if(!ret_val)
				SSI->SetLastError(SSI_INPUTQ_FULL);
      }
      else
        SSI->SetLastError(ERR_SSI_HWND);

	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}



/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall ReleaseTrigger(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to stop a session which acts like
*						a software trigger release.
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to input queue full
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall ReleaseTrigger(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);

   
	
   if(SSI)
   {
      if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
      {
         SSI->SetLastError(SSICOMM_NOERROR);

			ret_val = SSI->ProtocolHandler->ServiceTriggerRelease();
			if(!ret_val)
				SSI->SetLastError(SSI_INPUTQ_FULL);
      }
      else
        SSI->SetLastError(ERR_SSI_HWND);

   }
   if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  AimOn(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to turn on the aiming light
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall AimOn(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT, AIM_ON, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  AimOff(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to turn off the aiming light
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall AimOff(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,AIM_OFF, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  IlluminiationOn(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to turn on the aiming light
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value
*                       if ret_val is false,
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall IlluminationOn(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo))
		{
			SSI->SetLastError(SSICOMM_NOERROR);

			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT, ILLUMINATION_ON, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  IlluminiationOff(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to turn off the aiming light
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value
*                       if ret_val is false,
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall IlluminationOff(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo))
		{
			SSI->SetLastError(SSICOMM_NOERROR);

			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT, ILLUMINATION_OFF, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  LedOn(int nComPort, unsigned char nLEDselection)
*
*	DESCRIPTION:	Sends command to scanner to turn on the scanner led
*
*	PARAMETERS:		nComPort: the com port number
*                 nLEDselection: the bit numbers for leds to turn off
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall LedOn(int nComPort, unsigned char nLEDselection )
{
	int ret_val = FALSE;
	unsigned char operational_mode = nLEDselection;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,LED_ON, &operational_mode,1);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  LedOff(int nComPort, unsigned char nLEDselection)
*
*	DESCRIPTION:	Sends command to scanner to turn off the scanner led
*
*	PARAMETERS:		nComPort: the com port number
*                 nLEDselection: the bit numbers for leds to turn off
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall LedOff(int nComPort, unsigned char nLEDselection)
{
	int ret_val = FALSE;
	unsigned char operational_mode = nLEDselection;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,LED_OFF, &operational_mode,1);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success


}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  ScanEnable(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to enable scanning
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall ScanEnable(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,SCAN_ENABLE, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  ScanDisable(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to disable scanning
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall ScanDisable(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,SCAN_DISABLE, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success


}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  SoundBeeper(int nComPort, unsigned char nBeepCode)
*
*	DESCRIPTION:	Sends command to scanner to turn on the beeper using the sent nBeepCode
*
*	PARAMETERS:		nComPort: the com port number
*                 nBeepCode: which type of beep to use 
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall SoundBeeper(int nComPort, unsigned char nBeepCode)
{
	int ret_val = FALSE;
	unsigned char operational_mode = nBeepCode;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,BEEP, &operational_mode,1);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  EnterLowPwrMode(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to go to low power mode
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall EnterLowPwrMode(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,SLEEP, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success


}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  RequestScannerCapabilities(int nComPort)
*
*	DESCRIPTION:	Sends a command to the scanner to send it's capabilities - ie the commands it supports.
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall RequestScannerCapabilities(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CAPDATAREQ_EVENT,CAPABILITIES_REQUEST, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success
}


/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall SnapShot(nComPort)
*
*	DESCRIPTION:	Sends the SSI Command to the scanner to set the imager mode
*						to image capture mode ( as opposed to decode or video mode).
*						On the next trigger pull, the imager will send image data
*						on the com port.	
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*	NOTES:			The format of the image data may be set by a param send
*						before making this call.  The imager will stay in this mode
*						until the next trigger pull and resulting image capture or until
*						a timeout if no trigger pull occurrs within the timeout period.
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
******************************************************************************/
SSIDLL_API int __stdcall SnapShot(int nComPort)
{
	int ret_val = FALSE;
	unsigned char operational_mode = SNAPSHOT_OPERATION;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,IMAGER_MODE, &operational_mode,1);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}



/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall TransmitVersion(int nComPort)
*
*	DESCRIPTION:	Sends the command to request the version number from the scanner.
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if successful o/w the error code
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*	NOTES:			When the version string has been sent, a windows message will be sent to the application.
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
******************************************************************************/
SSIDLL_API int __stdcall TransmitVersion(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);

	
	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{

			/* 
			* build ssi command then call transmit 
			*/
			SSI->SetLastError(SSICOMM_NOERROR);	
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_REVDATAREQ_EVENT, REQUEST_REVISION, NULL,0);
			if(!ret_val)
				SSI->SetLastError(SSICOMM_BUSY);
			
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success


}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall SetParamPersistance(int nComPort, int bPersist)
*
*	DESCRIPTION:	Sets the persistance value for the com port.  When any param change command is sent, this value
*                 will be used to tell the scanner if it is to be a temporary or permanent change.
*
*	PARAMETERS:		nComPort: the com port number
*                 bPersist: TRUE if parameters are to be set permanently, FALSE if temporary change only
*
*	RETURN VALUE:	Zero if successful o/w the error code
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*	NOTES:			When the version string has been sent, a windows message will be sent to the application.
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Set the persistance value for the com port
*							   set the ret_val to TRUE 
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
******************************************************************************/
SSIDLL_API int __stdcall SetParamPersistance(int nComPort, int bPersist)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);

	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{

			/* 
			* build ssi command then call transmit 
			*/
			SSI->SetLastError(SSICOMM_NOERROR);	
			ret_val = TRUE;
			SSI->ProtocolHandler->bPersist = bPersist;

			
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success


}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  AbortMacroPdf(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to abort macro pdf
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall AbortMacroPdf(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);

	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,ABORT_MACRO_PDF, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall  FlushMacroPdf(int nComPort)
*
*	DESCRIPTION:	Sends command to scanner to abort the macro pdf sequence and send any macro pdf data 
*                 that was decoded.
*
*	PARAMETERS:		nComPort: the com port number
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
*
*	PSEUDOCODE:		Set ret_val to FALSE
*                 If there is an SSI Object
*						   if there is a valid com device and window handle
*							   Clear the error code 
*							   Call the function to send the command to the scanner
*							   seting the ret_val to that function's return value 
*                       if ret_val is false, 
*                          Set the error code to busy
*                    Else set the error code
*						If the ret_val is false return the error code 
*                 Else return zero for success
*
*
******************************************************************************/
SSIDLL_API int __stdcall FlushMacroPdf(int nComPort)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = SSI->ProtocolHandler->ServiceUserCommand(USER_CMD_EVENT,FLUSH_MACRO_PDF, NULL,0);
			if(ret_val == 0)
				SSI->SetLastError(SSICOMM_BUSY);
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API int __stdcall SetVideoBuffer(int nComPort, unsigned char *pData, long max_length)
*                 SSIDLL_API int __stdcall SetDecodeBuffer(int nComPort, unsigned char *pData, long max_length)
*                 SSIDLL_API int __stdcall SetImageBuffer(int nComPort, unsigned char *pData, long max_length)
*                 SSIDLL_API int __stdcall SetVersionBuffer(int nComPort, unsigned char *pData, long max_length)
*                 SSIDLL_API int __stdcall SetCapabilitiesBuffer(int nComPort, unsigned char *pData, long max_length)
*                 SSIDLL_API int __stdcall SetParameterBuffer(int nComPort, unsigned char *pData, long max_length)
*
*	DESCRIPTION:	Sets the user's buffer pointed to by pData, of length max_length to be used to copy Scanner Data
*                 from the dll back to the user's application. 
*
*	PARAMETERS:		nComPort: the com port number
*                 pData: pointer to the user's data buffer
*                 max_length: max number of data bytes to place in the buffer
*
*	RETURN VALUE:	Zero if command was sent to scanner successfully o/w the error code 	
*	INPUTS:			SSI
*	OUTPUTS:			The global error code will be set if unsuccessful send
*
* Note: the application may use one buffer for all data types to be returned IF the application makes sure the use
*       of the buffer will be mutually exclusive.
*
*
******************************************************************************/
SSIDLL_API int __stdcall SetVideoBuffer(int nComPort, unsigned char *pData, long max_length)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = TRUE;
			SSI->ProtocolHandler->pGlobalVideoData = pData;
			SSI->ProtocolHandler->GlobalVideoDataLen = max_length;
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}



SSIDLL_API int __stdcall SetDecodeBuffer(int nComPort, unsigned char *pData, long max_length)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = TRUE;
			SSI->ProtocolHandler->pGlobalDecodeData = pData;
			SSI->ProtocolHandler->GlobalDecodeDataLen = max_length;
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

SSIDLL_API int __stdcall SetImageBuffer(int nComPort, unsigned char *pData, long max_length)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = TRUE;
			SSI->ProtocolHandler->pGlobalImageData = pData;
			SSI->ProtocolHandler->GlobalImageDataLen = max_length;
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

SSIDLL_API int __stdcall SetVersionBuffer(int nComPort, unsigned char *pData, long max_length)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = TRUE;
			SSI->ProtocolHandler->pGlobalVersionData = pData;
			SSI->ProtocolHandler->GlobalVersionDataLen = max_length;
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}

SSIDLL_API int __stdcall SetCapabilitiesBuffer(int nComPort, unsigned char *pData, long max_length)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = TRUE;
			SSI->ProtocolHandler->pGlobalCapabilitiesData = pData;
			SSI->ProtocolHandler->GlobalCapabilitiesDataLen = max_length;
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}
SSIDLL_API int __stdcall SetParameterBuffer(int nComPort, unsigned char *pData, long max_length)
{
	int ret_val = FALSE;
	CComPort * SSI = GetSSIComHandler(nComPort);


	if(SSI)
	{
		if(SSI->g_hwnd  && COMDEV(SSI->PortInfo)) 
		{
			SSI->SetLastError(SSICOMM_NOERROR);	
				
			ret_val = TRUE;
			SSI->ProtocolHandler->pGlobalParamData = pData;
			SSI->ProtocolHandler->GlobalParamDataLen = max_length;
		}
      else
        SSI->SetLastError(ERR_SSI_HWND);
	}
	if(!ret_val)
		return GetLastProtocolError(nComPort);
	else
		return 0; // success

}



/*****************************************************************************
*	SYNOPSIS:		SSIDLL_API unsigned int __stdcall ReturnDLLVersion(void)
*
*	DESCRIPTION:	Returns the version of the dll - ver minor is in lower byte, 
*                 ver major is in second byte
******************************************************************************/
SSIDLL_API unsigned int __stdcall ReturnDLLVersion(void)
{
	return g_nVersion;
}