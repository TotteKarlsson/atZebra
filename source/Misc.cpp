#pragma hdrstop
#include "TMainForm.h"
#include "mtkVCLUtils.h"
#include "mtkLogger.h"
#include "TAboutZebraForm.h"
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

void __fastcall TMainForm::mConnectZebraBtnClick(TObject *Sender)
{
	if(mConnectZebraBtn->Caption == "Open")
    {
        if(mZebra.connect(getCOMPortNumber()))
        {
            Log(lInfo) << "Connected to a Zebra device";
        }
        else
        {
            Log(lInfo) << "Connection failed";
        }
    }
    else
    {
        if(!mZebra.disConnect())
        {
			Log(lError) << "Failed to close serial port";
        }
    }

    if(mZebra.isConnected())
    {
	    onConnectedToZebra();
    }
    else
    {
		onDisConnectedToZebra();
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
            case atZebraSplashWasClosed:
                Log(lDebug2) << "Splash form sent message that it was closed";
            break;

			case atZebraMessage:
            {
            	ZebraMessage* m = (ZebraMessage*) msg.lparam;
                Log(lInfo) << "Handling Zebra message:" << m->getMessageNameAsString();
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




