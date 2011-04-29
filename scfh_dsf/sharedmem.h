// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#ifndef SHAREDMEMH
#define SHAREDMEMH

class CSharedMemory
{
private:
	DWORD error;
	HANDLE map;
	char *data;
	int length;

public:
	CSharedMemory();
	~CSharedMemory();

	bool create(const char *name, int len);
	void release();

	char *getData(){ return data; }
	bool isAlreadyExists(){ return (error != ERROR_ALREADY_EXISTS); }
	int getLength(){ return length; }
};

#endif
