// TheRealBarcodeDlg.h : header file
//

#pragma once

// CTheRealBarcodeDlg dialog
class CTheRealBarcodeDlg : public CDialog
{
// Construction
public:
	CTheRealBarcodeDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_THEREALBARCODE_DIALOG };


	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	afx_msg void OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/);
#endif
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButtonConnect();
    afx_msg void OnBnClickedButtonDisconnect();
    afx_msg void OnBnClickedButtonAimOn();
    afx_msg void OnBnClickedButtonAimOff();
    afx_msg void OnBnClickedButtonPullTrigger();
    afx_msg LRESULT OnWMDecode(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMImage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMVideoImage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMError(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMTimeout(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMCMDCompleteMSG(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMXferStatus(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMSWVersion(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMParams(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMCapabilities(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWMEvent(WPARAM wParam, LPARAM lParam);

    CString m_TypeText;
    CString m_DataText;
};
