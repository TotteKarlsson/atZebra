#include <vcl.h>
#pragma hdrstop
#include "atVCLUtils.h"
#include "database/atDBUtils.h"
#include "mtkLogger.h"
#include "mtkMoleculixException.h"
#include "mtkSQLiteException.h"
#include "mtkSQLiteQuery.h"
#include "mtkSQLiteTable.h"
#include "mtkStringUtils.h"
#include "mtkUtils.h"
#include "mtkVCLUtils.h"
#include "Poco/DateTime.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Glob.h"
#include "Poco/Timezone.h"
#include "TMainForm.h"
#include "TMemoLogger.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mtkIniFileC"
#pragma link "TFloatLabeledEdit"
#pragma link "TSTDStringEdit"
#pragma resource "*.dfm"

TMainForm *MainForm;
using namespace mtk;
using namespace at;

extern string 	gLogFileName;
extern string 	gApplicationRegistryRoot;
extern string 	gFullDateTimeFormat;
extern bool 	gIsDevelopmentRelease;
extern string 	gCommonAppDataLocation;
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
    : TRegistryForm(gApplicationRegistryRoot, "MainForm", Owner),

    mBottomPanelHeight(190),
    mLogLevel(lAny),
    mLogFileReader(joinPath(gCommonAppDataLocation, gLogFileName), &logMsg),
    mCOMPort(17)
{
    TMemoLogger::mMemoIsEnabled = false;
    setupIniFile();
    setupAndReadIniParameters();
}

//---------------------------------------------------------------------------
int	TMainForm::getCOMPortNumber()
{
	return mComportCB->ItemIndex + 1;
}

void __fastcall TMainForm::mConnectZebraBtnClick(TObject *Sender)
{
	int Comport = getCOMPortNumber();
   	if(mConnectZebraBtn->Caption == "Open")
    {
        int status = SSIConnect(this->Handle, 9600, Comport);
        Log(lError) << "Connect status: "<<status;

        if(status == 0)
        {
//			PostMessage(WM_SENDGETVERSIONMSG, 0,0);

			SetVideoBuffer(Comport, VideoData, MAX_VIDEO_LEN);
         	SetDecodeBuffer(Comport, VideoData, MAX_VIDEO_LEN); // use is mutually exclusive so this is ok
	        SetParameterBuffer(Comport, VideoData, MAX_VIDEO_LEN); // as long as we reset it as soon as we
    	    SetVersionBuffer(Comport, VideoData, MAX_VIDEO_LEN); // get the data
        	SetCapabilitiesBuffer(Comport, VideoData, MAX_VIDEO_LEN);
        }

    }
    else
    {
    }

//    if(mZebra.isConnected())
//    {
//	    onConnectedToZebra();
//    }
//    else
//    {
//		onDisConnectedToZebra();
//    }
}

void __fastcall TMainForm::onConnectedToZebra()
{
    mConnectZebraBtn->Caption       = "Close";
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::onDisConnectedToZebra()
{
    mConnectZebraBtn->Caption         = "Open";
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::Button2Click(TObject *Sender)
{
	int status = SoundBeeper(getCOMPortNumber(), ONESHORTHI);
    Log(lInfo) << "Beep status: "<<status;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SettingsRGClick(TObject *Sender)
{
	//Check which one was clicked
    TRadioGroup* rg = dynamic_cast<TRadioGroup*>(Sender);

    int status;
    if(rg == mScannerAimRG)
    {
    	status  = (rg->ItemIndex == 0 ) ? AimOn(getCOMPortNumber()) : AimOff(getCOMPortNumber());
    }
    else if(rg == mScannerEnabledRG)
    {
    	status  = (rg->ItemIndex == 0 ) ? ScanEnable(getCOMPortNumber()) : ScanDisable(getCOMPortNumber());
    }
    else if(rg == mScannerIllumRG)
    {
    	status  = (rg->ItemIndex == 0 ) ? IlluminationOn(getCOMPortNumber()) : IlluminationOff(getCOMPortNumber());
    }
    else if(rg == mScannerLEDRG)
    {
		status  = (rg->ItemIndex == 0 ) ? LedOn(getCOMPortNumber(), 1) : LedOn(getCOMPortNumber(), 1);
    }

    Log(lInfo) << "Status: "<<status;
}


//---------------------------------------------------------------------------
//void __fastcall TMainForm::AppInBox(ATWindowStructMessage& msg)
//{
//    if(msg.lparam == NULL)
//    {
//        return;
//    }
//
//    try
//    {
//        ApplicationMessageEnum aMsg = msg.wparam;
//
//        switch(aMsg)
//        {
//            case atZebraSplashWasClosed:
//                Log(lDebug2) << "Splash form sent message that it was closed";
//            break;
//
//			case atZebraMessage:
//            {
//            	ZebraMessage* m = (ZebraMessage*) msg.lparam;
//                Log(lInfo) << "Handling Zebra message:" << m->getMessageNameAsString();
//                delete m;
//            }
//            default:
//            break ;
//        }
//	}
//	catch(...)
//	{
//		Log(lError) << "Received an unhandled windows message!";
//	}
//}

void __fastcall TMainForm::onWMDecode(TMessage& Msg)
{
	WPARAM w = Msg.WParam;
    LPARAM l = Msg.LParam;

	//Time do decode
    Log(lInfo) << "Got a onWMDecode message..";

    // wparam contains the status bits for the data ,
	// lparam is the length of the data in bytes (cast to int).
	unsigned char decode_buf[3001];

	// first thing is to copy the contents of the dll's data buffer to our own.
	if((l < 3000) && ((w & BUFFERSIZE_MASK) == BUFFERSIZE_GOOD ))
	{
		memcpy(decode_buf, VideoData,l);
		decode_buf[l] = 0;
	}
	else if( l && ((w & BUFFERSIZE_MASK) == BUFFERSIZE_ERROR ))
    {
		strcpy((char *)decode_buf, "TOO MUCH DECODE DATA");
    }
	else
   	{
    	strcpy((char *)decode_buf, "NO DECODE STORAGE BUFFER");
   	}

	decode_buf[3000] = 0;
    if(decode_buf[0] == 0x1B)
    {
    	Log(lInfo) << "Datamatrix barcode";
    }

    decode_buf[0] = ' ';
    Log(lInfo) <<decode_buf;

	SetDecodeBuffer(getCOMPortNumber(), VideoData, MAX_VIDEO_LEN); // give the dll back the data it needs for the next decode
}

void __fastcall TMainForm::onSSIEvent(TMessage& Msg)
{
	WPARAM w = Msg.WParam;
    LPARAM l = Msg.LParam;
    Log(lInfo) << "There was an onSSIEvent event..";
}

void __fastcall TMainForm::onSSIImage(TMessage& Msg)
{
	WPARAM w = Msg.WParam;
    LPARAM l = Msg.LParam;
    Log(lInfo) << "There was an onSSIImage event..";
}

void __fastcall TMainForm::onSSITimeout(TMessage& Msg)
{
	WPARAM w = Msg.WParam;
    LPARAM l = Msg.LParam;
    Log(lInfo) << "There was an onSSITimeout event..";
}

void __fastcall TMainForm::onSSIError(TMessage& Msg)
{
	WPARAM w = Msg.WParam;
    LPARAM l = Msg.LParam;
    Log(lInfo) << "There was an onSSIError event..";
}
