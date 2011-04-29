// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#pragma once

#ifndef RESIZEDDH
#define RESIZEDDH

#include "resize.h"

CResize *CreateInstanceResizeDD();

bool InitResizeDD(HWND hwnd);
void UninitResizeDD();

#endif
