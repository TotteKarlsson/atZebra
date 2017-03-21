/*****************************************************************************
*	FILENAME:		ComState.h
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Defines for the events monitored in the 
*						SSI protocol state machine
*
*	NOTES:
*	
*	AUTHOR:			A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM:	New File
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/COMSTATE.H-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:46   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:48   schuesla
 * Initial revision.
*
*****************************************************************************/

#ifndef _COM_STATE_H_

#define _COM_STATE_H_

/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/





// Define the events we monitor


// event affecting the state machine from the response timer and from the packet timer
#define PKT_TIMEOUT_EVENT		0	// pkt was started, but bad or incomplete within pkt timeout period
#define RESPONSE_TIMEOUT_EVENT 1 // one of the below events was expected but no pkt was received from the decoder
                                 // ...within the timeout period

// events affecting the the state machine from the user
#define USER_CMD_EVENT			2	// user wants to send a simple cmd to the decoder
#define USER_BATCHDATAREQ_EVENT		3	// user wants batch data, param data, revision data, or capabilities data from decoder
#define USER_PARAMDATAREQ_EVENT		4	// user wants batch data, param data, revision data, or capabilities data from decoder
#define USER_REVDATAREQ_EVENT		5	// user wants batch data, param data, revision data, or capabilities data from decoder
#define USER_CAPDATAREQ_EVENT		6	// user wants batch data, param data, revision data, or capabilities data from decoder
#define USR_NAK_CANCEL_EVENT	7  // for abort xfer during image transfer or response data transfer states


// events affecting the state machine from unsolicited scanner input that must be ack'ed or nak'ed
// multipacketed
#define NEXT_IMG_PKT_EVENT		8	// received an image packet from the decoder and it was not the last packet
#define LAST_IMG_PKT_EVENT		9	// received an image packet from the decoder and it was the last packet
#define NEXT_VIDEO_PKT_EVENT	10	// received a video data packet from the decoder and it was not the last packet
#define LAST_VIDEO_PKT_EVENT	11	// received a video data packet from the decoder and it was the last packet
// may be multipacketed
#define NEXT_BARCODE_PKT_EVENT 12	// received a barcode data packet from the decoder and it was not the last packet 
#define LAST_BARCODE_PKT_EVENT 13	// received a barcode data packet from the decoder and it was the last packet 
// never multipacketed
#define SCANNEREVENT_PKT_EVENT	14	// received an event data packet from the decoder - always last packet 


// events affecting the state machine from solicited scanner input that cannot be ack'ed or nak'ed
// these may all be considered ack's
// multipacketed
#define NXT_BATCHDATA_PKT_EVENT	15
#define LAST_BATCHDATA_PKT_EVENT	16
// may be multipacketed
#define NXT_PARAMDATA_PKT_EVENT	17
#define LAST_PARAMDATA_PKT_EVENT	18
// never multipacketed
#define REVDATA_PKT_EVENT	19
#define CAPABILITIESDATA_PKT_EVENT 20
#define ACK_EVENT 21
#define NAK_RESEND_EVENT 22
#define NAK_DENIED_OR_BADCONTEXT_EVENT 23


#define MAX_MONITORED_EVENTS 24



#endif // !defined _COM_STATE_H_
