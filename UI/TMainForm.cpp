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
#pragma link "mtkIniFileC"
#pragma link "pies"
#pragma link "TFloatLabeledEdit"
#pragma link "TSTDStringEdit"
#pragma resource "*.dfm"

TMainForm *MainForm;
using namespace mtk;
using namespace ab;

extern string 	gLogFileName;
extern string 	gApplicationRegistryRoot;
extern string 	gDefaultAppTheme;
extern string 	gFullDateTimeFormat;
extern bool 	gIsDevelopmentRelease;
extern string 	gCommonAppDataLocation;

//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
    : TRegistryForm(gApplicationRegistryRoot, "MainForm", Owner),

    mBottomPanelHeight(190),
    mSplashProperties(gApplicationRegistryRoot, "Splash"),
    mShowSplashOnStartup(true),
    mLogLevel(lAny),
    mIsStyleMenuPopulated(false),
    gCanClose(true),
    mLogFileReader(joinPath(gCommonAppDataLocation, gLogFileName), &logMsg),
    mCOMPort(0),
    mZebraConsumer(mZebra, Handle)
{
    TMemoLogger::mMemoIsEnabled = false;
    setupIniFile();
    setupAndReadIniParameters();
    mZebraConsumer.start();
}

//This one is called from the reader thread
void __fastcall TMainForm::logMsg()
{
    infoMemo->Lines->Add(vclstr(mLogFileReader.getData()));
    mLogFileReader.purge();
}

//---------------------------------------------------------------------------
int	TMainForm::getCOMPortNumber()
{
	return mComportCB->ItemIndex + 1;
}

void __fastcall TMainForm::mSendBtn1Click(TObject *Sender)
{
	string msg = mRawCMDE->getValue();
    ZebraMessage uc7Msg(msg, false);
    Log(lInfo) << "Sending message: " << uc7Msg.getFullMessage();
	mZebra.sendRawMessage(uc7Msg.getFullMessage());
}

void __fastcall TMainForm::onConnectedToZebra()
{
    mConnectZebraBtn->Caption         = "Close";
	mRawCMDE->Enabled 		        = true;
	mSendBtn1->Enabled 		        = true;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::onDisConnectedToZebra()
{
    mConnectZebraBtn->Caption         = "Open";
	mRawCMDE->Enabled 		        = false;
	mSendBtn1->Enabled 		        = false;
}

//---------------------------------------------------------------------------
bool TMainForm::handleZebraMessage(const ZebraMessage& m)
{
	//Find out controller address from sender parameter
	switch(toInt(m.sender()))
    {
    	case 1: 		return UNKNOWN;
        case 2:			return UNKNOWN;
        case 3:			return UNKNOWN;
        default: return UNKNOWN;
    }
}

void __fastcall TMainForm::mRawCMDEKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	//Auto calculate the checksum

//  if(Key == VK_RETURN)
    {
        ZebraMessage msg(mRawCMDE->getValue(), false);
        msg.calculateCheckSum();
        mCheckSumEdit->setValue(msg.checksum());
    }
}



