#pragma once

//----------------------------------------------------------
// CGraphEx Control
//----------------------------------------------------------
// Programmed by William Kim
//----------------------------------------------------------
// Last Update : 2016-10-14 13:32
// Modified by William Kim
//----------------------------------------------------------

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

	//----- 설정 저장 / 불러오기 -----//
	bool OnUserSetSave();
	bool OnUserSetLoad();

	//----- 확인 및 반환 함수 -----//
	bool IsConnected() { return m_pvDevice.IsConnected(); }	// 연결 상태 반환.
	bool IsActive() { return m_isAcquisition; };			// 영상 취득 상태 반환.
	bool GetDeviceUserID(CString &strValue);				// Device User ID 반환.
	bool GetDeviceModelName(CString &strValue);				// Device Model Name 반환.
	bool GetSerialNumber(CString &strValue);				// Serial Number 반환.
	bool GetOffsetX(int &nValue);							// Offset X 반환.
	bool GetOffsetY(int &nValue);							// Offset Y 반환.
	bool GetAcquisitionMode(CString &strValue);				// Acquisition Mode 반환.
	bool GetAcquisitionFrameRate(double &dValue);			// Frame Rate 반환.
	bool GetTriggerMode(CString &strValue);					// Trigger Mode 반환.
	bool GetTriggerSource(CString &strValue);				// Trigger Source 반환.
	bool GetTriggerOverlap(CString &strValue);				// Trigger Overlap 반환.
	bool GetExposureMode(CString &strValue);				// Exposure Mode 반환.
	bool GetExposureTimeRaw(int &nValue);					// Exposure Time 반환.
	bool GetPixelFormat(CString &strValue);					// Pixel Format 반환.
	bool GetUserSetDefaultSelector(CString &strValue);		// UserSet 기본 값 반환.
	CString GetIPAddress() { return m_strIP; }				// IP Address 반환.
	CString GetMACAddress() { return m_strMAC; }			// MAC Address 반환.
	int  GetWidth()  { return m_nWidth ; }					// Image Width 반환.
	int  GetHeight() { return m_nHeight; }					// Image Height 반환.
	int  GetBPP()	 { return m_nBpp;	 }					// Image Bit per Pixel 반환.
	double GetGrabTime() { return m_dTime; }				// Grab Tact Time 반환.
	BYTE* GetImageBuffer() { return m_pbyBuffer; }			// Buffer 반환.
	CString GetLastErrorMessage();							// 마지막 에러 메시지 반환.

	//----- 설정 함수 -----//
	bool SetDeviceUserID(CString strValue);					// Device User ID 설정.
	bool SetOffsetX(int nValue);							// Offset X 설정.
	bool SetOffsetY(int nValue);							// Offset Y 설정.
	bool SetAcquisitionFrameRate(double dValue);			// Frame Rate 설정.
	bool SetAcquisitionMode(CString strValue);				// Acquistiion Mode 설정.
	bool SetTriggerMode(CString strValue);					// Trigger Mode 설정.
	bool SetTriggerSource(CString strValue);				// Trigger Source 설정.
	bool SetTriggerOverlap(CString strValue);				// Trigger Overlap 설정.
	bool SetExposureMode(CString strValue);					// Exposure Mode 설정.
	bool SetExposureTime(double dValue);					// Exposure Time 설정.
	bool SetUserSetSelector(CString strValue);				// UserSetSelector 설정.

protected:
	LARGE_INTEGER m_Start;
	LARGE_INTEGER m_Stop;
	LARGE_INTEGER m_Freq;
	double		  m_dTime;

private :
	HANDLE m_hGrabDone;
	HANDLE m_hThTerminate;

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
	CString m_strIP;
	CString m_strMAC;

	static UINT BufferThread(LPVOID param);
	void ShowErrorMessage(char* pcMessage, PvUInt32 nError);
	void OnCreateBmpInfo(int nWidth, int nHeight, int nBpp);
};

