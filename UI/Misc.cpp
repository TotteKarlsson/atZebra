#pragma hdrstop
#include "TMainForm.h"
#include "mtkVCLUtils.h"
#include "mtkLogger.h"
#include "TAboutUC7Form.h"
#pragma package(smart_init)
using Poco::DateTimeFormatter;

using namespace mtk;

extern HWND gOtherAppWindow;
//extern TSplashForm*  gSplashForm;
void __fastcall TMainForm::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
    if(Key == VK_ESCAPE)
    {
        Close();
    }
}


void __fastcall TMainForm::OpenAboutFormAExecute(TObject *Sender)
{
	TAboutUC7Form* f = new TAboutUC7Form(this);
    f->ShowModal();
    delete f;
}

void __fastcall TMainForm::ClearMemoAExecute(TObject *Sender)
{
    infoMemo->Clear();
}

void TMainForm::setupWindowTitle()
{
	string title = createWindowTitle("atUC7", Application);
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

void __fastcall TMainForm::mConnectUC7BtnClick(TObject *Sender)
{
	if(mConnectUC7Btn->Caption == "Open")
    {
        if(mUC7.connect(getCOMPortNumber()))
        {
            Log(lInfo) << "Connected to a UC7 device";
        }
        else
        {
            Log(lInfo) << "Connection failed";
        }
    }
    else
    {
        if(!mUC7.disConnect())
        {
			Log(lError) << "Failed to close serial port";
        }
    }

    if(mUC7.isConnected())
    {
	    onConnectedToUC7();
    }
    else
    {
		onDisConnectedToUC7();
    }
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

//---------------------------------------------------------------------------
void __fastcall TMainForm::AppInBox(ATWindowStructMessage& msg)
{
    if(msg.lparam == NULL)
    {
        return;
    }

    try
    {
        ApplicationMessageEnum aMsg = msg.wparam;

        switch(aMsg)
        {
            case atUC7SplashWasClosed:
                Log(lDebug2) << "Splash form sent message that it was closed";
            break;

			case atUC7Message:
            {
            	UC7Message* m = (UC7Message*) msg.lparam;
                Log(lInfo) << "Handling UC7 message:" << m->getMessageNameAsString();
                handleUC7Message(*m);
                delete m;
            }
            default:
            break ;
        }
	}
	catch(...)
	{
		Log(lError) << "Received an unhandled windows message!";
	}
}




