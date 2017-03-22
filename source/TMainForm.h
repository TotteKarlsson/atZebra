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
#include "atExporter.h"
#include "atATObject.h"
#include "barcodereader/atDS457.h"
//---------------------------------------------------------------------------

using mtk::Property;
using mtk::SQLite;
using mtk::MessageContainer;
using mtk::IniFileProperties;

extern string gApplicationRegistryRoot;


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
	TGroupBox *mImagerSettingsGB;
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

        												//!The barcode reader
        DS457											mZebra;

                                                        //INI Parameters...
        IniFileProperties	      	                    mGeneralProperties;
        mtk::Property<int>	                            mBottomPanelHeight;
		mtk::Property<mtk::LogLevel>	                mLogLevel;
		mtk::Property<int>	                			mCOMPort;

        bool                                            setupAndReadIniParameters();
        void                                            setupIniFile();

		void __fastcall 								onConnectedToZebra();
        void __fastcall 								onDisConnectedToZebra();

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
