/*****************************************************************************
*	FILENAME:	gcpjpeg.h
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Defines and external function declaration to build the JPEG 
*						header for the image data sent.
*
*	NOTES:			File was used without comments otherwise as is, in gcp 
*						protocol implementation.
*	
*	AUTHOR:			Mike O'Haire originally, Header added by A.Schuessler
*	CREATION DATE: 10/28/02
*	DERIVED FROM:	File of same name from the gcp protocol implementation.
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/GCPJPEG.H-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:48   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:48   schuesla
 * Initial revision.
*
*****************************************************************************/

#ifndef gcpjpegH
#define gcpjpegH


/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/
#define UNPACK_WORD(x) ((*(BYTE *)(x) << 8) | (*((BYTE *)(x) + 1)))


#define JPEG_QUANT_OFFSET       0x19
#define JPEG_FILE_HEADER_SIZE   0x131
#define BMP_HEADER_SIZE         1078
#define BITMAP24_HEADER_SIZE    54


/****************************************************************************/
/*	Public Function Prototypes **********************************************/
extern int BuildHeader(unsigned char *Original, unsigned char *Updated, int PacketLength);

#endif  // gcpjpegH
