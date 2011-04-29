// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#pragma once

#ifndef RESIZEH
#define RESIZEH

#include <windows.h>
#include "common.h"
#include "event.h"
#include "tools.h"
#include "stools.h"

struct RGBX
{
	BYTE b, g, r, x;
};

enum RESIZE_MODE
{
	MODE_LOWQUALITY = 0,
	MODE_HIGHQUALITY,
};

class CResize
{
protected:
	RESIZE_MODE mode;

public:
	CResize(){}
	virtual ~CResize(){}

	void setMode(RESIZE_MODE _mode)
	{
		mode = _mode;
	}

	virtual void clear() = 0;
	virtual bool check(bool forceRecreate) = 0;
	virtual bool resize(HDC hdcDest, RECT *rectDest, HWND hwndSrc, RECT *rectSrc,
		bool zoom, bool fixAspect, bool showCursor, bool showLW) = 0;

	bool batchResize(HDC hdcDest, AreaInfo *info, int len, int width, int height);
};

struct ResizeInfo
{
	int dst_x, dst_y, dst_width, dst_height;
	int src_x, src_y, src_width, src_height;
	int off_x, off_y;
	float amp;
};

void calcResizeInfo(struct ResizeInfo *info, RECT *rectDest, RECT *rectSrc, SIZE *limDest, SIZE *limSrc, bool zoom, bool fixAspect);
void setBilinearPass(struct ResizeInfo *info, RECT *rectDst, RECT *rectSrc, bool twoPass);

class CResizeThread : public CThread
{
protected:
	void onWait();

public:
	int resizeMode;
	ResizeInfo rinfo;
	DWORD *src, *dst;
	DWORD *sten;
	DWORD sampleWidth, sampleHeight;
	int writecount;

	DWORD lineStart, lineEnd;
	CEvent eventStart, eventEnd;

	CResizeThread();
	~CResizeThread();

	void resize();
	unsigned int run();
};

#endif
