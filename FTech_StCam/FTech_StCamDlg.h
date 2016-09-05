
// FTech_StCamDlg.h : header file
//

#pragma once
#include "StCamera.h"
#include "CallbackThread.h"

// CFTech_StCamDlg dialog
class CFTech_StCamDlg : public CDialogEx
{
// Construction
public:
	CFTech_StCamDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_FTECH_STCAM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	
public :
	CStCamera m_StCamera;
	CCallbackThread m_ThdDisplay;

	int m_nWidth;
	int m_nHeight;
	int m_nBpp;
	BITMAPINFO*	m_pBitmapInfo;
	BYTE* m_pBuffer;
	void CallbackFunc();
	void OnDisplay(BYTE* pBuffer);
	void CreateBmpInfo(int nWidth, int nHeight, int nBpp);
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnConnection();
	afx_msg void OnBnClickedBtnAcq();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
