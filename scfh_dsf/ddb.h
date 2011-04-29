// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#pragma once

#ifndef DDBH
#define DDBH

#include <windows.h>

class CDDB
{
private:
	HDC _hdc;
	HWND _hwnd;
	HBITMAP _bitmap;
	int _width, _height;
	
public:
	CDDB();	
	~CDDB();
	
	bool create(HWND hwnd, int width, int height);	
	void release();
	
	HDC getDC(){ return _hdc; }
	HBITMAP getBitmap(){ return _bitmap; }
	HWND getHWND(){ return _hwnd; }
	int getWidth(){ return _width; }
	int getHeight(){ return _height; }
    int getLineSize(int bits);
    int getImageSize(int bits);
	bool getImage(void *pixels, int bits, bool invert, BITMAPINFO *bi, int width=0, int height=0);
};

#endif

