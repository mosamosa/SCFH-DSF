// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include "stools.h"

#if defined(_MSC_VER) && _MSC_VER >= 1400

#define MS_VC_EXCEPTION 0x406D1388

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;

void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
	THREADNAME_INFO info;

	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

#endif 

unsigned int __stdcall CThread::thread(void *arg)
{
	CThread *_this = (CThread*)arg;
	unsigned int exitCode;
//dprintf("Thread start: %s TID: %d", _this->threadName_, GetCurrentThreadId());
#if defined(_MSC_VER) && _MSC_VER >= 1400
	SetThreadName(GetCurrentThreadId(), _this->threadName_);
#endif

	exitCode = _this->run();

	_this->exitCode_ = exitCode;
	_this->state_ = TERMINATED;
	_this->eventThread_.set();
//dprintf("Thread end: %s TID: %d", _this->threadName_, GetCurrentThreadId());
	_endthreadex(exitCode);
	return exitCode;
}

CThread::CThread(const char *name) :
	eventThread_(true, true)
{
	terminate_ = false;
	state_ = TERMINATED;
	exitCode_ = 0;
	hThread_ = NULL;

	if(name)
		sprintf(threadName_, "%s (%p)", name, this);
	else
		sprintf(threadName_, "CThread (%p)", this);
}

CThread::~CThread()
{
	assert(state_ == TERMINATED);
	assert(hThread_ == NULL);
//	wait();
}

bool CThread::start(unsigned int stackSize)
{
	if(isRunning())
		return false;

	if(hThread_ != NULL)
		return false;

	exitCode_ = 0;

	hThread_ = (HANDLE)_beginthreadex(NULL, stackSize, thread, this, CREATE_SUSPENDED, NULL);
	if(hThread_)
	{
		terminate_ = false;
		state_ = RUNNING;
		eventThread_.reset();

		onStart();

		ResumeThread(hThread_);

		return true;
	}
	else
	{
		state_ = TERMINATED;
		eventThread_.set();

		return false;
	}
}

bool CThread::wait(DWORD timeout)
{
	if(hThread_)
	{
		bool ret = true;

		terminate();

		onWait();

		if(isRunning())
		{
			ret = eventThread_.wait(timeout);
		}

		if(ret)
		{
			CloseHandle(hThread_);
			hThread_ = NULL;
		}

		return ret;
	}

	return true;
}
