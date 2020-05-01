#if defined(_ogl)
#include "Hunt.h"

#include <GL/gl.h>
#include <GL/glext.h>

#include <cstdio>
#include <cmath>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#undef  TCMAX 
#undef  TCMIN 
#define TCMAX 128
#define TCMIN 0

/** Local RenderD3D11.cpp structures and classes **/
class VertexBufStruct {
public:
	float x, y, z;
	float s, t;
	uint32_t diffuse;

	VertexBufStruct(float X = 0.f, float Y = 0.f, float Z = 0.f, float S = 0.f, float T = 0.f, unsigned diffuse) :
		x(X), y(Y), z(Z),
		s(S), t(T),
		diffuse(diffuse)
	{}
};


/** Local RenderOGL.cpp variables **/
HGLRC g_GLContext

std::vector<VertexBufStruct> g_VertexBufferData;

bool g_OpenGLActive = false;


// Local RenderD3D11.cpp functions
void RenderSun(float, float, float);
bool R_InitShaders();
void R_TextOut(int, int, const std::string&, uint32_t);
void R_StartVBuffer();
void R_StartVBufferG();
void R_EndVBuffer();
void R_EndVBufferG();

uint16_t conv_555(uint16_t);
void conv_pic555(TPicture&);



uint16_t conv_555(uint16_t c)
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




/*
Clear the Depth/Z Buffer
*/
void Hardware_ZBuffer(bool bl)
{
	if (!bl) {
		// -- Clear
		//d3dContext->ClearDepthStencilView(d3dDepthBuffer, 0, 1.0f, 0);
	}
}


/*
Initialise anything to do with the 3D Renderer here, it is called prior to Activate3DHardware, but only once.
*/
void Init3DHardware()
{
	HARD3D = true;
	std::cout << "==== Initialising OpenGL 2.0 Hardware ====" << std::endl;

	// Ensure the interface pointers are nullptr
	g_RenderContext = nullptr;
}


/*
Activate the 3D renderer,
*/
bool Activate3DHardware()
{
	if (g_OpenGLActive)
		return true;

	HRESULT hres = S_OK;

	std::cout << std::setfill(' ') << std::setw(10) << "[Render]" << "-> Activate3DHardware( Direct3D 11 )" << std::endl;

	DXGI_SWAP_CHAIN_DESC scd;
	std::memset(&scd, 0, sizeof(DXGI_SWAP_CHAIN_DESC));

	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = hwndMain;							// the window to be used
	scd.SampleDesc.Count = 1;                               // how many multisamples
	scd.SampleDesc.Quality = 0;                             // multisample quality level

#if defined(_DEBUG)
	scd.Windowed = true;
#else
	scd.Windowed = FULLSCREEN;
	if (FULLSCREEN) {
		scd.BufferDesc.Width = WinW;
		scd.BufferDesc.Height = WinH;
		scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		SetWindowLong(hwndMain, GWL_STYLE, WS_VISIBLE);
		SetWindowPos(hwndMain, HWND_TOP, 0, 0, WinW, WinH, SWP_SHOWWINDOW);
	}
#endif

	unsigned deviceFlags = 0;
#if defined(_DEBUG)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif //_DEBUG

	/*hres = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &devcon);
	if (FAILED(hres)) {
		throw dohalt("Failed to create Direct3D Device and Context.", __FUNCTION__, __FILE__, __LINE__);
	}*/

	// Create a device, device context and swap chain
	hres = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &scd, &g_SwapChain, &g_Device, nullptr, &g_Context);
	if (FAILED(hres)) {
		throw dohalt("Failed to create Direct3D Device, Context and Swap Chain.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	ID3D11Texture2D* pBackBuffer = nullptr;
	g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);

	hres = g_Device->CreateRenderTargetView(pBackBuffer, nullptr, &g_BackBuffer);
	if (FAILED(hres)) { return false; }
	pBackBuffer->Release();

	g_Context->OMSetRenderTargets(1, &g_BackBuffer, nullptr);

	// Set up a Viewport
	D3D11_VIEWPORT viewport;
	std::memset(&viewport, 0, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0.f;
	viewport.TopLeftY = 0.f;
	viewport.Width = WinW * 1.f;
	viewport.Height = WinH * 1.f;

	g_Context->RSSetViewports(1, &viewport);

	if (R_InitShaders()) {
		g_Context->VSSetShader(g_VertexShader, nullptr, 0);
		g_Context->PSSetShader(g_PixelShader, nullptr, 0);
	}

	std::cout << "Creating vertex buffer object... ";

	D3D11_BUFFER_DESC bd;
	std::memset(&bd, 0, sizeof(D3D11_BUFFER_DESC));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = (sizeof(VertexBufStruct) * 3) * 1024;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hres = g_Device->CreateBuffer(&bd, nullptr, &g_VertexBuffer);
	if (FAILED(hres)) {
		std::cout << "Failed!" << std::endl;
		throw dohalt("Failed to create D3D Vertex Buffer Object.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
	std::cout << "Ok!" << std::endl;

	// -- Test vertex buffers
	std::cout << "Testing vertex buffer object...";

	VertexBufStruct testverts[] = {
		VertexBufStruct(0.0f,  0.5f,  0.f,  0.f, 0.f,  255U, 0U, 0U, 255U),
		VertexBufStruct(0.45f, -0.5f,  0.f,  1.f, 0.f,  0U, 255U, 0U, 255U),
		VertexBufStruct(-0.45f, -0.5f,  0.f,  1.f, 1.f,  0U, 0U, 255U, 255U)
	};

	D3D11_MAPPED_SUBRESOURCE ms;
	hres = g_Context->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (FAILED(hres)) {
		std::cout << "Failed!" << std::endl;
		throw dohalt("Failed to map D3D vertex buffer object.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
	std::cout << "Ok!" << std::endl;

	memcpy(ms.pData, testverts, sizeof(testverts));
	g_Context->Unmap(g_VertexBuffer, 0);
	std::memset(&g_VertexBufferMap, 0, sizeof(g_VertexBufferMap));

	std::cout << "Direct3D 11 Activated!" << std::endl;
	g_OpenGLActive = true;
	return true;
}


/*
Shut down 3D hardware, perform clean up
*/
void ShutDown3DHardware()
{
	if (!g_OpenGLActive)
		return;

	std::cout << std::setfill(' ') << std::setw(8) << "[Render]" << "-> ShutDown3DHardware()" << std::endl;

#if !defined(_DEBUG)
	if (FULLSCREEN) {
		d3dSwap->SetFullscreenState(false, nullptr);
	}
#endif

	R_EndVBufferG();
	R_EndVBuffer();

	if (g_VertexBuffer)
		g_VertexBuffer->Release();

	if (g_VertexShader)
		g_VertexShader->Release();

	if (g_PixelShader)
		g_PixelShader->Release();

	if (g_SwapChain)
		g_SwapChain->Release();

	if (g_BackBuffer)
		g_BackBuffer->Release();

	if (g_Device)
		g_Device->Release();

	if (g_Context)
		g_Context->Release();

	g_Device = nullptr;
	g_Context = nullptr;
	g_SwapChain = nullptr;
	g_BackBuffer = nullptr;
	g_VertexShader = nullptr;
	g_PixelShader = nullptr;
	g_VertexBuffer = nullptr;
	g_VertexBufferG = nullptr;

	std::cout << "[RENDER] Successfully shutdown 3D hardware." << std::endl;
	DIRECT3DACTIVE = false;
}


/**
Swap video buffers and handle end-of-frame rendering
**/
void ShowVideo()
{
	if (!DIRECT3DACTIVE)
		return;

	HRESULT hRes = S_OK;

	LOWRESTX = false;

	// RenderD3D.cpp does this for some reason:
	//SkyR = 0x60; SkyG = 0x60; SkyB = 0x65;

	float back_color[] = { ((float)SkyR) / 255.f, ((float)SkyG) / 255.f, ((float)SkyB) / 255.f, 1.0f };

	g_SwapChain->Present(0, 0);
	g_Context->ClearRenderTargetView(g_BackBuffer, back_color);

	/*hRes = lpd3dDevice->EndScene();

	hRes = lpddPrimary->Blt(NULL, lpddBack, NULL, DDBLT_WAIT, NULL);

	d3dClearBuffers();

	hRes = lpd3dDevice->BeginScene();*/
}


/*
SOFTWARE ONLY
Copy the 3D scene to the GDI DIB surface
*/
void CopyHARDToDIB()
{
}


/*
Draw the TPicture at screen x,y co-ordinates
*/
void DrawPicture(int x, int y, TPicture& pic)
{
}


/*
Print a C++ string to the screen
*/
void R_TextOut(int x, int y, const std::string& text, uint32_t color)
{
	if (text.empty())
		return;

	/*if (SmallFont) {
		oldfont = static_cast<HFONT>(SelectObject(ddBackDC, fnt_Small));
	}

	SetTextColor(ddBackDC, 0x00101010);
	TextOut(ddBackDC, x + 2, y + 1, text.c_str(), t.length());

	SetTextColor(ddBackDC, color);
	TextOut(ddBackDC, x + 1, y, text.c_str(), t.length());

	if (SmallFont) {
		SelectObject(ddBackDC, oldfont);
	}*/
}


void DrawTrophyText(int x0, int y0)
{
	int x;
	int tc = TrophyBody;
	std::stringstream ss;

	int& dtype = TrophyRoom.Body[tc].ctype;
	int& time = TrophyRoom.Body[tc].time;
	int& date = TrophyRoom.Body[tc].date;
	int& wep = TrophyRoom.Body[tc].weapon;
	int& score = TrophyRoom.Body[tc].score;
	float& scale = TrophyRoom.Body[tc].scale;
	float& range = TrophyRoom.Body[tc].range;

	x0 += 14; y0 += 18;
	x = x0;
	R_TextOut(x, y0, "Name: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Name: ");
	R_TextOut(x, y0, DinoInfo[dtype].Name, 0x0000BFBF);

	x = x0;
	R_TextOut(x, y0 + 16, "Weight: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Weight: ");
	if (OptSys)
		ss << std::setprecision(3) << (DinoInfo[dtype].Mass * scale * scale / 0.907f) << "t";
	else
		ss << std::setprecision(3) << (DinoInfo[dtype].Mass * scale * scale) << "T";

	R_TextOut(x, y0 + 16, ss.str().c_str(), 0x0000BFBF);    x += GetTextW(hdcMain, ss.str());
	ss.str("");  ss.clear();

	R_TextOut(x, y0 + 16, "Length: ", 0x00BFBFBF); x += GetTextW(hdcMain, "Length: ");

	if (OptSys)
		ss << std::setprecision(2) << (DinoInfo[dtype].Length * scale / 0.3f) << "ft";
	else
		ss << std::setprecision(2) << (DinoInfo[dtype].Length * scale) << "m";

	R_TextOut(x, y0 + 16, ss.str().c_str(), 0x0000BFBF);
	ss.str("");  ss.clear();

	x = x0;
	R_TextOut(x, y0 + 32, "Weapon: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Weapon: ");
	ss << WeapInfo[wep].Name << "    ";
	R_TextOut(x, y0 + 32, ss.str().c_str(), 0x0000BFBF);   x += GetTextW(hdcMain, ss.str());
	ss.str("");  ss.clear();

	R_TextOut(x, y0 + 32, "Score: ", 0x00BFBFBF);   x += GetTextW(hdcMain, "Score: ");
	ss << score;
	R_TextOut(x, y0 + 32, ss.str().c_str(), 0x0000BFBF);
	ss.str("");  ss.clear();

	x = x0;
	R_TextOut(x, y0 + 48, "Range of kill: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Range of kill: ");
	if (OptSys) ss << std::setprecision(2) << (range / 0.3f) << "ft";
	else        ss << std::setprecision(2) << (range) << "ft";
	R_TextOut(x, y0 + 48, ss.str().c_str(), 0x0000BFBF);
	ss.str("");  ss.clear();


	x = x0;
	R_TextOut(x, y0 + 64, "Date: ", 0x00BFBFBF);  x += GetTextW(hdcMain, "Date: ");
	if (OptSys)
		ss << ((date >> 10) & 255) << "." << (date & 255) << "." << (date >> 20) << "   ";
	else
		ss << (date & 255) << "." << ((date >> 10) & 255) << "." << (date >> 20) << "   ";

	R_TextOut(x, y0 + 64, ss.str().c_str(), 0x0000BFBF);   x += GetTextW(hdcMain, ss.str());
	ss.str("");  ss.clear();

	R_TextOut(x, y0 + 64, "Time: ", 0x00BFBFBF);   x += GetTextW(hdcMain, "Time: ");
	ss << ((time >> 10) & 255) << ":" << std::setfill('0') << std::setw(2) << (time & 255);
	R_TextOut(x, y0 + 64, ss.str().c_str(), 0x0000BFBF);
}




void Render_LifeInfo(int li)
{
	int x, y;

	int& ctype = Characters[li].CType;
	float& scale = Characters[li].scale;

	x = VideoCX + WinW / 64;
	y = VideoCY + (int)(WinH / 6.8);

	R_TextOut(x, y, DinoInfo[ctype].Name, 0x0000b000);

	std::stringstream ss;

	if (OptSys) ss << "Weight: " << std::setprecision(3) << (DinoInfo[ctype].Mass * scale * scale / 0.907) << "t";
	else        ss << "Weight: " << std::setprecision(3) << (DinoInfo[ctype].Mass * scale * scale) << "T";

	R_TextOut(x, y + 16, ss.str().c_str(), 0x0000b000);
}


/**
Render text-based HUD elements
**/
void ShowControlElements()
{
	std::stringstream ss;

	if (TIMER) {
		ss << "Frame Delta: " << TimeDt << "ms";
		R_TextOut(WinEX - 81, 11, ss.str(), 0x0020A0A0);
		ss.str("");  ss.clear();

		ss << "Polygon Count: " << dFacesCount;
		R_TextOut(WinEX - 90, 24, ss.str(), 0x0020A0A0);
		ss.str(""); ss.clear();
	}

	if (MessageList.timeleft) {
		if (RealTime > MessageList.timeleft) MessageList.timeleft = 0;
		R_TextOut(10, 10, MessageList.mtext, 0x0020A0A0);
	}

	if (ExitTime) {
		int y = WinH / 3;
		ss << "Preparing for evacuation...";
		R_TextOut(VideoCX - GetTextW(hdcCMain, ss.str()) / 2, y, ss.str().c_str(), 0x0060C0D0);
		ss.str(""); ss.clear();

		ss << (1 + ExitTime / 1000) << " seconds left.";
		R_TextOut(VideoCX - GetTextW(hdcCMain, ss.str()) / 2, y + 18, ss.str().c_str(), 0x0060C0D0);
	}

	// Render underwater overlay
	/*if (UNDERWATER)
		RenderFSRect(0x90004050);
	*/

	// Render Sunglare
	/*if (!UNDERWATER && (SunLight > 1.0f)) {
		RenderFSRect(0xFFFFC0 + ((int)SunLight << 24));
	}*/


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


int zs;

void DrawTPlaneClip(bool SECONT)
{
}


void DrawTPlane(bool SECONT)
{
	int n;
	bool SecondPass = false;

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
		//scrp[n].x = (VideoCX << 4) - (int)(ev[n].v.x / ev[n].v.z * CameraW * 16.f);
		scrp[n].x = -(int)(ev[n].v.x / ev[n].v.z * CameraW);
		scrp[n].y = (int)(ev[n].v.y / ev[n].v.z * CameraH);
		//scrp[n].y = (VideoCY << 4) + (int)(ev[n].v.y / ev[n].v.z * CameraH * 16.f);
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


	//if (alpha1 > WaterAlphaL) alpha1 = WaterAlphaL;
	//if (alpha2 > WaterAlphaL) alpha2 = WaterAlphaL;
	//if (alpha3 > WaterAlphaL) alpha3 = WaterAlphaL;

	if (g_VertexBufferData.size() > 380) {
		R_EndVBufferG();
	}

	R_StartVBufferG();

	VertexBufStruct vertex[3];
	VertexBufStruct* lpVertexG = nullptr;

	lpVertexG = &vertex[0];
	lpVertexG->x = 0.0f;// (float)scrp[0].x / 16.f;
	lpVertexG->y = 0.0f;// (float)scrp[0].y / 16.f;
	lpVertexG->z = 0.0f;// -8.f / ev[0].v.z;
	lpVertexG->r = 255; lpVertexG->g = 0; lpVertexG->b = 0; lpVertexG->a = 255;
	//lpVertexG->rhw = lpVertexG->sz * 0.125f;
	//lpVertexG->color = (int)(255 - scrp[0].Light * 4) * 0x00010101 | alpha1 << 24;
	//lpVertexG->specular = (255 - (int)ev[0].Fog) << 24;//0x7F000000;
	lpVertexG->s = (float)(scrp[0].tx) / (128.f);
	lpVertexG->t = (float)(scrp[0].ty) / (128.f);
	lpVertexG++;

	lpVertexG->x = (float)scrp[1].x / (float)CameraW;
	lpVertexG->y = (float)scrp[1].y / (float)CameraH;
	lpVertexG->z = -8.f / ev[1].v.z;
	lpVertexG->r = 0; lpVertexG->g = 255; lpVertexG->b = 0; lpVertexG->a = 255;
	//lpVertexG->rhw = lpVertexG->sz * 0.125f;
	//lpVertexG->color = (int)(255 - scrp[1].Light * 4) * 0x00010101 | alpha2 << 24;
	//lpVertexG->specular = (255 - (int)ev[1].Fog) << 24;//0x7F000000;
	lpVertexG->s = (float)(scrp[1].tx) / (128.f);
	lpVertexG->t = (float)(scrp[1].ty) / (128.f);
	lpVertexG++;

	lpVertexG->x = (float)scrp[2].x / (float)CameraW;
	lpVertexG->y = (float)scrp[2].y / (float)CameraH;
	lpVertexG->z = -8.f / ev[2].v.z;
	lpVertexG->r = 0; lpVertexG->g = 0; lpVertexG->b = 255; lpVertexG->a = 255;
	//lpVertexG->rhw = lpVertexG->sz * 0.125f;
	//lpVertexG->color = (int)(255 - scrp[2].Light * 4) * 0x00010101 | alpha3 << 24;
	//lpVertexG->specular = (255 - (int)ev[2].Fog) << 24;//0x7F000000;
	lpVertexG->s = (float)(scrp[2].tx) / (128.f);
	lpVertexG->t = (float)(scrp[2].ty) / (128.f);

	/*if (hGTexture != hTexture) {
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
	}*/

	/*lpTriangle = (LPD3DTRIANGLE)lpInstructionG;
	lpTriangle->wV1 = GVCnt;
	lpTriangle->wV2 = GVCnt + 1;
	lpTriangle->wV3 = GVCnt + 2;
	lpTriangle->wFlags = 0;
	lpTriangle++;
	*lpwTriCount = (*lpwTriCount) + 1;
	lpInstructionG = (LPD3DINSTRUCTION)lpTriangle;

	GVCnt += 3;*/

	for (unsigned i = 0; i < 3; i++)
		g_VertexBufferData.push_back(vertex[i]);
}


void ProcessWaterMap(int x, int y, int r)
{
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
	int ob = OMap[y][x];

	if (!MODELS) ob = 255;
	ReverseOn = (FMap[y][x] & fmReverse);
	TDirection = (FMap[y][x] & 3);

	if (UNDERWATER) {
		if (!t1) t1 = 1;
		if (!t2) t2 = 1;
		NeedWater = true;
	}

	// RH: Not used?
	//float dfi = (float)((FMap[y][x] >> 2) & 7) * 2.f * pi / 8.f;

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

	/*if (MIPMAP && (zs > 256 * 10 && t1 || LOWRESTX)) {
		int TW, TH;
		Textures[t1].getMipmap().getDimensions(TW, TH);
		d3dSetTexture(Textures[t1].getMipmap().getData(), TW, TH);
	}
	else {
		int TW, TH;
		Textures[t1].getDimensions(TW, TH);
		d3dSetTexture(Textures[t1].getData(), 128, 128);
	}*/

	if (r > 8) DrawTPlane(false);
	else DrawTPlaneClip(false);

	if (ReverseOn) {
		ev[0] = ev[2];
		ev[2] = VMap[y + 1][x + 1];
	}
	else {
		ev[1] = ev[2];
		ev[2] = VMap[y + 1][x];
	}


	/*if (MIPMAP && (zs > 256 * 10 && t2 || LOWRESTX)) {
		int TW, TH;
		Textures[t2].getMipmap().getDimensions(TW, TH);
		d3dSetTexture(Textures[t2].getMipmap().getData(), TW, TH);
	}
	else {
		int TW, TH;
		Textures[t2].getDimensions(TW, TH);
		d3dSetTexture(Textures[t2].getData(), 128, 128);
	}*/

	if (r > 8) DrawTPlane(true);
	else DrawTPlaneClip(true);

	/*x = x + CCX - 64;
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

		//CalcFogLevel_Gradient(v[0]);

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

	}*/

	//if (UNDERWATER) ProcessWaterMap(x, y, r);
}


void BuildTreeNoSort()
{
	Vector2di v[3];
	Current = -1;
	int LastFace = -1;
	TFace* fptr;
	int sg;

	for (int f = 0; f < mptr->gFace.size(); f++)
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
	if (std::fabs(y0) > -(z0 - 256 * 6)) return;
}







void RenderShadowClip(TModel* _mptr,
	float xm0, float ym0, float zm0,
	float x0, float y0, float z0, float cal, float al, float bt)
{
}





void RenderModelClip(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	if (std::fabs(y0) > -(z0 - 256.f * 6.f)) return;
}






void RenderModelSun(TModel* _mptr, float x0, float y0, float z0, int Alpha)
{
}






void RenderNearModel(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	BOOL b = LOWRESTX;
	Vector3d v;
	v.x = 0; v.y = -128; v.z = 0;

	//CalcFogLevel_Gradient(v);
	//FogYGrad = 0;

	LOWRESTX = FALSE;
	RenderModelClip(_mptr, x0, y0, z0, light, al, bt);
	LOWRESTX = b;
}


/*
RenderSoft only
*/
void RenderModelClipWater(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
}


/*
RenderSoft only
*/
void RenderCharacter(int index)
{
}


/*
RenderSoft only
*/
void RenderExplosion(int index)
{
}


/*
RenderSoft only
*/
void RenderShip()
{
}


/*
Hardware (3Dfx, Direct3D, OpenGL) only
*/
void RenderCharacterPost(TCharacter* cptr)
{
	if (!cptr)
		return;

	CreateChMorphedModel(*cptr);

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

	if (cptr->rpos.z > -256 * 10)
		RenderModelClip(&cptr->pinfo->Model,
			cptr->rpos.x, cptr->rpos.y, cptr->rpos.z, 10,
			-cptr->alpha + pi / 2 + CameraAlpha,
			CameraBeta);
	else
		RenderModel(&cptr->pinfo->Model,
			cptr->rpos.x, cptr->rpos.y, cptr->rpos.z, 10,
			-cptr->alpha + pi / 2 + CameraAlpha,
			CameraBeta);


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


/*
Hardware (3Dfx, Direct3D, OpenGL) only
*/
void RenderExplosionPost(TExplosion* eptr)
{
	CreateMorphedModel(&ExplodInfo.Model, &ExplodInfo.Animation[0], eptr->FTime);

	if (std::fabs(eptr->rpos.z) + std::fabs(eptr->rpos.x) < 800)
		RenderModelClip(&ExplodInfo.Model,
			eptr->rpos.x, eptr->rpos.y, eptr->rpos.z, 0, 0, 0);
	else
		RenderModel(&ExplodInfo.Model,
			eptr->rpos.x, eptr->rpos.y, eptr->rpos.z, 0, 0, 0);
}


/*
Hardware (3Dfx, Direct3D, OpenGL) only
*/
void RenderShipPost()
{
	if (Ship.State == -1) return;

	GlassL = 0;
	int zs = (int)VectorLength(Ship.rpos);

	if (zs > 256 * (ctViewR)) return;

	if (zs > 256 * (ctViewR - 4))
		GlassL = min(255, (int)(zs - 256 * (ctViewR - 4)) / 4);

	CreateMorphedModel(&ShipModel.Model, &ShipModel.Animation[0], Ship.FTime);

	if (std::fabs(Ship.rpos.z) < 4000)
		RenderModelClip(&ShipModel.Model,
			Ship.rpos.x, Ship.rpos.y, Ship.rpos.z, 10, -Ship.alpha - pi / 2 + CameraAlpha, CameraBeta);
	else
		RenderModel(&ShipModel.Model,
			Ship.rpos.x, Ship.rpos.y, Ship.rpos.z, 10, -Ship.alpha - pi / 2 + CameraAlpha, CameraBeta);
}


/*
Software Only
*/
void RenderPlayer(int index)
{
}


/*
Hardware (3Dfx, Direct3D, OpenGL) only
*/
void Render3DHardwarePosts()
{
	TCharacter* cptr;
	for (int c = 0; c < ChCount; c++) {
		cptr = &Characters[c];
		cptr->rpos.x = cptr->pos.x - CameraX;
		cptr->rpos.y = cptr->pos.y - CameraY;
		cptr->rpos.z = cptr->pos.z - CameraZ;


		float r = max(std::fabs(cptr->rpos.x), std::fabs(cptr->rpos.z));
		int ri = -1 + (int)(r / 256.f + 0.5f);
		if (ri < 0) ri = 0;
		if (ri > ctViewR) continue;

		//if (FOGON)
		//	CalcFogLevel_Gradient(cptr->rpos);

		cptr->rpos = RotateVector(cptr->rpos);

		float br = BackViewR + DinoInfo[cptr->CType].Radius;
		if (cptr->rpos.z > br) continue;
		if (std::fabs(cptr->rpos.x) > -cptr->rpos.z + br) continue;
		if (std::fabs(cptr->rpos.y) > -cptr->rpos.z + br) continue;

		RenderCharacterPost(cptr);
	}

	TExplosion* eptr;
	for (int c = 0; c < ExpCount; c++) {

		eptr = &Explosions[c];
		eptr->rpos.x = eptr->pos.x - CameraX;
		eptr->rpos.y = eptr->pos.y - CameraY;
		eptr->rpos.z = eptr->pos.z - CameraZ;


		float r = max(std::fabs(eptr->rpos.x), std::fabs(eptr->rpos.z));
		int ri = -1 + (int)(r / 256.f + 0.4f);
		if (ri < 0) ri = 0;
		if (ri > ctViewR) continue;

		eptr->rpos = RotateVector(eptr->rpos);

		if (eptr->rpos.z > BackViewR) continue;
		if (std::fabs(eptr->rpos.x) > -eptr->rpos.z + BackViewR) continue;
		RenderExplosionPost(eptr);
	}


	Ship.rpos.x = Ship.pos.x - CameraX;
	Ship.rpos.y = Ship.pos.y - CameraY;
	Ship.rpos.z = Ship.pos.z - CameraZ;
	float r = max(std::fabs(Ship.rpos.x), std::fabs(Ship.rpos.z));

	int ri = -1 + (int)(r / 256.f + 0.2f);
	if (ri < 0) ri = 0;
	if (ri < ctViewR) {
		//if (FOGON)
		//	CalcFogLevel_Gradient(Ship.rpos);

		Ship.rpos = RotateVector(Ship.rpos);
		if (Ship.rpos.z > BackViewR) goto NOSHIP;
		if (std::fabs(Ship.rpos.x) > -Ship.rpos.z + BackViewR) goto NOSHIP;

		RenderShipPost();
	}
NOSHIP:;

	//SunLight *= GetTraceK(SunScrX, SunScrY);
}





void ClearVideoBuf()
{
}



int  CircleCX, CircleCY;
uint16_t CColor;

void DrawCircle(int cx, int cy, int R)
{
}


void DrawHMap()
{
	DrawPicture(VideoCX - MapPic.W / 2, VideoCY - MapPic.H / 2 - 6, MapPic);

	// TODO: Implement radar
}


/*
Local
*/
void RenderSun(float x, float y, float z)
{
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
}



void RenderFSRect(DWORD Color)
{
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
}


void Render_Cross(int sx, int sy)
{
}


bool R_InitShaders()
{
	HRESULT hres = S_OK;
	std::fstream f;
	bool sh_exists = false;
	ID3DBlob* vs = nullptr,
		* ps = nullptr,
		* vse = nullptr,
		* pse = nullptr;
	uint8_t* byte_code = nullptr;
	size_t byte_code_len = 0;

	unsigned shaderCompileFlags = D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
#if defined(_DEBUG)
	shaderCompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	shaderCompileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL1; // Use D3DCOMPILE_OPTIMIZATION_LEVEL3 for final builds
#endif // _DEBUG

	std::cout << "Initialising shaders..." << std::endl;

#if !defined(_DEBUG)
	// Check if the shader binaries exist
	f.open("huntdat/shaders/hunt.vsb", std::ios::in | std::ios::binary);
	sh_exists = f.is_open();
#endif // _DEBUG

	if (!sh_exists) {
		std::cout << "Vertex shader binary not found, compiling from source..." << std::endl;

		hres = D3DCompileFromFile(L"huntdat/shaders/hunt.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VertexMain", "vs_5_0", shaderCompileFlags, 0, &vs, &vse);
		if (FAILED(hres)) {
			std::cout << "Error Code: " << std::hex << hres << "\n" << (char*)vse->GetBufferPointer() << std::endl;
			throw dohalt("Failed to compile vertex shader.", __FUNCTION__, __FILE__, __LINE__);
			return false;
		}

		std::cout << "Vertex shader binary compiled, writing binary to disk..." << std::endl;

		f.open("huntdat/shaders/hunt.vsb", std::ios::out | std::ios::binary);
		if (f.is_open()) {
			f.write((char*)vs->GetBufferPointer(), vs->GetBufferSize());
			f.close();
			std::cout << "Vertex shader binary saved succesfully!" << std::endl;
		}
	}
	else {
		std::cout << "Reading vertex shader binary from disk..." << std::endl;

		f.seekg(0, std::ios::end);
		byte_code_len = f.tellg();
		f.seekg(0, std::ios::beg);
		byte_code = new uint8_t[byte_code_len];
		f.read((char*)byte_code, byte_code_len);
		f.close();
	}

	std::cout << "Creating vertex shader..." << std::endl;

	if (vs)	hres = g_Device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &g_VertexShader);
	else	hres = g_Device->CreateVertexShader(byte_code, byte_code_len, nullptr, &g_VertexShader);
	if (FAILED(hres)) {
		throw dohalt("Failed to create vertex shader.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	ID3D11InputLayout* layout;    // global

	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	g_Device->CreateInputLayout(ied, 2, vs->GetBufferPointer(), vs->GetBufferSize(), &layout);
	g_Context->IASetInputLayout(layout);

	vs->Release();

#if !defined(_DEBUG)
	// Check if the shader binaries exist
	f.open("huntdat/shaders/hunt.psb", std::ios::in | std::ios::binary);
	sh_exists = f.is_open();
#endif // _DEBUG

	if (!sh_exists) {
		std::cout << "Pixel shader binary not found, compiling from source..." << std::endl;

		hres = D3DCompileFromFile(L"huntdat/shaders/hunt.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PixelMain", "ps_5_0", shaderCompileFlags, 0, &ps, &pse);
		if (FAILED(hres)) {
			std::cout << "Error Code: " << std::hex << hres << "\n" << (char*)pse->GetBufferPointer() << std::endl;
			throw dohalt("Failed to compile pixel shader.", __FUNCTION__, __FILE__, __LINE__);
			return false;
		}

		std::cout << "Vertex shader binary compiled, writing binary to disk..." << std::endl;

		f.open("huntdat/shaders/hunt.psb", std::ios::out | std::ios::binary);
		if (f.is_open()) {
			f.write((char*)ps->GetBufferPointer(), ps->GetBufferSize());
			f.close();
			std::cout << "Pixel shader binary saved succesfully!" << std::endl;
		}
	}
	else {
		std::cout << "Reading pixel shader binary from disk..." << std::endl;

		f.seekg(0, std::ios::end);
		byte_code_len = f.tellg();
		f.seekg(0, std::ios::beg);
		byte_code = new uint8_t[byte_code_len];
		f.read((char*)byte_code, byte_code_len);
		f.close();
	}

	std::cout << "Creating pixel shader..." << std::endl;

	if (ps)	hres = g_Device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &g_PixelShader);
	else	hres = g_Device->CreatePixelShader(byte_code, byte_code_len, nullptr, &g_PixelShader);
	if (FAILED(hres)) {
		throw dohalt("Failed to create pixel shader.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	std::cout << "Vertex and pixel shaders loaded successfully!" << std::endl;

	if (ps) ps->Release();

	std::cout << "Shader initialisation: Ok!" << std::endl;

	return true;
}


void R_StartVBuffer()
{
	if (g_VertexBufferMap.pData != nullptr)
		return;

	HRESULT hres = g_Context->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &g_VertexBufferMap);
	if (FAILED(hres)) {
		throw dohalt("Failed to map D3D vertex buffer object.", __FUNCTION__, __FILE__, __LINE__);
		return;
	}

	g_VertexBufferData.clear();
}


void R_EndVBuffer()
{
	if (g_VertexBufferMap.pData == nullptr)
		return;

	std::memcpy(g_VertexBufferMap.pData, g_VertexBufferData.data(), g_VertexBufferData.size() * sizeof(VertexBufStruct));
	g_Context->Unmap(g_VertexBuffer, 0);

	if (g_VertexBufferData.size() < 3) // If there aren't enough vertices to make a triangle then return
		return;

	// Select which vertex buffer to display
	unsigned stride = sizeof(VertexBufStruct);
	unsigned offset = 0;
	g_Context->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);

	// Select which primtive type we are using
	g_Context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw the vertex buffer to the back buffer
	g_Context->Draw(g_VertexBufferData.size() / 3, 0);

	std::memset(&g_VertexBufferMap, 0, sizeof(g_VertexBufferMap));
}

void R_StartVBufferG()
{
	R_StartVBuffer();
}


void R_EndVBufferG()
{
	R_EndVBuffer();
}

#endif //_ogl