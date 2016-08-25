#pragma once

#include <PvDeviceFinderWnd.h>
#include <PvDevice.h>
#include <PvGenParameter.h>
#include <PvGenBrowserWnd.h>
#include <PvBuffer.h>
#include <PvStream.h>
#include <PvPipeline.h>
#include <PvDisplayWnd.h>
#include <PvBufferWriter.h>
#include <PvSystem.h>
#include <PvDeviceInfo.h>
#include <PvDevice.h>
#include <PvAcquisitionStateManager.h>

class CStCamera
{
public:
	CStCamera(void);
	~CStCamera(void);

	HANDLE m_hGrabDone;
	HANDLE m_hThTerminate;

	//----- ���� �� ���� -----//
	bool OnConnect(); // Dialog ���� ����.
	bool OnConnect(CString strUserID); // User ID�� ����.
	bool OnConnect(int nAddr1, int nAddr2, int nAddr3, int nAddr4); // IP Address�� ����.
	void OnDisconnect(); // ���� ����.

	//----- ���� ��� ���� -----//
	bool OnStartAcquisition(); // ���� ��� ����.
	bool OnStopAcquisition();  // ���� ��� ����.
	
	//----- ���� ��� ��� ���� -----//
	bool SetContinuousMode();	// ������ ��� ����.
	bool SetSoftTriggerMode();	// ����Ʈ���� Ʈ���� ����.
	bool SetHardTriggerMode();  // �ϵ���� Ʈ���� ����.
	bool OnTriggerEvent();		// ����Ʈ���� Ʈ���� �̺�Ʈ.
	
	//----- ���� ����-----//
	bool OnSaveImage(CString strPath);	// �̹��� ����.

	//----- Ȯ�� �� ��ȯ �Լ� -----//
	bool IsConnected() { return m_pvDevice.IsConnected(); }	// ���� Ȯ��.
	bool IsActive() { return m_isAcquisition; };			// ���� ��� Ȯ��.
	bool GetDeviceUserID(CString &strCamID);				// Device User ID ��ȯ.
	bool GetDeviceModelName(CString &strCamID);				// Device Model Name ��ȯ.
	int  GetWidth()  { return m_nWidth ; }					// Image Width ��ȯ.
	int  GetHeight() { return m_nHeight; }					// Image Height ��ȯ.
	int  GetBPP()	 { return m_nBpp;	 }					// Image Bit per Pixel ��ȯ.
	double GetGrabTime() { return m_dTime; }				// Grab Tact Time ��ȯ.
	BYTE* GetImageBuffer() { return m_pbyBuffer; }			// Buffer ��ȯ.
	CString GetLastErrorMessage();							// ������ ���� �޽��� ��ȯ.

protected:
	LARGE_INTEGER m_Start;
	LARGE_INTEGER m_Stop;
	LARGE_INTEGER m_Freq;
	double		  m_dTime;

private :
	BYTE*			m_pbyBuffer;
	BITMAPINFO*		m_pBitmapInfo;
	PvAcquisitionStateManager *m_ppvAcqManager;
	PvSystem		m_pvSystem;
	PvStream		m_pvStream;
	PvPipeline		m_pvPipeline;
	PvDevice		m_pvDevice;
	
	CString m_strErrorMsg;
	bool m_isAcquisition;
	int	m_nWidth;
	int	m_nHeight;
	int	m_nBpp;

	static UINT BufferThread(LPVOID param);
	void ShowErrorMessage(char* pcMessage, PvUInt32 nError);
	void OnCreateBmpInfo(int nWidth, int nHeight, int nBpp);
};

