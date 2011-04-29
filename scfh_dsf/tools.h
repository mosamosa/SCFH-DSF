// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#pragma once
#pragma warning(disable: 4312 4311)

#ifndef TOOLSH
#define TOOLSH

#include <windows.h>
#include <process.h>
#include <string>
#include <assert.h>

#ifndef WIN64
typedef unsigned __int64 QWORD;
typedef DWORD IPTR;
#else
typedef unsigned __int64 QWORD;
typedef QWORD IPTR;
#endif

#define RELEASE(p) if(p){ (p)->Release(); (p) = NULL; }
#define ARRAY_SIZEOF(arr) (sizeof(arr)/sizeof((arr)[0]))

int dprintf(const char *format, ...);
void dprint(const char *str);
double pc2time(__int64 t);
double round(double n);
void *memcpy2(void *dst, const void *src, IPTR count);

void DrawCursor(HDC hdcDest, int x, int y);
void FixRect(RECT *rect, int left, int top, int right, int bottom);

template<typename T>
class CAlignedArray
{
private:
	char *org;
	T *p;
	IPTR len;

public:
	CAlignedArray()
	{
		org = NULL;
		p = NULL;
		len = 0;
	}

	~CAlignedArray()
	{
		remove();
	}

	void remove()
	{
		if(org)
		{
			delete [] org;
			org = NULL;
		}
		p = NULL;
		len = 0;
	}

	void create(IPTR _len)
	{
		if(len != _len)
		{
			IPTR size;
			IPTR aligned;

			remove();
	
			size = _len*sizeof(T) + 16-1;
			org = new char[size];

			aligned = reinterpret_cast<IPTR>(org);
			aligned += 16-1;
			aligned -= aligned & (16-1);

			p = reinterpret_cast<T*>(aligned);
			len = _len;
		}
	}

	void clear()
	{
		if(p)
			memset(p, 0, len*sizeof(T));
	}

	IPTR length()
	{
		return len;
	}

	T* ptr()
	{
		return p;
	}

	operator T*()
	{
		return p;
	}
};

#endif

