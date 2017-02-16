object MainForm: TMainForm
  Left = 0
  Top = 0
  Caption = 'UC7 Controller'
  ClientHeight = 643
  ClientWidth = 983
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
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Splitter1: TSplitter
    Left = 0
    Top = 431
    Width = 983
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
    Width = 983
    Height = 390
    Align = alClient
    TabOrder = 0
    object mMiddleLeftPanel: TPanel
      Left = 1
      Top = 1
      Width = 320
      Height = 388
      Align = alLeft
      TabOrder = 0
      object HandwheelGB: TGroupBox
        Left = 1
        Top = 240
        Width = 318
        Height = 147
        Align = alBottom
        Caption = 'Handwheel Position'
        TabOrder = 0
        object mCrankPositionPie: TPie
          Left = 95
          Top = 36
          Width = 90
          Height = 90
          Angles.StartAngle = 90
          Angles.EndAngle = 180
          Brush.Color = clCream
        end
        object mRetractLbl: TLabel
          Left = 40
          Top = 36
          Width = 36
          Height = 13
          Caption = 'Retract'
          Enabled = False
        end
        object Label2: TLabel
          Left = 191
          Top = 36
          Width = 70
          Height = 13
          Caption = 'Before Cutting'
          Enabled = False
        end
        object Label3: TLabel
          Left = 191
          Top = 116
          Width = 35
          Height = 13
          Caption = 'Cutting'
          Enabled = False
        end
        object Label4: TLabel
          Left = 40
          Top = 116
          Width = 63
          Height = 13
          Caption = 'After Cutting'
          Enabled = False
        end
      end
      object GroupBox3: TGroupBox
        Left = 1
        Top = 1
        Width = 318
        Height = 239
        Align = alClient
        Caption = 'Cutting Motor Controls'
        TabOrder = 1
        object mStartStopBtn: TButton
          Left = 40
          Top = 24
          Width = 219
          Height = 81
          Caption = 'Start'
          Enabled = False
          TabOrder = 0
          OnClick = mStartStopBtnClick
        end
        object Panel2: TPanel
          Left = 2
          Top = 128
          Width = 314
          Height = 109
          Align = alBottom
          BevelOuter = bvNone
          TabOrder = 1
          object GroupBox1: TGroupBox
            Left = 149
            Top = 0
            Width = 165
            Height = 109
            Align = alClient
            Caption = 'Return Speed (um/s)'
            TabOrder = 0
            object FloatLabeledEdit2: TFloatLabeledEdit
              Left = 6
              Top = 48
              Width = 89
              Height = 21
              EditLabel.Width = 30
              EditLabel.Height = 13
              EditLabel.Caption = 'Speed'
              TabOrder = 0
              Text = '-1.00'
              Value = -1.000000000000000000
            end
          end
          object GroupBox2: TGroupBox
            Left = 0
            Top = 0
            Width = 149
            Height = 109
            Align = alLeft
            Caption = 'Cutting Speed (um/s)'
            TabOrder = 1
            object FloatLabeledEdit1: TFloatLabeledEdit
              Left = 12
              Top = 48
              Width = 89
              Height = 21
              EditLabel.Width = 30
              EditLabel.Height = 13
              EditLabel.Caption = 'Speed'
              TabOrder = 0
              Text = '-1.00'
              Value = -1.000000000000000000
            end
          end
        end
      end
    end
  end
  object SB: TStatusBar
    Left = 0
    Top = 624
    Width = 983
    Height = 19
    Panels = <>
  end
  object BottomPanel: TPanel
    Left = 0
    Top = 434
    Width = 983
    Height = 190
    Align = alBottom
    TabOrder = 2
    object infoMemo: TMemo
      Left = 1
      Top = 49
      Width = 981
      Height = 140
      Align = alClient
      ScrollBars = ssBoth
      TabOrder = 0
      WordWrap = False
    end
    object Panel1: TPanel
      Left = 1
      Top = 1
      Width = 981
      Height = 48
      Align = alTop
      TabOrder = 1
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
    Width = 983
    Height = 41
    Align = alTop
    TabOrder = 3
    object mComportCB: TComboBox
      Left = 8
      Top = 12
      Width = 145
      Height = 21
      ItemIndex = 0
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
    object mConnectUC7Btn: TButton
      Left = 165
      Top = 10
      Width = 50
      Height = 25
      Caption = 'Open'
      TabOrder = 1
      OnClick = mConnectUC7BtnClick
    end
    object mSendBtn1: TButton
      Left = 396
      Top = 10
      Width = 50
      Height = 25
      Caption = '->'
      Enabled = False
      TabOrder = 2
      OnClick = mSendBtn1Click
    end
    object mResetBtn: TButton
      Left = 897
      Top = 1
      Width = 85
      Height = 39
      Align = alRight
      Caption = 'Reset UC7'
      Enabled = False
      TabOrder = 4
    end
    object mCheckSumEdit: TSTDStringEdit
      Left = 504
      Top = 10
      Width = 49
      Height = 21
      TabOrder = 5
      Text = 'CC'
      Value = 'CC'
    end
    object mRawCMDE: TSTDStringEdit
      Left = 245
      Top = 12
      Width = 145
      Height = 21
      Enabled = False
      TabOrder = 3
      Text = '81F0F8'
      OnKeyDown = mRawCMDEKeyDown
      Value = '81F0F8'
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
    Left = 512
    Top = 232
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
    Left = 533
    Top = 136
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
    Left = 578
    Top = 72
  end
end
