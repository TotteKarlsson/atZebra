object MainForm: TMainForm
  Left = 0
  Top = 0
  Caption = 'Zebra Test Application'
  ClientHeight = 695
  ClientWidth = 916
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  KeyPreview = True
  Menu = MainMenu1
  OldCreateOrder = False
  Position = poDefault
  Scaled = False
  OnClose = FormClose
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Splitter1: TSplitter
    Left = 0
    Top = 483
    Width = 916
    Height = 3
    Cursor = crVSplit
    Align = alBottom
    ExplicitLeft = 88
    ExplicitTop = 430
    ExplicitWidth = 990
  end
  object mMiddlePanel: TPanel
    Left = 0
    Top = 41
    Width = 916
    Height = 442
    Align = alClient
    Constraints.MinHeight = 276
    TabOrder = 0
    ExplicitWidth = 744
    ExplicitHeight = 369
    object mMiddleLeftPanel: TPanel
      Left = 1
      Top = 1
      Width = 336
      Height = 440
      Align = alLeft
      TabOrder = 0
      ExplicitHeight = 367
      object mImagerSettingsGB: TGroupBox
        Left = 1
        Top = 1
        Width = 334
        Height = 438
        Align = alClient
        Caption = 'Functions and Settings'
        TabOrder = 0
        ExplicitHeight = 365
        object mScannerAimRG: TRadioGroup
          Left = 9
          Top = 83
          Width = 145
          Height = 49
          Caption = 'Aim'
          Columns = 2
          ItemIndex = 1
          Items.Strings = (
            'On'
            'Off')
          TabOrder = 0
          OnClick = SettingsRGClick
        end
        object mScannerIllumRG: TRadioGroup
          Left = 171
          Top = 23
          Width = 145
          Height = 49
          Caption = 'Illumination'
          Columns = 2
          ItemIndex = 1
          Items.Strings = (
            'On'
            'Off')
          TabOrder = 1
          OnClick = SettingsRGClick
        end
        object mScannerLEDRG: TRadioGroup
          Left = 171
          Top = 83
          Width = 145
          Height = 49
          Caption = 'LED'
          Columns = 2
          ItemIndex = 0
          Items.Strings = (
            'On'
            'Off')
          TabOrder = 2
          OnClick = SettingsRGClick
        end
        object mScannerEnabledRG: TRadioGroup
          Left = 9
          Top = 23
          Width = 145
          Height = 49
          Caption = 'Enabled'
          Columns = 2
          ItemIndex = 0
          Items.Strings = (
            'On'
            'Off')
          TabOrder = 3
          OnClick = SettingsRGClick
        end
        object GroupBox1: TGroupBox
          Left = 2
          Top = 378
          Width = 330
          Height = 58
          Align = alBottom
          Caption = 'Beeper'
          TabOrder = 4
          ExplicitTop = 305
          object mBeepBtn: TButton
            Left = 16
            Top = 24
            Width = 75
            Height = 25
            Caption = 'Beep'
            TabOrder = 0
            OnClick = BtnClick
          end
        end
        object GroupBox2: TGroupBox
          Left = 2
          Top = 249
          Width = 330
          Height = 129
          Align = alBottom
          Caption = 'Session'
          TabOrder = 5
          ExplicitTop = 176
          object mDecodeSessionBtn: TButton
            Left = 16
            Top = 24
            Width = 89
            Height = 81
            Caption = 'Start'
            TabOrder = 0
            OnClick = BtnClick
          end
        end
      end
    end
    object Panel2: TPanel
      Left = 337
      Top = 1
      Width = 578
      Height = 440
      Align = alClient
      TabOrder = 1
      ExplicitWidth = 406
      ExplicitHeight = 367
      object mBCLabel: TLabel
        AlignWithMargins = True
        Left = 4
        Top = 4
        Width = 131
        Height = 39
        Align = alClient
        Alignment = taCenter
        Caption = 'Barcode'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clRed
        Font.Height = -32
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
        Layout = tlCenter
      end
      object GroupBox3: TGroupBox
        Left = 16
        Top = 320
        Width = 417
        Height = 105
        Caption = 'Testing'
        TabOrder = 0
        object mTestStartBtn: TButton
          Left = 16
          Top = 48
          Width = 75
          Height = 25
          Caption = 'Start'
          TabOrder = 0
          OnClick = mTestStartBtnClick
        end
      end
    end
  end
  object SB: TStatusBar
    Left = 0
    Top = 676
    Width = 916
    Height = 19
    Panels = <>
    ExplicitTop = 603
    ExplicitWidth = 744
  end
  object BottomPanel: TPanel
    Left = 0
    Top = 486
    Width = 916
    Height = 190
    Align = alBottom
    TabOrder = 2
    ExplicitTop = 413
    ExplicitWidth = 744
    object infoMemo: TMemo
      Left = 1
      Top = 49
      Width = 914
      Height = 140
      Align = alClient
      ScrollBars = ssBoth
      TabOrder = 0
      WordWrap = False
      ExplicitWidth = 742
    end
    object Panel1: TPanel
      Left = 1
      Top = 1
      Width = 914
      Height = 48
      Align = alTop
      TabOrder = 1
      ExplicitWidth = 742
      object Button1: TButton
        Left = 8
        Top = 12
        Width = 75
        Height = 25
        Caption = 'Clear'
        TabOrder = 0
        OnClick = ClearMemoAExecute
      end
      object LogLevelCB: TComboBox
        Left = 89
        Top = 12
        Width = 145
        Height = 21
        TabOrder = 1
        Text = 'INFO'
        OnChange = LogLevelCBChange
        Items.Strings = (
          'ERRORS'
          'WARNINGS'
          'INFO'
          'DEBUG'
          'DEBUG1'
          'DEBUG2'
          'DEBUG3'
          'DEBUG4'
          'DEBUG5'
          'EVERYTHING')
      end
    end
  end
  object mTopPanel: TPanel
    Left = 0
    Top = 0
    Width = 916
    Height = 41
    Align = alTop
    TabOrder = 3
    ExplicitWidth = 744
    object mComportCB: TComboBox
      Left = 8
      Top = 14
      Width = 87
      Height = 21
      TabOrder = 0
      Text = 'COM1'
      Items.Strings = (
        'COM1'
        'COM2'
        'COM3'
        'COM4'
        'COM5'
        'COM6'
        'COM7'
        'COM8'
        'COM9'
        'COM10'
        'COM11'
        'COM12'
        'COM13'
        'COM14'
        'COM15'
        'COM16'
        'COM17'
        'COM18'
        'COM19'
        'COM20')
    end
    object mConnectZebraBtn: TButton
      Left = 209
      Top = 12
      Width = 50
      Height = 25
      Caption = 'Open'
      TabOrder = 1
      OnClick = mConnectZebraBtnClick
    end
    object mBaudRateCB: TComboBox
      Left = 111
      Top = 15
      Width = 76
      Height = 21
      ItemIndex = 0
      TabOrder = 2
      Text = '9600'
      Items.Strings = (
        '9600'
        '14400'
        '19200'
        '38400'
        '57600'
        '115200'
        '128000'
        '256000')
    end
  end
  object ActionList1: TActionList
    Left = 584
    Top = 192
    object ClearMemoA: TAction
      Category = 'Memo'
      Caption = 'Clear Messages'
      OnExecute = ClearMemoAExecute
    end
    object FileExit1: TFileExit
      Caption = 'E&xit'
      Hint = 'Exit|Quits the application'
      ImageIndex = 43
    end
    object OpenAboutFormA: TAction
      Caption = 'About'
      OnExecute = OpenAboutFormAExecute
    end
  end
  object PopupMenu1: TPopupMenu
    Left = 272
    Top = 440
    object ClearMemoA1: TMenuItem
      Action = ClearMemoA
    end
  end
  object mIniFileC: mtkIniFileC
    IniFileName = 'atZebra.ini'
    RootFolder = '.'
    Left = 482
    Top = 88
  end
  object MainMenu1: TMainMenu
    Left = 413
    Top = 248
    object File1: TMenuItem
      Caption = 'File'
      object Exit1: TMenuItem
        Action = FileExit1
      end
    end
    object Help1: TMenuItem
      Caption = 'Help'
      object About1: TMenuItem
        Action = OpenAboutFormA
      end
    end
  end
  object ShutDownTimer: TTimer
    Enabled = False
    Interval = 50
    OnTimer = ShutDownTimerTimer
    Left = 506
    Top = 240
  end
  object TestingTimer: TTimer
    Enabled = False
    OnTimer = TestingTimerTimer
    Left = 648
    Top = 376
  end
end
