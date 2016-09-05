// *****************************************************************************
// CCallbackThread.h
// *****************************************************************************

#pragma once

class ListenerBase
{
public:
	virtual void Invoke(void *pMain) = 0;
};

template <class T>
class MemFuncListener : public ListenerBase
{
public:
	MemFuncListener(T* obj, void (T::*cbf)(void *))
	{
		m_obj = obj;
		m_cbf = cbf;
	};

	virtual void Invoke(void *pMain)
	{
		(m_obj->*m_cbf)(pMain);
	};
private:
	T* m_obj;
	void (T::*m_cbf)(void *);
};

class CCallbackThread
{
public:
	CCallbackThread();
	~CCallbackThread();

	template <class T>
	void RegisterCallback(T *obj, void (T::*cbf)(void *), void* pMain)
	{
		UnregisterCallback();
		m_pListener = new MemFuncListener<T>(obj, cbf);
		m_pMain = pMain;
	}

	void Start();
	void Stop();

	void SetPriority( int aPriority );
	int GetPriority() const;

	bool IsDone();
	DWORD GetReturnValue();

protected:
	static unsigned long WINAPI Link( void *aParam );
	virtual DWORD Function(LPVOID param);
	void OnImageCallback();

	void UnregisterCallback()
	{
		if( m_pListener )
		{
			delete m_pListener;
			m_pListener = NULL;
		}
		m_pMain = NULL;
	}

	bool IsStopping() const;
private:
	ListenerBase* m_pListener;
	void* m_pMain;
	HANDLE m_Handle;
	DWORD m_dwReturnValue;
	DWORD m_ID;
	bool m_bStop;
};
