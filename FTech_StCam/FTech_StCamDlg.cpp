
// FTech_StCamDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FTech_StCam.h"
#include "FTech_StCamDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFTech_StCamDlg dialog
CFTech_StCamDlg::CFTech_StCamDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFTech_StCamDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pBitmapInfo = NULL;
	m_nWidth	= 0;
	m_nHeight	= 0; 
	m_nBpp		= 0;
	m_pBuffer   = NULL;
}

void CFTech_StCamDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFTech_StCamDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECTION, &CFTech_StCamDlg::OnBnClickedBtnConnection)
	ON_BN_CLICKED(IDC_BTN_ACQ, &CFTech_StCamDlg::OnBnClickedBtnAcq)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_SAVE, &CFTech_StCamDlg::OnBnClickedBtnSave)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CFTech_StCamDlg message handlers

BOOL CFTech_StCamDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_ThdDisplay.RegisterCallback(this,&CFTech_StCamDlg::CallbackFunc);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFTech_StCamDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFTech_StCamDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CFTech_StCamDlg::OnBnClickedBtnConnection()
{
	CString caption=L"";
	GetDlgItemText(IDC_BTN_CONNECTION,caption);

	if (caption == L"Connect")
	{
		if (m_StCamera.IsConnected() == true) return;
		bool ret = m_StCamera.OnConnect();
		if (ret == true)
		{
			m_nWidth  = m_StCamera.GetWidth();
			m_nHeight = m_StCamera.GetHeight();
			m_nBpp	  = m_StCamera.GetBPP();

			m_pBuffer = new BYTE[m_nWidth*m_nHeight*m_nBpp/8];
			memset(m_pBuffer, 0, m_nWidth*m_nHeight*m_nBpp/8);

			CreateBmpInfo(m_nWidth, m_nHeight ,m_nBpp);
			CString model=L"";
			m_StCamera.GetDeviceModelName(model);
			SetDlgItemText(IDC_LB_MODEL,model);
			GetDlgItem(IDC_BTN_ACQ)->EnableWindow(TRUE);
			SetDlgItemText(IDC_BTN_ACQ, L"Start");
			SetDlgItemText(IDC_BTN_CONNECTION, L"Disconnect");

			m_StCamera.SetTriggerMode(L"On");
			m_StCamera.SetTriggerSource(L"Hardware");
			m_StCamera.SetExposureMode(L"TriggerWidth");
			m_StCamera.SetExposureTime(15.55);
		}
	}
	else
	{
		if (m_StCamera.IsConnected() == false) return;

		if (m_ThdDisplay.IsDone() == false)
			m_ThdDisplay.Stop();

		m_StCamera.OnDisconnect();
		SetDlgItemText(IDC_LB_MODEL,L"Disconnected");
		GetDlgItem(IDC_BTN_ACQ)->EnableWindow(FALSE);
		SetDlgItemText(IDC_BTN_CONNECTION, L"Connect");

		if (m_pBuffer != NULL)
		{
			delete []m_pBuffer;
			m_pBuffer = NULL;
		}
	}
}

void CFTech_StCamDlg::OnBnClickedBtnAcq()
{
	CString caption=L"";
	GetDlgItemText(IDC_BTN_ACQ,caption);

	if (caption == L"Start")
	{
		SetTimer(100,30,NULL);
		m_ThdDisplay.Start();
		m_StCamera.OnStartAcquisition();
		GetDlgItem(IDC_BTN_SAVE)->EnableWindow(TRUE);
		SetDlgItemText(IDC_BTN_ACQ, L"Stop");
	}
	else
	{
		KillTimer(100);
		m_ThdDisplay.Stop();
		m_StCamera.OnStopAcquisition();

		SetDlgItemText(IDC_BTN_ACQ, L"Start");
	}
}

void CFTech_StCamDlg::CreateBmpInfo(int nWidth, int nHeight, int nBpp)
{
	if (m_pBitmapInfo != NULL) 
	{
		delete []m_pBitmapInfo;
		m_pBitmapInfo = NULL;
	}

	if (nBpp == 8)
		m_pBitmapInfo = (BITMAPINFO *) new BYTE[sizeof(BITMAPINFO) + 255*sizeof(RGBQUAD)];
	else if (nBpp == 24)
		m_pBitmapInfo = (BITMAPINFO *) new BYTE[sizeof(BITMAPINFO)];

	m_pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_pBitmapInfo->bmiHeader.biPlanes = 1;
	m_pBitmapInfo->bmiHeader.biBitCount = nBpp;
	m_pBitmapInfo->bmiHeader.biCompression = BI_RGB;

	if (nBpp == 8)
		m_pBitmapInfo->bmiHeader.biSizeImage = 0;
	else if (nBpp == 24)
		m_pBitmapInfo->bmiHeader.biSizeImage = (((nWidth * 24 + 31) & ~31) >> 3) * nHeight;

	m_pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	m_pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	m_pBitmapInfo->bmiHeader.biClrUsed = 0;
	m_pBitmapInfo->bmiHeader.biClrImportant = 0;

	if (nBpp == 8)
	{
		for (int i = 0 ; i < 256 ; i++)
		{
			m_pBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbReserved = 0;
		}
	}

	m_pBitmapInfo->bmiHeader.biWidth = nWidth;
	m_pBitmapInfo->bmiHeader.biHeight = -nHeight;
}

void CFTech_StCamDlg::CallbackFunc()
{
	Sleep(30);

	memcpy(m_pBuffer, m_StCamera.GetImageBuffer(), m_nWidth*m_nHeight*m_nBpp/8);
	OnDisplay(m_pBuffer);
}

void CFTech_StCamDlg::OnDisplay(BYTE* pBuffer)
{
	CClientDC dc(GetDlgItem(IDC_PC_CAMERA));
	CRect rect;
	GetDlgItem(IDC_PC_CAMERA)->GetClientRect(&rect);

	dc.SetStretchBltMode(COLORONCOLOR); 
	StretchDIBits(dc.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), 0, 0, m_nWidth, m_nHeight, pBuffer, m_pBitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

void CFTech_StCamDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	KillTimer(100);

	if (m_ThdDisplay.IsDone() == false)
		m_ThdDisplay.Stop();

	if (m_StCamera.IsConnected() == true)
	{
		if (m_StCamera.IsActive() == true)
			m_StCamera.OnStopAcquisition();

		m_StCamera.OnDisconnect();
	}

	if (m_pBuffer != NULL)
	{
		delete []m_pBuffer;
		m_pBuffer = NULL;
	}

	if (m_pBitmapInfo != NULL)
	{
		delete []m_pBitmapInfo;
		m_pBitmapInfo = NULL;
	}
}


void CFTech_StCamDlg::OnBnClickedBtnSave()
{
	if (m_StCamera.IsConnected() == false) return;
	CString	strFilter = _T("All Files (*.*)|*.*||");

	CFileDialog FileDlg(FALSE, _T(".bmp"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter);

	if( FileDlg.DoModal() == IDOK )
	{
		m_StCamera.OnSaveImage(FileDlg.GetPathName());
	}
}


void CFTech_StCamDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 100)
	{
		double t = m_StCamera.GetGrabTime();
		CString tmp;
		tmp.Format(L"%.2f ms", t);
		SetDlgItemText(IDC_LB_TIME, tmp);
	}

	CDialogEx::OnTimer(nIDEvent);
}
