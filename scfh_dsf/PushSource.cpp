// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include "tools.h"

#include <streams.h>
#include <math.h>
#include <vector>
#include <process.h>
#include <xmmintrin.h>

#include "PushSource.h"
#include "PushGuids.h"

#include "reg.h"
#include "event.h"
#include "mmtimer.h"

const SIZE sizeVideo[20] = {
	{320, 240},	// 4:3
	{400, 300},
	{480, 360},
	{512, 384},
	{640, 480},
	{800, 600},
	{960, 720},
	{1024, 768},
	{1280, 960},
	{1600, 1200},
	{320, 180}, // 16:9
	{480, 270},
	{512, 288},
	{640, 360},
	{800, 450},
	{960, 540},
	{1024, 576},
	{1280, 720},
	{1600, 900},
	{1920, 1080}
};

class CDummyWindowThread : public CThread
{
private:
	static LRESULT CALLBACK DummyWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(iMsg)
		{
		case WM_CREATE:
			InitResizeDD(hwnd);
			break;

		case WM_DISPLAYCHANGE:
			CPushPin::setChangedResolution();
			break;

		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;

		case WM_DESTROY:
			UninitResizeDD();

			PostQuitMessage(0);
			break;
		}

		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}

protected:
	void onWait()
	{
		SendMessage(g.hwndDummy, WM_CLOSE, 0, 0);
	}

public:
	CDummyWindowThread() :
		CThread("CDummyWindowThread")
	{
	}

	~CDummyWindowThread()
	{
		wait(3000);
	}

	unsigned int run()
	{
		MSG msg;
		WNDCLASSA wc;
		HWND hwnd;

		wc.lpszClassName = DUMMY_WINDOW_CLASS;
		wc.lpfnWndProc = DummyWndProc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hInstance = g.hInstance;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;

		if(!RegisterClassA(&wc))
			return -1;

		hwnd = CreateWindowExA(0, DUMMY_WINDOW_CLASS, DUMMY_WINDOW_CLASS, 0,
			0, 0, 32, 32, NULL, NULL, g.hInstance, NULL);

		if(!hwnd)
		{
			UnregisterClassA(DUMMY_WINDOW_CLASS, g.hInstance);
			return -2;
		}

		g.hwndDummy = hwnd;

		while(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		g.hwndDummy = NULL;

		UnregisterClassA(DUMMY_WINDOW_CLASS, g.hInstance);

		return 0;
	}
}DummyWindowThread;

STDMETHODIMP GetFormat(CPushPin *pPin, int iIndex, AM_MEDIA_TYPE **ppmt)
{
	CheckPointer(ppmt, E_POINTER);

	HRESULT hr;
	CMediaType mt;

	hr = pPin->GetMediaType(iIndex, &mt);
	if(FAILED(hr)) return hr;

	*ppmt = static_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
	**ppmt = mt;
	(*ppmt)->pbFormat = static_cast<BYTE*>(CoTaskMemAlloc((*ppmt)->cbFormat));
	memcpy((*ppmt)->pbFormat, mt.Format(), (*ppmt)->cbFormat);

	return S_OK;
}

STDMETHODIMP SetFormat(CPushPin *pPin, AM_MEDIA_TYPE *pmt)
{
	CheckPointer(pmt, E_POINTER);

	HRESULT hr;
	CMediaType mt;

	mt = *pmt;

	hr = pPin->SetMediaType(&mt);
	if(FAILED(hr)) return hr;

	return S_OK;
}

STDMETHODIMP GetStreamCaps(CPushPin *pPin, int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC)
{
	CheckPointer(ppmt, E_POINTER);
	CheckPointer(pSCC, E_POINTER);

	HRESULT hr;

	if(iIndex < 0)
		return E_INVALIDARG;

	hr = ::GetFormat(pPin, iIndex, ppmt);
	if(hr == VFW_S_NO_MORE_ITEMS) return S_FALSE;
	if(FAILED(hr)) return hr;

	ASSERT((*ppmt)->formattype == FORMAT_VideoInfo);

	VIDEO_STREAM_CONFIG_CAPS *caps = (VIDEO_STREAM_CONFIG_CAPS*)pSCC;
	VIDEOINFO *info = reinterpret_cast<VIDEOINFO*>((*ppmt)->pbFormat);

	ZeroMemory(caps, sizeof(VIDEO_STREAM_CONFIG_CAPS));
	caps->VideoStandard = 0;
	caps->guid = FORMAT_VideoInfo;
	caps->CropAlignX = 1;
	caps->CropAlignY = 1;
	caps->CropGranularityX = 1;
	caps->CropGranularityY = 1;
	caps->OutputGranularityX = 1;
	caps->OutputGranularityY = 1;
	caps->InputSize.cx = info->bmiHeader.biWidth;
	caps->InputSize.cy = abs(info->bmiHeader.biHeight);
	caps->MinOutputSize.cx = 32;
	caps->MinOutputSize.cy = 32;
	caps->MaxOutputSize.cx = 4096;
	caps->MaxOutputSize.cy = 4096;
	caps->MinCroppingSize = caps->InputSize;
	caps->MaxCroppingSize = caps->InputSize;
	caps->MinBitsPerSecond = 1;
	caps->MaxBitsPerSecond = 1;
	caps->MinFrameInterval = 83333;
	caps->MaxFrameInterval = 100000000;
	caps->StretchTapsX = 0;
	caps->StretchTapsY = 0;
	caps->ShrinkTapsX = 0;
	caps->ShrinkTapsY = 0;

	return S_OK;
}

STDMETHODIMP GetNumberOfCapabilities(CPushPin *pPin, int *piCount, int *piSize)
{
	CheckPointer(piCount, E_POINTER);
	CheckPointer(piSize, E_POINTER);

	*piCount = pPin->GetMediaTypeCount();
	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);

	return S_OK;
}

CCriticalSection CPushPin::m_csList;
std::list<CPushPin*> CPushPin::m_lThis;

CPushPin::CPushPin(HRESULT *phr, CSource *pFilter, int width, int height, double framerate) :
	CSourceStream(FILTER_NAME, phr, pFilter, L"Capture"),
	m_pParentFilter(static_cast<CPushSource*>(pFilter))
{
	{
		CMutexLock lock(&m_csList);
		m_lThis.push_back(this);
	}

	m_ddbResetCount = 0;
	m_dwFrameNum = 0;
	m_rtOffset = 0;
	m_bChangedResolution = false;

	m_fFramerate = framerate;
	m_iBeginImageWidth = m_iImageWidth = width;
	m_iBeginImageHeight = m_iImageHeight = height;

	m_resizeDD = CreateInstanceResizeDD();

	m_pRC = NULL;
}

CPushPin::~CPushPin()
{
	size_t i, n;

	n = lResizeThread.size();
	for(i=0; i<n; i++)
	{
		delete lResizeThread.front();
		lResizeThread.pop_front();
	}

	delete m_resizeDD;

	RELEASE(m_pRC);

	{
		CMutexLock lock(&m_csList);
		m_lThis.remove(this);
//		dprintf("SCFH DSF: Remain instance: %d", m_lThis.size());
	}
}

class CFixedRC
{
private:
	IReferenceClock *prc;
	LARGE_INTEGER tBase;
	REFERENCE_TIME rtBase, rtBefore;

public:
	CFixedRC(IReferenceClock *prc_ = NULL)
	{
		prc = prc_;
		tBase.QuadPart = 0;
		rtBase = 0;
		rtBefore = 0;
	}

	void setReferenceClock(IReferenceClock *prc_ = NULL)
	{
		prc = prc_;
	}

	REFERENCE_TIME getTime()
	{
		LARGE_INTEGER tCurr;
		REFERENCE_TIME rtRet;

		QueryPerformanceCounter(&tCurr);

		if(pc2time(tCurr.QuadPart-tBase.QuadPart) > 60.0)
		{
			tBase = tCurr;
			prc->GetTime(&rtBase);
		}

		rtRet = rtBase + (REFERENCE_TIME)(pc2time(tCurr.QuadPart-tBase.QuadPart)*UNITS);
		if(rtRet < rtBefore)
		{
			rtRet = rtBefore;
		}

		rtBefore = rtRet;

		return rtRet;
	}
};

bool ObjectiveSleep(CFixedRC &frc, REFERENCE_TIME rtStart, REFERENCE_TIME rtObjective)
{
	double remain;
	REFERENCE_TIME rtCurr;

	rtCurr = frc.getTime();
	rtCurr -= rtStart;

	remain = (rtObjective - rtCurr) / (double)UNITS;
	remain *= 1000;
	remain -= CMicroTimer::error;

	if(remain > 0)
	{
		Sleep((DWORD)remain);
		return true;
	}
	else
	{
		return false;
	}
}

void blendSample(BYTE *bufa, long lena, BYTE *bufb, long lenb)
{
	int i;
	long len;

	if(lena != lenb)
		return;

#ifndef WIN64
	len = lena / 8;
	if(len > 0)
	{
		_asm
		{
			mov esi, len;
			mov ecx, bufa;
			mov edx, bufb;

		LOOP1:
			movq mm0, [ecx];
			movq mm1, [edx];

			pavgb mm0, mm1;

			movq [ecx], mm0;

			lea ecx, [ecx+8];
			lea edx, [edx+8];
			dec esi;

			jnz LOOP1;

			mov bufa, ecx;
			mov bufb, edx;

			emms;
		}
	}

	len = lena % 8;
	for(i=0; i<len; ++i)
	{
		bufa[i] = (bufa[i] + bufb[i]) >> 1;
	}
#else //WIN64
	len = lena;

	for(i=0; i<len; ++i)
	{
		bufa[i] = (bufa[i] + bufb[i]) >> 1;
	}
#endif //WIN64
}

void blendSample2(BYTE *bufa, long lena, BYTE *bufb, long lenb, double amp)
{
	int i;
	long len;
	DWORD ampa, ampb;

	if(amp == 0.5)
	{
		blendSample(bufa, lena, bufb, lenb);
		return;
	}

	if(lena != lenb)
		return;

	ampa = (DWORD)(0x0100 * amp);
	ampb = (DWORD)(0x0100 * (1.0 - amp));

	if(ampa + ampb < 0x0100)
		ampa += 1;

#ifndef WIN64
	len = lena / 8;
	if(len > 0)
	{
		_asm
		{
			mov esi, len;
			mov ecx, bufa;
			mov edx, bufb;

			movq mm6, ampa;
			punpcklwd mm6, mm6;
			punpcklwd mm6, mm6;

			movq mm7, ampb;
			punpcklwd mm7, mm7;
			punpcklwd mm7, mm7;

			pxor mm5, mm5;

		LOOP1:
			movq mm0, [ecx];
			movq mm2, [edx];

			movq mm1, mm0;
			movq mm3, mm2;

			punpcklbw mm0, mm5;
			punpckhbw mm1, mm5;
			punpcklbw mm2, mm5;
			punpckhbw mm3, mm5;

			pmullw mm0, mm6;
			pmullw mm1, mm6;
			pmullw mm2, mm7;
			pmullw mm3, mm7;

			paddusw mm0, mm2;
			paddusw mm1, mm3;

			psrlw mm0, 8;
			psrlw mm1, 8;

			packuswb mm0, mm1;

			movq [ecx], mm0;

			lea ecx, [ecx+8];
			lea edx, [edx+8];
			dec esi;

			jnz LOOP1;

			mov bufa, ecx;
			mov bufb, edx;

			emms;
		}
	}

	len = lena % 8;
	for(i=0; i<len; ++i)
	{
		bufa[i] = (BYTE)((bufa[i]*ampa + bufb[i]*ampb) >> 8);
	}
#else
	len = lena;

	for(i=0; i<len; ++i)
	{
		bufa[i] = (BYTE)((bufa[i]*ampa + bufb[i]*ampb) >> 8);
	}
#endif //WIN64
}

HRESULT CPushPin::DoBufferProcessingLoop(void)
{
	Command com;
	CFixedRC frc;
	REFERENCE_TIME rtStart;

	OnThreadStartPlay();

	frc.setReferenceClock(m_pRC);
	rtStart = frc.getTime();

	do
	{
		HRESULT hr;
		IMediaSample *pBefSample = NULL;
		bool enableBefSample = false;
		LARGE_INTEGER tstart, tend;

		hr = GetDeliveryBuffer(&pBefSample, NULL, NULL, 0);
		if(FAILED(hr))
		{
			return S_OK;
		}

		while(!CheckRequest(&com))
		{
			IMediaSample *pSample;
			REFERENCE_TIME rtMyStart;
			BOOL overSampling = g.info->overSampling;
			double processTime = 0;

			{
				REFERENCE_TIME t;

				t = rtStart - m_pParentFilter->m_tStart.m_time;
				if(t < 0)
					t = -t;

				if(t < UNITS)
				{
					rtMyStart = m_pParentFilter->m_tStart.m_time;
				}
				else
				{
					dprintf("SCFH DSF: Use my start time.");
					rtMyStart = rtStart;
				}
			}

			REFERENCE_TIME rtFrameLength = (REFERENCE_TIME)(UNITS / m_fFramerate);
			REFERENCE_TIME rtA, rtB, rtCurr;

			rtCurr = frc.getTime();
			rtCurr -= rtMyStart;

			rtA = m_dwFrameNum * rtFrameLength;
//			rtA += m_rtOffset;
			rtB = rtA + rtFrameLength;

			m_dwFrameNum++;

			if(rtCurr > rtB+rtFrameLength)
			{
				int count = 0;

				do
				{
					rtA = m_dwFrameNum * rtFrameLength;
//					rtA += m_rtOffset;
					rtB = rtA + rtFrameLength;

					m_dwFrameNum++;
					count++;
				}
				while(rtCurr > rtB);

				dprintf("SCFH DSF: Frame skip. (%d frame)", count);
			}

			hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
			if(FAILED(hr))
			{
				Sleep(1);
				continue;
			}

			QueryPerformanceCounter(&tstart);

			hr = FillBuffer(pSample);

			{
				long lena, lenb;
				BYTE *bufa, *bufb;

				lena = pSample->GetActualDataLength();
				pSample->GetPointer(&bufa);

				lenb = pBefSample->GetActualDataLength();
				pBefSample->GetPointer(&bufb);

				if(enableBefSample)
					blendSample(bufa, lena, bufb, lenb);
			}

			QueryPerformanceCounter(&tend);
			processTime += pc2time(tend.QuadPart-tstart.QuadPart);

			pSample->SetTime(&rtA, &rtB);

			if(hr == S_OK)
			{
				hr = Deliver(pSample);
				pSample->Release();

				if(hr != S_OK)
				{
					DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
					return S_OK;
				}
			}
			else if(hr == S_FALSE)
			{
				pSample->Release();
				DeliverEndOfStream();
				return S_OK;
			}
			else
			{
				pSample->Release();
				DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
				DeliverEndOfStream();
				m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
				return hr;
			}

			if(overSampling && ObjectiveSleep(frc, rtMyStart, rtB - rtFrameLength/2))
			{
				QueryPerformanceCounter(&tstart);

				hr = FillBuffer(pBefSample);
				if(SUCCEEDED(hr))
					enableBefSample = true;
				else
					enableBefSample = false;

				QueryPerformanceCounter(&tend);
				processTime += pc2time(tend.QuadPart-tstart.QuadPart);
			}
			else
			{
				enableBefSample = false;
			}

			ObjectiveSleep(frc, rtMyStart, rtB);

			g.processTime(processTime);
		}

		RELEASE(pBefSample);

		if(com == CMD_RUN || com == CMD_PAUSE)
		{
			Reply(NOERROR);
		}
		else if(com != CMD_STOP)
		{
			Reply((DWORD) E_UNEXPECTED);
			DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
		}
	}while(com != CMD_STOP);

	return S_FALSE;
}

HRESULT CPushPin::FillBuffer(IMediaSample *pSample)
{
	CheckPointer(pSample, E_POINTER);

	SharedInfo info;

	ASSERT(m_mt.formattype == FORMAT_VideoInfo);
	ASSERT(g.info);

	VIDEOINFOHEADER *vih = reinterpret_cast<VIDEOINFOHEADER*>(m_mt.pbFormat);
/*
	CMediaType *pmt = NULL;
	hr = pSample->GetMediaType((AM_MEDIA_TYPE**)&pmt);
	if(hr == S_OK)
	{
		SetMediaType(pmt);
		DeleteMediaType(pmt);
	}
*/
	{
		CMutexLock lock(g.mutexInfo);
		memcpy(&info, g.info, sizeof(SharedInfo));
	}

	{
		CMutexLock lock(&m_cSharedState);

		BYTE *pData = NULL;
		long cbData = 0;
		CResize *resize = NULL;
		DWORD threadNum = 0;
		bool ddbReset = false;

		if(++m_ddbResetCount >= 300)
		{
			m_ddbResetCount = 0;
			ddbReset = true;
		}

		pSample->GetPointer(&pData);
		cbData = pSample->GetSize();
		pSample->SetActualDataLength(cbData);

		int sampleWidth = vih->bmiHeader.biWidth;
		int sampleHeight = abs(vih->bmiHeader.biHeight);

		if(info.active)
		{
			switch(info.resizeMode)
			{
			case RESIZE_MODE_DD_1PASS:
			case RESIZE_MODE_DD_2PASS:
				resize = m_resizeDD;
				break;
			case RESIZE_MODE_SW_NEAREST:
			case RESIZE_MODE_SW_BILINEAR:
				resize = NULL;
				m_resizeDD->clear();
				break;
			}

			switch(info.resizeMode)
			{
			case RESIZE_MODE_DD_1PASS:
				resize->setMode(MODE_LOWQUALITY);
				break;
			case RESIZE_MODE_DD_2PASS:
				resize->setMode(MODE_HIGHQUALITY);
				break;
			}

			switch(info.resizeMode)
			{
			case RESIZE_MODE_DD_1PASS:
			case RESIZE_MODE_DD_2PASS:
				m_aSrcBuf.remove();
				m_aStencil.remove();
				break;
			}

			switch(info.resizeMode)
			{
			case RESIZE_MODE_DD_1PASS:
			case RESIZE_MODE_DD_2PASS:
				threadNum = 0;
				break;
			case RESIZE_MODE_SW_NEAREST:
			case RESIZE_MODE_SW_BILINEAR:
				threadNum = info.threadNum;
				if(threadNum < 2)
					threadNum = 1;
				if(threadNum > 16)
					threadNum = 16;
				break;
			}

			if(threadNum > lResizeThread.size())
			{
				size_t i;
				size_t n;

				n = threadNum - lResizeThread.size();
				for(i=0; i<n; i++)
				{
					CResizeThread *p;

					p = new CResizeThread();
					p->start();

					lResizeThread.push_back(p);
				}
			}
			else if(threadNum < lResizeThread.size())
			{
				size_t i, n;

				n = lResizeThread.size() - threadNum;
				for(i=0; i<n; i++)
				{
					delete lResizeThread.front();
					lResizeThread.pop_front();
				}
			}

			if(resize)
			{
				if(m_ddb.getWidth() != sampleWidth || m_ddb.getHeight() != sampleHeight || ddbReset)
					m_ddb.create(GetDesktopWindow(), sampleWidth, sampleHeight);

				if(m_bChangedResolution)
				{
					m_bChangedResolution = false;
					resize->check(true);
				}
				else
				{
					resize->check(false);
				}

				resize->batchResize(m_ddb.getDC(), info.area, MAX_AREANUM, sampleWidth, sampleHeight);

				m_ddb.getImage(pData, vih->bmiHeader.biBitCount, true, NULL);
			}
			else
			{
				DWORD *dst, *src, *sten;
				int i, len;

				ASSERT(vih->bmiHeader.biBitCount == 32);

				SIZE dstsize = {sampleWidth, sampleHeight};
				SIZE srcsize = {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};

				if(m_bChangedResolution)
					m_bChangedResolution = false;

				dst = reinterpret_cast<DWORD*>(pData);

				len = srcsize.cx * srcsize.cy;
				m_aSrcBuf.create(len);
				src = m_aSrcBuf;

				len = (int)(ceil(dstsize.cx*dstsize.cy / 32.0));
				m_aStencil.create(len);
				m_aStencil.clear();
				sten = m_aStencil;

				int writecount = 0;

				{
					int maxSrcWidth = 0, maxSrcHeight = 0;

					for(i=0; i<MAX_AREANUM; i++)
					{
						if(!info.area[i].active)
							continue;

						struct ResizeInfo rinfo;
						RECT rectDest;
						
						rectDest.left = (LONG)floor(info.area[i].dst.left * sampleWidth);
						rectDest.top = (LONG)floor(info.area[i].dst.top * sampleHeight);
						rectDest.right = (LONG)ceil((info.area[i].dst.right-info.area[i].dst.left) * sampleWidth) + rectDest.left;
						rectDest.bottom = (LONG)ceil((info.area[i].dst.bottom-info.area[i].dst.top) * sampleHeight) + rectDest.top;

						FixRect(&rectDest, 0, 0, sampleWidth, sampleHeight);

						calcResizeInfo(&rinfo, &rectDest, &info.area[i].src, &dstsize, &srcsize, info.area[i].zoom, info.area[i].fixAspect);

						maxSrcWidth = max(maxSrcWidth, rinfo.src_width);
						maxSrcHeight = max(maxSrcHeight, rinfo.src_height);
					}

					if(m_ddb.getWidth() != maxSrcWidth || m_ddb.getHeight() != maxSrcHeight || ddbReset)
						m_ddb.create(GetDesktopWindow(), maxSrcWidth, maxSrcHeight);
				}

				for(i=0; i<MAX_AREANUM; i++)
				{
					if(!info.area[i].active)
						continue;

					struct ResizeInfo rinfo;
					RECT rectDest;
					
					rectDest.left = (LONG)floor(info.area[i].dst.left * sampleWidth);
					rectDest.top = (LONG)floor(info.area[i].dst.top * sampleHeight);
					rectDest.right = (LONG)ceil((info.area[i].dst.right-info.area[i].dst.left) * sampleWidth) + rectDest.left;
					rectDest.bottom = (LONG)ceil((info.area[i].dst.bottom-info.area[i].dst.top) * sampleHeight) + rectDest.top;

					FixRect(&rectDest, 0, 0, sampleWidth, sampleHeight);

					calcResizeInfo(&rinfo, &rectDest, &info.area[i].src, &dstsize, &srcsize, info.area[i].zoom, info.area[i].fixAspect);

					if(rinfo.dst_width <= 0 || rinfo.dst_height <= 0)
						continue;

					{
						DWORD dwMyROP;
						RECT rect;

						if(info.area[i].showLW)
							dwMyROP = SRCCOPY | CAPTUREBLT;
						else
							dwMyROP = SRCCOPY;

						SetRect(&rect, 0, 0, rinfo.src_width, rinfo.src_height);
						FillRect(m_ddb.getDC(), &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

						HDC hdc = GetDC((HWND)info.area[i].hwnd);

						BitBlt(m_ddb.getDC(), 0, 0, rinfo.src_width, rinfo.src_height,
							hdc, rinfo.src_x, rinfo.src_y, dwMyROP);

						{
							POINT pos;
							int cursor_x, cursor_y;

							GetCursorPos(&pos);
							ScreenToClient((HWND)info.area[i].hwnd, &pos);

							cursor_x = pos.x - rinfo.src_x;
							cursor_y = pos.y - rinfo.src_y;

							if(info.area[i].showCursor)
								DrawCursor(m_ddb.getDC(), cursor_x, cursor_y);
						}

						ReleaseDC((HWND)info.area[i].hwnd, hdc);

						m_ddb.getImage(src, 32, false, NULL, rinfo.src_width, rinfo.src_height);
					}

					if(rinfo.amp == 1.0)
					{
						DWORD ix, iy;
						DWORD stenpos, dstpos, srcpos;
						DWORD mask;

						for(iy=0; iy<(DWORD)rinfo.dst_height; ++iy)
						{
							dstpos = (sampleHeight-(iy+rinfo.dst_y)-1)*sampleWidth + rinfo.dst_x;
							srcpos = iy*rinfo.src_width;

							for(ix=0; ix<(DWORD)rinfo.dst_width; ++ix)
							{
								stenpos = dstpos >> 5;
								mask = 1 << (dstpos&0x1f);

								if(!(sten[stenpos] & mask))
								{
									sten[stenpos] |= mask;
									writecount++;

									dst[dstpos] = src[srcpos];
								}

								dstpos++;
								srcpos++;
							}
						}
					}
					else
					{
						if(threadNum > 0)
						{
							int k, splitNum;

							splitNum = threadNum;
							for(k=0; k<(int)threadNum; k++)
							{
								if(rinfo.dst_height / splitNum >= 64)
									break;
								splitNum--;
							}

							if(splitNum < 1)
								splitNum = 1;

							std::list<CResizeThread*>::iterator it;

							for(k=0, it=lResizeThread.begin(); k<splitNum; k++, it++)
							{
								CResizeThread *thread = *it;

								thread->dst = dst;
								thread->src = src;
								thread->resizeMode = info.resizeMode;
								thread->rinfo = rinfo;
								thread->sampleHeight = sampleHeight;
								thread->sampleWidth = sampleWidth;
								thread->sten = sten;

								thread->lineStart = rinfo.dst_height*k / splitNum;
								thread->lineEnd = rinfo.dst_height*(k+1) / splitNum;

								thread->eventStart.set();
							}

							for(k=0, it=lResizeThread.begin(); k<splitNum; k++, it++)
							{
								CResizeThread *thread = *it;

								thread->eventEnd.wait();
								writecount += thread->writecount;
							}
						}
						else
						{
							CResizeThread thread;

							thread.dst = dst;
							thread.src = src;
							thread.resizeMode = info.resizeMode;
							thread.rinfo = rinfo;
							thread.sampleHeight = sampleHeight;
							thread.sampleWidth = sampleWidth;
							thread.sten = sten;

							thread.lineStart = 0;
							thread.lineEnd = rinfo.dst_height;

							thread.resize();

							writecount += thread.writecount;
						}
					}

					if(writecount >= sampleWidth*sampleHeight)
						break;
				}// for(i=0; i<MAX_AREANUM; i++)

				{	// 何も描画されていないところを塗りつぶし
					DWORD stenpos;
					DWORD mask;

					len = sampleWidth * sampleHeight;

					if(writecount < len)
					{
						for(i=0; i<len; ++i)
						{
							stenpos = i >> 5;
							mask = 1 << (i&0x1f);

							if(!(sten[stenpos] & mask))
								dst[i] = 0;
						}
					}
				}
			}
		}
		else
		{
			m_resizeDD->clear();

			{
				if(m_ddb.getWidth() != sampleWidth || m_ddb.getHeight() != sampleHeight || ddbReset)
					m_ddb.create(GetDesktopWindow(), sampleWidth, sampleHeight);	
	
				HDC hdc = GetDC(GetDesktopWindow());
				BitBlt(m_ddb.getDC(), 0, 0, sampleWidth, sampleHeight, hdc, 0, 0, SRCCOPY);
				ReleaseDC(GetDesktopWindow(), hdc);
	
				m_ddb.getImage(pData, vih->bmiHeader.biBitCount, true, NULL);
			}
		}

		pSample->SetSyncPoint(TRUE);
	}

	return S_OK;
}

HRESULT CPushPin::OnThreadCreate(void)
{
	CMutexLock lock(&m_cSharedState);

	return S_OK;
}

HRESULT CPushPin::OnThreadDestroy(void)
{
	CMutexLock lock(&m_cSharedState);

	m_resizeDD->clear();
	m_ddb.release();

	m_aSrcBuf.remove();
	m_aStencil.remove();

	return S_OK;
}

HRESULT CPushPin::OnThreadStartPlay(void)
{
	CMutexLock lock(&m_cSharedState);
	HRESULT hr;
	
	m_dwFrameNum = 0;
	m_rtOffset = 0;

	RELEASE(m_pRC);
	hr = m_pParentFilter->GetSyncSource(&m_pRC);
	if(SUCCEEDED(hr) && m_pRC)
	{
		dprintf("SCFH DSF: Use graph clock.");
	}
	else
	{
		dprintf("SCFH DSF: Use system clock.");

		hr = CoCreateInstance(
			CLSID_SystemClock,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IReferenceClock,
			reinterpret_cast<void**>(&m_pRC)
			);
		if(FAILED(hr)) return hr;
	}

	return S_OK;
}

int CPushPin::GetMediaTypeCount()
{
	HRESULT hr;
	int i, count = 0;

	for(i=0; i<100; i++)
	{
		CMediaType mt;

		hr = GetMediaType(i, &mt);
		if(hr == NOERROR)
			count++;
		else
			break;
	}

	return count;
}

HRESULT CPushPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);

	CAutoLock cAutoLock(m_pFilter->pStateLock());

	const int sizenum = ARRAY_SIZEOF(sizeVideo) + 3;
//	const int sizenum = 1;

	if(iPosition < 0)
		return E_INVALIDARG;
	if(iPosition >= sizenum)
		return VFW_S_NO_MORE_ITEMS;

	VIDEOINFO *pvi = reinterpret_cast<VIDEOINFO *>(pMediaType->AllocFormatBuffer(sizeof(VIDEOINFO)));
	if(NULL == pvi)
		return E_OUTOFMEMORY;

	ZeroMemory(pvi, sizeof(VIDEOINFO));

	pvi->bmiHeader.biCompression  = BI_RGB;
	pvi->bmiHeader.biBitCount     = 32;
	pvi->bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
	if(iPosition == 0)
	{
		pvi->bmiHeader.biWidth    = m_iImageWidth;
		pvi->bmiHeader.biHeight   = m_iImageHeight;
	}
	else if(iPosition == 1)
	{
		pvi->bmiHeader.biWidth    = m_iBeginImageWidth;
		pvi->bmiHeader.biHeight   = m_iBeginImageHeight;
	}
	else if(iPosition == sizenum-1)
	{
		pvi->bmiHeader.biWidth    = m_iBeginImageWidth;
		pvi->bmiHeader.biHeight   = m_iBeginImageHeight;
	}
	else
	{
		pvi->bmiHeader.biWidth        = sizeVideo[iPosition-2].cx;
		pvi->bmiHeader.biHeight       = sizeVideo[iPosition-2].cy;
	}
	pvi->bmiHeader.biPlanes       = 1;
	pvi->bmiHeader.biSizeImage    = GetBitmapSize(&pvi->bmiHeader);
	pvi->bmiHeader.biClrImportant = 0;

	pvi->AvgTimePerFrame = static_cast<REFERENCE_TIME>(UNITS/m_fFramerate);

	SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
	SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

	pMediaType->SetType(&MEDIATYPE_Video);
	pMediaType->SetFormatType(&FORMAT_VideoInfo);
	pMediaType->SetTemporalCompression(FALSE);

	const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
	pMediaType->SetSubtype(&SubTypeGUID);
	pMediaType->SetSampleSize(pvi->bmiHeader.biSizeImage);

	return NOERROR;
}

HRESULT CPushPin::SetMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);

	CAutoLock cAutoLock(m_pFilter->pStateLock());

	HRESULT hr;
	
	{
		VIDEOINFO *pvi = reinterpret_cast<VIDEOINFO*>(pMediaType->Format());
		if(pvi == NULL)
			return E_UNEXPECTED;

		dprintf("SCFH DSF: SetMediaType %dx%d@%.3f", pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight, (double)UNITS / pvi->AvgTimePerFrame);

		if(pvi->bmiHeader.biWidth == 0 || pvi->bmiHeader.biHeight == 0 || pvi->AvgTimePerFrame == 0)
			return E_INVALIDARG;
	}

	hr = CSourceStream::SetMediaType(pMediaType);
	if(SUCCEEDED(hr))
	{
		VIDEOINFO *pvi = reinterpret_cast<VIDEOINFO*>(m_mt.Format());
		if(pvi == NULL)
			return E_UNEXPECTED;

		if(pvi->AvgTimePerFrame <= 0)
			return E_INVALIDARG;

		switch(pvi->bmiHeader.biBitCount)
		{
		case 32:
			m_iImageWidth = pvi->bmiHeader.biWidth;
			m_iImageHeight = abs(pvi->bmiHeader.biHeight);
			m_fFramerate = (double)UNITS / pvi->AvgTimePerFrame;

			{
				CMutexLock lock(g.mutexInfo);
				g.info->width = m_iImageWidth;
				g.info->height = m_iImageHeight;
				g.info->framerate = m_fFramerate;
			}

			hr = S_OK;
			break;
		default:
			hr = E_INVALIDARG;
			break;
		}
	} 

	return hr;
}

HRESULT CPushPin::CheckMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);

	if(	(*(pMediaType->Type()) != MEDIATYPE_Video) ||
		!(pMediaType->IsFixedSize()))
	{                                                  
		return E_INVALIDARG;
	}

	const GUID *SubType = pMediaType->Subtype();
	if (SubType == NULL)
		return E_INVALIDARG;

	if(*SubType != MEDIASUBTYPE_RGB32)
	{
		return E_INVALIDARG;
	}

	VIDEOINFO *pvi = (VIDEOINFO *)pMediaType->Format();

	if(pvi == NULL)
		return E_INVALIDARG;

	if(pvi->bmiHeader.biWidth == 0 || pvi->bmiHeader.biHeight == 0 || pvi->AvgTimePerFrame == 0)
		return E_INVALIDARG;

	if(pvi->bmiHeader.biHeight < 0)
		return E_INVALIDARG;

	return S_OK;
}

HRESULT CPushPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);

	int bufnum;

	bufnum = (int)ceil(m_fFramerate * 0.3);
	if(bufnum < 4)
		bufnum = 4;

	CAutoLock cAutoLock(m_pFilter->pStateLock());

	HRESULT hr = NOERROR;

	VIDEOINFO *pvi = reinterpret_cast<VIDEOINFO*>(m_mt.Format());
	pProperties->cBuffers = bufnum;
	pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

	ASSERT(pProperties->cbBuffer);

	ALLOCATOR_PROPERTIES Actual;

	hr = pAlloc->SetProperties(pProperties, &Actual);
	if(FAILED(hr)) return hr;

	if(Actual.cbBuffer < pProperties->cbBuffer)
		return E_FAIL;

	ASSERT(Actual.cBuffers == bufnum);
	dprintf("SCFH DSF: Allocated buffer: %d", Actual.cBuffers);

	return NOERROR;
}

STDMETHODIMP CPushPin::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
	return E_NOTIMPL;
}

STDMETHODIMP CPushPin::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
	if (guidPropSet != AMPROPSETID_Pin)
		return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY)
		return E_PROP_ID_UNSUPPORTED;
	if (pPropData == NULL && pcbReturned == NULL)
		return E_POINTER;
	if (pcbReturned)
		*pcbReturned = sizeof(GUID);
	if (pPropData == NULL)			// 呼び出し元はサイズだけ知りたい。
		return S_OK;
	if (cbPropData < sizeof(GUID))	// バッファが小さすぎる。
		return E_UNEXPECTED;
	*(GUID *)pPropData = PIN_CATEGORY_CAPTURE;

	return S_OK;
}

STDMETHODIMP CPushPin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
	if (guidPropSet != AMPROPSETID_Pin)
		return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY)
		return E_PROP_ID_UNSUPPORTED;
	if (pTypeSupport)	// このプロパティの取得はサポートしているが、設定はサポートしていない。
		*pTypeSupport = KSPROPERTY_SUPPORT_GET; 

	return S_OK;
}

STDMETHODIMP CPushPin::Notify(IBaseFilter *pSelf, Quality q)
{
	return E_FAIL;
}

STDMETHODIMP CPushPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	return ::GetFormat(this, 0, ppmt);
}

STDMETHODIMP CPushPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
	return ::SetFormat(this, pmt);
}

STDMETHODIMP CPushPin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC)
{
	return ::GetStreamCaps(this, iIndex, ppmt, pSCC);
}

STDMETHODIMP CPushPin::GetNumberOfCapabilities(int *piCount, int *piSize)
{
	return ::GetNumberOfCapabilities(this, piCount, piSize);
}

// IAMLatency
STDMETHODIMP CPushPin::GetLatency(REFERENCE_TIME *prtLatency)
{
	CheckPointer(prtLatency, E_POINTER);

	if(m_fFramerate > 0.0)
		*prtLatency = (REFERENCE_TIME)(UNITS / m_fFramerate);
	else
		*prtLatency = 0;

	return S_OK;
}

// IAMPushSource
STDMETHODIMP CPushPin::GetPushSourceFlags(ULONG *pFlags)
{
	CheckPointer(pFlags, E_POINTER);
	*pFlags = 0;
	return S_OK;
}

STDMETHODIMP CPushPin::SetPushSourceFlags(ULONG Flags)
{
	return E_NOTIMPL;
}

STDMETHODIMP CPushPin::GetMaxStreamOffset(REFERENCE_TIME *prtMaxOffset)
{
	CheckPointer(prtMaxOffset, E_POINTER);
	*prtMaxOffset = UNITS * 60*60*24*365;
	return S_OK;
}

STDMETHODIMP CPushPin::SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset)
{
	return S_OK;
}

STDMETHODIMP CPushPin::GetStreamOffset(REFERENCE_TIME *prtOffset)
{
	CheckPointer(prtOffset, E_POINTER);
	*prtOffset = m_rtOffset;
	return S_OK;
}

STDMETHODIMP CPushPin::SetStreamOffset(REFERENCE_TIME rtOffset)
{
	dprintf("SCFH DSF SetStreamOffset(%llu)", rtOffset);
	m_rtOffset = rtOffset;
	return S_OK;
}



volatile LONG CPushSource::instanceNum = 0;

CPushSource::CPushSource(IUnknown *pUnk, HRESULT *phr):
	CSource(FILTER_NAME, pUnk, CLSID_SCFH, phr)
{
	LONG i;
	i = InterlockedIncrement(&instanceNum);
	if(i == 1)
	{
		DummyWindowThread.start();

		// ウィンドウができるまで待つ
		while(DummyWindowThread.isRunning() && !g.hwndDummy)
			Sleep(1);
	}

	{
		CMutexLock lock(&g.mutexGlobal);
		char sztemp[256], key[MAX_PATH];

		sprintf(key, "SOFTWARE\\SCFH DSF\\%s", g.szFilename);

		readRegistoryString(
			HKEY_CURRENT_USER,
			key,
			"Width",
			sztemp,
			256,
			"320"
			);
		if(!sscanf(sztemp, "%d", &m_iImageWidth))
			m_iImageWidth = 320;

		readRegistoryString(
			HKEY_CURRENT_USER,
			key,
			"Height",
			sztemp,
			256,
			"240"
			);
		if(!sscanf(sztemp, "%d", &m_iImageHeight))
			m_iImageHeight = 240;

		readRegistoryString(
			HKEY_CURRENT_USER,
			key,
			"Framerate",
			sztemp,
			256,
			"30.0"
			);
		if(!sscanf(sztemp, "%lf", &m_fFramerate))
			m_fFramerate = 30.0;
	}

	{
		CMutexLock lock(g.mutexInfo);
		g.info->width = m_iImageWidth;
		g.info->height = m_iImageHeight;
		g.info->framerate = m_fFramerate;
	}

	m_pPin = new CPushPin(phr, this, m_iImageWidth, m_iImageHeight, m_fFramerate);

	if(phr)
	{
		if (m_pPin == NULL)
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
}

CPushSource::~CPushSource()
{
	LONG i;

	saveSettings();

	delete m_pPin;

	i = InterlockedDecrement(&instanceNum);
	if(i == 0)
	{
		DummyWindowThread.wait(1000);
	}
}

bool CPushSource::saveSettings()
{
	CMutexLock lock(&g.mutexGlobal);
	char sztemp[256], key[MAX_PATH];

	sprintf(key, "SOFTWARE\\SCFH DSF\\%s", g.szFilename);

	sprintf(sztemp, "%d", m_iImageWidth);
	writeRegistoryString(
		HKEY_CURRENT_USER,
		key,
		"Width",
		sztemp
		);

	sprintf(sztemp, "%d", m_iImageHeight);
	writeRegistoryString(
		HKEY_CURRENT_USER,
		key,
		"Height",
		sztemp
		);

	sprintf(sztemp, "%lf", m_fFramerate);
	writeRegistoryString(
		HKEY_CURRENT_USER,
		key,
		"Framerate",
		sztemp
		);

	return true;
}

CUnknown* WINAPI CPushSource::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
	CPushSource *pNewFilter = new CPushSource(pUnk, phr);

	if (phr)
	{
		if (pNewFilter == NULL) 
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}

	return pNewFilter;
}

STDMETHODIMP CPushSource::GetPages(CAUUID *pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;

	pPages->pElems = reinterpret_cast<GUID*>(CoTaskMemAlloc(sizeof(GUID)));
	if(pPages->pElems == NULL) 
		return E_OUTOFMEMORY;

	pPages->pElems[0] = CLSID_SCFHProp;

	return S_OK;
}

STDMETHODIMP CPushSource::GetSize(SIZE *pSize)
{
	CMutexLock lock(&m_csShared);
	CheckPointer(pSize, E_POINTER);

	pSize->cx = m_iImageWidth;
	pSize->cy = m_iImageHeight;

	return S_OK;
}

STDMETHODIMP CPushSource::SetSize(SIZE *pSize)
{
	CMutexLock lock(&m_csShared);
	CheckPointer(pSize, E_POINTER);

	m_iImageWidth = pSize->cx;
	m_iImageHeight = pSize->cy;

	saveSettings();

	return S_OK;
}

STDMETHODIMP CPushSource::GetFramerate(double *pfFramerate)
{
	CMutexLock lock(&m_csShared);
	CheckPointer(pfFramerate, E_POINTER);

	*pfFramerate = m_fFramerate;

	return S_OK;
}

STDMETHODIMP CPushSource::SetFramerate(double fFramerate)
{
	CMutexLock lock(&m_csShared);

	m_fFramerate = fFramerate;

	saveSettings();

	return S_OK;
}

STDMETHODIMP CPushSource::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	return ::GetFormat(m_pPin, 0, ppmt);
}

STDMETHODIMP CPushSource::SetFormat(AM_MEDIA_TYPE *pmt)
{
	return ::SetFormat(m_pPin, pmt);
}

STDMETHODIMP CPushSource::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC)
{
	return ::GetStreamCaps(m_pPin, iIndex, ppmt, pSCC);
}

STDMETHODIMP CPushSource::GetNumberOfCapabilities(int *piCount, int *piSize)
{
	return ::GetNumberOfCapabilities(m_pPin, piCount, piSize);
}
