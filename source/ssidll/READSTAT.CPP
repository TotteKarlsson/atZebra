/*****************************************************************************
*	FILENAME:	Readstat.cpp
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Thread function for reading the com port.	
*
*	NOTES:
*	
*	AUTHOR:
*	CREATION DATE: 10/28/02
*	DERIVED FROM: mtty.c code sample
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/READSTAT.CPP-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:56   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:50   schuesla
 * Initial revision.
*
*****************************************************************************/

/****************************************************************************/
/*	Include Files ***********************************************************/

#include <windows.h>

#include "ssidll.h"
#include "commthreads.h"
#include "comPort.h"

#define SSI_SOURCE_DECODER 0x00
static void InterpretPacket (LPVOID lpV, BYTE *stream, unsigned long Length);




/*****************************************************************************
*	SYNOPSIS:		DWORD WINAPI ReaderProc(LPVOID lpV)
*
*	DESCRIPTION:	Com port read thread function.
*
*	PARAMETERS:		lpV:	pointer to void cast to pointer to the CComPort object
*
*	RETURN VALUE:	Returns TRUE if thread exited normally
*	INPUTS:
*	OUTPUTS:
*
*
*	PSEUDOCODE:		Cast the input pointer to point to the CComPort object
*                 Call the function to get the OSVersion information
*						initialzie fThreadNotDone, fWaitingOnRead and fWaitingOnStat to FALSE
*						Set pPortInfo to the PortInfo struct for the CComPort object
*						Create the reader and status events 
*						Store those events and the thread exit event in hArray
*						While fThreadNotDone
*							If not fWaitingOnRead
*								Issue the read
*								If the read returned false
*									Call the function to get the last error from the OS
*									If the last error was not io pending meaning it was not delayed
*										Send a fatal WM_ERROR to the user and exit the thread
*									Else
*										set fWaitingOnRead to TRUE
*								Else the read completed immediatediately, so...
*									if there are characters to be processed, 
*										call InterpretPacket
*							If not fWaitingOnStatus
*								Issue the status request
*								If the status request was unsuccessful
*									Call the function to get the last error from the OS
*									If the last error was not io pending meaning it was not delayed
*										Send a fatal WM_ERROR to the user 
*									Else
*										set fWaitingOnStatus to TRUE
*								Else the status request completed immediatediately so not waiting on status and
*									if the status was transmit buffer empty, clear request to send
*							If fWaitingOnStatus AND fWaitingOnRead
*								Call the function to wait for multiple objects with both events
*								If the function returns read event was signaled,
*									Call the GetOverlappedResult function
*									If it was unsuccessful, 
*										get the error code						
*										if error code wasn't operation aborted,
*											send user fatal WM_ERROR message 
*									Else 
*										if there were chars read, call the function to process the input
*									SetfWaitingRead to FALSE
*								Else if the function returns status event was signaled	
*									Call GetOverlappedResult function
*									If it was unsuccessful, 
*										get the error code						
*										if error code wasn't operation aborted,
*											send user fatal WM_ERROR message 
*									Else
*										if the status was transmit buffer empty, clear RTS
*									SetfWaitingOnStat to FALSE
*								Else if the function returns thread exit event
*									set fThreadDone to TRUE
*								Else if the function returns WAIT_TIMEOUT
*									call the function to clear the comm errors
*								Else ignore the return value - this should never happen
*
*						Close the reader and status events	
*						Return TRUE
*
******************************************************************************/

DWORD WINAPI ReaderProc(LPVOID lpV)
{
    OVERLAPPED osReader = {0};  // overlapped structure for read operations
    OVERLAPPED osStatus = {0};  // overlapped structure for status operations
    HANDLE     hArray[NUM_READSTAT_HANDLES];
    DWORD      dwStoredFlags = 0xFFFFFFFF;      // local copy of event flags
    DWORD      dwCommEvent;     // result from WaitCommEvent
    DWORD      dwOvRes;         // result from GetOverlappedResult
    DWORD 	   dwRead;          // bytes actually read
    DWORD      dwRes;           // result from WaitForSingleObject
    BOOL       fWaitingOnRead = FALSE;
    BOOL       fWaitingOnStat = FALSE;
    BOOL       fThreadDone = FALSE;
    char   	   lpBuf[AMOUNT_TO_READ];
	unsigned long LastError;
	int WaitCommFlag;
	CComPort *pSSI = (CComPort *)lpV;	
	TTYInfo *pPortInfo;

	OSVERSIONINFO OSVersionInfo;
	
	OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSVersionInfo);	
	
	pPortInfo = &(pSSI->PortInfo);

    //
    // create two overlapped structures, one for read events
    // and another for status events
    //

    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osReader.hEvent == NULL)
    {
        pSSI->ProtocolHandler->OnError(SSIRTHREAD_CREATEREVENT);
        ExitThread(0);  
    }
    osStatus.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osStatus.hEvent == NULL)
    {
        pSSI->ProtocolHandler->OnError(SSIRTHREAD_CREATESEVENT);
        CloseHandle(osReader.hEvent);
        ExitThread(0);
    }

	  //

    //
    // We want to detect the following events:
    //   Read events (from ReadFile)
    //   Status events (from WaitCommEvent)
    //   Thread exit events (from our shutdown functions)
    //
	hArray[0] = osReader.hEvent;
	hArray[1] = osStatus.hEvent;	
	hArray[2] = pSSI->ProtocolHandler->ghThreadExitEvent;



   while ( !fThreadDone ) 
   {

      //
      // if no read is outstanding, then issue another one
      //
      if (!fWaitingOnRead) 
      {
         if (!ReadFile(pCOMDEV(pPortInfo), lpBuf, AMOUNT_TO_READ, &dwRead, &osReader)) 
         {
            if (GetLastError() != ERROR_IO_PENDING)	  // read not delayed?
            {
               pSSI->ProtocolHandler->OnFatalError(SSIRTHREAD_BADREAD); // tell user to disconnect
               ExitThread(0);
            }

            fWaitingOnRead = TRUE;
         }
         else 
         {    
            // read completed immediately
            if (dwRead)
               InterpretPacket(pSSI,(BYTE*)lpBuf, dwRead);

            
         }
      }

        //
        // If status flags have changed, then reset comm mask.
        // This will cause a pending WaitCommEvent to complete
        // and the resultant event flag will be NULL.
        //
      if (dwStoredFlags != pEVENTFLAGS(pPortInfo)) 
      {
         dwStoredFlags = pEVENTFLAGS(pPortInfo);
    
			if (!SetCommMask(pCOMDEV(pPortInfo), EV_TXEMPTY))
            pSSI->ProtocolHandler->OnFatalError(SSIRTHREAD_SETMASK);
      }


        //
        // if no status check is outstanding, then issue another one
        //
      if (!fWaitingOnStat) 
      {

         WaitCommFlag = WaitCommEvent(pCOMDEV(pPortInfo), &dwCommEvent, &osStatus);
         if(OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
         {
            LastError = GetLastError();
	         WaitCommFlag = !(LastError == ERROR_IO_PENDING);
         }
         if (!WaitCommFlag)
         {     
            if (GetLastError() != ERROR_IO_PENDING)	  // Wait not delayed?
               pSSI->ProtocolHandler->OnFatalError(SSIRTHREAD_WAITCEVENT);
            else
               fWaitingOnStat = TRUE;
         }     
         // else WaitCommEvent returned immediately
			if(!fWaitingOnStat)
			{
				if(dwCommEvent & EV_TXEMPTY)
				{
					if(pSSI->ProtocolHandler->bFlagUserInterrupt) // last time thru, state machine sent nak cancel instead of
						                                           // a user command because the scanner might have been waiting
																				 // for an ack or nak and would not have recognized the command 
					{
						pSSI->ProtocolHandler->PacketTimer->SetInterval(0);
						pSSI->ProtocolHandler->ResponseTimer->SetInterval(0);
						EscapeCommFunction(pCOMDEV(pPortInfo),CLRRTS);     // scanner needs to see deassertion after the nak cancel
						pSSI->ProtocolHandler->PacketState = PacketStart; 
						pSSI->ProtocolHandler->bFlagUserInterrupt = FALSE;

						// bFlagUserInterrupt is set to TRUE then the state is set to UserCommandPending at which point
						// if a PKT_TIMEOUT_EVENT is seen, the command will be re-sent.  The event could have been set to
						// any of the events that would have caused a re-send of the last command whose value has been stored.
						pSSI->ProtocolHandler->RunStateMachine( PKT_TIMEOUT_EVENT, 0,0,0);
  
					}
					else
					{
						EscapeCommFunction(pCOMDEV(pPortInfo),CLRRTS);
						pSSI->ProtocolHandler->PacketState = PacketStart;
					}

				}
			}
		}
   

        //
        // wait for pending operations to complete
        //
      if ( fWaitingOnStat && fWaitingOnRead ) 
      {
         dwRes = WaitForMultipleObjects(NUM_READSTAT_HANDLES, hArray, FALSE, STATUS_CHECK_TIMEOUT);
         switch(dwRes)
         {
                //
                // read completed
                //
            case WAIT_OBJECT_0:
               if (!GetOverlappedResult(pCOMDEV(pPortInfo), &osReader, &dwRead, FALSE)) 
               {
                  if (GetLastError() != ERROR_OPERATION_ABORTED)
                  {
                      pSSI->ProtocolHandler->OnFatalError(SSIRTHREAD_ORESULT); 
                      ExitThread(0);
                  }
               }
               else 
               {      // read completed successfully
                  if (dwRead)
                     InterpretPacket(pSSI, (BYTE*)lpBuf, dwRead);
               }
               
               fWaitingOnRead = FALSE;
               break;

                //
                // status completed
                //
            case WAIT_OBJECT_0 + 1: 
               if (!GetOverlappedResult(pCOMDEV(pPortInfo), &osStatus, &dwOvRes, FALSE)) 
               {
                  if (GetLastError() != ERROR_OPERATION_ABORTED)
                  {
                     pSSI->ProtocolHandler->OnFatalError(SSIRTHREAD_ORESULT); // needs error 
                     ExitThread(0);
                  }
						else
							if(!fWaitingOnStat)
							{
								if(dwCommEvent & EV_TXEMPTY)
								{

									// see notes above AND in comstate.cpp regarding this flag
									if(pSSI->ProtocolHandler->bFlagUserInterrupt)
									{
										pSSI->ProtocolHandler->PacketTimer->SetInterval(0);
										pSSI->ProtocolHandler->ResponseTimer->SetInterval(0);
										EscapeCommFunction(pCOMDEV(pPortInfo),CLRRTS);
										pSSI->ProtocolHandler->PacketState = PacketStart;  
										pSSI->ProtocolHandler->bFlagUserInterrupt = FALSE;
										pSSI->ProtocolHandler->RunStateMachine( PKT_TIMEOUT_EVENT, 0,0,0);
									}	
									else
									{
										EscapeCommFunction(pCOMDEV(pPortInfo),CLRRTS);
										pSSI->ProtocolHandler->PacketState = PacketStart;
									}
									

								}
							}
					}
					else
					{
						if(dwCommEvent & EV_TXEMPTY)
						{
							// see notes above regarding this flag
							if(pSSI->ProtocolHandler->bFlagUserInterrupt)
							{
								pSSI->ProtocolHandler->PacketTimer->SetInterval(0);
								pSSI->ProtocolHandler->ResponseTimer->SetInterval(0);
								EscapeCommFunction(pCOMDEV(pPortInfo),CLRRTS);
								pSSI->ProtocolHandler->PacketState = PacketStart;  
								pSSI->ProtocolHandler->bFlagUserInterrupt = FALSE;
		
								pSSI->ProtocolHandler->RunStateMachine( PKT_TIMEOUT_EVENT, 0,0,0);
							}	
							else
							{
								EscapeCommFunction(pCOMDEV(pPortInfo),CLRRTS);
								pSSI->ProtocolHandler->PacketState = PacketStart;
							}

						}

					}


               fWaitingOnStat = FALSE;
               break;

 
				case WAIT_OBJECT_0 + 2:
               fThreadDone = TRUE;
               break;

            case WAIT_TIMEOUT:
               {
                  COMSTAT ComStatNew; 
                  DWORD dwErrors;

                  ClearCommError(pCOMDEV(pPortInfo), &dwErrors, &ComStatNew);
               }
               break;                       

            default:
                    
               break;
         }
      }
   }

    //
    // close event handles
    //
   CloseHandle(osReader.hEvent);
   CloseHandle(osStatus.hEvent);

   return 1;
}

/*****************************************************************************
*	SYNOPSIS:		static void InterpretPacket (LPVOID lpV, BYTE *stream, unsigned long Length)
*
*	DESCRIPTION:	Interprets the input stream using the SSI protocol.
*
*	PARAMETERS:		lpV:     cast to ponter to CComPort object
*                 stream:	scanner input data 
*						Length:	length of scanner input data
*	RETURN VALUE:	none
*	INPUTS:			CComPort PacketState			
*						CComPort PacketArray			
*						CComPort CurrentPacketNumber	
*
*	OUTPUTS:			CComPort ResponseTimer
*						CComPort PacketTimer
*						CComPort PacketStats 
*	NOTES:
*
*	PSEUDOCODE:		For i= 0, i < Length, i++
*							Set received to stream[i]
*							case the state is PacketStart, 
*								kill the response timer and start the packet timer
*								Get the next available packet buffer from the PacketArray and adjust the CurrentPacketNumber
*								Set the Packet's length to received byte
*								If length is valid 
*									init the packet checksum
*									set the state to PacketType
*								Else
*									set the state to PacketError
*									decrement i so that we're sure to go thru loop one more time to do any PacketError processing
*
*							case the state is PacketType
*								Set the Packet's type to received byte
*								If the Packet type is a valid type
*									set event_type to the event corresponding to the packet type
*									do intermediate checksum computation
*									set state to PacketSource
*								Else
*									set the state to PacketError
*									decrement i so that we're sure to go thru loop one more time to do any PacketError processing
*
*							case the state is PacketSource
*								Set the packet's source to the received byte
*								If the source is from the decoder ie valid
*									do intermediate checksum computation
*									set state to PacketStatus
*								Else
*									set the state to PacketError
*									decrement i so that we're sure to go thru loop one more time to do any PacketError processing

*							case the state is PacketStatus
*								Set the packet's status to the received byte
*								If the status indicates this is a contination packet
*									decrement the event_type
*								Do the intermediate checksum calculation
*								If the Packet length is just the SSI header length, 
*									Set the state to PacketCRC1 to get the first byte of the checksum data
*								Else set the state to PacketData and Packet's BytesReceived to SSI_HEADER_LEN
*
*							case the state is PacketData
*								Store the received byte in the Packet's PacketData and increment it's BytesReceived
*								do the intermediate checksum calculation
*								If the Packet's BytesReceived equals the Packet's length
*									Set the state to PacketCRC1
*							
*							case the state is CRC1
*								store a null char at the end of the Packet's data just for safety
*								Shift the byte received up 8 bits and store in packet's checksum
*								Set the packet state to PacketCRC2
*							
*							case the state is CRC2
*								OR the byte received into the Packet's Checksum
*								If the packet's checksum is the same as the packet's computed checksum	
*									If the event type is a NAK RESEND
*										Check for a data value of 1 in the Packet's 1st data byte and if so, change the event type
*										to NAK DENIED OR BAD CONTEXT
*									Set the Packet State to PacketStart	
*									Kill the PacketTimer
*                          If there is more data and the data is a resend, ignore the current packet
*                          Else
*									   Run the state machine with the event and "DON'T CARE" values for command code, params and param
*									bytes 			
*								Else
*									Set the PacketState to Packet Error 
*									decrement i so that we're sure to go thru loop one more time to do any PacketError processing
*
*							case the state is PacketError
*								Set the Length to zero to stop the for loop and wait for the PacketTimer Callback or Response
*								Timer, if state machine was run while we were processing this input and the Response timer was
*								started while we were here waiting for more input or a timeout.

*
******************************************************************************/
static void InterpretPacket (LPVOID lpV, BYTE *stream, unsigned long Length)
{


	unsigned long i;
	int index;
	static BYTE event_type;
	BYTE Received;
	CComPort *pSSI = (CComPort *)lpV;



	if((pSSI->ProtocolHandler->ResponseTimer == NULL) || (pSSI->ProtocolHandler->PacketTimer == NULL))
		return;

	for (i = 0; i < Length; i++)  
	{
		
		Received = stream[i];

		switch (pSSI->ProtocolHandler->PacketState)
		{
			case PacketStart:

				// another packet came in, kill the response timer since it's keeping track if scanner is 
				// responding to our commands
				pSSI->ProtocolHandler->ResponseTimer->Enable(FALSE);
				
				pSSI->ProtocolHandler->PacketTimer->Enable(FALSE);
				pSSI->ProtocolHandler->PacketTimer->SetInterval(pSSI->ProtocolHandler->nPACKETSTARTTIMEOUT);  
				pSSI->ProtocolHandler->PacketTimer->Tag = PacketStart;  
				pSSI->ProtocolHandler->PacketTimer->Enable(TRUE);


				if(++pSSI->ProtocolHandler->CurrentPacketNumber >= PACKET_Q_SIZE)
					pSSI->ProtocolHandler->CurrentPacketNumber = 0;
				
				pSSI->ProtocolHandler->Packet = &(pSSI->ProtocolHandler->PacketArray[pSSI->ProtocolHandler->CurrentPacketNumber]);

				pSSI->ProtocolHandler->Packet->Length = Received;


				if((pSSI->ProtocolHandler->Packet->Length >= SSI_HEADER_LEN) && (pSSI->ProtocolHandler->Packet->Length <= (SSI_MAX_PACKET_LENGTH - SSI_CHECKSUM_LEN)) )
				{
					pSSI->ProtocolHandler->Packet->ComputedChecksum = 0;
					pSSI->ProtocolHandler->Packet->ComputedChecksum -= Received;
					pSSI->ProtocolHandler->PacketState = PacketType;
					pSSI->ProtocolHandler->PacketStats.Total++;
				}
				else
				{
					pSSI->ProtocolHandler->PacketStats.ELength++;
					pSSI->ProtocolHandler->PacketState = PacketError;  // on packet timeout this will be seen as a checksum error
					i--; // decrement counter so that we are sure to go thru for loop again and end up in error state
				}

			break;

			case PacketType:
				pSSI->ProtocolHandler->Packet->Type = Received;
				pSSI->ProtocolHandler->Packet->ComputedChecksum -= Received;
				pSSI->ProtocolHandler->PacketState = PacketSource; 

				for( index = 0; index < MAX_DECODER_PKTTYPES; index++)
					if(Received == pSSI->ProtocolHandler->Recv_Pkt_Types[index])
						break;
				
				

				if(index < MAX_DECODER_PKTTYPES)				// map ssi type to pket types from scaner
				{														// ack, nak, batchdata, capabilities reply
					event_type = pSSI->ProtocolHandler->Recv_Pkt_Events[index];	// param send, reply revision,  
																		// plus unsolicited - decode data, event,
				}                                         // image data, video data
				else
				{
					pSSI->ProtocolHandler->PacketStats.ESource++;
					pSSI->ProtocolHandler->PacketTimer->Tag = PacketType;  // don't go to error state, see if we get a good checksum so we can nak deny this
				}
			break;

			case PacketSource:

				pSSI->ProtocolHandler->Packet->Source = Received;
				pSSI->ProtocolHandler->PacketState = PacketStatus;
				pSSI->ProtocolHandler->Packet->ComputedChecksum -= Received;

				if(!(Received == SSI_SOURCE_DECODER))
				{
					if(pSSI->ProtocolHandler->PacketTimer->Tag == PacketStart) // wasn't a packet type error also
						pSSI->ProtocolHandler->PacketTimer->Tag = PacketSource;
				}
			break;

			case PacketStatus:
				pSSI->ProtocolHandler->Packet->Status = Received;
				if(Received & 0x02)
					event_type -= 1;  // if this is a continuation packet, we need the "next" event not "last" one
			
				pSSI->ProtocolHandler->Packet->ComputedChecksum -= Received;
				if(pSSI->ProtocolHandler->Packet->Length == SSI_HEADER_LEN)
					pSSI->ProtocolHandler->PacketState = PacketCRC1;
				else 
					pSSI->ProtocolHandler->PacketState = PacketData;
				pSSI->ProtocolHandler->Packet->BytesReceived = SSI_HEADER_LEN;

			break;

			case PacketData:
				pSSI->ProtocolHandler->Packet->Data[pSSI->ProtocolHandler->Packet->BytesReceived - SSI_HEADER_LEN] = Received;
				pSSI->ProtocolHandler->Packet->BytesReceived++; 
				pSSI->ProtocolHandler->Packet->ComputedChecksum -= Received;
				if (pSSI->ProtocolHandler->Packet->BytesReceived == pSSI->ProtocolHandler->Packet->Length)
					pSSI->ProtocolHandler->PacketState = PacketCRC1;

			break;

			case PacketCRC1:
				pSSI->ProtocolHandler->Packet->Data[pSSI->ProtocolHandler->Packet->BytesReceived - SSI_HEADER_LEN] = '\0';
				pSSI->ProtocolHandler->Packet->Checksum = (WORD)((WORD)Received << 8);  // stored hibyte lo byte
				pSSI->ProtocolHandler->PacketState = PacketCRC2;
			break;

			case PacketCRC2:
				pSSI->ProtocolHandler->Packet->Checksum |= (Received);  


				
				if (pSSI->ProtocolHandler->Packet->Checksum == pSSI->ProtocolHandler->Packet->ComputedChecksum)
				{
					if(pSSI->ProtocolHandler->PacketTimer->Tag == PacketStart) // no data errors noted along the way
					{
					
						if(event_type == NAK_RESEND_EVENT)
							if(pSSI->ProtocolHandler->Packet->Data[0] != 1)
								event_type = NAK_DENIED_OR_BADCONTEXT_EVENT;
						pSSI->ProtocolHandler->PacketState = PacketStart;	// good packet checksum - start at beginning with next input
						pSSI->ProtocolHandler->PacketTimer->Enable(FALSE);
						pSSI->ProtocolHandler->PacketTimer->SetInterval(0);

						if(Length == (i + 1)) // we have processed all the input from the com port
							pSSI->ProtocolHandler->RunStateMachine( event_type,0,NULL,0);
						else if((Length - (i + 1)) > 3) // then we have another packet header
						{
							int resend_index =  i+4;
							if(!(stream[resend_index] & 0x01) )// this is a resend so ditch the current one and use the next one
								pSSI->ProtocolHandler->RunStateMachine( event_type,0,NULL,0);
						}	

						pSSI->ProtocolHandler->PacketStats.OK++;
					}
					else  // indicate data error by setting tag to PacketCRC1 so that when times out in error state, 
					{		// we will know that we got a command that we don't understand or that the source was not expected
						pSSI->ProtocolHandler->PacketTimer->Tag = PacketCRC1;
						pSSI->ProtocolHandler->PacketState = PacketError;
						i--; // decrement counter so that we are sure to go thru for loop again and end up in error state
					}

				}
				else
				{
					pSSI->ProtocolHandler->PacketStats.ECRC++;
					pSSI->ProtocolHandler->PacketTimer->Tag = PacketCRC2;
					pSSI->ProtocolHandler->PacketState = PacketError;
					i--; // decrement counter so that we are sure to go thru for loop again and end up in error state

				}
			break;
			case PacketError: // we stay here until packet timeout occurrs and is processed 
				Length = 0;
			break;

		}
	}


}

