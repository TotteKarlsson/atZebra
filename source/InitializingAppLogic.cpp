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
extern bool             gIsDevelopmentRelease;
extern string           gCommonAppDataLocation;
extern string           gLogFileLocation;
extern string           gLogFileName;
extern string           gFullDateTimeFormat;
extern string           gDateFormat;
extern string           gTimeFormat;
extern string           gAppName;

//extern TSplashForm*     gSplashForm;
extern string           gTimeFormat;

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

		StringList logs = getLinesInFile(joinPath(gLogFileLocation, gLogFileName));

		StringList msgs;
		msgs.append("WARNING");
		msgs.append("ERROR");
		msgs.append("INFO");

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
	mGeneralProperties.add((BaseProperty*)  &mCOMPort.setup( 	                    "COM_PORT",    	                	0));

	//Read from file. Create if file do not exist
	mGeneralProperties.read();

	//Setup UI elements
	mComportCB->ItemIndex = mCOMPort - 1;

    gLogger.setLogLevel(mLogLevel);

	return true;
}

