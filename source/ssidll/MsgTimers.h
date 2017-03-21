/*****************************************************************************
*	FILENAME:	MsgTimers.h
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Typedefs and defines for TTimer class
*
*	NOTES:			Some defines were changed or eliminated from the GCP 
*						protocol implementation. 
*						
*						
*	
*	AUTHOR:			A.Schuessler, original author Mike O'Haire
*	CREATION DATE: 10/28/02
*	DERIVED FROM:  File of same name from gcp protocol implementation
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/MsgTimers.h-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:50   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:48   schuesla
 * Initial revision.
*
*****************************************************************************/

#ifndef __MSGTIMERS_H__
#define __MSGTIMERS_H__

/****************************************************************************/
/*	Defines, typedefs, etc. *************************************************/

class TTimer
{
public:
	TTimer(int hwnd);
	~TTimer(void);
	void Enable(bool);
	void SetInterval(int);
	int GetInterval(void);
	BOOL RunningOn(void);
	void (CALLBACK *OnTimer)(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
	int Tag;
	UINT ID;
private:
	int Hwnd;
	bool Enabled;
	int Interval;
	CRITICAL_SECTION csTimer;

};


#define ABORTTIMEOUT						2000	// when  user asks to abort transfer of image data, set timer to this value
#define COMMANDTIMEOUT					2000	// after user command, we allow this much time to pass
#define RESPONSETIMEOUT					3000	// after user PARAM request AND PARAM SEND , we allow this much time to pass
#define PINGERTIMEOUT					3000	
#define PINGERDEADTIMEOUT				3000	
#define IMAGECAPTURESTARTTIMEOUT		30000	// after T/O send timeout to user if user commanded imager mode but got no data 
#define NEXTPACKETTIMEOUT				2000	// scanner must respond to us within 2 seconds
// #define PACKETSTARTTIMEOUT				500	// time from when get first char to when give up waiting to get entire packet


#endif  //  __MSGTIMERS_H__
