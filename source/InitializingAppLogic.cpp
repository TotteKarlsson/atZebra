#pragma hdrstop
#include "TMainForm.h"
#include "TMemoLogger.h"
#include "mtkVCLUtils.h"
#include "Poco/DateTime.h"
#include "Poco/DateTimeFormatter.h"
#include "mtkLogger.h"
#include "mtkIniSection.h"
#include "Poco/Timezone.h"

extern bool             gAppIsStartingUp;
extern string           gCommonAppDataLocation;
extern string           gLogFileLocation;
extern string           gLogFileName;
//
using namespace mtk;
using Poco::DateTime;
using Poco::DateTimeFormatter;

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCreate(TObject *Sender)
{
    TMemoLogger::mMemoIsEnabled = false;

	setupWindowTitle();
	LogLevelCB->ItemIndex = mLogLevel;
	if(mLogLevel == lInfo)
	{
		StringList msgs;
		msgs.append("WARNING");
		msgs.append("ERROR");
		msgs.append("INFO");

		StringList logs = getLinesInFile(joinPath(gLogFileLocation, gLogFileName));
		//Only add lines to logwindow with lInfo and "higher"
		for(uint i = 0; i < logs.size(); i++)
		{
			if(contains(msgs, logs[i]))
			{
				infoMemo->Lines->Add(vclstr(logs[i]));
			}
		}
	}
	else if(mLogLevel == lAny)
	{
		StringList logs = getLinesInFile(joinPath(gLogFileLocation, gLogFileName));

		//Only add lines to log window with lInfo and "igher"
		for(uint i = 0; i < logs.size(); i++)
		{
			infoMemo->Lines->Add(vclstr(logs[i]));
		}
	}
    enableDisableGroupBox(mImagerSettingsGB, false);
	mLogFileReader.start(true);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormShow(TObject *Sender)
{
	gAppIsStartingUp = false;

	//transfer INI values
	BottomPanel->Height     = mBottomPanelHeight + 1;
	SB->Top = MainForm->Top + MainForm->Height + 10;
	SB->SizeGrip = true;
}

void TMainForm::setupIniFile()
{
	if(!folderExists(gCommonAppDataLocation))
	{
		CreateDir(gCommonAppDataLocation.c_str());
	}

	mIniFileC->init(gCommonAppDataLocation);

	//For convenience and for option form, populate appProperties container
	mAppProperties.append(&mGeneralProperties);
}

bool TMainForm::setupAndReadIniParameters()
{
	mIniFileC->load();
	mGeneralProperties.setIniFile(mIniFileC->getIniFile());

	//Setup parameters
	mGeneralProperties.setSection("GENERAL");
	mGeneralProperties.add((BaseProperty*)  &mBottomPanelHeight.setup( 	            "HEIGHT_OF_BOTTOM_PANEL",    	    205));

	mGeneralProperties.add((BaseProperty*)  &mLogLevel.setup( 	                    "LOG_LEVEL",    	                lAny));
	mGeneralProperties.add((BaseProperty*)  &mCOMPort.setup( 	                    "ZEBRA_COM_PORT",                   0));
	mGeneralProperties.add((BaseProperty*)  &mBaudRate.setup( 	                    "ZEBRA_BAUD_RATE",                  9600));
	//Read from file. Create if file do not exist
	mGeneralProperties.read();

	//Setup UI elements
	mComportCB->ItemIndex = mCOMPort - 1;

    //Find which item should be selected
    for(int i = 0; i < mBaudRateCB->Items->Count; i++)
    {
		if(mBaudRateCB->Items->Strings[i].ToInt() == mBaudRate)
        {
			mBaudRateCB->ItemIndex = i;
            break;
        }
    }
    gLogger.setLogLevel(mLogLevel);

	return true;
}

