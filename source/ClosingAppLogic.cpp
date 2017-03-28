#pragma hdrstop
#include "TMainForm.h"
#include "mtkLogger.h"
#include "TMemoLogger.h"
#include "mtkVCLUtils.h"
//---------------------------------------------------------------------------

using namespace mtk;
extern string gCommonAppDataLocation;


__fastcall TMainForm::~TMainForm()
{
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ShutDownTimerTimer(TObject *Sender)
{
	ShutDownTimer->Enabled = false;

	if(mLogFileReader.isRunning())
	{
		Log(lDebug) << "Shutting down log file reader";
		mLogFileReader.stop();
	}

	if(mZebra.isConnected())
    {
    	mZebra.disconnect();
    }

    Sleep(50);
    Close();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	Log(lInfo) << "Closing down....";

	//Check if we can close.. abort all threads..
	CanClose = (mLogFileReader.isRunning() || mZebra.isConnected()) ? false : true;
    ShutDownTimer->Enabled = !CanClose;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormClose(TObject *Sender, TCloseAction &Action)
{
	Log(lInfo) << "In FormClose";
	mIniFileC->clear();

	//Save project history
	mBottomPanelHeight          	= BottomPanel->Height;

	mCOMPort = mComportCB->ItemIndex + 1;
    mBaudRate = mBaudRateCB->Items->Strings[mBaudRateCB->ItemIndex].ToInt();
	mGeneralProperties.write();

	//Write to file
	mIniFileC->save();
}

