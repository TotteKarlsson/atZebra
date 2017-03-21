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
#include "ssidll/SSIDLL.H"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mtkIniFileC"
#pragma link "mtkIniFileC"
#pragma link "pies"
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
    gCanClose(true),
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
	if(mConnectZebraBtn->Caption == "Open")
    {
        int status = SSIConnect(this->Handle, 9600, getCOMPortNumber());
        Log(lError) << "Connect status: "<<status;

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
void __fastcall TMainForm::aimClick(TObject *Sender)
{
	//Check which one was clicked
    TRadioButton *rb = dynamic_cast<TRadioButton *>(Sender);
    int status;
    if(rb == mAimOnRB)
    {
    	status = AimOn(getCOMPortNumber());
    }
    else
    {
		status = AimOff(getCOMPortNumber());
    }
    Log(lInfo) << "Aim on/off status: "<<status;
}


