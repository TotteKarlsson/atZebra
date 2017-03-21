/*****************************************************************************
*	FILENAME:	ComState.cpp
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Implementation of the state machine  for SSI Protocol
*
*	NOTES:			The state machine may be entered by a user request (api
*						call), by reception of a packet from the scanner, by a
*						response timeout or by a packet timeout - meaning some data was
*						received before the response timeout but it was not a valid packet.
*
*						The user may make a request for data which causes the scanner
*						to send the requested data within a timeout period..
*						The user may send the scanner a command which causes the scanner
*						to perform a function then send an ack or nak as completion notification
*                 within a timeout period.
*
*						The ResponseTimer may send a notification that no data has come back
*						from the scanner in response to a user request or a user command.
*
*						The Reader thread may send a notification that a valid ssi packet has
*						been received and is ready for processing.
*
*						The PacketTimer may send a notification that some data was received but
*						not all of it was sent or it failed it's checksum or some other format
*						error was detected.
*
*	AUTHOR: A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM: New File
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/COMSTATE.CPP-arc  $
 *
 *    Rev 1.0   Nov 22 2002 13:25:44   fitzsimj
 * Initial revision.
 *
 *    Rev 1.1   Nov 11 2002 11:49:44   schuesla
 * Fixed brackets for array delete
 *
 *    Rev 1.0   Oct 28 2002 14:38:46   schuesla
 * Initial revision.
*
*****************************************************************************/

/****************************************************************************/
/*	Include Files ************************************************************/
//#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "ssidll.h"
#include "CommThreads.h"
#include "gcpjpeg.h"
#include "comport.h"


/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/

#define IdleState								0 //  No pending host request, no decoder input expected

// these may have been folded into one state but extra logic would have made processing less clear  
// ... No pending host request, expecting next barcode, image or video data packet from decoder 
#define Unsolicited_Barcode_Pending		1 //  No pending host request, expecting next barcode data packet from decoder
#define Unsolicited_Image_Pending		2 //  No pending host request, expecting next image data packet from decoder
#define Unsolicited_Video_Pending		3 //  No pending host request, expecting next video data packet from decoder

#define HOST_ABORTXFER_Pending			4 //  Host has requested image transfer abort, waiting for line to go quiet

#define HOST_CMD_Pending					5 //  Host Command has been issued, expecting ACK packet from decoder

// these could have been folded into 2 states, but extra logic for handling which type of data would make code less clear
//... Host Request for data packet response has been issued, expecting next requested data packet
//... Host Request for data packet response has been issued, expecting last requested data packet
#define HOST_REQforBATCHDATA_Pending	6 //	This host request is not implemented
#define HOST_REQforPARAMDATA_Pending	7 //	Host Request for parameter data has been issued, expecting next requested data packet
#define HOST_REQforCapDATA_Pending		8 //	Host Request for capabilties data has been issued, expecting next requested data packet
#define HOST_REQforRevDATA_Pending		9 //	Host Request for revision data has been issued, expecting next requested data packet


// for  packet data that may or may not be multipacketed
#define FIRST_AND_LAST			3	// one packet and it's marked as the last packet
#define FIRST_IN_SERIES			2	// first packet of multipacket series
#define INTERMEDIATE_PACKET		1	// multipacket series - not first and not last packet
#define LAST_IN_SERIES			0	// mutlipacket series and it's the last packet


// image type defines
#define IMAGE_TYPE_INDEX 8

#define JPEG_IMAGE_TYPE  0x31
#define BMP_IMAGE_TYPE   0x33
#define TIFF_IMAGE_TYPE  0x34

#define PREAMBLE_BYTES 10

#define MAX_RETRIES	2	// max number of times we'll resend a packet to the scanner.


/*****************************************************************************
*	SYNOPSIS:		void CComThreads::SetupStateMachine(void)
*
*	DESCRIPTION:	Initializes variables used by the state machine.  Called by
*						GlobalInitialze after CComThreads object is constructed.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	none
*
******************************************************************************/
void CComThreads::SetupStateMachine(void)
{
    nRetries = 0;
    CurrentState = IdleState;

    SSIcmd_code = 0;
    SSIAckNakCode = 0;

    cur_barcode_len = 0;
    cur_frame_len = 0;
    cur_image_len = 0;
    cur_param_len = 0;

    pImageData = NULL;
    VideoImageQIndex	= 0;				// streaming video needs a queue to process new packet while sending
                                            // previous one to user
    curMAXIMAGESIZE = 0;

    StateMachineInUse = 0;

    pGlobalVideoData = NULL;
    GlobalVideoDataLen = 0;

    pGlobalImageData = NULL;
    GlobalImageDataLen;

    pGlobalDecodeData = NULL;
    GlobalDecodeDataLen = 0;

    pGlobalParamData = NULL;
    GlobalParamDataLen = 0;

    pGlobalCapabilitiesData = NULL;
    GlobalParamDataLen = 0;

    pGlobalVersionData = NULL;
    GlobalVersionDataLen = 0;
}

/*****************************************************************************
*	SYNOPSIS:		void CComThreads::CleanupStateMachine(void)
*
*	DESCRIPTION:	Releases global memory used by the state machine. Called from
*						GlobalCleanup before the CComThreads object is destroyed
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	none
*
******************************************************************************/
void CComThreads::CleanupStateMachine(void)
{

	if(pImageData)
		delete [] pImageData;
	pImageData = NULL;
}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ServiceTriggerRelease(void)
*
*	DESCRIPTION:	Called from API Entry module in response to a user request for
*						a trigger release. A trigger release command may only be accepted while
*						in idle mode or during a video transfer state, since that's the only time
*						a trigger release makes sense.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if trigger release commands were sent.
*
*	NOTES:			Trigger is released automatically if barcode has been decoded and now in barcode data transfer state
*						Trigger is released automatically if it began an image transfer
*						Therefore, Trigger only needs to be released while in idle and the barcode hasn't been decoded 
*						yet or during a video transfer.
*						A TRUE return value does not mean trigger was released, only that commands to release trigger were
*						attempted to be sent to the scanner.
*
*
*
******************************************************************************/
int CComThreads::ServiceTriggerRelease(void)
{
	int return_val = FALSE;

	return_val = RunStateMachine(USER_CMD_EVENT,STOP_SESSION, NULL, 0);
	return(return_val);
		
}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ServiceTriggerPull(void)
*
*	DESCRIPTION:	Called from API Entry module in response to a user request for
*						a trigger pull.	Trigger pull command may only be accepted while
*						in idle state or Video Mode in case we are in video viewfinder for SnapShot mode.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if trigger pull commands were sent.
*
*	NOTES:			Generally speaking, it doesn't make sense to pull the trigger while waiting for another command 
*						to complete.  Last command must be ack/nak'd before the scanner can take a new one. Exception is
*                 If snapshot mode command was sent to scanner that has video viewfinder parameter set.  This results
*                 in the start of video mode continuing until the trigger is pulled.  Therefore, we allow trigger pull
*                 during video mode.
*					
*
******************************************************************************/


int CComThreads::ServiceTriggerPull(void)
{
	int return_val = FALSE;


	return_val = RunStateMachine( USER_CMD_EVENT,START_SESSION, NULL, 0);
		

	return(return_val);
	
}



/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ServiceUserCommand(int nEvent, unsigned char CmdCode, 
*								unsigned char *pParams, int ParamBytes)
*
*	DESCRIPTION:	User has a command other than abort xfer, pull trigger or release trigger, or a request for data
*						that it wants to send to the scanner. The user input data is sent to the call to run the stae machine 
*						and if procssed successfully,ret_val is set to 
*						TRUE.  
*
*	PARAMETERS:		nEvent:	indicates the event to be processed by the state machine - user command or user data request
*						CmdCode:	the SSI Command code for the SSI message
*						pParams:	any data that needs to be sent for the command eg. the parameter data to set for a param send commmand
*						ParamBytes:	length of pParams
*
*	RETURN VALUE:	TRUE if the state machine was able to process the command
*
*
******************************************************************************/

int CComThreads::ServiceUserCommand(int nEvent, unsigned char CmdCode, unsigned char *pParams, int ParamBytes)
{
	int ret_val;

	ret_val = RunStateMachine( nEvent, CmdCode, pParams, ParamBytes);
	

	return ret_val;
	
}

/*****************************************************************************
*	SYNOPSIS:		int CComThreads::SendAbortCommands() 
*
*	DESCRIPTION:	For aborting an image transfer only. Just calls state
*						machine with a cancel event command.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	returns true if command is handled, FALSE if request is 
*						denied (eg not in state that handes image transfer
*
*	NOTES:			This is a user entry point into the state machine.
*
******************************************************************************/
int CComThreads::SendAbortCommands() 
{

	return RunStateMachine(USR_NAK_CANCEL_EVENT,0, NULL, 0);
}



/*****************************************************************************
*	SYNOPSIS:		int CComThreads::RunStateMachine(int nEvent, unsigned char CmdCode, 
*							unsigned char *Params, int ParamBytes)
*
*	DESCRIPTION:	For the input event, perform the function of the current state.	
*
*	PARAMETERS:		nEvent:		the event to be serviced depending on the current state
*						CmdCode:		the SSI command  code if the state machine is entered by user cmd or request or
*                             indicator of bad checksum if the state machine is entered by Packet timeout, o/w it's
*                             value is don't care.										
*						Params:		the user data to be sent with the command, if any, if entry is by user cmd or request
*										otherwise value is don't care
*						ParamBytes:	the length of Params buffer if entry is by user command or request
*										otherwise value is don't care		
*
*	RETURN VALUE:	If the state machine is busy FALSE is returned.  If not busy:
*                 If entered by user command or request for data, a value of TRUE is returned if the 
*						command or request was accepted and readied for sending to the scanner.  Later, when the scanner 
*						actually services a user request for data, a Windows message will be sent to the user
*						application.  A value of FALSE means the request or command could not be serviced at this time
*						If entered by a packet timer timeout, if the state machine is not busy, a TRUE is always returned.
*                 indicating that the response timer is running or the state machine is back in idle.  If False is returned
*                 because the state machine is busy, the timer will timeout again and the timeout will be serviced then.
*
*						If entered by response timer timeout, or reader thread with new data from
*						the scanner, the return value is don't care, since the response timer will just timeout again, and
*                 the reader thread data will just be dropped as if it were lost and the response timer will timeout and
*                 ask for a resend or the scanner will retry.
*
*	INPUTS:			
*
*	OUTPUTS:			
*
*	NOTES:			
*
*	PSEUDOCODE:		see inline comments
*
******************************************************************************/

int CComThreads::RunStateMachine( int nEvent, unsigned char CmdCode, unsigned char *Params, int ParamBytes)
{
	int ret_val = FALSE;
	BOOL bad_checksum = FALSE;
	
	if((ResponseTimer == NULL) || (PacketTimer == NULL))
		return FALSE;

	EnterCriticalSection(&(gcsStateMachine));


	if(StateMachineInUse)
	{
		LeaveCriticalSection(&(gcsStateMachine));
		return FALSE;
	}

	StateMachineInUse = TRUE;
	LeaveCriticalSection(&(gcsStateMachine));

	if(nEvent == PKT_TIMEOUT_EVENT)
		bad_checksum = (CmdCode != 0);

	switch(CurrentState)
	{
		//***************************************************************************************************************

		case IdleState:		//  No pending host request, no decoder input expected

		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timer CALLBACK routines 
					case PKT_TIMEOUT_EVENT:		// pkt was started, but bad or incomplete within pkt timeout period
						{
							// Reset response timer interval  then send nak resend or nak cancel, no message to user 
							if(bad_checksum) // input is set to 1 if checksum failure
								AssumeNakableUnsolicitedMsg( FALSE); // false because we don't want the response timer interval set
							else
							{
								nRetries = 0;
								ResponseTimer->SetInterval(0);
								PacketNakBADCONTEXT();
							}
							ret_val = TRUE; // tells packet timeout that timeout was handled and timer can be disabled
							                // since we are either in idle or response timer has been re-started 

							break;         
						}
					case RESPONSE_TIMEOUT_EVENT: 
						{	
							ret_val = TRUE; // just stay in idle - no message t user

							ResponseTimer->SetInterval(0);  // indicate response timer should be killed - we are still in
							nRetries = 0;						// ... the CALLBACK routine here so it will be done later.
							break;								// if no packet comes in, the response timer will time out
						}

					// events from the user

					// user command
					case USER_CMD_EVENT:			// idle state - user wants to send a simple cmd to the decoder
						{								// we always accept commands - even if a transmission is in process 
														// from the scanner. Response timer is not running in idle 

							int save_interval = PacketTimer->GetInterval();
							PacketTimer->SetInterval(0); // packetstate is reset to start when tx buffer is empty
																// callback won't be run until we get a new packet.
								// save command info for a resend
							SSIcmd_code = CmdCode;
							if(ParamBytes)
								memcpy(ParamData, Params, ParamBytes);
							nParamLen = ParamBytes;
							// set retries to zero, set save state to idle, send the command
							
							if(ret_val = SendUserCommand())  // only returns false if last cmd wasn't written yet.
								CurrentState = HOST_CMD_Pending;
							else
								PacketTimer->SetInterval(save_interval);
							
							break;
						}
					// // user request for  param data, revision data, or capabilities data from decoder

					case USER_PARAMDATAREQ_EVENT:		// idle state user wants param data from decoder
						{
							if(PacketTimer->GetInterval() != 0) // scanner is sending something to us	- return busy to user
								ret_val = FALSE;
							else
							{
								// save command info for a resend
								SSIcmd_code = CmdCode;
								if(ParamBytes)
									memcpy(ParamData, Params, ParamBytes);
								nParamLen = ParamBytes;
								// set retries to zero, set cur_param_len to zero, send the command
								if(ret_val = SendParamReqCommand()) 
									CurrentState = HOST_REQforPARAMDATA_Pending;
							}
							break;
						}
					case USER_REVDATAREQ_EVENT:		// user wants  revision data from decoder
						{
							if(PacketTimer->GetInterval() != 0) // scanner is sending something to us	- return busy to user
								ret_val = FALSE;
							else
							{
								// save command info for a resend
								SSIcmd_code = CmdCode;
								if(ParamBytes)
									memcpy(ParamData, Params, ParamBytes);
								nParamLen = ParamBytes;
								// set retries to zero,  send the command
								if(ret_val = SendRevReqCommand())
									CurrentState = HOST_REQforRevDATA_Pending;
							}
							break;
						}
					case USER_CAPDATAREQ_EVENT:		// idle state - user wants  capabilities data from decoder
						{
							if(PacketTimer->GetInterval() != 0) // scanner is sending something to us	- return busy to user
								ret_val = FALSE;
							else
							{
								// save command info for a resend
								SSIcmd_code = CmdCode;
								if(ParamBytes)
									memcpy(ParamData, Params, ParamBytes);
								nParamLen = ParamBytes;
								// set retries to zero,  send the command
								if(ret_val = SendCpabilitiesReqCommand()) 
									CurrentState = HOST_REQforCapDATA_Pending;
							}
							break;
						}
					case USR_NAK_CANCEL_EVENT:	// for abort image xfer 
						{	
							ret_val = FALSE; // DENY USER REQUEST nothing to abort during idle state
							break;
						}

					// Events from input packet processing -=- unsolicited scanner input that must be ack'ed or nak'ed
					// Response timer has been stopped by packet processor and will be re-started by write routine
					// ...when we send our ack or nak

					// multipacketed
					case NEXT_IMG_PKT_EVENT:		// received an image packet from the decoder and it was not the last packet
						{
							if(ret_val = ProcessImagePacket(FIRST_IN_SERIES))
								CurrentState = Unsolicited_Image_Pending;
							break;
						}
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder and it was the last packet
						{
							// This should never happen during idle - last packet is handled in image pending state
							// ReSet response timer interval, send nak cancel and send WM_ to user
		
							ret_val = CancelScanDataNsndErrMsg(ERR_UNEXPECTEDDATA, FALSE); // false for no response timer
							break;
						}
					case NEXT_VIDEO_PKT_EVENT:	// received a video data packet from the decoder and it was not the last packet
						{
						
							if(ret_val = ProcessVideoPacket(FIRST_IN_SERIES)) // always returns true
								CurrentState = Unsolicited_Video_Pending;
							break;
						}
					case LAST_VIDEO_PKT_EVENT:	// received a video data packet from the decoder and it was the last packet
						{
			
							ret_val = ProcessVideoPacket(FIRST_AND_LAST);  // just ack and ignore it - bad scanner input
							break;
						}
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder and it was not the last packet 
						{
						
							if(ret_val = ProcessBarcodePacket(FIRST_IN_SERIES))
								CurrentState = Unsolicited_Barcode_Pending;
							break;
						}
					case LAST_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder and it was the last packet 
						{
					
							ret_val = ProcessBarcodePacket(FIRST_AND_LAST);
							break;
						}
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{
							
							ret_val = ProcessEventPacket(); 
							break;
						}

					// events from packet timer -=- solicited scanner input that cannot be ack'ed or nak'ed
					// these may all be considered ack's -=- they are all unexpected in idle state and should never happen
					// the response timer has been stopped by the packet processor

					// multipacketed
					case NXT_BATCHDATA_PKT_EVENT:		// fall thru
					case LAST_BATCHDATA_PKT_EVENT:
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
					case LAST_PARAMDATA_PKT_EVENT:	
					// never multipacketed
					case REVDATA_PKT_EVENT:	
					case CAPABILITIESDATA_PKT_EVENT:
					case ACK_EVENT: 
					case NAK_RESEND_EVENT: // an error - we have nothing to re-send here -only possibility is an ack and can't ack a nak so let scanner time out
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
						{
							UserErrorMessage(ERR_UNEXPECTEDDATA);  // ALWAYS RETURNS FALSE but we want to return true here
							ret_val = TRUE;  
							nRetries = 0;
							break;
						}					
				}

				break;
			}
		//***************************************************************************************************************

		case Unsolicited_Barcode_Pending:	//  No pending host request, expecting next barcode packet from decoder

		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timer callback functions 
					case PKT_TIMEOUT_EVENT:		// pkt was started, but bad or incomplete within pkt timeout period 
						{
							// if no retries are left, got to idle here after nak cancel is sent  
							// else if retries are left, set response t/o and nak resend 
							
							if(ret_val = NakIfNotTimedOut(DECODE_DATA)) // timeout msg to user
							{
								
								CancelScanDataNsndErrMsg(0,FALSE);  // ReSet response timer interval, send nak cancel
								CurrentState = IdleState;
							}

							ret_val = TRUE; // tells packet timeout that timeout was handled and timer can be disabled
							                // since we are either in idle or response timer has been re-started 
							break;
						}
					case RESPONSE_TIMEOUT_EVENT: // one of the below events was expected but no pkt was received from the decoder
                                 // ...within the timeout period
						{
							// if retires are up  reset timer interval and retries to zero and go back to idle state
							// else if retries are left, set response T/O
							//    if last ack/nak was ack, send another ack else send another nak resend
							//		since the last msg we sent was an ack or nak for the first packet and we haven't gotten the next one yet
							if(ret_val = CheckIfTimedOut(DECODE_DATA)) // timeout msg to user
							{  	
								CurrentState = IdleState;
							}
							else if(SSIAckNakCode == CMD_ACK)  // we acked our last packet
								PacketACK();  // interval is still set from before timeout callback
							else
								PacketNAK();
								 // can be nak if pkt t/o previous loop
							break;
						}

					// events from the user - user wants to send a command or wants 
					// ...batch data, param data, revision data, or capabilities data from decoder

						// the only user cmd we handle here is abort macro pdf
						// In unbuffered mode, abort  will terminate the macro sequence and not send any more data
						// In buffered mode, we won't be here, we'll be in idle.
						// In unbuffered mode, a flush macro pdf would only be handled in idle since if we're here, there
						// is nothing to flush -=- we're sending the packets as we get them.
					case USER_CMD_EVENT:	// we'll accept user input of abort or flush macro pdf only
							if(CmdCode == ABORT_MACRO_PDF) 
							{											
								
								// save command info for a send in HostCmdPending once the response timer times out or
								// the packet timer does, or the packet processor sends data that makes no sense when
								// we're waiting for an ack in Host Cmd Pending.

								SSIcmd_code = CmdCode;
								memcpy(ParamData, Params, ParamBytes);
								nParamLen = ParamBytes;
								nRetries = -1; // since we can't send command here, we'll set retries so when incremented it's ok 

															
								CurrentState = HOST_CMD_Pending;
								bFlagUserInterrupt = TRUE; // since we are expecting another decode packet here, we know that
								ret_val = TRUE;				// the response timer will eventually time out in the HOST_CMD_PENDING
																	// state.  When that happens, host cmd pending will try to resend the
																	// last command but the flag will tell the writer 
								                           // function to send a nak cancel instead of re-sending the last command
								                           // Then, when the readstat proc sees the xmit buffer is empty after the
																	// nak cancel is sent, it will lower CTS and disable the timeout procs
																	// for both response and packet timers, set the packt processing
																	// to start and send a dummy message for pkt timeout to the
																	// state machine.  The pkt timeout in host cmd pending state
																	// will cause the last message to be re-sent.  Hence we save it
																	// here to be sent down the road.  After it is sent, we will get an
																	// ack while in host cmd pending and then we'll go to idle 
							}
							else
								ret_val = FALSE; // DENY USER REQUEST
							break;

						break;
					case USER_BATCHDATAREQ_EVENT:	// fall thru	
					case USER_PARAMDATAREQ_EVENT:		
					case USER_REVDATAREQ_EVENT:		
					case USER_CAPDATAREQ_EVENT:		
						{
							ret_val = FALSE; // DenyUserRequest
							break;
						}
						
					case USR_NAK_CANCEL_EVENT:	// barcode pending, user request abort xfer 						
						{
							// - dont send msg, just move to next state and nak cancel next msg 
							nRetries = 0;
							CurrentState = HOST_ABORTXFER_Pending;
							break;
						}

					// events from packet processing -=- unsolicited scanner input that must be ack'ed or nak'ed
					// nak it here and when it's re-sent it will be handled correctly in idle state

					// multipacketed 
					case NEXT_IMG_PKT_EVENT:		// fall thru	
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder	
					case NEXT_VIDEO_PKT_EVENT:		// received a video data packet from the decoder 
					case LAST_VIDEO_PKT_EVENT:	
						{
							// set response timer interval to zero so writer kills timer and send nak resend - process in idle
							AssumeNakableUnsolicitedMsg(FALSE); // FALSE means don't reset response timer, set retries to zero
							UserErrorMessage(SSI_DATAFORMAT_ERR);	
							CurrentState = IdleState;
							break;
						}
					
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder and it was not the last packet 
						{
							if(!(ret_val = ProcessBarcodePacket(INTERMEDIATE_PACKET)))
								CurrentState = IdleState;
							break;
						}
					case LAST_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder and it was the last packet 
						{
							ret_val = ProcessBarcodePacket(LAST_IN_SERIES);
							CurrentState = IdleState;
							break;
						}
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{
							// set response timer interval for next packet timeout, post a WM_ and send nak resend
							ret_val = NakWithTimeoutMsg(DECODE_DATA);  // resend handled in idle
							CurrentState = IdleState;
							break;
						}

					// events from packet timer -=- solicited scanner input that cannot be ack'ed or nak'ed
					// these may all be considered ack's -=- all are unexpected here.  Ignore any more of these
					// packets when back in idle.					

					// multipacketed
					case NXT_BATCHDATA_PKT_EVENT: // fall thru
					case LAST_BATCHDATA_PKT_EVENT:
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
					case LAST_PARAMDATA_PKT_EVENT:	
					// never multipacketed
					case REVDATA_PKT_EVENT:	
					case CAPABILITIESDATA_PKT_EVENT:
					case ACK_EVENT: 
					case NAK_RESEND_EVENT: // our last message was an ack - can't respond to ack or nak with ack or nak
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
						{
							// the response timer is already dead from packet processor so just reset retires and WM_ to user
							ret_val = UserErrorMessage(ERR_UNEXPECTEDDATA);  // ALWAYS RETURNS FALSE
							CurrentState = IdleState;
							break;
						}
				}

				break;
			}
			
		//***************************************************************************************************************

		case Unsolicited_Image_Pending: //  No pending host request, expecting next image data packet from decoder

		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timers
					case PKT_TIMEOUT_EVENT:		// pkt was started, but bad or incomplete within pkt timeout period
						{
							// if no retries are left, got to idle here after send nak cancel  
							// else if retries are left, set response t/o and nak resend 
							
							if(ret_val = NakIfNotTimedOut(IMAGE_DATA)) // timeout msg to user
							{
								
								CancelScanDataNsndErrMsg(0, FALSE);  // ReSet response timer interval, send nak cancel, no errmsg
								CurrentState = IdleState;
							}
							ret_val = TRUE; // tells packet timeout that timeout was handled and timer can be disabled
							                // since we are either in idle or response timer has been re-started 

							break;
						}
					case RESPONSE_TIMEOUT_EVENT: // one of the below events was expected but no pkt was received from the decoder
                                 // ...within the timeout period
						{
							// if retires are up  set timer interval and retries to zero and go back to idle state
							// else if retries are left, set packet T/O
							//    if last ack/nak was ack, send another ack else send another nak resend
							//		since the last msg we sent was an ack or nak for the first packet and we haven't gotten the next one yet
							if(ret_val = CheckIfTimedOut(IMAGE_DATA)) // timeout msg to user
								CurrentState = IdleState;
							else if(SSIAckNakCode == CMD_ACK)
								PacketACK();
							else
								PacketNAK();

								 // resend our ack or nak
							break;
						}

					// events from the user 
					// user wants to send a command or wants batch data, param data, revision data, or capabilities data from decoder

					case USER_CMD_EVENT:					//  fall thru
					case USER_BATCHDATAREQ_EVENT:		
					case USER_PARAMDATAREQ_EVENT:		
					case USER_REVDATAREQ_EVENT:		
					case USER_CAPDATAREQ_EVENT:		
						{
							ret_val = FALSE;  //DenyUserRequest
							break;
						}
					case USR_NAK_CANCEL_EVENT:	// image pending,  abort xfer 
						{
							// just move to next state and nak cancel next message there
							nRetries = 0;
							CurrentState = HOST_ABORTXFER_Pending;
							ret_val = TRUE;
							break;
						}

					// events from packet timer -=- unsolicited scanner input that must be ack'ed or nak'ed
					
						// multipacketed
					case NEXT_IMG_PKT_EVENT:		// received an image packet from the decoder 
						{
							if((ret_val = ProcessImagePacket(INTERMEDIATE_PACKET)) == 0)
								CurrentState = IdleState;
							break;
						}
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder 
						{
							ret_val = ProcessImagePacket(LAST_IN_SERIES);
							CurrentState = IdleState;
							break;
						}
					
					case NEXT_VIDEO_PKT_EVENT:	//  fall thru      received an unexpected video data packet 
					case LAST_VIDEO_PKT_EVENT:	
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	// received an unexpected barcode data packet 
					case LAST_BARCODE_PKT_EVENT:
						{
							// set response timer interval to zero so writer kills timer and send nak resend
							AssumeNakableUnsolicitedMsg(FALSE); // don't reset response timer, set retries to zero
							UserErrorMessage(SSI_DATAFORMAT_ERR);
							CurrentState = IdleState;
							break;
						}
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{
							// set response timer interval to zero, post a WM_ and send nak reent
							ret_val = NakWithTimeoutMsg(IMAGE_DATA);  // resend handled in idle
							CurrentState = IdleState;
							break;
						}

					// Events from packet timer -=- solicited scanner input that cannot be ack'ed or nak'ed
					// These are all considered ack's from scanner in response to our asking for the data
					//... but we didn't.  Send user an error message and ignore any more of these packets in idle state.

					// multipacketed

					case NXT_BATCHDATA_PKT_EVENT:		// fall thru
					case LAST_BATCHDATA_PKT_EVENT:
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
					case LAST_PARAMDATA_PKT_EVENT:	
					// never multipacketed
					case REVDATA_PKT_EVENT:	
					case CAPABILITIESDATA_PKT_EVENT:
					case ACK_EVENT: 
					case NAK_RESEND_EVENT: // can't re-send our ack or nak because cant ack or nak another ack or nak
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
						{
							//  reset retires and WM_ to user AND return false
							ret_val = UserErrorMessage(ERR_UNEXPECTEDDATA); 
							CurrentState = IdleState;
							break;
						}
				}

				break;
			}
		//***************************************************************************************************************

		case Unsolicited_Video_Pending:	//  No pending host request, expecting next  video data packet from decoder

		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timers
					case PKT_TIMEOUT_EVENT:		// pkt was started, but bad or incomplete within pkt timeout period
						{
	

							// send nak cancel, response timer reset and set retries to 0 and return true always
								ret_val = CancelScanDataNsndErrMsg(0, FALSE); // no error message sent, reset response timer
								CurrentState = IdleState; 
						
							break;
						}
					case RESPONSE_TIMEOUT_EVENT: // one of the below events was expected but no pkt was received from the decoder
                                 // ...within the timeout period - we only do this if not last packet
						{
							// if retires are up  set timer interval and retries to zero and go back to idle state
							// else if retries are left, set response T/O and an ACK - don't bother to nak a video packet.
							if(ret_val = CheckIfTimedOut(VIDEO_DATA))
								CurrentState = IdleState;
							else
								PacketACK(); // resend ack

							break;
						}

					// events from the user
					case USER_CMD_EVENT:			// user wants to send a simple cmd to the decoder - only cmds allowed 
														// are pull and release trigger
						{								// After command is handled, we will go to back to idle state,  so we set
														// up the command here then send it in wait for cmd handled state
							if((CmdCode == START_SESSION) || (CmdCode == STOP_SESSION))
							{

								// save command info for a send in HostCmdPending
								SSIcmd_code = CmdCode;
								memcpy(ParamData, Params, ParamBytes);
								nParamLen = ParamBytes;
								nRetries = -1; // since we can't send command here, we'll set retries so when incremented it's ok 

															
								CurrentState = HOST_CMD_Pending;
								bFlagUserInterrupt = TRUE;
								ret_val = TRUE;
							}
							else
								ret_val = FALSE;  //DenyUserRequest
							break;
						}
					case USER_BATCHDATAREQ_EVENT:		// fall thru
					case USER_PARAMDATAREQ_EVENT:		// user wants batch data, param data, revision data, 
					case USER_REVDATAREQ_EVENT:		// ...or capabilities data from decoder
					case USER_CAPDATAREQ_EVENT:		
					case USR_NAK_CANCEL_EVENT:			// for abort xfer during image transfer or response data transfer states
						{
							ret_val = FALSE;  // DenyUserRequest  unallowed command during video
							break;
						}

					// Events from packet timer -=- unsolicited scanner input that must be ack'ed or nak'ed

					// multipacketed					
					case NEXT_IMG_PKT_EVENT:		// fall thru
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder - in video viewfinder this can happen
						{
							AssumeNakableUnsolicitedMsg(FALSE); // don't reset response timer, set retries to zero
							CurrentState = IdleState;				// when resent, handle in idle
							break;
						}
					case NEXT_VIDEO_PKT_EVENT:	// received a video data packet from the decoder and it was not the last packet
						{
							if((ret_val = ProcessVideoPacket(INTERMEDIATE_PACKET)) == 0)
								CurrentState = IdleState;
							break;
						}
					case LAST_VIDEO_PKT_EVENT:	// received a video data packet from the decoder and it was the last packet
						{
							ret_val = ProcessVideoPacket(LAST_IN_SERIES);
							CurrentState = IdleState; 
							break;
						}
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	// fall thru
					case LAST_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder
						{
							AssumeNakableUnsolicitedMsg(FALSE); // don't reset response timer, set retries to zero
							UserErrorMessage(SSI_DATAFORMAT_ERR);
							CurrentState = IdleState;				// handle the retry in idle
							break;
						}
					
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{
							// Reset response timer interval for next packet timeout, post a WM_ and send nak resend
							ret_val = NakWithTimeoutMsg(VIDEO_DATA);  // resend handled in idle
							CurrentState = IdleState;
							break;
						}

					// Events from packet timer -=- solicited scanner input that cannot be ack'ed or nak'ed
					// These may all be considered ack's - for data we didn't ask for.  Ignore data here and also
					// when get back to idle mode, if any more follow.

					// multipacketed
					case NXT_BATCHDATA_PKT_EVENT:		// fall thru
					case LAST_BATCHDATA_PKT_EVENT:
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
					case LAST_PARAMDATA_PKT_EVENT:	
					// never multipacketed
					case REVDATA_PKT_EVENT:	
					case CAPABILITIESDATA_PKT_EVENT:
					case ACK_EVENT: 
					case NAK_RESEND_EVENT: 
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
					default: // just in case
						{
							//  reset retires and WM_ to user and return false
							ret_val = UserErrorMessage(ERR_UNEXPECTEDDATA);
							CurrentState = IdleState;
							break;
						}

				}

				break;
			}
		//***************************************************************************************************************

		case HOST_ABORTXFER_Pending:	//  Host has requested transfer abort, only accepted in image pending - wait here for line to go quiet

		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timers
					case PKT_TIMEOUT_EVENT:		// pkt was started, but bad or incomplete within pkt timeout period
						{
							// Set response timer interval, send nak cancel - no WM_ to user - and return true always
							ret_val = CancelScanDataNsndErrMsg(0, TRUE);  
							break;
						}
					case RESPONSE_TIMEOUT_EVENT: // what we've been waiting for
                                 
						{
							ret_val = TRUE;
							nRetries = 0;
							ResponseTimer->SetInterval(0); // tell writer to disable timer and now we'll allow uer commands again
							CurrentState = IdleState;

							break;
						}

					// events from the user

					// wants to send a simple cmd to the decoder or
					// user wants batch data, param data, revision data, or capabilities data from decoder

					case USER_CMD_EVENT:			//  fall thru  
					case USER_BATCHDATAREQ_EVENT:		
					case USER_PARAMDATAREQ_EVENT:		
					case USER_REVDATAREQ_EVENT:		
					case USER_CAPDATAREQ_EVENT:		
					case USR_NAK_CANCEL_EVENT:	// for abort xfer during image transfer or response data transfer states
						{
							ret_val = FALSE;  //DenyUserRequest 
							break;
						}

					// events from packet processor -=- unsolicited scanner input that must be ack'ed or nak'ed
					// send nak cancel and wait for timeout

					// multipacketed
					case NEXT_IMG_PKT_EVENT:		// fall thru
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder
					case NEXT_VIDEO_PKT_EVENT:	// received a video data packet from the decoder and it was not the last packet
					case LAST_VIDEO_PKT_EVENT:	// received a video data packet from the decoder and it was the last packet
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	//  fall thru 
					case LAST_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{
							// Set response timer interval, send nak cancel and no message to user
							ret_val = CancelScanDataNsndErrMsg(0, TRUE);
							break;
						}
						
					// events from packet processor -=- solicited scanner input that cannot be ack'ed or nak'ed
					// these may all be considered ack's - restart response timer and wait for timeout

					// multipacketed
					case NXT_BATCHDATA_PKT_EVENT:		// fall thru
					case LAST_BATCHDATA_PKT_EVENT:
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
					case LAST_PARAMDATA_PKT_EVENT:	
					// never multipacketed
					case REVDATA_PKT_EVENT:	
					case CAPABILITIESDATA_PKT_EVENT:
					case ACK_EVENT: 
					case NAK_RESEND_EVENT: 
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
						{
						// after got packet, which stopped response timer, nothing to write so we need to restart
							ResponseTimer->SetInterval(ABORTTIMEOUT);  
							ResponseTimer->Enable(TRUE);  
							ret_val = TRUE; 
							break;
						}
				}

				break;
			}



		//***************************************************************************************************************

		case HOST_CMD_Pending:			//  Host Command has been issued, expecting ACK packet from decoder

		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timers			// fall thru these timer events
					case PKT_TIMEOUT_EVENT:			// pkt was started, but bad or incomplete RESPONSE within pkt timeout period
															// don't know if we got an ack or a nak from decoder so we'll resend
					case RESPONSE_TIMEOUT_EVENT:	// one of the below events was expected but no pkt was received from the decoder
				                                 // ...within the timeout period
						{
							// If retries are up, send Windows msg, reset timer interval and retries to zero and go to idle
							//	Otherwise, resend last command to the decoder and stay in this state, even if write failed
							bFlagUserInterrupt = FALSE;  // if here because of response timer timeout we need to do this

							if((ret_val = ReSendUserCommand(COMMAND_NOTHANDLED)) == 0)
								CurrentState = IdleState;
							

							ret_val = TRUE; // tells packet timeout that timeout was handled and timer can be disabled
							                // since we are either in idle or response timer has been re-started 

							break;
						}

					// events from the user
						// user wants to send a simple cmd to the decoder or
						// user wants batch data, param data, revision data, or capabilities data from decoder

					case USER_CMD_EVENT:					// fall thru		
					case USER_BATCHDATAREQ_EVENT:		
					case USER_PARAMDATAREQ_EVENT:		
					case USER_REVDATAREQ_EVENT:		
					case USER_CAPDATAREQ_EVENT:		
					case USR_NAK_CANCEL_EVENT:			
						{
							ret_val = FALSE; // DenyUserRequest
							break;
						}

					// events from packet processor -=- unsolicited scanner input that must be ack'ed or nak'ed, but we're looking 
					// for an ack or nak ourseleves.  All we can do is nak cancel and resend our command next time
				
					

					// multipacketed
					case NEXT_IMG_PKT_EVENT:		// fall thru
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder 
					case NEXT_VIDEO_PKT_EVENT:	// received a video data packet from the decoder and it was not the last packet
					case LAST_VIDEO_PKT_EVENT:	// received a video data packet from the decoder and it was the last packet
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	// fall thru
					case LAST_BARCODE_PKT_EVENT:	// received a barcode data packet
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{
							bFlagUserInterrupt = TRUE;  // try again to come back with a pkt timeout msg to resend the command
							ret_val = CancelScanDataNsndErrMsg( 0, TRUE); // no error code, set response timer and always return true
							break;
						}
					// events from packet processor -=- solicited scanner input that cannot be ack'ed or nak'ed
					// these may all be considered ack's

					// multipacketed
					case NXT_BATCHDATA_PKT_EVENT:
					case LAST_BATCHDATA_PKT_EVENT:
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
					case LAST_PARAMDATA_PKT_EVENT:	
					// never multipacketed
					case REVDATA_PKT_EVENT:	
					case CAPABILITIESDATA_PKT_EVENT:
						{
							bFlagUserInterrupt = FALSE;
							//If retries are up or last command couldn't be resent, return to idle after setting cur_param_len,
							//cur_batch_len, nRetries and the ResponseTimer interval to zero. 
							if((ret_val = MustResendHostCmd(COMMAND_NOTHANDLED))  == 0)
							{
								
								CurrentState = IdleState;
							}
							break;
						}						
					case ACK_EVENT: // we can accept a user command only in idle mode except for a stop session in video mode
						             // or abort or flush macro pdf - however, 
						{				 // this is what we were looking for right now
							bFlagUserInterrupt = FALSE;
							ret_val = TRUE;
							CurrentState = IdleState;
							SendWindowsMessage(WM_CMDCOMPLETEMSG, 0, 0);
							nRetries = 0;

							break;
						}
					case NAK_RESEND_EVENT: 
						{
							//If retries are up or last command couldn't be resent, return to idle after setting cur_param_len,
							//cur_batch_len, nRetries and the ResponseTimer interval to zero. 
							bFlagUserInterrupt = FALSE;
							if((ret_val = MustResendHostCmd(COMMAND_NOTHANDLED))  == 0)
							{
								CurrentState = IdleState;
							}
							break;
						}
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
						{
						   // always returns false
							bFlagUserInterrupt = FALSE;
							ret_val = UserErrorMessage(ERR_UNSUPPORTED_COMMAND); // scanner didn't like our command
						
							CurrentState = IdleState;
							break;
						}
				}

				break;
			}

		//***************************************************************************************************************

		// case HOST_REQforBATCHDATA_Pending:	//	 Host Request for data has been issued, expecting next requested data packet
		// unimplemented

		//***************************************************************************************************************
		//***************************************************************************************************************

		case HOST_REQforPARAMDATA_Pending:	//	 Host Request for data has been issued, expecting next requested data packet
		
		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timers
					case PKT_TIMEOUT_EVENT:		// pkt was started, but bad or incomplete within pkt timeout period
						{
							//  We don't resend this packet
							//...because we can't guarantee we will have gotten all the packets if it's a multipacket
							// ...response.  If we get more PARAM packets when we return to idle, we'll ignore them.

							// kill the response timer, reset retries  and WM_ to user since we can't nak-resend this packet
							ret_val = UserErrorMessage(COMMAND_NOTHANDLED); // returns false always
							CurrentState = IdleState;  // if paramdata is multipacketed, user may get second packet in idle
							cur_param_len = 0;
							
							ret_val = TRUE; // tells packet timeout that timeout was handled and timer can be disabled
							                // since we are either in idle or response timer has been re-started 
							
							break;
						}
					case RESPONSE_TIMEOUT_EVENT: // one of the below events was expected but no pkt was received from the decoder
											           // ...within the timeout period
						{
							// Set response timer interval to next packet t/o if retries are not up
							// o/w set nRetries, cur_param_len, cur_batch_len and Response timer interval to zero
							// and return to idle
							if((ret_val = IncreaseTimeout(COMMAND_NOTHANDLED)) == 0)
							{
								CurrentState = IdleState;
							}
							break;
						}

					// Events from the user -=- user wants to send a simple cmd to the decoder or 
					// ...user wants batch data, param data, revision data, or capabilities data from decoder
					case USER_CMD_EVENT:			// fall thru		
					case USER_BATCHDATAREQ_EVENT:	
					case USER_PARAMDATAREQ_EVENT:	
					case USER_REVDATAREQ_EVENT:	
					case USER_CAPDATAREQ_EVENT:	
					case USR_NAK_CANCEL_EVENT:	// for abort xfer 
						{
							ret_val = FALSE;  // DenyUserRequest
							break;
						}

					// events from input packet processing -=- unsolicited scanner input that must be ack'ed or nak'ed
					// multipacketed
					case NEXT_IMG_PKT_EVENT:		// fall thru
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder
					case NEXT_VIDEO_PKT_EVENT:		// received a video data packet from the decoder
					case LAST_VIDEO_PKT_EVENT:	
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder
					case LAST_BARCODE_PKT_EVENT:	
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{

							ret_val = CancelScanDataNsndErrMsg( 0, TRUE); // no error code, set response timer and always return true
							break;
						}

					// Events from input packet processing -=- solicited scanner input that cannot be ack'ed or nak'ed
					// These may all be considered ack's

					// multipacketed
					case NXT_BATCHDATA_PKT_EVENT:		// fall thru
					case LAST_BATCHDATA_PKT_EVENT:
						{
							// kill the response timer and WM_ to user and return false
							ret_val = UserErrorMessage(COMMAND_NOTHANDLED);
							CurrentState = IdleState;
							cur_param_len = 0;
							break;
						}
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
						{
							if((ret_val  = ProcessParamPacket(INTERMEDIATE_PACKET)) == 0)
							{
								cur_param_len = 0;
								CurrentState = IdleState;
							}
							break;
						}						
					case LAST_PARAMDATA_PKT_EVENT:	
						{
							ret_val = ProcessParamPacket(LAST_IN_SERIES);
							cur_param_len = 0;
							CurrentState = IdleState;
							break;
						}
					// never multipacketed
					case REVDATA_PKT_EVENT:				// fall thru
					case CAPABILITIESDATA_PKT_EVENT:
					case ACK_EVENT: 
					case NAK_RESEND_EVENT: 
						{
							// reset retries WM_ to user and return false
							ret_val = UserErrorMessage(COMMAND_NOTHANDLED);
							CurrentState = IdleState;
							cur_param_len = 0;
							break;
						}
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
						{
							// the response timer is already dead, reset retries and WM_ to user
							ret_val = UserErrorMessage(ERR_UNSUPPORTED_COMMAND);
							CurrentState = IdleState;
							cur_param_len = 0;
							break;
						}
				}

				break;
			}
		//**************************************************************************************************************

		case HOST_REQforCapDATA_Pending:	//	 Host Request for data has been issued, expecting next requested data packet

		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timers		// fall thru
					case PKT_TIMEOUT_EVENT:		// pkt was started, but bad or incomplete within pkt timeout period
					case RESPONSE_TIMEOUT_EVENT: // one of the below events was expected but no pkt was received from the decoder
															// ...within the timeout period
						{
							//If retries are up or last command couldn't be resent, return to idle after setting cur_param_len,
							//cur_batch_len, nRetries and the ResponseTimer interval to zero. 
							if((ret_val = MustResendHostCmd(COMMAND_NOTHANDLED)) == 0)
								CurrentState = IdleState;

							ret_val = TRUE; // tells packet timeout that timeout was handled and timer can be disabled
							                // since we are either in idle or response timer has been re-started 

							break;
						}

					// Events from the user -=- user wants to send a simple cmd to the decoder or 
					// ...user wants batch data, param data, revision data, or capabilities data from decoder
					case USER_CMD_EVENT:			// fall thru		
					case USER_BATCHDATAREQ_EVENT:	
					case USER_PARAMDATAREQ_EVENT:	
					case USER_REVDATAREQ_EVENT:	
					case USER_CAPDATAREQ_EVENT:	
					case USR_NAK_CANCEL_EVENT:	// for abort xfer 
					
						{
							ret_val = FALSE;  // DenyUserRequest
							break;
						}

					// Events from packet processing -=- unsolicited scanner input that must be ack'ed or nak'ed
					//

					// multipacketed
					case NEXT_IMG_PKT_EVENT:		// fall thru
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder
					case NEXT_VIDEO_PKT_EVENT:		// received a video data packet from the decoder
					case LAST_VIDEO_PKT_EVENT:	
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder 
					case LAST_BARCODE_PKT_EVENT:	
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{

							ret_val = CancelScanDataNsndErrMsg( 0, TRUE); // no error code, set response timer and always return true
							break;
						}


					// Events from packet processing -=- solicited scanner input that cannot be ack'ed or nak'ed
					// These may all be considered ack's
					//	request.

					// multipacketed
					case NXT_BATCHDATA_PKT_EVENT:
					case LAST_BATCHDATA_PKT_EVENT:
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
					case LAST_PARAMDATA_PKT_EVENT:	
					// never multipacketed
					case REVDATA_PKT_EVENT:	
						{
							//If retries are up or last command couldn't be resent, return to idle after setting cur_param_len,
							//cur_batch_len, nRetries and the ResponseTimer interval to zero. 
							if((ret_val = MustResendHostCmd(COMMAND_NOTHANDLED)) == 0)
								CurrentState = IdleState;
							break;
						}
					case CAPABILITIESDATA_PKT_EVENT:
						{
							ret_val = ProcessCapabilitiesPacket();
							CurrentState = IdleState;
							break;
						}						
					case ACK_EVENT:		// fall thru 
					case NAK_RESEND_EVENT: 
						{
							//If retries are up or last command couldn't be resent, return to idle after setting cur_param_len,
							//cur_batch_len, nRetries and the ResponseTimer interval to zero. 
							if((ret_val = MustResendHostCmd(COMMAND_NOTHANDLED)) == 0)
								CurrentState = IdleState;
							break;
						}
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
						{
							// kill the response timer, reset retries and WM_ to user and return false always
							ret_val = UserErrorMessage(ERR_UNSUPPORTED_COMMAND);  // scanner didn't like our request for data
							CurrentState = IdleState;
							break;
						}
				}

				break;
			}
		//***************************************************************************************************************

		case HOST_REQforRevDATA_Pending:	//	 Host Request for data has been issued, expecting next requested data packet

		//***************************************************************************************************************
			{
				switch(nEvent)
				{
					// events from timers		// fall thru
					case PKT_TIMEOUT_EVENT:		// pkt was started, but bad or incomplete within pkt timeout period

					case RESPONSE_TIMEOUT_EVENT: // one of the below events was expected but no pkt was received from the decoder
                                 // ...within the timeout period

						{
							if((ret_val = MustResendHostCmd(COMMAND_NOTHANDLED)) == 0)
								CurrentState = IdleState;

							ret_val = TRUE; // tells packet timeout that timeout was handled and timer can be disabled
							                // since we are either in idle or response timer has been re-started 

							break;
						}

					// Events from the user -=- user wants to send a simple cmd to the decoder or 
					// ...user wants batch data, param data, revision data, or capabilities data from decoder
					case USER_CMD_EVENT:			// fall thru		
					case USER_BATCHDATAREQ_EVENT:	
					case USER_PARAMDATAREQ_EVENT:	
					case USER_REVDATAREQ_EVENT:	
					case USER_CAPDATAREQ_EVENT:	
					case USR_NAK_CANCEL_EVENT:	// for abort xfer 					
						{
							ret_val = FALSE;  //DenyUserRequest
							break;
						}

					// events from packet processor -=- unsolicited scanner input that must be ack'ed or nak'ed
					// multipacketed
					case NEXT_IMG_PKT_EVENT:		// fall thru
					case LAST_IMG_PKT_EVENT:		// received an image packet from the decoder 
					case NEXT_VIDEO_PKT_EVENT:		// received a video data packet from the decoder 
					case LAST_VIDEO_PKT_EVENT:	
					// may be multipacketed
					case NEXT_BARCODE_PKT_EVENT:	// received a barcode data packet from the decoder 
					case LAST_BARCODE_PKT_EVENT:
					// never multipacketed
					case SCANNEREVENT_PKT_EVENT:	// received an event data packet from the decoder - always last packet 
						{

							ret_val = CancelScanDataNsndErrMsg( 0, TRUE); // no error code, set response timer and always return true
							break;
						}

					// events from packet processor -=- solicited scanner input that cannot be ack'ed or nak'ed
					// these may all be considered ack's

					// multipacketed
					case NXT_BATCHDATA_PKT_EVENT:
					case LAST_BATCHDATA_PKT_EVENT:
					// may be multipacketed
					case NXT_PARAMDATA_PKT_EVENT:
					case LAST_PARAMDATA_PKT_EVENT:	
						{
							//If retries are up or last command couldn't be resent, return to idle after setting cur_param_len,
							//cur_batch_len, nRetries and the ResponseTimer interval to zero. 
							if((ret_val = MustResendHostCmd(COMMAND_NOTHANDLED)) == 0)
								CurrentState = IdleState;
							break;
						}
					// never multipacketed
					case REVDATA_PKT_EVENT:	
						{
							ret_val = ProcessRevisionPacket();
							CurrentState = IdleState;
							break;
						}
					case CAPABILITIESDATA_PKT_EVENT:		// fall thru again
					case ACK_EVENT: 
					case NAK_RESEND_EVENT: 
						{
							//If retries are up or last command couldn't be resent, return to idle after setting cur_param_len,
							//cur_batch_len, nRetries and the ResponseTimer interval to zero. 
							if((ret_val = MustResendHostCmd(COMMAND_NOTHANDLED)) == 0)
								CurrentState = IdleState;
							break;
						}
					case NAK_DENIED_OR_BADCONTEXT_EVENT: 
						{
							// kill the response timer, reset retries and WM_ to user and return false
							ret_val = UserErrorMessage(ERR_UNSUPPORTED_COMMAND);  // scanner didn't like our request for data
							CurrentState = IdleState;
							break;
						}
				}

				break;
			}

	}




	EnterCriticalSection(&(gcsStateMachine));
	StateMachineInUse = FALSE;
	LeaveCriticalSection(&(gcsStateMachine));
	return ret_val;

}





/*****************************************************************************
*	SYNOPSIS:		int CComThreads::UserErrorMessage( int err_code)
*
*	DESCRIPTION:	set nRetries to zero and send
*						the user an error message.
*
*	PARAMETERS:		The error code to send to the user
*
*	RETURN VALUE:	Always FALSE
*
******************************************************************************/

int CComThreads::UserErrorMessage(  int err_code)
{
	

	// ResponseTimer was killed already during packet processing   
	nRetries = 0;
	SendWindowsMessage(WM_ERROR, (WPARAM)err_code, 0);
	return FALSE;
}






/*****************************************************************************
*	SYNOPSIS:		int CComThreads::CancelScanDataNsndErrMsg(int error_code, int set_timeout)
*
*	DESCRIPTION:	Sends the nak cancel command to the scanner after setting/clearing
*						the interval value for the response timer.  If the input
*						Error code is non-zero, sends a WM_ error message to the user.
*
*	PARAMETERS:		error_code:	error code to send to uer as param to WM_ERROR
*						OR zero if no message is to be sent.
*
*	RETURN VALUE:	TRUE always
*
******************************************************************************/

int CComThreads::CancelScanDataNsndErrMsg( int error_code, int set_timeout)
{
	

	nRetries = 0;

	if(set_timeout)
		ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);
	else
		ResponseTimer->SetInterval(0);


	PacketNakCANCEL();

	if(error_code)
		PostWindowsMessage(WM_ERROR, (WPARAM)error_code, 0);  // unexpected message 
	return TRUE;
}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::AssumeNakableUnsolicitedMsg(int bSetResponseTimer)
*
*	DESCRIPTION:	Sets the response timer to timeout so we get an indication that no input
*						was received from the scanner after we send a nak-resend, or set the 
*						response timer to be turned off because we will try the nak-resend but 
*						we don't care if we get a response or not.
*						
*
*	PARAMETERS:		bSetResponseTimer:	if TRUE, set the ResponseTimer interval to NEXTPACKETTIMEOUT
*													if FALSE set it to zero so the writer function will kill it	
*
*	RETURN VALUE:	always returns TRUE
*
*	NOTES:
*
*	PSEUDOCODE:		Store the timeout interval in ResponseTimer->Interval
*						Call the function to send a nak resend to the scanner
*
******************************************************************************/

int CComThreads::AssumeNakableUnsolicitedMsg( int bSetResponseTimer)
{
	if(bSetResponseTimer)
		ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);
	else 
	{
		nRetries = 0;
		ResponseTimer->SetInterval(0);  // indicate to writer function to kill timer 
	}

	PacketNAK();  // resend
	return TRUE;
}




/*****************************************************************************
*	SYNOPSIS:		int CComThreads::NakWithTimeoutMsg( int timer_function_code )
*
*	DESCRIPTION:	ReSets ResponseTimer interval, posts
*						a Windows message to notify of the timeout for the input
*						timer_function_code, and calls the function to nak resend.
*						Always returns TRUE
*
*	Note:				After calling this function, you must always return to idle since 
*						response timer is reset.  The resent message will be handled in idle.
*
******************************************************************************/

int CComThreads::NakWithTimeoutMsg(  int timer_function_code )
{
	

	ResponseTimer->SetInterval(0);
	
	PostWindowsMessage(WM_TIMEOUT, 0, (LPARAM) timer_function_code );
	PacketNAK();
	return TRUE;

}




/*****************************************************************************
*	SYNOPSIS:		int CComThreads::NakIfNotTimedOut( int timer_function_code )
*
*	DESCRIPTION: 
*
*	PARAMETERS:		timer_function_code:	what kind of data we were waiting for
*
*	RETURN VALUE:	TRUE if timeout because all retries are used up, 
*						FALSE if response timer was restarted and nak resend sent 

*	INPUTS:			nRetries
*				
*	OUTPUTS:			nRetries
*						ResponseTimer
*
*	NOTES:
*
*	PSEUDOCODE:		Set return val to FALSE	
*						Increment nRetries
*						If retries are up
*							post a windows message for timeout sending timer_function code as a param
*							set return val to TRUE and response timer interval to zero
*						Else
*							set response timer interval to next packet timeout
*							call the function to send a nak resend message
*						return return val
*
******************************************************************************/

int CComThreads::NakIfNotTimedOut(  int timer_function_code )
{

	int return_val = FALSE;
	


	if(nRetries++ >= MAX_RETRIES)
	{
		nRetries = 0;
		// lparam is set to image data or video data or decode data
		PostWindowsMessage(WM_TIMEOUT, 0, (LPARAM) timer_function_code ); 
		return_val = TRUE;
		
		ResponseTimer->SetInterval(0);
	}
	else 
	{
		
		ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);
      PacketNAK();
	}
	return return_val;

}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::CheckIfTimedOut( int timer_function_code )
*
*	DESCRIPTION:	Same as NakIfNotTimedOut function above except, no 
*						nak is sent when retries are up but response timer sets
*						timeout value to next packet timeout if retries are left or zero if not.
*
*
******************************************************************************/

int CComThreads::CheckIfTimedOut(  int timer_function_code )
{

	int return_val = FALSE;
	


	if(nRetries++ >= MAX_RETRIES)
	{
		nRetries = 0;
		PostWindowsMessage(WM_TIMEOUT, 0, (LPARAM) timer_function_code ); 
		return_val = TRUE;
		
		ResponseTimer->SetInterval(0);
	}
	else
		ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);
	
	return return_val;


}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::IncreaseTimeout( int error_code )
*
*	DESCRIPTION:	Sets response timer interval to next packet t/o if retries are not up
*						o/w sets nRetries, cur_param_len, cur_batch_len and Response timer interval to zero
*						and posts a WM_ERROR message to user app. 
*
*	PARAMETERS:		error_code:	the error code to be sent to user app if retries are up
*
*	RETURN VALUE:	FALSE if retries are up, TRUE if the response timer interval was set
*						to next packt timeout.
*
*	INPUTS:			nRetries
*
*	OUTPUTS:			nRetries
*						cur_param_len
*						cur_batch_len
*						ResponseTimer
*
*	NOTES:
*
*	PSEUDOCODE:		Set return_val to FALSE
*						If nRetries are up
*							Set nRetries, cur_param_len, cur_batch_len and ResponseTimer interval to zero
*							Post a windows message with the input error code 
*
******************************************************************************/

int CComThreads::IncreaseTimeout(  int error_code )
{

	int return_val = FALSE;
	


	if(nRetries++ >= MAX_RETRIES)
	{
		nRetries = 0;	
		cur_param_len = 0;

		PostWindowsMessage(WM_ERROR, (WPARAM)error_code, 0);
		
		ResponseTimer->SetInterval(0);
	}
	else 
	{
		
		ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);
		return_val = TRUE;
	}
	return return_val;


}




/*****************************************************************************
*	SYNOPSIS:		int CComThreads::SendParamReqCommand(void)
*
*	DESCRIPTION:	Send user request for paramter data to the scanner.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if command was able to be set up for sending by storing
*						in the output buffer and setting the event.
*	INPUTS:			SSIcmd_code
*						ParamData
*						nParamLen
*						
*	OUTPUTS:			nRetries
*						cur_param_len
*
*	NOTES:			SSI command info is stored in "global" var so that it can
*						be re-sent if necessary.
*					
*	PSEUDOCODE:		Set nRetries to zero
*						Set cur_param_len to zero
*						Call the function to send the stored command to the scanner
*						If the command was unable to be sent, 
*							Call the function to set the dll error code and return FALSE
*						Else return TRUE
*							
******************************************************************************/
int CComThreads::SendParamReqCommand(void)
{
	

	nRetries = 0;
	cur_param_len = 0;

	int return_val = SendSimpleCommand(SSIcmd_code,
		                        ParamData,
										nParamLen,
										nRetries); 

	return return_val;

}

/*****************************************************************************
*	SYNOPSIS:		int CComThreads::SendCpabilitiesReqCommand(void)
*
*	DESCRIPTION:	Send the request for scanner capabilities data to the scanner.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if command was able to be set up for sending by storing
*						in the output buffer and setting the event.
*	INPUTS:			SSIcmd_code
*						ParamData
*						nParamLen
*						
*	OUTPUTS:			nRetries
*
*	NOTES:			SSI command info is stored in "global" var so that it can
*						be re-sent if necessary.
*					
*	PSEUDOCODE:		Set nRetries to zero
*						Call the function to send the stored command to the scanner
*						If the command was unable to be sent, 
*							Call the function to set the dll error code and return FALSE
*						Else return TRUE
*
******************************************************************************/
int CComThreads::SendCpabilitiesReqCommand(void)
{
	

	nRetries = 0;
	int return_val = SendSimpleCommand(SSIcmd_code,
		                        ParamData,
										nParamLen,
										nRetries); 
	return return_val;

}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::SendRevReqCommand(void)
*
*	DESCRIPTION:	Sends the request for revision data to the scanner.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if command was able to be set up for sending by storing
*						in the output buffer and setting the event.
*	INPUTS:			SSIcmd_code
*						ParamData
*						nParamLen
*						
*	OUTPUTS:			nRetries
*
*	NOTES:			SSI command info is stored in "global" var so that it can
*						be re-sent if necessary.
*					
*	PSEUDOCODE:		Set nRetries to zero
*						Call the function to send the stored command to the scanner
*						If the command was unable to be sent, 
*							Call the function to set the dll error code and return FALSE
*						Else return TRUE
*
******************************************************************************/

int CComThreads::SendRevReqCommand(void)
{
	

	nRetries = 0;
	int return_val = SendSimpleCommand(SSIcmd_code,
		                        ParamData,
										nParamLen,
										nRetries); 
	return return_val;
}



/*****************************************************************************
*	SYNOPSIS:		int CComThreads::SendUserCommand(  void)
*
*	DESCRIPTION:	Sends the user command that has been stored in
*						SSIcmd_code, ParamData and nParamLen with the 
*						current value for nRetries.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE if command was able to be set up for sending by storing
*						in the output buffer and setting the event.
*	INPUTS:			SSIcmd_code
*						ParamData
*						nParamLen
*						
*	OUTPUTS:			nRetries
*						SavedState
*
*	NOTES:			SSI command info is stored in "global" var so that it can
*						be re-sent if necessary. Return value can be sent to user.
*
*	PSEUDOCODE:		Set nRetries to zero 
*						Call the function to send the ssi command that was stored
*						if the function returns FALSE
*							call the function to set the error code for the dll and return FALSE
*						else return TRUE 
*
******************************************************************************/
int CComThreads::SendUserCommand(void)
{
	int return_val = FALSE;
	


	nRetries = 0;
	

	// this gets the response timer going also -=- it returns FALSE if the output queue is not empty
	return_val = SendSimpleCommand(SSIcmd_code,
		                        ParamData,
										nParamLen,
										nRetries); 

	return return_val;

}

/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ReSendUserCommand( int error_code )
*
*	DESCRIPTION:	If the retries are up, sends Windows message for timeout,
*						resets timer interval and nRetries to zero and returns FALSE.
*						Otherwise, resends last command to the decoder and returns TRUE, 
*                 
*
*	PARAMETERS:		error_code: to send to the user applicaion if a WM_ERROR must be sent
*
*	RETURN VALUE:	TRUE if retries are up 
*	INPUTS:			nRetries
*
*	OUTPUTS:			ResponseTimer
*						SSIcmd_code
*		            ParamData
*						nParamLen
*						nRetries
*
*
*	NOTES:
*
*	PSEUDOCODE:		If nRetries are used up, 
*							Set nRetries to zero
*							Post a Windows message with the errror code
*							Set the ResponseTimer Interval to zero
*							return FALSE
*						Else call the function to send the command with the stored values
*							...for the command opcode, parm data and param length
*							return TRUE
*
******************************************************************************/
int CComThreads::ReSendUserCommand(  int error_code )
{
	int return_val = FALSE;
	


	if(nRetries++ >= MAX_RETRIES)
	{
		nRetries = 0;	
		PostWindowsMessage(WM_ERROR, error_code, 0); 
		ResponseTimer->SetInterval(0);
	}
	else // resend the last message - if we got nothing back from the decoder, that's all we can do
	{
		 // we were expecting an ack but if we got a message we can't understand - we can't nak an ack but
	    // we can assume we got an ack and continue or we can resend
		 // We'll assume we got a nak resend.  That way we'll force the decoder to resend it's previous packet
		 // return val of this function will be ignored by  callback function  It will be FALSE
		// if queue was full -- we'll let this go and timeout next time thru since this function will reset the 
		// response timer
		SendSimpleCommand(SSIcmd_code, ParamData, nParamLen, nRetries);

		return_val = TRUE;
	}
	return return_val;				
}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::MustResendHostCmd(int error_code)
*
*	DESCRIPTION:		If retries are up or last command couldn't be resent, return FALSE after setting cur_param_len,
*							cur_batch_len, nRetries and the ResponseTimer interval to zero. 
*
*	PARAMETERS:			error_code:	the code to be sent if a windows error message needs to be sent to the 
*							user application.
*
*	RETURN VALUE:		TRUE if command was resent successfully, FALSE if user should return to 
*							idle state (or saved state) since  command could not be reset because retries
*							were up or error condition.
*
*	INPUTS:				nRetries
*							SSIcmd_code
*							ParamData
*							nParamLen
*
*	OUTPUTS:				nRetries
*							cur_param_len
*							cur_batch_len
*							ResponseTimer
*
*	NOTES:				This function is called when we are expecting a response to a previous command or
*							request and we received a packet that was not the response we were expecting - ie our data
*							or an ack.
*
*	PSEUDOCODE:			Set return val to FALSE
*							If retries are up
*								set nRetries, cur_param_len, cur_batch_len and ResponseTimer interval to zero
*								post a windows message with the reason for error
*							Else
*								call the function to send the last command that was sent
*								... which was stored in SSIcmd_code, ParamData and nParamLen,
*								along with the value for nRetries.
*								set return_val to the function's return value.
*								if the function returns FALSE, the command was unable to be sent so we are in error condition
*									set nRetries, cur_param_len, cur_batch_len and ResponseTimer interval to zero
*									post a windows error message with the input error condition
*
******************************************************************************/

int CComThreads::MustResendHostCmd( int error_code) 
{
	int return_val = FALSE;
	


	// We were looking for an ACK from the decoder after we sent our command.
	// Decoder sent NAK resend 
	// or any other "non-nakable" batch, capabilites etc. packet.  We'll send our last cmd again
	// because this should cause the decoder to ack our command and then it may re-send this
	// unexpected data but by then we'll be in idle mode and we'll take care of it there.

	// We also may have been waiting for the host revision request and instead got a response timeout.

	if(nRetries++ >= MAX_RETRIES) 
	{ 
		nRetries = 0;	
		cur_param_len = 0;
		ResponseTimer->SetInterval(0);
		PostWindowsMessage(WM_ERROR, error_code, 0);  
	}
	else // resend the last message 
	{
     
		return_val = SendSimpleCommand(SSIcmd_code, ParamData, nParamLen, nRetries);
	
		if(!return_val) // queue was full so couldn't send message - we must be in an error condition
		{ 
			nRetries = 0;
    		cur_param_len = 0;
			ResponseTimer->SetInterval(0); // the call to send the command above set the response timer - we need to reset it
			PostWindowsMessage(WM_ERROR, error_code, 0);
		}

	}
	return return_val;
}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ProcessImagePacket(int first_intermediate_last)
*
*	DESCRIPTION:	Processes the current packet as an ssi image data packet by storing
*						data from intermediate packets, appending them into a buffer until the
*						last packet is reached. While appending intermediate packets, it sends 
*						the current total number of bytes stored and the total number expected 
*						to the user app using windows messages. when the last packet is processed,
*						it sends the data to the user app using another window's message.  
*
*	PARAMETERS:		first_intermediate_last:	set to FIRST_IN_SERIES during idle state then
*															set to INTERMEDIATE_PACKET or LAST_IN_SERIES 
*															in image processing state	
*
*	RETURN VALUE:	FALSE if done with image, successfully or unsuccessfully,  and user should return to idle state.
*	INPUTS:			pImageData
*						curMAXIMAGESIZE
*						Packet
*
*	OUTPUTS:			nRetries
*						cur-image_len
*						pImageData
*						curMAXIMAGESIZE
*						ResponseTimer
*
*	NOTES:
*
*	PSEUDOCODE:		Set ret_val to TRUE and set nRetries to zero
*						If this is the first packet of the image data
*							Set cur_image_len to zero
*							Get the length of the image from the 1st 4 bytes of the image data
*							Add the bytes for the header and store the result in new_image_len
*							If pImageData is not null, meaning we have previously allocated memory
*								If the number of bytes currently allocated at pImageData is less than new_image_len
*									delete what we have now
*									new what we need now
*									if new succeeded
*										save the number of bytes allocated in curMAXIMAGESIZE
*									else set that value to zero
*							Else
*									new what we need
*									if new succeeded
*										save the number of bytes allocated in curMAXIMAGESIZE
*									else set that value to zero
*						If the cur_image_len plus the data in this packet is less than curMAXIMAGESIZE
*							append the new data to the end of the buffer and adjust cur_image_len
*						Else
*							set the return value to FALSE
*						Store NEXTPACKETTIMEOUT as the ResponseTimer's interval
*						If return value is TRUE
*							Call PacketACK to send the ack to the scanner
*							If this is the last packet of the image
*								Call the function to send the windows message that data is available to the user
*								Set the cur_image_len back to zero
*							Else
*								Call the function to send the windows message with the transfer status to the user
*						Else
*							Call the function to send a nak cancel to the scanner to abort the transfer
*							Set the cur_image_len to zero
*						Return the ret_val
*							
******************************************************************************/

int CComThreads::ProcessImagePacket( int first_intermediate_last)
{
	int ret_val = TRUE;
	long new_image_len;
	


	nRetries = 0;
	

	if(first_intermediate_last == FIRST_IN_SERIES)
	{
		cur_image_len = 0;

		new_image_len = Packet->Data[0] << 24;
		new_image_len |= Packet->Data[1] << 16;
		new_image_len |= Packet->Data[2] << 8;
		new_image_len |= Packet->Data[3];
		
		new_image_len += PREAMBLE_BYTES; // add in the bytes for the header

		if(pImageData != NULL)
		{
			if(curMAXIMAGESIZE <= new_image_len)
			{
				delete [] pImageData;
				pImageData = new BYTE[new_image_len + 1];
				if(pImageData != NULL)
					curMAXIMAGESIZE = new_image_len;
				else
					curMAXIMAGESIZE = 0;
			}
		}
		else
		{
			pImageData = new BYTE[new_image_len];
			if(pImageData != NULL)
				curMAXIMAGESIZE = new_image_len;
			else
				curMAXIMAGESIZE = 0;
		}

	}
	if((cur_image_len + Packet->Length - SSI_HEADER_LEN) <= curMAXIMAGESIZE)
	{
		memcpy(&pImageData[cur_image_len], Packet->Data, Packet->Length - SSI_HEADER_LEN);
		cur_image_len += Packet->Length - SSI_HEADER_LEN;
	}
	else
	{
		ret_val = FALSE;
	}


	if(ret_val)
	{
		if((first_intermediate_last == FIRST_AND_LAST)  || (first_intermediate_last == LAST_IN_SERIES))
			ResponseTimer->SetInterval(0);
		else
			ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);

		PacketACK();

      if((first_intermediate_last == FIRST_AND_LAST)  || (first_intermediate_last == LAST_IN_SERIES))
      {
   	   if(GlobalImageDataLen >= (cur_image_len - PREAMBLE_BYTES))
			{
				memcpy(pGlobalImageData, pImageData+10 , cur_image_len - PREAMBLE_BYTES);
				PostWindowsMessage(WM_IMAGE, BUFFERSIZE_GOOD, cur_image_len - PREAMBLE_BYTES);
			}
         else if(GlobalImageDataLen)
            PostWindowsMessage(WM_IMAGE, BUFFERSIZE_ERROR, cur_image_len - PREAMBLE_BYTES);
         else
            PostWindowsMessage(WM_IMAGE, NOBUFFER_ERROR, cur_image_len - PREAMBLE_BYTES);
      }
      else
		{	// tell user how many bytes so far out of how many to expect for total image 
			PostWindowsMessage( WM_XFERSTATUS, (WPARAM) (cur_image_len - PREAMBLE_BYTES), 
				                                          (LPARAM) (curMAXIMAGESIZE - PREAMBLE_BYTES));
		}

	}
	else
	{
		ResponseTimer->SetInterval(0);
		PacketNakCANCEL();
		cur_image_len = 0;
	}

	return ret_val;


}



/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ProcessVideoPacket(int first_intermediate_last)
*
*	DESCRIPTION:	Processes the 
*
*	PARAMETERS:		first_intermediate_last:	indicates order of packet in video stream
*															first, intermediate, last, etc.
*
*	RETURN VALUE:	FALSE if next state should be idle state - done with this frame 
*						either successfully or unsuccessfully.  Return TRUE to stay in current mode.
*
*	INPUTS:			cur_frame_len
*						VideoBarcodeOrEvent	
*						VideoImageQ		
*
*	OUTPUTS:			nRetries
*						cur_frame_len	
*						VideoBarcodeOrEvent
*						VideoImageQ
*						VideoImageQIndex
*
*	NOTES:
*
*	PSEUDOCODE:		set ret_val to TRUE and nRetries to zero
*						if this is the first packet in a series
*							set cur_frame_len to zero
*						else if first packet is marked first and last 
*							this is an error so set cur_frame_len to VIDEOIMAGESIZE  so it won't get processed
*						determine the size needed - what we have plus what we just got plus a jpeg file header
*						if the size needed is less than VIDEOIMAGESIZE
*							append it to the end of the data in intermediate buffer, VideoBarcodeOrEvent
*							adjust cur_frame_len
*						else set cur_frame_len to VIDEOIMAGESIZE
*						if current packet is last packet of frame
*							set ret_val to FALSE to indicate next state should be idle state
*							if the cur_frame_len is greater than the minimum, and less than max
*								Call the function to prepend a JPEG header in front of the video data and store
*									in an image queue buffer
*								Call the function to post a windows message that video data is available and new length
*								Increment the video queue index
*							Set the cur_frame_len to zero
*						Set the response timeout inteval for the next packet
*						Send an ACK always and return ret_val
*								
*
******************************************************************************/

int CComThreads::ProcessVideoPacket( int first_intermediate_last)
{
	int ret_val = TRUE;
	long new_image_len;
	int Length;	


	nRetries = 0;
	

	if(first_intermediate_last == FIRST_IN_SERIES)
	{
		cur_frame_len = 0;

		new_image_len = Packet->Data[0] << 24;
		new_image_len |= Packet->Data[1] << 16;
		new_image_len |= Packet->Data[2] << 8;
		new_image_len |= Packet->Data[3];

		if(VIDEOIMAGESIZE <= (new_image_len + JPEG_FILE_HEADER_SIZE + 2) ) // won't fit
			cur_frame_len = VIDEOIMAGESIZE; // we'll throw out the rest of the packets for this frame
	}
	else if (first_intermediate_last == FIRST_AND_LAST)
		cur_frame_len =  VIDEOIMAGESIZE;  // this is an error - always more than one packet for video

	if(Packet->Status & 0x01) // re-transmitting an intermediate packet - we'll throw out the frame 
		cur_frame_len =  VIDEOIMAGESIZE; // ... by acking the rest of the packets and waiting for the beginning of the next
	else if((cur_frame_len + Packet->Length - SSI_HEADER_LEN) <= VIDEOIMAGESIZE)
	{
		memcpy(&VideoBarcodeOrEvent[cur_frame_len], Packet->Data, Packet->Length - SSI_HEADER_LEN);
		cur_frame_len += Packet->Length - SSI_HEADER_LEN;
	}
	else
		cur_frame_len = VIDEOIMAGESIZE;

	if((first_intermediate_last == FIRST_AND_LAST)  || (first_intermediate_last == LAST_IN_SERIES))
	{
		ret_val = FALSE;  // indicate go to idle, not error indicator here
		new_image_len = VideoBarcodeOrEvent[0] << 24;
		new_image_len |= VideoBarcodeOrEvent[1] << 16;
		new_image_len |= VideoBarcodeOrEvent[2] << 8;
		new_image_len |= VideoBarcodeOrEvent[3];

		if((cur_frame_len - 10) == new_image_len)
		{	
			if((cur_frame_len > 11) && (cur_frame_len < VIDEOIMAGESIZE))
			{
				Length = BuildHeader(VideoBarcodeOrEvent+10, (unsigned char *)VideoImageQ[VideoImageQIndex], cur_frame_len -10);
				
				if(GlobalVideoDataLen >= Length)
				{
					memcpy(pGlobalVideoData, VideoImageQ[VideoImageQIndex], Length);
					PostWindowsMessage(WM_VIDEOIMAGE, BUFFERSIZE_GOOD, Length);
				}
            else if(GlobalVideoDataLen)
               PostWindowsMessage(WM_VIDEOIMAGE, BUFFERSIZE_ERROR, Length);
            else
               PostWindowsMessage(WM_VIDEOIMAGE, NOBUFFER_ERROR, Length);
				
				
				pGlobalVideoData = NULL; // can't use buffer anymore
				GlobalVideoDataLen = 0;
				VideoImageQIndex = (VideoImageQIndex +1) % VIDEOIMAGEQSIZE;
			}
		}
		else
			cur_frame_len = VIDEOIMAGESIZE;
		cur_frame_len = 0;  // ready for next frame
		ResponseTimer->SetInterval(0);
	}
	else
		ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);
	// we'll ack and return TRUE to wait for last packet from scanner or response timeout


	PacketACK();

	return ret_val;


}




/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ProcessBarcodePacket(int first_intermediate_last)
*
*	DESCRIPTION:	Copies the barocode data to a buffer for sending to user
*						and sends a WM_DECODE message  to the user
*						application.
*
*	PARAMETERS:		first_intermediate_last:	set to FIRST_IN_SERIES or FIRST_AND_LAST
*															if this is the first of a multipacketed transmissison
*															
*
*	RETURN VALUE:	TRUE if done with barcode data processing and user should return to idle 
*						
*
*	INPUTS:			Packet
*						cur_barcode_len
*						VideoBarcodeOrEvent
*						Packet
*
*	OUTPUTS:			nRetries
*						VideoBarocodeOrEvent
*						cur_barcode_len
*
*	NOTES:			Sends the barcode type preceeding the data 
*
*			 
*
*	PSEUDOCODE:		Set return val to TRUE and nRetries to zero
*						if this is the first barcode of a series or first and last because there's only one
*							set cur_barcode_len to zero and allocate a temp buffer for the data if needed
**						if the data fits in our data buffer, 
*							copy the data (including the codetype indicator in the first byte of first packet)
*						else try to resize the buffer an copy if successful o/w
*							set ret_val to FALSE so we will return to idle at end of processing
*						Set the ResponseTimer->Interval to NEXTPACKETTIMEOUT;
*						If ret val is TRUE
*							call function to send ACK
*							if this is last packet of barcode data
*								Store a null after last byte of data stored
*								Send a Windows message  to the user
*								Set cur_barcode_len to zero
*						Else
*							call the function to send a NAK Cancel
*							Send a Window's message with the error to the user
*							set cur_barcode_len to zero
*						return the ret val
*
******************************************************************************/
int CComThreads::ProcessBarcodePacket( int first_intermediate_last)
{
	int ret_val = TRUE;
	

	nRetries = 0;

	if((first_intermediate_last == FIRST_IN_SERIES) || (first_intermediate_last == FIRST_AND_LAST))	
		cur_barcode_len = 0;
	

	if(cur_barcode_len == 0)
	{
		if(pImageData != NULL) // was previously allocated - we'll re-use it if big enough
		{
			if(curMAXIMAGESIZE < VIDEOIMAGESIZE)  // we allocate in blocks of VIDEOIMAGESIZE if it's not an image
			{
				delete [] pImageData;
				pImageData = new BYTE[VIDEOIMAGESIZE + 1];
				if(pImageData != NULL)
					curMAXIMAGESIZE = VIDEOIMAGESIZE;
				else
					curMAXIMAGESIZE = 0;
			}
		}
		else
		{
			pImageData = new BYTE[VIDEOIMAGESIZE + 1];
			if(pImageData != NULL)
				curMAXIMAGESIZE = VIDEOIMAGESIZE;
			else
				curMAXIMAGESIZE = 0;
		}
	}

	// deal with subpackets later

	if((cur_barcode_len + Packet->Length - SSI_HEADER_LEN) < VIDEOIMAGESIZE)
	{
			// copy everything including the barcode type that preceeds the data
		memcpy(&pImageData[cur_barcode_len], &Packet->Data[0], Packet->Length - SSI_HEADER_LEN);
		cur_barcode_len += (Packet->Length - SSI_HEADER_LEN);
	}
	else // try to resize
	{
		unsigned char *ptemp;
		ptemp = new BYTE[curMAXIMAGESIZE + VIDEOIMAGESIZE + 1];  // get buffer that's  VIDEOIMAGESIZE larger than cur size
		if(ptemp != NULL)
		{
			memcpy(ptemp,pImageData,cur_barcode_len);
			delete [] pImageData;
			pImageData = ptemp;
			curMAXIMAGESIZE += VIDEOIMAGESIZE;	
			memcpy(&pImageData[cur_barcode_len], Packet->Data, Packet->Length - SSI_HEADER_LEN);
			cur_barcode_len += Packet->Length - SSI_HEADER_LEN;
		}
		else
			ret_val = FALSE;

	}

	



	if(ret_val)
	{
		if((first_intermediate_last == FIRST_AND_LAST) || (first_intermediate_last == LAST_IN_SERIES))
			ResponseTimer->SetInterval(0);
		else
			ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);
		PacketACK();
		if((first_intermediate_last == FIRST_AND_LAST) || (first_intermediate_last == LAST_IN_SERIES))
		{
			pImageData[cur_barcode_len] = 0;

     	   if(GlobalDecodeDataLen >= cur_barcode_len)
			{
				memcpy(pGlobalDecodeData, pImageData , cur_barcode_len);
				PostWindowsMessage(WM_DECODE, BUFFERSIZE_GOOD, cur_barcode_len);
			}
         else if(GlobalDecodeDataLen)
            PostWindowsMessage(WM_DECODE, BUFFERSIZE_ERROR, cur_barcode_len);
         else
            PostWindowsMessage(WM_DECODE, NOBUFFER_ERROR, cur_barcode_len);
 

			cur_barcode_len = 0;
		}

	}
	else
	{
		ResponseTimer->SetInterval(0);
		PacketNakCANCEL();
		SendWindowsMessage(WM_ERROR, (WPARAM)SSI_DATAFORMAT_ERR, 0);
		cur_barcode_len = 0;
	}	

	return ret_val;
}





/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ProcessEventPacket(void)
*
*	DESCRIPTION:	The event data is  sent to the user - the packet is
*						acked after the response timer interval is reset.
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	TRUE always
*	INPUTS:
*	OUTPUTS:			nRetries
*
*	NOTES:	Event data comes in a single packet - not multipacketed
*
*
******************************************************************************/
int CComThreads::ProcessEventPacket(void)
{


	int ret_val = TRUE;
	int len = Packet->Length - SSI_HEADER_LEN; // this is a single byte of data
	WORD nEvent;
	
	
	nRetries = 0;
	ResponseTimer->SetInterval(0);

	nEvent = Packet->Data[0];
	
	
	PacketACK();



	SendWindowsMessage( WM_EVENT, nEvent & 0x00ff, 1);  // data goes out with the message
	
	return ret_val;


}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ProcessRevisionPacket()
*
*	DESCRIPTION:	Package the version data sent by the scanner into a buffer
*						and send it to the user app in a Windows message.  The revision
*						string is a kind of ACK from the scanner to our request so we dont
*						ever ack or nak it.
*
*	PARAMETERS:		none
*:
*	RETURN VALUE:	Always TRUE 
*		
*	INPUTS:			Packet			
*
*	OUTPUTS:			nRetries
*						VideoBarcodeOrEvent
*
*	NOTES:			This is always a single packet.  
*
*	PSEUDOCODE:		Set ret val to TRUE and nRetries to FALSE and len to length of packet data
*						copy the revision string as is into VideoBarocodeOrEvent
*						Send  a Windows message to the user app
*						return the ret val which is always TRUE - meaning go to idle
*
******************************************************************************/

int CComThreads::ProcessRevisionPacket(void)
{
	int ret_val = TRUE;
	int len = Packet->Length - SSI_HEADER_LEN;
	int i;

	nRetries = 0;

	for( i = 0; i < len; i++)
		VideoBarcodeOrEvent[i] = Packet->Data[i];

	VideoBarcodeOrEvent[len] = '\0';  // for safety

   
   if(GlobalVersionDataLen >= len)
   {
		memcpy(pGlobalVersionData, VideoBarcodeOrEvent, len);
		PostWindowsMessage(WM_SWVERSION, BUFFERSIZE_GOOD, len);
	}
   else if(GlobalVersionDataLen)
      PostWindowsMessage(WM_SWVERSION, BUFFERSIZE_ERROR, len);
   else
      PostWindowsMessage(WM_SWVERSION, NOBUFFER_ERROR, len);




	return ret_val;
}


/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ProcessCapabilitiesPacket(void)
*
*	DESCRIPTION:	For getting the scanner capablilities.  This is like an ACK
*						to us from the scanner. 
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	True always
*	INPUTS:			Packet
*	OUTPUTS:			nRetries
*						VideoBarcodeOrEvent
*
*	NOTES:			
*						 
*
*
*	PSEUDOCODE:   Just copy the data and send the Windows message that data is available.
*
******************************************************************************/
int CComThreads::ProcessCapabilitiesPacket(void)
{

	int ret_val = TRUE;
	int len = Packet->Length - SSI_HEADER_LEN;
	int i;

	nRetries = 0;
	for( i = 0; i < len; i++)
		VideoBarcodeOrEvent[i] = Packet->Data[i];
	VideoBarcodeOrEvent[len] = '\0';  // for safety
	
   if(GlobalCapabilitiesDataLen >= len)
   {
		memcpy(pGlobalCapabilitiesData, VideoBarcodeOrEvent, len);
		PostWindowsMessage(WM_CAPABILITIES, BUFFERSIZE_GOOD, len);
	}
   else if(GlobalCapabilitiesDataLen)
      PostWindowsMessage(WM_CAPABILITIES, BUFFERSIZE_ERROR, len);
   else
      PostWindowsMessage(WM_CAPABILITIES, NOBUFFER_ERROR, len);


	return ret_val;
}

/*****************************************************************************
*	SYNOPSIS:		int CComThreads::ProcessParamPacket(int first_intermediate_last)
*
*	DESCRIPTION:	Processes parameter data packet/s from the scanner and sends
*						Windows message to user.  This packet
*						is like an ack from the scanner to us in response to our request
*						for data.  We never ack or nak this packet
*
*	PARAMETERS:		first_intermediate_last:	 set to intermediate packet or last in series	
*
*	RETURN VALUE:	TRUE if done and caller should return to idle state
*
*	INPUTS:			cur_param_len
*
*	OUTPUTS:			nRetries
*						cur_param_len
*						VideoBarcodeOrEvent
*
*	NOTES:			When command is sent, cur_param_len is set to zero
*
*	PSEUDOCODE:		Set ret val to TRUE and nRetries to zero
*						if cur_param_len  plus the data from this packet will fit in the output buffer,
*							copy the data to VideoBarcodeOrEvent buffer (removing the beep code)
*							add the length of this packet, minus the first byte which is the beep code, to cur_param_len
*                 else set the ret val to FALSE
*                 if the ret val is true,
*                    if this is not the last packet, set the response timer  for the next packet
*                    else copy the data to the user buffer, reset retries and param length and send user a windows message.
*						return ret val
*
******************************************************************************/
int CComThreads::ProcessParamPacket( int first_intermediate_last)
{
	int ret_val = TRUE;
	


	nRetries = 0;
	
	
	if((cur_param_len + Packet->Length - SSI_HEADER_LEN) < VIDEOIMAGESIZE)
	{
		// remove beep code
		if((Packet->Length - SSI_HEADER_LEN - 1) > 0)
			memcpy(&VideoBarcodeOrEvent[cur_param_len], &Packet->Data[1], Packet->Length - SSI_HEADER_LEN - 1);
	

		cur_param_len += Packet->Length - SSI_HEADER_LEN - 1;
	
	}
	else
	{
		ret_val = FALSE;
	}


	if(ret_val)
	{
	
		if(first_intermediate_last == LAST_IN_SERIES)
		{
			VideoBarcodeOrEvent[cur_param_len] = 0;
			
         if(GlobalParamDataLen >= cur_param_len)
         {
		      memcpy(pGlobalParamData, VideoBarcodeOrEvent, cur_param_len);
		      PostWindowsMessage(WM_PARAMS, BUFFERSIZE_GOOD, cur_param_len);
	      }
         else if(GlobalParamDataLen)
            PostWindowsMessage(WM_PARAMS, BUFFERSIZE_ERROR, cur_param_len);
         else
            PostWindowsMessage(WM_PARAMS, NOBUFFER_ERROR, cur_param_len);
		
					
			cur_param_len = 0;
			nRetries = 0;
		}
		else  // must restart response timer to wait for last packet.  The packet timer disabled it when it processed
		{		// the param packet we're working on.
			ResponseTimer->SetInterval(NEXTPACKETTIMEOUT);
			ResponseTimer->Enable(TRUE);
		}

	}
	else
	{

		cur_param_len = 0;
	}
	
	return ret_val;
}


/*****************************************************************************
*	SYNOPSIS:		void CComThreads::SendWindowsMessage(int msg, WPARAM wParam, LPARAM lParam)
*
*	DESCRIPTION:	Call function to Post a windows message msg, using wParam and lParam
*						when doing so.
*
*
******************************************************************************/

void CComThreads::SendWindowsMessage( int msg, WPARAM wParam, LPARAM lParam)
{


	PostMessage(hWnd, msg, wParam, lParam);

}


/*****************************************************************************
*	SYNOPSIS:		void CComThreads::PostWindowsMessage(int msg, WPARAM wParam, LPARAM lParam)
*
*	DESCRIPTION:	Call function to Post a windows message msg, using wParam and lParam
*						when doing so.
*
******************************************************************************/
void CComThreads::PostWindowsMessage( int msg, WPARAM wParam, LPARAM lParam)
{
	

	PostMessage(hWnd, msg, wParam, lParam);

}

