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
    mCOMPort(17),
    mBaudRate(9600),
    mZebra(this->Handle)
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
        if(mZebra.connect(Comport, mBaudRate) != true)
        {
        	Log(lError) << "Failed to connect barcode reader";
        }
    }
    else
    {
    	mZebra.disconnect();
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

void __fastcall TMainForm::onConnectedToZebra()
{
    mConnectZebraBtn->Caption = "Close";
    enableDisableGroupBox(mImagerSettingsGB, true);

    //Turn into a 'known' state
//	mZebra.beep(ONESHORTLO);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::onDisConnectedToZebra()
{
    mConnectZebraBtn->Caption = "Open";
    enableDisableGroupBox(mImagerSettingsGB, false);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::BtnClick(TObject *Sender)
{
	TButton* b = dynamic_cast<TButton*>(Sender);
    int status(-1);
    if(b == mBeepBtn)
    {
		int status = mZebra.beep(ONESHORTLO);
    }
    else if(b == mDecodeSessionBtn)
    {
    	if(b->Caption == "Start")
        {
        	//Start session
			status = mZebra.startDecodeSession();
			b->Caption = "Stop";
        }
        else
        {
        	//Stop session
			status = mZebra.stopDecodeSession();
            b->Caption = "Start";
            Sleep(100);
            mZebra.illuminationOff();
        }
    }

    Log(lInfo) << "Command return status: "<<status;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::SettingsRGClick(TObject *Sender)
{
	//Check which one was clicked
    TRadioGroup* rg = dynamic_cast<TRadioGroup*>(Sender);

    int status;
    if(rg == mScannerAimRG)
    {
    	status  = (rg->ItemIndex == 0 ) ? mZebra.aimOn() : mZebra.aimOff();
    }
    else if(rg == mScannerEnabledRG)
    {
    	status  = (rg->ItemIndex == 0 ) ? mZebra.scanEnable() : mZebra.scanDisable();
    }
    else if(rg == mScannerIllumRG)
    {
    	status  = (rg->ItemIndex == 0 ) ? mZebra.illuminationOn() : mZebra.illuminationOff();
    }
    else if(rg == mScannerLEDRG)
    {
		status  = (rg->ItemIndex == 0 ) ? mZebra.LEDsOn() : mZebra.LEDsOff();
    }

    Log(lInfo) << "Status: "<<status;
}

void __fastcall TMainForm::onWMDecode(TMessage& Msg)
{
	WPARAM w = Msg.WParam;
    LPARAM l = Msg.LParam;

	//Time do decode
    Log(lInfo) << "Got a onWMDecode message..";

    // wparam contains the status bits for the data ,
	// lparam is the length of the data in bytes (cast to int).
	unsigned char decodeBuffer[3001];

	// first thing is to copy the contents of the dll's data buffer to our own.
	if((l < 3000) && ((w & BUFFERSIZE_MASK) == BUFFERSIZE_GOOD ))
	{
		memcpy(decodeBuffer, mZebra.getMemoryBuffer(), l);
		decodeBuffer[l] = 0;
	}
	else if( l && ((w & BUFFERSIZE_MASK) == BUFFERSIZE_ERROR ))
    {
		strcpy((char *)decodeBuffer, "TOO MUCH DECODE DATA");
    }
	else
   	{
    	strcpy((char *)decodeBuffer, "NO DECODE STORAGE BUFFER");
   	}

	decodeBuffer[3000] = 0;
    if(decodeBuffer[0] == 0x1B)
    {

        decodeBuffer[0] = ' ';
        string data(reinterpret_cast<char const*>(decodeBuffer));
        data = trimWS(data);
    	Log(lInfo) << "A Datamatrix barcode was encoded: "<<data;
        Log(lInfo) << decodeBuffer;
        mBCLabel->Caption = vclstr(data);
        //Stop session
//		mZebra.stopDecodeSession();
        mDecodeSessionBtn->Caption = "Start";
//        Sleep(100);
//        mZebra.illuminationOff();
//        Sleep(100);
//        mZebra.aimOff();
//        Sleep(100);
    }
    else
    {
    	Log(lError) << "Bad barcode reader memory buffer";
    }
}

void __fastcall TMainForm::onSSIEvent(TMessage& Msg)
{
	WPARAM w = Msg.WParam;
    LPARAM l = Msg.LParam;
    Log(lInfo) << "There was an onSSIEvent event..";
}

void __fastcall TMainForm::onSSIImage(TMessage& Msg)
{
//	WPARAM w = Msg.WParam;
//    LPARAM l = Msg.LParam;
//    Log(lInfo) << "There was an onSSIImage event..";
//
//	String msg;
//	unsigned char *pData = NULL;   //(unsigned char *) w;
//	DWORD length = (DWORD)l;
//	unsigned char *pImageType = pData;
//	int filetype = IMAGE_TYPE_UNKNOWN;
//	//CMainFrame * pFrame = (CMainFrame *)AfxGetApp()->m_pMainWnd;  // This is our access to menu related stuff
//
//	if((w & BUFFERSIZE_MASK) == BUFFERSIZE_GOOD)
//	{
//    	msg.Format("Image Complete");	// Main pane of status bar gives status
//		pData = g_pImageData;
//		pImageType = pData;
//	}
//   	else if ((w & BUFFERSIZE_MASK) == BUFFERSIZE_ERROR)
//    {
//    	msg.Format("Image TOO BIG");	// Main pane of status bar gives status
//    }
//   	else
//    {
//    	msg.Format("NO Image BUFFER");	// Main pane of status bar gives status
//    }
//
////	MyMessage = "Received Image";	// Right side tells what message we got from the dll
//	//onSSIxferStatus((WPARAM)l, l);	// Update the transfer status display to show we got all of the expected image data
//
//
//	// Since we are done with the transfer, we'll enable the user to do whatever they want again
//	// ...(as long as it's supported).
//
////	GetDlgItem(IDC_BUTTON_VIDEO)->EnableWindow(1);
////	GetDlgItem(IDC_BUTTON_SNAPSHOT)->EnableWindow(1);
////	if(m_bViewFinderSupported)
////		GetDlgItem(IDC_CHECK_VIEWFINDER)->EnableWindow(1);
////	GetDlgItem(IDC_RADIO_BMP)->EnableWindow(1);
////	GetDlgItem(IDC_RADIO_TIFF)->EnableWindow(1);
////	GetDlgItem(IDC_RADIO_JPEG)->EnableWindow(1);
////	GetDlgItem(IDC_CHECK_SWTRIGGER)->EnableWindow(1);
//
//
//	// If the input data begins with "BM" it's a bitmap
//	// Else if it starts with "MM" or "II" its a tiff - note that we expect big endian from the scanner, and ignore
//	// ...the II for indication for byte ordering.
//
//	if(pImageType != NULL)
//   	{
//   		if((*pImageType == 'B') && (*(pImageType +1) == 'M'))
//	   	{
//        	filetype = BITMAP_TYPE;
//		   	SetupBMPforDisplay( pData,  l);			 // Process input and next Paint message will display our data
//		   	pFrame->m_ImgFileExtension = "bmp";  // Let user choose file save from menu, and set file filter to bmp extension.
//	   	}
//	   	else if (((*pImageType == 'M') && (*(pImageType +1) == 'M')) || ((*pImageType == 'I') && (*(pImageType +1) == 'I')))
//	   	{
//        	SetupTIFFforDisplay( pData,  l);			// Process input and next Paint message will display our data
//		   	filetype = TIFF_TYPE;
//		   	pFrame->m_ImgFileExtension = "tif"; // Let user choose file save from menu, and set file filter to tif extension.
//	   	}
//	   	else if((*pImageType == 0xff) && (*(pImageType +1) == 0xd8))
//	   	{
//        	filetype = JPEG_TYPE;
//			SetupJPEGforDisplay(pData,l, FALSE); // False indicates this is not video
//		   							// Process input and next Paint message will display our data
//
//		   	pFrame->m_ImgFileExtension = "jpg";	// Let user choose file save from menu, and set file filter to jpg extension.
//	   	}
//	   	else
//	   	{
//        	MessageBox("Unknown image format received");
////		   	pFrame->m_ImgFileExtension = "";		// This will indicate  file save from menu needs to be disabled.
//	   	}
//
//	   	m_ImgFileExt = 	pFrame->m_ImgFileExtension;	// Our copy, so we stay in sync with the menu.
//
//	   	if(filetype != IMAGE_TYPE_UNKNOWN)
//        {
//		   WriteImgfile((LPTSTR)pData, length, filetype);	// We always write a temp file since we can't hang on to the
//															// ...dll's data to do it later.
//        }
//   }
//	m_WaitingForSnapShot = FALSE;
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
    Log(lInfo) << "There was an onSSIError event.."<<w<<" : "<<l;
}

void __fastcall TMainForm::onSSICapabilities(TMessage& Msg)
{
	WPARAM w = Msg.WParam;
    LPARAM l = Msg.LParam;
    Log(lInfo) << "There was an onSSICapabilities event..";
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mTestStartBtnClick(TObject *Sender)
{
	TestingTimer->Enabled = !TestingTimer->Enabled;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::TestingTimerTimer(TObject *Sender)
{
	//Read barcode
    mDecodeSessionBtn->Click();
}


