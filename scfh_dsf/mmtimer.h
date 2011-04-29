// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#pragma once

#ifndef MMTIMERH
#define MMTIMERH

#define USE_WT
#define USE_REFTIME

#include <windows.h>
#ifdef USE_REFTIME
#include <streams.h>
#endif

#include "stools.h"

class CMicroTimer : public CThread
{
private:
#ifdef USE_REFTIME
	IReferenceClock *prc;
#endif
    double interval;
    CEvent eventTimer;
	CCriticalSection csState;

protected:
	void onWait();
	
public:
	int skip;

	CMicroTimer();	
	~CMicroTimer();
#ifdef USE_REFTIME	
	bool start(double sec, IReferenceClock *_prc);
#else
	bool start(double sec);
#endif
	void setInterval(double _interval)
	{
		CMutexLock lock(&csState);

		if(_interval <= 0.002)
			return;

		interval = _interval;
	}
	
	int waitTimer()
	{
		int ret;

		eventTimer.wait((DWORD)(interval * 1000.0) + 100);

		{
			CMutexLock lock(&csState);
			ret = skip;
			skip = 0;
		}

		return ret;
	}

	unsigned int run();

	static __int64 orgTime;
	static double error;	// Sleep�̌덷
	static void calcError();
};

#endif
