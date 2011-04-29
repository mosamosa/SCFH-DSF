// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include <windows.h>

bool readRegistory(HKEY hKeyRoot, const char *szKey, const char *szName, BYTE *pBuf, DWORD *pdwSize, DWORD *pdwType)
{
	int ret;
	HKEY hKey;

	ret = RegOpenKeyExA(
		hKeyRoot,
		szKey,
		0,
		KEY_READ,
		&hKey
		);

	if(ret != ERROR_SUCCESS)
		return false;

	ret = RegQueryValueExA(
		hKey,
		szName,
		NULL,
		pdwType,
		pBuf,
		pdwSize
		);

	RegCloseKey(hKey);

	if(ret != ERROR_SUCCESS)
		return false;

	return true;
}

void readRegistoryString(HKEY hKeyRoot, const char *szKey, const char *szName, char *szBuf, DWORD dwSize, const char *szDefault)
{
	DWORD type;

	if(readRegistory(hKeyRoot, szKey, szName, (BYTE *)szBuf, &dwSize, &type))
	{
		if(type == REG_SZ)
			return;
	}

	strcpy(szBuf, szDefault);
}

bool writeRegistory(HKEY hKeyRoot, const char *szKey, const char *szName, const BYTE *pBuf, DWORD dwSize, DWORD dwType)
{
	int ret;
	HKEY hKey;

	ret = RegCreateKeyExA(
		hKeyRoot,
		szKey,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&hKey,
		NULL
		);

	if(ret != ERROR_SUCCESS)
		return false;

	ret = RegSetValueExA(
		hKey,
		szName,
		0,
		dwType,
		pBuf,
		dwSize
		);

	RegCloseKey(hKey);

	if(ret != ERROR_SUCCESS)
		return false;

	return true;
}

bool writeRegistoryString(HKEY hKeyRoot, const char *szKey, const char *szName, const char *szBuf)
{
	return writeRegistory(hKeyRoot, szKey, szName, (BYTE *)szBuf, (DWORD)strlen(szBuf)+1, REG_SZ);
}
