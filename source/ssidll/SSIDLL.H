/*****************************************************************************
*	FILENAME:		SSIdll.h
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	SSIdll API Functions - Entry points into the dll and error codes
*
*
*  Using the Simple Serial Interface (SSI) API
*
*  This library implements serial communications, reader and writer threads, SSI Message building
*  and the SSI Protocol handling needed to provide a communicaitons link between Symbol Technologies decoders 
*  and a Windows host.  
*
*  The Windows host program calls the functions provided by the API and recieves Windows
*  messages  when the decoder has data for the host or when a timeout or error has occurred.
*  
*
*  The calling application should first call SSIConnect with the desired baude rate, com port and handle of the 
*  window whose window procedure will receive the windows messages from the library. 
*  The calling application should check the return value of SSIConnect to determine if it was successful. 
* 
*  A call to SSIDisconnect will terminate the communications link and release all memory used by the library. 
*  This function should always be called when the application is finished communicating with the decoder.
* 
*  A call to PullTrigger will perform a software trigger.  The scanning device will  behave as if the trigger 
*  were physically pulled.
*		If the scanner is in it's normal mode Decode Mode, the laser (or camera) will turn off after a successful decode
*		or a call to ReleaseTrigger. 
*		If the scanner is in Image Capture Mode and a call to Snapshot() was made prior to the PullTrigger call, 
*		the camera will capture an image and then turn off - no ReleaseTrigger() call is necessary.  
*		If the scanner is in Video Mode, the camera will turn off after a call to ReleaseTrigger.
*
*		In video mode (or decode mode if no barcode was decoded), the application must call ReleaseTrigger(),
*     after a call to PullTrigger. 
*
*  A call to ReleaseTrigger will abort a decode attempt or video transmission.
*  
*  If the scanner supports imaging, a call to SnapShot will put the imager into Image Capture Mode.
*  The scanner will remain in Image Capture Mode until the trigger is pulled - physically or with a call to PullTrigger -
*  and an image is captured or until the timeout for a trigger pull has expired.  After the timeout or
*  the image capture, the scanner will return to Decode Mode.
*  
*  A call to TransmitVideo will put the imager into Video Mode.  After the trigger is pulled - physically or with a
*  call to PullTrigger, the decoder will produce a continuous video stream until the trigger is physically released, or
*  released with a call to ReleaseTrigger (depending on if the trigger was pulled physically or with the sw trigger). 
*  This function is currently not fully supported and should not be used.
*     
*  A call to TransmitVersion will send a request to get the software release name from the scanning device.
* 
*	A call to AbortImageXfer will send a request to the scanner to stop transmitting the image that is being sent.
*
*  A call to SetParameters will send a request to the scanner to change one or more of it's parameter values.
*  The parameters to change and the new values are sent to the scanner along with the request.  The format of the
*  parameters and their values is either  <param_num><value>  if param num is in the range 0..EFh or the format 
*  is <extended parameter code><parm_num_offset><value> for parameters whose param_num is 256 or higher.
*  The other param is the length of the param data in bytes  Note that if the param value is a word value rather than a
*  byte value, a byte having the value 0xf4 must preceed <param_num>. 
*
*  A call to RequestParameters will send a request to the scanner to send back the parameter values specified in the 
*  given parameter string - no 0xf4 is needed to preceed the param num - and the other param is the length of 
*  the param string.
*
*  A call to RequestAllParameters will send a request to the scanner to send back all the parameters and their values.
*
*  A call to AimOn sends a command to turn on the aiming pattern of an imager, and a call to AimOff sends the command
*  to turn it off.
*
*  A call to LedOn sends a command to turns on the led of the scanner, and LedOff sends the command to turn it off.
*
*  A call to ScanEnable sends the command to enable scanning, while a call to ScanDisable sends the command to disable 
*  scanning.  When scanning is disabled, the scanner will not respond to a physical or software trigger pull.
* 
*  A call to SoundBeeper will send the command to turn the beeper on with the tone and for the duration specified 
*  in the beeper code param.
*  
*  A call to EnterLowPwrMode will send the command to the scanner to go to low power mode.
*
*  A call to RequestScannerCapabilities will send a request for the scanner to send it's capabilities data - ie what 
*  commands it will perform.
*
*  A call to SetParamPersistance sets the persistance quality for any parameter changes that are requested.  Parameters
*  may be changed permanently or temporarily.
*
*  A call to AbortMacroPdf sends the command to the scanner to abort the current macro pdf scanning session and throw out
*  any macro pdf data that has been stored.
* 
*  A call to FlushMacroPDF sends the command to the scanner to abort the current macro pdf scanning session and send any
*  macro pdf data that has been stored.
*  
*  A call to GetDLLVersion will return the version data for the dll.
*
*  A call to any of the SetXXXXBuffer functions will store the buffer and it's length for the dll to use to return 
*  scanner data to the calling application.
*
*	NOTES:			Microsoft C++ Generated file to which API functions were added.
*	
*	AUTHOR:			A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM:	New file
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/SSIDLL.H-arc  $
 *
 *    Rev 1.0   Nov 22 2002 13:25:58   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:50   schuesla
 * Initial revision.
*
*****************************************************************************/

#ifndef __SSIDLL_API_H__
#define __SSIDLL_API_H__

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SSIDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SSIDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef SSIDLL_EXPORTS
#define SSIDLL_API extern "C" __declspec(dllexport)
#else 
#ifdef __cplusplus
#define SSIDLL_API extern "C" __declspec(dllimport)
#else
#define SSIDLL_API __declspec(dllimport)
#endif
#endif

/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/
#define MAX_COM_PORTS 20

///////////////////////////////////////////////////////////////////////////////
//
//	Library Error reporting 
//
// ALL library function calls return 0 if successful, o/w an error code is returned.
// If the error code is a fatal error, SSIDisconnect() should be called.  
// The list of error codes below describes the errors that can be reported.
//
// In addition to the failure status returned by library function and error codes
// the library can also send or post a WM_ERROR message to the application.
// The application should handle that message and respond appropriately
// The list of Windows messages includes a description of the WM_ERROR message.
//
///////////////////////////////////////////////////////////////////////////////

// Error codes - return values for API CALLS

#define SSICOMM_NOERROR			     0  // No error code is set - this will be the value when an API call returns success
#define ERR_SSI_NOOBJECT		    -1  // Another API function was called before a successful call to SSIConnect - no connection was established
#define ERR_SSI_HWND			    -2  // The hwnd parameter to the SSIConnect function was NULL - no connection established
#define SSICOMM_BAD_SETSTATE	    -3  // The library was unable to set the state of the com port - no connection established
#define SSICOMM_BAD_SETTIMEOUTS	    -4  // The library was unable to set the com timeouts - no connection established
#define SSICOMM_BAD_GETTIMEOUTS	    -5  // The library was unable to get the curent com timeouts - no connection established
#define SSICOMM_BAD_GETCOMSTATE	    -6  // The library was unable to get the curent com state - no connection established
#define SSICOMM_ALREADY_CLOSED	    -7	// Call to close com port was made when the com port is not open.  There is no connection.
#define SSICOMM_UNABLE_PURGE	    -8	// Call to purge the com port before closing it was not successful.
#define SSICOMM_THREADS_BADEXIT	    -9	// fatal error - the threads didn't exit properly
#define SSICOMM_ERROR_CLRDTR	    -10	// unable to lower DTR when closing com port
#define SSICOMM_BAD_CREATEFILE	    -11	// unable to open the com port
#define SSICOMM_BAD_READTHREAD	    -12	// unable to create the read/status thread - no connection
#define SSICOMM_BAD_WRITETHREAD	    -13	// unable to create the writer thread - no connection
#define SSICOMM_BAD_CREATEEVENT	    -14 // call to CreateEvent failed - fatal error
#define SSICOMM_BUSY                -15 // not fatal - try your request again later
#define SSICMD_UNIMPLEMENTED        -16 // not fatal - this command is not implemented in the library
#define SSICOMM_ALREADYCONNECTED    -17 // if already connected, can't connect without a call to disconnect
#define ERR_SSI_MISMATCHHWND        -18 // the hwnd paramter for the function does not match the stored hwnd for the connection
#define SSICOMM_TOOMUCHDATA         -19 //the maximum allowable input data length was exceeded
#define SSICOMM_ERRVERSION          -20 // Can't run on this version of windows
#define SSI_INPUTQ_FULL			    -21 // unable to add new user request to input queue for transmitting to scanner
								    	// user should re-try request
#define SSICOMM_BADDATA			    -22	// param data is in incorrect format


// these generate WM_ERROR messages to the app - all are either fatal/nonfatal errors occurring during program execution,
// ... or are the return value of an api function indicating a fatal error

#define SSICOMM_WAITMOWRITER	    -23	// Wait for Multiple Objects gave WAIT_FAILED in writer proc - if not fatal, protocol retry may recover
#define SSITHREAD_CREATEWEVENT	    -24	// Failure to create write event - fatal error
#define SSITHREAD_OLRESW		    -25	// Get overlapped result failed - fatal error
#define SSITHREAD_WRITEERR		    -26	// Number of bytes written is not the number requested to be written -if not fatal, retry may recover
#define SSITHREAD_WMOW			    -27	// Wait multiple objects failure in overlapped write - fatal error
#define SSITHREAD_WRITEFILEFAIL	    -28	// Call to Write failed, but isn't just delayed - fatal error
#define SSITHREAD_BADSETEV		    -29	// write thread returned error on set event
#define SSIRTHREAD_ORESULT		    -30	// read thread bad overlapped result - fatal error
#define SSIRTHREAD_SETMASK		    -31	// read thread bad set mask return - fatal error
#define SSIRTHREAD_BADREAD		    -32	// read thread bad read - fatal error
#define SSIRTHREAD_CREATEREVENT	    -33	// read thread bad create read event -  error code set, AIP call will return false
#define SSIRTHREAD_CREATESEVENT	    -34	// read thread bad create status event-  error code set, AIP call will return false
#define SSIRTHREAD_WAITCEVENT	    -35	// read thread wait com event bad return - fatal error


// One of these error codes is place in the WPARAM of a WM_ERROR messges during ssi protocol handling of scanner msgs

#define COMMAND_NOTHANDLED          -36   // command was not processed successfully by decoder
#define ERR_UNSUPPORTED_COMMAND     -37   // command was not processed successfully by decoder
#define SSI_DATAFORMAT_ERR		    -38	  // scanner data packet was not of correct format from decoder
#define ERR_UNEXPECTEDDATA          -39   // state machine has received data that was unexpected for the current state


// One of these OPCODES are placed  in the LPARAM of a WM_TIMEOUT msg
#define DECODE_DATA_TIMEOUT         0xF3
#define IMAGE_DATA_TIMEOUT	        0xB1
#define VIDEO_DATA_TIMEOUT          0xB4

// These are the beep codes for the beep functions
#define ONESHORTHI	                0x00
#define TWOSHORTHI	                0x01
#define THREESHORTHI                0x02
#define FOURSHORTHI	                0x03
#define FIVESHORTHI	                0x04

#define ONESHORTLO	                0x05
#define TWOSHORTLO	                0x06
#define THREESHORTLO                0x07
#define FOURSHORTLO	                0x08
#define FIVESHORTLO	                0x09

#define ONELONGHI	                0x0A
#define TWOLONGHI	                0x0B
#define THREELONGHI	                0x0C
#define FOURLONGHI	                0x0D
#define FIVELONGHI	                0x0E

#define ONELONGLO	                0x0F
#define TWOLONGLO	                0x10
#define THREELONGLO	                0x11
#define FOURLONGLO	                0x12
#define FIVELONGLO	                0x13


#define FASTHILOHILO 		        0x14
#define SLOWHILOHILO 		        0x15
#define HILO			            0x16
#define LOHI			            0x17
#define HILOHI			            0x18
#define LOHILO			            0x19

/****************************************************************************/
/*	Extern Function Prototypes **********************************************/
SSIDLL_API int __stdcall            SSIConnect(HWND hwnd, long Baud, int Port);
SSIDLL_API int __stdcall            SSIDisconnect(HWND hwnd, int nComPort);
SSIDLL_API int __stdcall            SnapShot(int nComPort);
SSIDLL_API int __stdcall            TransmitVersion(int nComPort);
SSIDLL_API int __stdcall            AbortImageXfer(int nComPort);
SSIDLL_API int __stdcall            PullTrigger(int nComPort);
SSIDLL_API int __stdcall            ReleaseTrigger(int nComPort);
SSIDLL_API int __stdcall            SetParameters(unsigned char *Params, int ParamBytes, int nComPort);
SSIDLL_API int __stdcall            TransmitVideo(int nComPort);
SSIDLL_API int __stdcall            RequestParameters(unsigned char *Params, int ParamBytes, int nComPort);
SSIDLL_API int __stdcall            RequestAllParameters(int nComPort);

SSIDLL_API int __stdcall            AimOn(int nComPort);
SSIDLL_API int __stdcall            AimOff(int nComPort);

SSIDLL_API int __stdcall            IlluminationOn(int nComPort);
SSIDLL_API int __stdcall            IlluminationOff(int nComPort);

SSIDLL_API int __stdcall            LedOn(int nComPort, unsigned char nLEDselection);
SSIDLL_API int __stdcall            LedOff(int nComPort, unsigned char nLEDselection);
SSIDLL_API int __stdcall            ScanEnable(int nComPort);
SSIDLL_API int __stdcall            ScanDisable(int nComPort);
SSIDLL_API int __stdcall            SoundBeeper(int nComPort, unsigned char nBeepCode);
SSIDLL_API int __stdcall            EnterLowPwrMode(int nComPort);

SSIDLL_API int __stdcall            RequestScannerCapabilities(int nComPort);
SSIDLL_API int __stdcall            SetParamPersistance(int nComPort, int bPersist);
SSIDLL_API int __stdcall            AbortMacroPdf(int nComPort);  // aborts macro session and throws out any data
SSIDLL_API int __stdcall            FlushMacroPdf(int nComPort);  // aborts macro session and sends any data


SSIDLL_API unsigned int __stdcall 	ReturnDLLVersion(void);


/* Once filled by the DLL, the app is sent a WM_XXX messagge with the number of bytes of data that were (or should have
** been stored) in lParam and if the buffer wasn't big enough to hold all the data, wparam will have it's last 2 bits set
** to zero.  If no buffer was given to the DLL for the data to be stored into, the last 2 bits of wparam will be 01.
*  If the data was stored correctly, the last 2 bits of wparam will be 11.
**
** After the message is sent, the buffer is marked by the dll as
** NULL indicating no user buffer available for storage - the buffer should be reset as soon as possible
*  after a WM_xxx message is processed.  A second call to set the data buffer will cause the new buffer to be used for any incoming data.
**
**
** An ample video buffer size would be 5000 bytes
*
** Decode data depends on the type of barcode.  If MacroPDF is buffered, large amounts of data are possible.
*  The first byte is the ssi codetype id, followed by the decode data sent from the scanner.
**
** The image data buffer can be set when the first image transfer status message is sent to the application which holds
** the length information for the entire image.
**
** The parameter data buffer can be set immediately before the call to RequestParameters is made.  A size of 4000 bytes
** should be ample to hold all the parameter number/value pairs.  A call for a single parameter only requires a small
** buffer - 10 bytes is more than enough.
**
** The version data buffer can be set immediately before the call to TransmitVersion.  The amount of data returned is
** variable but a buffer of length 256 should be ample.
**
** Capabilities data buffer can be set immediately before the call to RequestScannerCapabilities.  The amount of data returned is
** variable but a buffer of length 256 should be ample.
**
*/

#define BUFFERSIZE_MASK 	        0x0003
#define BUFFERSIZE_GOOD 	        0x0003
#define BUFFERSIZE_ERROR 	        0x0000
#define NOBUFFER_ERROR 		        0x0001


SSIDLL_API int __stdcall            SetVideoBuffer(int nComPort, unsigned char *pData, long max_length);
SSIDLL_API int __stdcall            SetImageBuffer(int nComPort, unsigned char *pData, long max_length);
SSIDLL_API int __stdcall            SetDecodeBuffer(int nComPort, unsigned char *pData, long max_length);
SSIDLL_API int __stdcall            SetParameterBuffer(int nComPort, unsigned char *pData, long max_length);
SSIDLL_API int __stdcall            SetVersionBuffer(int nComPort, unsigned char *pData, long max_length);
SSIDLL_API int __stdcall            SetCapabilitiesBuffer(int nComPort, unsigned char *pData, long max_length);

/**********************************************************************************************/
/**   Windows Messages sent to calling process ************************************************/



#define WM_DECODE			         WM_APP+1         // Sent if there is decode data available from the scanner
							         		          // wparam is the buffer status of the data stored
							         		          // lparam is the length of the data in bytes (cast to int).


#define WM_IMAGE			         WM_APP+2         // Sent if there is image data available from the scanner
							         		          // wparam is the buffer status of the data stored
							         		          // lparam is the length of the data in bytes (cast to long).

#define WM_VIDEOIMAGE		         WM_APP+3         // Sent if there is a video frame available from the scanner
							         		          // wparam is the buffer status of the data stored
							         		          // lparam is the length of the data in bytes (cast to int).

#define WM_ERROR			         WM_APP+4         // Sent if an error occurred.
							         		          // wParam is the error code (cast to int) (see WM_ERROR codes list )

#define WM_TIMEOUT			         WM_APP+5         // Sent if the scanner does not respond to a request from the library within
							         		          // ...the timeout for the request.
							         		          // wparam is set to zero (reserved for future use)
							         		          // lparam is the request code (int) that did not receive the response
							         		          // ...(see WM_TIMEOUT codes list)

#define WM_CMDCOMPLETEMSG	         WM_APP+6         // Sent when an ACK is received from the scanner in response to a handled
                                                      // user command.

#define WM_XFERSTATUS		         WM_APP+7         // Sent during the transfer of image data from the scanner.
							         		          // wParam is the total number of bytes recieved so far (cast to uint)
							         		          // lParam is the total number of bytes expected (cast to uint)


#define WM_SWVERSION		         WM_APP+8         // Sent when the software version information is available from the scanner
							         		          // wparam is the buffer status code
							         		          // lparam is the length of the data in bytes (cast to int).
							         		          // version data is device dependent


#define WM_PARAMS			         WM_APP+9         // Sent when parameter information is available from the scanner
							         		          // wparam is the buffer status,
							         		          // lparam is the length of the param data (cast to int).

#define WM_CAPABILITIES		         WM_APP+10        // Sent when capabilities data is available from the scanner
							         		          // wparam is the buffer status
							         		          // lparam is the length of the data in bytes (cast to int).


#define WM_EVENT			         WM_APP+11        // Sent when event data is available from the scanner
											          // wparam is the event data
											          // lparam is the length of the data in bytes (always 1 byte).

#endif





