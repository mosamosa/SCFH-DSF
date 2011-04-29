// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#pragma once

#pragma warning(disable : 4995)

#ifndef COMMONH
#define COMMONH

#include <windows.h>
#include <stdio.h>
#include <psapi.h>

#include "sharedmem.h"
#include "stools.h"

#define FILTER_NAME L"SCFH DSF"
#define PROP_NAME L"SCFH DSF Property"
#define DUMMY_WINDOW_CLASS "SCFH_Dummy"

#define SHARED_BASENAME "SCFH_DSF_AREA"
#define SHARED_PIDLIST "SCFH_DSF_PIDS"
#define MUTEX_GLOBAL "SCFH_DSF_GMUTEX"
#define MUTEX_BASENAME "SCFH_DSF_MUTEX"

#define MAX_AREANUM 8
#define MAX_PIDLIST 32
#define MAX_TIMELIST 30

typedef DWORD HWND32;

#pragma pack(push, 1)

struct RECTF
{
	float left, top, right, bottom;
};

struct AreaInfo
{
	BOOL active;
	HWND32 hwnd;
	RECT src;
	RECTF dst;
	bool zoom, fixAspect, showCursor, showLW;
};

enum
{
	RESIZE_MODE_SW_NEAREST = 0,
	RESIZE_MODE_SW_BILINEAR,
	RESIZE_MODE_DD_1PASS,
	RESIZE_MODE_DD_2PASS
};

struct SharedInfo
{
	BOOL active;
	struct AreaInfo area[MAX_AREANUM];
	POINT cursor;
	int resizeMode;
	int rotateMode;
	double aveTime;
	int width, height;
	double framerate;
	DWORD threadNum;
	BOOL overSampling;
};

struct ProcessInfo
{
	DWORD pid;
	char name[MAX_PATH];
};

#pragma pack(pop)

struct Global
{
	volatile HINSTANCE hInstance;
	volatile HWND hwndDummy;
	CMutex mutexGlobal;

	bool isVista;

	char szFilename[MAX_PATH];

	SharedInfo *info;
	CSharedMemory csInfo;
	CMutex *mutexInfo;

	struct ProcessInfo *pids;
	CSharedMemory csPids;

	volatile int timePos;
	double times[MAX_TIMELIST];

	void processTime(double t)
	{
		int i;
		double ave = 0.0;

		times[timePos] = t;

		timePos++;
		if(timePos >= MAX_TIMELIST)
			timePos = 0;

		for(i=0; i<MAX_TIMELIST; i++)
			ave += times[i];
		ave /= MAX_TIMELIST;

		{
			CMutexLock lock(mutexInfo);
			info->aveTime = ave;
		}
	}

	Global() :
		mutexGlobal(MUTEX_GLOBAL)
	{
		char sztemp[256];

		isVista = false;

		timePos = 0;
		memset(times, 0, sizeof(double)*MAX_TIMELIST);

		sprintf(sztemp, "%s%d", MUTEX_BASENAME, GetCurrentProcessId());
		mutexInfo = new CMutex(sztemp);
		
		sprintf(sztemp, "%s%d", SHARED_BASENAME, GetCurrentProcessId());
		csInfo.create(sztemp, sizeof(SharedInfo));
		info = (SharedInfo *)csInfo.getData();

		csPids.create(SHARED_PIDLIST, sizeof(ProcessInfo)*MAX_PIDLIST);
		pids = (ProcessInfo *)csPids.getData();

		{
			CMutexLock lock(&mutexGlobal);
			int i;

			for(i=0; i<MAX_PIDLIST; i++)
			{
				if(pids[i].pid == 0)
				{
					pids[i].pid = GetCurrentProcessId();
					GetModuleBaseNameA(GetCurrentProcess(), NULL, pids[i].name, MAX_PATH);
					break;
				}
			}
		}
	}

	~Global()
	{
		if(info)
			info->active = FALSE;

		{
			CMutexLock lock(&mutexGlobal);
			int i;

			for(i=0; i<MAX_PIDLIST; i++)
			{
				if(pids[i].pid == GetCurrentProcessId())
				{
					pids[i].pid = 0;
					pids[i].name[0] = 0;
					break;
				}
			}
		}

		delete mutexInfo;
	}
};

extern struct Global g;

#endif
