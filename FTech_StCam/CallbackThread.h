// *****************************************************************************
// CCallbackThread.h
// *****************************************************************************

#pragma once

class ListenerBase
{
public:
	virtual void Invoke() = 0;    
};

template <class T>
class MemFuncListener : public ListenerBase
{
public:
	MemFuncListener(T* obj, void (T::*cbf)())
	{
		m_obj = obj;
		m_cbf = cbf;
	};

	virtual void Invoke()
	{
		(m_obj->*m_cbf)();
	};
private:
	T* m_obj;
	void (T::*m_cbf)();
};

class CCallbackThread
{
public:
	CCallbackThread();
	~CCallbackThread();

	template <class T>
	void RegisterCallback(T *obj, void (T::*cbf)())
	{
		UnregisterCallback();
		m_pListener = new MemFuncListener<T>(obj, cbf);
	}

	void Start();
	void Start(int nRepeat);
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
	}

	bool IsStopping() const;
private:
	ListenerBase* m_pListener;
	HANDLE m_Handle;
	DWORD m_dwReturnValue;
	DWORD m_ID;
	bool m_bStop;
	int m_nRptCnt;
};
