// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include <windows.h>

#include "sharedmem.h"
#include "tools.h"

CSharedMemory::CSharedMemory()
{
	error = 0;
	map = NULL;
	data = NULL;
	length = 0;
}

CSharedMemory::~CSharedMemory()
{
	release();
}

bool CSharedMemory::create(const char *name, int len)
{
	release();

	map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, len, name);
	if(!map)
	{
		dprintf("CreateFileMapping() failed. (%s)", name);
		return false;
	}
	error = GetLastError();
	
	data = (char *)MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if(!data)
	{
		dprintf("MapViewOfFile() failed. (%s)", name);

		CloseHandle(map);
		map = NULL;
		error = 0;
		return false;
	}

	length = len;

	if(data != NULL && error != ERROR_ALREADY_EXISTS)
	{	// ç≈èâÇ…ã§óLÉÅÉÇÉäÇämï€ÇµÇΩ
		memset(data, 0, len);
	}

//	dprintf("Created shared mem: %s size: %d", name, len);

	return true;
}

void CSharedMemory::release()
{
	if(data != NULL)
		UnmapViewOfFile(data);

	if(map != NULL)
		CloseHandle(map);

	error = 0;
	map = NULL;
	data = NULL;
	length = 0;
}
