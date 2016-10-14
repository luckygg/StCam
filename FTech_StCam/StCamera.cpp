#include "StdAfx.h"
#include "StCamera.h"


CStCamera::CStCamera(void)
	: m_pvPipeline(&m_pvStream)
	, m_ppvAcqManager(NULL)
{
	m_pbyBuffer		= NULL;
	m_pBitmapInfo	= NULL;
	
	m_isAcquisition	= false;
	m_strErrorMsg	= L"";
	m_nWidth	= 0;
	m_nHeight	= 0;
	m_nBpp		= 0;
	m_dTime		= 0;
	m_strIP		= L"";
	m_strMAC	= L"";

	m_hGrabDone = CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hThTerminate = CreateEvent(NULL,TRUE,FALSE,NULL);
}


CStCamera::~CStCamera(void)
{
	if (m_isAcquisition == true)
	{
		m_isAcquisition = false;
		WaitForSingleObject(m_hThTerminate,2000);
	}

	if (m_pbyBuffer != NULL)
	{
		delete []m_pbyBuffer;
		m_pbyBuffer = NULL;
	}

	if (m_pBitmapInfo != NULL)
	{
		delete m_pBitmapInfo;
		m_pBitmapInfo = NULL;
	}

	OnStopAcquisition();

	OnDisconnect();
}

bool CStCamera::OnConnect()
{
	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvDeviceFinderWnd DeviceFinderWnd;
	if ( !DeviceFinderWnd.ShowModal().IsOK() )
		return false;
	
	PvDeviceInfo* ppvDeviceInfo = DeviceFinderWnd.GetSelected();
	if( ppvDeviceInfo == NULL )
		return false;
	
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	
	StResult = m_pvDevice.Connect( ppvDeviceInfo, PvAccessControl );
	if ( !StResult.IsOK() )
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		OnDisconnect();
		return false;
	}

#ifdef _DEBUG
	m_pvDevice.GetGenParameters()->SetIntegerValue( "GevHeartbeatTimeout", 60000 );
#endif
	StResult = m_pvDevice.NegotiatePacketSize( 0, 1440 );
	if ( !StResult.IsOK() )
	{
		::Sleep( 2500 );
	}

	for ( ;; )
	{
		StResult = m_pvStream.Open( ppvDeviceInfo->GetIPAddress() );
		if ( StResult.IsOK() )
			break;
		
		::Sleep( 1000 );
	}

	m_pvDevice.SetStreamDestination( m_pvStream.GetLocalIPAddress(), m_pvStream.GetLocalPort() );
	if ( !StResult.IsOK() )
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		OnDisconnect();
		return false;
	}

	ASSERT( m_ppvAcqManager == NULL );
	m_ppvAcqManager = new PvAcquisitionStateManager( &m_pvDevice, &m_pvStream );

	m_strIP = (CString)ppvDeviceInfo->GetIPAddress().GetUnicode();
	m_strMAC = (CString)ppvDeviceInfo->GetMACAddress().GetUnicode();

	PvInt64 Value=0;
	m_pvDevice.GetGenParameters()->GetInteger( "Width" )->GetValue(Value);
	m_nWidth = (int)Value;

	m_pvDevice.GetGenParameters()->GetInteger( "Height" )->GetValue(Value);
	m_nHeight = (int)Value;

	PvString Format;
	m_pvDevice.GetGenParameters()->GetEnumValue("PixelFormat",Format);
	CString FormatStr = Format.GetUnicode();
	if (FormatStr.Find(L"Mono",0) >= 0)
	{
		m_pvDevice.GetGenParameters()->SetEnumValue("PixelFormat", "Mono8");
		m_nBpp = 8;
		OnCreateBmpInfo(m_nWidth, m_nHeight, m_nBpp);
	}
	else if (FormatStr.Find(L"Bayer",0) >= 0)
	{
		FormatStr = FormatStr.Left(7);
		FormatStr += L"8";

		Format = (PvString)FormatStr;
		m_pvDevice.GetGenParameters()->SetEnumValue("PixelFormat", Format);
		m_nBpp = 24;
		OnCreateBmpInfo(m_nWidth, m_nHeight, m_nBpp);
	}

	m_pvDevice.GetGenParameters()->GetInteger( "PayloadSize" )->GetValue(Value);
	m_pbyBuffer = new BYTE[(int)Value*m_nBpp/8];
	memset(m_pbyBuffer, 0, (int)Value*m_nBpp/8);

	::SetCursor(NULL);

	return true;
}

bool CStCamera::OnConnect(CString strUserID)
{
	PvDeviceInfo* ppvDeviceInfo = NULL;
	PvResult StResult = PvResult::Code::NOT_CONNECTED;
	bool Break = FALSE;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	m_pvSystem.SetDetectionTimeout( 1000 );
	StResult = m_pvSystem.Find();
	if( StResult.IsOK() )
	{
		PvUInt32 lInterfaceCount = m_pvSystem.GetInterfaceCount();

		for( PvUInt32 x = 0; x < lInterfaceCount; x++)
		{
			if(Break) break;
			PvInterface *pInterface = m_pvSystem.GetInterface(x);
			PvUInt32 DeviceCount = pInterface->GetDeviceCount();
			for( PvUInt32 y = 0; y < DeviceCount; y++)
			{
				ppvDeviceInfo = pInterface->GetDeviceInfo(y);
				CString ID;
				
				ID = ppvDeviceInfo->GetUserDefinedName().GetUnicode();
				if(strUserID.Compare(ID) == 0) 
				{
					Break = TRUE;
					break;
				}
				Break = FALSE;
			}
		}
	}
	else
	{
		return false;
	}

	if (Break == false) return false;

	StResult = m_pvDevice.Connect( ppvDeviceInfo, PvAccessControl );
	if(!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

#ifdef _DEBUG
	m_pvDevice.GetGenParameters()->SetIntegerValue( "GevHeartbeatTimeout", 60000 );
#endif
	StResult = m_pvDevice.NegotiatePacketSize( 0, 1476 );
	if ( !StResult.IsOK() )
	{
		::Sleep( 2500 );
	}

	StResult = m_pvStream.Open( ppvDeviceInfo->GetIPAddress() );
	if ( !StResult.IsOK() )
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	m_pvDevice.SetStreamDestination( m_pvStream.GetLocalIPAddress(), m_pvStream.GetLocalPort() );

	m_pvStream.GetParameters()->SetIntegerValue("RequestTimeout", 1000 );

	ASSERT( m_ppvAcqManager == NULL );
	m_ppvAcqManager = new PvAcquisitionStateManager( &m_pvDevice, &m_pvStream );
	
	m_strIP = (CString)ppvDeviceInfo->GetIPAddress().GetUnicode();
	m_strMAC = (CString)ppvDeviceInfo->GetMACAddress().GetUnicode();

	PvInt64 Value=0;
	m_pvDevice.GetGenParameters()->GetInteger( "Width" )->GetValue(Value);
	m_nWidth = (int)Value;

	m_pvDevice.GetGenParameters()->GetInteger( "Height" )->GetValue(Value);
	m_nHeight = (int)Value;

	PvString Format;
	m_pvDevice.GetGenParameters()->GetEnumValue("PixelFormat",Format);
	CString FormatStr = Format.GetUnicode();
	if (FormatStr.Find(L"Mono",0) >= 0)
	{
		m_pvDevice.GetGenParameters()->SetEnumValue("PixelFormat", "Mono8");
		m_nBpp = 8;
		OnCreateBmpInfo(m_nWidth, m_nHeight, m_nBpp);
	}
	else if (FormatStr.Find(L"Bayer",0) >= 0)
	{
		FormatStr = FormatStr.Left(7);
		FormatStr += L"8";

		Format = (PvString)FormatStr;
		m_pvDevice.GetGenParameters()->SetEnumValue("PixelFormat", Format);
		m_nBpp = 24;
		OnCreateBmpInfo(m_nWidth, m_nHeight, m_nBpp);
	}

	m_pvDevice.GetGenParameters()->GetInteger( "PayloadSize" )->GetValue(Value);
	m_pbyBuffer = new BYTE[(int)Value*m_nBpp/8];
	memset(m_pbyBuffer, 0, (int)Value*m_nBpp/8);

	SetCursor(NULL);

	return true;
}

bool CStCamera::OnConnect(int nAddr1, int nAddr2, int nAddr3, int nAddr4)
{
	PvDeviceInfo* ppvDeviceInfo = NULL;
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	CString IP;
	IP.Format(L"%d.%d.%d.%d", nAddr1,nAddr2,nAddr3,nAddr4);

	PvResult StResult = PvResult::Code::NOT_CONNECTED;
	bool Break = FALSE;

	m_pvSystem.SetDetectionTimeout( 1000 );
	StResult = m_pvSystem.Find();
	if( StResult.IsOK() )
	{
		PvUInt32 lInterfaceCount = m_pvSystem.GetInterfaceCount();

		for( PvUInt32 x = 0; x < lInterfaceCount; x++)
		{
			if(Break) break;
			PvInterface *pInterface = m_pvSystem.GetInterface(x);
			PvUInt32 DeviceCount = pInterface->GetDeviceCount();
			for( PvUInt32 y = 0; y < DeviceCount; y++)
			{
				ppvDeviceInfo = pInterface->GetDeviceInfo(y);
				CString ID=L"";

				ID = ppvDeviceInfo->GetIPAddress().GetUnicode();
				if(IP.Compare(ID) == 0) 
				{
					Break = TRUE;
					break;
				}
				Break = FALSE;
			}
		}
	}
	else
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	if (Break == false) return false;

	StResult = m_pvDevice.Connect( ppvDeviceInfo, PvAccessControl );
	if(!StResult.IsOK())
	{
		ShowErrorMessage("Connect() - PvDevice::Connect()", StResult.GetCode());
		return false;
	}

#ifdef _DEBUG
	m_pvDevice.GetGenParameters()->SetIntegerValue( "GevHeartbeatTimeout", 60000 );
#endif
	StResult = m_pvDevice.NegotiatePacketSize( 0, 1476 );
	if ( !StResult.IsOK() )
	{
		::Sleep( 2500 );
	}

	StResult = m_pvStream.Open( ppvDeviceInfo->GetIPAddress() );
	if ( !StResult.IsOK() )
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	m_pvDevice.SetStreamDestination( m_pvStream.GetLocalIPAddress(), m_pvStream.GetLocalPort() );

	m_pvStream.GetParameters()->SetIntegerValue("RequestTimeout", 1000 );

	ASSERT( m_ppvAcqManager == NULL );
	m_ppvAcqManager = new PvAcquisitionStateManager( &m_pvDevice, &m_pvStream );

	m_strIP = (CString)ppvDeviceInfo->GetIPAddress().GetUnicode();
	m_strMAC = (CString)ppvDeviceInfo->GetMACAddress().GetUnicode();

	PvInt64 Value=0;
	m_pvDevice.GetGenParameters()->GetInteger( "Width" )->GetValue(Value);
	m_nWidth = (int)Value;

	m_pvDevice.GetGenParameters()->GetInteger( "Height" )->GetValue(Value);
	m_nHeight = (int)Value;

	PvString Format;
	m_pvDevice.GetGenParameters()->GetEnumValue("PixelFormat",Format);
	CString FormatStr = Format.GetUnicode();
	if (FormatStr.Find(L"Mono",0) >= 0)
	{
		m_pvDevice.GetGenParameters()->SetEnumValue("PixelFormat", "Mono8");
		m_nBpp = 8;
		OnCreateBmpInfo(m_nWidth, m_nHeight, m_nBpp);
	}
	else if (FormatStr.Find(L"Bayer",0) >= 0)
	{
		FormatStr = FormatStr.Left(7);
		FormatStr += L"8";

		Format = (PvString)FormatStr;
		m_pvDevice.GetGenParameters()->SetEnumValue("PixelFormat", Format);
		m_nBpp = 24;
		OnCreateBmpInfo(m_nWidth, m_nHeight, m_nBpp);
	}

	m_pvDevice.GetGenParameters()->GetInteger( "PayloadSize" )->GetValue(Value);
	m_pbyBuffer = new BYTE[(int)Value*m_nBpp/8];
	memset(m_pbyBuffer, 0, (int)Value*m_nBpp/8);

	SetCursor(NULL);

	return true;
}

void CStCamera::OnDisconnect()
{
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if (m_isAcquisition == true)
	{
		m_isAcquisition = false;
		WaitForSingleObject(m_hThTerminate,2000);
	}

	if(m_pvPipeline.IsStarted())
		m_pvPipeline.Stop();

	if(m_pvStream.IsOpen())
		m_pvStream.Close();

	if(m_pvDevice.IsConnected())
		m_pvDevice.Disconnect();

	if (m_pbyBuffer != NULL)
	{
		delete []m_pbyBuffer;
		m_pbyBuffer = NULL;
	}

	if (m_pBitmapInfo != NULL)
	{
		delete m_pBitmapInfo;
		m_pBitmapInfo = NULL;
	}

	if ( m_ppvAcqManager != NULL )
	{
		delete m_ppvAcqManager;
		m_ppvAcqManager = NULL;
	}

	SetCursor(NULL);
}

bool CStCamera::OnStartAcquisition()
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	if (m_isAcquisition == true)
		OnStopAcquisition();
	
	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	m_isAcquisition = true;

	AfxBeginThread(BufferThread, this);

	StResult = m_pvPipeline.Start();
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	PvGenInteger *lPayloadSize = m_pvDevice.GetGenParameters()->GetInteger( "PayloadSize" );

	PvInt64 lPayloadSizeValue = 0;
	if ( lPayloadSize != NULL )
		lPayloadSize->GetValue( lPayloadSizeValue );

	if ( lPayloadSizeValue > 0 )
	{
		m_pvPipeline.SetBufferSize( static_cast<PvUInt32>( lPayloadSizeValue ) );
	}

	StResult = m_pvPipeline.Reset();
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	StResult = m_pvStream.GetParameters()->ExecuteCommand( "Reset" );
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	ASSERT( m_ppvAcqManager != NULL );
	StResult = m_ppvAcqManager->Start();
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::OnStopAcquisition()
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	m_isAcquisition = false;

	if ( m_pvPipeline.IsStarted() )
	{
		StResult = m_pvPipeline.Stop();
		if (!StResult.IsOK())
		{
			m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
			return false;
		}
	}

	ASSERT( m_ppvAcqManager != NULL );
	StResult = m_ppvAcqManager->Stop();
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

void CStCamera::ShowErrorMessage(char* pcMessage, PvUInt32 nError)
{
	int Len = (int)strlen(pcMessage);
	char* Tmp = new char[128];
	strcpy_s(Tmp,Len+1,pcMessage);

	if		(nError == PvResult::Code::ABORTED				) strcat_s(Tmp,128," - The operation was aborted."); 
	else if (nError == PvResult::Code::AUTO_ABORTED			) strcat_s(Tmp,128," - Buffer reception failed."); 
	else if (nError == PvResult::Code::BAD_VERSION			) strcat_s(Tmp,128," - Some component versions are not compatible."); 
	else if (nError == PvResult::Code::BUFFER_TOO_SMALL 	) strcat_s(Tmp,128," - The buffer was not large enough to hold the payload of the block being received."); 
	else if (nError == PvResult::Code::CANCEL 				) strcat_s(Tmp,128," - The user closed a dialog and the operation was not performed."); 
	else if (nError == PvResult::Code::CANNOT_OPEN_FILE 	) strcat_s(Tmp,128," - The file doesn't exist or can't be opened."); 
	else if (nError == PvResult::Code::ERR_OVERFLOW 		) strcat_s(Tmp,128," - Overflow occurred."); 
	else if (nError == PvResult::Code::FILE_ERROR 			) strcat_s(Tmp,128," - A file operation error occured."); 
	else if (nError == PvResult::Code::GENERIC_ERROR 		) strcat_s(Tmp,128," - An undefined error occurred."); 
	else if (nError == PvResult::Code::GENICAM_XML_ERROR 	) strcat_s(Tmp,128," - The GenICam XML file could not be loaded into GenApi."); 
	else if (nError == PvResult::Code::IMAGE_ERROR 			) strcat_s(Tmp,128," - Error with an image."); 
	else if (nError == PvResult::Code::INVALID_DATA_FORMAT 	) strcat_s(Tmp,128," - The data format is not supported for the requested operation."); 
	else if (nError == PvResult::Code::INVALID_PARAMETER 	) strcat_s(Tmp,128," - A parameter passed to the method is invalid."); 
	else if (nError == PvResult::Code::MISSING_PACKETS 		) strcat_s(Tmp,128," - Some packets are missing in the buffer."); 
	else if (nError == PvResult::Code::NETWORK_ERROR 		) strcat_s(Tmp,128," - A network error occurred while performing the requested operation."); 
	else if (nError == PvResult::Code::NO_AVAILABLE_DATA 	) strcat_s(Tmp,128," - There is no available data to enumerate."); 
	else if (nError == PvResult::Code::NO_LICENSE 			) strcat_s(Tmp,128," - An eBUS SDK license is missing."); 
	else if (nError == PvResult::Code::NO_MORE_ENTRY 		) strcat_s(Tmp,128," - There are no more entries to retrieve/enumerate."); 
	else if (nError == PvResult::Code::NO_MORE_ITEM 		) strcat_s(Tmp,128," - No more of what was requested is currently available."); 
	else if (nError == PvResult::Code::NOT_CONNECTED 		) strcat_s(Tmp,128," - The object (PvDevice or PvStream object) isn't connected."); 
	else if (nError == PvResult::Code::NOT_ENOUGH_MEMORY 	) strcat_s(Tmp,128," - Not enough memory."); 
	else if (nError == PvResult::Code::NOT_FOUND 			) strcat_s(Tmp,128," - The expected item wasn't found."); 
	else if (nError == PvResult::Code::NOT_IMPLEMENTED 		) strcat_s(Tmp,128," - The requested feature or functionality is not implemented."); 
	else if (nError == PvResult::Code::NOT_INITIALIZED 		) strcat_s(Tmp,128," - An error code hasn't been set."); 
	else if (nError == PvResult::Code::NOT_SUPPORTED 		) strcat_s(Tmp,128," - The requested feature or functionality is not supported."); 
	else if (nError == PvResult::Code::RESENDS_FAILURE 		) strcat_s(Tmp,128," - Failure to receive all missing packets for a buffer through resend packets."); 
	else if (nError == PvResult::Code::STATE_ERROR 			) strcat_s(Tmp,128," - The method is (probably) legal, but the system's current state doesn't allow the action."); 
	else if (nError == PvResult::Code::THREAD_ERROR 		) strcat_s(Tmp,128," - An error occurred while attempting to perform an operation on a thread like starting, stopping or changing priority."); 
	else if (nError == PvResult::Code::TIMEOUT 				) strcat_s(Tmp,128," - The operation timed out."); 
	else if (nError == PvResult::Code::TOO_MANY_RESENDS 	) strcat_s(Tmp,128," - Too many resend packets were requested, buffer acquisition failure."); 
	else if (nError == PvResult::Code::TOO_MANY_CONSECUTIVE_RESENDS	) strcat_s(Tmp,128," - Buffer reception failed, consecutive missing buffers higher than allowed."); 

	MessageBoxA(NULL, Tmp, "Error", MB_OK | MB_ICONERROR);

	delete Tmp;
}


void CStCamera::OnCreateBmpInfo(int nWidth, int nHeight, int nBpp)
{
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

bool CStCamera::OnSaveImage(CString strPath)
{
	if (strPath.IsEmpty()) return false;

	if (m_pbyBuffer == NULL) return false;

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	BITMAPFILEHEADER dib_format_layout;
	ZeroMemory(&dib_format_layout, sizeof(BITMAPFILEHEADER));
	dib_format_layout.bfType = 0x4D42;//*(WORD*)"BM";
	if (m_nBpp == 8)
		dib_format_layout.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(m_pBitmapInfo->bmiColors) * 256;
	else
		dib_format_layout.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	
	dib_format_layout.bfSize = dib_format_layout.bfOffBits + m_pBitmapInfo->bmiHeader.biSizeImage;

	wchar_t* wchar_str;     
	char*    char_str;      
	int      char_str_len;  
	wchar_str = strPath.GetBuffer(strPath.GetLength());

	char_str_len = WideCharToMultiByte(CP_ACP, 0, wchar_str, -1, NULL, 0, NULL, NULL);
	char_str = new char[char_str_len];
	WideCharToMultiByte(CP_ACP, 0, wchar_str, -1, char_str, char_str_len, 0,0);  

	FILE *p_file;
	fopen_s(&p_file,char_str, "wb");
	if(p_file != NULL)
	{
		fwrite(&dib_format_layout, 1, sizeof(BITMAPFILEHEADER), p_file);
		fwrite(m_pBitmapInfo, 1, sizeof(BITMAPINFOHEADER), p_file);
		if (m_nBpp == 8)
			fwrite(m_pBitmapInfo->bmiColors, sizeof(m_pBitmapInfo->bmiColors), 256, p_file);

		fwrite(m_pbyBuffer, 1, m_nWidth * m_nHeight * m_nBpp/8, p_file);
		fclose(p_file);
	}

	delete char_str;

	return true;
}

bool CStCamera::GetDeviceUserID(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetString("DeviceUserID", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetDeviceModelName(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetString("DeviceModelName", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetSerialNumber(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetString("DeviceID", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetOffsetX(int &nValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvInt64 value;
	StResult = m_pvDevice.GetGenParameters()->GetIntegerValue("OffsetX", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	nValue = (int)value;

	return true;
}

bool CStCamera::GetOffsetY(int &nValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvInt64 value;
	StResult = m_pvDevice.GetGenParameters()->GetIntegerValue("OffsetY", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	nValue = (int)value;

	return true;
}

bool CStCamera::GetAcquisitionMode(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetEnumValue("AcquisitionMode", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetAcquisitionFrameRate(double &dValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	double value;
	StResult = m_pvDevice.GetGenParameters()->GetFloatValue("AcquisitionFrameRate", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	dValue = value;

	return true;
}

bool CStCamera::GetTriggerMode(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetEnumValue("TriggerMode", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetTriggerSource(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetEnumValue("TriggerSource", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetTriggerOverlap(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetEnumValue("TriggerOverlap", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetExposureMode(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetEnumValue("ExposureMode", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetExposureTimeRaw(int &nValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvInt64 value;
	StResult = m_pvDevice.GetGenParameters()->GetIntegerValue("ExposureTimeRaw", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	nValue = (int)value;

	return true;
}

bool CStCamera::GetPixelFormat(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetEnumValue("PixelFormat", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::GetUserSetDefaultSelector(CString &strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value;
	StResult = m_pvDevice.GetGenParameters()->GetEnumValue("UserSetDefaultSelector", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	strValue = value.GetUnicode();

	return true;
}

bool CStCamera::SetDeviceUserID(CString strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value(strValue);
	StResult = m_pvDevice.GetGenParameters()->SetString("DeviceUserID", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetOffsetX(int nValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	StResult = m_pvDevice.GetGenParameters()->SetIntegerValue("OffsetX", nValue);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetOffsetY(int nValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	StResult = m_pvDevice.GetGenParameters()->SetIntegerValue("OffsetY", nValue);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetAcquisitionFrameRate(double dValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	StResult = m_pvDevice.GetGenParameters()->SetFloatValue("AcquisitionFrameRate", dValue);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetAcquisitionMode(CString strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value(strValue);
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("AcquisitionMode", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetTriggerMode(CString strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value(strValue);
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerMode", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetTriggerSource(CString strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value(strValue);
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerSource", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetTriggerOverlap(CString strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value(strValue);
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerOverlap", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetExposureMode(CString strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value(strValue);
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("ExposureMode", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetUserSetSelector(CString strValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString value(strValue);
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("UserSetSelector", value);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetExposureTime(double dValue)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	StResult = m_pvDevice.GetGenParameters()->SetFloatValue("ExposureTime", dValue);
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetContinuousMode()
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString Name;
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerMode", "Off");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("ExposureMode", "Timed");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetSoftTriggerMode()
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString Name;
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerMode", "On");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerSource", "Software");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerOverlap", "ReadOut");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerSoftwareSource", "Command");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::SetHardTriggerMode()
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString Name;
	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerMode", "On");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerOverlap", "ReadOut");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	StResult = m_pvDevice.GetGenParameters()->SetEnumValue("TriggerSource", "Hardware");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::OnTriggerEvent()
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString Name;
	StResult = m_pvDevice.GetGenParameters()->ExecuteCommand("TriggerSoftware");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::OnUserSetLoad()
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString Name;
	StResult = m_pvDevice.GetGenParameters()->ExecuteCommand("UserSetLoad");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

bool CStCamera::OnAutoWhiteBalance(AWB Type)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString Name;
	switch (Type)
	{
	case AWB_Off :
		StResult = m_pvDevice.GetGenParameters()->SetEnumValue("BalanceWhiteAuto", "Off");
		break;
	case AWB_Preset1 :
		StResult = m_pvDevice.GetGenParameters()->SetEnumValue("BalanceWhiteAuto", "Preset1");
		break;
	case AWB_Preset2 :
		StResult = m_pvDevice.GetGenParameters()->SetEnumValue("BalanceWhiteAuto", "Preset2");
		break;
	case AWB_Preset3 :
		StResult = m_pvDevice.GetGenParameters()->SetEnumValue("BalanceWhiteAuto", "Preset3");
		break;
	case AWB_Continuous :
		StResult = m_pvDevice.GetGenParameters()->SetEnumValue("BalanceWhiteAuto", "Continuous");
		break;
	case AWB_Once :
		StResult = m_pvDevice.GetGenParameters()->SetEnumValue("BalanceWhiteAuto", "Once");
		break;
	}

	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}
bool CStCamera::OnSaveAWBValueOnceToPreset(int nPresetNum)
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	if (nPresetNum < 1 || nPresetNum > 3)
	{
		m_strErrorMsg = L"Out of range of AWB Preset number.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;
	CString strPreset=L"";
	strPreset.Format(L"Preset%d",nPresetNum);

	CString Parameter=L"";
	PvString Name="";
	PvInt64 value=0;

	for (int i=0; i<4; i++)
	{
		switch (i)
		{
			case 0 : Parameter = L"BalanceRatio_R_"; break;
			case 1 : Parameter = L"BalanceRatio_Gr_"; break;
			case 2 : Parameter = L"BalanceRatio_B_"; break;
			case 3 : Parameter = L"BalanceRatio_Gb_"; break;
		}
		
		Name = PvString(Parameter+L"Once");
		CString Debug = Name.GetUnicode();
		StResult = m_pvDevice.GetGenParameters()->GetIntegerValue(Name, value);
		if (!StResult.IsOK())
		{
			m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
			return false;
		}
		Name = PvString(Parameter+strPreset);
		Debug = Name.GetUnicode();
		StResult = m_pvDevice.GetGenParameters()->SetIntegerValue(Name, value);
		if (!StResult.IsOK())
		{
			m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
			return false;
		}
	}
	
	return true;
}

bool CStCamera::OnUserSetSave()
{
	if (!m_pvDevice.IsConnected())
	{
		m_strErrorMsg = L"Device is not connected.";
		return false;
	}

	PvResult StResult = PvResult::Code::NOT_CONNECTED;

	PvString Name;
	StResult = m_pvDevice.GetGenParameters()->ExecuteCommand("UserSetSave");
	if (!StResult.IsOK())
	{
		m_strErrorMsg = (CString)StResult.GetCodeString().GetUnicode();
		return false;
	}

	return true;
}

UINT CStCamera::BufferThread(LPVOID param)
{
	CStCamera* pThis = reinterpret_cast<CStCamera*>(param);

	PvResult StResult;

	while(pThis->m_isAcquisition)
	{
		PvBuffer *lBuffer = NULL;
		PvResult  OperationResult;
		ResetEvent(pThis->m_hGrabDone);

		QueryPerformanceFrequency(&pThis->m_Freq);
		QueryPerformanceCounter(&pThis->m_Start);
		StResult = pThis->m_pvPipeline.RetrieveNextBuffer( & lBuffer, 0xFFFFFFFF, &OperationResult );
		if ( StResult.IsOK() )
		{ 
			if (OperationResult.IsOK())
			{
				if (pThis->m_nBpp == 8)
				{
					int size = lBuffer->GetImage()->GetImageSize();
					memcpy(pThis->m_pbyBuffer,(BYTE*)lBuffer->GetImage()->GetDataPointer(), lBuffer->GetImage()->GetImageSize());
					SetEvent(pThis->m_hGrabDone);
					QueryPerformanceCounter(&pThis->m_Stop);
					
					pThis->m_dTime = (double)(pThis->m_Stop.QuadPart - pThis->m_Start.QuadPart)/pThis->m_Freq.QuadPart*1000; 
				}
				else
				{
					PvPixelType lPixelTypeID = lBuffer->GetImage()->GetPixelType();
					PvBuffer outBuffer;
					PvPixelType aDestination = PvPixelWinRGB24;
					PvBufferConverter lBufferConverter;
					lBufferConverter.SetBayerFilter( PvBayerFilter3X3 );
					bool bConversionSupported = lBufferConverter.IsConversionSupported(lPixelTypeID,aDestination);
					if( bConversionSupported )
					{
						BYTE* pbyteBuffer = new BYTE[pThis->m_nWidth * pThis->m_nHeight * pThis->m_nBpp/8];
						outBuffer.GetImage()->Attach( pbyteBuffer, pThis->m_nWidth, pThis->m_nHeight, aDestination );
						PvResult lResult = lBufferConverter.Convert( lBuffer, &outBuffer, FALSE );
						if( lResult.IsOK() )
						{
							memcpy(pThis->m_pbyBuffer,(BYTE*)outBuffer.GetImage()->GetDataPointer(), outBuffer.GetImage()->GetImageSize());
							
							SetEvent(pThis->m_hGrabDone);
							QueryPerformanceCounter(&pThis->m_Stop);

							pThis->m_dTime = (double)(pThis->m_Stop.QuadPart - pThis->m_Start.QuadPart)/pThis->m_Freq.QuadPart*1000; 
						}
						outBuffer.GetImage()->Detach();
						delete [] pbyteBuffer;
						pbyteBuffer = NULL;
					}
				}
			}
			pThis->m_pvPipeline.ReleaseBuffer( lBuffer );
		}
	}

	SetEvent(pThis->m_hThTerminate);
	return 0;
}