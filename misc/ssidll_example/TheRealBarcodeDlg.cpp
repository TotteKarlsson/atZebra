// TheRealBarcodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TheRealBarcode.h"
#include "TheRealBarcodeDlg.h"
#include "SSIControl.h"
#include "SSIDLL.H"


//globally defined data storage
unsigned char DecodeData[DECODE_DATA_LEN];
unsigned char ParamReturnData[PARAM_RETURN_DATA_LEN];
unsigned char ParamSendData[PARAM_SEND_DATA_LEN];
int status;
TCHAR message[128];
HANDLE  g_hCmdComplete;
HANDLE  g_hParams;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CTheRealBarcodeDlg dialog

CTheRealBarcodeDlg::CTheRealBarcodeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTheRealBarcodeDlg::IDD, pParent)
    , m_TypeText(_T(""))
    , m_DataText(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTheRealBarcodeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_TYPE, m_TypeText);
    DDX_Text(pDX, IDC_EDIT_DATA, m_DataText);
}

BEGIN_MESSAGE_MAP(CTheRealBarcodeDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	ON_WM_SIZE()
#endif
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BUTTON_CONNECT,      &CTheRealBarcodeDlg::OnBnClickedButtonConnect)
    ON_BN_CLICKED(IDC_BUTTON_DISCONNECT,   &CTheRealBarcodeDlg::OnBnClickedButtonDisconnect)
    ON_BN_CLICKED(IDC_BUTTON_AIM_ON,       &CTheRealBarcodeDlg::OnBnClickedButtonAimOn)
    ON_BN_CLICKED(IDC_BUTTON_AIM_OFF,      &CTheRealBarcodeDlg::OnBnClickedButtonAimOff)
    ON_BN_CLICKED(IDC_BUTTON_PULL_TRIGGER, &CTheRealBarcodeDlg::OnBnClickedButtonPullTrigger)
    
    //Messages that can come back from SSIDLL
    ON_MESSAGE(WM_DECODE,           &CTheRealBarcodeDlg::OnWMDecode)
    ON_MESSAGE(WM_IMAGE,            &CTheRealBarcodeDlg::OnWMImage)
    ON_MESSAGE(WM_VIDEOIMAGE,       &CTheRealBarcodeDlg::OnWMVideoImage)
    ON_MESSAGE(WM_ERROR,            &CTheRealBarcodeDlg::OnWMError)
    ON_MESSAGE(WM_TIMEOUT,          &CTheRealBarcodeDlg::OnWMTimeout)
    ON_MESSAGE(WM_CMDCOMPLETEMSG,   &CTheRealBarcodeDlg::OnWMCMDCompleteMSG)
    ON_MESSAGE(WM_XFERSTATUS,       &CTheRealBarcodeDlg::OnWMXferStatus)
    ON_MESSAGE(WM_SWVERSION,        &CTheRealBarcodeDlg::OnWMSWVersion)
    ON_MESSAGE(WM_PARAMS,           &CTheRealBarcodeDlg::OnWMParams)
    ON_MESSAGE(WM_CAPABILITIES,     &CTheRealBarcodeDlg::OnWMCapabilities)
    ON_MESSAGE(WM_EVENT,            &CTheRealBarcodeDlg::OnWMEvent)
END_MESSAGE_MAP()



#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CTheRealBarcodeDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
	if (AfxIsDRAEnabled())
	{
		DRA::RelayoutDialog(
			AfxGetResourceHandle(), 
			this->m_hWnd, 
			DRA::GetDisplayMode() != DRA::Portrait ? 
			MAKEINTRESOURCE(IDD_THEREALBARCODE_DIALOG_WIDE) : 
			MAKEINTRESOURCE(IDD_THEREALBARCODE_DIALOG));
	}
}
#endif


BOOL CTheRealBarcodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    // Create events used to coordinate command sequences
    g_hCmdComplete = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == g_hCmdComplete)
    {
        wsprintf(message, L"failed to create g_hCmdComplete event");
        AfxMessageBox(message);    
    }
    g_hParams     = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == g_hParams)
    {
        wsprintf(message, L"failed to create g_hParams event");
        AfxMessageBox(message);    
    }

    return TRUE;  // return TRUE  unless you set the focus to a control
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//   BUTTON HANDLERS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTheRealBarcodeDlg::OnBnClickedButtonConnect()
{   // CONNECT

    status = SSIConnect(this->m_hWnd, 9600, BARCODE_COM_PORT);
    if(status)
    {
        wsprintf(message, L"SSIConnect ERROR number %d", status);
        AfxMessageBox(message);    
    }

    // Need a few sequential commands to configure the module
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) InitializeScannerThread, this->m_hWnd, 0, NULL);
}

void CTheRealBarcodeDlg::OnBnClickedButtonDisconnect()
{   // DISCONNECT

    status = SSIDisconnect(this->m_hWnd, BARCODE_COM_PORT);
    if(status)
    {
        wsprintf(message, L"ERROR number %d", status);
        AfxMessageBox(message);    
    }
}

void CTheRealBarcodeDlg::OnBnClickedButtonAimOn()
{   // AIM ON

    status = AimOn(BARCODE_COM_PORT);
    if(status)
    {
        wsprintf(message, L"ERROR number %d", status);
        AfxMessageBox(message);
    }
}

void CTheRealBarcodeDlg::OnBnClickedButtonAimOff()
{   // AIM OFF

    status = AimOff(BARCODE_COM_PORT);
    if(status)
    {
        wsprintf(message, L"ERROR number %d", status);
        AfxMessageBox(message);
    }
}

void CTheRealBarcodeDlg::OnBnClickedButtonPullTrigger()
{   // PULL TRIGGER
    // Sequence of events requires it to be done in a seperate thread
    //  to be able to receive Windows Messages during progress
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ScanBarcodeThread, NULL, 0, NULL);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//   WINDOWS MESSAGE HANDLERS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
LRESULT CTheRealBarcodeDlg::OnWMDecode(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_DECODE: 0x%X, 0x%X\r\n"), wParam, lParam));

    int i;
    TCHAR tmpData[DECODE_DATA_LEN];

    // copy barcode symbol data into a string buffer
    for( i = 0; i < DECODE_DATA_LEN-1; i++ )
    {
        tmpData[i] = (TCHAR)DecodeData[i+1];
    }

    wsprintf(message, L"0x%02X", DecodeData[0]);
    m_TypeText = message;      // show the barcode type ID
    m_DataText = tmpData;      // show the barcode data
    UpdateData(FALSE);
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMImage(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_IMAGE: 0x%X, 0x%X\r\n"), wParam, lParam));
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMVideoImage(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_VIDEOIMAGE: 0x%X, 0x%X\r\n"), wParam, lParam));
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMError(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_ERROR: %d, 0x%X\r\n"), wParam, lParam));
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMTimeout(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_TIMEOUT: 0x%X, 0x%X\r\n"), wParam, lParam));
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMCMDCompleteMSG(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_CMDCOMPLETEMSG: 0x%X, 0x%X\r\n"), wParam, lParam));
    PulseEvent(g_hCmdComplete);
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMXferStatus(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_XFERSTATUS: 0x%X, 0x%X\r\n"), wParam, lParam));
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMSWVersion(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_VERSION: 0x%X, 0x%X\r\n"), wParam, lParam));
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMParams(WPARAM wParam, LPARAM lParam)

{
    RETAILMSG(1, (TEXT("WM_PARAMS: 0x%X, 0x%X\r\n"), wParam, lParam));
    PulseEvent(g_hParams);
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMCapabilities(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_CAPABILITIES: 0x%X, 0x%X\r\n"), wParam, lParam));
    return 0;
}
LRESULT CTheRealBarcodeDlg::OnWMEvent(WPARAM wParam, LPARAM lParam)
{
    RETAILMSG(1, (TEXT("WM_EVENT: 0x%X, 0x%X\r\n"), wParam, lParam));
    return 0;
}