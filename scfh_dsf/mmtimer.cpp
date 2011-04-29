// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include <math.h>

#include "mmtimer.h"
#include "tools.h"

static struct sMMTimerGlobal
{
	TIMECAPS tc;

    sMMTimerGlobal()
	{
		timeGetDevCaps(&tc, sizeof(TIMECAPS));
		timeBeginPeriod(tc.wPeriodMin);
	}

	~sMMTimerGlobal()
	{
		timeEndPeriod(tc.wPeriodMin);
    }
}MMTimerGlobal;
	
unsigned int CMicroTimer::run()
{
	__int64 freq, org, _org, crr, obj;
	double remain;
#ifdef USE_WT
	HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
#endif

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#ifdef USE_REFTIME
	if(prc)
	{
		freq = UNITS;
		prc->GetTime(&_org);
	}
	else
#endif
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		QueryPerformanceCounter((LARGE_INTEGER*)&_org);
	}
	
    while(!terminated())
	{
#ifdef USE_REFTIME
		if(prc)
		{
			prc->GetTime(&crr);
		}
		else
#endif
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&crr);
		}

		if(orgTime)
			org = orgTime;
		else
			org = _org;
		
		obj = (__int64)(interval * (double)freq);
		obj = obj - ((crr - org) % obj);
		remain = (double)obj / (double)freq;
		obj += crr;
#ifdef USE_WT
		{
			LARGE_INTEGER t;

			remain *= 1000.0;
			remain -= error;	// Œë·

			t.QuadPart = -(LONGLONG)(remain*10000);
			SetWaitableTimer(hTimer, &t, 0, NULL, NULL, FALSE);
			WaitForSingleObject(hTimer, INFINITE);
		}
#else
		remain *= 1000.0;
		remain -= error;	// Sleep()‚ÌŒë·
		remain = floor(remain);
		if(remain >= 1.0)
			Sleep((DWORD)remain);
#endif
#ifdef USE_REFTIME
		if(prc)
		{
			for(;;)
			{
				prc->GetTime(&crr);
				if(crr >= obj || terminated())
					break;
			}
		}
		else
#endif
		{
			for(;;)
			{
				QueryPerformanceCounter((LARGE_INTEGER*)&crr);
				if(crr >= obj || terminated())
					break;
			}
		}

		{
			CMutexLock lock(&csState);
			skip++;
		}

        eventTimer.set();
		Sleep(0);
	}

#ifdef USE_WT
	CloseHandle(hTimer);
#endif
	return 0;
}

CMicroTimer::CMicroTimer() :
	CThread("CMicroTimer"),
	eventTimer(false, false, NULL)
{
	skip = 0;
	interval = 1.0;
#ifdef USE_REFTIME
	prc = NULL;
#endif
}

CMicroTimer::~CMicroTimer()
{
	wait();
}

#ifdef USE_REFTIME
bool CMicroTimer::start(double sec, IReferenceClock *_prc)
#else
bool CMicroTimer::start(double sec)
#endif
{
	wait();
	
	if(sec <= 0.002)
		return false;
	
	skip = 0;
	interval = sec;
#ifdef USE_REFTIME
	prc = _prc;
#endif		
	return __super::start();
}

void CMicroTimer::onWait()
{
#ifdef USE_REFTIME
	prc = NULL;
	eventTimer.set();
#endif
}

double CMicroTimer::error = 0.0;
__int64 CMicroTimer::orgTime = 0;

void CMicroTimer::calcError()
{

	HANDLE hThread;
	__int64 start, end;
	double ave = 0.0;
	int i, pri;
	const int interval = 10, loop = 20;
#ifdef USE_WT
	LARGE_INTEGER t;
	HANDLE hWTimer = CreateWaitableTimer(NULL, FALSE, NULL);
#endif

	hThread = GetCurrentThread();
	pri = GetThreadPriority(hThread);
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

	for(i=0; i<loop; i++)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&start);
#ifdef USE_WT
		t.QuadPart = -10000 * interval;
		SetWaitableTimer(hWTimer, &t, 0, NULL, NULL, FALSE);
		WaitForSingleObject(hWTimer, INFINITE);
#else
		Sleep(interval);
#endif
		QueryPerformanceCounter((LARGE_INTEGER*)&end);

		ave += pc2time(end-start)*1000.0 - (double)interval;
	}
	ave /= loop;

	SetThreadPriority(hThread, pri);

	error = ave;	// + 0.1;
	if(error < 0.0)
		error = 0.0;
	if(error > 1.5)
		error = 1.5;

#ifdef USE_WT
	CloseHandle(hWTimer);
#endif
}
