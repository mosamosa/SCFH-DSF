// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#ifndef PROPH
#define PROPH

#include <streams.h>

DEFINE_GUID(IID_ISCFHSettings,
	0x1b1afbaf, 0xcb92, 0x42da, 0x83, 0x07, 0x5a, 0x7b, 0xe8, 0xc2, 0xb4, 0xb0);

DEFINE_GUID(CLSID_SCFHProp,
	0xc1a05412, 0xac19, 0x4c29, 0x9d, 0x43, 0x6e, 0x9c, 0xb8, 0xef, 0xfd, 0x11);

interface ISCFHSettings : public IUnknown
{
    STDMETHOD(GetSize)(SIZE *pSize) = 0;
    STDMETHOD(SetSize)(SIZE *pSize) = 0;
    STDMETHOD(GetFramerate)(double *pfFramerate) = 0;
    STDMETHOD(SetFramerate)(double fFramerate) = 0;
};

class CSCFHProp : public CBasePropertyPage
{
private:
	ISCFHSettings *settings;

	CSCFHProp(IUnknown *pUnk);
	~CSCFHProp();

public:
	static CUnknown* WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);

	void SetDirty();

	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnActivate(void);
	HRESULT OnApplyChanges(void);
	HRESULT OnDisconnect(void);
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


#endif
