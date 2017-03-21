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
using namespace at;

extern string 	gLogFileName;
extern string 	gApplicationRegistryRoot;
extern string 	gDefaultAppTheme;
extern string 	gFullDateTimeFormat;
extern bool 	gIsDevelopmentRelease;
extern string 	gCommonAppDataLocation;

unsigned char highByte(unsigned int val);
unsigned char lowByte(unsigned int val);
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

void __fastcall TMainForm::onConnectedToZebra()
{
    mConnectZebraBtn->Caption       = "Close";
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

void __fastcall TMainForm::mSendBtn1Click(TObject *Sender)
{
	string msg = mRawCMDE->getValue();
    ZebraMessage uc7Msg(msg, false);
    Log(lInfo) << "Sending message: " << uc7Msg.getFullMessage();
	mZebra.sendRawMessage(uc7Msg.getFullMessage());
}

uint8_t twos_complement(uint8_t val)
{
    return -(unsigned int)val;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Button2Click(TObject *Sender)
{
	//Create a beep message
	vector<unsigned char> header(4);
	vector<unsigned char> data(1);
    vector<unsigned char> message;

    //Message length
    header[0] = 0x5;

    //Beep command
    header[1] = 0xE6;

    //Message Source
    header[2] = 0x4;

    //Status
    header[3] = 0x0;

    //Beep code
    data[0] = 0x0;

    unsigned int cs = 0;
    for(int i = 0; i < header.size(); i++)
    {
        unsigned int t = (unsigned int) header[i];
        cs += t;
    }

    cs += (unsigned int) data[0];
    cs = twos_complement(cs);

    Log(lInfo) << "Checksum: " << cs;


    unsigned char hByte(highByte(cs));
    unsigned char lByte(lowByte(cs));

    header.push_back(hByte);
	header.push_back(lByte);
	header.push_back('\r');

    //Send the message
    string msg;
    for(int i = 0; i < header.size(); i++)
    {
    	msg.push_back(header[i]);

    }
   	mZebra.sendRawMessage(msg);

}

unsigned char highByte(unsigned int val)
{
	return val & 0xff;
}

unsigned char lowByte(unsigned int val)
{
	return (val >> 8) & 0xff;

}
