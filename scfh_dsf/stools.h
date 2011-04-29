// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#ifndef STOOLS
#define STOOLS

#include <windows.h>
#include "tools.h"

class CMutexBase
{
public:
	virtual void lock() = 0;
	virtual void unlock() = 0;
};

class CMutexLock
{
private:
	CMutexBase *m_mutex;

public:
	CMutexLock(CMutexBase *mutex)
	{
		m_mutex = mutex;
		m_mutex->lock();
	}

	~CMutexLock()
	{
		m_mutex->unlock();
	}
};

class CMutexUnlock
{
private:
	CMutexBase *m_mutex;

public:
	CMutexUnlock(CMutexBase *mutex)
	{
		m_mutex = mutex;
	}

	~CMutexUnlock()
	{
		m_mutex->unlock();
	}
};

class CMutex : public CMutexBase
{
private:
	HANDLE mutex;
	int error;

public:
	CMutex(const char *name = NULL)
	{
		mutex = CreateMutexA(NULL, false, name);
		if(!mutex)
		{
			if(name)
				dprintf("CreateMutex() failed. (%s)", name);
			else
				dprintf("CreateMutex() failed.");
		}
		error = GetLastError();
	}

	~CMutex()
	{
		if(mutex)
			CloseHandle(mutex);
	}

	void lock()
	{
		if(mutex)
			WaitForSingleObject(mutex, INFINITE);
	}

	void unlock()
	{
		if(mutex)
			ReleaseMutex(mutex);
	}

	bool isAlreadyExists()
	{
		if(error == ERROR_ALREADY_EXISTS)
			return true;
		else
			return false;
	}
};

class CCriticalSection : public CMutexBase
{
private:
	CRITICAL_SECTION cs;

public:
	CCriticalSection()
	{
		InitializeCriticalSection(&cs);
	}

	~CCriticalSection()
	{
		DeleteCriticalSection(&cs);
	}

	void lock()
	{
		EnterCriticalSection(&cs);
	}

	void unlock()
	{
		LeaveCriticalSection(&cs);
	}

	bool tryLock()
	{
		return (TryEnterCriticalSection(&cs) != 0);
	}
};

class CEvent
{
private:
	HANDLE hEvent;

public:
	CEvent(bool manual_reset, bool initial_state, const char *szName = NULL)
	{
		hEvent = CreateEventA(NULL, manual_reset, initial_state, szName);
		if(!hEvent)
		{
			if(szName)
				dprintf("CreateEvent() failed. (%d,%d,%s)", manual_reset, initial_state, szName);
			else
				dprintf("CreateEvent() failed. (%d,%d)", manual_reset, initial_state);
		}
	}

	~CEvent()
	{
		if(hEvent)
			CloseHandle(hEvent);
	}

	void set()
	{
		if(hEvent)
			SetEvent(hEvent);
	}

	void reset()
	{
		if(hEvent)
			ResetEvent(hEvent);
	}

	bool wait(DWORD timeout = INFINITE)
	{
		if(hEvent)
			return (WaitForSingleObject(hEvent, timeout) != WAIT_TIMEOUT);

		return true;
	}

	bool isSet()
	{
		if(hEvent)
			return (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0);

		return false;
	}

	void pulse()
	{
		if(hEvent)
			PulseEvent(hEvent);
	}

	HANDLE getHandle()
	{
		return hEvent;
	}
};

class CThread
{
private:
	enum
	{
		TERMINATED,
		RUNNING
	};

	CEvent eventThread_;
	volatile bool terminate_;
	volatile int state_;
	DWORD exitCode_;
	HANDLE hThread_;
	char threadName_[MAX_PATH];

	static unsigned int __stdcall thread(void *arg);

protected:
	bool terminated(){ return terminate_; }
	void terminate(){ terminate_ = true; }

	virtual void onStart(){}
	virtual void onWait(){}

public:
	CThread(const char *name = NULL);
	virtual ~CThread();

	bool start(unsigned int stackSize = 0);
	bool wait(DWORD timeout = INFINITE);

	unsigned int getExitCode(){ return exitCode_; }
	bool isRunning(){ return (state_ == RUNNING); }

	virtual unsigned int run() = 0;
};

#endif
