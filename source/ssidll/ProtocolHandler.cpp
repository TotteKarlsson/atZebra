/*****************************************************************************
*	FILENAME:	ProtocolHandler.cpp
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Routines to send SSI commands to the writer thread, and process
*						input data as SSI packets during the reader thread.
*
*	NOTES:
*	
*	AUTHOR:	A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM: New File
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/ProtocolHandler.cpp-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:50   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:48   schuesla
 * Initial revision.
*
*****************************************************************************/

/****************************************************************************/
/*	Include Files ************************************************************/
//#include "stdafx.h"


#include "ssidll.h"
#include "CommThreads.h"
#include "ProtocolHandler.h"


/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/

// defines for the SSIMESSAGE source
#define SSI_SOURCE_HOST 0x04
#define SSI_SOURCE_DECODER 0x00

// defines for NAK message, Cause field
#define NAK_RESEND	1
#define NAK_CANCEL	10
#define NAK_DENIED	6
#define NAK_BADCONTEXT	2

#define PERMANENT_CHANGE 0x08;


/****************************************************************************/
/*	Local (private) Variables *********************************************************/

// Constant tables

// Packets that come from the decoder
const int CComThreads::Recv_Pkt_Types[MAX_DECODER_PKTTYPES] = { 
		DECODE_DATA, 
		CMD_ACK, 
		CMD_NAK, 
		PARAM_SEND, 
		IMAGE_DATA, 
		VIDEO_DATA, 
      REPLY_REVISION, 
		EVENT, 
		BATCH_DATA, 
		CAPABILITIES_REPLY }; 


// PacketTimer function processes these in the state machine - each packet type above will map to one of these events
const BYTE CComThreads::Recv_Pkt_Events[MAX_DECODER_PKTTYPES] = {
	LAST_BARCODE_PKT_EVENT,    // all last_ events may be changed to next_ events by subtracting status byte, bit 1 
	ACK_EVENT, 
	NAK_RESEND_EVENT,				// may change to NAK_DENIED_OR_BADCONTEXT_EVENT if first data byte is 2 or 6 or 10
	LAST_PARAMDATA_PKT_EVENT, 
	LAST_IMG_PKT_EVENT, 
	LAST_VIDEO_PKT_EVENT, 
    REVDATA_PKT_EVENT, 
	SCANNEREVENT_PKT_EVENT, 
	LAST_BATCHDATA_PKT_EVENT, 
	CAPABILITIESDATA_PKT_EVENT}; 


// This data structure and static array are used only in the timeout callback functions: ResponseTimeout and PacketTimeout
// They are necessary in order to map the timer id, which we get as a param to the callback function, to it's thread and
// ...hWnd.

typedef	struct thread_array_tag{
		CComThreads *pThread;
		UINT *pResponseTimerID;
		UINT *pPacketTimerID;
		int hWnd;
	}thread_array_struct;

static thread_array_struct ThreadArray[MAX_COM_PORTS] = {0};


/*****************************************************************************
*	SYNOPSIS:		void CComThreads::SetupProtocolHandler(int nComPortIndex)
*
*	DESCRIPTION:	Initialization - called during global initialization
*						of the CComThreads object.  Initializes the object's 
*						variables and creates the timers needed for protocol 
*                 handling in the state machine.
*
*	PARAMETERS:		nComPortIndex - com port number ( not zero offset )
*
*	RETURN VALUE:	none
*
******************************************************************************/
void CComThreads::SetupProtocolHandler(int nComPortIndex) 
{
	nComPortIndex -= 1;

	PacketState = PacketStart;			// init to looking for first byte of a packet from scanner
	CurrentPacketNumber = 0;			// index into PacketArray
	Packet = &PacketArray[0];			// for filling from scanner input and processing in state mach.
	
	// time to wait for response from scanner to our command 
	ResponseTimer = new TTimer((int)hWnd);
	ResponseTimer->OnTimer = ResponseTimeout;

	// time to wait from receipt of first byte of packet for the rest of the packet
	PacketTimer = new TTimer((int)hWnd);
	PacketTimer->OnTimer = PacketTimeout; 
	PacketTimer->SetInterval(0);  // init interval to zero

	ThreadArray[nComPortIndex].pResponseTimerID = &(ResponseTimer->ID);
	ThreadArray[nComPortIndex].pPacketTimerID = &(PacketTimer->ID);
	ThreadArray[nComPortIndex].hWnd = (int) hWnd;
	ThreadArray[nComPortIndex].pThread = this;

	PacketQInsertIndex	= 0;  // don't really use queues since this is a transaction protocol
	bPersist = 0;
}

/*****************************************************************************
*	SYNOPSIS:		void CComThreads::TearDownProtocolHandler(int nComPortIndex)
*
*	DESCRIPTION:	Called during destruction of the CComThreads object.  Sets pointer 
*                 variables to NULL and deletes the Response and Packet Timer objects for
*                 the given com port number.
*                 
*
*	PARAMETERS:		nComPortIndex - com port number ( not zero offset )
*
*	RETURN VALUE:	none
*
******************************************************************************/
void CComThreads::TearDownProtocolHandler(int nComPortIndex) 
{
	PacketQInsertIndex	= 1;


	nComPortIndex -= 1;

	if((nComPortIndex >= 0)&&(nComPortIndex <16))
	{
		ThreadArray[nComPortIndex].pResponseTimerID = NULL;
		ThreadArray[nComPortIndex].pPacketTimerID = NULL;
		ThreadArray[nComPortIndex].hWnd = NULL;
		ThreadArray[nComPortIndex].pThread = NULL;
	}
	
	if(ResponseTimer)
	{
		ResponseTimer->Enable(FALSE);
		delete ResponseTimer;
		ResponseTimer = NULL;
	};

	PacketState = PacketStart;

	if(PacketTimer)
	{	
		PacketTimer->Enable(FALSE);
		delete PacketTimer;
		PacketTimer = NULL;
	}



}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//                                        TRANSMITTING
//
//                
//
//												   Messages To Scanner
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////




/*****************************************************************************
*	SYNOPSIS:		int CComThreads::SendSimpleCommand(unsigned char CmdCode, 
*									unsigned char *Params, int ParamBytes, int retries)
*
*	DESCRIPTION:	Sets up ResponseTimer with the input command code to time 
*						out after COMMANDTIMEOUT if no response is recieved from
*						the scanner, then calls the funciton to Write the Packet
*						which sets up the write request and signals a write request
*						event.  
*
*	PARAMETERS:		CmdCode:		SSI Command opcode
*						Params:		The data to be sent to the scanner
*						ParamBytes:	number of bytes in Params
*						retries:		If zero, retry bit will be set to zero in SSI packet
*										o/w it will be set to 1.
*
*	RETURN VALUE:	returns TRUE if command could be serviced o/w returns FALSE.
*
*	NOTES:			This function can only be called by the state machine when 
*						the state machine allows a write request to be issued.
*
******************************************************************************/
int CComThreads::SendSimpleCommand(unsigned char CmdCode, unsigned char *Params, int ParamBytes, int retries)
{

	if((CmdCode == PARAM_SEND) || (CmdCode == PARAM_REQUEST))
		ResponseTimer->SetInterval(RESPONSETIMEOUT);       // Allow for response to request for data
	else
		ResponseTimer->SetInterval(COMMANDTIMEOUT);       // Allow for response to command

	ResponseTimer->Tag = CmdCode;  // use for timeout message code
   return WritePacket(CmdCode, Params, ParamBytes, retries);

}




/*****************************************************************************
*	SYNOPSIS:		int CComThreads::PacketACK ()
*
*	DESCRIPTION:	Calls the function to Write the ACK packet to the decoder
*						with the retries bit set according to the nRetries member
*						variable.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	true if write event was set o/w false
*	INPUTS:			nRetries
*	OUTPUTS:
*
*	NOTES:			Only called by state machine
*
*
******************************************************************************/
int CComThreads::PacketACK ()
{
	SSIAckNakCode = CMD_ACK;

	return WritePacket (CMD_ACK, NULL, 0, nRetries);  
}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::PacketNAK (void)
*
*	DESCRIPTION:	Calls the function to Write the NAK resend packet to the decoder
*						with the retries bit set according to the nRetries member
*						variable.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	true if write event was set o/w false
*	INPUTS:			nRetries
*	OUTPUTS:
*
*	NOTES:			Only called by state machine
*
*
******************************************************************************/
int CComThreads::PacketNAK (void)
{
	BYTE Reason = NAK_RESEND;  // CHECKSUM FAILURE - resend

	SSIAckNakCode = CMD_NAK;
	return WritePacket (CMD_NAK,&Reason , 1, nRetries);  // 1 is the length of the Reason
}

/*****************************************************************************
*	SYNOPSIS:		int CComThreads::PacketNakCANCEL (void)
*
*	DESCRIPTION:	Calls the function to Write the NAK cancel packet to the decoder
*						with the retries bit set according to the nRetries member
*						variable.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	true if write event was set o/w false

*	INPUTS:			nRetries
*	OUTPUTS:
*
*	NOTES:			Only called by state machine - this nak tells the decoder we
*						don't want the message in progress - don't resend it.
*
*
******************************************************************************/
int CComThreads::PacketNakCANCEL (void)
{
	BYTE Reason = NAK_CANCEL;   // TELL DECODER WE DON'T WANT THE MESSAGE IN PROGRESS - DON'T RESEND IT

	
	return WritePacket (CMD_NAK, &Reason, 1, nRetries);
}

int CComThreads::PacketNakBADCONTEXT (void)
{
	BYTE Reason = NAK_BADCONTEXT;   // TELL DECODER WE DON'T UNDERSTAND THE MESSAGE IN PROGRESS - DON'T RESEND IT

	
	return WritePacket (CMD_NAK, &Reason, 1, nRetries);
}






/*****************************************************************************
*	SYNOPSIS:		int CComThreads::WritePacket(unsigned char CmdCode, 
*										unsigned char *Params, int ParamBytes, int retries)
*
*	DESCRIPTION:	Builds the SSI packet, computes the checksum and stores it in 
*						the packet then calls WriteBuffer with the packet and it's length
*
*	PARAMETERS:		CmdCode:	SSI opcode
*						Params:	Data for the data field of the SSI packet
*						ParamBytes:	Length of Params
*						retries:	if non-zero, the resend bit of the status byte in the packet is set
*
*	RETURN VALUE:	Returns true if a write event was able to be signaled, FALSE if the last
*						packet has not been written yet.
*
******************************************************************************/
int CComThreads::WritePacket(unsigned char CmdCode, unsigned char *Params, int ParamBytes, int retries)
{
	unsigned char SSIBuffer[SSI_MAX_PACKET_LENGTH];
	int Checksum;
	int i;

	// fill in SSI header

	SSIBuffer[ 0 ] = 0;
	SSIBuffer[ 1 ] = CmdCode;
	SSIBuffer[ 2 ] = SSI_SOURCE_HOST;
	SSIBuffer[ 3 ] = (retries) ? (unsigned char)0x01 : (unsigned char)0x00;

	if((CmdCode == PARAM_SEND) && bPersist)
		SSIBuffer[ 3 ] |= PERMANENT_CHANGE;

   if(ParamBytes)
	   memcpy(&SSIBuffer[SSI_HEADER_LEN], Params, ParamBytes);

	SSIBuffer[0] = (unsigned char)(SSI_HEADER_LEN + ParamBytes);  // holds length of data in data field

	Checksum = 0;
	for (i = 0; i < SSIBuffer[ 0 ]; i++)
		Checksum += SSIBuffer[i];
	
	i = SSI_HEADER_LEN + ParamBytes;
	SSIBuffer[i++] = (unsigned char)(((-Checksum) >> 8) & 0xFF);
	SSIBuffer[i++] = (unsigned char)((-Checksum) & 0xFF);



	return WriteBuffer ( SSIBuffer, i);
	
}


/*****************************************************************************
*	SYNOPSIS:		void CALLBACK CComThreads::ResponseTimeout(HWND hwnd, UINT uMsg, 
*											UINT idEvent, DWORD dwTime )
*
*	DESCRIPTION:	Timer callback function for signaling no response was received
*						from the scanner.  The response timer is started during 
*						the transmission process and is killed during input packet processing.
*						It may be disabled during the running of the state machine by
*						having it's interval set to zero.  
*						
*
*						This function also kills the Packet timer.  This stops the Packet
*						timer in the case of a checksum error found during packet processing. 
*						A zero value for the Packet timer's Interval is set during PacketTimeout.
*						
*
*	PARAMETERS:		unused - standard windows timer function prameters
*
*	RETURN VALUE:	none
*
*	NOTES:			We don't want to kill the response timer during it's CALLBACK function
*						so setting it's interval to zero disables it and indicates another thread 
*						may kill it.  The interval may be reset to zero during the state machine or set to
*                 it's next timeout value.  Then during the writer thread, it will be killed
*                 and possibly re-started.  
*
******************************************************************************/
void CALLBACK ResponseTimeout(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
	CComThreads *pThread = NULL;
	int i;

	// find the instance that this response timer belongs to 
	for( i = 0; i < MAX_COM_PORTS; i++)
	{
		if(ThreadArray[i].pResponseTimerID)
		{
			if(*(ThreadArray[i].pResponseTimerID) == idEvent)
			{
   			 // id with hwnd is a unique pair.
				if((int)hwnd == ThreadArray[i].hWnd)
					pThread = ThreadArray[i].pThread;
				if(pThread)
					break;
			}
		}
	}
	if(pThread)
	{

		if((pThread->PacketTimer == NULL) || (pThread->ResponseTimer == NULL))
			return;

		if(!pThread->PacketTimer->GetInterval())	// packet timeout indicated that it's disabled during it's callback
			pThread->PacketTimer->Enable(FALSE);	// so kill it here.

		if(pThread->ResponseTimer->GetInterval())	// response timer has not been disabled so process the timeout.	
			pThread->RunStateMachine(RESPONSE_TIMEOUT_EVENT, 0,NULL, 0);   
		   // if state machine was busy here, the response timer will usually be reset again and
			// this timeout can be ignored.  If this timeout 
			// is ignored because the state machine is busy denying a 
			// user request, the timer will timeout again since no-one
			// is stopping it or disabling it and the timeout will be handled then.
	}                               
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//                                        RECEIVING
//
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////




/*****************************************************************************
*	SYNOPSIS:		void CALLBACK CComThreads::PacketTimeout(HWND hwnd, UINT uMsg, 
*						UINT idEvent, DWORD dwTime )
*
*	DESCRIPTION:	Called when packet timer times out.  When input is received 
*						from the com port, the packet timer is started. If a valid packet is
*						not received within the timeout period, this function will
*						handle re-setting the packet processing to the beginning and
*						calling the state machine to handle the fact that there was a
*						checksum error.  
*						We don't want to kill the timer during this CALLBACK function so
*						we set the Interval value to 0 to indicate that the timer should be 
*						ignored when it times out again.
*						We also take this opportunity to kill the response timer if it's on.
*
*
*	PARAMETERS:		No parameter values are used.  
*
*	RETURN VALUE:	None
*	INPUTS:			PacketTimer
*	OUTPUTS:			ResponseTimer
*						PacketState
*						Packettimer
*
*	NOTES:		Every Packet Timeout event handler in state machine MUST set a new Interval value
*              for the ResponseTimer if a message is to be re-sent.
*
*	PSEUDOCODE:	
*
******************************************************************************/
void CALLBACK PacketTimeout(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{


	CComThreads *pThread = NULL;
	int i;

	for( i = 0; i < MAX_COM_PORTS; i++)
	{
		if(ThreadArray[i].pPacketTimerID)
		{
			if(*(ThreadArray[i].pPacketTimerID) == idEvent)
			{	
				if((int)hwnd == ThreadArray[i].hWnd)
					pThread = ThreadArray[i].pThread;
				if(pThread)
					break;
			}
		}
	}

	if(pThread == NULL)
		return;
	if(pThread->PacketTimer == NULL)
		return;

	if(pThread->PacketTimer->GetInterval())	// ignore this timeout if Interval is zero - done last time we were in here
	{												// we need to wait for a response timeout to kill this packet timer since
													// we don't want to do it here in its CALLBACK function.

			
		// we can have a timeout if we got less than a full packet's worth of chars, OR if we got a full 
		// packet but an error occurred

		// we know we had fewer than a full packet's worth of chars if the timer tag is not CRC1 or CRC2
		// if the tag is CRC2 then we got a bad checksum
		// if the tag is CRC1 then we got a good checksum but an error in the ssi opcode or packet source
		// if the tag is anything other than either of these, we didn't get a full packet's worth of data
		// ...so that will be reported here as a checksum error.


			
		// The state machine returns FALSE on a PKT_TIMEOUT_EVENT only if the state machine was busy.
		// If it returned true but the ResponseTimeout was reset, then it was not able to be handled and we are back in idle
		if(pThread->RunStateMachine(PKT_TIMEOUT_EVENT, (pThread->PacketTimer->Tag != PacketCRC1) ? 1: 0, NULL,0) ) 
		{
			pThread->PacketTimer->SetInterval(0);		// indicate packet timer is disabled - when checked elsewhere, it may be disabled
			if(pThread->ResponseTimer->GetInterval())	            // eg during a subsequent response timeout.
				pThread->PacketState = PacketStart;

		}



      
		// Packet State is reset to Start in the reader function the next time 
		// the transmit buffer is empty and we deassert CTS.  But if we give up here meaning we didn't send
		// any message to the scanner to resend because retries are up, then we are back in idle.  
		// If a user request is next, a message will be written to the scanner and when done, PacketStart will be set
		// in reader function.
		// However, if an unsolicited packet comes from the scanner while we in idle now, the PACKET state will be in the state 
		// was in now when this  packet timeout occurred - the error state or an intermediate state if an incomplete packet was
		// ...obtained.  If it was an intermediate state, the unsolicited packet will cause it to go to the error state
		// ...but no packet timeout will occur since it only gets set in PacketStart state. So we must look at the return
		// ...value from RunStateMachine and if it's true, then we see if we are in idle ( response timer will be off )
		// ...and then we reset the packet state to start.



		
	}
}





/*****************************************************************************
*	SYNOPSIS:		void CComThreads::OnError(int error_code)
*
*	DESCRIPTION:	Calls the function to send a windows message  with the given
*						error code for WPARAM and 0 for NON-fatal error for LPARAM
*
*	PARAMETERS:		error code:	 value to use for WPARAM
*
*	RETURN VALUE:	none
*
******************************************************************************/
void CComThreads::OnError(int error_code)
{

	SendWindowsMessage( WM_ERROR, (WPARAM) error_code, 0);
}


/*****************************************************************************
*	SYNOPSIS:		void CComThreads::OnFatalError(int error_code)
*
*	DESCRIPTION:	Calls the function to send a windows message  with the given
*						error code for WPARAM and 1 for fatal error for LPARAM
*
*	PARAMETERS:		error code:	 value to use for WPARAM
*
*	RETURN VALUE:	none
*
******************************************************************************/
void CComThreads::OnFatalError(int error_code)
{

	SendWindowsMessage(WM_ERROR, (WPARAM) error_code, 1);
	
}



