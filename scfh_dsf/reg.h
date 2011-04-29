// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#ifndef REGH
#define REGH

#include <windows.h>

bool readRegistory(HKEY hKeyRoot, const char *szKey, const char *szName, BYTE *pBuf, DWORD *pdwSize, DWORD *pdwType);
void readRegistoryString(HKEY hKeyRoot, const char *szKey, const char *szName, char *szBuf, DWORD dwSize, const char *szDefault);
bool writeRegistory(HKEY hKeyRoot, const char *szKey, const char *szName, const BYTE *pBuf, DWORD dwSize, DWORD dwType);
bool writeRegistoryString(HKEY hKeyRoot, const char *szKey, const char *szName, const char *szBuf);

#endif
