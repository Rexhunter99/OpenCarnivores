#ifdef _3dfx
#include "Hunt.h"

#include <glide.h>
#include <sst1vid.h>
#include <stdio.h>

void RenderShadowClip(TModel*, float, float, float, float, float, float, int, float, float);

GrVertex gvtx[16];
int zs;
float SunLight;
float TraceK, SkyTraceK, FogYGrad, FogYBase;
int SunScrX, SunScrY;
int SkySumR, SkySumG, SkySumB;
int LowHardMemory;
float vFogT[1024];
GrLfbInfo_t linfo;
int lsw;
BOOL SmallFont;
//float mdlScale = 1.0f;


FxU32 FXConstTstartAddress, FXConstTendAddress;

int FxMemUsageCount;
int FxMemLoaded;
int FxLastTexture;
int FxTMUNumber;

typedef struct _fxmemmap {
	int cpuaddr, size, lastused;
	FxU32 FXTbaseaddr;
	GrTexInfo FXtexinfo;
} Tfxmemmap;

#define fxmemmapsize 128
Tfxmemmap FxMemMap[fxmemmapsize + 2];

void CalcFogLevel_Gradient(Vector3d v)
{
	FogYBase = CalcFogLevel(v);
	if (FogYBase > 0) {
		v.y += 800;
		FogYGrad = (CalcFogLevel(v) - FogYBase) / 800.f;
	}
	else FogYGrad = 0;
}


void Hardware_ZBuffer(BOOL bl)
{
	if (bl) {
		grDepthBiasLevel(0);
	}
	else {
		grDepthBiasLevel(8000);
	}
}

void Init3DHardware()
{
	PrintLog("Checking 3dfx:\n");
	GrHwConfiguration hwconfig;
	FxTMUNumber = 1;
	grGlideInit();
	grSstQueryHardware(&hwconfig);
	if (hwconfig.SSTs[0].type == GR_SSTTYPE_VOODOO) {
		FxTMUNumber = hwconfig.SSTs[0].sstBoard.VoodooConfig.nTexelfx;
		PrintLog("Voodoo1 card.\n");
	}

	if (hwconfig.SSTs[0].type == GR_SSTTYPE_Voodoo2) {
		FxTMUNumber = hwconfig.SSTs[0].sstBoard.Voodoo2Config.nTexelfx;
		PrintLog("Voodoo2 card.\n");
	}

	if (hwconfig.SSTs[0].type == GR_SSTTYPE_SST96) {
		FxTMUNumber = hwconfig.SSTs[0].sstBoard.VoodooConfig.nTexelfx;
		PrintLog("Voodoo Rush card.\n");
	}

	grSstSelect(0);
	HARD3D = TRUE;

	FXConstTstartAddress = grTexMinAddress(GR_TMU0);
	FXConstTendAddress = grTexMaxAddress(GR_TMU0);
	wsprintf(logt, "Start Address: %d\n", FXConstTstartAddress);  PrintLog(logt);
	wsprintf(logt, "End   Address: %d\n", FXConstTendAddress);   PrintLog(logt);
	//if (FXConstTendAddress  > 2097144) FXConstTendAddress = 2097144;

}


void Activate3DHardware()
{
	//  Init3DHardware();
	if (WinW < 800)
		SetVideoMode(800, 600);
	else
		SetVideoMode(WinW, WinH);

	int fxmode = GR_RESOLUTION_640x480;
	switch (WinW) {
	case  320: fxmode = GR_RESOLUTION_320x240;  break;
	case  400: fxmode = GR_RESOLUTION_400x300;  break;
	case  512: fxmode = GR_RESOLUTION_512x384;  break;
	case  640: fxmode = GR_RESOLUTION_640x480;  break;
	case  800: fxmode = GR_RESOLUTION_800x600;  break;
	case 1024: fxmode = GR_RESOLUTION_1024x768; break;
	}


	lpDD->SetCooperativeLevel(hwndMain, DDSCL_NORMAL);


RESET:
	if (!grSstWinOpen(
		(unsigned int)hwndMain,
		fxmode,
		GR_REFRESH_60Hz,
		GR_COLORFORMAT_ABGR,
		GR_ORIGIN_UPPER_LEFT,
		2, 1)) {
		if (fxmode != GR_RESOLUTION_640x480) {
			fxmode = GR_RESOLUTION_640x480;
			SetVideoMode(640, 480);
			goto RESET;
		}
		else
			DoHalt("Voodoo: Can't set video mode");
	}


	grHints(GR_HINT_STWHINT, 0);
	grHints(GR_HINT_FPUPRECISION, 0);
	grHints(GR_HINT_ALLOW_MIPMAP_DITHER, 0);


	FXConstTstartAddress = grTexMinAddress(GR_TMU0);
	FXConstTendAddress = grTexMaxAddress(GR_TMU0);
	//if (FXConstTendAddress  > 2097144) FXConstTendAddress = 2097144;


	grColorMask(FXTRUE, FXFALSE);

	grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
	grDepthBufferFunction(GR_CMP_GEQUAL);
	grDepthMask(FXTRUE);

	//grCullMode(GR_CULL_NEGATIVE);


	//======= setup gouraud ===========//
	guColorCombineFunction(GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);

	grTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_DECAL);
	grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);

	//====== setup alpha ==========//
	guAlphaSource(GR_ALPHASOURCE_CC_ALPHA);
	grConstantColorValue(0xFF000000);
	grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,
		GR_BLEND_ONE_MINUS_SRC_ALPHA,
		GR_BLEND_ONE,
		GR_BLEND_ONE);


	grDitherMode(GR_DITHER_2x2);
	grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	//grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);


	grFogColorValue((SkyB << 16) + (SkyG << 8) + SkyR);

	grFogMode(GR_FOG_DISABLE);

	grChromakeyValue(0x00);
	grChromakeyMode(GR_CHROMAKEY_ENABLE);
	grChromakeyMode(GR_CHROMAKEY_DISABLE);

	grBufferClear(0xFF000000, 0, 0);

	grGammaCorrectionValue(1.2f);
	FxLastTexture = fxmemmapsize + 1;
}


void ResetTextureMap()
{
	FxMemUsageCount = 0;
	FxLastTexture = fxmemmapsize + 1;
	for (int m = 0; m < fxmemmapsize + 2; m++) {
		FxMemMap[m].lastused = 0;
		FxMemMap[m].cpuaddr = 0;
		FxMemMap[m].FXTbaseaddr = 0;
		FxMemMap[m].size = 0;
	}
	while (grBufferNumPending() > 0);

}


void ShutDown3DHardware()
{
	grSstWinClose();
	grGlideShutdown();
	ResetTextureMap();

	lpDD->SetCooperativeLevel(hwndMain, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
}


int Get_GR_LOD(int TW)
{
	if (TW == 128)  return GR_LOD_128;
	if (TW == 256)  return GR_LOD_256;
	if (TW == 64)   return GR_LOD_64;
	if (TW == 32)   return GR_LOD_32;
	return 0;
}


void guDownLoad(int fxm, int FXTstartAddress, LPVOID tptr, int w, int h)
{
	int textureSize = w * w * 2;
	/*
	if (FXTstartAddress<=2097152 && FXTstartAddress+textureSize>=2097152) {
		FxMemMap[fxm-1].size = 2097152 - FxMemMap[fxm-1].FXTbaseaddr;
		FXTstartAddress=2097152;
	}
	*/
	/*
	   if (FXTstartAddress + textureSize >= FXConstTendAddress)
		   DoHalt("3DFX memory overflow.");
	  */
	FxMemMap[fxm].FXtexinfo.smallLod = Get_GR_LOD(w);
	FxMemMap[fxm].FXtexinfo.largeLod = FxMemMap[fxm].FXtexinfo.smallLod;
	FxMemMap[fxm].FXtexinfo.aspectRatio = GR_ASPECT_1x1;
	FxMemMap[fxm].FXtexinfo.format = GR_TEXFMT_ARGB_1555;
	FxMemMap[fxm].FXtexinfo.data = tptr;

	FxMemMap[fxm].FXTbaseaddr = FXTstartAddress;
	FxMemMap[fxm].size = textureSize;
	FxMemMap[fxm].cpuaddr = (int)tptr;

	FxMemLoaded += textureSize;

	grTexDownloadMipMap(GR_TMU0,
		FXTstartAddress + FXConstTstartAddress,
		GR_MIPMAPLEVELMASK_BOTH,
		&FxMemMap[fxm].FXtexinfo);
}


void InsertFxMM(int m)
{
	/* memcpy(&FxMemMap[m+1],
			  &FxMemMap[m],
			  (fxmemmapsize-m) * sizeof(Tfxmemmap) ); */
	for (int mm = fxmemmapsize - 1; mm > m; mm--)
		FxMemMap[m] = FxMemMap[m - 1];
}





int FXDownLoadTexture(LPVOID tptr, int w, int h)
{
	FxU32 FXTstartAddress = 0;
	FxU32 FXTendAddress = FXConstTendAddress;
	int   textureSize = w * w * 2;
	int fxm = 0;
	int m;


	//========== if no memory used ==========//
	if (!FxMemMap[0].cpuaddr) {
		guDownLoad(0, 0, tptr, w, h);
		return 0;
	}



	//======= search for hole in the memory usage ==========//

	for (m = 0; m < fxmemmapsize; m++) {
		int nextbegin;
		int curend = FxMemMap[m].FXTbaseaddr + FxMemMap[m].size;

		if (!FxMemMap[m].cpuaddr) break;

		if (FxMemMap[m + 1].cpuaddr) nextbegin = FxMemMap[m + 1].FXTbaseaddr;
		else nextbegin = FXTendAddress;

		if (curend <= 2097152 && curend + textureSize >= 2097152)
			curend = 2097152;

		if (nextbegin - curend >= textureSize) {
			InsertFxMM(m + 1);
			guDownLoad(m + 1, curend, tptr, w, h);

			return m + 1;
		}
	}



	//====== search for most older acceptable block =====================//
	fxm = -1;
	int unusedtime = 1;
	for (m = 0; m < fxmemmapsize; m++) {
		if (!FxMemMap[m].cpuaddr) break;

		int ut = FxMemUsageCount - FxMemMap[m].lastused;

		if (FxMemMap[m].size == textureSize)
			if (ut >= unusedtime) {
				unusedtime = ut;
				fxm = m;
			}

		if (FxMemMap[m].size > textureSize)
			if (ut > unusedtime) {
				unusedtime = ut;
				fxm = m;
			}
	}

	if (fxm != -1) {
		guDownLoad(fxm, FxMemMap[fxm].FXTbaseaddr, tptr, w, h);
		return fxm;
	}



	//=== not free or unused memory found => perfoming half-memory reset ======//
	ResetTextureMap();
	guDownLoad(0, 0, tptr, w, h);
	return 0;
}



void SetFXTexture(LPVOID tptr, int w, int h)
{
	if (FxMemMap[FxLastTexture].cpuaddr == (int)tptr) return;

	int fxm = -1;
	for (int m = 0; m < fxmemmapsize; m++) {
		if (FxMemMap[m].cpuaddr == (int)tptr) { fxm = m; break; }
		if (!FxMemMap[m].cpuaddr) break;
	}

	if (fxm == -1) fxm = FXDownLoadTexture(tptr, w, h);

	FxMemMap[fxm].lastused = FxMemUsageCount;
	grTexSource(GR_TMU0,
		FxMemMap[fxm].FXTbaseaddr + FXConstTstartAddress,
		GR_MIPMAPLEVELMASK_BOTH,
		&FxMemMap[fxm].FXtexinfo);
	FxLastTexture = fxm;
}




float GetTraceK(int x, int y)
{
	if (x<8 || y<8 || x>WinW - 8 || y>WinH - 8) return 0.f;

	float k = 0;

	linfo.size = sizeof(GrLfbInfo_t);
	if (!grLfbLock(
		GR_LFB_READ_ONLY,
		GR_BUFFER_AUXBUFFER,
		GR_LFBWRITEMODE_ANY,
		GR_ORIGIN_UPPER_LEFT,
		FXFALSE,
		&linfo)) return 0.f;

	WORD CC = 0;
	int bw = (linfo.strideInBytes >> 1);
	if (*((WORD*)linfo.lfbPtr + (y + 0) * bw + x + 0) == CC) k += 1.f;
	if (*((WORD*)linfo.lfbPtr + (y + 10) * bw + x + 0) == CC) k += 1.f;
	if (*((WORD*)linfo.lfbPtr + (y - 10) * bw + x + 0) == CC) k += 1.f;
	if (*((WORD*)linfo.lfbPtr + (y + 0) * bw + x + 10) == CC) k += 1.f;
	if (*((WORD*)linfo.lfbPtr + (y + 0) * bw + x - 10) == CC) k += 1.f;

	if (*((WORD*)linfo.lfbPtr + (y + 8) * bw + x + 8) == CC) k += 1.f;
	if (*((WORD*)linfo.lfbPtr + (y + 8) * bw + x - 8) == CC) k += 1.f;
	if (*((WORD*)linfo.lfbPtr + (y - 8) * bw + x + 8) == CC) k += 1.f;
	if (*((WORD*)linfo.lfbPtr + (y - 8) * bw + x - 8) == CC) k += 1.f;

	grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_AUXBUFFER);
	k /= 9;

	DeltaFunc(TraceK, k, TimeDt / 1024.f);
	return TraceK;
}


void AddSkySum(WORD C)
{
	int R = C >> 11;
	int G = (C >> 5) & 63;
	int B = C & 31;
	SkySumR += R * 8;
	SkySumG += G * 4;
	SkySumB += B * 8;
}

float GetSkyK(int x, int y)
{
	if (x<10 || y<10 || x>WinW - 10 || y>WinH - 10) return 0.5;
	SkySumR = 0;
	SkySumG = 0;
	SkySumB = 0;
	float k = 0;

	linfo.size = sizeof(GrLfbInfo_t);
	if (!grLfbLock(
		GR_LFB_READ_ONLY,
		GR_BUFFER_BACKBUFFER,
		GR_LFBWRITEMODE_ANY,
		GR_ORIGIN_UPPER_LEFT,
		FXFALSE,
		&linfo)) return 0.f;

	int bw = (linfo.strideInBytes >> 1);
	AddSkySum(*((WORD*)linfo.lfbPtr + (y + 0) * bw + x + 0));
	AddSkySum(*((WORD*)linfo.lfbPtr + (y + 6) * bw + x + 0));
	AddSkySum(*((WORD*)linfo.lfbPtr + (y - 6) * bw + x + 0));
	AddSkySum(*((WORD*)linfo.lfbPtr + (y + 0) * bw + x + 6));
	AddSkySum(*((WORD*)linfo.lfbPtr + (y + 0) * bw + x - 6));

	AddSkySum(*((WORD*)linfo.lfbPtr + (y + 4) * bw + x + 4));
	AddSkySum(*((WORD*)linfo.lfbPtr + (y + 4) * bw + x - 4));
	AddSkySum(*((WORD*)linfo.lfbPtr + (y - 4) * bw + x + 4));
	AddSkySum(*((WORD*)linfo.lfbPtr + (y - 4) * bw + x - 4));

	grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_BACKBUFFER);

	//  char t[128];
	//  wsprintf(t, "%d  %d  %d", SkySumR, SkySumG, SkySumB);
	//  AddMessage(t);


	SkySumR -= SkyTR * 9;
	SkySumG -= SkyTG * 9;
	SkySumB -= SkyTB * 9;
	/*
	  SkySumR-=55*9;
	  SkySumG-=88*9;
	  SkySumB-=122*9;
	  */
	k = (float)sqrt(SkySumR * SkySumR + SkySumG * SkySumG + SkySumB * SkySumB) / 9;


	if (k > 80) k = 80;
	if (k < 0) k = 0;
	k = 1.0f - k / 80.f;
	if (k < 0.2) k = 0.2f;
	DeltaFunc(SkyTraceK, k, (0.07f + (float)fabs(k - SkyTraceK)) * (TimeDt / 512.f));
	return SkyTraceK;
}



void TryHiResTx()
{
	int UsedMem = 0;
	for (int m = 0; m < fxmemmapsize; m++) {
		if (!FxMemMap[m].cpuaddr) break;
		if (FxMemMap[m].lastused + 2 >= FxMemUsageCount)
			UsedMem += FxMemMap[m].size;
	}

	if (UsedMem * 3 < (int)FXConstTendAddress)
		LOWRESTX = FALSE;
}

void ShowVideo()
{
	if (FxMemLoaded > 1024 * 1024) LowHardMemory++;
	else LowHardMemory = 0;
	if (LowHardMemory > 2) {
		LOWRESTX = TRUE;
		LowHardMemory = 0;
	}

	if (OptText == 0) LOWRESTX = TRUE;
	if (OptText == 1) LOWRESTX = FALSE;
	if (OptText == 2)
		if (LOWRESTX && (Takt & 63) == 0) TryHiResTx();
	/*
	   char t[128];
	   wsprintf(t, "FX mem loaded: %dK", FxMemLoaded >> 10);
	   if (FxMemLoaded) AddMessage(t); */
	FxMemLoaded = 0;


	gvtx[0].a = 0;
	gvtx[1].a = 0;

	if (UNDERWATER && CORRECTION) {
		guColorCombineFunction(GR_COLORCOMBINE_CCRGB);
		grConstantColorValue(0x60504000);

		gvtx[0].tmuvtx[0].sow = 0.0;
		gvtx[0].tmuvtx[0].tow = 0.0;
		gvtx[0].z = (float)1.f;
		gvtx[0].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
		gvtx[0].oow = 1.0f / gvtx[0].z;

		gvtx[1].tmuvtx[0].sow = 0.0;
		gvtx[1].tmuvtx[0].tow = 0.0;
		gvtx[1].z = (float)1.f;
		gvtx[1].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
		gvtx[1].oow = 1.0f / gvtx[0].z;

		for (int y = 0; y < WinW; y++)
		{
			gvtx[0].x = 0;
			gvtx[0].y = (float)y;
			gvtx[1].x = (float)WinEX;
			gvtx[1].y = (float)y;

			grDrawLine(&gvtx[0], &gvtx[1]);
			dFacesCount++;
		}

		guColorCombineFunction(GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);
		grConstantColorValue(0xFF000000);
	}



	if (!UNDERWATER && (SunLight > 1.0f)) {
		SunLight *= GetTraceK(SunScrX, SunScrY);
		guColorCombineFunction(GR_COLORCOMBINE_CCRGB);
		grConstantColorValue(0xC0FFFF + ((int)SunLight << 24));

		gvtx[0].tmuvtx[0].sow = 0.0;
		gvtx[0].tmuvtx[0].tow = 0.0;
		gvtx[0].z = (float)1.f;
		gvtx[0].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
		gvtx[0].oow = 1.0f / gvtx[0].z;

		gvtx[1].tmuvtx[0].sow = 0.0;
		gvtx[1].tmuvtx[0].tow = 0.0;
		gvtx[1].z = (float)1.f;
		gvtx[1].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
		gvtx[1].oow = 1.0f / gvtx[0].z;

		for (int y = 0; y < WinW; y++) {
			gvtx[0].x = 0;
			gvtx[0].y = (float)y;
			gvtx[1].x = (float)WinEX;
			gvtx[1].y = (float)y;

			grDrawLine(&gvtx[0], &gvtx[1]);
			dFacesCount++;
		}

		guColorCombineFunction(GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);
		grConstantColorValue(0xFF000000);
	}

	RenderHealthBar();


	while (grBufferNumPending() > 2);
	grBufferSwap(0);
	grBufferClear(0xFF000000 + (SkyB << 16) + (SkyG << 8) + SkyR, 0, 0);
	FxMemUsageCount++;
}


void CopyHARDToDIB()
{
	/*if (_shotcounter & 1)
	 grLfbReadRegion(
	  GR_BUFFER_AUXBUFFER,
	  0,0,WinW,WinH, 2048, lpVideoBuf);
	else*/
	grLfbReadRegion(
		GR_BUFFER_FRONTBUFFER,
		0, 0, WinW, WinH, 2048, lpVideoBuf);
	/*
	 if (_shotcounter & 1)
	  for (int y=0; y<WinH; y++)
		for (int x=0; x<WinW; x++) {
			int z = *((WORD*)lpVideoBuf + x + (y<<10));
			z+=2;
			z = (int)(65535 / z) / 64;
			if (z<0) z = 0;
			if (z>31) z = 31;
			*((WORD*)lpVideoBuf + x + (y<<10) ) = z + (z<<6) + (z<<11);
		  }
		  */
}




void FXPutBitMapWithKey(int x0, int y0, int w, int h, int smw, LPVOID lpData)
{

	//grLfbConstantDepth(65000);
	//grConstantColorValue(0xD0000000);

	linfo.size = sizeof(GrLfbInfo_t);
	if (!grLfbLock(
		GR_LFB_WRITE_ONLY,
		GR_BUFFER_BACKBUFFER,
		GR_LFBWRITEMODE_565,
		GR_ORIGIN_UPPER_LEFT,
		FXFALSE,
		&linfo)) return;

	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			WORD c = *((WORD*)lpData + y * smw + x);
			if (c != 0xFFFF)
				*((WORD*)linfo.lfbPtr + (y + y0) * (linfo.strideInBytes >> 1) + x + x0) = conv_565(c);
		}

	grLfbUnlock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER);
}




void FXPutBitMap(int x0, int y0, int w, int h, int smw, LPVOID lpData)
{

	//grLfbConstantDepth(65000);
	//grConstantColorValue(0xD0000000);

	linfo.size = sizeof(GrLfbInfo_t);
	if (!grLfbLock(
		GR_LFB_WRITE_ONLY,
		GR_BUFFER_BACKBUFFER,
		GR_LFBWRITEMODE_565,
		GR_ORIGIN_UPPER_LEFT,
		FXFALSE,
		&linfo)) return;

	smw *= 2; w *= 2;
	linfo.lfbPtr = (void*)((WORD*)linfo.lfbPtr + x0);
	for (int y = 0; y < h; y++)
		memcpy((BYTE*)linfo.lfbPtr + (y + y0) * (linfo.strideInBytes),
			(BYTE*)lpData + y * smw,
			w);

	grLfbUnlock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER);
}



void DrawPicture(int x, int y, TPicture& pic)
{
	FXPutBitMap(x, y, pic.W, pic.H, pic.W, pic.lpImage);
}



void FXTextOut(int x, int y, LPSTR t, int color)
{

	HDC _hdc = hdcCMain;
	HBITMAP hbmpOld = SelectObject(_hdc, hbmpVideoBuf);
	SetBkMode(_hdc, TRANSPARENT);

	HFONT oldfont;
	if (SmallFont) oldfont = SelectObject(_hdc, fnt_Small);

	int w = GetTextW(_hdc, t) + 4;
	for (int h = 0; h < 18; h++)
		FillMemory((BYTE*)lpVideoBuf + h * 2048, w * 2, 0xFF);


	SetTextColor(_hdc, 0x00101010);
	TextOut(_hdc, 2, 1, t, strlen(t));

	SetTextColor(_hdc, color);
	TextOut(_hdc, 1, 0, t, strlen(t));

	if (SmallFont) SelectObject(_hdc, oldfont);
	SelectObject(_hdc, hbmpOld);


	FXPutBitMapWithKey(x, y, w, 18, 1024, lpVideoBuf);

}


void DrawTrophyText(int x0, int y0)
{
	int x;
	SmallFont = TRUE;
	HFONT oldfont = SelectObject(hdcMain, fnt_Small);
	int tc = TrophyBody;

	int   dtype = TrophyRoom.Body[tc].ctype;
	int   time = TrophyRoom.Body[tc].time;
	int   date = TrophyRoom.Body[tc].date;
	int   wep = TrophyRoom.Body[tc].weapon;
	int   score = TrophyRoom.Body[tc].score;
	float scale = TrophyRoom.Body[tc].scale;
	float range = TrophyRoom.Body[tc].range;
	char t[32];

	x0 += 14; y0 += 18;
	x = x0;
	FXTextOut(x, y0, "Name: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Name: ");
	FXTextOut(x, y0, DinoInfo[dtype].Name, 0x0000BFBF);

	x = x0;
	FXTextOut(x, y0 + 16, "Weight: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Weight: ");
	if (OptSys)
		sprintf(t, "%3.2ft ", DinoInfo[dtype].Mass * scale * scale / 0.907);
	else
		sprintf(t, "%3.2fT ", DinoInfo[dtype].Mass * scale * scale);


	FXTextOut(x, y0 + 16, t, 0x0000BFBF);    x += GetTextW(hdcMain, t);
	FXTextOut(x, y0 + 16, "Length: ", 0x00BFBFBF); x += GetTextW(hdcMain, "Length: ");

	if (OptSys)
		sprintf(t, "%3.2fft", DinoInfo[dtype].Length * scale / 0.3);
	else
		sprintf(t, "%3.2fm", DinoInfo[dtype].Length * scale);

	FXTextOut(x, y0 + 16, t, 0x0000BFBF);

	x = x0;
	FXTextOut(x, y0 + 32, "Weapon: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Weapon: ");
	wsprintf(t, "%s    ", WeapInfo[wep].Name);
	FXTextOut(x, y0 + 32, t, 0x0000BFBF);   x += GetTextW(hdcMain, t);
	FXTextOut(x, y0 + 32, "Score: ", 0x00BFBFBF);   x += GetTextW(hdcMain, "Score: ");
	wsprintf(t, "%d", score);
	FXTextOut(x, y0 + 32, t, 0x0000BFBF);


	x = x0;
	FXTextOut(x, y0 + 48, "Range of kill: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Range of kill: ");
	if (OptSys) sprintf(t, "%3.1fft", range / 0.3);
	else        sprintf(t, "%3.1fm", range);
	FXTextOut(x, y0 + 48, t, 0x0000BFBF);


	x = x0;
	FXTextOut(x, y0 + 64, "Date: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Date: ");
	if (OptSys)
		wsprintf(t, "%d.%d.%d   ", ((date >> 10) & 255), (date & 255), date >> 20);
	else
		wsprintf(t, "%d.%d.%d   ", (date & 255), ((date >> 10) & 255), date >> 20);

	FXTextOut(x, y0 + 64, t, 0x0000BFBF);   x += GetTextW(hdcMain, t);
	FXTextOut(x, y0 + 64, "Time: ", 0x00BFBFBF);   x += GetTextW(hdcMain, "Time: ");
	wsprintf(t, "%d:%02d", ((time >> 10) & 255), (time & 255));
	FXTextOut(x, y0 + 64, t, 0x0000BFBF);

	SmallFont = FALSE;

	SelectObject(hdcMain, oldfont);
}




void Render_LifeInfo(int li)
{
	int x, y;
	SmallFont = TRUE;
	HFONT oldfont = SelectObject(hdcMain, fnt_Small);

	int   ctype = Characters[li].CType;
	float  scale = Characters[li].scale;
	char t[32];

	x = VideoCX + WinW / 64;
	y = VideoCY + (int)(WinH / 6.8);

	FXTextOut(x, y, DinoInfo[ctype].Name, 0x0000b000);

	if (OptSys) sprintf(t, "Weight: %3.2ft ", DinoInfo[ctype].Mass * scale * scale / 0.907);
	else        sprintf(t, "Weight: %3.2fT ", DinoInfo[ctype].Mass * scale * scale);

	FXTextOut(x, y + 16, t, 0x0000b000);

	SmallFont = FALSE;
	SelectObject(hdcMain, oldfont);
}


void RenderFXMMap()
{
	WORD fxmm[32][256];
	byte mused[1024];

	FillMemory(fxmm, sizeof(fxmm), 0);
	FillMemory(mused, sizeof(mused), 4);
	FillMemory(mused, (FXConstTendAddress + 8) >> 13, 0);

	for (int m = 0; m < fxmemmapsize; m++) {
		if (!FxMemMap[m].cpuaddr) break;
		int b0 = FxMemMap[m].FXTbaseaddr >> 13;
		int bc = FxMemMap[m].size >> 13;
		bc += b0;

		for (int mm = b0; mm <= bc; mm++)
			if (FxMemMap[m].lastused + 2 < FxMemUsageCount) mused[mm] = 1; else mused[mm] = 2;
	}

	WORD c;
	for (int y = 0; y < 8; y++)
		for (int x = 0; x < 64; x++) {
			if (mused[y * 64 + x] == 0) c = 16 << 5; else
				if (mused[y * 64 + x] == 4) c = 8 * 0x421; else
					if (mused[y * 64 + x] == 1) c = 10 << 10;
					else c = 18 << 10;

			for (int yy = y * 4; yy < y * 4 + 3; yy++)
				for (int xx = x * 4; xx < x * 4 + 3; xx++)
					fxmm[yy][xx] = c;
		}

	FXPutBitMap(16, WinH - 46, 256, 32, 256, fxmm);
}

/**
Render text-based HUD elements
**/
void ShowControlElements()
{

	//if (DEBUG) RenderFXMMap();  

	char buf[128];

	if (TIMER) {
		wsprintf(buf, "msc: %d", TimeDt);
		FXTextOut(WinEX - 81, 11, buf, 0x0020A0A0);

		wsprintf(buf, "polys: %d", dFacesCount);
		FXTextOut(WinEX - 90, 24, buf, 0x0020A0A0);
	}

	if (MessageList.timeleft) {
		if (RealTime > MessageList.timeleft) MessageList.timeleft = 0;
		FXTextOut(10, 10, MessageList.mtext, 0x0020A0A0);
	}

	if (ExitTime) {
		int y = WinH / 3;
		wsprintf(buf, "Preparing for evacuation...");
		FXTextOut(VideoCX - GetTextW(hdcCMain, buf) / 2, y, buf, 0x0060C0D0);
		wsprintf(buf, "%d seconds left.", 1 + ExitTime / 1000);
		FXTextOut(VideoCX - GetTextW(hdcCMain, buf) / 2, y + 18, buf, 0x0060C0D0);
	}

}









void ClipVector(CLIPPLANE& C, int vn)
{
	int ClipRes = 0;
	float s, s1, s2;
	int vleft = (vn - 1); if (vleft < 0) vleft = vused - 1;
	int vright = (vn + 1); if (vright >= vused) vright = 0;

	MulVectorsScal(cp[vn].ev.v, C.nv, s); /*s=SGN(s-0.01f);*/
	if (s >= 0) return;

	MulVectorsScal(cp[vleft].ev.v, C.nv, s1); /* s1=SGN(s1+0.01f); */ s1 += 0.0001f;
	MulVectorsScal(cp[vright].ev.v, C.nv, s2); /* s2=SGN(s2+0.01f); */ s2 += 0.0001f;

	if (s1 > 0) {
		ClipRes += 1;
		CalcHitPoint(C, cp[vn].ev.v,
			cp[vleft].ev.v, hleft.ev.v);

		float ll = VectorLength(SubVectors(cp[vleft].ev.v, cp[vn].ev.v));
		float lc = VectorLength(SubVectors(hleft.ev.v, cp[vn].ev.v));
		lc = lc / ll;
		hleft.tx = cp[vn].tx + (int)((cp[vleft].tx - cp[vn].tx) * lc);
		hleft.ty = cp[vn].ty + (int)((cp[vleft].ty - cp[vn].ty) * lc);
		hleft.ev.Light = cp[vn].ev.Light + (int)((cp[vleft].ev.Light - cp[vn].ev.Light) * lc);
		hleft.ev.Fog = cp[vn].ev.Fog + (int)((cp[vleft].ev.Fog - cp[vn].ev.Fog) * lc);
	}

	if (s2 > 0) {
		ClipRes += 2;
		CalcHitPoint(C, cp[vn].ev.v,
			cp[vright].ev.v, hright.ev.v);

		float ll = VectorLength(SubVectors(cp[vright].ev.v, cp[vn].ev.v));
		float lc = VectorLength(SubVectors(hright.ev.v, cp[vn].ev.v));
		lc = lc / ll;
		hright.tx = cp[vn].tx + (int)((cp[vright].tx - cp[vn].tx) * lc);
		hright.ty = cp[vn].ty + (int)((cp[vright].ty - cp[vn].ty) * lc);
		hright.ev.Light = cp[vn].ev.Light + (int)((cp[vright].ev.Light - cp[vn].ev.Light) * lc);
		hright.ev.Fog = cp[vn].ev.Fog + (int)((cp[vright].ev.Fog - cp[vn].ev.Fog) * lc);
	}

	if (ClipRes == 0) {
		u--; vused--;
		cp[vn] = cp[vn + 1];
		cp[vn + 1] = cp[vn + 2];
		cp[vn + 2] = cp[vn + 3];
		cp[vn + 3] = cp[vn + 4];
		cp[vn + 4] = cp[vn + 5];
		cp[vn + 5] = cp[vn + 6];
		//memcpy(&cp[vn], &cp[vn+1], (15-vn)*sizeof(ClipPoint)); 
	}
	if (ClipRes == 1) { cp[vn] = hleft; }
	if (ClipRes == 2) { cp[vn] = hright; }
	if (ClipRes == 3) {
		u++; vused++;
		//memcpy(&cp[vn+1], &cp[vn], (15-vn)*sizeof(ClipPoint)); 
		cp[vn + 6] = cp[vn + 5];
		cp[vn + 5] = cp[vn + 4];
		cp[vn + 4] = cp[vn + 3];
		cp[vn + 3] = cp[vn + 2];
		cp[vn + 2] = cp[vn + 1];
		cp[vn + 1] = cp[vn];

		cp[vn] = hleft;
		cp[vn + 1] = hright;
	}
}



void DrawTPlaneClip(BOOL SECONT)
{
	if (!WATERREVERSE) {
		MulVectorsVect(SubVectors(ev[1].v, ev[0].v), SubVectors(ev[2].v, ev[0].v), nv);
		if (nv.x * ev[0].v.x + nv.y * ev[0].v.y + nv.z * ev[0].v.z < 0) return;
	}

	cp[0].ev = ev[0]; cp[1].ev = ev[1]; cp[2].ev = ev[2];

	if (ReverseOn)
		if (SECONT) {
			switch (TDirection) {
			case 0:
				cp[0].tx = TCMIN;   cp[0].ty = TCMAX;
				cp[1].tx = TCMAX;   cp[1].ty = TCMIN;
				cp[2].tx = TCMAX;   cp[2].ty = TCMAX;
				break;
			case 1:
				cp[0].tx = TCMAX;   cp[0].ty = TCMAX;
				cp[1].tx = TCMIN;   cp[1].ty = TCMIN;
				cp[2].tx = TCMAX;   cp[2].ty = TCMIN;
				break;
			case 2:
				cp[0].tx = TCMAX;   cp[0].ty = TCMIN;
				cp[1].tx = TCMIN;   cp[1].ty = TCMAX;
				cp[2].tx = TCMIN;   cp[2].ty = TCMIN;
				break;
			case 3:
				cp[0].tx = TCMIN;   cp[0].ty = TCMIN;
				cp[1].tx = TCMAX;   cp[1].ty = TCMAX;
				cp[2].tx = TCMIN;   cp[2].ty = TCMAX;
				break;
			}
		}
		else {
			switch (TDirection) {
			case 0:
				cp[0].tx = TCMIN;   cp[0].ty = TCMIN;
				cp[1].tx = TCMAX;   cp[1].ty = TCMIN;
				cp[2].tx = TCMIN;   cp[2].ty = TCMAX;
				break;
			case 1:
				cp[0].tx = TCMIN;   cp[0].ty = TCMAX;
				cp[1].tx = TCMIN;   cp[1].ty = TCMIN;
				cp[2].tx = TCMAX;   cp[2].ty = TCMAX;
				break;
			case 2:
				cp[0].tx = TCMAX;   cp[0].ty = TCMAX;
				cp[1].tx = TCMIN;   cp[1].ty = TCMAX;
				cp[2].tx = TCMAX;   cp[2].ty = TCMIN;
				break;
			case 3:
				cp[0].tx = TCMAX;   cp[0].ty = TCMIN;
				cp[1].tx = TCMAX;   cp[1].ty = TCMAX;
				cp[2].tx = TCMIN;   cp[2].ty = TCMIN;
				break;
			}
		}
	else
		if (SECONT) {
			switch (TDirection) {
			case 0:
				cp[0].tx = TCMIN;   cp[0].ty = TCMIN;
				cp[1].tx = TCMAX;   cp[1].ty = TCMAX;
				cp[2].tx = TCMIN;   cp[2].ty = TCMAX;
				break;
			case 1:
				cp[0].tx = TCMIN;   cp[0].ty = TCMAX;
				cp[1].tx = TCMAX;   cp[1].ty = TCMIN;
				cp[2].tx = TCMAX;   cp[2].ty = TCMAX;
				break;
			case 2:
				cp[0].tx = TCMAX;   cp[0].ty = TCMAX;
				cp[1].tx = TCMIN;   cp[1].ty = TCMIN;
				cp[2].tx = TCMAX;   cp[2].ty = TCMIN;
				break;
			case 3:
				cp[0].tx = TCMAX;   cp[0].ty = TCMIN;
				cp[1].tx = TCMIN;   cp[1].ty = TCMAX;
				cp[2].tx = TCMIN;   cp[2].ty = TCMIN;
				break;
			}
		}
		else {
			switch (TDirection) {
			case 0:
				cp[0].tx = TCMIN;   cp[0].ty = TCMIN;
				cp[1].tx = TCMAX;   cp[1].ty = TCMIN;
				cp[2].tx = TCMAX;   cp[2].ty = TCMAX;
				break;
			case 1:
				cp[0].tx = TCMIN;   cp[0].ty = TCMAX;
				cp[1].tx = TCMIN;   cp[1].ty = TCMIN;
				cp[2].tx = TCMAX;   cp[2].ty = TCMIN;
				break;
			case 2:
				cp[0].tx = TCMAX;   cp[0].ty = TCMAX;
				cp[1].tx = TCMIN;   cp[1].ty = TCMAX;
				cp[2].tx = TCMIN;   cp[2].ty = TCMIN;
				break;
			case 3:
				cp[0].tx = TCMAX;   cp[0].ty = TCMIN;
				cp[1].tx = TCMAX;   cp[1].ty = TCMAX;
				cp[2].tx = TCMIN;   cp[2].ty = TCMAX;
				break;
			}
		}

	vused = 3;


	for (u = 0; u < vused; u++) cp[u].ev.v.z += 16.0f;
	for (u = 0; u < vused; u++) ClipVector(ClipZ, u);
	for (u = 0; u < vused; u++) cp[u].ev.v.z -= 16.0f;
	if (vused < 3) return;

	for (u = 0; u < vused; u++) ClipVector(ClipA, u); if (vused < 3) return;
	for (u = 0; u < vused; u++) ClipVector(ClipB, u); if (vused < 3) return;
	for (u = 0; u < vused; u++) ClipVector(ClipC, u); if (vused < 3) return;
	for (u = 0; u < vused; u++) ClipVector(ClipD, u); if (vused < 3) return;

	float dy = -(0.05f + (10 - max(3, r)) / 10.f) * 16.f;
	//dy = 0;

	if (WATERREVERSE) dy = 0;
	for (u = 0; u < vused; u++) {
		cp[u].ev.scrx = (VideoCX << 4) - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW * 16.f);
		cp[u].ev.scry = (VideoCY << 4) + (int)(dy + cp[u].ev.v.y / cp[u].ev.v.z * CameraH * 16.f);
	}


	gvtx[0].a = (float)cp[0].ev.Fog;

	gvtx[0].x = (float)cp[0].ev.scrx / 16.f;
	gvtx[0].y = (float)cp[0].ev.scry / 16.f;
	gvtx[0].z = (float)-cp[0].ev.v.z;
	gvtx[0].r = (float)(255 - cp[0].ev.Light * 4); gvtx[0].g = gvtx[0].r; gvtx[0].b = gvtx[0].r;
	gvtx[0].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
	gvtx[0].oow = 1.0f / gvtx[0].z;
	gvtx[0].tmuvtx[0].sow = (float)cp[0].tx / 65534.f * 2 * gvtx[0].oow;
	gvtx[0].tmuvtx[0].tow = (float)cp[0].ty / 65534.f * 2 * gvtx[0].oow;

	for (u = 0; u < vused - 2; u++) {

		gvtx[1].a = (float)cp[1 + u].ev.Fog;
		gvtx[1].x = (float)cp[1 + u].ev.scrx / 16.f;
		gvtx[1].y = (float)cp[1 + u].ev.scry / 16.f;
		gvtx[1].z = -cp[1 + u].ev.v.z;
		gvtx[1].r = (float)(255 - cp[1 + u].ev.Light * 4); gvtx[1].g = gvtx[1].r; gvtx[1].b = gvtx[1].r;
		gvtx[1].ooz = (1.0f / gvtx[1].z) * 8.f * 65534.f;
		gvtx[1].oow = 1.0f / gvtx[1].z;
		gvtx[1].tmuvtx[0].sow = (float)cp[1 + u].tx / 65534.f * 2 * gvtx[1].oow;
		gvtx[1].tmuvtx[0].tow = (float)cp[1 + u].ty / 65534.f * 2 * gvtx[1].oow;

		gvtx[2].a = (float)cp[2 + u].ev.Fog;
		gvtx[2].x = (float)cp[2 + u].ev.scrx / 16.f;
		gvtx[2].y = (float)cp[2 + u].ev.scry / 16.f;
		gvtx[2].z = -cp[2 + u].ev.v.z;
		gvtx[2].r = (float)(255 - cp[2 + u].ev.Light * 4); gvtx[2].g = gvtx[2].r; gvtx[2].b = gvtx[2].r;
		gvtx[2].ooz = (1.0f / gvtx[2].z) * 8.f * 65534.f;
		gvtx[2].oow = 1.0f / gvtx[2].z;
		gvtx[2].tmuvtx[0].sow = (float)cp[2 + u].tx / 65534.f * 2 * gvtx[2].oow;
		gvtx[2].tmuvtx[0].tow = (float)cp[2 + u].ty / 65534.f * 2 * gvtx[2].oow;

		grDrawTriangle(&gvtx[0], &gvtx[1], &gvtx[2]);
		dFacesCount++;
	}


}








void DrawTPlane(BOOL SECONT)
{
	int n;
	BOOL SecondPass = FALSE;

	if (!WATERREVERSE) {
		/*if ((ev[1].scrx-ev[0].scrx)*(ev[2].scry-ev[0].scry) -
			(ev[1].scry-ev[0].scry)*(ev[2].scrx-ev[0].scrx) < 0) return; */
		MulVectorsVect(SubVectors(ev[1].v, ev[0].v), SubVectors(ev[2].v, ev[0].v), nv);
		if (nv.x * ev[0].v.x + nv.y * ev[0].v.y + nv.z * ev[0].v.z < 0) return;
	}

	Mask1 = 0x007F;
	for (n = 0; n < 3; n++) {
		if (ev[n].DFlags & 128) return;
		Mask1 = Mask1 & ev[n].DFlags;
	}
	if (Mask1 > 0) return;

	for (n = 0; n < 3; n++) {
		//scrp[n].x = ev[n].scrx;
		//scrp[n].y = ev[n].scry;
		scrp[n].x = (VideoCX << 4) - (int)(ev[n].v.x / ev[n].v.z * CameraW * 16.f);
		scrp[n].y = (VideoCY << 4) + (int)(ev[n].v.y / ev[n].v.z * CameraH * 16.f);
		scrp[n].Light = ev[n].Light;
	}

	if (ReverseOn)
		if (SECONT) {
			switch (TDirection) {
			case 0:
				scrp[0].tx = TCMIN;   scrp[0].ty = TCMAX;
				scrp[1].tx = TCMAX;   scrp[1].ty = TCMIN;
				scrp[2].tx = TCMAX;   scrp[2].ty = TCMAX;
				break;
			case 1:
				scrp[0].tx = TCMAX;   scrp[0].ty = TCMAX;
				scrp[1].tx = TCMIN;   scrp[1].ty = TCMIN;
				scrp[2].tx = TCMAX;   scrp[2].ty = TCMIN;
				break;
			case 2:
				scrp[0].tx = TCMAX;   scrp[0].ty = TCMIN;
				scrp[1].tx = TCMIN;   scrp[1].ty = TCMAX;
				scrp[2].tx = TCMIN;   scrp[2].ty = TCMIN;
				break;
			case 3:
				scrp[0].tx = TCMIN;   scrp[0].ty = TCMIN;
				scrp[1].tx = TCMAX;   scrp[1].ty = TCMAX;
				scrp[2].tx = TCMIN;   scrp[2].ty = TCMAX;
				break;
			}
		}
		else {
			switch (TDirection) {
			case 0:
				scrp[0].tx = TCMIN;   scrp[0].ty = TCMIN;
				scrp[1].tx = TCMAX;   scrp[1].ty = TCMIN;
				scrp[2].tx = TCMIN;   scrp[2].ty = TCMAX;
				break;
			case 1:
				scrp[0].tx = TCMIN;   scrp[0].ty = TCMAX;
				scrp[1].tx = TCMIN;   scrp[1].ty = TCMIN;
				scrp[2].tx = TCMAX;   scrp[2].ty = TCMAX;
				break;
			case 2:
				scrp[0].tx = TCMAX;   scrp[0].ty = TCMAX;
				scrp[1].tx = TCMIN;   scrp[1].ty = TCMAX;
				scrp[2].tx = TCMAX;   scrp[2].ty = TCMIN;
				break;
			case 3:
				scrp[0].tx = TCMAX;   scrp[0].ty = TCMIN;
				scrp[1].tx = TCMAX;   scrp[1].ty = TCMAX;
				scrp[2].tx = TCMIN;   scrp[2].ty = TCMIN;
				break;
			}
		}
	else
		if (SECONT) {
			switch (TDirection) {
			case 0:
				scrp[0].tx = TCMIN;   scrp[0].ty = TCMIN;
				scrp[1].tx = TCMAX;   scrp[1].ty = TCMAX;
				scrp[2].tx = TCMIN;   scrp[2].ty = TCMAX;
				break;
			case 1:
				scrp[0].tx = TCMIN;   scrp[0].ty = TCMAX;
				scrp[1].tx = TCMAX;   scrp[1].ty = TCMIN;
				scrp[2].tx = TCMAX;   scrp[2].ty = TCMAX;
				break;
			case 2:
				scrp[0].tx = TCMAX;   scrp[0].ty = TCMAX;
				scrp[1].tx = TCMIN;   scrp[1].ty = TCMIN;
				scrp[2].tx = TCMAX;   scrp[2].ty = TCMIN;
				break;
			case 3:
				scrp[0].tx = TCMAX;   scrp[0].ty = TCMIN;
				scrp[1].tx = TCMIN;   scrp[1].ty = TCMAX;
				scrp[2].tx = TCMIN;   scrp[2].ty = TCMIN;
				break;
			}
		}
		else {
			switch (TDirection) {
			case 0:
				scrp[0].tx = TCMIN;   scrp[0].ty = TCMIN;
				scrp[1].tx = TCMAX;   scrp[1].ty = TCMIN;
				scrp[2].tx = TCMAX;   scrp[2].ty = TCMAX;
				break;
			case 1:
				scrp[0].tx = TCMIN;   scrp[0].ty = TCMAX;
				scrp[1].tx = TCMIN;   scrp[1].ty = TCMIN;
				scrp[2].tx = TCMAX;   scrp[2].ty = TCMIN;
				break;
			case 2:
				scrp[0].tx = TCMAX;   scrp[0].ty = TCMAX;
				scrp[1].tx = TCMIN;   scrp[1].ty = TCMAX;
				scrp[2].tx = TCMIN;   scrp[2].ty = TCMIN;
				break;
			case 3:
				scrp[0].tx = TCMAX;   scrp[0].ty = TCMIN;
				scrp[1].tx = TCMAX;   scrp[1].ty = TCMAX;
				scrp[2].tx = TCMIN;   scrp[2].ty = TCMAX;
				break;
			}
		}

	gvtx[0].a = (float)ev[0].Fog;
	gvtx[1].a = (float)ev[1].Fog;
	gvtx[2].a = (float)ev[2].Fog;

	if (!WATERREVERSE)
		if (zs > (ctViewR - 8) << 8) {
			guAlphaSource(GR_ALPHASOURCE_ITERATED_ALPHA);
			grFogMode(GR_FOG_DISABLE);
			SecondPass = TRUE;

			float zz;
			zz = VectorLength(ev[0].v) - 256 * (ctViewR - 4);
			if (zz > 0) gvtx[0].a = max(0.f, 255.f - zz / 3.f); else gvtx[0].a = 255.f;

			zz = VectorLength(ev[1].v) - 256 * (ctViewR - 4);
			if (zz > 0) gvtx[1].a = max(0.f, 255.f - zz / 3.f); else gvtx[1].a = 255.f;

			zz = VectorLength(ev[2].v) - 256 * (ctViewR - 4);
			if (zz > 0) gvtx[2].a = max(0, 255.f - zz / 3.f); else gvtx[2].a = 255.f;
		}


	gvtx[0].x = (float)scrp[0].x / 16.f;
	gvtx[0].y = (float)scrp[0].y / 16.f;
	gvtx[0].z = -ev[0].v.z;
	gvtx[0].r = (float)(255 - scrp[0].Light * 4); gvtx[0].g = gvtx[0].r; gvtx[0].b = gvtx[0].r;
	gvtx[0].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
	gvtx[0].oow = 1.0f / gvtx[0].z;
	gvtx[0].tmuvtx[0].sow = (float)scrp[0].tx / 65534.f * 2 * gvtx[0].oow;
	gvtx[0].tmuvtx[0].tow = (float)scrp[0].ty / 65534.f * 2 * gvtx[0].oow;


	gvtx[1].x = (float)scrp[1].x / 16.f;
	gvtx[1].y = (float)scrp[1].y / 16.f;
	gvtx[1].z = -ev[1].v.z;
	gvtx[1].r = (float)(255 - scrp[1].Light * 4); gvtx[1].g = gvtx[1].r; gvtx[1].b = gvtx[1].r;
	gvtx[1].ooz = (1.0f / gvtx[1].z) * 8.f * 65534.f;
	gvtx[1].oow = 1.0f / gvtx[1].z;
	gvtx[1].tmuvtx[0].sow = (float)scrp[1].tx / 65534.f * 2 * gvtx[1].oow;
	gvtx[1].tmuvtx[0].tow = (float)scrp[1].ty / 65534.f * 2 * gvtx[1].oow;


	gvtx[2].x = (float)scrp[2].x / 16.f;
	gvtx[2].y = (float)scrp[2].y / 16.f;
	gvtx[2].z = -ev[2].v.z;
	gvtx[2].r = (float)(255 - scrp[2].Light * 4); gvtx[2].g = gvtx[2].r; gvtx[2].b = gvtx[2].r;
	gvtx[2].ooz = (1.0f / gvtx[2].z) * 8.f * 65534.f;
	gvtx[2].oow = 1.0f / gvtx[2].z;
	gvtx[2].tmuvtx[0].sow = (float)scrp[2].tx / 65534.f * 2 * gvtx[2].oow;
	gvtx[2].tmuvtx[0].tow = (float)scrp[2].ty / 65534.f * 2 * gvtx[2].oow;

	grDrawTriangle(&gvtx[0], &gvtx[1], &gvtx[2]);
	dFacesCount++;

	if (SecondPass) {
		if (FOGON) {
			gvtx[0].a *= min(1.f, ev[0].Fog / 255.f);
			gvtx[1].a *= min(1.f, ev[1].Fog / 255.f);
			gvtx[2].a *= min(1.f, ev[2].Fog / 255.f);
			guColorCombineFunction(GR_COLORCOMBINE_CCRGB);

			grConstantColorValue(CurFogColor);
			grDrawTriangle(&gvtx[0], &gvtx[1], &gvtx[2]);

			grFogMode(GR_FOG_WITH_ITERATED_ALPHA);
		}

		guAlphaSource(GR_ALPHASOURCE_CC_ALPHA);
		grConstantColorValue((255 - GlassL) << 24);
		guColorCombineFunction(GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);
	}


}



void ProcessWaterMap(int x, int y, int r)
{
	WATERREVERSE = TRUE;
	ReverseOn = (FMap[y][x] & fmReverse);
	TDirection = (FMap[y][x] & 3);

	int t1 = TMap1[y][x];
	int t2 = TMap2[y][x];

	x = x - CCX + 64;
	y = y - CCY + 64;

	if ((VMap2[y][x].DFlags & VMap2[y][x + 1].DFlags & VMap2[y + 1][x + 1].DFlags & VMap2[y + 1][x].DFlags) == 0xFFFF) return;

	if (VMap2[y][x].DFlags != 0xFFFF) ev[0] = VMap2[y][x]; else ev[0] = VMap[y][x];
	if (VMap2[y][x + 1].DFlags != 0xFFFF) ev[1] = VMap2[y][x + 1]; else ev[1] = VMap[y][x + 1];

	if (ReverseOn)
		if (VMap2[y + 1][x].DFlags != 0xFFFF) ev[2] = VMap2[y + 1][x]; else ev[2] = VMap[y + 1][x];
	else
		if (VMap2[y + 1][x + 1].DFlags != 0xFFFF) ev[2] = VMap2[y + 1][x + 1]; else ev[2] = VMap[y + 1][x + 1];

	int AlphaL = 0x70;
	if (zs > ctViewR * 128) AlphaL -= (AlphaL - 0x10) * (zs - ctViewR * 128) / (ctViewR * 128);

	grConstantColorValue(AlphaL << 24);
	grDepthMask(FXFALSE);
	SetFXTexture(Textures[0]->DataA, 128, 128);


	if (!t1)
		if (r > 8) DrawTPlane(FALSE);
		else DrawTPlaneClip(FALSE);

	if (ReverseOn) {
		ev[0] = ev[2];
		if (VMap2[y + 1][x + 1].DFlags != 0xFFFF) ev[2] = VMap2[y + 1][x + 1]; else ev[2] = VMap[y + 1][x + 1];
	}
	else {
		ev[1] = ev[2];
		if (VMap2[y + 1][x].DFlags != 0xFFFF) ev[2] = VMap2[y + 1][x]; else ev[2] = VMap[y + 1][x];
	}

	if (!t2)
		if (r > 8) DrawTPlane(TRUE);
		else DrawTPlaneClip(TRUE);

	grConstantColorValue(0xFF000000);
	grDepthMask(FXTRUE);
}



void ProcessMap(int x, int y, int r)
{
	WATERREVERSE = FALSE;
	if (x >= ctMapSize - 1 || y >= ctMapSize - 1 ||
		x < 0 || y < 0) return;

	ev[0] = VMap[y - CCY + 64][x - CCX + 64];
	if (ev[0].v.z > BackViewR) return;

	if (FOGON) {
		int cf = FogsMap[y >> 1][x >> 1];
		if (UNDERWATER) cf = 127;
		if (cf) {
			CurFogColor = FogsList[cf].fogRGB;
			grFogColorValue(CurFogColor);
		}
		else if (CAMERAINFOG) {
			cf = CameraFogI;
			CurFogColor = FogsList[cf].fogRGB;
			grFogColorValue(CurFogColor);
		}
	}

	int t1 = TMap1[y][x];
	int t2 = TMap2[y][x];
	int rm = RandomMap[y & 31][x & 31] >> 7;
	//mdlScale = (float)(1600 + RandomMap[y & 31][x & 31]) / 2000.f;

	int ob = OMap[y][x];
	if (!MODELS) ob = 255;
	ReverseOn = (FMap[y][x] & fmReverse);
	TDirection = (FMap[y][x] & 3);

	if (UNDERWATER) {
		if (!t1) t1 = 1;
		if (!t2) t2 = 1;
		NeedWater = TRUE;
	}

	float dfi = (float)((FMap[y][x] >> 2) & 7) * 2.f * pi / 8.f;

	x = x - CCX + 64;
	y = y - CCY + 64;
	ev[1] = VMap[y][x + 1];
	if (ReverseOn) ev[2] = VMap[y + 1][x];
	else ev[2] = VMap[y + 1][x + 1];
	int mlight = rm + ((ev[0].Light + VMap[y + 1][x + 1].Light) >> 2);

	float xx = (ev[0].v.x + VMap[y + 1][x + 1].v.x) / 2;
	float yy = (ev[0].v.y + VMap[y + 1][x + 1].v.y) / 2;
	float zz = (ev[0].v.z + VMap[y + 1][x + 1].v.z) / 2;
	int GT;

	if (fabs(xx) > -zz + BackViewR) return;


	zs = (int)sqrt(xx * xx + zz * zz + yy * yy);
	if (zs > ctViewR * 256) return;

	GT = 0;
	FadeL = 0;
	GlassL = 0;

	if (t1 == 0 || t2 == 0) NeedWater = TRUE;
	if (zs > 256 * (ctViewR - 8)) {
		FadeL = (zs - 256 * (ctViewR - 8)) / 4;
		if (FadeL > 255) { GlassL = min(255, FadeL - 255); FadeL = 255; }
	}

	grConstantColorValue((255 - GlassL) << 24);

	if (MIPMAP && (zs > 256 * 10 && t1 || LOWRESTX)) SetFXTexture(Textures[t1]->DataB, 64, 64);
	else SetFXTexture(Textures[t1]->DataA, 128, 128);

	if (r > 8) DrawTPlane(FALSE);
	else DrawTPlaneClip(FALSE);

	if (ReverseOn) { ev[0] = ev[2]; ev[2] = VMap[y + 1][x + 1]; }
	else { ev[1] = ev[2]; ev[2] = VMap[y + 1][x]; }


	if (MIPMAP && (zs > 256 * 10 && t2 || LOWRESTX)) SetFXTexture(Textures[t2]->DataB, 64, 64);
	else SetFXTexture(Textures[t2]->DataA, 128, 128);

	if (r > 8) DrawTPlane(TRUE);
	else DrawTPlaneClip(TRUE);

	x = x + CCX - 64;
	y = y + CCY - 64;
	if (ob != 255) if (zz < BackViewR)
	{
		if (mlight > 42) mlight = 42;
		if (mlight < 9) mlight = 9;

		v[0].x = x * 256 + 128 - CameraX;
		v[0].z = y * 256 + 128 - CameraZ;
		v[0].y = (float)(-48 + HMapO[y][x]) * ctHScale - CameraY;

		if (!UNDERWATER)
			if (v[0].y + MObjects[ob].info.YHi < (int)(HMap[y][x] + HMap[y + 1][x + 1]) / 2 * ctHScale - CameraY) return;

		CalcFogLevel_Gradient(v[0]);

		gvtx[0].a = FogYBase;
		gvtx[1].a = gvtx[0].a;
		gvtx[2].a = gvtx[0].a;

		v[0] = RotateVector(v[0]);

		//InsertModelList(MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, CameraAlpha + dfi, CameraBeta);      

		if (MObjects[ob].info.flags & ofANIMATED)
			if (MObjects[ob].info.LastAniTime != RealTime) {
				MObjects[ob].info.LastAniTime = RealTime;
				CreateMorphedObject(MObjects[ob].model,
					MObjects[ob].vtl,
					RealTime % MObjects[ob].vtl.AniTime);
			}

		if (v[0].z < -256 * 8)
			RenderModel(MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, CameraAlpha, CameraBeta);
		else
			RenderModelClip(MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, CameraAlpha, CameraBeta);
	}

	if (UNDERWATER)
		ProcessWaterMap(x, y, r);
}








void BuildTreeNoSort()
{
	Vector2di v[3];
	Current = -1;
	int LastFace = -1;
	TFace* fptr;
	int sg;

	for (int f = 0; f < mptr->FCount; f++)
	{
		fptr = &mptr->gFace[f];
		v[0] = gScrp[fptr->v1];
		v[1] = gScrp[fptr->v2];
		v[2] = gScrp[fptr->v3];

		if (v[0].x == 0xFFFFFF) continue;
		if (v[1].x == 0xFFFFFF) continue;
		if (v[2].x == 0xFFFFFF) continue;

		if (fptr->Flags & (sfDarkBack + sfNeedVC)) {
			sg = (v[1].x - v[0].x) * (v[2].y - v[1].y) - (v[1].y - v[0].y) * (v[2].x - v[1].x);
			if (sg < 0) continue;
		}

		fptr->Next = -1;
		if (Current == -1) { Current = f; LastFace = f; }
		else
		{
			mptr->gFace[LastFace].Next = f; LastFace = f;
		}

	}
}



int  BuildTreeClipNoSort()
{
	Current = -1;
	int fc = 0;
	int LastFace = -1;
	TFace* fptr;

	for (int f = 0; f < mptr->FCount; f++)
	{
		fptr = &mptr->gFace[f];

		if (fptr->Flags & (sfDarkBack + sfNeedVC)) {
			MulVectorsVect(SubVectors(rVertex[fptr->v2], rVertex[fptr->v1]), SubVectors(rVertex[fptr->v3], rVertex[fptr->v1]), nv);
			if (nv.x * rVertex[fptr->v1].x + nv.y * rVertex[fptr->v1].y + nv.z * rVertex[fptr->v1].z < 0) continue;
		}

		fc++;
		fptr->Next = -1;
		if (Current == -1) { Current = f; LastFace = f; }
		else
		{
			mptr->gFace[LastFace].Next = f; LastFace = f;
		}

	}
	return fc;
}






void RenderModel(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	int f;

	if (fabs(y0) > -(z0 - 256 * 6)) return;

	mptr = _mptr;

	float ca = (float)cos(al);
	float sa = (float)sin(al);

	float cb = (float)cos(bt);
	float sb = (float)sin(bt);

	int minx = 10241024;
	int maxx = -10241024;
	int miny = 10241024;
	int maxy = -10241024;


	float ml = (float)(255 - light * 4);
	gvtx[0].r = ml;  gvtx[0].g = ml; gvtx[0].b = ml;
	gvtx[1].r = ml;  gvtx[1].g = ml; gvtx[1].b = ml;
	gvtx[2].r = ml;  gvtx[2].g = ml; gvtx[2].b = ml;

	gvtx[0].z = 1.0f;
	gvtx[1].z = 1.0f;
	gvtx[2].z = 1.0f;

	gvtx[0].oow = 1.0f;
	gvtx[1].oow = 1.0f;
	gvtx[2].oow = 1.0f;

	BOOL FOGACTIVE = (FOGON && (FogYBase > 0));

	if (!FOGACTIVE) {
		gvtx[0].a = 0;
		gvtx[1].a = 0;
		gvtx[2].a = 0;
	}

	TPoint3d p;
	for (int s = 0; s < mptr->VCount; s++) {
		p = mptr->gVertex[s];

		if (FOGACTIVE) {
			vFogT[s] = FogYBase + p.y * FogYGrad;
			if (vFogT[s] < 0.f) vFogT[s] = 0.f;
			if (vFogT[s] > 250.f) vFogT[s] = 250.f;
		}

		rVertex[s].x = (p.x * ca + p.z * sa) /* * mdlScale*/ + x0;

		float vz = p.z * ca - p.x * sa;

		rVertex[s].y = (p.y * cb - vz * sb) /* * mdlScale */ + y0;
		rVertex[s].z = (vz * cb + p.y * sb)  /* * mdlScale */ + z0;

		if (rVertex[s].z < -64) {
			gScrp[s].x = VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
			gScrp[s].y = VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);
		}
		else gScrp[s].x = 0xFFFFFF;

		if (gScrp[s].x > maxx) maxx = gScrp[s].x;
		if (gScrp[s].x < minx) minx = gScrp[s].x;
		if (gScrp[s].y > maxy) maxy = gScrp[s].y;
		if (gScrp[s].y < miny) miny = gScrp[s].y;
	}

	if (minx == 10241024) return;
	if (minx > WinW || maxx<0 || miny>WinH || maxy < 0) return;


	BuildTreeNoSort();


	float d = (float)sqrt(x0 * x0 + y0 * y0 + z0 * z0);
	if (LOWRESTX) d = 14 * 256;
	if (MIPMAP && (d > 12 * 256)) SetFXTexture(mptr->lpTexture2, 128, 128);
	else SetFXTexture(mptr->lpTexture, 256, 256);

	int PrevOpacity = 0;
	int NewOpacity = 0;
	int PrevTransparent = 0;
	int NewTransparent = 0;

#define oozs -8.f*65534.f

	f = Current;
	while (f != -1) {
		TFace* fptr = &mptr->gFace[f];

		NewOpacity = (fptr->Flags & sfOpacity);
		NewTransparent = (fptr->Flags & sfTransparent);

		if (NewOpacity != PrevOpacity)
			if (NewOpacity) {
				grChromakeyMode(GR_CHROMAKEY_ENABLE);
				grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
			}
			else {
				grChromakeyMode(GR_CHROMAKEY_DISABLE);
				grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
			}


		if (NewTransparent != PrevTransparent)
			if (NewTransparent) grConstantColorValue(0x80000000);
			else grConstantColorValue(0xFF000000);

		PrevOpacity = NewOpacity;
		PrevTransparent = NewTransparent;

		if (FOGACTIVE) {
			gvtx[0].a = vFogT[fptr->v1];
			gvtx[1].a = vFogT[fptr->v2];
			gvtx[2].a = vFogT[fptr->v3];
		}

		gvtx[0].x = (float)gScrp[fptr->v1].x;
		gvtx[0].y = (float)gScrp[fptr->v1].y;
		gvtx[0].ooz = oozs / rVertex[fptr->v1].z;
		gvtx[0].tmuvtx[0].sow = (float)(fptr->tax >> 16);
		gvtx[0].tmuvtx[0].tow = (float)(fptr->tay >> 16);
		float _ml = ml + mptr->VLight[fptr->v1];
		gvtx[0].r = _ml; gvtx[0].g = _ml; gvtx[0].b = _ml;

		gvtx[1].x = (float)gScrp[fptr->v2].x;
		gvtx[1].y = (float)gScrp[fptr->v2].y;
		gvtx[1].ooz = oozs / rVertex[fptr->v2].z;
		gvtx[1].tmuvtx[0].sow = (float)(fptr->tbx >> 16);
		gvtx[1].tmuvtx[0].tow = (float)(fptr->tby >> 16);
		_ml = ml + mptr->VLight[fptr->v2];
		gvtx[1].r = _ml; gvtx[1].g = _ml; gvtx[1].b = _ml;

		gvtx[2].x = (float)gScrp[fptr->v3].x;
		gvtx[2].y = (float)gScrp[fptr->v3].y;
		gvtx[2].ooz = oozs / rVertex[fptr->v3].z;
		gvtx[2].tmuvtx[0].sow = (float)(fptr->tcx >> 16);
		gvtx[2].tmuvtx[0].tow = (float)(fptr->tcy >> 16);
		_ml = ml + mptr->VLight[fptr->v3];
		gvtx[2].r = _ml; gvtx[2].g = _ml; gvtx[2].b = _ml;


		grDrawTriangle(&gvtx[0], &gvtx[1], &gvtx[2]);
		dFacesCount++;

		f = mptr->gFace[f].Next;
	}

	grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	grChromakeyMode(GR_CHROMAKEY_DISABLE);
	grConstantColorValue(0xFF000000);
}







void RenderShadowClip(TModel* _mptr,
	float xm0, float ym0, float zm0,
	float x0, float y0, float z0, float cal, float al, float bt)
{
	int f, CMASK;

	mptr = _mptr;


	float cla = (float)cos(cal);
	float sla = (float)sin(cal);

	float ca = (float)cos(al);
	float sa = (float)sin(al);

	float cb = (float)cos(bt);
	float sb = (float)sin(bt);

	float flight = 0;


	BOOL BL = FALSE;
	for (int s = 0; s < mptr->VCount; s++) {
		float mrx = mptr->gVertex[s].x * cla + mptr->gVertex[s].z * sla;
		float mrz = mptr->gVertex[s].z * cla - mptr->gVertex[s].x * sla;

		float shx = mrx + mptr->gVertex[s].y / 2;
		float shz = mrz + mptr->gVertex[s].y / 2;
		float shy = GetLandH(shx + xm0, shz + zm0) - ym0;

		rVertex[s].x = (shx * ca + shz * sa) + x0;
		float vz = shz * ca - shx * sa;
		rVertex[s].y = (shy * cb - vz * sb) + y0;
		rVertex[s].z = (vz * cb + shy * sb) + z0;
		if (rVertex[s].z < 0) BL = TRUE;

		if (rVertex[s].z > -256) { gScrp[s].x = 0xFFFFFF; gScrp[s].y = 0xFF; }
		else {
			int f = 0;
			int sx = VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
			int sy = VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);

			if (sx >= WinEX) f += 1;
			if (sx <= 0) f += 2;

			if (sy >= WinEY) f += 4;
			if (sy <= 0) f += 8;

			gScrp[s].y = f;
		}

	}

	if (!BL) return;


	float d = (float)sqrt(x0 * x0 + y0 * y0 + z0 * z0);
	if (LOWRESTX) d = 14 * 256;
	if (MIPMAP && (d > 12 * 256)) SetFXTexture(mptr->lpTexture2, 128, 128);
	else SetFXTexture(mptr->lpTexture, 256, 256);

	int BiasValue = BuildTreeClipNoSort();

	f = Current;
	while (f != -1) {

		vused = 3;
		TFace* fptr = &mptr->gFace[f];

		CMASK = 0;
		CMASK |= gScrp[fptr->v1].y;
		CMASK |= gScrp[fptr->v2].y;
		CMASK |= gScrp[fptr->v3].y;

		cp[0].ev.v = rVertex[fptr->v1];  cp[0].tx = fptr->tax;  cp[0].ty = fptr->tay;
		cp[1].ev.v = rVertex[fptr->v2];  cp[1].tx = fptr->tbx;  cp[1].ty = fptr->tby;
		cp[2].ev.v = rVertex[fptr->v3];  cp[2].tx = fptr->tcx;  cp[2].ty = fptr->tcy;

		if (CMASK == 0xFF) {
			for (u = 0; u < vused; u++) cp[u].ev.v.z += 16.0f;
			for (u = 0; u < vused; u++) ClipVector(ClipZ, u);
			for (u = 0; u < vused; u++) cp[u].ev.v.z -= 16.0f;
			if (vused < 3) goto LNEXT;
		}

		if (CMASK & 1) for (u = 0; u < vused; u++) ClipVector(ClipA, u); if (vused < 3) goto LNEXT;
		if (CMASK & 2) for (u = 0; u < vused; u++) ClipVector(ClipC, u); if (vused < 3) goto LNEXT;
		if (CMASK & 4) for (u = 0; u < vused; u++) ClipVector(ClipB, u); if (vused < 3) goto LNEXT;
		if (CMASK & 8) for (u = 0; u < vused; u++) ClipVector(ClipD, u); if (vused < 3) goto LNEXT;

		for (u = 0; u < vused; u++) {
			gvtx[u].x = (float)(VideoCX - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW));
			gvtx[u].y = (float)(VideoCY + (int)(cp[u].ev.v.y / cp[u].ev.v.z * CameraH));
			gvtx[u].z = -cp[u].ev.v.z - 16.f;
			gvtx[u].ooz = (1.0f / gvtx[u].z) * 8.f * 65534.f;
			gvtx[u].oow = 1.0f / gvtx[u].z;
			gvtx[u].tmuvtx[0].sow = (float)cp[u].tx / 65534.f * gvtx[u].oow;
			gvtx[u].tmuvtx[0].tow = (float)cp[u].ty / 65534.f * gvtx[u].oow;
			gvtx[u].r = flight; gvtx[u].g = gvtx[u].r; gvtx[u].b = gvtx[u].r;
		}


		if (fptr->Flags & sfOpacity) {
			grChromakeyMode(GR_CHROMAKEY_ENABLE);
			grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
		}
		else {
			grChromakeyMode(GR_CHROMAKEY_DISABLE);
			grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
		}

		grDepthBiasLevel((BiasValue--) >> 3);
		for (u = 0; u < vused - 2; u++) {
			grDrawTriangle(&gvtx[0], &gvtx[u + 1], &gvtx[u + 2]);
			dFacesCount++;
		}
	LNEXT:
		f = mptr->gFace[f].Next;

	}

	grConstantColorValue(0xFF000000);
	grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	grChromakeyMode(GR_CHROMAKEY_DISABLE);
	grDepthBiasLevel(0);

	//grDepthMask( FXTRUE );

}





void RenderModelClip(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	int f, CMASK;

	if (fabs(y0) > -(z0 - 256 * 6)) return;

	mptr = _mptr;

	float ca = (float)cos(al);
	float sa = (float)sin(al);

	float cb = (float)cos(bt);
	float sb = (float)sin(bt);


	float flight = (float)(255 - light * 4);



	BOOL BL = FALSE;
	BOOL FOGACTIVE = (FOGON && (FogYBase > 0));

	for (int s = 0; s < mptr->VCount; s++) {

		if (FOGACTIVE) {
			vFogT[s] = FogYBase + mptr->gVertex[s].y * FogYGrad;
			if (vFogT[s] < 0.f) vFogT[s] = 0.f;
			if (vFogT[s] > 250.f) vFogT[s] = 250.f;
		}
		else vFogT[s] = 0;

		rVertex[s].x = (mptr->gVertex[s].x * ca + mptr->gVertex[s].z * sa) /* * mdlScale */ + x0;
		float vz = mptr->gVertex[s].z * ca - mptr->gVertex[s].x * sa;
		rVertex[s].y = (mptr->gVertex[s].y * cb - vz * sb) /* * mdlScale */ + y0;
		rVertex[s].z = (vz * cb + mptr->gVertex[s].y * sb) /* * mdlScale */ + z0;
		if (rVertex[s].z < 0) BL = TRUE;

		if (rVertex[s].z > -256) { gScrp[s].x = 0xFFFFFF; gScrp[s].y = 0xFF; }
		else {
			int f = 0;
			int sx = VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
			int sy = VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);

			if (sx >= WinEX) f += 1;
			if (sx <= 0) f += 2;

			if (sy >= WinEY) f += 4;
			if (sy <= 0) f += 8;

			gScrp[s].y = f;
		}

	}

	if (!BL) return;

	if (!FOGACTIVE) {
		gvtx[0].a = 0;
		gvtx[1].a = 0;
		gvtx[2].a = 0;
	}

	gvtx[3].a = FogYBase;
	gvtx[4].a = FogYBase;
	gvtx[5].a = FogYBase;
	gvtx[6].a = FogYBase;
	gvtx[7].a = FogYBase;


	if (LOWRESTX) SetFXTexture(mptr->lpTexture2, 128, 128);
	else SetFXTexture(mptr->lpTexture, 256, 256);

	BuildTreeClipNoSort();
	int NewOpacity = 0;
	int PrevOpacity = 0;
	int NewTransparent = 0;
	int PrevTransparent = 0;

	f = Current;
	while (f != -1) {

		vused = 3;
		TFace* fptr = &mptr->gFace[f];

		CMASK = 0;

		CMASK |= gScrp[fptr->v1].y;
		CMASK |= gScrp[fptr->v2].y;
		CMASK |= gScrp[fptr->v3].y;


		cp[0].ev.v = rVertex[fptr->v1]; cp[0].ev.Fog = vFogT[fptr->v1];  cp[0].tx = fptr->tax;  cp[0].ty = fptr->tay;  cp[0].ev.Light = (int)mptr->VLight[fptr->v1];
		cp[1].ev.v = rVertex[fptr->v2]; cp[1].ev.Fog = vFogT[fptr->v2];  cp[1].tx = fptr->tbx;  cp[1].ty = fptr->tby;  cp[1].ev.Light = (int)mptr->VLight[fptr->v2];
		cp[2].ev.v = rVertex[fptr->v3]; cp[2].ev.Fog = vFogT[fptr->v3];  cp[2].tx = fptr->tcx;  cp[2].ty = fptr->tcy;  cp[2].ev.Light = (int)mptr->VLight[fptr->v3];

		//if (CMASK == 0xFF) 
		{
			for (u = 0; u < vused; u++) cp[u].ev.v.z += 16.0f;
			for (u = 0; u < vused; u++) ClipVector(ClipZ, u);
			for (u = 0; u < vused; u++) cp[u].ev.v.z -= 16.0f;
			if (vused < 3) goto LNEXT;
		}

		if (CMASK & 1) for (u = 0; u < vused; u++) ClipVector(ClipA, u); if (vused < 3) goto LNEXT;
		if (CMASK & 2) for (u = 0; u < vused; u++) ClipVector(ClipC, u); if (vused < 3) goto LNEXT;
		if (CMASK & 4) for (u = 0; u < vused; u++) ClipVector(ClipB, u); if (vused < 3) goto LNEXT;
		if (CMASK & 8) for (u = 0; u < vused; u++) ClipVector(ClipD, u); if (vused < 3) goto LNEXT;

		for (u = 0; u < vused; u++) {
			gvtx[u].a = cp[u].ev.Fog;
			gvtx[u].x = (float)(VideoCX - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW));
			gvtx[u].y = (float)(VideoCY + (int)(cp[u].ev.v.y / cp[u].ev.v.z * CameraH));
			gvtx[u].z = -cp[u].ev.v.z;
			gvtx[u].ooz = (1.0f / gvtx[u].z) * 8.f * 65534.f;
			gvtx[u].oow = 1.0f / gvtx[u].z;
			gvtx[u].tmuvtx[0].sow = (float)cp[u].tx / 65534.f * gvtx[u].oow;
			gvtx[u].tmuvtx[0].tow = (float)cp[u].ty / 65534.f * gvtx[u].oow;
			float _flight = flight + cp[u].ev.Light;
			gvtx[u].r = _flight; gvtx[u].g = gvtx[u].r; gvtx[u].b = gvtx[u].r;
		}


		NewOpacity = (fptr->Flags & sfOpacity);
		NewTransparent = (fptr->Flags & sfTransparent);

		if (NewOpacity != PrevOpacity)
			if (NewOpacity) {
				grChromakeyMode(GR_CHROMAKEY_ENABLE);
				grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
			}
			else {
				grChromakeyMode(GR_CHROMAKEY_DISABLE);
				grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
			}

		if (NewTransparent != PrevTransparent)
			if (NewTransparent) grConstantColorValue(0x60000000);
			else grConstantColorValue(0xFF000000);

		PrevOpacity = NewOpacity;
		PrevTransparent = NewTransparent;


		for (u = 0; u < vused - 2; u++) {
			grDrawTriangle(&gvtx[0], &gvtx[u + 1], &gvtx[u + 2]);
			dFacesCount++;
		}
	LNEXT:
		f = mptr->gFace[f].Next;
	}

	grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	grChromakeyMode(GR_CHROMAKEY_DISABLE);
	grConstantColorValue(0xFF000000);
}






void RenderModelSun(TModel* _mptr, float x0, float y0, float z0, int Alpha)
{
	int f;

	mptr = _mptr;

	int minx = 10241024;
	int maxx = -10241024;
	int miny = 10241024;
	int maxy = -10241024;

	gvtx[0].r = 255.f; gvtx[0].g = gvtx[0].r; gvtx[0].b = gvtx[0].r;
	gvtx[1].r = gvtx[0].r; gvtx[1].g = gvtx[0].g; gvtx[1].b = gvtx[0].b;
	gvtx[2].r = gvtx[0].r; gvtx[2].g = gvtx[0].g; gvtx[2].b = gvtx[0].b;

	gvtx[0].z = 1.0f;
	gvtx[1].z = 1.0f;
	gvtx[2].z = 1.0f;

	gvtx[0].oow = 1.0f;
	gvtx[1].oow = 1.0f;
	gvtx[2].oow = 1.0f;

	grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);

	for (int s = 0; s < mptr->VCount; s++) {
		rVertex[s].x = mptr->gVertex[s].x + x0;
		rVertex[s].y = mptr->gVertex[s].y + y0;
		rVertex[s].z = mptr->gVertex[s].z + z0;

		if (rVertex[s].z > -64) gScrp[s].x = 0xFFFFFF; else {
			gScrp[s].x = VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
			gScrp[s].y = VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);
		}

		if (gScrp[s].x > maxx) maxx = gScrp[s].x;
		if (gScrp[s].x < minx) minx = gScrp[s].x;
		if (gScrp[s].y > maxy) maxy = gScrp[s].y;
		if (gScrp[s].y < miny) miny = gScrp[s].y;
	}

	if (minx == 10241024) return;
	if (minx > WinW || maxx<0 || miny>WinH || maxy < 0) return;

	BuildTreeNoSort();

	/*if (LOWRESTX) SetFXTexture(mptr->lpTexture2, 128, 128);
			 else SetFXTexture(mptr->lpTexture, 256, 256);*/
	SetFXTexture(mptr->lpTexture2, 128, 128);


	grConstantColorValue(Alpha << 24);
	grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,
		GR_BLEND_ONE,
		GR_BLEND_ONE,
		GR_BLEND_ONE);

	f = Current;
	while (f != -1) {
		TFace* fptr = &mptr->gFace[f];

		gvtx[0].x = (float)gScrp[fptr->v1].x;
		gvtx[0].y = (float)gScrp[fptr->v1].y;
		gvtx[0].ooz = 8.f * 65534.f / -rVertex[fptr->v1].z;
		gvtx[0].tmuvtx[0].sow = (float)(fptr->tax >> 16);
		gvtx[0].tmuvtx[0].tow = (float)(fptr->tay >> 16);

		gvtx[1].x = (float)gScrp[fptr->v2].x;
		gvtx[1].y = (float)gScrp[fptr->v2].y;
		gvtx[1].ooz = 8.f * 65534.f / -rVertex[fptr->v2].z;
		gvtx[1].tmuvtx[0].sow = (float)(fptr->tbx >> 16);
		gvtx[1].tmuvtx[0].tow = (float)(fptr->tby >> 16);

		gvtx[2].x = (float)gScrp[fptr->v3].x;
		gvtx[2].y = (float)gScrp[fptr->v3].y;
		gvtx[2].ooz = 8.f * 65534.f / -rVertex[fptr->v3].z;
		gvtx[2].tmuvtx[0].sow = (float)(fptr->tcx >> 16);
		gvtx[2].tmuvtx[0].tow = (float)(fptr->tcy >> 16);

		grDrawTriangle(&gvtx[0], &gvtx[1], &gvtx[2]);
		dFacesCount++;

		f = mptr->gFace[f].Next;
	}

	grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	grChromakeyMode(GR_CHROMAKEY_DISABLE);
	grConstantColorValue(0xFF000000);

	grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,
		GR_BLEND_ONE_MINUS_SRC_ALPHA,
		GR_BLEND_ONE,
		GR_BLEND_ONE);
}






void RenderNearModel(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	BOOL b = LOWRESTX;
	Vector3d v;
	v.x = 0; v.y = -128; v.z = 0;
	CalcFogLevel_Gradient(v);
	grFogColorValue(CurFogColor);
	FogYGrad = 0;

	//mdlScale = 1.0f;
	LOWRESTX = FALSE;
	RenderModelClip(_mptr, x0, y0, z0, light, al, bt);
	LOWRESTX = b;
}



void RenderModelClipWater(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
}



void RenderCharacter(int index)
{
}

void RenderExplosion(int index)
{
}

void RenderShip()
{
}

void RenderCharacterPost(TCharacter* cptr)
{
	//mdlScale = 1.0f;
	CreateChMorphedModel(cptr);

	float zs = (float)sqrt(cptr->rpos.x * cptr->rpos.x + cptr->rpos.y * cptr->rpos.y + cptr->rpos.z * cptr->rpos.z);
	if (zs > ctViewR * 256) return;

	GlassL = 0;
	if (zs > 256 * (ctViewR - 8)) {
		FadeL = (int)(zs - 256 * (ctViewR - 8)) / 4;
		if (FadeL > 255) {
			GlassL = min(255, FadeL - 255); FadeL = 255;
		}
	}

	waterclip = FALSE;

	grConstantColorValue((255 - GlassL) << 24);
	if (cptr->rpos.z > -256 * 10)
		RenderModelClip(cptr->pinfo->mptr,
			cptr->rpos.x, cptr->rpos.y, cptr->rpos.z, 10,
			-cptr->alpha + pi / 2 + CameraAlpha,
			CameraBeta);
	else
		RenderModel(cptr->pinfo->mptr,
			cptr->rpos.x, cptr->rpos.y, cptr->rpos.z, 10,
			-cptr->alpha + pi / 2 + CameraAlpha,
			CameraBeta);


	grConstantColorValue(0xFF000000);
	if (!SHADOWS3D) return;
	if (zs > 256 * (ctViewR - 8)) return;

	int Al = 0x50;
	if (cptr->Health == 0) {
		int at = cptr->pinfo->Animation[cptr->Phase].AniTime;
		if (Tranq) return;
		if (cptr->CType == CTYPE_HUNTER) return;
		if (cptr->FTime == at - 1) return;
		Al = Al * (at - cptr->FTime) / at;
	}

	grConstantColorValue(Al << 24);

	RenderShadowClip(cptr->pinfo->mptr,
		cptr->pos.x, cptr->pos.y, cptr->pos.z,
		cptr->rpos.x, cptr->rpos.y, cptr->rpos.z,
		pi / 2 - cptr->alpha,
		CameraAlpha,
		CameraBeta);

	grConstantColorValue(0xFF000000);
}


void RenderExplosionPost(TExplosion* eptr)
{
	CreateMorphedModel(ExplodInfo.mptr, &ExplodInfo.Animation[0], eptr->FTime);

	if (fabs(eptr->rpos.z) + fabs(eptr->rpos.x) < 800)
		RenderModelClip(ExplodInfo.mptr,
			eptr->rpos.x, eptr->rpos.y, eptr->rpos.z, 0, 0, 0);
	else
		RenderModel(ExplodInfo.mptr,
			eptr->rpos.x, eptr->rpos.y, eptr->rpos.z, 0, 0, 0);
}


void RenderShipPost()
{
	if (Ship.State == -1) return;
	GlassL = 0;
	zs = (int)VectorLength(Ship.rpos);
	if (zs > 256 * (ctViewR)) return;

	if (zs > 256 * (ctViewR - 4))
		GlassL = min(255, (int)(zs - 256 * (ctViewR - 4)) / 4);


	grConstantColorValue((255 - GlassL) << 24);

	CreateMorphedModel(ShipModel.mptr, &ShipModel.Animation[0], Ship.FTime);

	if (fabs(Ship.rpos.z) < 4000)
		RenderModelClip(ShipModel.mptr,
			Ship.rpos.x, Ship.rpos.y, Ship.rpos.z, 10, -Ship.alpha - pi / 2 + CameraAlpha, CameraBeta);
	else
		RenderModel(ShipModel.mptr,
			Ship.rpos.x, Ship.rpos.y, Ship.rpos.z, 10, -Ship.alpha - pi / 2 + CameraAlpha, CameraBeta);
	grConstantColorValue(0xFF000000);
}


void RenderPlayer(int index)
{
}



void Render3DHardwarePosts()
{

	TCharacter* cptr;
	for (int c = 0; c < ChCount; c++) {
		cptr = &Characters[c];
		cptr->rpos.x = cptr->pos.x - CameraX;
		cptr->rpos.y = cptr->pos.y - CameraY;
		cptr->rpos.z = cptr->pos.z - CameraZ;


		float r = (float)max(fabs(cptr->rpos.x), fabs(cptr->rpos.z));
		int ri = -1 + (int)(r / 256.f + 0.5f);
		if (ri < 0) ri = 0;
		if (ri > ctViewR) continue;

		if (FOGON) {
			CalcFogLevel_Gradient(cptr->rpos);
			grFogColorValue(CurFogColor);
		}

		cptr->rpos = RotateVector(cptr->rpos);

		float br = BackViewR + DinoInfo[cptr->CType].Radius;
		if (cptr->rpos.z > br) continue;
		if (fabs(cptr->rpos.x) > -cptr->rpos.z + br) continue;
		if (fabs(cptr->rpos.y) > -cptr->rpos.z + br) continue;

		RenderCharacterPost(cptr);
	}


	TExplosion* eptr;
	for (c = 0; c < ExpCount; c++) {

		eptr = &Explosions[c];
		eptr->rpos.x = eptr->pos.x - CameraX;
		eptr->rpos.y = eptr->pos.y - CameraY;
		eptr->rpos.z = eptr->pos.z - CameraZ;


		float r = (float)max(fabs(eptr->rpos.x), fabs(eptr->rpos.z));
		int ri = -1 + (int)(r / 256.f + 0.4f);
		if (ri < 0) ri = 0;
		if (ri > ctViewR) continue;

		eptr->rpos = RotateVector(eptr->rpos);

		if (eptr->rpos.z > BackViewR) continue;
		if (fabs(eptr->rpos.x) > -eptr->rpos.z + BackViewR) continue;
		RenderExplosionPost(eptr);
	}


	Ship.rpos.x = Ship.pos.x - CameraX;
	Ship.rpos.y = Ship.pos.y - CameraY;
	Ship.rpos.z = Ship.pos.z - CameraZ;
	float r = (float)max(fabs(Ship.rpos.x), fabs(Ship.rpos.z));

	int ri = -1 + (int)(r / 256.f + 0.2f);
	if (ri < 0) ri = 0;
	if (ri < ctViewR) {

		if (FOGON) {
			CalcFogLevel_Gradient(Ship.rpos);
			grFogColorValue(CurFogColor);
		}

		Ship.rpos = RotateVector(Ship.rpos);
		if (Ship.rpos.z > BackViewR) goto NOSHIP;
		if (fabs(Ship.rpos.x) > -Ship.rpos.z + BackViewR) goto NOSHIP;

		RenderShipPost();
	}
NOSHIP:;
}





void ClearVideoBuf()
{
	//grBufferClear( 0xFF000000, 0, 0);
}



int CircleCX, CircleCY;


void PutPixel(int x, int y)
{
	*((WORD*)linfo.lfbPtr + y * lsw + x) = 18 << 6;
}

void Put8pix(int X, int Y)
{
	PutPixel(CircleCX + X, CircleCY + Y);
	PutPixel(CircleCX + X, CircleCY - Y);
	PutPixel(CircleCX - X, CircleCY + Y);
	PutPixel(CircleCX - X, CircleCY - Y);
	PutPixel(CircleCX + Y, CircleCY + X);
	PutPixel(CircleCX + Y, CircleCY - X);
	PutPixel(CircleCX - Y, CircleCY + X);
	PutPixel(CircleCX - Y, CircleCY - X);
}

void DrawCircle(int cx, int cy, int R)
{
	int d = 3 - (2 * R);
	int x = 0;
	int y = R;
	CircleCX = cx;
	CircleCY = cy;
	do {
		Put8pix(x, y); x++;
		if (d < 0) d = d + (x << 2) + 6;  else
		{
			d = d + (x - y) * 4 + 10; y--;
		}
	} while (x < y);
	Put8pix(x, y);
}








void DrawHMap()
{
	int c;

	DrawPicture(VideoCX - MapPic.W / 2, VideoCY - MapPic.H / 2 - 6, MapPic);

	linfo.size = sizeof(GrLfbInfo_t);
	if (!grLfbLock(
		GR_LFB_WRITE_ONLY,
		GR_BUFFER_BACKBUFFER,
		GR_LFBWRITEMODE_565,
		GR_ORIGIN_UPPER_LEFT,
		FXFALSE,
		&linfo)) return;

	lsw = linfo.strideInBytes / 2;



	int xx = VideoCX - 128 + (CCX >> 1);
	int yy = VideoCY - 128 + (CCY >> 1);


	if (yy < 0 || yy >= WinH) goto endmap;
	if (xx < 0 || xx >= WinW) goto endmap;
	*((WORD*)linfo.lfbPtr + yy * lsw + xx) = 30 << 11;
	*((WORD*)linfo.lfbPtr + yy * lsw + xx + 1) = 30 << 11;
	yy++;
	*((WORD*)linfo.lfbPtr + yy * lsw + xx) = 30 << 11;
	*((WORD*)linfo.lfbPtr + yy * lsw + xx + 1) = 30 << 11;

	DrawCircle(xx, yy, 17);

	if (RadarMode)
		for (c = 0; c < ChCount; c++) {
			//if (Characters[c].CType<CTYPE_PARA) continue;
			if (Characters[c].CType != TargetDino + CTYPE_PARA) continue;
			if (!Characters[c].Health) continue;
			xx = VideoCX - 128 + (int)Characters[c].pos.x / 512;
			yy = VideoCY - 128 + (int)Characters[c].pos.z / 512;
			if (yy <= 0 || yy >= WinH) goto endmap;
			if (xx <= 0 || xx >= WinW) goto endmap;
			*((WORD*)linfo.lfbPtr + yy * lsw + xx) = 30 << 6;
			*((WORD*)linfo.lfbPtr + yy * lsw + xx + 1) = 30 << 6;
			yy++;
			*((WORD*)linfo.lfbPtr + yy * lsw + xx) = 30 << 6;
			*((WORD*)linfo.lfbPtr + yy * lsw + xx + 1) = 30 << 6;
		}

endmap:
	grLfbUnlock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER);
}




void RenderSun(float x, float y, float z)
{
	SunScrX = VideoCX + (int)(x / (-z) * CameraW);
	SunScrY = VideoCY - (int)(y / (-z) * CameraH);
	GetSkyK(SunScrX, SunScrY);
	float d = (float)sqrt(x * x + y * y);
	if (d < 2048) {
		SunLight = (220.f - d * 220.f / 2048.f);
		if (SunLight > 140) SunLight = 140;
		SunLight *= SkyTraceK;
	}


	if (d > 812.f) d = 812.f;
	d = (2048.f + d) / 3048.f;
	d += (1.f - SkyTraceK) / 2.f;
	RenderModelSun(SunModel, x * d, y * d, z * d, (int)(200.f * SkyTraceK));
}



void RotateVVector(Vector3d& v)
{
	float x = v.x * ca - v.z * sa;
	float y = v.y;
	float z = v.z * ca + v.x * sa;

	float xx = x;
	float xy = y * cb + z * sb;
	float xz = z * cb - y * sb;

	v.x = xx; v.y = xy; v.z = xz;
}


void RenderSkyPlane()
{
	Vector3d v, vbase;
	Vector3d tx, ty, nv;
	float p, q, qx, qy, qz, px, py, pz, rx, ry, rz, ddx, ddy;
	float lastdt = 0.f;

	SetFXTexture(SkyPic, 256, 256);

	nv.x = 512; nv.y = 4024; nv.z = 0;
	int FogBase = (int)CalcFogLevel(nv);

	cb = (float)cos(CameraBeta);
	sb = (float)sin(CameraBeta);
	SKYDTime = RealTime & ((1 << 16) - 1);

	float sh = -CameraY;
	if (MapMinY == 10241024) MapMinY = 0;
	sh = (float)((int)MapMinY) * ctHScale - CameraY;

	v.x = 0;
	v.z = (ctViewR * 4.f) / 5.f * 256.f;
	v.y = sh;

	vbase.x = v.x;
	vbase.y = v.y * cb + v.z * sb;
	vbase.z = v.z * cb - v.y * sb;

	if (vbase.z < 128) vbase.z = 128;

	int scry = VideoCY - (int)(vbase.y / vbase.z * CameraH);

	if (scry < 0) return;
	if (scry > WinEY + 1) scry = WinEY + 1;

	cb = (float)cos(CameraBeta - 0.30);
	sb = (float)sin(CameraBeta - 0.30);

	tx.x = 0.004f; tx.y = 0;    tx.z = 0;
	ty.x = 0.0f;   ty.y = 0;    ty.z = 0.004f;
	nv.x = 0;      nv.y = -1.f; nv.z = 0;

	RotateVVector(tx);
	RotateVVector(ty);
	RotateVVector(nv);

	sh = 4 * 512 * 16;
	vbase.x = -CameraX;
	vbase.y = sh;
	vbase.z = +CameraZ;
	RotateVVector(vbase);

	//============= calc render params =================//
	p = nv.x * vbase.x + nv.y * vbase.y + nv.z * vbase.z;
	ddx = vbase.x * tx.x + vbase.y * tx.y + vbase.z * tx.z;
	ddy = vbase.x * ty.x + vbase.y * ty.y + vbase.z * ty.z;

	qx = CameraH * nv.x;   qy = CameraW * nv.y;   qz = CameraW * CameraH * nv.z;
	px = p * CameraH * tx.x;   py = p * CameraW * tx.y;   pz = p * CameraW * CameraH * tx.z;
	rx = p * CameraH * ty.x;   ry = p * CameraW * ty.y;   rz = p * CameraW * CameraH * ty.z;

	px = px - ddx * qx;  py = py - ddx * qy;   pz = pz - ddx * qz;
	rx = rx - ddy * qx;  ry = ry - ddy * qy;   rz = rz - ddy * qz;

	int sx1 = -VideoCX;
	int sx2 = +VideoCX;

	float qx1 = qx * sx1 + qz;
	float qx2 = qx * sx2 + qz;
	float qyy;

	grDepthMask(FXFALSE);
	grFogMode(GR_FOG_WITH_ITERATED_ALPHA);
	grConstantColorValue(0xFF000000);
	grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
	grFogColorValue((SkyB << 16) + (SkyG << 8) + SkyR);

	float l = 255.f;
	gvtx[0].r = l; gvtx[0].g = l; gvtx[0].b = l;
	gvtx[1].r = l; gvtx[1].g = l; gvtx[1].b = l;

	gvtx[0].a = 0.f;
	gvtx[1].a = 0.f;

	gvtx[0].x = 0;
	gvtx[1].x = (float)WinEX + 1;

	gvtx[0].z = (float)1.f;
	gvtx[1].z = (float)1.f;

	gvtx[0].oow = 1.0f;
	gvtx[1].oow = 1.0f;

	for (int sky = 0; sky <= scry; sky++) {
		int sy = VideoCY - sky;
		qyy = qy * sy;

		q = qx1 + qyy;
		float fxa = (px * sx1 + py * sy + pz) / q;
		float fya = (rx * sx1 + ry * sy + rz) / q;

		q = qx2 + qyy;
		float fxb = (px * sx2 + py * sy + pz) / q;
		float fyb = (rx * sx2 + ry * sy + rz) / q;

		float dtt = (float)(SKYDTime) / 256.f;

		float dt = ((float)sqrt((fxb - fxa) * (fxb - fxa) + (fyb - fya) * (fyb - fya)) / 0x60) - 6.f;
		if (dt > 10.f) dt = 10.f;
		if (dt < lastdt) dt = lastdt;
		lastdt = dt;
		gvtx[0].a = max(dt * 225.f / 10.f, FogBase);
		gvtx[1].a = gvtx[0].a;

		gvtx[0].y = (float)sky;
		gvtx[0].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
		gvtx[0].tmuvtx[0].sow = fxa + dtt;
		gvtx[0].tmuvtx[0].tow = fya - dtt;

		gvtx[1].y = (float)sky;
		gvtx[1].ooz = (1.0f / gvtx[1].z) * 8.f * 65534.f;
		gvtx[1].tmuvtx[0].sow = fxb + dtt;
		gvtx[1].tmuvtx[0].tow = fyb - dtt;

		grDrawLine(&gvtx[0], &gvtx[1]);
		dFacesCount++;
	}

	if (!FOGON) grFogMode(GR_FOG_DISABLE);
	grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);

	gvtx[0].a = 0;
	gvtx[1].a = 0;
	gvtx[2].a = 0;

	nv.x = -2048;
	nv.y = +4048;
	nv.z = -2048;
	nv = RotateVector(nv);
	SunLight = 0;
	if (nv.z < -2024) RenderSun(nv.x, nv.y, nv.z);

	grDepthMask(FXTRUE);
	grFogColorValue(CurFogColor);

}



void RenderHealthBar()
{
	if (MyHealth >= 100000) return;
	if (MyHealth == 000000) return;

	int L = WinW / 4;
	int x0 = WinW - (WinW / 20) - L;
	int y0 = WinH / 40;
	int G = min((MyHealth * 240 / 100000), 160);
	int R = min(((100000 - MyHealth) * 240 / 100000), 160);


	int L0 = (L * MyHealth) / 100000;
	int H = WinH / 200;

	gvtx[0].tmuvtx[0].sow = 0.0;
	gvtx[0].tmuvtx[0].tow = 0.0;
	gvtx[0].z = (float)1.f;
	gvtx[0].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
	gvtx[0].oow = 1.0f / gvtx[0].z;

	gvtx[1].tmuvtx[0].sow = 0.0;
	gvtx[1].tmuvtx[0].tow = 0.0;
	gvtx[1].z = (float)1.f;
	gvtx[1].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
	gvtx[1].oow = 1.0f / gvtx[0].z;

	guColorCombineFunction(GR_COLORCOMBINE_CCRGB);

	grConstantColorValue(0xD0000000);

	for (int y = -1; y < 3; y++) {
		gvtx[0].x = (float)x0 - 1;      gvtx[0].y = (float)y0 + y;
		gvtx[1].x = (float)x0 + L0 + 1;   gvtx[1].y = (float)y0 + y;
		grDrawLine(&gvtx[0], &gvtx[1]);
	}


	grConstantColorValue(0xFF000000 + (G << 8) + R);

	gvtx[0].x = (float)x0;      gvtx[0].y = (float)y0;
	gvtx[1].x = (float)x0 + L0;   gvtx[1].y = (float)y0;
	grDrawLine(&gvtx[0], &gvtx[1]);

	gvtx[0].x = (float)x0;      gvtx[0].y = (float)y0 + 1.f;
	gvtx[1].x = (float)x0 + L0;   gvtx[1].y = (float)y0 + 1.f;
	grDrawLine(&gvtx[0], &gvtx[1]);

	guColorCombineFunction(GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);
	grConstantColorValue(0xFF000000);
}

void Render_Cross(int sx, int sy)
{

	guColorCombineFunction(GR_COLORCOMBINE_CCRGB);
	grConstantColorValue(0x60000000);

	gvtx[0].tmuvtx[0].sow = 0.0;
	gvtx[0].tmuvtx[0].tow = 0.0;
	gvtx[0].z = (float)1.f;
	gvtx[0].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
	gvtx[0].oow = 1.0f / gvtx[0].z;

	gvtx[1].tmuvtx[0].sow = 0.0;
	gvtx[1].tmuvtx[0].tow = 0.0;
	gvtx[1].z = (float)1.f;
	gvtx[1].ooz = (1.0f / gvtx[0].z) * 8.f * 65534.f;
	gvtx[1].oow = 1.0f / gvtx[0].z;


	float w = (float)WinW / 12.f;

	gvtx[0].x = (float)(sx - w);
	gvtx[0].y = (float)sy;
	gvtx[1].x = (float)(sx + w);
	gvtx[1].y = (float)sy;

	grDrawLine(&gvtx[0], &gvtx[1]);

	gvtx[0].x = (float)(sx);
	gvtx[0].y = (float)(sy - w);
	gvtx[1].x = (float)(sx);
	gvtx[1].y = (float)(sy + w);

	grDrawLine(&gvtx[0], &gvtx[1]);

	guColorCombineFunction(GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB);
	grConstantColorValue(0xFF000000);
}

#endif