// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include <streams.h>
#include <stdio.h>

#include "common.h"
#include "resource.h"
#include "prop.h"
#include "tools.h"

CSCFHProp::CSCFHProp(IUnknown *pUnk) :
	CBasePropertyPage(PROP_NAME, pUnk, IDD_PROPPAGE, IDS_PROPPAGE_TITLE)
{
	settings = NULL;
}

CSCFHProp::~CSCFHProp()
{
}

void CSCFHProp::SetDirty()
{
	m_bDirty = TRUE;
	if(m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

CUnknown* WINAPI CSCFHProp::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
	CSCFHProp *pNewFilter = new CSCFHProp(pUnk);

	*phr = S_OK;

	return pNewFilter;
}

HRESULT CSCFHProp::OnConnect(IUnknown *pUnk)
{
	CheckPointer(pUnk, E_POINTER);

	return pUnk->QueryInterface(IID_ISCFHSettings, (void**)&settings);
}

HRESULT CSCFHProp::OnActivate(void)
{
	ASSERT(settings);

	SIZE size;
	double framerate;
	HWND hwnd;
	char sztemp[256];

	settings->GetSize(&size);
	settings->GetFramerate(&framerate);

	hwnd = GetDlgItem(m_Dlg, IDC_EDIT_WIDTH);
	sprintf(sztemp, "%d", size.cx);
	SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)sztemp);

	hwnd = GetDlgItem(m_Dlg, IDC_EDIT_HEIGHT);
	sprintf(sztemp, "%d", size.cy);
	SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)sztemp);

	hwnd = GetDlgItem(m_Dlg, IDC_EDIT_FRAMERATE);
	sprintf(sztemp, "%.3lf", framerate);
	SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)sztemp);

	return S_OK;
}

HRESULT CSCFHProp::OnApplyChanges(void)
{
	ASSERT(settings);

	SIZE size;
	double framerate;
	HWND hwnd;
	char sztemp[256];

	hwnd = GetDlgItem(m_Dlg, IDC_EDIT_WIDTH);
	SendMessageA(hwnd, WM_GETTEXT, 256, (LPARAM)sztemp);
	sscanf(sztemp, "%d", &size.cx);

	hwnd = GetDlgItem(m_Dlg, IDC_EDIT_HEIGHT);
	SendMessageA(hwnd, WM_GETTEXT, 256, (LPARAM)sztemp);
	sscanf(sztemp, "%d", &size.cy);

	hwnd = GetDlgItem(m_Dlg, IDC_EDIT_FRAMERATE);
	SendMessageA(hwnd, WM_GETTEXT, 256, (LPARAM)sztemp);
	sscanf(sztemp, "%lf", &framerate);

	if(size.cx <= 32)
		size.cx = 32;
	if(size.cx > 4096)
		size.cx = 4096;
	if(size.cy <= 32)
		size.cy = 32;
	if(size.cy > 4096)
		size.cy = 4096;

	size.cx -= size.cx % 4;
	size.cy -= size.cy % 4;

	if(framerate < 0.1)
		framerate = 0.1;
	if(framerate > 120.0)
		framerate = 120.0;

	settings->SetSize(&size);
	settings->SetFramerate(framerate);

	return S_OK;
} 

HRESULT CSCFHProp::OnDisconnect(void)
{
	RELEASE(settings);

	return S_OK;
}

INT_PTR CSCFHProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_EDIT_WIDTH:
		case IDC_EDIT_HEIGHT:
		case IDC_EDIT_FRAMERATE:
			SetDirty();
			break;
		}
		break;
	}

	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
} 
