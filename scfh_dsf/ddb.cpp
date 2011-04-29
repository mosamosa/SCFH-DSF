// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include "ddb.h"

#include <stdio.h>

CDDB::CDDB()
{
	_hdc = NULL;
	_hwnd = NULL;
	_bitmap = NULL;
	_width = _height = 0;
}

CDDB::~CDDB()
{
	release();
}

bool CDDB::create(HWND hwnd, int width, int height)
{
	HDC hdcsrc;
	BITMAPINFOHEADER ih;
	bool succeed = false;
	
	release();
	
	ih.biSize = sizeof(BITMAPINFOHEADER);
	ih.biWidth = width;
	ih.biHeight = height;
	ih.biPlanes = 1;
	ih.biBitCount = 32;
	ih.biCompression = BI_RGB;
	ih.biSizeImage = width * height * ih.biBitCount / 8;
	ih.biXPelsPerMeter = 0;
	ih.biYPelsPerMeter = 0;
	ih.biClrUsed = 0;
	ih.biClrImportant = 0;
	
	do
	{
		hdcsrc = GetDC(hwnd);
		if(!hdcsrc)
			break;
		
		_hdc = CreateCompatibleDC(hdcsrc);
		if(!_hdc)
			break;
		
		_bitmap = CreateDIBitmap(hdcsrc, &ih, 0, 0, 0, DIB_RGB_COLORS);
		if(!_bitmap)
			break;

		SelectObject(_hdc, _bitmap);
		
		succeed = true;
		
	}while(false);
	
	if(hdcsrc)
		ReleaseDC(hwnd, hdcsrc);
	
	if(succeed)
	{
		_width = width;
		_height = height;
		_hwnd = hwnd;
	}
	else
	{
		release();
	}
	
	return succeed;
}

void CDDB::release()
{
	if(_bitmap)
		DeleteObject(_bitmap);
	if(_hdc)
		DeleteDC(_hdc);

	_hdc = NULL;
	_hwnd = NULL;
	_bitmap = NULL;
	_width = _height = 0;
}

#define BYTES_ALIGN 4

int CDDB::getLineSize(int bits)
{
	int align;

	if(bits % 8)
		return 0;
	if(bits < 8 || bits > 32)
		return 0;

	align = _width * bits / 8;
	align = align % BYTES_ALIGN;
	if(align > 0)
		align = BYTES_ALIGN - align;

	return ((_width * bits / 8) + align);
}

int CDDB::getImageSize(int bits)
{
	return (getLineSize(bits) * _height);
}

bool CDDB::getImage(void *pixels, int bits, bool invert, BITMAPINFO *bi, int width, int height)
{
	BITMAPINFO info;

	if(!_hdc)
		return false;
	if(bits % 8)
		return false;
	if(bits < 8 || bits > 32)
		return false;

	ZeroMemory(&info, sizeof(BITMAPINFO));
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	if(width > 0 && height > 0)
	{
		info.bmiHeader.biWidth = width;
		info.bmiHeader.biHeight = invert ? height : -height;
	}
	else
	{
		info.bmiHeader.biWidth = _width;
		info.bmiHeader.biHeight = invert ? _height : -_height;
	}
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = bits;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biSizeImage = getImageSize(bits);
	info.bmiHeader.biXPelsPerMeter = 0;
	info.bmiHeader.biYPelsPerMeter = 0;
	info.bmiHeader.biClrUsed = 0;
	info.bmiHeader.biClrImportant = 0;

    if(info.bmiHeader.biSizeImage == 0)
    	return false;

	if(width > 0 && height > 0)
	{
		if(GetDIBits(_hdc, _bitmap, 0, height, pixels, &info, DIB_RGB_COLORS) == 0)
			return false;
	}
	else
	{
		if(GetDIBits(_hdc, _bitmap, 0, _height, pixels, &info, DIB_RGB_COLORS) == 0)
			return false;
	}

    if(bi)
    	memcpy(bi, &info, sizeof(BITMAPINFO));

	return true;
}

