/*****************************************************************************
*	FILENAME:		ProtocolHandler.h
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Defines and typedefs for SSI packets - lengths, opcodes, data fields.	
*
*	NOTES:
*	
*	AUTHOR:			A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM:  New File
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/ProtocolHandler.h-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:50   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:48   schuesla
 * Initial revision.
*
*****************************************************************************/

#ifndef __PROTOCOLHANDLER_H__
#define __PROTOCOLHANDLER_H__

/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/

#define SSI_MAX_PACKET_LENGTH 258
#define SSI_HEADER_LEN        4
#define SSI_CHECKSUM_LEN      2


/* Defines all the SSI commands -=- these are the SSIMESSAGE opcodes - KEEP IN ASCENDING NUMERIC ORDER */
#define		FLUSH_MACRO_PDF	      0x10
#define		ABORT_MACRO_PDF		  0x11
#define		REQUEST_REVISION	  0xa3
#define		REPLY_REVISION		  0xA4
#define		IMAGE_DATA			  0xB1
#define		VIDEO_DATA		      0xB4
#define		ILLUMINATION_OFF	  0xC0
#define		ILLUMINATION_ON	  	  0xC1
#define		AIM_OFF				  0xC4
#define		AIM_ON				  0xC5
#define		PARAM_SEND			  0xC6
#define		PARAM_REQUEST		  0xC7
#define		PARAM_DEFAULTS		  0xC8
#define		CMD_ACK				  0xD0
#define		CMD_NAK				  0xD1
#define		FLUSH_QUEUE			  0xD2
#define		CAPABILITIES_REQUEST  0XD3
#define		CAPABILITIES_REPLY	  0xD4
#define		BATCH_REQUEST		  0xD5
#define		BATCH_DATA			  0xD6
#define		START_SESSION		  0xE4
#define		STOP_SESSION		  0xE5
#define		BEEP				  0xE6
#define		LED_ON				  0xE7
#define		LED_OFF				  0xE8
#define		SCAN_ENABLE			  0xE9
#define		SCAN_DISABLE		  0xEA
#define		SLEEP				  0xEB
#define		DECODE_DATA			  0xF3
#define		EVENT				  0xF6
#define		IMAGER_MODE			  0xF7
#define		LAST_OPCODE			  IMAGER_MOdE



#define MAX_DECODER_PKTTYPES 10


// Defines for data field of IMAGER_MODE command
#define VIDEO_OPERATION 2
#define SNAPSHOT_OPERATION 1
#define BARCODE_READER_OPERATION 0




// param for PARAM_REQUEST -=- this the the SSIMESSAGE data for a param request for all parameters
#define SSI_PARAM_ALL 0xFE  // for the parm send from the decoder, it will be 0XFF

#define SSI_256_PREFIX 0xF0  // TBD - NOT USED BUT MIGHT BE USEFUL LATER



// Defines for packet queuing - we are not queuing since this protocol uses strict handshaking, but use for
// creating data structures anyway.

#define MAX_BUFFERS		2 

#define VIDEOIMAGESIZE	4000
#define VIDEOIMAGEQSIZE	4  // in case user can't process as fast as we can

#define OUTPUTPACKETQSIZE	2
#define PACKET_Q_SIZE   3

#define MAX_SSI_PACKET   (SSI_MAX_PACKET_LENGTH - SSI_HEADER_LEN)  

typedef struct
{
  BYTE Length;
  int Type;
  BYTE Status;
  BYTE Source;
  int BytesReceived;
  BYTE Data[MAX_SSI_PACKET];
  WORD Checksum;
  WORD ComputedChecksum;
} PacketStruct;

typedef struct
{
  //Packet Statistics
  long Total;
  long ECRC;
  long ETimeout;
  long ELength;
  long ESource;
  long OK;
} PacketStatsStruct;



// values for ScannerStatus CComThreads member var
#define SCANNER_ALIVE   0
#define SCANNER_DEAD    1
#define SCANNER_QUERY   2



// values for PacketState
#define PacketStart	0
#define PacketType	1
#define PacketSource	2
#define PacketStatus	3
#define PacketData	4
#define PacketCRC1	5
#define PacketCRC2	6
#define PacketError	7



                     


#endif   // __PROTOCOLHANDLER_H__
