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
    mCOMPort(0)//,
//    mZebraConsumer(mZebra, Handle)
{
    TMemoLogger::mMemoIsEnabled = false;
    setupIniFile();
    setupAndReadIniParameters();
//    mZebraConsumer.start();
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
    mStartStopBtn->Enabled 	        = true;
    mResetBtn->Enabled 		        = true;
    mRetractLbl->Enabled            = true;
    mCrankPositionPie->Brush->Color 		= clRed;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::onDisConnectedToZebra()
{
    mConnectZebraBtn->Caption         = "Open";
	mRawCMDE->Enabled 		        = false;
	mSendBtn1->Enabled 		        = false;
    mStartStopBtn->Enabled 	        = false;
    mResetBtn->Enabled 		        = false;
    mRetractLbl->Enabled 	        = false;
    mCrankPositionPie->Brush->Color 		= this->Color;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mStartStopBtnClick(TObject *Sender)
{
	if(mStartStopBtn->Caption == "Start")
    {
    	Log(lInfo) << "Starting cutter";
	    mZebra.startCutter();
	}
    else
    {
	   	Log(lInfo) << "Stopping cutter";
	    mZebra.stopCutter();
    }
}


bool TMainForm::handleZebraMessage(const ZebraMessage& m)
{
	//Find out controller address from sender parameter
	switch(toInt(m.sender()))
    {
    	case 1: 		return UNKNOWN;
        case 2:			return UNKNOWN;
        case 3:			return UNKNOWN;
        case 4:
        {
        	if(m.command() == "20")
            {

            }
            else if(m.command() == "21")
            {

            }
            else if(m.command() == "23")
            {

            }
            else if(m.command() == "30")
            {

            }
            else if(m.command() == "31")
            {

            }
            else if(m.command() == "40")	//Handwheel position
            {

            }
            else if(m.command() == "41")
            {

            }
			else
            {

            }
        }
		case 5:
        {
        	if(m.command() == "20")
            {
            	string d = m.data().substr(2,2);
                if(d == "00")
                {
                	Log(lInfo) << "Cutting motor is off";
                    mStartStopBtn->Caption = "Start";
                }
                else if(d == "01")
                {
                	Log(lInfo) << "Cutting motor is on";
                    mStartStopBtn->Caption = "Stop";
                }
                else if(d == "E0")
                {
                	Log(lError) << "Invalid calibration";
                }

            }
        	else if(m.command() == "30")
            {

            }
        	else if(m.command() == "31")
            {

            }
        	else if(m.command() == "40")
            {
            	string d = m.data().substr(2,2);
                if(d == "00")  //Retract
                {
                	mCrankPositionPie->Angles->EndAngle = 180;
                	mCrankPositionPie->Angles->StartAngle = 90;
                }
                else if(d == "01")  //Before cutting
                {
                	mCrankPositionPie->Angles->EndAngle = 90;
                	mCrankPositionPie->Angles->StartAngle = 0;
                }
                else if(d == "03") //Cutting
                {
                	mCrankPositionPie->Angles->EndAngle = 0;
                	mCrankPositionPie->Angles->StartAngle = 270;
                }
                else if(d == "02") //After cutting
                {
                	mCrankPositionPie->Angles->EndAngle = 270;
                	mCrankPositionPie->Angles->StartAngle = 180;
                }
                else if(d == "E0")
                {

                }
            }
            else
            {

            }
        }
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



