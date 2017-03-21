/*****************************************************************************
*	FILENAME:	MsgTimers.cpp
*	COPYRIGHT(c) 2002 Symbol Technologies Inc.  All rights reserved
*
*	DESCRIPTION:	Implementation file for TTimer Class
*
*	NOTES:
*	
*	AUTHOR:	A.Schuessler, Originally M.O'Haire
*	CREATION DATE: 10/28/02
*	DERIVED FROM: file of same name by Mike O'Haire
*
*	EDIT HISTORY:
*	$Log:   U:/SSI_SDK/archives/SSI_SDKv2.0/ssi_sdk/ssidll/MsgTimers.cpp-arc  $
 * 
 *    Rev 1.0   Nov 22 2002 13:25:48   fitzsimj
 * Initial revision.
 * 
 *    Rev 1.0   Oct 28 2002 14:38:48   schuesla
 * Initial revision.
*
*****************************************************************************/


/****************************************************************************/
/*	Include Files ***********************************************************/

//#include "stdafx.h"
#include <windows.h>
#include "MsgTimers.h"

/****************************************************************************/
/*	Public Variables *********************************************************/
UINT IDs = 0;


/*****************************************************************************
*	SYNOPSIS:		TTimer::TTimer(int hwnd)
*
*	DESCRIPTION:	Constructor for TTimer class - initialization of data members
*
*	PARAMETERS:		hwnd:	handle to window that receives windows messages from the
*								dll 
*
*	RETURN VALUE:	none
*
******************************************************************************/

TTimer::TTimer(int hwnd)
{
	Enabled = FALSE;

	OnTimer = 0;
	Interval = 0;
	Tag = 0;
	Hwnd = hwnd;
	InitializeCriticalSection(&csTimer);
};


/*****************************************************************************
*	SYNOPSIS:		TTimer::~TTimer(void)
*
*	DESCRIPTION:	Destructor for the TTimer class - if enabled, Kills timer
*
*	PARAMETERS:		none
*
*	RETURN VALUE:	none
*
******************************************************************************/



TTimer::~TTimer(void)
{
	if(OnTimer && Enabled)
		KillTimer((HWND)Hwnd, ID);
	Enabled = FALSE;
	OnTimer = 0;
	Interval = 0;
	Tag = 0;
	ID = 0;
	DeleteCriticalSection(&csTimer);
};

/*****************************************************************************
*	SYNOPSIS:		void TTimer::Enable(bool flag)
*
*	DESCRIPTION:	Enables/Disables timer
*
*	PARAMETERS:		flags:	TRUE to enable timer, FALSE to disable timer
*
*	RETURN VALUE:	none
*	INPUTS:
*	OUTPUTS:
*
*	NOTES:
*
*	PSEUDOCODE:		if enabled flag is true and input flag is FALSE
*							call the kill timer function with the window handle and timer id
*						else if the enabled flag is FALSE and the input flag is true
*							Set the value for ID with the next IDs value and increment IDs value
*							Call the SetTimer function
*							if SetTimer returns true, set enabled to true
*							if enabled is FALSE
*								set ID to 0;
*
*
******************************************************************************/

void TTimer::Enable(bool flag)
{
	UINT Set;
	

	EnterCriticalSection(&csTimer);
	if(Enabled && (flag == FALSE))  // don't reset interval here - let it keep it's former value
	{
		KillTimer((HWND)Hwnd, ID);
		Enabled = FALSE;
	}
	else if(!Enabled && (flag) && Interval)
	{
		ID = IDs++;
		Set = SetTimer((HWND)Hwnd, ID, Interval, (TIMERPROC)OnTimer);
		if(Hwnd == NULL)
			ID = Set;
		Enabled = (Set != 0);
		if(!Enabled)
			ID = 0;
	}
	LeaveCriticalSection(&csTimer);

	
};
void TTimer::SetInterval(int i)
{
	EnterCriticalSection(&csTimer);
	Interval = i;
	LeaveCriticalSection(&csTimer);
}
int TTimer::GetInterval(void)
{
	int i;

	EnterCriticalSection(&csTimer);
	i = Interval;
	LeaveCriticalSection(&csTimer);
	return i;
}

BOOL TTimer::RunningOn(void)
{
	BOOL i = FALSE;

	EnterCriticalSection(&csTimer);
	if(Interval  && Enabled)
		i = TRUE;
	LeaveCriticalSection(&csTimer);
	return i;
}
