#pragma hdrstop
#include "TMainForm.h"
#include "mtkVCLUtils.h"
#include "mtkLogger.h"
#include "TAboutZebraForm.h"
#pragma package(smart_init)
using Poco::DateTimeFormatter;

using namespace mtk;

extern HWND gOtherAppWindow;

//This one is called from the reader thread
void __fastcall TMainForm::logMsg()
{
    infoMemo->Lines->Add(vclstr(mLogFileReader.getData()));
    mLogFileReader.purge();
}

void __fastcall TMainForm::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
    if(Key == VK_ESCAPE)
    {
        Close();
    }
}

void __fastcall TMainForm::OpenAboutFormAExecute(TObject *Sender)
{
	TAboutZebraForm* f = new TAboutZebraForm(this);
    f->ShowModal();
    delete f;
}

void __fastcall TMainForm::ClearMemoAExecute(TObject *Sender)
{
    infoMemo->Clear();
}

void TMainForm::setupWindowTitle()
{
	string title = createWindowTitle("atZebra", Application);
	this->Caption = vclstr(title);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::LogLevelCBChange(TObject *Sender)
{
    if(LogLevelCB->ItemIndex == 0)
    {
        mLogLevel = lError;
    }
    else if(LogLevelCB->ItemIndex == 1)
    {
        mLogLevel = lWarning;
    }
    else if(LogLevelCB->ItemIndex == 2)
    {
        mLogLevel = lInfo;
    }
    else if(LogLevelCB->ItemIndex == 3)
    {
        mLogLevel = lDebug;
    }
    else if(LogLevelCB->ItemIndex == 4)
    {
        mLogLevel = lDebug1;
    }
    else if(LogLevelCB->ItemIndex == 5)
    {
        mLogLevel = lDebug2;
    }
    else if(LogLevelCB->ItemIndex == 6)
    {
        mLogLevel = lDebug3;
    }
    else if(LogLevelCB->ItemIndex == 7)
    {
        mLogLevel = lDebug4;
    }
    else if(LogLevelCB->ItemIndex == 8)
    {
        mLogLevel = lDebug5;
    }

    else if(LogLevelCB->ItemIndex == 9)
    {
        mLogLevel = lAny;
    }

    gLogger.setLogLevel(mLogLevel);
}

BOOL CALLBACK FindOtherWindow(HWND hwnd, LPARAM lParam)
{
	static TCHAR buffer[50];
	GetWindowText(hwnd, buffer, 50);

	if(_tcsstr(buffer, "atDB"))
	{
		// do something with hwnd here
		gOtherAppWindow = hwnd;
		return FALSE;
	}

	return TRUE;
}



