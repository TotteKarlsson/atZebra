#ifndef TMainFormH
#define TMainFormH
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Grids.hpp>
#include <System.Actions.hpp>
#include <Vcl.ActnList.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.StdActns.hpp>
//---------------------------------------------------------------------------
#include "source/atZebraApplicationMessages.h"
#include "mtkIniFileC.h"
#include "mtkIniFileProperties.h"
#include "mtkLogFileReader.h"
#include "mtkLogLevel.h"
#include "mtkMessageContainer.h"
#include "mtkProperty.h"
#include "mtkSQLite.h"
#include "TApplicationProperties.h"
#include "TRegistryForm.h"
#include "TSTDStringLabeledEdit.h"
#include "mtkIniFileC.h"
#include "TFloatLabeledEdit.h"
#include "TSTDStringEdit.h"
#include "ssidll/SSIDLL.H"

using mtk::Property;
using mtk::SQLite;
using mtk::MessageContainer;
using mtk::IniFileProperties;

extern string gApplicationRegistryRoot;

#define MAX_VIDEO_LEN 5000
#define SWTRIG_PARAMNUM			        0x8a
#define HOST_SWTRIG				        0x08
#define EXTENDED_PARAMNUM		        0xf0
#define IMAGE_FILETYPE_PARAMNUM	        0x30
#define BMP_FILE_SELECTION		        0x03
#define TIFF_FILE_SELECTION		        0x04
#define JPEG_FILE_SELECTION		        0x01
#define VIDEOVIEWFINDER_PARAMNUM		0x44

#define WM_USER_GETSWTRIGPARAM 			(WM_USER + 100)
#define WM_USER_GETIMAGETYPES 			(WM_USER_GETSWTRIGPARAM + 1)
#define WM_USER_GETVIEWFINDERPARAM 		(WM_USER_GETIMAGETYPES + 1)
#define WM_SENDGETVERSIONMSG			(WM_USER_GETVIEWFINDERPARAM + 1)
#define WM_SENDGETCAPABILITIESMSG		(WM_SENDGETVERSIONMSG + 1)
#define BITMAP_TYPE				         0	// for m_ImageType
#define TIFF_TYPE				         1
#define JPEG_TYPE				         2
#define IMAGE_TYPE_UNKNOWN		         3

class TMainForm : public TRegistryForm
{
    __published:	// IDE-managed Components
    TStatusBar *SB;
        TActionList *ActionList1;
        TPopupMenu *PopupMenu1;
        TAction *ClearMemoA;
        TMenuItem *ClearMemoA1;
    TSplitter *Splitter1;
	TPanel *mMiddlePanel;
    mtkIniFileC *mIniFileC;
    TMainMenu *MainMenu1;
    TMenuItem *File1;
    TMenuItem *Help1;
    TFileExit *FileExit1;
    TMenuItem *Exit1;
    TAction *OpenAboutFormA;
    TMenuItem *About1;
    TTimer *ShutDownTimer;
	TMemo *infoMemo;
	TPanel *BottomPanel;
	TPanel *Panel1;
	TButton *Button1;
	TComboBox *LogLevelCB;
	TComboBox *mComportCB;
	TButton *mConnectZebraBtn;
	TPanel *mTopPanel;
	TPanel *mMiddleLeftPanel;
	TButton *Button2;
	TGroupBox *GroupBox1;
	TGroupBox *GroupBox2;
	TRadioGroup *mScannerAimRG;
	TRadioGroup *mScannerIllumRG;
	TRadioGroup *mScannerLEDRG;
	TRadioGroup *mScannerEnabledRG;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
    void __fastcall FormCreate(TObject *Sender);

    void __fastcall ClearMemoAExecute(TObject *Sender);
    void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall ShutDownTimerTimer(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall LogLevelCBChange(TObject *Sender);
	void __fastcall mConnectZebraBtnClick(TObject *Sender);
	void __fastcall OpenAboutFormAExecute(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall SettingsRGClick(TObject *Sender);

    private:
        bool                                            gCanClose;
        TApplicationProperties                          mAppProperties;
        int												getCOMPortNumber();
        void __fastcall                                 logMsg();
		LogFileReader                                   mLogFileReader;
		void                                            setupWindowTitle();
		void                                            updateWindowTitle();

                                                        //INI Parameters...
        IniFileProperties	      	                    mGeneralProperties;
        mtk::Property<int>	                            mBottomPanelHeight;
		mtk::Property<mtk::LogLevel>	                mLogLevel;
		mtk::Property<int>	                			mCOMPort;

        bool                                            setupAndReadIniParameters();
        void                                            setupIniFile();

		void __fastcall 								onConnectedToZebra();
        void __fastcall 								onDisConnectedToZebra();
//		void __fastcall                                 AppInBox(ATWindowStructMessage& Msg);


		unsigned char 									VideoData[MAX_VIDEO_LEN];
		unsigned char*									g_pImageData;

														//Decoder events
		void __fastcall                                 onWMDecode(TMessage& Msg);
		void __fastcall                                 onSSIEvent(TMessage& Msg);
		void __fastcall                                 onSSIImage(TMessage& Msg);
		void __fastcall                                 onSSIError(TMessage& Msg);
		void __fastcall                                 onSSITimeout(TMessage& Msg);
		void __fastcall                                 onSSICapabilities(TMessage& Msg);

    public:
                    __fastcall                          TMainForm(TComponent* Owner);
                    __fastcall                          ~TMainForm();

        BEGIN_MESSAGE_MAP
            MESSAGE_HANDLER(WM_DECODE, TMessage, onWMDecode);
//          ON_MESSAGE(WM_SWVERSION, OnSSIVersion)
          	MESSAGE_HANDLER(WM_CAPABILITIES, TMessage, onSSICapabilities)
			MESSAGE_HANDLER(WM_IMAGE, TMessage, onSSIImage)
//          ON_MESSAGE(WM_XFERSTATUS, OnSSIxferStatus)
//          ON_MESSAGE(WM_VIDEOIMAGE, OnSSIVideo)
          	MESSAGE_HANDLER(WM_ERROR, TMessage, onSSIError)
//          ON_MESSAGE(WM_PARAMS, OnSSIParams)
          	MESSAGE_HANDLER(WM_TIMEOUT, TMessage, onSSITimeout)
            MESSAGE_HANDLER(WM_EVENT, TMessage, onSSIEvent)
//          ON_MESSAGE(WM_CMDCOMPLETEMSG, OnSSICommandCompleted)
//          ON_MESSAGE(WM_USER_GETSWTRIGPARAM, OnGetSWTrigParam)
//          ON_MESSAGE(WM_USER_GETIMAGETYPES, OnGetImageFileTypesParam)
//          ON_MESSAGE(WM_USER_GETVIEWFINDERPARAM, OnGetViewFinderParam)
//          ON_MESSAGE(WM_SENDGETVERSIONMSG, OnWM_SENDGETVERSIONMSG)
//          ON_MESSAGE(WM_SENDGETCAPABILITIESMSG, OnWM_SENDGETCAPABILITIESMSG)

        END_MESSAGE_MAP(TForm)


};

extern PACKAGE TMainForm *MainForm;
#endif
