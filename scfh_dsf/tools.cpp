// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "tools.h"

#define BUFLEN 512

int dprintf(const char *format, ...)
{
	va_list ap;
	char buffer[BUFLEN];
	int iret;

	va_start(ap, format);
	iret = vsprintf(buffer, format, ap);
	va_end(ap);

    OutputDebugStringA(buffer);

	return iret;
}

void dprint(const char *str)
{
    OutputDebugStringA(str);
}

double pc2time(__int64 t)
{
	static __int64 freq = 1;

	if(freq == 1)
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

	if(freq > 0)
		return ((double)t / (double)freq);
	else
		return 0.0;
}

double round(double n)
{
	int i;
	bool plus = n > 0.0;

	n = fabs(n);
	i = (int)n;
	n -= (double)i;

	if(n >= 0.5)
		return plus ? (double)(i+1) : -(double)(i+1);
	else
		return plus ? (double)i : -(double)i;
}

void DrawCursor(HDC hdcDest, int x, int y)
{
	CURSORINFO ci;
	ICONINFO ii;

	memset(&ci, 0, sizeof(ci));
	ci.cbSize = sizeof(ci);
	ci.flags = CURSOR_SHOWING;

	if(GetCursorInfo(&ci) && GetIconInfo(ci.hCursor, &ii))
	{
		DrawIcon(hdcDest, x-ii.xHotspot, y-ii.yHotspot, ci.hCursor);

		DeleteObject(ii.hbmColor);
		DeleteObject(ii.hbmMask);
	}
}

void FixRect(RECT *rect, int left, int top, int right, int bottom)
{
	if(rect->left < left)
		rect->left = left;
	if(rect->left > right)
		rect->left = right;

	if(rect->top < top)
		rect->top = top;
	if(rect->top > bottom)
		rect->top = bottom;

	if(rect->right < left)
		rect->right = left;
	if(rect->right > right)
		rect->right = right;

	if(rect->bottom < top)
		rect->bottom = top;
	if(rect->bottom > bottom)
		rect->bottom = bottom;

	if(rect->left > rect->right)
		rect->left = rect->right;
	if(rect->top > rect->bottom)
		rect->top = rect->bottom;
}