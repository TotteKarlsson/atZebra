/*****************************************************************************
*	FILENAME:		CommThreads.h
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:		CComThreads Class and WRITEREQUEST data structure.
*
*	NOTES:				Class that handles both Read and Write threads, and 
*							includes the SSI protocol handling implemented in 
*							ProtocolHandler.cpp and the  SSI State Machine 
*							implemented in ComState.cpp.
*							
*	
*	AUTHOR:				A.Schuessler
*	CREATION DATE:		10/28/02
*	DERIVED FROM:     New File
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/CommThreads.h-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:44   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:46   schuesla
 * Initial revision.
*
*****************************************************************************/

#ifndef __CommThreads_H
#define __CommThreads_H


/****************************************************************************/
/*	Nested Include Files ****************************************************/

#include <windows.h>

#include "PortInfoStruct.h"
#include "ProtocolHandler.h"
#include "MsgTimers.h"
#include "ComState.h"

/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/
#define MAX_WRITE_BUFFER        (3 * SSI_MAX_PACKET_LENGTH)
#define MAX_READ_BUFFER         (3 * SSI_MAX_PACKET_LENGTH)
#define NUM_READSTAT_HANDLES    3
#define AMOUNT_TO_READ          (3* SSI_MAX_PACKET_LENGTH)
#define STATUS_CHECK_TIMEOUT    INFINITE
#define WRITE_CHECK_TIMEOUT     INFINITE

typedef struct WRITEREQUEST
{
  DWORD 				dwSize;			   							// size of buffer
  unsigned char       lpBuf[SSI_MAX_PACKET_LENGTH + 10];            // buffer to send
  DWORD 			  dwSize2;										// in case we need to send a second buffer right after
  unsigned char       lpBuf2[SSI_MAX_PACKET_LENGTH + 10];      		// second buffer to send

} WRITEREQUEST, *PWRITEREQUEST;


class CComThreads
{
	public:
        // public methods
        							CComThreads();
        virtual 					~CComThreads();
        int 						StartThreads(LPVOID lpV);
        DWORD 						WaitForThreadsToExit( DWORD dwTimeout);
        int 						GlobalInitialize(HWND hwnd, int nPort);
        void 						GlobalCleanup(int nPort);

        // public data
        TTYInfo*					pPortInfo;
        CRITICAL_SECTION            gcsWriterData;   // Set cs and events in Global init and Global Cleanup
        CRITICAL_SECTION            gcsStateMachine;   // Set cs and events in Global init and Global Cleanup
        HANDLE 						ghThreadExitEvent;
        BOOL 						WriteBuffer(BYTE *buffer, DWORD ByteCount);
        HANDLE                      ghWriterData;
        struct                      WRITEREQUEST gWriteRequest;
        HANDLE                      ghWriterEvent;
        BOOL 						bFlagUserInterrupt;

        void                        SetupProtocolHandler(int nComPort);
        void                        TearDownProtocolHandler(int nComPortIndex);
       	int                         SendAbortCommands(); // abort xfer
        int                         SendSimpleCommand( unsigned char CmdCode, unsigned char *CmdData, int CmdDataLen, int retries);
        int                         PacketACK ();
        int                         PacketNAK ();
        int                         PacketNakCANCEL ();
        int                         PacketNakBADCONTEXT();

        int                         WritePacket(unsigned char CmdCode, unsigned char *Params, int ParamBytes, int retries);
        void                        OnError(int error_code);
        void                        OnFatalError(int error_code);

        HWND                        hWnd;
        static const 				Recv_Pkt_Types[MAX_DECODER_PKTTYPES];
        static const BYTE 			Recv_Pkt_Events[MAX_DECODER_PKTTYPES];
        char 						VideoImageQ[VIDEOIMAGEQSIZE][VIDEOIMAGESIZE];  // storage for video frames as they come in
        int 						VideoImageQIndex;
        TTimer*                     ResponseTimer;   // timer for a response from the scanner after we send it an ssi command
        TTimer*                     PacketTimer;     // timer for a complete message to be received after we get the first byte
        int                         nPACKETSTARTTIMEOUT;  // timeout for receiving a complete packet - based on the baud rate
        int                         PacketQInsertIndex;	// incremented before write event is signaled, decremented when handled
        int                         PacketState;
        int                         CurrentPacketNumber;
        PacketStruct                PacketArray[PACKET_Q_SIZE];
        PacketStruct*               Packet;
        PacketStatsStruct 			PacketStats;
        int 						bPersist;

        int                         RunStateMachine(int nEvent, unsigned char CmdCode, unsigned char *Params, int ParamBytes );
        int                         ServiceTriggerRelease(void); // if the SSIcmd_code is STOP SESSION, calls SendUserCommand and if sent returns true, o/w returns false
        int                         ServiceTriggerPull(void); // if the SSIcmd_code is STOP SESSION, calls SendUserCommand and if sent returns true, o/w returns false
        int                         ServiceUserCommand(int nEvent, unsigned char CmdCode, unsigned char *pParams, int ParamBytes);

   		// these are the "Action" functions performed by the states for a particular event

        int                         UserErrorMessage( int err_code); // sends WM_ERROR and returns false
        int                         CancelScanDataNsndErrMsg(int error_code, int set_timeout); // enables response timer and sends nak cancel, may send WM_ERROR and always returns true
        int                         AssumeNakableUnsolicitedMsg(int bSetResponseTimer); // may enable response timer, sends nak resend and returns true
        int                         NakWithTimeoutMsg( int timer_function_code ); //enables response timer, sends WM_TIMEOUT and nak then returns true
        int                         CheckIfTimedOut( int timer_function_code); // if retries are up, sends WM_TIMEOUT and returns true, else enables respons timer and returns false
        int                         NakIfNotTimedOut( int timer_function_code); // if retries are up, sends WM_TIMEOUT and returns true, else enables response timer, sends NAK resend and returns false
        int                         IncreaseTimeout( int error_code ); // if retries are up resets cur_param_len and batch len, sends WM_ERROR and returns false else enables timer and returns T.
        int                         SendParamReqCommand(void);  // same as above for params
        int                         SendCpabilitiesReqCommand(void); // same as above for capabilities
        int                         SendRevReqCommand(void); // same as above for revision request
        int                         SendUserCommand( void); // saves current state, calls SendSimpleCmd and if not successful, returns false O/W returns true
        int                         ReSendUserCommand( int error_code ); // if retries are up, sends WM_TIMEOUT and returns false o/w calls SendSimpleCmd and returns T.
        int                         MustResendHostCmd(int error_code); // if retries are up or if SendSimpeCmd is not successful, resets nRetries, param and batch len, sends WM_TIMEOUT (or WM_ERROR) & returns F.o/w returns T.

        int                         ProcessImagePacket(int first_intermediate_last);
        int                         ProcessVideoPacket(int first_intermediate_last);
        int                         ProcessBarcodePacket(int first_intermediate_last);
        int                         ProcessEventPacket(void);
        int                         ProcessRevisionPacket(void);
        int                         ProcessCapabilitiesPacket(void);
        int                         ProcessParamPacket(int first_intermediate_last);


        void                        PostWindowsMessage(int msg, WPARAM wParam, LPARAM lParam);
        void                        SendWindowsMessage(int msg, WPARAM wParam, LPARAM lParam);

        unsigned char*				pGlobalVideoData;
        long						GlobalVideoDataLen;

        unsigned char*				pGlobalImageData;
        long						GlobalImageDataLen;

        unsigned char*              pGlobalDecodeData;
        long		                GlobalDecodeDataLen;

        unsigned char*              pGlobalParamData;
        long		                GlobalParamDataLen;

        unsigned char*              pGlobalCapabilitiesData;
        long		                GlobalCapabilitiesDataLen;

        unsigned char*              pGlobalVersionData;
        long		                GlobalVersionDataLen;

        BOOL 						StateMachineInUse;
        int 						CurrentState;

        // when caller wants to send an SSI Command, these 3 data structures are filled and then the state machine is involked
        // ...in case a re-send is required.
        BYTE                        SSIcmd_code;
        BYTE                        SSIAckNakCode;
        BYTE                        ParamData[SSI_MAX_PACKET_LENGTH];  // we really don't need the header bytes or checksum here
        int                         nParamLen;

        // keeps track of how many times we have timed out waiting for a correct response from the scanner
        int 						nRetries;


        // fine for video and events, also for params but barcode will need dynamic allocation if macropdf
        BYTE 						VideoBarcodeOrEvent[VIDEOIMAGESIZE + 1];  // a single macro pdf symbol may hold as many as 2700 digits and
                                               // even with a max sized macro control block, there will be fewer than
                                               // what is needed for a max sized video image. However, if the scanner
                                               // buffers the data, we would need up to the max buffer size in the scanner
                                               // plus one macro control block so we need dynamic allocation

        // ptr to dynamically allocated storage for an image or barcode data

        BYTE*                       pImageData;
        long                        curMAXIMAGESIZE; // the size of the allocated block that pImageData is pointing to


        // number of bytes of data currently stored at pImageData or in VideoBarcodeOrEvent
        long                        cur_barcode_len;
        long                        cur_frame_len;
        long                        cur_image_len;
        long                        cur_param_len;

	private:
        void                        SetupStateMachine(void);
        void                        CleanupStateMachine(void);
};


DWORD WINAPI 	ReaderProc(LPVOID lpV);
DWORD WINAPI 	WriterProc(LPVOID lpV);
void CALLBACK 	PacketTimeout (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK 	ResponseTimeout (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

#endif
