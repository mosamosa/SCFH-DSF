// SCFH DSF, a DirectShow source filter for screencast.
// Copyright (C) 2009 mosamosa (pcyp4g@gmail.com)
// 
// Released under GNU Lesser General Public License Version  3.
// Read the file 'COPYING' for more information.

#include "resize.h"
#include "tools.h"

#include <math.h>
#include <process.h>
#include <intrin.h>
#include <mmintrin.h>
#include <emmintrin.h>

bool CResize::batchResize(HDC hdcDest, AreaInfo *info, int len, int width, int height)
{
	int i;
	RECT rect;

	SetRect(&rect, 0, 0, width, height);
	FillRect(hdcDest, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	for(i=len-1; i>=0; i--)
	{
		HWND hwnd;

		if(!info[i].active)
			continue;

		hwnd = (HWND)info[i].hwnd;
		if(hwnd == NULL)
			hwnd = GetDesktopWindow();

		rect.left = (LONG)floor(info[i].dst.left * width);
		rect.top = (LONG)floor((info[i].dst.top * height));
		rect.right = (LONG)ceil((info[i].dst.right-info[i].dst.left) * width) + rect.left;
		rect.bottom = (LONG)ceil((info[i].dst.bottom-info[i].dst.top) * height) + rect.top;

		resize(hdcDest, &rect, hwnd, &info[i].src,
			info[i].zoom, info[i].fixAspect, info[i].showCursor, info[i].showLW);
	}

	return true;
}

void calcResizeInfo(struct ResizeInfo *info, RECT *rectDest, RECT *rectSrc, SIZE *limDest, SIZE *limSrc, bool zoom, bool fixAspect)
{
	int orgd_width, orgd_height;

	memset(info, 0, sizeof(ResizeInfo));

	info->src_x = rectSrc->left;
	info->src_y = rectSrc->top;
	info->src_width = rectSrc->right - rectSrc->left;
	info->src_height = rectSrc->bottom - rectSrc->top;

	info->dst_x = rectDest->left;
	info->dst_y = rectDest->top;
	info->dst_width = rectDest->right - rectDest->left;
	info->dst_height = rectDest->bottom - rectDest->top;

	if(limSrc)
	{
		if(info->src_width > limSrc->cx)
			info->src_width = limSrc->cx;
		if(info->src_height > limSrc->cy)
			info->src_height = limSrc->cy;
	}

	if(limDest)
	{
		if(info->dst_width > limDest->cx)
			info->dst_width = limDest->cx;
		if(info->dst_height > limDest->cy)
			info->dst_height = limDest->cy;
	}

	if(info->src_width <= 0)
		info->src_width = 0;
	if(info->src_height <= 0)
		info->src_height = 0;
	
	if(info->dst_width <= 0)
		info->dst_width = 0;
	if(info->dst_height <= 0)
		info->dst_height = 0;

	if(info->src_width <= 0 || info->src_height <= 0)
	{
		memset(info, 0, sizeof(ResizeInfo));
		return;
	}

	if(info->dst_width <= 0 || info->dst_height <= 0)
	{
		memset(info, 0, sizeof(ResizeInfo));
		return;
	}

	orgd_width = info->dst_width;
	orgd_height = info->dst_height;

	// 描画位置計算
	if(!fixAspect)
	{
		if(orgd_width != info->src_width || orgd_height != info->src_height)
			info->amp = 1.5f;	// 適当 1.0f 以外
		else
			info->amp = 1.0f;
	}
	else
	{	// アスペクト比維持
		if(zoom || orgd_width<info->src_width || orgd_height<info->src_height)
		{
			float aspect1, aspect2;

			aspect1 = (float)info->src_width / info->src_height;
			aspect2 = (float)orgd_width / orgd_height;

			if(aspect1 > aspect2)
			{
				info->dst_height = (int)ceil(orgd_width / aspect1);
				info->off_y = (orgd_height - info->dst_height) / 2;
				info->dst_y += info->off_y;

				info->amp = (float)orgd_width / info->src_width; 
			}
			else
			{
				info->dst_width  = (int)ceil(orgd_height * aspect1);
				info->off_x = (orgd_width - info->dst_width) / 2;
				info->dst_x += info->off_x;

				info->amp = (float)orgd_height / info->src_height;
			}
		}
		else
		{
			info->dst_width = info->src_width;
			info->dst_height = info->src_height;
			info->off_x = (orgd_width - info->dst_width) / 2;
			info->off_y = (orgd_height - info->dst_height) / 2;
			info->dst_x += info->off_x;
			info->dst_y += info->off_y;

			info->amp = 1.0f;
		}
	}
}


void setBilinearPass(struct ResizeInfo *info, RECT *rectDst, RECT *rectSrc, bool twoPass)
{
	if(twoPass)
	{
		if(info->amp >= 0.5625 && info->amp < 0.75)
		{
			rectSrc[0].right = info->src_width;
			rectSrc[0].bottom = info->src_height;

			rectDst[0].right = (LONG)(info->src_width * sqrt(info->amp));
			rectDst[0].bottom = (LONG)(info->src_height * sqrt(info->amp));

			rectSrc[1] = rectDst[0];

			rectDst[1].right = info->dst_width;
			rectDst[1].bottom = info->dst_height;
		}
		else if(info->amp > 0.5 && info->amp < 0.5625)
		{
			rectSrc[0].right = info->src_width;
			rectSrc[0].bottom = info->src_height;

			rectDst[0].right = (LONG)(info->src_width * (info->amp / 0.75));
			rectDst[0].bottom = (LONG)(info->src_height * (info->amp / 0.75));

			rectSrc[1] = rectDst[0];

			rectDst[1].right = info->dst_width;
			rectDst[1].bottom = info->dst_height;
		}
		else if(info->amp > 0.45 && info->amp < 0.5)
		{
			rectSrc[0].right = info->src_width;
			rectSrc[0].bottom = info->src_height;

			rectDst[0].right = (LONG)(info->src_width * (info->amp / 0.5));
			rectDst[0].bottom = (LONG)(info->src_height * (info->amp / 0.5));

			rectSrc[1] = rectDst[0];

			rectDst[1].right = info->dst_width;
			rectDst[1].bottom = info->dst_height;
		}
		else if(info->amp >= 0.25 && info->amp <= 0.45)
		{
			rectSrc[0].right = info->src_width;
			rectSrc[0].bottom = info->src_height;

			rectDst[0].right = (LONG)(info->src_width * 0.5);
			rectDst[0].bottom = (LONG)(info->src_height * 0.5);

			rectSrc[1] = rectDst[0];

			rectDst[1].right = info->dst_width;
			rectDst[1].bottom = info->dst_height;
		}
		else if(info->amp < 0.25)
		{
			rectSrc[0].right = info->src_width;
			rectSrc[0].bottom = info->src_height;

			rectDst[0].right = (LONG)(info->src_width * (info->amp / 0.5));
			rectDst[0].bottom = (LONG)(info->src_height * (info->amp / 0.5));

			rectSrc[1] = rectDst[0];

			rectDst[1].right = info->dst_width;
			rectDst[1].bottom = info->dst_height;
		}
		else
		{
			rectSrc[0].right = info->src_width;
			rectSrc[0].bottom = info->src_height;

			rectDst[0].right = info->dst_width;
			rectDst[0].bottom = info->dst_height;
		}
	}
	else
	{
		rectSrc[0].right = info->src_width;
		rectSrc[0].bottom = info->src_height;

		rectDst[0].right = info->dst_width;
		rectDst[0].bottom = info->dst_height;
	}
}

CResizeThread::CResizeThread() :
	CThread("CResizeThread"),
	eventStart(false, false, NULL),
	eventEnd(false, false, NULL)
{
}

CResizeThread::~CResizeThread()
{
	wait();
}

void CResizeThread::onWait()
{
	eventStart.set();
	eventEnd.set();
}

unsigned int CResizeThread::run()
{
	while(!terminated())
	{
		eventStart.wait();

		if(terminated())
			return 0;

		resize();

		eventEnd.set();
	}

	return 0;
}

#pragma pack(push, 1)

typedef struct
{
	BYTE b, g, r, a;
}RGB32;

typedef struct
{
	RGB32 c[2];
}RGB32_2;

#pragma pack(pop)

void CResizeThread::resize()
{
	DWORD mask;

	writecount = 0;

	if(resizeMode == RESIZE_MODE_SW_NEAREST)
	{	// ニアレストネイバー
		DWORD ix, iy, xx, yy;
		DWORD x0, y0;
		DWORD stenpos, dstpos;

		DWORD wfac = (rinfo.src_width<<16) / rinfo.dst_width;
		DWORD hfac = (rinfo.src_height<<16) / rinfo.dst_height;

		yy = hfac*lineStart;
		for(iy=lineStart; iy<lineEnd; ++iy)
		{
			dstpos = (sampleHeight-(iy+rinfo.dst_y)-1)*sampleWidth + rinfo.dst_x;

			xx = 0;
			for(ix=0; ix<(DWORD)rinfo.dst_width; ++ix)
			{
				stenpos = dstpos >> 5;
				mask = 1 << (dstpos&0x1f);

				if(!(sten[stenpos] & mask))
				{
					sten[stenpos] |= mask;
					writecount++;

					x0 = xx >> 16;
					y0 = yy >> 16;

					dst[dstpos] = src[y0*rinfo.src_width + x0];
				}

				dstpos++;
				xx += wfac;
			}

			yy += hfac;
		}
	}
	else if(resizeMode == RESIZE_MODE_SW_BILINEAR)
	{	// バイリニア
		DWORD coefx00, coefx01, coefy00, coefy01;
		DWORD ix, iy;
		DWORD xx, yy;
		DWORD stenpos, dstpos;
		DWORD *pix00, *pix01;

		DWORD wfac = (rinfo.src_width*0xffff) / rinfo.dst_width;
		DWORD hfac = (rinfo.src_height*0xffff) / rinfo.dst_height;

		DWORD *dstx = dst;
		DWORD *srcx = src;

		DWORD edgey, zoom;
		DWORD offsetx = 0, offsety = 0;

		zoom = rinfo.dst_width/rinfo.src_width >= 1 || rinfo.dst_height/rinfo.src_height >= 1;

		if(rinfo.dst_width / rinfo.src_width < 1)
			offsetx = (DWORD)(0x00010000 * (1.0 - (double)rinfo.dst_width/rinfo.src_width));

		if(rinfo.dst_height / rinfo.src_height < 1)
			offsety = (DWORD)(0x00010000 * (1.0 - (double)rinfo.dst_height/rinfo.src_height));

		yy = offsety + hfac*lineStart;
		for(iy=lineStart; iy<lineEnd; ++iy)
		{
			dstpos = (sampleHeight-1-(iy+rinfo.dst_y))*sampleWidth + rinfo.dst_x;

			edgey = iy >= (DWORD)rinfo.dst_height-1;

			xx = offsetx;
			for(ix=0; ix<(DWORD)rinfo.dst_width; ++ix)
			{
				stenpos = dstpos >> 5;
				mask = 1 << (dstpos&0x1f);

				if(!(sten[stenpos] & mask))
				{
					sten[stenpos] |= mask;
					writecount++;

					coefx01 = (xx>>8) & 0x00ff;
					coefx00 = 0x0100 - coefx01;
					coefy01 = (yy>>8) & 0x00ff;
					coefy00 = 0x0100 - coefy01;

					if(zoom)
					{
						if(ix >= (DWORD)rinfo.dst_width-1)
							pix00 = srcx + ((yy>>16) * rinfo.src_width + (xx>>16)) - 1;
						else
							pix00 = srcx + ((yy>>16) * rinfo.src_width + (xx>>16));

						if(edgey)
							pix01 = pix00;
						else
							pix01 = pix00 + rinfo.src_width;
					}
					else
					{
						pix00 = srcx + ((yy>>16) * rinfo.src_width + (xx>>16));
						pix01 = pix00 + rinfo.src_width;
					}
#ifndef WIN64
					_asm
					{
						pxor mm7, mm7;

						mov eax, pix00;
						movq mm0, [eax];
						movq mm2, mm0;
						psrlq mm2, 32;

						mov eax, pix01;
						movq mm1, [eax];
						movq mm3, mm1;
						psrl mm3, 32;

						punpcklbw mm0, mm7;		// BYTE->WORD
						punpcklbw mm1, mm7;
						punpcklbw mm2, mm7;
						punpcklbw mm3, mm7;

						movd mm4, coefx00;		// BYTE->WORDx4
						punpcklwd mm4, mm4;
						punpcklwd mm4, mm4;

						pmullw mm0, mm4;		// d00*coefx00

						movd mm5, coefx01;
						punpcklwd mm5, mm5;
						punpcklwd mm5, mm5;

						pmullw mm1, mm4;		// d01*coefx00

						movd mm6, coefy00;
						punpcklwd mm6, mm6;
						punpcklwd mm6, mm6;

						pmullw mm2, mm5;		// d10*coefx01

						movd mm7, coefy01;
						punpcklwd mm7, mm7;
						punpcklwd mm7, mm7;

						pmullw mm3, mm5;		// d11*coefx01

						paddusw mm0, mm2;		// c1 = d00*coefx00 + d10*coefx01
						psrlw mm0, 8;

						pmullw mm0, mm6;		// c1*coefy00

						paddusw mm1, mm3;		// c2 = d01*coefx00 + d11*coefx01
						psrlw mm1, 8;

						pmullw mm1, mm7;		// c2*coefy01

						paddusw mm0, mm1;		// c1*coefy00 + c2*coefy01
						psrlw mm0, 8;

						packuswb mm0, mm0;

						mov eax, dstpos;
						shl eax, 2;
						add eax, dstx;
						movd [eax], mm0;
					}
#else
					RGB32_2 *c00 = (RGB32_2*)pix00;
					RGB32_2 *c10 = (RGB32_2*)pix01;
					RGB32 *cd = (RGB32*)(dstx + dstpos);
					DWORD d0, d1;

					d0 = c00->c[0].b*coefx00 + c00->c[1].b*coefx01;
					d1 = c10->c[0].b*coefx00 + c10->c[1].b*coefx01;
					cd->b = (BYTE)((d0*coefy00 + d1*coefy01) >> 16);

					d0 = c00->c[0].g*coefx00 + c00->c[1].g*coefx01;
					d1 = c10->c[0].g*coefx00 + c10->c[1].g*coefx01;
					cd->g = (BYTE)((d0*coefy00 + d1*coefy01) >> 16);

					d0 = c00->c[0].r*coefx00 + c00->c[1].r*coefx01;
					d1 = c10->c[0].r*coefx00 + c10->c[1].r*coefx01;
					cd->r = (BYTE)((d0*coefy00 + d1*coefy01) >> 16);
#endif
				}// if(!(sten[stenpos] & mask))

				dstpos++;
				xx += wfac;
			}// for(ix=0; ix<rinfo.dst_width; ++ix)

			yy += hfac;
		}// for(iy=0; iy<rinfo.dst_height; ++iy)
#ifndef WIN64
		_asm
		{
			emms;
		}
#endif
	}// if(info.resizeMode == RESIZE_MODE_SW_BILINEAR)
}
