#define INITGUID
#include "Hunt.h"
#include <cstdio>
#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <exception>
#include <stdexcept>



typedef struct _TMenuSet {
	int x0, y0;
	int Count;
	std::string Item[32];
} TMenuSet;


TMenuSet Options[3];
char HMLtxt[3][12];
char CKtxt[2][16];
std::vector<std::pair<int, int>> Resolutions;
char Textxt[3][12];
char Ontxt[2][12];
char Systxt[2][12];
int CurPlayer = -1;
int MaxDino, AreaMax;

#define REGLISTX 320
#define REGLISTY 370

bool NEWPLAYER = false;

int  MapVKKey(int k)
{
	if (k == VK_LBUTTON) return 124;
	if (k == VK_RBUTTON) return 125;
	return MapVirtualKey(k, 0);
}

void AddMenuItem(TMenuSet& ms, const std::string& txt)
{
	ms.Item[ms.Count++] = txt;
}


POINT p;
int OptMode = 0;
int OptLine = 0;


void wait_mouse_release()
{
	while (GetAsyncKeyState(VK_RBUTTON) & 0x80000000);
	while (GetAsyncKeyState(VK_LBUTTON) & 0x80000000);

}


int GetTextW(HDC hdc, const char* s)
{
	SIZE sz;
	GetTextExtentPoint(hdc, s, strlen(s), &sz);
	return sz.cx;
}

int GetTextW(HDC hdc, const std::string& s)
{
	SIZE sz;
	GetTextExtentPoint(hdc, s.c_str(), s.length(), &sz);
	return sz.cx;
}

void PrintText(const char* s, int x, int y, int rgb)
{
	HBITMAP hbmpOld = static_cast<HBITMAP>(SelectObject(hdcCMain, hbmpVideoBuf));
	SetBkMode(hdcCMain, TRANSPARENT);

	SetTextColor(hdcCMain, 0x00000000);
	TextOut(hdcCMain, x + 1, y + 1, s, strlen(s));
	SetTextColor(hdcCMain, rgb);
	TextOut(hdcCMain, x, y, s, strlen(s));

	SelectObject(hdcCMain, hbmpOld);
}

void PrintText(const std::string& s, int x, int y, int rgb)
{
	HBITMAP hbmpOld = static_cast<HBITMAP>(SelectObject(hdcCMain, hbmpVideoBuf));
	SetBkMode(hdcCMain, TRANSPARENT);

	SetTextColor(hdcCMain, 0x00000000);
	TextOut(hdcCMain, x + 1, y + 1, s.c_str(), s.length());
	SetTextColor(hdcCMain, rgb);
	TextOut(hdcCMain, x, y, s.c_str(), s.length());

	SelectObject(hdcCMain, hbmpOld);
}


/*
DEPRECATED
Halt the application with `Mess` as an explanation
*/
void DoHalt(const char* Mess)
{
	if (!HARD3D && DirectActive && FULLSCREEN) {
		lpDD->RestoreDisplayMode();
	}

	EnableWindow(hwndMain, FALSE);

	if (strlen(Mess)) {
		ShowCursor(true);
		std::cout << "ABNORMAL_HALT: " << Mess << std::endl;
		MessageBox(HWND_DESKTOP, Mess, "HUNT Termination", IDOK | MB_SYSTEMMODAL);
	}

	throw std::runtime_error(Mess);
	TerminateProcess(GetCurrentProcess(), 0);
}

/*
Halt the application with `Mess` as an explanation
TODO: Implement DoHalt(const char*) contents here and use C++ exception instead of TerminateProcess.
*/
void DoHalt(const std::string& Mess)
{
	DoHalt(Mess.c_str());
}


void WaitRetrace()
{
	/*
	This function is entirely pointless in modern Windows as of Windows Vista
	Windows DWM already handles waiting for vertical blank for GDI and other
	WinApi drawing features, it actually can cause problems for people
	using Windows Vista and up if they have FreeSync or G-Sync monitors!
	*/
	/*BOOL bv = FALSE;
	if (DirectActive)
		while (!bv)  lpDD->GetVerticalBlankStatus(&bv);*/
}


void SetFullScreen()
{
	SetCursorPos(VideoCX, VideoCY);
}


void Wait(int time)
{
	unsigned int t = TimerGetMS() + time;
	while (t > TimerGetMS());
}



void SetVideoMode(int W, int H)
{
	WinW = W;
	WinH = H;

	WinEX = WinW - 1;
	WinEY = WinH - 1;
	VideoCX = WinW / 2;
	VideoCY = WinH / 2;

	CameraW = (float)VideoCX * 1.25f;
	CameraH = CameraW;

	if (g_FullScreen) {
		SetWindowPos(hwndMain, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
		SetCursorPos(VideoCX, VideoCY);
	}
	else {
		int WX = (GetSystemMetrics(SM_CXSCREEN) / 2) - 400;
		int WY = (GetSystemMetrics(SM_CYSCREEN) / 2) - 300;
		VideoCX += WX;
		VideoCY += WY;
		RECT rc = { WX, WY, WX + 800, WY + 600 };
		AdjustWindowRect(&rc, GetWindowLong(hwndMain, GWL_STYLE), false);
		SetWindowPos(hwndMain, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
		SetCursorPos(VideoCX, VideoCY);
	}

	LoDetailSky = (W > 400);
	SetCursor(hcArrow);

	if (g_FullScreen)
		while (ShowCursor(FALSE) >= 0);
	else
		while (ShowCursor(TRUE) < 0);
}


void SetMenuVideoMode()
{
	SetWindowLong(hwndMain, GWL_STYLE, WS_VISIBLE | WS_BORDER | WS_CAPTION | WS_POPUP);

	int WX = (GetSystemMetrics(SM_CXSCREEN) / 2) - 400;
	int WY = (GetSystemMetrics(SM_CYSCREEN) / 2) - 300;
	RECT rc = { WX, WY, WX + 800, WY + 600 };
	AdjustWindowRect(&rc, GetWindowLong(hwndMain, GWL_STYLE), false);
	SetWindowPos(hwndMain, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
	SetCursorPos(WX + 400, WY + 300);

	/*//Original:
	SetWindowPos(hwndMain, HWND_TOP, 0, 0, 800, 600, SWP_SHOWWINDOW);
	SetCursorPos(400, 300);
	*/

	while (ShowCursor(true) < 0);
}




void ReloadDinoInfo()
{
	switch (TargetDino) {
	case 0: LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mpara.tga");
		LoadPictureTGA(DinoPicM, "huntdat/menu/dinopic/mpara_on.tga");
		if (OptSys)
			LoadTextFile(DinoText, "huntdat/menu/dinopic/para.txu");
		else
			LoadTextFile(DinoText, "huntdat/menu/dinopic/para.txm");
		break;
	case 1: LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mpach.tga");
		LoadPictureTGA(DinoPicM, "huntdat/menu/dinopic/mpach_on.tga");
		if (OptSys)
			LoadTextFile(DinoText, "huntdat/menu/dinopic/pach.txu");
		else
			LoadTextFile(DinoText, "huntdat/menu/dinopic/pach.txm");
		break;
	case 2: LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/msteg.tga");
		LoadPictureTGA(DinoPicM, "huntdat/menu/dinopic/msteg_on.tga");
		if (OptSys)
			LoadTextFile(DinoText, "huntdat/menu/dinopic/steg.txu");
		else
			LoadTextFile(DinoText, "huntdat/menu/dinopic/steg.txm");
		break;
	case 3: LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mallo.tga");
		LoadPictureTGA(DinoPicM, "huntdat/menu/dinopic/mallo_on.tga");
		if (OptSys)
			LoadTextFile(DinoText, "huntdat/menu/dinopic/allo.txu");
		else
			LoadTextFile(DinoText, "huntdat/menu/dinopic/allo.txm");
		break;

	case 4: if (MaxDino < 4)
		LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mtric_no.tga");
		  else {
		LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mtric.tga");
		LoadPictureTGA(DinoPicM, "huntdat/menu/dinopic/mtric_on.tga");
	}
		  if (OptSys)
			  LoadTextFile(DinoText, "huntdat/menu/dinopic/tric.txu");
		  else
			  LoadTextFile(DinoText, "huntdat/menu/dinopic/tric.txm");
		  break;
	case 5: if (MaxDino < 4)
		LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mvelo_no.tga");
		  else {
		LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mvelo.tga");
		LoadPictureTGA(DinoPicM, "huntdat/menu/dinopic/mvelo_on.tga");
	}
		  if (OptSys)
			  LoadTextFile(DinoText, "huntdat/menu/dinopic/velo.txu");
		  else
			  LoadTextFile(DinoText, "huntdat/menu/dinopic/velo.txm");
		  break;

	case 6: if (MaxDino < 6)
		LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mtrex_no.tga");
		  else {
		LoadPictureTGA(DinoPic, "huntdat/menu/dinopic/mtrex.tga");
		LoadPictureTGA(DinoPicM, "huntdat/menu/dinopic/mtrex_on.tga");
	}
		  if (OptSys)
			  LoadTextFile(DinoText, "huntdat/menu/dinopic/trex.txu");
		  else
			  LoadTextFile(DinoText, "huntdat/menu/dinopic/trex.txm");
		  break;
	}
}


void ReloadWeaponInfo()
{
	switch (TargetWeapon) {
	case 0:
		LoadPictureTGA(WepPic, "huntdat/menu/weppic/weapon1.tga");
		LoadTextFile(WepText, "huntdat/menu/weppic/weapon1.txt");
		break;
	case 1:
		LoadPictureTGA(WepPic, "huntdat/menu/weppic/weapon2.tga");
		LoadTextFile(WepText, "huntdat/menu/weppic/weapon2.txt");
		break;
	case 2:
		if (TrophyRoom.Rank < 1)
			LoadPictureTGA(WepPic, "huntdat/menu/weppic/weapon3a.tga");
		else
			LoadPictureTGA(WepPic, "huntdat/menu/weppic/weapon3.tga");
		LoadTextFile(WepText, "huntdat/menu/weppic/weapon3.txt");
		break;
	}
}

void ReloadAreaInfo()
{
	int target_area = TargetArea + 1;
	std::stringstream aname, tname;

	if (TargetArea > AreaMax)	aname << "HUNTDAT/MENU/LANDPIC/area" << target_area << "_no.bmp";
	else						aname << "HUNTDAT/MENU/LANDPIC/area" << target_area << ".bmp";
	tname << "HUNTDAT/MENU/LANDPIC/area" << target_area << ".txt";
	LoadPicture(LandPic, aname.str().c_str());
	LoadTextFile(LandText, tname.str().c_str());
}


#include "Targa.h"

void LoadMenuTGA()
{
	std::string m1, m2, mm;
	MenuSelect = 0;

	switch (MenuState) {
	case -3:
		m1 = "HUNTDAT/MENU/menud.tga";
		m2 = "HUNTDAT/MENU/menud_on.tga";
		mm = "HUNTDAT/MENU/md_map.raw";
		break;
	case -2:
		m1 = "HUNTDAT/MENU/menul.tga";
		m2 = "HUNTDAT/MENU/menul_on.tga";
		mm = "HUNTDAT/MENU/ml_map.raw";
		break;
	case -1:
		m1 = "HUNTDAT\\MENU\\MENUR.TGA";
		m2 = "HUNTDAT/MENU/menur_on.tga";
		mm = "HUNTDAT/MENU/mr_map.raw";
		break;
	case 0:
		m1 = "HUNTDAT/MENU/menum.tga";
		m2 = "HUNTDAT/MENU/menum_on.tga";
		mm = "HUNTDAT/MENU/main_map.raw";
		break;
	case 1:
		m1 = "HUNTDAT/MENU/loc_off.tga";
		m2 = "HUNTDAT/MENU/loc_on.tga";
		mm = "HUNTDAT/MENU/loc_map.raw";
		break;
	case 2:
		m1 = "HUNTDAT/MENU/wep_off.tga";
		m2 = "HUNTDAT/MENU/wep_on.tga";
		mm = "HUNTDAT/MENU/wep_map.raw";
		break;
	case 3:
		m1 = "HUNTDAT/MENU/opt_off.tga";
		m2 = "HUNTDAT/MENU/opt_on.tga";
		mm = "HUNTDAT/MENU/opt_map.raw";
		break;
	case 4:
		m1 = "HUNTDAT/MENU/credits.tga";
		m2 = "HUNTDAT/MENU/credits.tga";
		mm = "";
		break;
	case 8:
		m1 = "HUNTDAT/MENU/menuq.tga";
		m2 = "HUNTDAT/MENU/menuq_on.tga";
		mm = "HUNTDAT/MENU/mq_map.raw";
		break;
	case 111:
		m1 = "HUNTDAT/MENU/menus.tga";
		m2 = "HUNTDAT/MENU/menus.tga";
		mm = "";
		break;
	case 112:
		m1 = "HUNTDAT/MENU/menua.tga";
		m2 = "HUNTDAT/MENU/menua.tga";
		mm = "";
		break;
	case 113:
		m1 = "HUNTDAT/MENU/menue.tga";
		m2 = "HUNTDAT/MENU/menue.tga";
		mm = "";
		break;
	}

	std::fstream f;

	f.open(m1, std::ios::in | std::ios::binary);
	if (f.is_open()) {
		uint16_t* lpImage = (uint16_t*)lpMenuBuf;
		f.seekg(18);
		for (int y = 599; y >= 0; --y)
			f.read((char*)&lpImage[y * 800], 800 * sizeof(uint16_t));
		f.close();
	}
	else {
		std::stringstream es;
		es << "Failed to open menu1 file '" << m1 << "'" << std::endl;
		throw dohalt(es.str(), __FUNCSIG__, __FILE__, __LINE__);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(2));

	f.open(m2, std::ios::in | std::ios::binary);
	if (f.is_open()) {
		f.seekg(18);
		for (int y = 599; y >= 0; --y)
			f.read((char*)((uint16_t*)lpMenuBuf2 + y * 800), 800 * sizeof(uint16_t));
		f.close();
	}
	else {
		std::stringstream es;
		es << "Failed to open menu2 file \'" << m2 << "\'\n" << std::endl;
		throw dohalt(es.str(), __FUNCSIG__, __FILE__, __LINE__);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(2));

	std::memset(MenuMap, 0, sizeof(MenuMap));

	f.open(mm, std::ios::in | std::ios::binary);
	if (f.is_open()) {
		f.seekg(18);
		f.read((char*)MenuMap, 400 * 300);
		f.close();
	}
	else {
		//std::stringstream es;
		//es << "Failed to open menumap file \'" << mm << "\'\n" << std::endl;
		//throw dohalt(es.str(), __FUNCSIG__, __FILE__, __LINE__);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
}






void ShowMenuVideo()
{
	HDC& _hdc = hdcCMain;
	HBITMAP hbmpOld = static_cast<HBITMAP>(SelectObject(_hdc, hbmpVideoBuf));
	if (RestartMode)
		std::memset(lpVideoBuf, 0, 1024 * (768 * sizeof(uint16_t)));
	BitBlt(hdcMain, 0, 0, 800, 600, _hdc, 0, 0, SRCCOPY);
	SelectObject(_hdc, hbmpOld);
}




void AddPicture(TPicture& pic, int x0, int y0)
{
	for (int y = 0; y < pic.H; y++)
		std::memcpy((uint16_t*)lpVideoBuf + x0 + ((y0 + y) << 10), pic.lpImage + (y * pic.W), pic.W * sizeof(uint16_t));
}


void Line(HDC hdc, int x1, int y1, int x2, int y2)
{
	MoveToEx(hdc, x1, y1, NULL);
	LineTo(hdc, x2, y2);
}



void DrawSlider(int x, int y, float l)
{
	int W = 120; y += 13;

	HPEN wp = CreatePen(PS_SOLID, 0, 0x009F9F9F);
	HBRUSH wb = CreateSolidBrush(0x003FAF3F);

	HPEN oldpen = static_cast<HPEN>(SelectObject(hdcCMain, GetStockObject(BLACK_PEN)));
	HBRUSH  oldbrs = static_cast<HBRUSH>(SelectObject(hdcCMain, GetStockObject(BLACK_BRUSH)));
	HBITMAP oldbmp = static_cast<HBITMAP>(SelectObject(hdcCMain, hbmpVideoBuf));


	x += 1; y += 1;
	Line(hdcCMain, x, y - 9, x + W + 1, y - 9);
	Line(hdcCMain, x, y, x + W + 1, y);
	Line(hdcCMain, x, y - 8, x, y);
	Line(hdcCMain, x + W, y - 8, x + W, y);
	Line(hdcCMain, x + W / 2, y - 8, x + W / 2, y);
	Line(hdcCMain, x + W / 4, y - 8, x + W / 4, y);
	Line(hdcCMain, x + W * 3 / 4, y - 8, x + W * 3 / 4, y);

	x -= 1; y -= 1;
	SelectObject(hdcCMain, wp);
	Line(hdcCMain, x, y - 9, x + W + 1, y - 9);
	Line(hdcCMain, x, y, x + W + 1, y);
	Line(hdcCMain, x, y - 8, x, y);
	Line(hdcCMain, x + W, y - 8, x + W, y);
	Line(hdcCMain, x + W / 2, y - 8, x + W / 2, y);
	Line(hdcCMain, x + W / 4, y - 8, x + W / 4, y);
	Line(hdcCMain, x + W * 3 / 4, y - 8, x + W * 3 / 4, y);

	W -= 2;
	PatBlt(hdcCMain, x + 2, y - 5, (int)(W * l / 2.f), 4, PATCOPY);

	SelectObject(hdcCMain, wb);
	PatBlt(hdcCMain, x + 1, y - 6, (int)(W * l / 2.f), 4, PATCOPY);


	SelectObject(hdcCMain, oldpen);
	SelectObject(hdcCMain, oldbrs);
	SelectObject(hdcCMain, oldbmp);
	DeleteObject(wp);
	DeleteObject(wb);
}


void DrawOptions()
{
	HFONT oldfont = static_cast<HFONT>(SelectObject(hdcCMain, fnt_BIG));

	for (int m = 0; m < 3; m++)
		for (int l = 0; l < Options[m].Count; l++) {
			int x0 = Options[m].x0;
			int y0 = Options[m].y0 + l * 25;

			int c = 0x005282b2;
			if (m == OptMode && l == OptLine) c = 0x00a0d0f0;
			PrintText(Options[m].Item[l],
				x0 - GetTextW(hdcCMain, Options[m].Item[l]), y0, c);


			c = 0xB0B0A0;
			x0 += 16;
			if (m == 0)
				switch (l) {
				case 0:
					PrintText(HMLtxt[OptAgres], x0, y0, c);
					break;
				case 1:
					PrintText(HMLtxt[OptDens], x0, y0, c);
					break;
				case 2:
					PrintText(HMLtxt[OptSens], x0, y0, c);
					break;
				case 3:
					PrintText(Systxt[OptSys], x0, y0, c);
					break;

				}

			if (m == 1)
				if (l < Options[m].Count - 1)
					if (WaitKey == l)
						PrintText("<?>", x0, y0, c);
					else
						PrintText(KeysName[MapVKKey(*((int*)(&KeyMap) + l))], x0, y0, c);
				else
					PrintText(Ontxt[REVERSEMS], x0, y0, c);

			if (m == 2)
				switch (l) {
				case 0: {
					std::stringstream ss;
					ss << Resolutions.at(OptRes).first << " x " << Resolutions.at(OptRes).second;
					PrintText(ss.str(), x0, y0, c);
				} break;
				case 1:
					PrintText(Ontxt[FOGENABLE], x0, y0, c);
					break;
				case 2:
					PrintText(Textxt[OptText], x0, y0, c);
					break;
				case 3:
					PrintText(Ontxt[SHADOWS3D], x0, y0, c);
					break;
				case 4:
					PrintText(CKtxt[OPT_ALPHA_COLORKEY], x0, y0, c);
					break;
				}

		}

	SelectObject(hdcCMain, oldfont);
}





void DrawMainStats()
{
	HFONT oldfont = static_cast<HFONT>(SelectObject(hdcCMain, fnt_BIG));
	int  c = 0x003070A0;

	PrintText(TrophyRoom.PlayerName, 90, 9, c);

	std::stringstream ss;
	ss << TrophyRoom.Score;
	PrintText(ss.str(), 540, 9, c);

	switch (TrophyRoom.Rank) {
	case 0: PrintText("Novice  ", 344, 9, c); break;
	case 1: PrintText("Advanced", 344, 9, c); break;
	case 2: PrintText("Expert  ", 344, 9, c); break;
	}

	SelectObject(hdcCMain, oldfont);
}


void DrawMainStats2()
{
	std::stringstream t;
	int  c = 0x00209090;
	int  ttm = (int)TrophyRoom.Total.time;
	int  ltm = (int)TrophyRoom.Last.time;

	PrintText("Path travelled  ", 718 - GetTextW(hdcCMain, "Path travelled  "), 78, c);

	if (OptSys)  t << (TrophyRoom.Last.path / 0.3f) << " ft.";
	else         t << TrophyRoom.Last.path << " m.";

	PrintText(t.str(), 718, 78, c);
	t.str(""); t.clear();

	PrintText("Time hunted  ", 718 - GetTextW(hdcCMain, "Time hunted  "), 98, c);
	t << (ltm / 3600) << ((ltm % 3600) / 60) << ":" << (ltm % 60);
	PrintText(t.str(), 718, 98, c);
	t.str(""); t.clear();

	PrintText("Shots made  ", 718 - GetTextW(hdcCMain, "Shots made  "), 118, c);
	t << TrophyRoom.Last.smade;
	PrintText(t.str(), 718, 118, c);
	t.str(""); t.clear();

	PrintText("Shots succeed  ", 718 - GetTextW(hdcCMain, "Shots succeed  "), 138, c);
	t << TrophyRoom.Last.ssucces;
	PrintText(t.str(), 718, 138, c);
	t.str(""); t.clear();

	PrintText("Path travelled  ", 718 - GetTextW(hdcCMain, "Path travelled  "), 208, c);
	if (TrophyRoom.Total.path < 1000)
		if (OptSys)  t << (TrophyRoom.Total.path / 0.3f) << " ft.";
		else        t << (TrophyRoom.Total.path) << " m.";
	else
		if (OptSys)  t << (TrophyRoom.Total.path / 1667) << " miles.";
		else         t << (TrophyRoom.Total.path / 1000.f) << " km.";

	PrintText(t.str(), 718, 208, c);
	t.str(""); t.clear();


	PrintText("Time hunted  ", 718 - GetTextW(hdcCMain, "Time hunted  "), 228, c);
	t << (ttm / 3600) << ((ttm % 3600) / 60) << ":" << (ttm % 60);
	PrintText(t.str(), 718, 228, c);
	t.str(""); t.clear();

	PrintText("Shots made  ", 718 - GetTextW(hdcCMain, "Shots made  "), 248, c);
	t << TrophyRoom.Total.smade;
	PrintText(t.str(), 718, 248, c);
	t.str(""); t.clear();

	PrintText("Shots succeed  ", 718 - GetTextW(hdcCMain, "Shots succeed  "), 268, c);
	t << TrophyRoom.Total.ssucces;
	PrintText(t.str(), 718, 268, c);
	t.str(""); t.clear();

	PrintText("Succes ratio  ", 718 - GetTextW(hdcCMain, "Succes ratio  "), 288, c);
	if (TrophyRoom.Total.smade)
		t << (TrophyRoom.Total.ssucces * 100 / TrophyRoom.Total.smade) << "%";
	else
		t << "100%";
	PrintText(t.str(), 718, 288, c);
	t.str(""); t.clear();

	DrawMainStats();
}


void DrawRegistry()
{
	int  c = 0x00309070;
	std::stringstream ss;

	//if ((timeGetTime() % 800) > 300)
	if ((TimerGetMS() % 800) > 300)
		ss << TrophyRoom.PlayerName << "_";
	else
		ss << TrophyRoom.PlayerName;

	PrintText(ss.str(), 330, 326, c);
	ss.str(""); ss.clear();

	for (int i = 0; i < 6; i++) {
		if (PlayerR[i].PName.empty()) {
			PrintText("...", REGLISTX, REGLISTY + i * 20, 0x003070A0);
			continue;
		}

		if (i == CurPlayer)
			c = 0x005090E0;
		else
			c = 0x003070A0;
		PrintText(PlayerR[i].PName, REGLISTX, REGLISTY + i * 20, c);

		if (PlayerR[i].Rank == 2)
			PrintText("Exp", REGLISTX + 146, REGLISTY + i * 20, c);
		else if (PlayerR[i].Rank == 1)
			PrintText("Adv", REGLISTX + 146, REGLISTY + i * 20, c);
		else
			PrintText("Nov", REGLISTX + 146, REGLISTY + i * 20, c);
	}
}



void DrawRemove()
{
	//HFONT oldfont = static_cast<HFONT>(SelectObject(hdcCMain, fnt_Small));
	std::stringstream ss;
	int  c = 0x00B08030;

	PrintText("Do you want to delete player", 290, 370, c);
	ss << "\'" << PlayerR[CurPlayer].PName << "\' ?";
	PrintText(ss.str(), 300, 394, c);

	//SelectObject(hdcCMain, oldfont);
}


void CopyMenuToVideo(int m)
{
	int smap[256];
	std::memset(smap, 0, 256 * sizeof(int));
	LastMenuSelect = MenuSelect;

	if (!m)
		m = 255;

	smap[m] = 1;

	if (MenuState == 1) {
		if (ObservMode)
			smap[3] = 1;
		else
			smap[3] = 0;
	}

	if (MenuState == 2) {
		if (Tranq)
			smap[3] = 1;
		else
			smap[3] = 0;

		if (ScentMode)
			smap[4] = 1;
		else
			smap[4] = 0;
		if (CamoMode)
			smap[5] = 1;
		else
			smap[5] = 0;
		if (RadarMode)
			smap[6] = 1;
		else
			smap[6] = 0;
	}

	if (MenuState == 3) {
		smap[1] = 0;
		smap[2] = 0;
		smap[3] = 0;
		smap[OptMode + 1] = 1;
	}

	int* msrc = nullptr;
	int black = 0;
	int mda = 0;

	for (int y = 0; y < 300; y++) {
		int* vbuf = (int*)lpVideoBuf + (y << 10);
		mda = y * 800;
		for (int x = 0; x < 400; x++) {
			if (smap[MenuMap[y][x]]) {
				msrc = (int*)lpMenuBuf2 + mda;
			}
			else {
				msrc = (int*)lpMenuBuf + mda;
			}

			*(vbuf) = *(msrc);
			*(vbuf + 512) = *(msrc + 400);
			vbuf++;
			mda++;
		}
	}


	if (MenuState == 0) DrawMainStats();
	if (MenuState == 8) DrawMainStats();
	if (MenuState == 112) DrawMainStats();
	if (MenuState == 113) DrawMainStats();

	if (MenuState == 111) DrawMainStats2();
	if (MenuState == -1) DrawRegistry();


	if (MenuState == 1) {
		AddPicture(LandPic, 143, 71);
		if (MenuSelect == 6 && (TargetDino > MaxDino)) m = 0;
		if (m == 6) AddPicture(DinoPicM, 401, 64);
		else AddPicture(DinoPic, 401, 64);

		for (int t = 0; t < DinoText.Lines; t++)
			PrintText(DinoText.Text[t], 420, 330 + t * 16, 0x209F85);

		PrintText("Sight", 520 - GetTextW(hdcCMain, "Sight"), 450 + 0 * 20, 0x209F85);
		PrintText("Scent", 520 - GetTextW(hdcCMain, "Scent"), 450 + 1 * 20, 0x209F85);
		PrintText("Hearing", 520 - GetTextW(hdcCMain, "Hearing"), 450 + 2 * 20, 0x209F85);

		DrawSlider(526, 450 + 0 * 20, DinoInfo[TargetDino + 4].LookK * 2);
		DrawSlider(526, 450 + 1 * 20, DinoInfo[TargetDino + 4].SmellK * 2);
		DrawSlider(526, 450 + 2 * 20, DinoInfo[TargetDino + 4].HearK * 2);


		if (MenuSelect == 3)
			for (int t = 0; t < ObserText.Lines; t++)
				PrintText(ObserText.Text[t], 50, 330 + t * 16, 0x809F25);
		else
			for (int t = 0; t < LandText.Lines; t++)
				PrintText(LandText.Text[t], 50, 330 + t * 16, 0x809F25);
	}

	if (MenuState == 2) {
		AddPicture(WepPic, 120, 120);
		for (int t = 0; t < WepText.Lines; t++)
			PrintText(WepText.Text[t], 60, 330 + t * 16, 0xB09F45);

		PrintText("Fire power:", 160 - GetTextW(hdcCMain, "Fire power:"), 454 + 0 * 20, 0xB09F45);
		PrintText("Shot precision:", 160 - GetTextW(hdcCMain, "Shot precision:"), 454 + 1 * 20, 0xB09F45);
		PrintText("Volume", 160 - GetTextW(hdcCMain, "Volume:"), 454 + 2 * 20, 0xB09F45);
		PrintText("Rate of fire:", 160 - GetTextW(hdcCMain, "Rate of fire:"), 454 + 3 * 20, 0xB09F45);

		DrawSlider(166, 454 + 0 * 20, WeapInfo[TargetWeapon].Power);
		DrawSlider(166, 454 + 1 * 20, WeapInfo[TargetWeapon].Prec);
		DrawSlider(166, 454 + 2 * 20, 2.0f - WeapInfo[TargetWeapon].Loud);
		DrawSlider(166, 454 + 3 * 20, WeapInfo[TargetWeapon].Rate);

		if (MenuSelect == 3)
			for (int t = 0; t < TranqText.Lines; t++)
				PrintText(TranqText.Text[t], 420, 330 + t * 16, 0xB09F45);

		if (MenuSelect == 4)
			for (int t = 0; t < ScentText.Lines; t++)
				PrintText(ScentText.Text[t], 420, 330 + t * 16, 0xB09F45);

		if (MenuSelect == 5)
			for (int t = 0; t < ComfText.Lines; t++)
				PrintText(ComfText.Text[t], 420, 330 + t * 16, 0xB09F45);

		if (MenuSelect == 6)
			for (int t = 0; t < RadarText.Lines; t++)
				PrintText(RadarText.Text[t], 420, 330 + t * 16, 0xB09F45);
	}

	if (MenuState == 3) DrawOptions();

	if (MenuState == -3) DrawRemove();
}






void SelectMenu0(int s)
{
	if (!s) return;
	AddVoice3d(fxMenuGo.length, fxMenuGo.lpData,
		(float)1024 + (p.x - 200) * 3, 0.f, 200.f);
	CopyMenuToVideo(0); ShowMenuVideo();        Wait(50);
	CopyMenuToVideo(s); ShowMenuVideo();
	LastMenuSelect = 255;

	switch (s) {
		//============ new =============//
	case 1:
		TrophyMode = false;
		MenuState = 1;
		LoadMenuTGA();
		ReloadDinoInfo();
		ReloadAreaInfo();
		break;
		//============ options =============//
	case 2:
		/*if (WinW == 320) OptRes = 0;
		if (WinW == 400) OptRes = 1;
		if (WinW == 512) OptRes = 2;
		if (WinW == 640) OptRes = 3;
		if (WinW == 800) OptRes = 4;
		if (WinW == 1024) OptRes = 5;*/

		MenuState = 3;
		LoadMenuTGA();
		break;
		//============ trphy =============//
	case 3:
		ProjectName = "huntdat/areas/trophy";
		TrophyMode = true;
		GameState = 1;
		break;
		//============ credits =============//
	case 4:
		MenuState = 4;
		LoadMenuTGA();
		break;
		//============ quit =============//
	case 5:
		//DestroyWindow(hwndMain);
		MenuState = 8;
		LoadMenuTGA();
		break;
	case 6:
		MenuState = 111;
		LoadMenuTGA();
		break;
	}

	wait_mouse_release();
}



void SelectMenu1(int s)
{
	if (!s) return;
	AddVoice3d(fxMenuGo.length, fxMenuGo.lpData,
		(float)1024 + (p.x - 200) * 3, 0, 200);
	CopyMenuToVideo(0);
	ShowMenuVideo();        Wait(50);

	CopyMenuToVideo(s);
	ShowMenuVideo();

	LastMenuSelect = 255;

	switch (s) {
	case 1:
		if (TargetArea < 5) TargetArea++; else TargetArea = 0;
		ReloadAreaInfo();
		break;
	case 2:
		if (TargetArea) TargetArea--; else TargetArea = 5;
		ReloadAreaInfo();
		break;
	case 3:
		ObservMode = !ObservMode;
		break;


	case 4:
		if (TargetDino < 6) TargetDino++; else TargetDino = 0;
		ReloadDinoInfo();
		break;
	case 5:
		if (TargetDino) TargetDino--; else TargetDino = 6;
		ReloadDinoInfo();
		break;

	case 7:
		MenuState = 0;
		LoadMenuTGA();
		break;
	case 8:
		if (TargetDino > MaxDino) {
			MessageBeep(0xFFFFFFFF);
			break;
		}
		if (TargetArea > AreaMax) {
			MessageBeep(0xFFFFFFFF);
			break;
		}

		///if (!ChInfo[TargetDino + 4].mptr) break;
		if (ObservMode) {
			std::stringstream ss;
			ss << "huntdat/areas/area" << (TargetArea + 1);
			ProjectName = ss.str();
			GameState = 1;
		}
		else {
			MenuState = 2;
			LoadMenuTGA();
			ReloadWeaponInfo();
		}
		break;
	}


	wait_mouse_release();
}


void SelectMenu2(int s)
{
	if (!s) return;
	AddVoice3d(fxMenuGo.length, fxMenuGo.lpData,
		(float)1024 + (p.x - 200) * 3, 0, 200);
	CopyMenuToVideo(0); ShowMenuVideo();  Wait(50);
	CopyMenuToVideo(s); ShowMenuVideo();

	LastMenuSelect = 255;


	switch (s) {
	case 1:
		if (TargetWeapon < 2) TargetWeapon++; else TargetWeapon = 0;
		ReloadWeaponInfo();
		break;
	case 2:
		if (TargetWeapon) TargetWeapon--; else TargetWeapon = 2;
		ReloadWeaponInfo();
		break;
	case 3:
		Tranq = !Tranq;
		break;

	case 4:
		ScentMode = !ScentMode;
		break;
	case 5:
		CamoMode = !CamoMode;
		break;
	case 6:
		RadarMode = !RadarMode;
		break;

	case 7:
		MenuState = 1;
		LoadMenuTGA();
		break;
	case 8:
		if (TrophyRoom.Rank < 1 && TargetWeapon>1) {
			MessageBeep(0xFFFFFFFF);
			break;
		}
		std::stringstream ss;
		ss << "huntdat/areas/area" << (TargetArea + 1);
		ProjectName = ss.str();
		GameState = 1;
		break;
	}


	wait_mouse_release();
}



void SelectOptions()
{
	switch (OptMode) {
	case 0:
		switch (OptLine) {
		case 0: OptAgres++; if (OptAgres > 2) OptAgres = 0; break;
		case 1: OptDens++; if (OptDens > 2) OptDens = 0; break;
		case 2: OptSens++; if (OptSens > 2) OptSens = 0; break;
		case 3: OptSys = 1 - OptSys; break;
		}
		break;

	case 2:
		switch (OptLine) {
		case 0: {
			OptRes++;
			if (OptRes >= Resolutions.size())
				OptRes = 0;
		} break;
		case 1: FOGENABLE = !FOGENABLE; break;
		case 2: OptText++; if (OptText > 2) OptText = 0; break;
		case 3: SHADOWS3D = !SHADOWS3D; break;
		case 4: OPT_ALPHA_COLORKEY = !OPT_ALPHA_COLORKEY; break;
		}
		break;

	case 1:
		if (OptLine == Options[1].Count - 1) REVERSEMS = !REVERSEMS;
		break;
	}
}

HCURSOR menucurs;



void ProcessMainMenu()
{
	if (KeyboardState[VK_LBUTTON] & 128)
		SelectMenu0(MenuSelect);

	if (MenuSelect != LastMenuSelect) {
		CopyMenuToVideo(MenuSelect);
		ShowMenuVideo();
	}
}


void ProcessCreditMenu()
{
	if (KeyboardState[VK_ESCAPE] & 128) KeyboardState[VK_LBUTTON] |= 128;
	if (KeyboardState[VK_RETURN] & 128) KeyboardState[VK_LBUTTON] |= 128;

	if (KeyboardState[VK_LBUTTON] & 128) {
		MenuState = 0;
		LoadMenuTGA();
		wait_mouse_release();
	}

	CopyMenuToVideo(MenuSelect);
	ShowMenuVideo();
}


void ProcessLicense()
{
	if (MenuSelect)
		if (KeyboardState[VK_LBUTTON] & 128) {

			AddVoice3d(fxMenuGo.length, fxMenuGo.lpData, (float)1024 + (p.x - 200) * 3, 0.f, 200.f);
			CopyMenuToVideo(0);
			ShowMenuVideo();
			Wait(50);
			CopyMenuToVideo(MenuSelect);
			ShowMenuVideo();
			LastMenuSelect = 255;
			wait_mouse_release();

			if (MenuSelect == 1) {
				MenuState = 0;
				LoadMenuTGA();
			}
			else {
				std::stringstream ss;
				ss << "trophy" << TrophyRoom.RegNumber << ".sav";
				DeleteFile(ss.str().c_str()); // RH: Not cross platform
				throw dohalt("Delete trophy file", __FUNCSIG__, __FILE__, __LINE__);
			}

		}

	CopyMenuToVideo(MenuSelect);
	ShowMenuVideo();
}


void RemovePlayer()
{
	if (CurPlayer == -1) return;
	TrophyRoom.PlayerName = "";
	std::stringstream ss;
	ss << "trophy" << CurPlayer << ".sav";
	DeleteFile(ss.str().c_str()); // RH: Not cross platform
	LoadPlayersInfo();
}



void ProcessRemove()
{
	if (MenuSelect)
		if (KeyboardState[VK_LBUTTON] & 128) {

			AddVoice3d(fxMenuGo.length, fxMenuGo.lpData, (float)1024 + (p.x - 200) * 3, 0.f, 200.f);
			CopyMenuToVideo(0);
			ShowMenuVideo();
			Wait(50);
			CopyMenuToVideo(MenuSelect);
			ShowMenuVideo();
			LastMenuSelect = 255;
			wait_mouse_release();

			if (MenuSelect == 1) {
				RemovePlayer();
				MenuState = -1;
				LoadMenuTGA();
			}
			else {
				MenuState = -1;
				LoadMenuTGA();
				return;
			}

		}

	CopyMenuToVideo(MenuSelect);
	ShowMenuVideo();
}


void IdentifyPlayer()
{
	CurPlayer = -1;
	if (TrophyRoom.PlayerName.empty())
		return;

	NEWPLAYER = false;
	//=== search for registered player =======//
	for (int i = 0; i < 6; i++)
		if (TrophyRoom.PlayerName == PlayerR[i].PName) {
			CurPlayer = i;
			return;
		}

	//=== search for free slot =======//
	for (int i = 0; i < 6; i++)
		if (PlayerR[i].PName.empty()) {
			NEWPLAYER = true;
			CurPlayer = i;
			break;
		}


	if (CurPlayer != -1) {
		TrophyRoom.RegNumber = CurPlayer;
		/*if (HARD3D)
			OptRes = 3;
		else
			OptRes = 2;*/
		RadarMode = false;
		ScentMode = false;
		CamoMode = false;
		SaveTrophy();
	}
}





void ProcessRegistry()
{
	if (KeyboardState[VK_RETURN] & 128) {
		MenuSelect = 1;
		KeyboardState[VK_LBUTTON] |= 128;
	}

	if (KeyboardState[VK_LBUTTON] & 128) {
		for (int i = 0; i < 6; i++)
			if (p.x << 1 > REGLISTX && p.x << 1 < REGLISTX + 200 &&
				p.y << 1 > REGLISTY + i * 20 && p.y << 1 < REGLISTY + i * 20 + 18)
				if (!PlayerR[i].PName.empty())
				{
					if (CurPlayer == i) MenuSelect = 1;
					CurPlayer = i;
					TrophyRoom.PlayerName = PlayerR[i].PName;
				}

		AddVoice3d(fxMenuGo.length, fxMenuGo.lpData, (float)1024 + (p.x - 200) * 3, 0.f, 200.f);
		CopyMenuToVideo(0);
		ShowMenuVideo();
		Wait(50);
		CopyMenuToVideo(MenuSelect);
		ShowMenuVideo();
		LastMenuSelect = 255;
		wait_mouse_release();

		if (MenuSelect == 2)
			if (CurPlayer >= 0)
				if (!PlayerR[CurPlayer].PName.empty()) {
					MenuState = -3;
					LoadMenuTGA();
					//RemovePlayer();	
				}

		if (MenuSelect == 1) {
			IdentifyPlayer();
			if (CurPlayer >= 0) {
				if (NEWPLAYER) MenuState = -2;
				else MenuState = 0;
				TrophyRoom.RegNumber = CurPlayer;
				LoadTrophy();
				LoadMenuTGA();
			}
		}
	}


	CopyMenuToVideo(MenuSelect);
	ShowMenuVideo();

}





void ProcessOptionsMenu()
{
	if (KeyboardState[VK_LBUTTON] & 128) {

		if (MenuSelect)
			AddVoice(fxMenuGo.length, fxMenuGo.lpData);

		if (!MenuSelect && WaitKey == -1) {
			for (int m = 0; m < 3; m++)
				for (int l = 0; l < Options[m].Count; l++) {
					int x0 = Options[m].x0;
					int y0 = Options[m].y0 + l * 25;
					if (p.x * 2 > x0 - 120 && p.x * 2 < x0 + 120 && p.y * 2 > y0 && p.y * 2 < y0 + 23) {
						OptMode = m; OptLine = l;
						if (m == 1)
							if (l < Options[m].Count - 1)
								WaitKey = l;
						AddVoice(fxMenuGo.length, fxMenuGo.lpData);
						SelectOptions();
						CopyMenuToVideo(0); ShowMenuVideo();
						//SetupKey();
					}
				}
		}

		// -- Resolution option
		if (MenuSelect)
			if (MenuSelect == 4) {
				auto screen_res = *Resolutions.begin();
				try {
					screen_res = Resolutions.at(OptRes);
				}
				catch (std::exception& e) {
					std::cout << "! EXCEPTION !\n\tFailed to set `screen_res` due to: " << e.what() << std::endl;
					screen_res = *Resolutions.begin();
				}

				WinW = screen_res.first;
				WinH = screen_res.second;

				SaveTrophy();
				CopyMenuToVideo(0);
				ShowMenuVideo();
				Wait(50);
				CopyMenuToVideo(MenuSelect);
				ShowMenuVideo();
				MenuState = 0;
				LoadMenuTGA();
			}
			else {
				OptMode = MenuSelect - 1;
				OptLine = 0;
			}
		wait_mouse_release();
	}

	CopyMenuToVideo(MenuSelect);

	ShowMenuVideo();
}


void ProcessLocationMenu()
{
	MaxDino = 3;
	AreaMax = 2;
	if (TrophyRoom.Rank) {
		MaxDino = 5;
		AreaMax = 4;
	}
	if (TrophyRoom.Rank == 2) {
		MaxDino = 6;
		AreaMax = 5;
	}

	if (KeyboardState[VK_LBUTTON] & 128)
		SelectMenu1(MenuSelect);

	if (MenuSelect != LastMenuSelect) {
		CopyMenuToVideo(MenuSelect);
		ShowMenuVideo();
	}
}


void ProcessWeaponMenu()
{
	if (TargetDino == 6) Tranq = FALSE;
	if (KeyboardState[VK_LBUTTON] & 128)
		SelectMenu2(MenuSelect);
	if (TargetDino == 6) Tranq = FALSE;

	if (MenuSelect != LastMenuSelect) {
		CopyMenuToVideo(MenuSelect);
		ShowMenuVideo();
	}
}


void ProcessQuitMenu()
{
	if (KeyboardState[VK_RETURN] & 128) {
		MenuSelect = 1; KeyboardState[VK_LBUTTON] |= 128;
	}

	if (KeyboardState[VK_ESCAPE] & 128) {
		MenuSelect = 2; KeyboardState[VK_LBUTTON] |= 128;
	}

	if (KeyboardState[VK_LBUTTON] & 128) {
		if (MenuSelect == 1) DestroyWindow(hwndMain);
		if (MenuSelect == 2) MenuState = 0;
		if (MenuSelect)    AddVoice(fxMenuGo.length, fxMenuGo.lpData);
	}

	if (KeyboardState[VK_ESCAPE] & 128) MenuState = 0;

	//  //SetCursor(hcArrow);
	CopyMenuToVideo(MenuSelect);
	ShowMenuVideo();

	if (MenuState != 8) LoadMenuTGA();
}



void ProcessMenu()
{

	if (_GameState != GameState) {
		std::cout << "\tEntered menu" << std::endl;
		LastMenuSelect = 255;

		if (MenuState > 0 && MenuState < 112) {
			MenuState = 0;
		}

		LoadMenuTGA();
		ShutDown3DHardware();
		SetMenuVideoMode();
		SetCursor(hcArrow);
		AudioStop();
		AudioSetCameraPos(1024, 0, 0, 0);
		if (MenuState >= 0 && !RestartMode)
			SetAmbient(fxMenuAmb.length,
				fxMenuAmb.lpData, 240);
	}
	_GameState = GameState;

	if (RestartMode) {
		GameState = 1;
		return;
	}

	///GetCursorPos(&p); p.x /= 2; p.y /= 2;
	GetCursorPos(&p);
	ScreenToClient(hwndMain, &p);
	p.x /= 2;
	p.y /= 2;
	if (p.x > 400 - 1) p.x = 400 - 1;
	if (p.y > 400 - 1) p.y = 400 - 1;

	int m = MenuSelect;
	MenuSelect = MenuMap[p.y][p.x];
	GetKeyboardState(KeyboardState);

	if (MenuState >= 0)
		SetAmbient3d(fxMenuAmb.length,
			fxMenuAmb.lpData,
			(float)1024 + (p.x - 200) * 2, 0, 200);

	if (m != MenuSelect && MenuSelect)
		AddVoice3d(fxMenuMov.length, fxMenuMov.lpData,
			(float)1024 + (p.x - 200) * 2, 0, 200);

	switch (MenuState) {
	case -3: ProcessRemove(); break;
	case -2: ProcessLicense(); break;
	case -1: ProcessRegistry(); break;
	case 0: ProcessMainMenu(); break;
	case 1: ProcessLocationMenu(); break;
	case 2: ProcessWeaponMenu(); break;
	case 3: ProcessOptionsMenu(); break;

	case 111:
	case 112:
	case 113:
	case 4: ProcessCreditMenu(); break;
	case 8: ProcessQuitMenu(); break;
	}
}


void InitMenu()
{
	Options[0].x0 = 200;
	Options[0].y0 = 100;
	AddMenuItem(Options[0], "Agressivity");
	AddMenuItem(Options[0], "Density");
	AddMenuItem(Options[0], "Sensitivity");
	AddMenuItem(Options[0], "Measurement");

	Options[2].x0 = 200;
	Options[2].y0 = 370;
	AddMenuItem(Options[2], "Resolution");
	AddMenuItem(Options[2], "Fog");
	AddMenuItem(Options[2], "Textures");
	AddMenuItem(Options[2], "Shadows");
	AddMenuItem(Options[2], "ColorKey");

	Options[1].x0 = 610;
	Options[1].y0 = 90;

	AddMenuItem(Options[1], "Forward");
	AddMenuItem(Options[1], "Backward");
	AddMenuItem(Options[1], "Turn Up");
	AddMenuItem(Options[1], "Turn Down");
	AddMenuItem(Options[1], "Turn Left");
	AddMenuItem(Options[1], "Turn Right");
	AddMenuItem(Options[1], "Fire");
	AddMenuItem(Options[1], "Get weapon");

	AddMenuItem(Options[1], "Step Left");
	AddMenuItem(Options[1], "Step Right");
	AddMenuItem(Options[1], "Strafe");
	AddMenuItem(Options[1], "Jump");
	AddMenuItem(Options[1], "Run");
	AddMenuItem(Options[1], "Crouch");
	AddMenuItem(Options[1], "Call");
	AddMenuItem(Options[1], "Binoculars");

	AddMenuItem(Options[1], "Reverse mouse");
	//AddMenuItem(Options[1], "Mouse sensitivity");

	wsprintf(CKtxt[0], "Color");
	wsprintf(CKtxt[1], "Alpha Channel");

	wsprintf(HMLtxt[0], "Low");
	wsprintf(HMLtxt[1], "Medium");
	wsprintf(HMLtxt[2], "High");

	Resolutions.clear();
	Resolutions.push_back(std::make_pair<int, int>(800, 600));
	Resolutions.push_back(std::make_pair<int, int>(1024, 768));
	Resolutions.push_back(std::make_pair<int, int>(1280, 960));
	Resolutions.push_back(std::make_pair<int, int>(1400, 1050));
	Resolutions.push_back(std::make_pair<int, int>(1440, 1080));
	Resolutions.push_back(std::make_pair<int, int>(1600, 1200));
	Resolutions.push_back(std::make_pair<int, int>(1856, 1392));
	Resolutions.push_back(std::make_pair<int, int>(1920, 1440));
	Resolutions.push_back(std::make_pair<int, int>(2048, 1536));
	// Custom 16:9 Resolutions
	Resolutions.push_back(std::make_pair<int, int>(1024, 576));
	Resolutions.push_back(std::make_pair<int, int>(1280, 720));
	Resolutions.push_back(std::make_pair<int, int>(1360, 768));
	Resolutions.push_back(std::make_pair<int, int>(1366, 768));
	Resolutions.push_back(std::make_pair<int, int>(1600, 900));
	Resolutions.push_back(std::make_pair<int, int>(1920, 1080));
	Resolutions.push_back(std::make_pair<int, int>(2560, 1440));
	Resolutions.push_back(std::make_pair<int, int>(3840, 2160));
	// Custom 16:10 Resolutions
	Resolutions.push_back(std::make_pair<int, int>(1280, 800));
	Resolutions.push_back(std::make_pair<int, int>(1440, 900));
	Resolutions.push_back(std::make_pair<int, int>(1680, 1050));
	Resolutions.push_back(std::make_pair<int, int>(1920, 1200));
	Resolutions.push_back(std::make_pair<int, int>(2560, 1600));

	wsprintf(Textxt[0], "Low");
	wsprintf(Textxt[1], "High");
	wsprintf(Textxt[2], "Auto");

	wsprintf(Ontxt[0], "Off");
	wsprintf(Ontxt[1], "On");

	wsprintf(Systxt[0], "Metric");
	wsprintf(Systxt[1], "US");


	OptText = 2;
	FOGENABLE = 1;
	SHADOWS3D = 1;
	OptDens = 1;
	OptSens = 1;
	OptAgres = 1;

	KeyMap.fkForward = 'W';
	KeyMap.fkBackward = 'S';
	KeyMap.fkSLeft = 'A';
	KeyMap.fkSRight = 'D';
	KeyMap.fkFire = VK_LBUTTON;
	KeyMap.fkShow = VK_RBUTTON;
	KeyMap.fkJump = VK_SPACE;
	KeyMap.fkCall = VK_MENU;
	KeyMap.fkBinoc = 'B';
	KeyMap.fkCrouch = 'X';
	KeyMap.fkRun = VK_SHIFT;
	KeyMap.fkUp = VK_UP;
	KeyMap.fkDown = VK_DOWN;
	KeyMap.fkLeft = VK_LEFT;
	KeyMap.fkRight = VK_RIGHT;
	WaitKey = -1;

}
