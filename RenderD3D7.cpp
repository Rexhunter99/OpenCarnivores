#ifdef _d3d
#include "Hunt.h"

#include <d3d.h>

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "ddraw_error.h"

#undef  TCMAX 
#undef  TCMIN 
#define TCMAX ((128<<16)-62024)
#define TCMIN ((000<<16)+62024)



LPDIRECTDRAWSURFACE     lpddPrimary = NULL;
LPDIRECTDRAWSURFACE     lpddBack = NULL;
LPDIRECTDRAWSURFACE     lpddZBuffer = NULL;
LPDIRECTDRAWSURFACE     lpddTexture = NULL;
DDSURFACEDESC           ddsd;

LPDIRECT3D              lpd3d = NULL;
LPDIRECT3DDEVICE        lpd3dDevice = NULL;
LPDIRECT3DVIEWPORT      lpd3dViewport = NULL;
LPDIRECT3DEXECUTEBUFFER lpd3dExecuteBuffer = NULL;
LPDIRECT3DEXECUTEBUFFER lpd3dExecuteBufferG = NULL;
LPD3DTLVERTEX           lpVertex, lpVertexG;
WORD					*lpwTriCount;

HRESULT                 hRes;
D3DEXECUTEBUFFERDESC    d3dExeBufDesc;
D3DEXECUTEBUFFERDESC    d3dExeBufDescG;
LPD3DINSTRUCTION        lpInstruction, lpInstructionG;
LPD3DPROCESSVERTICES    lpProcessVertices;
LPD3DTRIANGLE           lpTriangle;
LPD3DLINE               lpLine;
LPD3DSTATE              lpState;

bool                    fDeviceFound = false;
DWORD                   dwDeviceBitDepth = 0UL;
GUID                    guidDevice;
std::string             szDeviceName = "";
std::string             szDeviceDesc = "";
D3DDEVICEDESC           d3dHWDeviceDesc;
D3DTEXTUREHANDLE        hTexture, hGTexture;
HDC                     ddBackDC;

bool                    D3DACTIVE;
bool                    VMFORMAT565;
bool                    STARTCONV555;
bool                    HANDLECHANGED;

#define NUM_INSTRUCTIONS       5UL
#define NUM_STATES            20UL
#define NUM_PROCESSVERTICES    1UL

void RenderFSRect(DWORD Color);
void d3dDownLoadTexture(int i, int w, int h, void* tptr);
bool d3dAllocTexture(int i, int w, int h);
void d3dSetTexture(LPVOID tptr, int w, int h);
void RenderShadowClip(TModel*, float, float, float, float, float, float, int, float, float);
void ResetTextureMap();


int WaterAlphaL = 255;
int d3dTexturesMem;
int d3dLastTexture;
int d3dMemUsageCount;
int d3dMemLoaded;
int GVCnt;

int zs;
float SunLight;
float TraceK, SkyTraceK, FogYGrad, FogYBase;;
int SunScrX, SunScrY;
int SkySumR, SkySumG, SkySumB;
int LowHardMemory;
int lsw;
int vFogT[1024];
bool SmallFont;


typedef struct _d3dmemmap {
	int cpuaddr, size, lastused;
	LPDIRECTDRAWSURFACE     lpddTexture;
	D3DTEXTUREHANDLE        hTexture;
} Td3dmemmap;


#define d3dmemmapsize 128
Td3dmemmap d3dMemMap[d3dmemmapsize + 2];


WORD conv_555(WORD c)
{
	return (c & 31) + ((c & 0xFFE0) >> 1);
}

void conv_pic555(TPicture& pic)
{
	if (!HARD3D) return;
	for (int y = 0; y < pic.H; y++)
		for (int x = 0; x < pic.W; x++)
			*(pic.lpImage + x + y * pic.W) = conv_555(*(pic.lpImage + x + y * pic.W));
}


void CalcFogLevel_Gradient(Vector3d v)
{
	FogYBase = CalcFogLevel(v);
	if (FogYBase > 0) {
		v.y += 800;
		FogYGrad = (CalcFogLevel(v) - FogYBase) / 800.f;
	}
	else FogYGrad = 0;
}


void Hardware_ZBuffer(bool bl)
{
	if (!bl) {
		DDBLTFX ddbltfx;
		ddbltfx.dwSize = sizeof(DDBLTFX);
		ddbltfx.dwFillDepth = 0x0000;
		lpddZBuffer->Blt(NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &ddbltfx);
	}
}

void d3dClearBuffers()
{
	DDBLTFX ddbltfx;

	ddbltfx.dwSize = sizeof(DDBLTFX);
	SkyR = 0x60; SkyG = 0x60; SkyB = 0x65;
	if (VMFORMAT565) ddbltfx.dwFillColor = (SkyR >> 3) * 32 * 32 * 2 + (SkyG >> 2) * 32 + (SkyB >> 3);
	else ddbltfx.dwFillColor = (SkyR >> 3) * 32 * 32 + (SkyG >> 3) * 32 + (SkyB >> 3);

	lpddBack->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);

	ddbltfx.dwSize = sizeof(DDBLTFX);
	ddbltfx.dwFillDepth = 0x0000;
	lpddZBuffer->Blt(NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &ddbltfx);
}


void d3dStartBuffer()
{
	ZeroMemory(&d3dExeBufDesc, sizeof(d3dExeBufDesc));
	d3dExeBufDesc.dwSize = sizeof(d3dExeBufDesc);
	hRes = lpd3dExecuteBuffer->Lock(&d3dExeBufDesc);
	
	if (FAILED(hRes)) {
		throw dohalt("Error locking execute buffer", __FUNCSIG__, __FILE__, __LINE__);
	}

	lpVertex = (LPD3DTLVERTEX)d3dExeBufDesc.lpData;
}



void d3dStartBufferG()
{
	ZeroMemory(&d3dExeBufDescG, sizeof(d3dExeBufDescG));
	d3dExeBufDescG.dwSize = sizeof(d3dExeBufDescG);
	hRes = lpd3dExecuteBufferG->Lock(&d3dExeBufDescG);

	if (FAILED(hRes)) {
		throw dohalt("Error locking execute buffer", __FUNCSIG__, __FILE__, __LINE__);
	}

	GVCnt = 0;
	hGTexture = -1;

	lpVertexG = (LPD3DTLVERTEX)d3dExeBufDescG.lpData;
	lpInstructionG = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDescG.lpData + 400 * 3);

	lpInstructionG->bOpcode = D3DOP_PROCESSVERTICES;
	lpInstructionG->bSize = sizeof(D3DPROCESSVERTICES);
	lpInstructionG->wCount = 1U;
	lpInstructionG++;

	lpProcessVertices = (LPD3DPROCESSVERTICES)lpInstructionG;
	lpProcessVertices->dwFlags = D3DPROCESSVERTICES_COPY;
	lpProcessVertices->wStart = 0U;
	lpProcessVertices->wDest = 0U;
	lpProcessVertices->dwCount = 400 * 3;
	lpProcessVertices->dwReserved = 0UL;
	lpProcessVertices++;

	lpInstructionG = (LPD3DINSTRUCTION)lpProcessVertices;

	if (FOGENABLE) {
		lpInstructionG->bOpcode = D3DOP_STATERENDER;
		lpInstructionG->bSize = sizeof(D3DSTATE);
		lpInstructionG->wCount = 1;
		lpInstructionG++;
		lpState = (LPD3DSTATE)lpInstructionG;
		lpState->drstRenderStateType = D3DRENDERSTATE_FOGCOLOR;
		if (UNDERWATER) lpState->dwArg[0] = 0x00004560;
		else lpState->dwArg[0] = 0x00606065;
		lpState++;
		lpInstructionG = (LPD3DINSTRUCTION)lpState;
	}

}


void d3dEndBufferG()
{
	if (!lpVertexG) return;

	lpInstructionG->bOpcode = D3DOP_EXIT;
	lpInstructionG->bSize = 0UL;
	lpInstructionG->wCount = 0U;

	lpInstructionG = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDescG.lpData + 400 * 3);

	lpInstructionG->bOpcode = D3DOP_PROCESSVERTICES;
	lpInstructionG->bSize = sizeof(D3DPROCESSVERTICES);
	lpInstructionG->wCount = 1U;
	lpInstructionG++;

	lpProcessVertices = (LPD3DPROCESSVERTICES)lpInstructionG;
	lpProcessVertices->dwFlags = D3DPROCESSVERTICES_COPY;
	lpProcessVertices->wStart = 0U;
	lpProcessVertices->wDest = 0U;
	lpProcessVertices->dwCount = GVCnt;
	lpProcessVertices->dwReserved = 0UL;
	lpProcessVertices++;


	lpd3dExecuteBufferG->Unlock();

	dFacesCount += GVCnt / 3;
	lpInstructionG = NULL;
	lpVertexG = NULL;
	GVCnt = 0;

	hRes = lpd3dDevice->Execute(lpd3dExecuteBufferG, lpd3dViewport, D3DEXECUTE_UNCLIPPED);
}



void d3dFlushBuffer(int fproc1, int fproc2)
{
	BOOL ColorKey = (fproc2 > 0);

	lpInstruction = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDesc.lpData + 1024 * 3);
	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 1;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREHANDLE;
	lpState->dwArg[0] = hTexture;
	lpState++;


	lpInstruction = (LPD3DINSTRUCTION)lpState;
	lpInstruction->bOpcode = D3DOP_PROCESSVERTICES;
	lpInstruction->bSize = sizeof(D3DPROCESSVERTICES);
	lpInstruction->wCount = 1U;
	lpInstruction++;

	lpProcessVertices = (LPD3DPROCESSVERTICES)lpInstruction;
	lpProcessVertices->dwFlags = D3DPROCESSVERTICES_COPY;
	lpProcessVertices->wStart = 0U;
	lpProcessVertices->wDest = 0U;
	lpProcessVertices->dwCount = (fproc1 + fproc2) * 3;
	lpProcessVertices->dwReserved = 0UL;
	lpProcessVertices++;

	lpInstruction = (LPD3DINSTRUCTION)lpProcessVertices;
	lpInstruction->bOpcode = D3DOP_TRIANGLE;
	lpInstruction->bSize = sizeof(D3DTRIANGLE);
	lpInstruction->wCount = fproc1;
	lpInstruction++;
	lpTriangle = (LPD3DTRIANGLE)lpInstruction;

	int ii = 0;
	for (int i = 0; i < fproc1; i++) {
		lpTriangle->wV1 = ii++;
		lpTriangle->wV2 = ii++;
		lpTriangle->wV3 = ii++;
		lpTriangle->wFlags = 0;
		lpTriangle++;
	}

	lpInstruction = (LPD3DINSTRUCTION)lpTriangle;

	if (ColorKey) {
		lpInstruction->bOpcode = D3DOP_STATERENDER;
		lpInstruction->bSize = sizeof(D3DSTATE);
		lpInstruction->wCount = 4;
		lpInstruction++;
		lpState = (LPD3DSTATE)lpInstruction;

		lpState->drstRenderStateType = D3DRENDERSTATE_COLORKEYENABLE;
		lpState->dwArg[0] = TRUE;
		lpState++;

		lpState->drstRenderStateType = D3DRENDERSTATE_ALPHATESTENABLE;
		lpState->dwArg[0] = TRUE;
		lpState++;

		lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMAG;
		lpState->dwArg[0] = D3DFILTER_NEAREST;
		lpState++;

		lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMIN;
		lpState->dwArg[0] = D3DFILTER_NEAREST;
		lpState++;

		lpInstruction = (LPD3DINSTRUCTION)lpState;

		lpInstruction->bOpcode = D3DOP_TRIANGLE;
		lpInstruction->bSize = sizeof(D3DTRIANGLE);
		lpInstruction->wCount = fproc2;
		lpInstruction++;
		lpTriangle = (LPD3DTRIANGLE)lpInstruction;

		for (int i = 0; i < fproc2; i++) {
			lpTriangle->wV1 = ii++;
			lpTriangle->wV2 = ii++;
			lpTriangle->wV3 = ii++;
			lpTriangle->wFlags = 0;
			lpTriangle++;
		}

		lpInstruction = (LPD3DINSTRUCTION)lpTriangle;

		lpInstruction->bOpcode = D3DOP_STATERENDER;
		lpInstruction->bSize = sizeof(D3DSTATE);
		lpInstruction->wCount = 4;
		lpInstruction++;
		lpState = (LPD3DSTATE)lpInstruction;

		lpState->drstRenderStateType = D3DRENDERSTATE_COLORKEYENABLE;
		lpState->dwArg[0] = FALSE;
		lpState++;

		lpState->drstRenderStateType = D3DRENDERSTATE_ALPHATESTENABLE;
		lpState->dwArg[0] = FALSE;
		lpState++;

		lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMAG;
		lpState->dwArg[0] = D3DFILTER_LINEAR;
		lpState++;

		lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMIN;
		lpState->dwArg[0] = D3DFILTER_LINEAR;
		lpState++;
		lpInstruction = (LPD3DINSTRUCTION)lpState;

	}


	lpInstruction->bOpcode = D3DOP_EXIT;
	lpInstruction->bSize = 0UL;
	lpInstruction->wCount = 0U;

	lpd3dExecuteBuffer->Unlock();

	hRes = lpd3dDevice->Execute(lpd3dExecuteBuffer, lpd3dViewport, D3DEXECUTE_UNCLIPPED);
	//if (FAILED(hRes)) DoHalt("Error execute buffer");   
	dFacesCount += fproc1 + fproc2;
}


DWORD BitDepthToFlags(DWORD dwBitDepth)
{
	switch (dwBitDepth) {
	case  1UL: return DDBD_1;
	case  2UL: return DDBD_2;
	case  4UL: return DDBD_4;
	case  8UL: return DDBD_8;
	case 16UL: return DDBD_16;
	case 24UL: return DDBD_24;
	case 32UL: return DDBD_32;
	default: return 0UL;
	}
}

DWORD FlagsToBitDepth(DWORD dwFlags)
{
	if (dwFlags & DDBD_1)       return 1UL;
	else if (dwFlags & DDBD_2)  return 2UL;
	else if (dwFlags & DDBD_4)  return 4UL;
	else if (dwFlags & DDBD_8)  return 8UL;
	else if (dwFlags & DDBD_16) return 16UL;
	else if (dwFlags & DDBD_24) return 24UL;
	else if (dwFlags & DDBD_32) return 32UL;
	else                        return 0UL;
}




HRESULT FillExecuteBuffer_State(LPDIRECT3DEXECUTEBUFFER lpd3dExecuteBuffer)
{
	HRESULT              hRes;
	D3DEXECUTEBUFFERDESC d3dExeBufDesc;
	LPD3DINSTRUCTION     lpInstruction;
	LPD3DSTATE           lpState;


	ZeroMemory(&d3dExeBufDesc, sizeof(d3dExeBufDesc));
	d3dExeBufDesc.dwSize = sizeof(d3dExeBufDesc);
	hRes = lpd3dExecuteBuffer->Lock(&d3dExeBufDesc);
	if (FAILED(hRes)) {
		std::cout << "Error locking execute buffer" << std::endl;
		return hRes;
	}


	lpInstruction = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDesc.lpData + 1024 * 3);
	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 22;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_ZENABLE;
	lpState->dwArg[0] = TRUE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_ZWRITEENABLE;
	lpState->dwArg[0] = TRUE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_ZFUNC;
	lpState->dwArg[0] = D3DCMP_GREATER;//EQUAL;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREPERSPECTIVE;
	lpState->dwArg[0] = TRUE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMAPBLEND;
	if (OPT_ALPHA_COLORKEY)  lpState->dwArg[0] = D3DTBLEND_MODULATEALPHA;
	else  lpState->dwArg[0] = D3DTBLEND_MODULATE;

	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMAG;
	lpState->dwArg[0] = D3DFILTER_LINEAR;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMIN;
	lpState->dwArg[0] = D3DFILTER_LINEAR;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_SHADEMODE;
	lpState->dwArg[0] = D3DSHADE_GOURAUD;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_DITHERENABLE;
	lpState->dwArg[0] = TRUE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_CULLMODE;
	lpState->dwArg[0] = D3DCULL_NONE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_BLENDENABLE;
	lpState->dwArg[0] = TRUE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_COLORKEYENABLE;
	lpState->dwArg[0] = FALSE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_ALPHABLENDENABLE;
	lpState->dwArg[0] = TRUE;//FALSE;
	lpState++;


	lpState->drstRenderStateType = D3DRENDERSTATE_ALPHATESTENABLE;
	lpState->dwArg[0] = FALSE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_ALPHAREF;
	lpState->dwArg[0] = 0;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_ALPHAFUNC;
	lpState->dwArg[0] = D3DCMP_GREATER;
	lpState++;



	lpState->drstRenderStateType = D3DRENDERSTATE_SPECULARENABLE;
	lpState->dwArg[0] = FALSE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_FOGENABLE;
	lpState->dwArg[0] = FOGENABLE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_FOGCOLOR;
	lpState->dwArg[0] = 0x00606070;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_SRCBLEND;
	lpState->dwArg[0] = D3DBLEND_SRCALPHA;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_DESTBLEND;
	lpState->dwArg[0] = D3DBLEND_INVSRCALPHA;
	lpState++;


	lpState->drstRenderStateType = D3DRENDERSTATE_STIPPLEDALPHA;
	lpState->dwArg[0] = FALSE;
	lpState++;


	lpInstruction = (LPD3DINSTRUCTION)lpState;
	lpInstruction->bOpcode = D3DOP_EXIT;
	lpInstruction->bSize = 0UL;
	lpInstruction->wCount = 0U;

	lpd3dExecuteBuffer->Unlock();

#if defined(_DEBUG)
	std::cout << "Execute buffer filled successfully" << std::endl;
#endif

	return DD_OK;

}


HRESULT WINAPI EnumDeviceCallback(
	LPGUID lpGUID,
	LPSTR lpszDeviceDesc,
	LPSTR lpszDeviceName,
	LPD3DDEVICEDESC lpd3dHWDeviceDesc,
	LPD3DDEVICEDESC lpd3dSWDeviceDesc,
	LPVOID lpUserArg)
{
	LPD3DDEVICEDESC lpd3dDeviceDesc;

#if defined(_DEBUG)
	std::stringstream ss;
	ss << "Device enumerate:\n\tDDesc: " << lpszDeviceDesc << "\n\tDName: " << lpszDeviceName << "\n";
	std::cout << ss.str() << std::endl;
	ss.str(""); ss.clear();
#endif

	if (!lpd3dHWDeviceDesc->dcmColorModel)
		return D3DENUMRET_OK; // we don't need SW rasterizer

	lpd3dDeviceDesc = lpd3dHWDeviceDesc;

	if ((lpd3dDeviceDesc->dwDeviceRenderBitDepth & dwDeviceBitDepth) == 0UL)
		return D3DENUMRET_OK;

	if (!(lpd3dDeviceDesc->dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB))
		return D3DENUMRET_OK;

	fDeviceFound = TRUE;
	std::memcpy(&guidDevice, lpGUID, sizeof(GUID));
	szDeviceDesc = lpszDeviceDesc;
	szDeviceName = lpszDeviceName;
	std::memcpy(&d3dHWDeviceDesc, lpd3dHWDeviceDesc, sizeof(D3DDEVICEDESC));

	return D3DENUMRET_CANCEL;
}


HRESULT WINAPI EnumVideoModes(LPDDSURFACEDESC ddsd, LPVOID ptr)
{
	std::cout << "\t" << ddsd->dwWidth << "x" << ddsd->dwHeight << " - " << ddsd->ddpfPixelFormat.dwRGBBitCount << std::endl;
	return DDENUMRET_OK;
}


HRESULT CreateDirect3D(HWND hwnd)
{
	HRESULT hRes;
 	std::cout << "\n=== Init Direct3D ===" << std::endl;

	DDSURFACEDESC       ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	//std::cout << "Enumerate display modes..." << std::endl;
	//lpDD->EnumDisplayModes(0, &ddsd, nullptr, EnumVideoModes);

	if (FULLSCREEN) {
		hRes = lpDD->SetCooperativeLevel(hwnd, DDSCL_NORMAL);//DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
		if (FAILED(hRes)) {
			throw dohalt("Error setting cooperative level", __FUNCTION__, __FILE__, __LINE__);
			return hRes;
		}

		hRes = lpDD->SetDisplayMode(WinW, WinH, 32);
		if (FAILED(hRes)) {
			throw dohalt("Error setting display mode", __FUNCTION__, __FILE__, __LINE__);
			return hRes;
		}
		std::cout << "Set Display mode " << WinW << "x" << WinH << ", 16bpp" << std::endl;
	}
	else {
		hRes = lpDD->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
		if (FAILED(hRes)) {
			throw dohalt("Error setting cooperative level", __FUNCTION__, __FILE__, __LINE__);
			return hRes;
		}
	}

	hRes = lpDD->QueryInterface(IID_IDirect3D, (LPVOID*)&lpd3d);
	if (FAILED(hRes)) {
		throw dohalt("Error querying Direct3D interface", __FUNCTION__, __FILE__, __LINE__);
		return hRes;
	}
	std::cout << "QueryInterface: Ok. (IID_IDirect3D)" << std::endl;

	hRes = lpDD->CreateSurface(&ddsd, &lpddPrimary, NULL);
	if (FAILED(hRes)) {
		throw dohalt("Error creating primary surface", __FUNCTION__, __FILE__, __LINE__);
		return hRes;
	}

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	hRes = lpddPrimary->GetSurfaceDesc(&ddsd);
	if (FAILED(hRes)) {
		throw dohalt("Error getting surface description", __FUNCSIG__, __FILE__, __LINE__);
		return hRes;
	}
	std::cout << "CreateSurface: Ok. (Primary)" << std::endl;

	dwDeviceBitDepth = BitDepthToFlags(ddsd.ddpfPixelFormat.dwRGBBitCount);

	fDeviceFound = false;
	hRes = lpd3d->EnumDevices(EnumDeviceCallback, &fDeviceFound);
	if (FAILED(hRes)) {
		throw dohalt("EnumDevices failed.", __FUNCSIG__, __FILE__, __LINE__);
		return hRes;
	}
	if (!fDeviceFound) {
		throw dohalt("No devices found.", __FUNCSIG__, __FILE__, __LINE__);
		return hRes;
	}
	std::cout << "EnumDevices: Ok." << std::endl;

	return DD_OK;
}




HRESULT CreateDevice(DWORD dwWidth, DWORD dwHeight)
{
	DDSURFACEDESC   ddsd;
	HRESULT         hRes = DD_OK;
	DWORD           dwZBufferBitDepth;

	if (dwWidth == 0)
		dwWidth = 800;
	if (dwHeight == 0)
		dwHeight = 600;

	std::memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwWidth = dwWidth;
	ddsd.dwHeight = dwHeight;
	ddsd.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	hRes = lpDD->CreateSurface(&ddsd, &lpddBack, NULL);
	if (FAILED(hRes)) {
		std::stringstream es;
		es << "Error creating back buffer surface\nlpDD->CreateSurface() failed with error:\n\t" << ddrawGetErrorString(hRes) << "\n";
		throw dohalt(es.str(), __FUNCSIG__, __FILE__, __LINE__);
	}
	std::cout << "CreateSurface: Ok. (BackBuffer)" << std::endl;


	if (d3dHWDeviceDesc.dwDeviceZBufferBitDepth != 0UL) {
		dwZBufferBitDepth = FlagsToBitDepth(d3dHWDeviceDesc.dwDeviceZBufferBitDepth);

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_ZBUFFERBITDEPTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
		ddsd.dwWidth = dwWidth;
		ddsd.dwHeight = dwHeight;
		ddsd.dwZBufferBitDepth = dwZBufferBitDepth;
		hRes = lpDD->CreateSurface(&ddsd, &lpddZBuffer, NULL);
		if (FAILED(hRes)) {
			throw dohalt("Error creating z-buffer", __FUNCSIG__, __FILE__, __LINE__);
		}

		hRes = lpddBack->AddAttachedSurface(lpddZBuffer);
		if (FAILED(hRes)) {
			throw dohalt("Error attaching Z-Buffer", __FUNCSIG__, __FILE__, __LINE__);
		}
		std::cout << "CreateSurface: Ok. (Z-buffer)" << std::endl;
	}


	hRes = lpddBack->QueryInterface(guidDevice, (LPVOID*)&lpd3dDevice);
	if (FAILED(hRes)) {
		throw dohalt("Error querying device interface", __FUNCSIG__, __FILE__, __LINE__);
	}
	std::cout << "QueryInterface: Ok. (lpd3dDevice)" << std::endl;

	return DD_OK;
}




HRESULT CreateScene(void)
{
	HRESULT              hRes = DD_OK;
	DWORD                dwVertexSize = 0U;
	DWORD                dwInstructionSize = 0U;
	DWORD                dwExecuteBufferSize = 0U;
	D3DEXECUTEBUFFERDESC d3dExecuteBufferDesc;
	D3DEXECUTEDATA       d3dExecuteData;

	hRes = lpd3d->CreateViewport(&lpd3dViewport, NULL);
	if (FAILED(hRes)) {
		std::stringstream es;
		es << "Error creating viewport:\n" << ddrawGetErrorString(hRes);
		throw dohalt(es.str(), __FUNCTION__, __FILE__, __LINE__ - 4);
		return hRes;
	}
	std::cout << "CreateViewport: Ok." << std::endl;

	hRes = lpd3dDevice->AddViewport(lpd3dViewport);
	if (FAILED(hRes)) {
		std::stringstream es;
		es << "Error adding viewport:\n" << ddrawGetErrorString(hRes);
		throw dohalt(es.str(), __FUNCTION__, __FILE__, __LINE__ - 4);
		return hRes;
	}
	std::cout << "AddViewport: Ok." << std::endl;


	D3DVIEWPORT d3dViewport;
	std::memset(&d3dViewport, 0, sizeof(d3dViewport));
	d3dViewport.dwSize = sizeof(d3dViewport);
	d3dViewport.dwX = 0UL;
	d3dViewport.dwY = 0UL;
	d3dViewport.dwWidth = (DWORD)WinW;
	d3dViewport.dwHeight = (DWORD)WinH;
	d3dViewport.dvScaleX = D3DVAL((float)d3dViewport.dwWidth / 2.0);
	d3dViewport.dvScaleY = D3DVAL((float)d3dViewport.dwHeight / 2.0);
	d3dViewport.dvMaxX = D3DVAL(1.0);
	d3dViewport.dvMaxY = D3DVAL(1.0);

	lpd3dViewport->SetViewport(&d3dViewport);


	//=========== CREATING EXECUTE BUFFER ======================//
	dwVertexSize = ((1024 * 3) * sizeof(D3DVERTEX));
	dwInstructionSize = (NUM_INSTRUCTIONS * sizeof(D3DINSTRUCTION)) +
		(NUM_STATES * sizeof(D3DSTATE)) +
		(NUM_PROCESSVERTICES * sizeof(D3DPROCESSVERTICES)) +
		((1024) * sizeof(D3DTRIANGLE));

	dwExecuteBufferSize = dwVertexSize + dwInstructionSize;
	ZeroMemory(&d3dExecuteBufferDesc, sizeof(d3dExecuteBufferDesc));
	d3dExecuteBufferDesc.dwSize = sizeof(d3dExecuteBufferDesc);
	d3dExecuteBufferDesc.dwFlags = D3DDEB_BUFSIZE;
	d3dExecuteBufferDesc.dwBufferSize = dwExecuteBufferSize;

	hRes = lpd3dDevice->CreateExecuteBuffer(&d3dExecuteBufferDesc, &lpd3dExecuteBuffer, NULL);
	if (FAILED(hRes)) {
		std::stringstream es;
		es << "Error creating execute buffer:\n" << ddrawGetErrorString(hRes);
		throw dohalt(es.str(), __FUNCTION__, __FILE__, __LINE__-4);
		return hRes;
	}
	std::cout << "CreateExecuteBuffer: Ok." << std::endl;

	ZeroMemory(&d3dExecuteData, sizeof(d3dExecuteData));
	d3dExecuteData.dwSize = sizeof(d3dExecuteData);
	d3dExecuteData.dwVertexCount = 1024;
	d3dExecuteData.dwInstructionOffset = dwVertexSize;
	d3dExecuteData.dwInstructionLength = dwInstructionSize;

	hRes = lpd3dExecuteBuffer->SetExecuteData(&d3dExecuteData);
	if (FAILED(hRes)) {
		std::stringstream es;
		es << "Error setting execute data:\n" << ddrawGetErrorString(hRes);
		throw dohalt(es.str(), __FUNCTION__, __FILE__, __LINE__ - 4);
		return hRes;
	}
	std::cout << "SetExecuteData: Ok." << std::endl;


	//=========== CREATING EXECUTE BUFFER ======================//
	dwVertexSize = ((400 * 3) * sizeof(D3DVERTEX));
	dwInstructionSize = (300 * sizeof(D3DINSTRUCTION)) +
		(300 * sizeof(D3DSTATE)) +
		(10 * sizeof(D3DPROCESSVERTICES)) +
		((400) * sizeof(D3DTRIANGLE));

	dwExecuteBufferSize = dwVertexSize + dwInstructionSize;
	std::memset(&d3dExecuteBufferDesc, 0, sizeof(d3dExecuteBufferDesc));
	d3dExecuteBufferDesc.dwSize = sizeof(d3dExecuteBufferDesc);
	d3dExecuteBufferDesc.dwFlags = D3DDEB_BUFSIZE;
	d3dExecuteBufferDesc.dwBufferSize = dwExecuteBufferSize;

	hRes = lpd3dDevice->CreateExecuteBuffer(&d3dExecuteBufferDesc, &lpd3dExecuteBufferG, NULL);
	if (FAILED(hRes)) {
		std::stringstream es;
		es << "Error creating execute buffer:\n" << ddrawGetErrorString(hRes);
		throw dohalt(es.str(), __FUNCTION__, __FILE__, __LINE__ - 4);
		return hRes;
	}
	std::cout << "CreateExecuteBuffer: Ok." << std::endl;

	ZeroMemory(&d3dExecuteData, sizeof(d3dExecuteData));
	d3dExecuteData.dwSize = sizeof(d3dExecuteData);
	d3dExecuteData.dwVertexCount = 400;
	d3dExecuteData.dwInstructionOffset = dwVertexSize;
	d3dExecuteData.dwInstructionLength = dwInstructionSize;
	hRes = lpd3dExecuteBufferG->SetExecuteData(&d3dExecuteData);
	if (FAILED(hRes)) {
		throw dohalt("Error setting execute data", __FUNCSIG__, __FILE__, __LINE__);
		return hRes;
	}
	std::cout << "SetExecuteData: Ok." << std::endl;

	FillExecuteBuffer_State(lpd3dExecuteBuffer);
	hRes = lpd3dDevice->Execute(lpd3dExecuteBuffer, lpd3dViewport, D3DEXECUTE_UNCLIPPED);

	return DD_OK;
}




void Init3DHardware()
{
	HARD3D = true;
}



void d3dDetectCaps()
{
	int max_texture = 0;

	for (int t = 0; t < d3dmemmapsize; t++) {
		if (!d3dAllocTexture(t, 256, 256)) break;
		max_texture = t;
	}

	d3dTexturesMem = max_texture * 256 * 256 * 2;


	d3dDownLoadTexture(0, 256, 256, SkyPic);
	//DWORD T = timeGetTime();
	int64_t T = TimerGetMS();
	for (int t = 0; t < 10; t++) d3dDownLoadTexture(0, 256, 256, SkyPic);
	T = TimerGetMS() - T;
	//T = timeGetTime() - T;

	std::cout << "DETECTED: Texture memory : " << (d3dTexturesMem >> 10) << "K." << std::endl;

	ResetTextureMap();

	if (T == 0)
		T = 1;
	int tex_trans_speed = 128 * 10000 / T;
	std::cout << "DETECTED: Texture transfer speed: " << (tex_trans_speed) << "K/sec." << std::endl;

	DDSURFACEDESC ddsd;
	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);

	if (lpddBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) {
		return;
	}
	
	lpddBack->Unlock(ddsd.lpSurface);

	if (ddsd.ddpfPixelFormat.dwGBitMask == 0x3E0) 
		VMFORMAT565 = false;
	else 
		VMFORMAT565 = true;
	
	if (VMFORMAT565) {
		std::cout << "DETECTED: PixelFormat RGB565" << std::endl;
	}
	else {
		std::cout << "DETECTED: PixelFormat RGB555" << std::endl;
		if (!STARTCONV555) {
			STARTCONV555 = true;
			conv_pic555(PausePic);
			conv_pic555(ExitPic);
			conv_pic555(TrophyExit);
		}
		conv_pic555(MapPic);
		conv_pic555(BulletPic);
		conv_pic555(TrophyPic);
	}
}


bool Activate3DHardware()
{
	HRESULT hRes = CreateDirect3D(hwndMain);
	if (FAILED(hRes)) {
		throw dohalt("CreateDirect3D failed.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	hRes = CreateDevice((DWORD)WinW, (DWORD)WinH);
	if (FAILED(hRes)) {
		throw dohalt("Create Device failed", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	d3dClearBuffers();

	hRes = CreateScene();
	if (FAILED(hRes)) {
		throw dohalt("CreateScene Failed.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	d3dDetectCaps();


	LPDIRECTDRAWCOLORCONTROL lpCC;
	HRESULT hres = lpDD->QueryInterface(IID_IDirectDrawColorControl, (LPVOID*)&lpCC);

	std::cout << "QueryInterface returned: " << std::hex << hres << std::endl;

	if (lpCC) {
		std::cout << "ColorControl Started." << std::endl;
		DDCOLORCONTROL ColorControl;
		ColorControl.dwSize = sizeof(ColorControl);
		ColorControl.dwFlags = DDCOLOR_GAMMA;
		ColorControl.lGamma = 200;
		lpCC->SetColorControls(&ColorControl);
	}

	hRes = lpd3dDevice->BeginScene();

	switch (OptText) {
	case 0: LOWRESTX = true; break;
	case 1: LOWRESTX = false; break;
	case 2: LOWRESTX = false; break;
	}
	d3dMemLoaded = 0;

	D3DACTIVE = true;

	d3dLastTexture = d3dmemmapsize + 1;
	std::cout << "=== Direct3D started === \n" << std::endl;
	return true;
}


void ResetTextureMap()
{
	d3dEndBufferG();

	d3dMemUsageCount = 0;
	//d3dMemLoaded = 0;
	d3dLastTexture = d3dmemmapsize + 1;
	for (int m = 0; m < d3dmemmapsize + 2; m++) {
		d3dMemMap[m].lastused = 0;
		d3dMemMap[m].cpuaddr = 0;
		if (d3dMemMap[m].lpddTexture) {
			d3dMemMap[m].lpddTexture->Release();
			d3dMemMap[m].lpddTexture = NULL;
		}
	}
}



void ShutDown3DHardware()
{
	D3DACTIVE = false;

	if (lpd3dDevice)
		hRes = lpd3dDevice->EndScene();

	ResetTextureMap();

	lpInstructionG = NULL;
	lpVertexG = NULL;

	if (FULLSCREEN) {
		lpDD->RestoreDisplayMode();
	}

	if (NULL != lpd3dExecuteBuffer) {
		lpd3dExecuteBuffer->Release();
		lpd3dExecuteBufferG->Release();
		lpd3dExecuteBuffer = NULL;
	}

	if (NULL != lpd3dViewport) {
		lpd3dViewport->Release();
		lpd3dViewport = NULL;
	}

	if (NULL != lpd3dDevice) {
		lpd3dDevice->Release();
		lpd3dDevice = NULL;
	}

	if (NULL != lpddZBuffer) {
		lpddZBuffer->Release();
		lpddZBuffer = NULL;
	}

	if (NULL != lpddBack) {
		lpddBack->Release();
		lpddBack = NULL;
	}

	if (NULL != lpddPrimary) {
		lpddPrimary->Release();
		lpddPrimary = NULL;
	}

	if (NULL != lpd3d) {
		lpd3d->Release();
		lpd3d = NULL;
	}

	std::cout << "Successfully shutdown 3D hardware." << std::endl;
}





void InsertFxMM(int m)
{
	for (int mm = d3dmemmapsize - 1; mm > m; mm--)
		d3dMemMap[m] = d3dMemMap[m - 1];
}



bool d3dAllocTexture(int i, int w, int h)
{
	DDSURFACEDESC ddsd;
	DDPIXELFORMAT ddpf;
	ZeroMemory(&ddpf, sizeof(DDPIXELFORMAT));
	ddpf.dwSize = sizeof(DDPIXELFORMAT);
	ddpf.dwFlags = DDPF_RGB;

	if (OPT_ALPHA_COLORKEY) {
		ddpf.dwFlags |= DDPF_ALPHAPIXELS;
		ddpf.dwRGBAlphaBitMask = 0x8000;
	}

	ddpf.dwRGBBitCount = 16;
	ddpf.dwRBitMask = 0x7c00;
	ddpf.dwGBitMask = 0x3e0;
	ddpf.dwBBitMask = 0x1f;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;
	CopyMemory(&ddsd.ddpfPixelFormat, &ddpf, sizeof(DDPIXELFORMAT));

	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;

	hRes = lpDD->CreateSurface(&ddsd, &d3dMemMap[i].lpddTexture, NULL);
	if (FAILED(hRes)) {
		d3dMemMap[i].lpddTexture = NULL;
		return false;
	}


	DDCOLORKEY ddck;
	ddck.dwColorSpaceLowValue = ddck.dwColorSpaceHighValue = 0x0000;
	d3dMemMap[i].lpddTexture->SetColorKey(DDCKEY_SRCBLT, &ddck);

	return true;
}



void d3dDownLoadTexture(int i, int w, int h, LPVOID tptr)
{
	DDSURFACEDESC ddsd;

	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	/*
	if (OPT_ALPHA_COLORKEY)
	for (int d=0; d<w*w; d++) {
		WORD c = *((WORD*)tptr+d);
		if (c>0x7FFF) break;
		if ( (c & 0x7FFF) > 0)
		  *((WORD*)tptr+d) |= 0x8000;
	} else
	for (int d=0; d<w*w; d++) {
		WORD c = *((WORD*)tptr+d);
		if (c>0x7FFF) break;
		*((WORD*)tptr+d) |= 0x8000;
	}*/



	if (d3dMemMap[i].lpddTexture->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return;
	CopyMemory(ddsd.lpSurface, tptr, w * h * 2);
	d3dMemMap[i].lpddTexture->Unlock(ddsd.lpSurface);

	IDirect3DTexture* Texture;
	if (d3dMemMap[i].lpddTexture->QueryInterface(IID_IDirect3DTexture, (LPVOID*)&Texture) != S_OK) return;
	if (Texture->GetHandle(lpd3dDevice, &d3dMemMap[i].hTexture) != D3D_OK) return;
	Texture->Release();

	d3dMemMap[i].cpuaddr = (int)tptr;
	d3dMemMap[i].size = w * h * 2;
	d3dMemLoaded += w * h * 2;
	//---------------------------------------------------------------------------
}


int DownLoadTexture(LPVOID tptr, int w, int h)
{
	int textureSize = w * w * 2;
	int fxm = 0;
	int m;

	//========== if no memory used ==========//
	if (!d3dMemMap[0].cpuaddr) {
		d3dAllocTexture(0, w, h);
		d3dDownLoadTexture(0, w, h, tptr);
		return 0;
	}


	//====== search for last used block and try to alloc next ============//
	for (m = 0; m < d3dmemmapsize; m++)
		if (!d3dMemMap[m].cpuaddr)
			if (d3dAllocTexture(m, w, h)) {
				d3dDownLoadTexture(m, w, h, tptr);
				return m;
			}
			else break;



	//====== search for unused texture and replace it with new ============//
	int unusedtime = 2;
	int rt = -1;
	for (m = 0; m < d3dmemmapsize; m++) {
		if (!d3dMemMap[m].cpuaddr) break;
		if (d3dMemMap[m].size != w * h * 2) continue;

		int ut = d3dMemUsageCount - d3dMemMap[m].lastused;

		if (ut >= unusedtime) {
			unusedtime = ut;
			rt = m;
		}
	}

	if (rt != -1) {
		d3dDownLoadTexture(rt, w, h, tptr);
		return rt;
	}

	ResetTextureMap();

	d3dAllocTexture(0, w, h);
	d3dDownLoadTexture(0, w, h, tptr);
	return 0;
}




void d3dSetTexture(LPVOID tptr, int w, int h)
{

	if (d3dMemMap[d3dLastTexture].cpuaddr == (int)tptr) return;

	int fxm = -1;
	for (int m = 0; m < d3dmemmapsize; m++) {
		if (d3dMemMap[m].cpuaddr == (int)tptr) { fxm = m; break; }
		if (!d3dMemMap[m].cpuaddr) break;
	}

	if (fxm == -1) fxm = DownLoadTexture(tptr, w, h);

	d3dMemMap[fxm].lastused = d3dMemUsageCount;
	hTexture = d3dMemMap[fxm].hTexture;
	d3dLastTexture = fxm;


}






float GetTraceK(int x, int y)
{

	if (x<8 || y<8 || x>WinW - 8 || y>WinH - 8) return 0.f;

	float k = 0;

	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	if (lpddZBuffer->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) {
		return 0;
	}

	WORD CC = 200;
	int bw = (ddsd.lPitch >> 1);
	if (*((WORD*)ddsd.lpSurface + (y + 0) * bw + x + 0) < CC) k += 1.f;
	if (*((WORD*)ddsd.lpSurface + (y + 10) * bw + x + 0) < CC) k += 1.f;
	if (*((WORD*)ddsd.lpSurface + (y - 10) * bw + x + 0) < CC) k += 1.f;
	if (*((WORD*)ddsd.lpSurface + (y + 0) * bw + x + 10) < CC) k += 1.f;
	if (*((WORD*)ddsd.lpSurface + (y + 0) * bw + x - 10) < CC) k += 1.f;

	if (*((WORD*)ddsd.lpSurface + (y + 8) * bw + x + 8) < CC) k += 1.f;
	if (*((WORD*)ddsd.lpSurface + (y + 8) * bw + x - 8) < CC) k += 1.f;
	if (*((WORD*)ddsd.lpSurface + (y - 8) * bw + x + 8) < CC) k += 1.f;
	if (*((WORD*)ddsd.lpSurface + (y - 8) * bw + x - 8) < CC) k += 1.f;

	lpddZBuffer->Unlock(ddsd.lpSurface);
	k /= 9.f;

	DeltaFunc(TraceK, k, TimeDt / 1024.f);
	return TraceK;
}


void AddSkySum(WORD C)
{
	int R, G, B;

	if (VMFORMAT565) {
		R = C >> 11;  G = (C >> 5) & 63; B = C & 31;
	}
	else {
		R = C >> 10; G = (C >> 5) & 31; B = C & 31; C = C * 2;
	}

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

	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	if (lpddBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) {
		return 0;
	}

	int bw = (ddsd.lPitch >> 1);
	AddSkySum(*((WORD*)ddsd.lpSurface + (y + 0) * bw + x + 0));
	AddSkySum(*((WORD*)ddsd.lpSurface + (y + 6) * bw + x + 0));
	AddSkySum(*((WORD*)ddsd.lpSurface + (y - 6) * bw + x + 0));
	AddSkySum(*((WORD*)ddsd.lpSurface + (y + 0) * bw + x + 6));
	AddSkySum(*((WORD*)ddsd.lpSurface + (y + 0) * bw + x - 6));

	AddSkySum(*((WORD*)ddsd.lpSurface + (y + 4) * bw + x + 4));
	AddSkySum(*((WORD*)ddsd.lpSurface + (y + 4) * bw + x - 4));
	AddSkySum(*((WORD*)ddsd.lpSurface + (y - 4) * bw + x + 4));
	AddSkySum(*((WORD*)ddsd.lpSurface + (y - 4) * bw + x - 4));

	lpddBack->Unlock(ddsd.lpSurface);

	SkySumR -= SkyTR * 9;
	SkySumG -= SkyTG * 9;
	SkySumB -= SkyTB * 9;

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
	for (int m = 0; m < d3dmemmapsize; m++) {
		if (!d3dMemMap[m].cpuaddr) break;
		if (d3dMemMap[m].lastused + 2 >= d3dMemUsageCount)
			UsedMem += d3dMemMap[m].size;
	}
	/*
	  wsprintf(logt, "TOTALL: %d USED: %d", d3dTexturesMem, UsedMem);
	  AddMessage(logt);
	*/
	if (UsedMem * 4 < (int)d3dTexturesMem)
		LOWRESTX = FALSE;
}


/**
Swap video buffers and handle end-of-frame rendering
**/
void ShowVideo()
{
	/*
  char t[128];
  wsprintf(t, "T-mem loaded: %dK", d3dMemLoaded >> 10);
  if (d3dMemLoaded) AddMessage(t);
  */

	if (d3dMemLoaded > 200 * 1024)
		LowHardMemory++;
	else
		LowHardMemory = 0;

	if (LowHardMemory > 2) {
		LOWRESTX = true;
		LowHardMemory = 0;
	}

	switch (OptText) {
	case 0: LOWRESTX = true; break;
	case 1: LOWRESTX = false; break;
	case 2: {
		if (LOWRESTX && (Takt & 63) == 0)
			TryHiResTx();
	} break;
	}

	d3dMemUsageCount++;
	d3dMemLoaded = 0;

	hRes = lpd3dDevice->EndScene();

	hRes = lpddPrimary->Blt(NULL, lpddBack, NULL, DDBLT_WAIT, NULL);

	d3dClearBuffers();

	hRes = lpd3dDevice->BeginScene();
}


void CopyHARDToDIB()
{

}


void FXPutBitMap(int x0, int y0, int w, int h, int smw, LPVOID lpData)
{
	DDSURFACEDESC ddsd;
	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	if (lpddBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return;

	uint16_t* lpVMem = (uint16_t*)ddsd.lpSurface;
	ddsd.lPitch /= 2;
	lpVMem += x0 + y0 * ddsd.lPitch;

	for (int y = 0; y < h; y++)
		std::memcpy(lpVMem + y * ddsd.lPitch, ((uint16_t*)lpData) + y * smw, w * 2);

	lpddBack->Unlock(ddsd.lpSurface);
}



void DrawPicture(int x, int y, TPicture& pic)
{
	FXPutBitMap(x, y, pic.W, pic.H, pic.W, pic.lpImage);
}


/*
DEPRECATED
Print text to the screen
*/
void ddTextOut(int x, int y, const char *t, int color)
{
	if (t == nullptr) {
		return;
	}

	lpddBack->GetDC(&ddBackDC);
	SetBkMode(ddBackDC, TRANSPARENT);

	HFONT oldfont = NULL;

	if (SmallFont) {
		oldfont = static_cast<HFONT>(SelectObject(ddBackDC, fnt_Small));
	}

	SetTextColor(ddBackDC, 0x00101010);
	TextOut(ddBackDC, x + 2, y + 1, t, strlen(t));

	SetTextColor(ddBackDC, color);
	TextOut(ddBackDC, x + 1, y, t, strlen(t));

	if (SmallFont) {
		SelectObject(ddBackDC, oldfont);
	}

	lpddBack->ReleaseDC(ddBackDC);
}

/*
Print a C++ string to the screen
*/
void ddTextOut(int x, int y, const std::string &t, int color)
{
	lpddBack->GetDC(&ddBackDC);
	SetBkMode(ddBackDC, TRANSPARENT);

	HFONT oldfont = NULL;

	if (SmallFont) {
		oldfont = static_cast<HFONT>(SelectObject(ddBackDC, fnt_Small));
	}

	SetTextColor(ddBackDC, 0x00101010);
	TextOut(ddBackDC, x + 2, y + 1, t.c_str(), t.length());

	SetTextColor(ddBackDC, color);
	TextOut(ddBackDC, x + 1, y, t.c_str(), t.length());

	if (SmallFont) {
		SelectObject(ddBackDC, oldfont);
	}

	lpddBack->ReleaseDC(ddBackDC);
}


void DrawTrophyText(int x0, int y0)
{
	int x;
	SmallFont = true;
	HFONT oldfont = static_cast<HFONT>(SelectObject(hdcMain, fnt_Small));
	int tc = TrophyBody;
	std::stringstream ss;

	int   dtype = TrophyRoom.Body[tc].ctype;
	int   time = TrophyRoom.Body[tc].time;
	int   date = TrophyRoom.Body[tc].date;
	int   wep = TrophyRoom.Body[tc].weapon;
	int   score = TrophyRoom.Body[tc].score;
	float scale = TrophyRoom.Body[tc].scale;
	float range = TrophyRoom.Body[tc].range;

	x0 += 14; y0 += 18;
	x = x0;
	ddTextOut(x, y0, "Name: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Name: ");
	ddTextOut(x, y0, DinoInfo[dtype].Name, 0x0000BFBF);

	x = x0;
	ddTextOut(x, y0 + 16, "Weight: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Weight: ");
	if (OptSys)
		ss << std::setprecision(3) << (DinoInfo[dtype].Mass * scale * scale / 0.907f) << "t";
	else
		ss << std::setprecision(3) << (DinoInfo[dtype].Mass * scale * scale) << "T";

	ddTextOut(x, y0 + 16, ss.str().c_str(), 0x0000BFBF);    x += GetTextW(hdcMain, ss.str());
	ss.clear();

	ddTextOut(x, y0 + 16, "Length: ", 0x00BFBFBF); x += GetTextW(hdcMain, "Length: ");

	if (OptSys)
		ss << std::setprecision(2) << (DinoInfo[dtype].Length * scale / 0.3f) << "ft";
	else
		ss << std::setprecision(2) << (DinoInfo[dtype].Length * scale) << "m";

	ddTextOut(x, y0 + 16, ss.str().c_str(), 0x0000BFBF);
	ss.clear();

	x = x0;
	ddTextOut(x, y0 + 32, "Weapon: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Weapon: ");
	ss << WeapInfo[wep].Name << "    ";
	ddTextOut(x, y0 + 32, ss.str().c_str(), 0x0000BFBF);   x += GetTextW(hdcMain, ss.str()); 
	ss.clear();

	ddTextOut(x, y0 + 32, "Score: ", 0x00BFBFBF);   x += GetTextW(hdcMain, "Score: ");
	ss << score;
	ddTextOut(x, y0 + 32, ss.str().c_str(), 0x0000BFBF);
	ss.clear();

	x = x0;
	ddTextOut(x, y0 + 48, "Range of kill: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Range of kill: ");
	if (OptSys) ss << std::setprecision(2) << (range / 0.3f) << "ft";
	else        ss << std::setprecision(2) << (range) << "ft";
	ddTextOut(x, y0 + 48, ss.str().c_str(), 0x0000BFBF);
	ss.clear();


	x = x0;
	ddTextOut(x, y0 + 64, "Date: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Date: ");
	if (OptSys)
		ss << ((date >> 10) & 255) << "."  << (date & 255) << "." << (date >> 20) << "   ";
	else
		ss << (date & 255) << "." << ((date >> 10) & 255) << "." << (date >> 20) << "   ";

	ddTextOut(x, y0 + 64, ss.str().c_str(), 0x0000BFBF);   x += GetTextW(hdcMain, ss.str());
	ss.clear();

	ddTextOut(x, y0 + 64, "Time: ", 0x00BFBFBF);   x += GetTextW(hdcMain, "Time: ");
	ss << ((time >> 10) & 255) << ":" << std::setfill('0') << std::setw(2) << (time & 255);
	ddTextOut(x, y0 + 64, ss.str().c_str(), 0x0000BFBF);

	SmallFont = FALSE;

	SelectObject(hdcMain, oldfont);

}




void Render_LifeInfo(int li)
{
	int x, y;
	SmallFont = TRUE;
	//HFONT oldfont = SelectObject(hdcMain, fnt_Small);

	int    ctype = Characters[li].CType;
	float  scale = Characters[li].scale;

	x = VideoCX + WinW / 64;
	y = VideoCY + (int)(WinH / 6.8);

	ddTextOut(x, y, DinoInfo[ctype].Name, 0x0000b000);

	std::stringstream ss;

	if (OptSys) ss << "Weight: " << std::setprecision(3) << (DinoInfo[ctype].Mass * scale * scale / 0.907) << "t";
	else        ss << "Weight: " << std::setprecision(3) << (DinoInfo[ctype].Mass * scale * scale) << "T";

	ddTextOut(x, y + 16, ss.str().c_str(), 0x0000b000);

	SmallFont = FALSE;
	//SelectObject(hdcMain, oldfont);	
}


void RenderFXMMap()
{

}

/**
Render text-based HUD elements
**/
void ShowControlElements()
{
	std::stringstream ss;

	if (TIMER) {
		ss << "Frame Delta: " << TimeDt << "ms";
		ddTextOut(WinEX - 81, 11, ss.str(), 0x0020A0A0);
		ss.clear();

		ss << "Polygon Count: " << dFacesCount;
		ddTextOut(WinEX - 90, 24, ss.str(), 0x0020A0A0);
		ss.clear();
	}

	if (MessageList.timeleft) {
		if (RealTime > MessageList.timeleft) MessageList.timeleft = 0;
		ddTextOut(10, 10, MessageList.mtext, 0x0020A0A0);
	}

	if (ExitTime) {
		int y = WinH / 3;
		ss << "Preparing for evacuation...";
		ddTextOut(VideoCX - GetTextW(hdcCMain, ss.str()) / 2, y, ss.str().c_str(), 0x0060C0D0);
		ss.clear();

		ss << (1 + ExitTime / 1000) << " seconds left.";
		ddTextOut(VideoCX - GetTextW(hdcCMain, ss.str()) / 2, y + 18, ss.str().c_str(), 0x0060C0D0);
	}

	if (UNDERWATER)
		RenderFSRect(0x90004050);

	if (!UNDERWATER && (SunLight > 1.0f)) {
		RenderFSRect(0xFFFFC0 + ((int)SunLight << 24));
	}


	RenderHealthBar();
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
		hleft.tx = cp[vn].tx + ((cp[vleft].tx - cp[vn].tx) * lc);
		hleft.ty = cp[vn].ty + ((cp[vleft].ty - cp[vn].ty) * lc);
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
		hright.tx = cp[vn].tx + ((cp[vright].tx - cp[vn].tx) * lc);
		hright.ty = cp[vn].ty + ((cp[vright].ty - cp[vn].ty) * lc);
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



void DrawTPlaneClip(bool SECONT)
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

	if (WATERREVERSE) dy = 0;
	for (u = 0; u < vused; u++) {
		cp[u].ev.scrx = (VideoCX << 4) - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW * 16.f);
		cp[u].ev.scry = (VideoCY << 4) + (int)(dy + cp[u].ev.v.y / cp[u].ev.v.z * CameraH * 16.f);
	}


	if (!lpVertexG)
		d3dStartBufferG();

	if (GVCnt > 380) {
		if (lpVertexG) d3dEndBufferG();
		d3dStartBufferG();
	}

	if (hGTexture != hTexture) {
		hGTexture = hTexture;
		lpInstructionG->bOpcode = D3DOP_STATERENDER;
		lpInstructionG->bSize = sizeof(D3DSTATE);
		lpInstructionG->wCount = 1;
		lpInstructionG++;
		lpState = (LPD3DSTATE)lpInstructionG;
		lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREHANDLE;
		lpState->dwArg[0] = hTexture;
		lpState++;
		lpInstructionG = (LPD3DINSTRUCTION)lpState;

		lpwTriCount = &(lpInstructionG->wCount);
		lpInstructionG->bOpcode = D3DOP_TRIANGLE;
		lpInstructionG->bSize = sizeof(D3DTRIANGLE);
		lpInstructionG->wCount = 0;
		lpInstructionG++;

	}


	lpTriangle = (LPD3DTRIANGLE)lpInstructionG;

	for (u = 0; u < vused - 2; u++) {

		lpVertexG->sx = (float)cp[0].ev.scrx / 16.f;
		lpVertexG->sy = (float)cp[0].ev.scry / 16.f;
		lpVertexG->sz = -8.0f / cp[0].ev.v.z;
		lpVertexG->rhw = lpVertexG->sz * 0.125f;
		lpVertexG->color = (int)(255 - cp[0].ev.Light * 4) * 0x00010101 | (WaterAlphaL << 24);
		lpVertexG->specular = (255 - (int)cp[0].ev.Fog) << 24;//0x7F000000;
		lpVertexG->tu = (float)(cp[0].tx) / (128.f * 65536.f);
		lpVertexG->tv = (float)(cp[0].ty) / (128.f * 65536.f);
		lpVertexG++;

		lpVertexG->sx = (float)cp[u + 1].ev.scrx / 16.f;
		lpVertexG->sy = (float)cp[u + 1].ev.scry / 16.f;
		lpVertexG->sz = -8.0f / cp[u + 1].ev.v.z;
		lpVertexG->rhw = lpVertexG->sz * 0.125f;
		lpVertexG->color = (int)(255 - cp[u + 1].ev.Light * 4) * 0x00010101 | (WaterAlphaL << 24);
		lpVertexG->specular = (255 - (int)cp[u + 1].ev.Fog) << 24;//0x7F000000;
		lpVertexG->tu = (float)(cp[u + 1].tx) / (128.f * 65536.f);
		lpVertexG->tv = (float)(cp[u + 1].ty) / (128.f * 65536.f);
		lpVertexG++;

		lpVertexG->sx = (float)cp[u + 2].ev.scrx / 16.f;
		lpVertexG->sy = (float)cp[u + 2].ev.scry / 16.f;
		lpVertexG->sz = -8.0f / cp[u + 2].ev.v.z;
		lpVertexG->rhw = lpVertexG->sz * 0.125f;
		lpVertexG->color = (int)(255 - cp[u + 2].ev.Light * 4) * 0x00010101 | (WaterAlphaL << 24);
		lpVertexG->specular = (255 - (int)cp[u + 2].ev.Fog) << 24;//0x7F000000;
		lpVertexG->tu = (float)(cp[u + 2].tx) / (128.f * 65536.f);
		lpVertexG->tv = (float)(cp[u + 2].ty) / (128.f * 65536.f);
		lpVertexG++;

		lpTriangle->wV1 = GVCnt;
		lpTriangle->wV2 = GVCnt + 1;
		lpTriangle->wV3 = GVCnt + 2;
		lpTriangle->wFlags = 0;
		lpTriangle++;
		*lpwTriCount = (*lpwTriCount) + 1;

		GVCnt += 3;
	}

	lpInstructionG = (LPD3DINSTRUCTION)lpTriangle;
}








void DrawTPlane(bool SECONT)
{
	int n;
	BOOL SecondPass = FALSE;

	if (!WATERREVERSE) {
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


	int alpha1 = 255;
	int alpha2 = 255;
	int alpha3 = 255;

	if (!WATERREVERSE)
		if (zs > (ctViewR - 8) << 8) {
			SecondPass = TRUE;

			int zz;
			zz = (int)VectorLength(ev[0].v) - 256 * (ctViewR - 4);
			if (zz > 0) alpha1 = max(0, 255 - zz / 3); else alpha1 = 255;

			zz = (int)VectorLength(ev[1].v) - 256 * (ctViewR - 4);
			if (zz > 0) alpha2 = max(0, 255 - zz / 3); else alpha2 = 255;

			zz = (int)VectorLength(ev[2].v) - 256 * (ctViewR - 4);
			if (zz > 0) alpha3 = max(0, 255 - zz / 3); else alpha3 = 255;
		}


	if (alpha1 > WaterAlphaL) alpha1 = WaterAlphaL;
	if (alpha2 > WaterAlphaL) alpha2 = WaterAlphaL;
	if (alpha3 > WaterAlphaL) alpha3 = WaterAlphaL;


	if (!lpVertexG)
		d3dStartBufferG();

	if (GVCnt > 380) {
		if (lpVertexG) d3dEndBufferG();
		d3dStartBufferG();
	}

	lpVertexG->sx = (float)scrp[0].x / 16.f;
	lpVertexG->sy = (float)scrp[0].y / 16.f;
	lpVertexG->sz = -8.f / ev[0].v.z;
	lpVertexG->rhw = lpVertexG->sz * 0.125f;
	lpVertexG->color = (int)(255 - scrp[0].Light * 4) * 0x00010101 | alpha1 << 24;
	lpVertexG->specular = (255 - (int)ev[0].Fog) << 24;//0x7F000000;
	lpVertexG->tu = (float)(scrp[0].tx) / (128.f * 65536.f);
	lpVertexG->tv = (float)(scrp[0].ty) / (128.f * 65536.f);
	lpVertexG++;

	lpVertexG->sx = (float)scrp[1].x / 16.f;
	lpVertexG->sy = (float)scrp[1].y / 16.f;
	lpVertexG->sz = -8.f / ev[1].v.z;
	lpVertexG->rhw = lpVertexG->sz * 0.125f;
	lpVertexG->color = (int)(255 - scrp[1].Light * 4) * 0x00010101 | alpha2 << 24;
	lpVertexG->specular = (255 - (int)ev[1].Fog) << 24;//0x7F000000;
	lpVertexG->tu = (float)(scrp[1].tx) / (128.f * 65536.f);
	lpVertexG->tv = (float)(scrp[1].ty) / (128.f * 65536.f);
	lpVertexG++;

	lpVertexG->sx = (float)scrp[2].x / 16.f;
	lpVertexG->sy = (float)scrp[2].y / 16.f;
	lpVertexG->sz = -8.f / ev[2].v.z;
	lpVertexG->rhw = lpVertexG->sz * 0.125f;
	lpVertexG->color = (int)(255 - scrp[2].Light * 4) * 0x00010101 | alpha3 << 24;
	lpVertexG->specular = (255 - (int)ev[2].Fog) << 24;//0x7F000000;
	lpVertexG->tu = (float)(scrp[2].tx) / (128.f * 65536.f);
	lpVertexG->tv = (float)(scrp[2].ty) / (128.f * 65536.f);
	lpVertexG++;

	if (hGTexture != hTexture) {
		hGTexture = hTexture;
		lpInstructionG->bOpcode = D3DOP_STATERENDER;
		lpInstructionG->bSize = sizeof(D3DSTATE);
		lpInstructionG->wCount = 1;
		lpInstructionG++;
		lpState = (LPD3DSTATE)lpInstructionG;
		lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREHANDLE;
		lpState->dwArg[0] = hTexture;
		lpState++;
		lpInstructionG = (LPD3DINSTRUCTION)lpState;

		lpwTriCount = (&lpInstructionG->wCount);
		lpInstructionG->bOpcode = D3DOP_TRIANGLE;
		lpInstructionG->bSize = sizeof(D3DTRIANGLE);
		lpInstructionG->wCount = 0;
		lpInstructionG++;
	}

	lpTriangle = (LPD3DTRIANGLE)lpInstructionG;
	lpTriangle->wV1 = GVCnt;
	lpTriangle->wV2 = GVCnt + 1;
	lpTriangle->wV3 = GVCnt + 2;
	lpTriangle->wFlags = 0;
	lpTriangle++;
	*lpwTriCount = (*lpwTriCount) + 1;
	lpInstructionG = (LPD3DINSTRUCTION)lpTriangle;

	GVCnt += 3;
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

	WaterAlphaL = 0x70;
	if (zs > ctViewR * 128) WaterAlphaL -= (WaterAlphaL - 0x10) * (zs - ctViewR * 128) / (ctViewR * 128);

	d3dSetTexture(Textures[0].getData(), 128, 128);

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

	WaterAlphaL = 255;

}



void ProcessMap(int x, int y, int r)
{
	WATERREVERSE = false;
	if (x >= ctMapSize - 1 || y >= ctMapSize - 1 ||
		x < 0 || y < 0) return;

	ev[0] = VMap[y - CCY + 64][x - CCX + 64];
	if (ev[0].v.z > BackViewR) return;

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
		NeedWater = true;
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

	if (std::fabs(xx) > -zz + BackViewR) return;


	zs = (int)std::sqrtf(xx * xx + zz * zz + yy * yy);
	if (zs > ctViewR * 256) return;

	GT = 0;
	FadeL = 0;
	GlassL = 0;

	if (t1 == 0 || t2 == 0) NeedWater = true;
	if (zs > 256 * (ctViewR - 8)) {
		FadeL = (zs - 256 * (ctViewR - 8)) / 4;
		if (FadeL > 255) { GlassL = min(255, FadeL - 255); FadeL = 255; }
	}

	//grConstantColorValue( (255-GlassL) << 24);   

	if (MIPMAP && (zs > 256 * 10 && t1 || LOWRESTX)) {
		int TW, TH;
		Textures[t1].getMipmap().getDimensions(TW, TH);
		d3dSetTexture(Textures[t1].getMipmap().getData(), TW, TH);
	}
	else {
		int TW, TH;
		Textures[t1].getDimensions(TW, TH);
		d3dSetTexture(Textures[t1].getData(), 128, 128);
	}

	if (r > 8) DrawTPlane(FALSE);
	else DrawTPlaneClip(FALSE);

	if (ReverseOn) { ev[0] = ev[2]; ev[2] = VMap[y + 1][x + 1]; }
	else { ev[1] = ev[2]; ev[2] = VMap[y + 1][x]; }


	if (MIPMAP && (zs > 256 * 10 && t2 || LOWRESTX)) {
		int TW, TH;
		Textures[t2].getMipmap().getDimensions(TW, TH);
		d3dSetTexture(Textures[t2].getMipmap().getData(), TW, TH);
	}
	else {
		int TW, TH;
		Textures[t2].getDimensions(TW, TH);
		d3dSetTexture(Textures[t2].getData(), 128, 128);
	}

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

		v[0] = RotateVector(v[0]);


		if (MObjects[ob].info.flags & ofANIMATED)
			if (MObjects[ob].info.LastAniTime != RealTime) {
				MObjects[ob].info.LastAniTime = RealTime;
				CreateMorphedObject(&MObjects[ob].model,
					MObjects[ob].vtl,
					RealTime % MObjects[ob].vtl.AniTime);
			}

		if (v[0].z < -256 * 8)
			RenderModel(&MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, CameraAlpha, CameraBeta);
		else
			RenderModelClip(&MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, CameraAlpha, CameraBeta);

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

	if (std::fabs(y0) > -(z0 - 256 * 6)) return;

	mptr = _mptr;

	float ca = std::cosf(al);
	float sa = std::sinf(al);

	float cb = std::cosf(bt);
	float sb = std::sinf(bt);

	int minx = 10241024;
	int maxx = -10241024;
	int miny = 10241024;
	int maxy = -10241024;

	bool FOGACTIVE = (FOGON && (FogYBase > 0));

	int alphamask = (255 - GlassL) << 24;
	int ml = (255 - light * 4);

	TPoint3d p;
	for (int s = 0; s < mptr->VCount; s++) {
		p = mptr->gVertex[s];

		if (FOGACTIVE) {
			vFogT[s] = 255 - (int)(FogYBase + p.y * FogYGrad);
			if (vFogT[s] < 5) vFogT[s] = 5;
			if (vFogT[s] > 255) vFogT[s] = 255;
			vFogT[s] <<= 24;
		}
		else vFogT[s] = 255 << 24;



		rVertex[s].x = (p.x * ca + p.z * sa) + x0;

		float vz = p.z * ca - p.x * sa;

		rVertex[s].y = (p.y * cb - vz * sb) + y0;
		rVertex[s].z = (vz * cb + p.y * sb) + z0;

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

	float d = std::sqrtf(x0 * x0 + y0 * y0 + z0 * z0);

	TTexture* tptr = nullptr;

	if (MIPMAP && (d > 12 * 256)) {
		tptr = &mptr->texture.getMipmap();
	}
	else {
		tptr = &mptr->texture;
	}

	tptr = &mptr->texture; // temp

	d3dSetTexture(tptr->getData(), tptr->getWidth(), tptr->getHeight());

	int PrevOpacity = 0;
	int NewOpacity = 0;
	int PrevTransparent = 0;
	int NewTransparent = 0;

	d3dStartBuffer();

	int fproc1 = 0;
	int fproc2 = 0;
	f = Current;
	BOOL CKEY = FALSE;
	while (f != -1) {
		TFace* fptr = &mptr->gFace[f];

		if (fptr->Flags & (sfOpacity | sfTransparent)) fproc2++; else fproc1++;

		int _ml = ml + mptr->VLight[fptr->v1];
		lpVertex->sx = (float)gScrp[fptr->v1].x;
		lpVertex->sy = (float)gScrp[fptr->v1].y;
		lpVertex->sz = -8.f / rVertex[fptr->v1].z;
		lpVertex->rhw = 1.f;
		lpVertex->color = _ml * 0x00010101 | alphamask;
		lpVertex->specular = vFogT[fptr->v1];
		lpVertex->tu = (float)(fptr->tax);
		lpVertex->tv = (float)(fptr->tay);
		lpVertex++;

		_ml = ml + mptr->VLight[fptr->v2];
		lpVertex->sx = (float)gScrp[fptr->v2].x;
		lpVertex->sy = (float)gScrp[fptr->v2].y;
		lpVertex->sz = -8.f / rVertex[fptr->v2].z;
		lpVertex->rhw = 1.f;
		lpVertex->color = _ml * 0x00010101 | alphamask;;
		lpVertex->specular = vFogT[fptr->v2];
		lpVertex->tu = (float)(fptr->tbx);
		lpVertex->tv = (float)(fptr->tby);
		lpVertex++;

		_ml = ml + mptr->VLight[fptr->v3];
		lpVertex->sx = (float)gScrp[fptr->v3].x;
		lpVertex->sy = (float)gScrp[fptr->v3].y;
		lpVertex->sz = -8.f / rVertex[fptr->v3].z;
		lpVertex->rhw = 1.f;
		lpVertex->color = _ml * 0x00010101 | alphamask;;
		lpVertex->specular = vFogT[fptr->v3];
		lpVertex->tu = (float)(fptr->tcx);
		lpVertex->tv = (float)(fptr->tcy);
		lpVertex++;


		f = mptr->gFace[f].Next;
	}

	d3dFlushBuffer(fproc1, fproc2);
}







void RenderShadowClip(TModel* _mptr,
	float xm0, float ym0, float zm0,
	float x0, float y0, float z0, float cal, float al, float bt)
{
	int f, CMASK, j;
	mptr = _mptr;

	float cla = (float)cos(cal);
	float sla = (float)sin(cal);

	float ca = (float)cos(al);
	float sa = (float)sin(al);

	float cb = (float)cos(bt);
	float sb = (float)sin(bt);


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
	if (MIPMAP && (d > 12 * 256))
		d3dSetTexture(mptr->texture.getMipmap().getData(), 128, 128);
	else
		d3dSetTexture(mptr->texture.getData(), 256, 256);

	BuildTreeClipNoSort();

	d3dStartBuffer();
	int fproc1 = 0;

	f = Current;
	while (f != -1) {

		vused = 3;
		TFace* fptr = &mptr->gFace[f];

		CMASK = 0;
		CMASK |= gScrp[fptr->v1].y;
		CMASK |= gScrp[fptr->v2].y;
		CMASK |= gScrp[fptr->v3].y;

		cp[0].ev.v = rVertex[fptr->v1];  cp[0].tx = (fptr->tax / 256.0f);  cp[0].ty = (fptr->tay / 256.0f);
		cp[1].ev.v = rVertex[fptr->v2];  cp[1].tx = (fptr->tbx / 256.0f);  cp[1].ty = (fptr->tby / 256.0f);
		cp[2].ev.v = rVertex[fptr->v3];  cp[2].tx = (fptr->tcx / 256.0f);  cp[2].ty = (fptr->tcy / 256.0f);

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

		for (j = 0; j < vused - 2; j++) {
			u = 0;
			lpVertex->sx = (float)(VideoCX - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW));
			lpVertex->sy = (float)(VideoCY + (int)(cp[u].ev.v.y / cp[u].ev.v.z * CameraH));
			lpVertex->sz = -8.5f / cp[u].ev.v.z;
			lpVertex->rhw = 1.f;
			lpVertex->color = GlassL;
			lpVertex->specular = 0xFF000000;
			lpVertex->tu = 0.f;
			lpVertex->tv = 0.f;
			lpVertex++;

			u = j + 1;
			lpVertex->sx = (float)(VideoCX - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW));
			lpVertex->sy = (float)(VideoCY + (int)(cp[u].ev.v.y / cp[u].ev.v.z * CameraH));
			lpVertex->sz = -8.5f / cp[u].ev.v.z;
			lpVertex->rhw = 1.f;
			lpVertex->color = GlassL;
			lpVertex->specular = 0xFF000000;
			lpVertex->tu = 0.f;
			lpVertex->tv = 0.f;
			lpVertex++;

			u = j + 2;
			lpVertex->sx = (float)(VideoCX - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW));
			lpVertex->sy = (float)(VideoCY + (int)(cp[u].ev.v.y / cp[u].ev.v.z * CameraH));
			lpVertex->sz = -8.5f / cp[u].ev.v.z;
			lpVertex->rhw = 1.f;
			lpVertex->color = GlassL;
			lpVertex->specular = 0xFF000000;
			lpVertex->tu = 0.f;
			lpVertex->tv = 0.f;
			lpVertex++;
			fproc1++;
		}



	LNEXT:
		f = mptr->gFace[f].Next;
	}


	d3dFlushBuffer(fproc1, 0);

}





void RenderModelClip(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	int f, CMASK;

	if (std::fabs(y0) > -(z0 - 256.f * 6.f)) return;

	mptr = _mptr;

	float ca = std::cosf(al);
	float sa = std::sinf(al);

	float cb = std::cosf(bt);
	float sb = std::sinf(bt);


	int flight = (int)(255 - light * 4);
	int almask;


	bool BL = false;
	bool FOGACTIVE = (FOGON && (FogYBase > 0));

	for (int s = 0; s < mptr->VCount; s++) {

		if (FOGACTIVE) {
			vFogT[s] = 255 - (int)(FogYBase + mptr->gVertex[s].y * FogYGrad);
			if (vFogT[s] < 5) vFogT[s] = 5;
			if (vFogT[s] > 255) vFogT[s] = 255;
			vFogT[s] <<= 24;
		}
		else vFogT[s] = 255 << 24;


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

	d3dSetTexture(mptr->texture.getData(), mptr->texture.getWidth(), mptr->texture.getHeight());

	BuildTreeClipNoSort();

	d3dStartBuffer();

	f = Current;
	int fproc1 = 0;
	int fproc2 = 0;
	BOOL CKEY = FALSE;

	while (f != -1) {

		vused = 3;
		TFace* fptr = &mptr->gFace[f];

		CMASK = 0;

		CMASK |= gScrp[fptr->v1].y;
		CMASK |= gScrp[fptr->v2].y;
		CMASK |= gScrp[fptr->v3].y;


		cp[0].ev.v = rVertex[fptr->v1];
		cp[0].tx = (float)(fptr->tax); 
		cp[0].ty = ((float)fptr->tay);
		cp[0].ev.Fog = vFogT[fptr->v1];
		cp[0].ev.Light = mptr->VLight[fptr->v1];

		cp[1].ev.v = rVertex[fptr->v2];
		cp[1].tx = (float)(fptr->tbx);
		cp[1].ty = ((float)fptr->tby);
		cp[1].ev.Fog = vFogT[fptr->v2];
		cp[1].ev.Light = mptr->VLight[fptr->v2];
		
		cp[2].ev.v = rVertex[fptr->v3];
		cp[2].tx = (float)(fptr->tcx);
		cp[2].ty = ((float)fptr->tcy);
		cp[2].ev.Fog = vFogT[fptr->v3];
		cp[2].ev.Light = mptr->VLight[fptr->v3];

		for (u = 0; u < vused; u++) cp[u].ev.v.z += 16.0f;
		for (u = 0; u < vused; u++) ClipVector(ClipZ, u);
		for (u = 0; u < vused; u++) cp[u].ev.v.z -= 16.0f;
		if (vused < 3) goto LNEXT;

		if (CMASK & 1) for (u = 0; u < vused; u++) ClipVector(ClipA, u); if (vused < 3) goto LNEXT;
		if (CMASK & 2) for (u = 0; u < vused; u++) ClipVector(ClipC, u); if (vused < 3) goto LNEXT;
		if (CMASK & 4) for (u = 0; u < vused; u++) ClipVector(ClipB, u); if (vused < 3) goto LNEXT;
		if (CMASK & 8) for (u = 0; u < vused; u++) ClipVector(ClipD, u); if (vused < 3) goto LNEXT;

		almask = 0xFF000000;

		if (fptr->Flags & sfTransparent) almask = 0x70000000;

		for (u = 0; u < vused - 2; u++) {
			int _flight = flight + cp[0].ev.Light;
			lpVertex->sx = (float)(VideoCX - (int)(cp[0].ev.v.x / cp[0].ev.v.z * CameraW));
			lpVertex->sy = (float)(VideoCY + (int)(cp[0].ev.v.y / cp[0].ev.v.z * CameraH));
			lpVertex->sz = -8.f / cp[0].ev.v.z;
			lpVertex->rhw = lpVertex->sz * 0.125f;
			lpVertex->color = _flight * 0x00010101 | almask;
			lpVertex->specular = (int)cp[0].ev.Fog;
			lpVertex->tu = (float)(cp[0].tx);
			lpVertex->tv = (float)(cp[0].ty);
			lpVertex++;

			_flight = flight + cp[u + 1].ev.Light;
			lpVertex->sx = (float)(VideoCX - (int)(cp[u + 1].ev.v.x / cp[u + 1].ev.v.z * CameraW));
			lpVertex->sy = (float)(VideoCY + (int)(cp[u + 1].ev.v.y / cp[u + 1].ev.v.z * CameraH));
			lpVertex->sz = -8.f / cp[u + 1].ev.v.z;
			lpVertex->rhw = lpVertex->sz * 0.125f;
			lpVertex->color = _flight * 0x00010101 | almask;
			lpVertex->specular = (int)cp[u + 1].ev.Fog;
			lpVertex->tu = (float)(cp[u + 1].tx);
			lpVertex->tv = (float)(cp[u + 1].ty);
			lpVertex++;

			_flight = flight + cp[u + 2].ev.Light;
			lpVertex->sx = (float)(VideoCX - (int)(cp[u + 2].ev.v.x / cp[u + 2].ev.v.z * CameraW));
			lpVertex->sy = (float)(VideoCY + (int)(cp[u + 2].ev.v.y / cp[u + 2].ev.v.z * CameraH));
			lpVertex->sz = -8.f / cp[u + 2].ev.v.z;
			lpVertex->rhw = lpVertex->sz * 0.125f;
			lpVertex->color = _flight * 0x00010101 | almask;
			lpVertex->specular = (int)cp[u + 2].ev.Fog;
			lpVertex->tu = (float)(cp[u + 2].tx);
			lpVertex->tv = (float)(cp[u + 2].ty);
			lpVertex++;

			if (fptr->Flags & (sfOpacity | sfTransparent)) fproc2++; else fproc1++;
		}
	LNEXT:
		f = mptr->gFace[f].Next;
	}

	d3dFlushBuffer(fproc1, fproc2);

}






void RenderModelSun(TModel* _mptr, float x0, float y0, float z0, int Alpha)
{
	int f;

	mptr = _mptr;

	int minx = 10241024;
	int maxx = -10241024;
	int miny = 10241024;
	int maxy = -10241024;


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

	d3dSetTexture(mptr->texture.getMipmap().getData(), 128, 128);

	d3dStartBuffer();

	DWORD alpha = Alpha;
	alpha = (alpha << 24) | 0x00FFFFFF;
	int fproc1 = 0;
	f = Current;
	while (f != -1) {

		TFace* fptr = &mptr->gFace[f];

		fproc1++;

		lpVertex->sx = (float)gScrp[fptr->v1].x;
		lpVertex->sy = (float)gScrp[fptr->v1].y;
		lpVertex->sz = -8.f / rVertex[fptr->v1].z;
		lpVertex->rhw = 1.f;
		lpVertex->color = alpha;
		lpVertex->specular = 0xFF000000;
		lpVertex->tu = (float)(fptr->tax);
		lpVertex->tv = (float)(fptr->tay);
		lpVertex++;

		lpVertex->sx = (float)gScrp[fptr->v2].x;
		lpVertex->sy = (float)gScrp[fptr->v2].y;
		lpVertex->sz = -8.f / rVertex[fptr->v2].z;
		lpVertex->rhw = 1.f;
		lpVertex->color = alpha;
		lpVertex->specular = 0xFF000000;
		lpVertex->tu = (float)(fptr->tbx);
		lpVertex->tv = (float)(fptr->tby);
		lpVertex++;

		lpVertex->sx = (float)gScrp[fptr->v3].x;
		lpVertex->sy = (float)gScrp[fptr->v3].y;
		lpVertex->sz = -8.f / rVertex[fptr->v3].z;
		lpVertex->rhw = 1.f;
		lpVertex->color = alpha;
		lpVertex->specular = 0xFF000000;
		lpVertex->tu = (float)(fptr->tcx);
		lpVertex->tv = (float)(fptr->tcy);
		lpVertex++;

		f = mptr->gFace[f].Next;
	}

	lpInstruction = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDesc.lpData + 1024 * 3);
	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 2;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREHANDLE;
	lpState->dwArg[0] = hTexture;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_DESTBLEND;
	lpState->dwArg[0] = D3DBLEND_ONE;
	lpState++;



	lpInstruction = (LPD3DINSTRUCTION)lpState;
	lpInstruction->bOpcode = D3DOP_PROCESSVERTICES;
	lpInstruction->bSize = sizeof(D3DPROCESSVERTICES);
	lpInstruction->wCount = 1U;
	lpInstruction++;

	lpProcessVertices = (LPD3DPROCESSVERTICES)lpInstruction;
	lpProcessVertices->dwFlags = D3DPROCESSVERTICES_COPY;
	lpProcessVertices->wStart = 0U;
	lpProcessVertices->wDest = 0U;
	lpProcessVertices->dwCount = fproc1 * 3;
	lpProcessVertices->dwReserved = 0UL;
	lpProcessVertices++;

	lpInstruction = (LPD3DINSTRUCTION)lpProcessVertices;
	lpInstruction->bOpcode = D3DOP_TRIANGLE;
	lpInstruction->bSize = sizeof(D3DTRIANGLE);
	lpInstruction->wCount = fproc1;
	lpInstruction++;
	lpTriangle = (LPD3DTRIANGLE)lpInstruction;

	int ii = 0;
	for (int i = 0; i < fproc1; i++) {
		lpTriangle->wV1 = ii++;
		lpTriangle->wV2 = ii++;
		lpTriangle->wV3 = ii++;
		lpTriangle->wFlags = 0;
		lpTriangle++;
	}

	lpInstruction = (LPD3DINSTRUCTION)lpTriangle;

	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 1;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_DESTBLEND;
	lpState->dwArg[0] = D3DBLEND_INVSRCALPHA;
	lpState++;

	lpInstruction = (LPD3DINSTRUCTION)lpState;

	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 3;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_COLORKEYENABLE;
	lpState->dwArg[0] = FALSE;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMAG;
	lpState->dwArg[0] = D3DFILTER_LINEAR;
	lpState++;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREMIN;
	lpState->dwArg[0] = D3DFILTER_LINEAR;
	lpState++;
	lpInstruction = (LPD3DINSTRUCTION)lpState;



	lpInstruction->bOpcode = D3DOP_EXIT;
	lpInstruction->bSize = 0UL;
	lpInstruction->wCount = 0U;

	lpd3dExecuteBuffer->Unlock();

	hRes = lpd3dDevice->Execute(lpd3dExecuteBuffer, lpd3dViewport, D3DEXECUTE_UNCLIPPED);
	//if (FAILED(hRes)) DoHalt("Error execute buffer");   
	dFacesCount += fproc1;

}






void RenderNearModel(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	BOOL b = LOWRESTX;
	Vector3d v;
	v.x = 0; v.y = -128; v.z = 0;

	CalcFogLevel_Gradient(v);
	FogYGrad = 0;

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
	//CreateChMorphedModel(*cptr);
	CreateMorphedModel(&cptr->pinfo->Model, &cptr->pinfo->Animation[cptr->Phase], cptr->FTime);

	float zs = std::sqrtf(cptr->rpos.x * cptr->rpos.x + cptr->rpos.y * cptr->rpos.y + cptr->rpos.z * cptr->rpos.z);
	if (zs > ctViewR * 256) return;

	GlassL = 0;
	if (zs > 256 * (ctViewR - 8)) {
		FadeL = (int)(zs - 256 * (ctViewR - 8)) / 4;
		if (FadeL > 255) {
			GlassL = min(255, FadeL - 255); FadeL = 255;
		}
	}

	waterclip = false;

	//   grConstantColorValue( (255-GlassL) << 24);
	if (cptr->rpos.z > -256 * 10) {
		//RenderModelClip(&cptr->pinfo->Model, cptr->rpos.x, cptr->rpos.y, cptr->rpos.z, 10, -cptr->alpha + pi / 2 + CameraAlpha, CameraBeta);
	}
	else {
		RenderModel(&cptr->pinfo->Model, cptr->rpos.x, cptr->rpos.y, cptr->rpos.z, 10, -cptr->alpha + pi / 2 + CameraAlpha, CameraBeta);
	}


	if (!SHADOWS3D) return;
	if (zs > 256 * (ctViewR - 8)) return;

	int Al = 0x60;

	if (cptr->Health == 0) {
		int at = cptr->pinfo->Animation[cptr->Phase].AniTime;
		if (Tranq) return;
		if (cptr->CType == CTYPE_HUNTER) return;
		if (cptr->FTime == at - 1) return;
		Al = Al * (at - cptr->FTime) / at;
	}

	GlassL = Al << 24;

	RenderShadowClip(&cptr->pinfo->Model,
		cptr->pos.x, cptr->pos.y, cptr->pos.z,
		cptr->rpos.x, cptr->rpos.y, cptr->rpos.z,
		pi / 2 - cptr->alpha,
		CameraAlpha,
		CameraBeta);

}


void RenderExplosionPost(TExplosion* eptr)
{
	CreateMorphedModel(&ExplodInfo.Model, &ExplodInfo.Animation[0], eptr->FTime);

	if (fabs(eptr->rpos.z) + fabs(eptr->rpos.x) < 800)
		RenderModelClip(&ExplodInfo.Model,
			eptr->rpos.x, eptr->rpos.y, eptr->rpos.z, 0, 0, 0);
	else
		RenderModel(&ExplodInfo.Model,
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


	/*grConstantColorValue( (255-GlassL) << 24);*/

	CreateMorphedModel(&ShipModel.Model, &ShipModel.Animation[0], Ship.FTime);

	if (fabs(Ship.rpos.z) < 4000)
		RenderModelClip(&ShipModel.Model,
			Ship.rpos.x, Ship.rpos.y, Ship.rpos.z, 10, -Ship.alpha - pi / 2 + CameraAlpha, CameraBeta);
	else
		RenderModel(&ShipModel.Model,
			Ship.rpos.x, Ship.rpos.y, Ship.rpos.z, 10, -Ship.alpha - pi / 2 + CameraAlpha, CameraBeta);

	/*grConstantColorValue( 0xFF000000);*/
}


void RenderPlayer(int index)
{
}



void Render3DHardwarePosts()
{

	d3dEndBufferG();



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

		if (FOGON)
			CalcFogLevel_Gradient(cptr->rpos);

		cptr->rpos = RotateVector(cptr->rpos);

		float br = BackViewR + DinoInfo[cptr->CType].Radius;
		if (cptr->rpos.z > br) continue;
		if (fabs(cptr->rpos.x) > -cptr->rpos.z + br) continue;
		if (fabs(cptr->rpos.y) > -cptr->rpos.z + br) continue;

		RenderCharacterPost(cptr);
	}

	TExplosion* eptr;
	for (int c = 0; c < ExpCount; c++) {

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
		if (FOGON)
			CalcFogLevel_Gradient(Ship.rpos);

		Ship.rpos = RotateVector(Ship.rpos);
		if (Ship.rpos.z > BackViewR) goto NOSHIP;
		if (fabs(Ship.rpos.x) > -Ship.rpos.z + BackViewR) goto NOSHIP;

		RenderShipPost();
	}
NOSHIP:;

	SunLight *= GetTraceK(SunScrX, SunScrY);
}





void ClearVideoBuf()
{
}



int  CircleCX, CircleCY;
WORD CColor;

void PutPixel(int x, int y)
{
	*((WORD*)ddsd.lpSurface + y * lsw + x) = CColor;
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

	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	if (lpddBack->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return;

	lsw = ddsd.lPitch / 2;
	int RShift, GShift;

	if (VMFORMAT565) {
		RShift = 11; GShift = 6; CColor = 18 << 6;
	}
	else {
		RShift = 10; GShift = 5; CColor = 18 << 5;
	}

	int xx = VideoCX - 128 + (CCX >> 1);
	int yy = VideoCY - 128 + (CCY >> 1);

	if (yy < 0 || yy >= WinH) goto endmap;
	if (xx < 0 || xx >= WinW) goto endmap;
	*((WORD*)ddsd.lpSurface + yy * lsw + xx) = 30 << RShift;
	*((WORD*)ddsd.lpSurface + yy * lsw + xx + 1) = 30 << RShift;
	yy++;
	*((WORD*)ddsd.lpSurface + yy * lsw + xx) = 30 << RShift;
	*((WORD*)ddsd.lpSurface + yy * lsw + xx + 1) = 30 << RShift;

	DrawCircle(xx, yy, 17);

	if (RadarMode)
		for (c = 0; c < ChCount; c++) {
			if (Characters[c].CType != TargetDino + CTYPE_PARA) continue;
			if (!Characters[c].Health) continue;
			xx = VideoCX - 128 + (int)Characters[c].pos.x / 512;
			yy = VideoCY - 128 + (int)Characters[c].pos.z / 512;
			if (yy <= 0 || yy >= WinH) goto endmap;
			if (xx <= 0 || xx >= WinW) goto endmap;
			*((WORD*)ddsd.lpSurface + yy * lsw + xx) = 30 << GShift;
			*((WORD*)ddsd.lpSurface + yy * lsw + xx + 1) = 30 << GShift;
			yy++;
			*((WORD*)ddsd.lpSurface + yy * lsw + xx) = 30 << GShift;
			*((WORD*)ddsd.lpSurface + yy * lsw + xx + 1) = 30 << GShift;
		}

endmap:
	lpddBack->Unlock(ddsd.lpSurface);
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
	RenderModelSun(&SunModel, x * d, y * d, z * d, (int)(200.f * SkyTraceK));
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

	d3dSetTexture(SkyPic, 256, 256);

	nv.x = 512; nv.y = 4024; nv.z = 0;

	cb = (float)cos(CameraBeta);
	sb = (float)sin(CameraBeta);
	SKYDTime = RealTime & ((1 << 16) - 1);

	float sh = -CameraY;
	if (MapMinY == 10241024)
		MapMinY = 0;
	sh = (float)((int)MapMinY) * ctHScale - CameraY;

	if (sh < -2024) sh = -2024;

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

	cb = cosf(CameraBeta - 0.30f);
	sb = sinf(CameraBeta - 0.30f);

	tx.x = 0.004f;  tx.y = 0;     tx.z = 0;
	ty.x = 0.0f;    ty.y = 0;     ty.z = 0.004f;
	nv.x = 0;       nv.y = -1.f;  nv.z = 0;

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

	float za = CameraW * CameraH * p / (qy * VideoCY + qz);
	float zb = CameraW * CameraH * p / (qy * (VideoCY - scry / 2.f) + qz);
	float zc = CameraW * CameraH * p / (qy * (VideoCY - scry) + qz);

	float _za = fabs(za) - 50200; if (_za < 0) _za = 0;
	float _zb = fabs(zb) - 50200; if (_zb < 0) _zb = 0;
	float _zc = fabs(zc) - 50200; if (_zc < 0) _zc = 0;

	int alpha = (int)(255 * 40240 / (40240 + _za));
	int alphb = (int)(255 * 40240 / (40240 + _zb));
	int alphc = (int)(255 * 40240 / (40240 + _zc));

	int sx1 = -VideoCX;
	int sx2 = +VideoCX;

	float qx1 = qx * sx1 + qz;
	float qx2 = qx * sx2 + qz;
	float qyy;


	//   d3dStartBuffer();
	ZeroMemory(&d3dExeBufDesc, sizeof(d3dExeBufDesc));
	d3dExeBufDesc.dwSize = sizeof(d3dExeBufDesc);
	hRes = lpd3dExecuteBufferG->Lock(&d3dExeBufDesc);
	if (FAILED(hRes)) DoHalt("Error locking execute buffer");
	lpVertex = (LPD3DTLVERTEX)d3dExeBufDesc.lpData;

	float dtt = (float)(SKYDTime) / 256.f;

	float sky = 0;
	float sy = VideoCY - sky;
	qyy = qy * sy;
	q = qx1 + qyy;
	float fxa = (px * sx1 + py * sy + pz) / q;
	float fya = (rx * sx1 + ry * sy + rz) / q;
	q = qx2 + qyy;
	float fxb = (px * sx2 + py * sy + pz) / q;
	float fyb = (rx * sx2 + ry * sy + rz) / q;

	lpVertex->sx = 0.f;
	lpVertex->sy = (float)sky;
	lpVertex->sz = 0.0001f;//-8.f / za;
	lpVertex->rhw = -1.f / za;
	if (FOGENABLE) {
		lpVertex->color = 0xFFFFFFFF;
		lpVertex->specular = alpha << 24;
	}
	else {
		lpVertex->color = 0x00FFFFFF | alpha << 24;
		lpVertex->specular = 0xFF000000;
	}
	lpVertex->tu = (fxa + dtt) / 256.f;
	lpVertex->tv = (fya - dtt) / 256.f;
	lpVertex++;

	lpVertex->sx = (float)WinW;
	lpVertex->sy = (float)sky;
	lpVertex->sz = 0.0001f;//-8.f / za;
	lpVertex->rhw = -1.f / za;
	if (FOGENABLE) {
		lpVertex->color = 0xFFFFFFFF;
		lpVertex->specular = alpha << 24;
	}
	else {
		lpVertex->color = 0x00FFFFFF | alpha << 24;
		lpVertex->specular = 0xFF000000;
	}
	lpVertex->tu = (fxb + dtt) / 256.f;
	lpVertex->tv = (fyb - dtt) / 256.f;
	lpVertex++;


	sky = scry / 2.f;
	sy = VideoCY - sky;
	qyy = qy * sy;
	q = qx1 + qyy;
	fxa = (px * sx1 + py * sy + pz) / q;
	fya = (rx * sx1 + ry * sy + rz) / q;
	q = qx2 + qyy;
	fxb = (px * sx2 + py * sy + pz) / q;
	fyb = (rx * sx2 + ry * sy + rz) / q;

	lpVertex->sx = 0.f;
	lpVertex->sy = (float)sky;
	lpVertex->sz = 0.0001f;//-8.f / zb;
	lpVertex->rhw = -1.f / zb;
	if (FOGENABLE) {
		lpVertex->color = 0xFFFFFFFF;
		lpVertex->specular = alphb << 24;
	}
	else {
		lpVertex->color = 0x00FFFFFF | alphb << 24;
		lpVertex->specular = 0xFF000000;
	}
	lpVertex->tu = (fxa + dtt) / 256.f;
	lpVertex->tv = (fya - dtt) / 256.f;
	lpVertex++;

	lpVertex->sx = (float)WinW;
	lpVertex->sy = (float)sky;
	lpVertex->sz = 0.0001f;//-8.f / zb;
	lpVertex->rhw = -1.f / zb;
	if (FOGENABLE) {
		lpVertex->color = 0xFFFFFFFF;
		lpVertex->specular = alphb << 24;
	}
	else {
		lpVertex->color = 0x00FFFFFF | alphb << 24;
		lpVertex->specular = 0xFF000000;
	}
	lpVertex->tu = (fxb + dtt) / 256.f;
	lpVertex->tv = (fyb - dtt) / 256.f;
	lpVertex++;




	sky = scry;
	sy = VideoCY - sky;
	qyy = qy * sy;
	q = qx1 + qyy;
	fxa = (px * sx1 + py * sy + pz) / q;
	fya = (rx * sx1 + ry * sy + rz) / q;
	q = qx2 + qyy;
	fxb = (px * sx2 + py * sy + pz) / q;
	fyb = (rx * sx2 + ry * sy + rz) / q;

	lpVertex->sx = 0.f;
	lpVertex->sy = (float)sky;
	lpVertex->sz = 0.0001f;//-8.f / zb;
	lpVertex->rhw = -1.f / zc;
	if (FOGENABLE) {
		lpVertex->color = 0xFFFFFFFF;
		lpVertex->specular = alphc << 24;
	}
	else {
		lpVertex->color = 0x00FFFFFF | alphc << 24;
		lpVertex->specular = 0xFF000000;
	}
	lpVertex->tu = (fxa + dtt) / 256.f;
	lpVertex->tv = (fya - dtt) / 256.f;
	lpVertex++;

	lpVertex->sx = (float)WinW;
	lpVertex->sy = (float)sky;
	lpVertex->sz = 0.0001f;//-8.f / zb;
	lpVertex->rhw = -1.f / zc;
	if (FOGENABLE) {
		lpVertex->color = 0xFFFFFFFF;
		lpVertex->specular = alphc << 24;
	}
	else {
		lpVertex->color = 0x00FFFFFF | alphc << 24;
		lpVertex->specular = 0xFF000000;
	}
	lpVertex->tu = (fxb + dtt) / 256.f;
	lpVertex->tv = (fyb - dtt) / 256.f;
	lpVertex++;



	lpInstruction = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDesc.lpData + 400 * 3);
	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 1;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREHANDLE;
	lpState->dwArg[0] = hTexture;
	lpState++;

	lpInstruction = (LPD3DINSTRUCTION)lpState;
	lpInstruction->bOpcode = D3DOP_PROCESSVERTICES;
	lpInstruction->bSize = sizeof(D3DPROCESSVERTICES);
	lpInstruction->wCount = 1U;
	lpInstruction++;

	lpProcessVertices = (LPD3DPROCESSVERTICES)lpInstruction;
	lpProcessVertices->dwFlags = D3DPROCESSVERTICES_COPY;
	lpProcessVertices->wStart = 0U;
	lpProcessVertices->wDest = 0U;
	lpProcessVertices->dwCount = 6;
	lpProcessVertices->dwReserved = 0UL;
	lpProcessVertices++;


	lpInstruction = (LPD3DINSTRUCTION)lpProcessVertices;
	lpInstruction->bOpcode = D3DOP_TRIANGLE;
	lpInstruction->bSize = sizeof(D3DTRIANGLE);
	lpInstruction->wCount = 4;
	lpInstruction++;
	lpTriangle = (LPD3DTRIANGLE)lpInstruction;

	lpTriangle->wV1 = 0;
	lpTriangle->wV2 = 1;
	lpTriangle->wV3 = 2;
	lpTriangle->wFlags = 0;
	lpTriangle++;

	lpTriangle->wV1 = 1;
	lpTriangle->wV2 = 2;
	lpTriangle->wV3 = 3;
	lpTriangle->wFlags = 0;
	lpTriangle++;


	lpTriangle->wV1 = 2;
	lpTriangle->wV2 = 3;
	lpTriangle->wV3 = 4;
	lpTriangle->wFlags = 0;
	lpTriangle++;

	lpTriangle->wV1 = 3;
	lpTriangle->wV2 = 4;
	lpTriangle->wV3 = 5;
	lpTriangle->wFlags = 0;
	lpTriangle++;

	lpInstruction = (LPD3DINSTRUCTION)lpTriangle;
	lpInstruction->bOpcode = D3DOP_EXIT;
	lpInstruction->bSize = 0UL;
	lpInstruction->wCount = 0U;

	lpd3dExecuteBufferG->Unlock();

	hRes = lpd3dDevice->Execute(lpd3dExecuteBufferG, lpd3dViewport, D3DEXECUTE_UNCLIPPED);

	nv.x = -2048;
	nv.y = +4048;
	nv.z = -2048;
	nv = RotateVector(nv);
	SunLight = 0;
	if (nv.z < -2024) RenderSun(nv.x, nv.y, nv.z);
}



void RenderFSRect(DWORD Color)
{
	d3dStartBuffer();

	lpVertex->sx = 0.f;
	lpVertex->sy = 0.f;
	lpVertex->sz = 0.999f;
	lpVertex->rhw = 1.f;
	lpVertex->color = Color;
	lpVertex->specular = 0xFF000000;
	lpVertex->tu = 0;
	lpVertex->tv = 0;
	lpVertex++;

	lpVertex->sx = (float)WinW;
	lpVertex->sy = 0.f;
	lpVertex->sz = 0.999f;
	lpVertex->rhw = 1.f;
	lpVertex->color = Color;
	lpVertex->specular = 0xFF000000;
	lpVertex->tu = 0;
	lpVertex->tv = 0;
	lpVertex++;

	lpVertex->sx = 0.f;
	lpVertex->sy = (float)WinH;
	lpVertex->sz = 0.999f;
	lpVertex->rhw = 1.f;
	lpVertex->color = Color;
	lpVertex->specular = 0xFF000000;
	lpVertex->tu = 0;
	lpVertex->tv = 0;
	lpVertex++;

	lpVertex->sx = (float)WinW;
	lpVertex->sy = (float)WinH;
	lpVertex->sz = (float)0.999f;
	lpVertex->rhw = 1.f;
	lpVertex->color = Color;
	lpVertex->specular = 0xFF000000;
	lpVertex->tu = 0;
	lpVertex->tv = 0;
	lpVertex++;


	lpInstruction = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDesc.lpData + 1024 * 3);
	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 1;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREHANDLE;
	lpState->dwArg[0] = NULL;
	lpState++;

	lpInstruction = (LPD3DINSTRUCTION)lpState;
	lpInstruction->bOpcode = D3DOP_PROCESSVERTICES;
	lpInstruction->bSize = sizeof(D3DPROCESSVERTICES);
	lpInstruction->wCount = 1U;
	lpInstruction++;

	lpProcessVertices = (LPD3DPROCESSVERTICES)lpInstruction;
	lpProcessVertices->dwFlags = D3DPROCESSVERTICES_COPY;
	lpProcessVertices->wStart = 0U;
	lpProcessVertices->wDest = 0U;
	lpProcessVertices->dwCount = 4;
	lpProcessVertices->dwReserved = 0UL;
	lpProcessVertices++;


	lpInstruction = (LPD3DINSTRUCTION)lpProcessVertices;
	lpInstruction->bOpcode = D3DOP_TRIANGLE;
	lpInstruction->bSize = sizeof(D3DTRIANGLE);
	lpInstruction->wCount = 2;
	lpInstruction++;
	lpTriangle = (LPD3DTRIANGLE)lpInstruction;

	lpTriangle->wV1 = 0;
	lpTriangle->wV2 = 1;
	lpTriangle->wV3 = 2;
	lpTriangle->wFlags = 0;
	lpTriangle++;

	lpTriangle->wV1 = 1;
	lpTriangle->wV2 = 2;
	lpTriangle->wV3 = 3;
	lpTriangle->wFlags = 0;
	lpTriangle++;

	lpInstruction = (LPD3DINSTRUCTION)lpTriangle;
	lpInstruction->bOpcode = D3DOP_EXIT;
	lpInstruction->bSize = 0UL;
	lpInstruction->wCount = 0U;

	lpd3dExecuteBuffer->Unlock();

	hRes = lpd3dDevice->Execute(lpd3dExecuteBuffer, lpd3dViewport, D3DEXECUTE_UNCLIPPED);
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

	d3dStartBuffer();

	for (int y = 0; y < 4; y++) {
		lpVertex->sx = (float)x0 - 1;
		lpVertex->sy = (float)y0 + y;
		lpVertex->sz = 0.9999f;
		lpVertex->rhw = 1.f;
		lpVertex->color = 0xF0000010;
		lpVertex->specular = 0xFF000000;
		lpVertex->tu = 0;
		lpVertex->tv = 0;
		lpVertex++;

		lpVertex->sx = (float)x0 + L0 + 1;
		lpVertex->sy = (float)y0 + y;
		lpVertex->sz = 0.9999f;
		lpVertex->rhw = 1.f;
		lpVertex->color = 0xF0000010;
		lpVertex->specular = 0xFF000000;
		lpVertex->tu = 0;
		lpVertex->tv = 0;
		lpVertex++;
	}

	for (int y = 1; y < 3; y++) {
		lpVertex->sx = (float)x0;
		lpVertex->sy = (float)y0 + y;
		lpVertex->sz = 0.99999f;
		lpVertex->rhw = 1.f;
		lpVertex->color = 0xF0000000 + (G << 8) + (R << 16);
		lpVertex->specular = 0xFF000000;// + (G<<8) + (R<<16);
		lpVertex->tu = 0;
		lpVertex->tv = 0;
		lpVertex++;

		lpVertex->sx = (float)x0 + L0;
		lpVertex->sy = (float)y0 + y;
		lpVertex->sz = 0.99999f;
		lpVertex->rhw = 1.f;
		lpVertex->color = 0xF0000000 + (G << 8) + (R << 16);
		lpVertex->specular = 0xFF000000;// + (G<<8) + (R<<16);
		lpVertex->tu = 0;
		lpVertex->tv = 0;
		lpVertex++;
	}



	lpInstruction = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDesc.lpData + 1024 * 3);
	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 1;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREHANDLE;
	lpState->dwArg[0] = NULL;
	lpState++;

	lpInstruction = (LPD3DINSTRUCTION)lpState;
	lpInstruction->bOpcode = D3DOP_PROCESSVERTICES;
	lpInstruction->bSize = sizeof(D3DPROCESSVERTICES);
	lpInstruction->wCount = 1U;
	lpInstruction++;

	lpProcessVertices = (LPD3DPROCESSVERTICES)lpInstruction;
	lpProcessVertices->dwFlags = D3DPROCESSVERTICES_COPY;
	lpProcessVertices->wStart = 0U;
	lpProcessVertices->wDest = 0U;
	lpProcessVertices->dwCount = 12;
	lpProcessVertices->dwReserved = 0UL;
	lpProcessVertices++;

	lpInstruction = (LPD3DINSTRUCTION)lpProcessVertices;
	lpInstruction->bOpcode = D3DOP_LINE;
	lpInstruction->bSize = sizeof(D3DLINE);
	lpInstruction->wCount = 6;
	lpInstruction++;
	lpLine = (LPD3DLINE)lpInstruction;

	for (int y = 0; y < 6; y++) {
		lpLine->wV1 = y * 2;
		lpLine->wV2 = y * 2 + 1;
		lpLine++;
	}

	lpInstruction = (LPD3DINSTRUCTION)lpLine;
	lpInstruction->bOpcode = D3DOP_EXIT;
	lpInstruction->bSize = 0UL;
	lpInstruction->wCount = 0U;

	lpd3dExecuteBuffer->Unlock();

	hRes = lpd3dDevice->Execute(lpd3dExecuteBuffer, lpd3dViewport, D3DEXECUTE_UNCLIPPED);

}



void Render_Cross(int sx, int sy)
{

	float w = (float)WinW / 12.f;
	d3dStartBuffer();

	lpVertex->sx = (float)sx - w;
	lpVertex->sy = (float)sy;
	lpVertex->sz = 0.99999f;
	lpVertex->rhw = 1.f;
	lpVertex->color = 0x80000010;
	lpVertex->specular = 0xFF000000;
	lpVertex->tu = 0;
	lpVertex->tv = 0;
	lpVertex++;

	lpVertex->sx = (float)sx + w;
	lpVertex->sy = (float)sy;
	lpVertex->sz = 0.99999f;
	lpVertex->rhw = 1.f;
	lpVertex->color = 0x80000010;
	lpVertex->specular = 0xFF000000;
	lpVertex->tu = 0;
	lpVertex->tv = 0;
	lpVertex++;


	lpVertex->sx = (float)sx;
	lpVertex->sy = (float)sy - w;
	lpVertex->sz = 0.99999f;
	lpVertex->rhw = 1.f;
	lpVertex->color = 0x80000010;
	lpVertex->specular = 0xFF000000;
	lpVertex->tu = 0;
	lpVertex->tv = 0;
	lpVertex++;


	lpVertex->sx = (float)sx;
	lpVertex->sy = (float)sy + w;
	lpVertex->sz = 0.99999f;
	lpVertex->rhw = 1.f;
	lpVertex->color = 0x80000010;
	lpVertex->specular = 0xFF000000;
	lpVertex->tu = 0;
	lpVertex->tv = 0;
	lpVertex++;


	lpInstruction = (LPD3DINSTRUCTION)((LPD3DTLVERTEX)d3dExeBufDesc.lpData + 1024 * 3);
	lpInstruction->bOpcode = D3DOP_STATERENDER;
	lpInstruction->bSize = sizeof(D3DSTATE);
	lpInstruction->wCount = 1;
	lpInstruction++;
	lpState = (LPD3DSTATE)lpInstruction;

	lpState->drstRenderStateType = D3DRENDERSTATE_TEXTUREHANDLE;
	lpState->dwArg[0] = NULL;
	lpState++;

	lpInstruction = (LPD3DINSTRUCTION)lpState;
	lpInstruction->bOpcode = D3DOP_PROCESSVERTICES;
	lpInstruction->bSize = sizeof(D3DPROCESSVERTICES);
	lpInstruction->wCount = 1U;
	lpInstruction++;

	lpProcessVertices = (LPD3DPROCESSVERTICES)lpInstruction;
	lpProcessVertices->dwFlags = D3DPROCESSVERTICES_COPY;
	lpProcessVertices->wStart = 0U;
	lpProcessVertices->wDest = 0U;
	lpProcessVertices->dwCount = 4;
	lpProcessVertices->dwReserved = 0UL;
	lpProcessVertices++;


	lpInstruction = (LPD3DINSTRUCTION)lpProcessVertices;
	lpInstruction->bOpcode = D3DOP_LINE;
	lpInstruction->bSize = sizeof(D3DLINE);
	lpInstruction->wCount = 2;
	lpInstruction++;
	lpLine = (LPD3DLINE)lpInstruction;

	lpLine->wV1 = 0;
	lpLine->wV2 = 1;
	lpLine++;

	lpLine->wV1 = 2;
	lpLine->wV2 = 3;
	lpLine++;

	lpInstruction = (LPD3DINSTRUCTION)lpLine;
	lpInstruction->bOpcode = D3DOP_EXIT;
	lpInstruction->bSize = 0UL;
	lpInstruction->wCount = 0U;

	lpd3dExecuteBuffer->Unlock();

	hRes = lpd3dDevice->Execute(lpd3dExecuteBuffer, lpd3dViewport, D3DEXECUTE_UNCLIPPED);


}


#endif

