// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#pragma warning(disable : 4995)

#include "common.h"
#include "resizedd.h"
#include "tools.h"
#include "mmtimer.h"

#include <streams.h>
#include <initguid.h>
#include <process.h>

#include "PushGuids.h"
#include "PushSource.h"
#include "prop.h"

static const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_Video,	// Major type
	&MEDIASUBTYPE_RGB32	// Minor type
};

static const AMOVIESETUP_PIN sudOutputPin = 
{
	L"Output",		// Obsolete, not used.
	FALSE,			// Is this pin rendered?
	TRUE,			// Is it an output pin?
	FALSE,			// Can the filter create zero instances?
	FALSE,			// Does the filter create multiple instances?
	&CLSID_NULL,	// Obsolete.
	NULL,			// Obsolete.
	1,				// Number of media types.
	&sudPinTypes	// Pointer to media types.
};

static const REGFILTERPINS2 sudOutputPin2 = 
{
	REG_PINFLAG_B_OUTPUT,
	1,
	1,
	&sudPinTypes,
	0,
	NULL,
	&PIN_CATEGORY_CAPTURE
};

static const REGFILTER2 rf2FilterReg =
{
	2,					// Version 1 (ピン メディアとピン カテゴリなし)。
	MERIT_DO_NOT_USE,	// メリット。
	1,					// ピンの数。
	(REGFILTERPINS *)&sudOutputPin2		// ピン情報へのポインタ。
};

static const AMOVIESETUP_FILTER sudPushSource =
{
	&CLSID_SCFH,		// Filter CLSID
	FILTER_NAME,		// String name
	MERIT_DO_NOT_USE,	// Filter merit
	1,					// Number pins
	&sudOutputPin		// Pin details
};

CFactoryTemplate g_Templates[2] = 
{
	{ 
		FILTER_NAME,					// Name
		&CLSID_SCFH,					// CLSID
		CPushSource::CreateInstance,	// Method to create an instance of MyComponent
		NULL,							// Initialization function
		&sudPushSource					// Set-up information (for filters)
	},
	{ 
		PROP_NAME,
		&CLSID_SCFHProp,
		CSCFHProp::CreateInstance, 
		NULL,
		NULL
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);    

STDAPI DllRegisterServer()
{
	HRESULT hr;
	IFilterMapper2 *pFM2 = NULL;

	hr = AMovieDllRegisterServer2(TRUE);
	if(FAILED(hr)) return hr;

	hr = CoCreateInstance(
		CLSID_FilterMapper2,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IFilterMapper2,
		(void **)&pFM2
		);
	if(FAILED(hr)) return hr;

	hr = pFM2->RegisterFilter(
		CLSID_SCFH,							// フィルタ CLSID。
		FILTER_NAME,						// フィルタ名。
		NULL,								// デバイス モニカ。
		&CLSID_VideoInputDeviceCategory,	// ビデオ コンプレッサ カテゴリ。
		FILTER_NAME,						// インスタンス データ。
		&rf2FilterReg						// フィルタ情報へのポインタ。
		);

	pFM2->Release();

	return hr;
}

STDAPI DllUnregisterServer()
{
	HRESULT hr;
	IFilterMapper2 *pFM2 = NULL;

	hr = AMovieDllRegisterServer2(FALSE);
	if(FAILED(hr)) return hr;

	hr = CoCreateInstance(
		CLSID_FilterMapper2,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IFilterMapper2,
		(void **)&pFM2
		);
	if (FAILED(hr)) return hr;

	hr = pFM2->UnregisterFilter(
		&CLSID_VideoInputDeviceCategory, 
		FILTER_NAME,
		CLSID_SCFH
		);

	pFM2->Release();

	return hr;
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH:
		g.hInstance = hInstance;

		CMicroTimer::calcError();

		{
			OSVERSIONINFOEXW ver;
			DWORDLONG dwlConditionMask = 0;

			memset(&ver, 0, sizeof(ver));

			ver.dwOSVersionInfoSize = sizeof(ver);
			ver.dwMajorVersion = 6;
			ver.dwMinorVersion = 0;
			ver.wProductType = VER_NT_WORKSTATION;

			VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
			VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
			VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_GREATER_EQUAL);

			if(VerifyVersionInfoW(&ver, VER_MAJORVERSION | VER_MINORVERSION | VER_PRODUCT_TYPE, dwlConditionMask))
			{
				g.isVista = true;
			}
		}

		{
			size_t i, len;
			char sztemp[256];

			GetModuleFileNameA(GetModuleHandle(NULL), sztemp, 256);
		
			len = strlen(sztemp);
			for(i=len-1; i>=0; i--)
				if(sztemp[i] == '\\')
					break;

			strcpy(g.szFilename, sztemp+i+1);
		}

		break;

	case DLL_PROCESS_DETACH:

		if(g.info)
			g.info->active = FALSE;

		break;
	}

	return DllEntryPoint(hInstance, dwReason, lpReserved);
}
