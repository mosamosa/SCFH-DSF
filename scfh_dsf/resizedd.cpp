// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include <ddraw.h>
#include <math.h>

#include "resize.h"
#include "resizedd.h"
#include "tools.h"

typedef HRESULT (WINAPI *typeDirectDrawCreate)(GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *);

HRESULT RecreateSurface(LPDIRECTDRAW lpDD, LPDIRECTDRAWSURFACE *lplpDDS, int width, int height)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;

	RELEASE(*lplpDDS);

	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY; // | DDSCAPS_LOCALVIDMEM;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;

	hr = lpDD->CreateSurface(&ddsd, lplpDDS, NULL);
	if(SUCCEEDED(hr))
	{
		HDC hdc = NULL;

		(*lplpDDS)->GetDC(&hdc);	// 一回ロックしておかないとなぜか重くなる
		(*lplpDDS)->ReleaseDC(hdc);

		return hr;
	}
	else
	{
		*lplpDDS = NULL;
		return hr;
	}
}

#define RECTNUM 2

class CResizeDD : public CResize
{
private:
	static IDirectDraw *dd;
	IDirectDrawSurface *ddsWork1, *ddsWork2;
	int workWidth, workHeight;

	CResizeDD() : CResize()
	{
		ddsWork1 = NULL;
		ddsWork2 = NULL;
		workWidth = 0;
		workHeight = 0;
	}

	~CResizeDD()
	{
		clear();
	}

public:
	static bool init(HWND hwnd)
	{
		HRESULT hr;
		HMODULE hDll;
		typeDirectDrawCreate pDirectDrawCreate;

		hDll = LoadLibrary(L"ddraw.dll");
		if(!hDll)
		{
			dprintf("ddraw.dllが存在しません.");

			return false;
		}

		pDirectDrawCreate = (typeDirectDrawCreate)GetProcAddress(hDll, "DirectDrawCreate");
		if(!pDirectDrawCreate)
		{
			dprintf("DirectDrawCreateが存在しません.");

			return false;
		}

		hr = pDirectDrawCreate(NULL, &dd, NULL);
		if(FAILED(hr))
		{
			dprintf("DirectDrawの初期化に失敗しました. (%08x)", hr);

			return false;
		}

		hr = dd->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
		if(FAILED(hr))
		{
			dprintf("SetCooperativeLevel() failed. (%08x)", hr);

			RELEASE(dd);
			return false;
		}

		return true;
	}

	static void uninit()
	{
		RELEASE(dd);
	}

	static CResize *CreateInstance()
	{
		return (CResize *)(new CResizeDD());
	}

	void clear()
	{
		RELEASE(ddsWork1);
		RELEASE(ddsWork2);

		workWidth = 0;
		workHeight = 0;
	}

	bool check(bool forceRecreate)
	{
		HRESULT hr;

		if(dd == NULL)
			return false;

		int deskWidth = GetSystemMetrics(SM_CXSCREEN);
		int deskHeight = GetSystemMetrics(SM_CYSCREEN);

		if(	!ddsWork1 || !ddsWork2 ||
			deskWidth != workWidth || deskHeight != workHeight ||
			forceRecreate)
		{
			hr = RecreateSurface(dd, &ddsWork1, deskWidth, deskHeight);
			if(FAILED(hr))
			{
				dprintf("サーフェイスの作成に失敗しました. (1) (%08x)", hr);
	
				return false;
			}

			hr = RecreateSurface(dd, &ddsWork2, deskWidth, deskHeight);
			if(FAILED(hr))
			{
				dprintf("サーフェイスの作成に失敗しました. (2) (%08x)", hr);
	
				RELEASE(ddsWork1);
				return false;
			}
		}

		workWidth = deskWidth;
		workHeight = deskHeight;

		return true;
	}

	bool resize(HDC hdcDest, RECT *rectDest, HWND hwndSrc, RECT *rectSrc,
		bool zoom, bool fixAspect, bool showCursor, bool showLW)
	{
		struct ResizeInfo info;
		int cursor_x, cursor_y;
		DWORD dwMyROP;

		if(!dd || !ddsWork1 || !ddsWork2)
			return false;

		if(showLW)
			dwMyROP = SRCCOPY | CAPTUREBLT;
		else
			dwMyROP = SRCCOPY;

		SIZE size = {workWidth, workHeight};

		calcResizeInfo(&info, rectDest, rectSrc, &size, &size, zoom, fixAspect);

		if(info.src_width <= 0 || info.src_height <= 0)
			return true;

		if(info.dst_width <= 0 || info.dst_height <= 0)
			return true;

		{	// カーソル位置計算
			POINT pos;

			GetCursorPos(&pos);
			ScreenToClient(hwndSrc, &pos);

			cursor_x = pos.x - info.src_x;
			cursor_y = pos.y - info.src_y;
		}

		if(info.amp != 1.0f)
		{	// 拡大・縮小処理
			HRESULT hr;
			bool succeeded = false;

			do
			{
				HDC hdcPri, hdcSec;
				IDirectDrawSurface *ddsPri, *ddsSec;

				ddsPri = ddsWork1;
				ddsSec = ddsWork2;

				if(ddsPri->IsLost() == DDERR_SURFACELOST)
				{
					hr = ddsPri->Restore();
					if(FAILED(hr)) break;
				}

				if(ddsSec->IsLost() == DDERR_SURFACELOST)
				{
					hr = ddsSec->Restore();
					if(FAILED(hr)) break;
				}

				{
					hr = ddsPri->GetDC(&hdcPri);
					if(FAILED(hr)) break;

					RECT rect;
					SetRect(&rect, 0, 0, info.src_width, info.src_height);
					FillRect(hdcPri, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

					HDC hdcSrc = GetDC(hwndSrc);

					BitBlt(hdcPri, 0, 0, info.src_width, info.src_height, hdcSrc, info.src_x, info.src_y, dwMyROP);

					ReleaseDC(hwndSrc, hdcSrc);

					if(showCursor)
						DrawCursor(hdcPri, cursor_x, cursor_y);

					ddsPri->ReleaseDC(hdcPri);
				}

				{
					int i;
					RECT rectSrc[RECTNUM], rectDst[RECTNUM];

					memset(rectSrc, 0, sizeof(RECT)*RECTNUM);
					memset(rectDst, 0, sizeof(RECT)*RECTNUM);

					setBilinearPass(&info, rectDst, rectSrc, mode == MODE_HIGHQUALITY);

					for(i=0; i<RECTNUM; i++)
					{
						if(rectSrc[i].right == 0 || rectSrc[i].bottom == 0)
							break;

						if(i & 1)
						{
							ddsPri = ddsWork2;
							ddsSec = ddsWork1;
						}
						else
						{
							ddsPri = ddsWork1;
							ddsSec = ddsWork2;
						}

						hr = ddsSec->Blt(&rectDst[i], ddsPri, &rectSrc[i], 0, NULL);
						if(FAILED(hr)) break;
					}

					if(FAILED(hr))
						break;
				}

				hr = ddsSec->GetDC(&hdcSec);
				if(FAILED(hr)) break;

				BitBlt(hdcDest, info.dst_x, info.dst_y, info.dst_width, info.dst_height, hdcSec, 0, 0, SRCCOPY);

				ddsSec->ReleaseDC(hdcSec);

				succeeded = true;
			}
			while(false);
			
			if(!succeeded)
			{
				dprintf("縮小処理中にエラーが発生しました. (DD) (%08x)", hr);
				return false;
			}
		}
		else
		{	// 等倍処理
			HDC hdcSrc = GetDC(hwndSrc);

			BitBlt(hdcDest, info.dst_x, info.dst_y, info.dst_width, info.dst_height, hdcSrc, info.src_x, info.src_y, dwMyROP);

			if(showCursor)
			{
				if(
					cursor_x >= 0 && cursor_y >= 0 &&
					cursor_x < info.src_width && cursor_y < info.src_height)
				{
					DrawCursor(
						hdcDest,
						cursor_x + info.off_x + rectDest->left,
						cursor_y + info.off_y + rectDest->top
						);
				}
			}

			ReleaseDC(hwndSrc, hdcSrc);
		}

		return true;
	}
};

IDirectDraw* CResizeDD::dd = NULL;

CResize *CreateInstanceResizeDD()
{
	return CResizeDD::CreateInstance();
}

bool InitResizeDD(HWND hwnd)
{
	return CResizeDD::init(hwnd);
}

void UninitResizeDD()
{
	CResizeDD::uninit();
}
