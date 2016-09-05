// *****************************************************************************
// CCallbackThread.cpp
// *****************************************************************************

#include "stdafx.h"
#include "CallbackThread.h"

// ==========================================================================
CCallbackThread::CCallbackThread()
	: m_Handle( 0 )
	, m_ID( 0 )
	, m_bStop( false )
	, m_dwReturnValue( 0 )
{
	m_pListener = NULL;
}

// ==========================================================================
CCallbackThread::~CCallbackThread()
{
	UnregisterCallback();

	if ( m_Handle != INVALID_HANDLE_VALUE )
		Stop();
}

// ==========================================================================
void CCallbackThread::Start()
{
	m_bStop = false;

	m_Handle = ::CreateThread(
		NULL,				// Security attributes
		0,					// Stack size, 0 is default
		Link,				// Start address
		this,				// Parameter
		0,					// Creation flags
		&m_ID );			// Thread ID
}

// ==========================================================================
void CCallbackThread::Stop()
{
	ASSERT( m_Handle != INVALID_HANDLE_VALUE );

	m_bStop = true;
	DWORD lRetVal = ::WaitForSingleObject( m_Handle, INFINITE );
	ASSERT( lRetVal != WAIT_TIMEOUT  );

	::CloseHandle( m_Handle );
	m_Handle = INVALID_HANDLE_VALUE;

	m_ID = 0;
}

// ==========================================================================
void CCallbackThread::SetPriority( int aPriority )
{
	ASSERT( m_Handle != INVALID_HANDLE_VALUE );
	::SetThreadPriority( m_Handle, aPriority );
}

// ==========================================================================
int CCallbackThread::GetPriority() const
{
	ASSERT( m_Handle != INVALID_HANDLE_VALUE );
	return ::GetThreadPriority( m_Handle );
}

// ==========================================================================
bool CCallbackThread::IsStopping() const
{
	ASSERT( m_Handle != INVALID_HANDLE_VALUE );
	return m_bStop;
}

// ==========================================================================
bool CCallbackThread::IsDone()
{
	if ( ( m_Handle == INVALID_HANDLE_VALUE ) || ( m_ID == 0 ) )
		return true;

	return ( ::WaitForSingleObject( m_Handle, 0 ) == WAIT_OBJECT_0 );
}

// ==========================================================================
unsigned long WINAPI CCallbackThread::Link( void *aParam )
{
	CCallbackThread *lThis = reinterpret_cast<CCallbackThread *>( aParam );
	lThis->m_dwReturnValue = lThis->Function(lThis);
	return lThis->m_dwReturnValue;
}

// ==========================================================================
DWORD CCallbackThread::GetReturnValue()
{
	return m_dwReturnValue;
}

// ==========================================================================
void CCallbackThread::OnImageCallback()
{
	if( m_pListener )
		m_pListener->Invoke(m_pMain);
}

// ==========================================================================
DWORD CCallbackThread::Function(LPVOID param)
{
	CCallbackThread *pMain = (CCallbackThread*)param; 

	for ( ;; )
	{
		if ( IsStopping() )
			break;

		pMain->OnImageCallback();

		Sleep(0);
	}

	return 0;
}

