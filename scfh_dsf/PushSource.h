// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#pragma once

#include <strsafe.h>
#include <list>

#include "prop.h"
#include "stools.h"
#include "ddb.h"
#include "common.h"
#include "resizedd.h"
#include "tools.h"

class CPushSource;

class CPushPin :
	public CSourceStream,
	public IKsPropertySet,
	public IAMStreamConfig,
	public IAMPushSource
{
private:
	CPushSource *m_pParentFilter;

	static CCriticalSection m_csList;
	static std::list<CPushPin*> m_lThis;

	bool m_bChangedResolution;
	int m_ddbResetCount;

	REFERENCE_TIME m_rtOffset;

	CDDB m_ddb;

	REFERENCE_TIME m_rtStart;
	CCriticalSection m_cSharedState;
	IReferenceClock *m_pRC;

	CResize *m_resizeDD;

	CAlignedArray<DWORD> m_aSrcBuf, m_aStencil;

	std::list<CResizeThread*> lResizeThread;

	DWORD m_dwFrameNum;
	double m_fFramerate;
	int m_iImageHeight, m_iImageWidth;
	int m_iBeginImageHeight, m_iBeginImageWidth;

public:
	CPushPin(HRESULT *phr, CSource *pFilter, int width, int height, double framerate);
	~CPushPin();

	static void setChangedResolution()
	{
		CMutexLock lock(&m_csList);
		std::list<CPushPin*>::iterator i;

		for(i=m_lThis.begin(); i!=m_lThis.end(); i++)
		{
			(*i)->m_bChangedResolution = true;
		}
	}

	int GetMediaTypeCount();

	// IUnknown
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
	/*	dprintf("SCFH Pin QI: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			riid.Data1, riid.Data2, riid.Data3,
			riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
			riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);*/

		if(riid == IID_IKsPropertySet)
		{
			return GetInterface(static_cast<IKsPropertySet*>(this), ppv);
		}

		if(riid == IID_IAMStreamConfig)
		{
			return GetInterface(static_cast<IAMStreamConfig*>(this), ppv);
		}

		if(riid == IID_IAMPushSource)
		{
			return GetInterface(static_cast<IAMPushSource*>(this), ppv);
		}

		return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
	}

	// CSourceStream
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
	HRESULT FillBuffer(IMediaSample *pSample);
	HRESULT SetMediaType(const CMediaType *pMediaType);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q);
	HRESULT OnThreadCreate(void);
	HRESULT OnThreadDestroy(void);
	HRESULT OnThreadStartPlay(void);
	HRESULT DoBufferProcessingLoop(void);


	// IKsPropertySet
	STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
	STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned);
	STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

	// IAMStreamConfig
	STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
	STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
	STDMETHODIMP GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC);
	STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);

	// IAMLatency
	STDMETHODIMP GetLatency(REFERENCE_TIME *prtLatency);

	// IAMPushSource
	STDMETHODIMP GetPushSourceFlags(ULONG *pFlags);
	STDMETHODIMP SetPushSourceFlags(ULONG Flags);
	STDMETHODIMP GetMaxStreamOffset(REFERENCE_TIME *prtMaxOffset);
	STDMETHODIMP SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset);
	STDMETHODIMP GetStreamOffset(REFERENCE_TIME *prtOffset);
	STDMETHODIMP SetStreamOffset(REFERENCE_TIME rtOffset);
};

class CPushSource :
	public CSource,
	public IAMStreamConfig,
	public ISpecifyPropertyPages,
	public ISCFHSettings
{
private:
	volatile static LONG instanceNum;

	CPushPin *m_pPin;

	CCriticalSection  m_csShared;
	double m_fFramerate;
	int m_iImageHeight;
	int m_iImageWidth;

	CPushSource(IUnknown *pUnk, HRESULT *phr);
	~CPushSource();

	bool saveSettings();

public:
	static CUnknown* WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);

	// IUnknown
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
	/*	dprintf("SCFH Source QI: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			riid.Data1, riid.Data2, riid.Data3,
			riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
			riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);*/

		if(riid == IID_IAMStreamConfig)
		{
			return GetInterface(static_cast<IAMStreamConfig*>(this), ppv);
		}

		if(riid == IID_ISCFHSettings)
		{
			return GetInterface(static_cast<ISCFHSettings*>(this), ppv);
		}

		if(riid == IID_ISpecifyPropertyPages)
		{
			return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
		}

		return CSource::NonDelegatingQueryInterface(riid, ppv);
	}

	// IAMStreamConfig
	STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
	STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
	STDMETHODIMP GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC);
	STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);

	// ISpecifyPropertyPages
	STDMETHODIMP GetPages(CAUUID *pPages);

	// ISCFHSettings
	STDMETHODIMP GetSize(SIZE *pSize);
    STDMETHODIMP SetSize(SIZE *pSize);
    STDMETHODIMP GetFramerate(double *pfFramerate);
    STDMETHODIMP SetFramerate(double fFramerate);

	friend class CPushPin;
};
