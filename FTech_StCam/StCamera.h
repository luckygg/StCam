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

	//----- 연결 및 해제 -----//
	bool OnConnect(); // Dialog 선택 연결.
	bool OnConnect(CString strUserID); // User ID로 연결.
	bool OnConnect(int nAddr1, int nAddr2, int nAddr3, int nAddr4); // IP Address로 연결.
	void OnDisconnect(); // 연결 해제.

	//----- 영상 취득 제어 -----//
	bool OnStartAcquisition(); // 영상 취득 시작.
	bool OnStopAcquisition();  // 영상 취득 중지.
	
	//----- 영상 취득 방식 선택 -----//
	bool SetContinuousMode();	// 프리런 모드 설정.
	bool SetSoftTriggerMode();	// 소프트웨어 트리거 설정.
	bool SetHardTriggerMode();  // 하드웨어 트리거 설정.
	bool OnTriggerEvent();		// 소프트웨어 트리거 이벤트.
	
	//----- 영상 저장-----//
	bool OnSaveImage(CString strPath);	// 이미지 저장.

	//----- 확인 및 반환 함수 -----//
	bool IsConnected() { return m_pvDevice.IsConnected(); }	// 연결 확인.
	bool IsActive() { return m_isAcquisition; };			// 영상 취득 확인.
	bool GetDeviceUserID(CString &strCamID);				// Device User ID 반환.
	bool GetDeviceModelName(CString &strCamID);				// Device Model Name 반환.
	int  GetWidth()  { return m_nWidth ; }					// Image Width 반환.
	int  GetHeight() { return m_nHeight; }					// Image Height 반환.
	int  GetBPP()	 { return m_nBpp;	 }					// Image Bit per Pixel 반환.
	double GetGrabTime() { return m_dTime; }				// Grab Tact Time 반환.
	BYTE* GetImageBuffer() { return m_pbyBuffer; }			// Buffer 반환.
	CString GetLastErrorMessage();							// 마지막 에러 메시지 반환.

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

