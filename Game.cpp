#include "Hunt.h"

#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>


extern std::vector<std::pair<int, int>> Resolutions; // Defined in 'Interface.cpp'


/*
Configure the `WinW` and `WinH` global variables to use the correct resolutions based on the chose option stored in `OptRes`
*/
void SetupRes()
{
	if (OptRes >= 0 && OptRes < Resolutions.size()) {
		WinW = Resolutions[OptRes].first;
		WinH = Resolutions[OptRes].second;
	}
	else {
		WinW = 800;
		WinH = 600;
		OptRes = 0;
		std::cout << "! ERROR SetupRes() !\n\t" << "Failed to set resolution, OptRes(" << OptRes << ") out of bounds." << std::endl;
	}
}


float GetLandOH(int x, int y)
{
	return (float)(-48 + HMapO[y][x]) * ctHScale;
}


float GetLandOUH(int x, int y)
{
	if (FMap[y][x] & fmReverse)
		return (float)((int)(HMap[y][x + 1] + HMap[y + 1][x]) / 2.f) * ctHScale;
	else
		return (float)((int)(HMap[y][x] + HMap[y + 1][x + 1]) / 2.f) * ctHScale;
}


float GetLandUpH(float x, float y)
{
	int CX = (int)x / 256;
	int CY = (int)y / 256;

	int dx = (int)x % 256;
	int dy = (int)y % 256;

	int h1 = HMap[CY][CX];
	int h2 = HMap[CY][CX + 1];
	int h3 = HMap[CY + 1][CX + 1];
	int h4 = HMap[CY + 1][CX];


	if (FMap[CY][CX] & fmReverse) {
		if (256 - dx > dy) h3 = h2 + h4 - h1;
		else h1 = h2 + h4 - h3;
	}
	else {
		if (dx > dy) h4 = h1 + h3 - h2;
		else h2 = h1 + h3 - h4;
	}

	float h = (float)
		(h1 * (256 - dx) + h2 * dx) * (256 - dy) +
		(h4 * (256 - dx) + h3 * dx) * dy;

	return  h / 256.f / 256.f * ctHScale;
}


float GetLandH(float x, float y)
{
	int CX = (int)x / 256;
	int CY = (int)y / 256;

	int dx = (int)x % 256;
	int dy = (int)y % 256;

	int h1 = HMap2[CY][CX];
	int h2 = HMap2[CY][CX + 1];
	int h3 = HMap2[CY + 1][CX + 1];
	int h4 = HMap2[CY + 1][CX];


	if (FMap[CY][CX] & fmReverse) {
		if (256 - dx > dy) h3 = h2 + h4 - h1;
		else h1 = h2 + h4 - h3;
	}
	else {
		if (dx > dy) h4 = h1 + h3 - h2;
		else h2 = h1 + h3 - h4;
	}

	float h = (float)
		(h1 * (256 - dx) + h2 * dx) * (256 - dy) +
		(h4 * (256 - dx) + h3 * dx) * dy;

	return  (h / 256.f / 256.f - 48) * ctHScale;
}


float GetLandQH(float CameraX, float CameraZ)
{
	float h, hh;

	h = GetLandH(CameraX, CameraZ);
	hh = GetLandH(CameraX - 90.f, CameraZ - 90.f); if (hh > h) h = hh;
	hh = GetLandH(CameraX + 90.f, CameraZ - 90.f); if (hh > h) h = hh;
	hh = GetLandH(CameraX - 90.f, CameraZ + 90.f); if (hh > h) h = hh;
	hh = GetLandH(CameraX + 90.f, CameraZ + 90.f); if (hh > h) h = hh;

	hh = GetLandH(CameraX + 128.f, CameraZ); if (hh > h) h = hh;
	hh = GetLandH(CameraX - 128.f, CameraZ); if (hh > h) h = hh;
	hh = GetLandH(CameraX, CameraZ + 128.f); if (hh > h) h = hh;
	hh = GetLandH(CameraX, CameraZ - 128.f); if (hh > h) h = hh;

	int ccx = (int)CameraX / 256;
	int ccz = (int)CameraZ / 256;

	for (int z = -2; z <= 2; z++)
		for (int x = -2; x <= 2; x++)
			if (OMap[ccz + z][ccx + x] != 255) {
				int ob = OMap[ccz + z][ccx + x];
				float CR = (float)MObjects[ob].info.Radius - 1.f;

				float oz = (ccz + z) * 256.f + 128.f;
				float ox = (ccx + x) * 256.f + 128.f;

				if (MObjects[ob].info.YHi + GetLandOH(ccx + x, ccz + z) < h) continue;
				if (MObjects[ob].info.YHi + GetLandOH(ccx + x, ccz + z) > PlayerY + 128) continue;
				if (MObjects[ob].info.YLo + GetLandOH(ccx + x, ccz + z) > PlayerY + 256) continue;
				float r = (float)sqrt((ox - CameraX) * (ox - CameraX) + (oz - CameraZ) * (oz - CameraZ));
				if (r < CR)
					h = MObjects[ob].info.YHi + GetLandOH(ccx + x, ccz + z);
			}
	return h;
}


float GetLandHObj(float CameraX, float CameraZ)
{
	float h;

	h = 0;

	int ccx = (int)CameraX / 256;
	int ccz = (int)CameraZ / 256;

	for (int z = -2; z <= 2; z++)
		for (int x = -2; x <= 2; x++)
			if (OMap[ccz + z][ccx + x] != 255) {
				int ob = OMap[ccz + z][ccx + x];
				float CR = (float)MObjects[ob].info.Radius - 1.f;

				float oz = (ccz + z) * 256.f + 128.f;
				float ox = (ccx + x) * 256.f + 128.f;

				if (MObjects[ob].info.YHi + GetLandOH(ccx + x, ccz + z) < h) continue;
				if (MObjects[ob].info.YLo + GetLandOH(ccx + x, ccz + z) > PlayerY + 256) continue;
				float r = (float)sqrt((ox - CameraX) * (ox - CameraX) + (oz - CameraZ) * (oz - CameraZ));
				if (r < CR)
					h = MObjects[ob].info.YHi + GetLandOH(ccx + x, ccz + z);
			}

	return h;
}


float GetLandQHNoObj(float CameraX, float CameraZ)
{
	float h, hh;

	h = GetLandH(CameraX, CameraZ);
	hh = GetLandH(CameraX - 90.f, CameraZ - 90.f); if (hh > h) h = hh;
	hh = GetLandH(CameraX + 90.f, CameraZ - 90.f); if (hh > h) h = hh;
	hh = GetLandH(CameraX - 90.f, CameraZ + 90.f); if (hh > h) h = hh;
	hh = GetLandH(CameraX + 90.f, CameraZ + 90.f); if (hh > h) h = hh;

	hh = GetLandH(CameraX + 128.f, CameraZ); if (hh > h) h = hh;
	hh = GetLandH(CameraX - 128.f, CameraZ); if (hh > h) h = hh;
	hh = GetLandH(CameraX, CameraZ + 128.f); if (hh > h) h = hh;
	hh = GetLandH(CameraX, CameraZ - 128.f); if (hh > h) h = hh;

	return h;
}


void ProcessCommandLine()
{
	std::string cmdLine = "";

	for (int a = 0; a < __argc; a++) {
		std::string s = __argv[a];
		if (s.find("x=") != std::string::npos) {
			PlayerX = (float)atof(&s[2]) * 256.f;
			LockLanding = true;
		}
		if (s.find("y=") != std::string::npos) {
			PlayerZ = (float)atof(&s[2]) * 256.f;
			LockLanding = true;
		}
		if (s.find("-nofullscreen") != std::string::npos) g_FullScreen = false;
		if (s.find("prj=") != std::string::npos) {
			ProjectName = s.substr(4);
			GameState = 1;
		}
	}
}


void AddExplosion(float x, float y, float z)
{
	Explosions[ExpCount].pos.x = x;
	Explosions[ExpCount].pos.y = y;
	Explosions[ExpCount].pos.z = z;
	Explosions[ExpCount].FTime = 0;
	ExpCount++;
}


void AddShipTask(int cindex)
{
	if (cindex >= Characters.size()) {
		return;
	}

	TCharacter* cptr = &Characters[cindex];

	bool TROPHYON = (GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) < 100) &&
		(!Tranq);

	if (TROPHYON) {
		ShipTask.clist[ShipTask.tcount] = cindex;
		ShipTask.tcount++;
		AddVoice(ShipModel.SoundFX[3].length,
			ShipModel.SoundFX[3].lpData);
	}

	//===== trophy =======//
	SYSTEMTIME st;
	GetLocalTime(&st);
	int t = 0;
	for (t = 0; t < 23; t++)
		if (!TrophyRoom.Body[t].ctype) break;

	float score = (float)DinoInfo[Characters[cindex].CType].BaseScore;

	if (TrophyRoom.Last.ssucces > 1)
		score *= (1.f + TrophyRoom.Last.ssucces / 10.f);

	if (Characters[cindex].CType != TargetDino + CTYPE_PARA) score /= 2.f;

	if (Tranq) score *= 1.25f;
	if (RadarMode) score *= 0.70f;
	if (ScentMode) score *= 0.80f;
	if (CamoMode) score *= 0.85f;
	TrophyRoom.Score += (int)score;


	if (TROPHYON) {
		TrophyTime = 20 * 1000;
		TrophyBody = t;
		TrophyRoom.Body[t].ctype = Characters[cindex].CType;
		TrophyRoom.Body[t].scale = Characters[cindex].scale;
		TrophyRoom.Body[t].weapon = TargetWeapon;
		TrophyRoom.Body[t].score = (int)score;
		TrophyRoom.Body[t].phase = (RealTime & 3);
		TrophyRoom.Body[t].time = (st.wHour << 10) + st.wMinute;
		TrophyRoom.Body[t].date = (st.wYear << 20) + (st.wMonth << 10) + st.wDay;
		TrophyRoom.Body[t].range = VectorLength(SubVectors(Characters[cindex].pos, PlayerPos)) / 128.f;
		std::cout << "Trophy added: " << DinoInfo[Characters[cindex].CType].Name << std::endl;
	}
}


void InitShip(int cindex)
{
	TCharacter* cptr = &Characters[cindex];

	Ship.DeltaY = 2048.f + DinoInfo[cptr->CType].ShDelta * cptr->scale;

	Ship.pos.x = PlayerX - 50 * 256;
	if (Ship.pos.x < 256) Ship.pos.x = PlayerX + 50 * 256;
	Ship.pos.z = PlayerZ - 50 * 256;
	if (Ship.pos.z < 256) Ship.pos.z = PlayerZ + 50 * 256;
	Ship.pos.y = GetLandUpH(Ship.pos.x, Ship.pos.z) + Ship.DeltaY;

	Ship.tgpos.x = cptr->pos.x;
	Ship.tgpos.z = cptr->pos.z;
	Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z) + Ship.DeltaY;
	Ship.State = 0;

	Ship.retpos = Ship.pos;
	Ship.cindex = cindex;
	Ship.FTime = 0;
}


void InitGameInfo()
{
	// TODO: move all of this to _RES loading
	WeapInfo[0].Name = "Semi Rifle";
	WeapInfo[0].Power = 1.0f;
	WeapInfo[0].Prec = 1.8f;
	WeapInfo[0].Loud = 0.6f;
	WeapInfo[0].Rate = 1.0f;
	WeapInfo[0].Shots = 6;
	
	/* C1 Shotgun
	WeapInfo[0].Name = "Shotgun";
	WeapInfo[0].Power = 1.5f;
	WeapInfo[0].Prec = 1.1f;
	WeapInfo[0].Loud = 0.3f;
	WeapInfo[0].Rate = 1.6f;
	WeapInfo[0].Shots = 6;
	*/

	WeapInfo[1].Name = "X-Bow";
	WeapInfo[1].Power = 1.1f;
	WeapInfo[1].Prec = 0.7f;
	WeapInfo[1].Loud = 1.9f;
	WeapInfo[1].Rate = 1.2f;
	WeapInfo[1].Shots = 8;

	WeapInfo[2].Name = "Sniper Rifle";
	WeapInfo[2].Power = 1.0f;
	WeapInfo[2].Prec = 1.8f;
	WeapInfo[2].Loud = 0.6f;
	WeapInfo[2].Rate = 1.0f;
	WeapInfo[2].Shots = 6;

	for (size_t c = 0; c < DinoInfo.size(); ++c) {
		DinoInfo[c].Scale0 = 800;
		DinoInfo[c].ScaleA = 600;
		DinoInfo[c].ShDelta = 0;
	}

	DinoInfo[CTYPE_MOSCH].Name = "Moschops";
	DinoInfo[CTYPE_MOSCH].Health0 = 2;
	DinoInfo[CTYPE_MOSCH].Mass = 0.15f;

	DinoInfo[CTYPE_GALLI].Name = "Gallimimus";
	DinoInfo[CTYPE_GALLI].Health0 = 2;
	DinoInfo[CTYPE_GALLI].Mass = 0.1f;

	DinoInfo[CTYPE_DIMOR].Name = "Dimorphodon";
	DinoInfo[CTYPE_DIMOR].Health0 = 1;
	DinoInfo[CTYPE_DIMOR].Mass = 0.05f;

	// RH Note: This is the Ship for some weird reason
	DinoInfo[CTYPE_DIMOR+1].Name = "";

	DinoInfo[CTYPE_PARA].Name = "Parasaurolophus";
	DinoInfo[CTYPE_PARA].Mass = 1.5f;
	DinoInfo[CTYPE_PARA].Length = 5.8f;
	DinoInfo[CTYPE_PARA].Radius = 320.f;
	DinoInfo[CTYPE_PARA].Health0 = 5;
	DinoInfo[CTYPE_PARA].BaseScore = 6;
	DinoInfo[CTYPE_PARA].SmellK = 0.8f;
	DinoInfo[CTYPE_PARA].HearK = 1.f;
	DinoInfo[CTYPE_PARA].LookK = 0.4f;
	DinoInfo[CTYPE_PARA].ShDelta = 48;

	DinoInfo[CTYPE_PACHY].Name = "Pachycephalosaurus";
	DinoInfo[CTYPE_PACHY].Mass = 0.8f;
	DinoInfo[CTYPE_PACHY].Length = 4.5f;
	DinoInfo[CTYPE_PACHY].Radius = 280.f;
	DinoInfo[CTYPE_PACHY].Health0 = 4;
	DinoInfo[CTYPE_PACHY].BaseScore = 8;
	DinoInfo[CTYPE_PACHY].SmellK = 0.4f;
	DinoInfo[CTYPE_PACHY].HearK = 0.8f;
	DinoInfo[CTYPE_PACHY].LookK = 0.6f;
	DinoInfo[CTYPE_PACHY].ShDelta = 36;

	DinoInfo[CTYPE_STEGO].Name = "Stegosaurus";
	DinoInfo[CTYPE_STEGO].Mass = 7.f;
	DinoInfo[CTYPE_STEGO].Length = 7.f;
	DinoInfo[CTYPE_STEGO].Radius = 480.f;
	DinoInfo[CTYPE_STEGO].Health0 = 5;
	DinoInfo[CTYPE_STEGO].BaseScore = 7;
	DinoInfo[CTYPE_STEGO].SmellK = 0.4f;
	DinoInfo[CTYPE_STEGO].HearK = 0.8f;
	DinoInfo[CTYPE_STEGO].LookK = 0.6f;
	DinoInfo[CTYPE_STEGO].ShDelta = 128;

	DinoInfo[CTYPE_ALLO].Name = "Allosaurus";
	DinoInfo[CTYPE_ALLO].Mass = 0.5;
	DinoInfo[CTYPE_ALLO].Length = 4.2f;
	DinoInfo[CTYPE_ALLO].Radius = 256.f;
	DinoInfo[CTYPE_ALLO].Health0 = 3;
	DinoInfo[CTYPE_ALLO].BaseScore = 12;
	DinoInfo[CTYPE_ALLO].Scale0 = 1000;
	DinoInfo[CTYPE_ALLO].ScaleA = 600;
	DinoInfo[CTYPE_ALLO].SmellK = 1.0f;
	DinoInfo[CTYPE_ALLO].HearK = 0.3f;
	DinoInfo[CTYPE_ALLO].LookK = 0.5f;
	DinoInfo[CTYPE_ALLO].ShDelta = 32;
	DinoInfo[CTYPE_ALLO].DangerCall = true;

	DinoInfo[CTYPE_TRIKE].Name = "Triceratops";
	DinoInfo[CTYPE_TRIKE].Mass = 3.f;
	DinoInfo[CTYPE_TRIKE].Length = 5.0f;
	DinoInfo[CTYPE_TRIKE].Radius = 512.f;
	DinoInfo[CTYPE_TRIKE].Health0 = 8;
	DinoInfo[CTYPE_TRIKE].BaseScore = 9;
	DinoInfo[CTYPE_TRIKE].SmellK = 0.6f;
	DinoInfo[CTYPE_TRIKE].HearK = 0.5f;
	DinoInfo[CTYPE_TRIKE].LookK = 0.4f;
	DinoInfo[CTYPE_TRIKE].ShDelta = 148;

	DinoInfo[CTYPE_VELOCI].Name = "Velociraptor";
	DinoInfo[CTYPE_VELOCI].Mass = 0.3f;
	DinoInfo[CTYPE_VELOCI].Length = 4.0f;
	DinoInfo[CTYPE_VELOCI].Radius = 256.f;
	DinoInfo[CTYPE_VELOCI].Health0 = 3;
	DinoInfo[CTYPE_VELOCI].BaseScore = 16;
	DinoInfo[CTYPE_VELOCI].ScaleA = 400;
	DinoInfo[CTYPE_VELOCI].SmellK = 1.0f;
	DinoInfo[CTYPE_VELOCI].HearK = 0.5f;
	DinoInfo[CTYPE_VELOCI].LookK = 0.4f;
	DinoInfo[CTYPE_VELOCI].ShDelta = -24;
	DinoInfo[CTYPE_VELOCI].DangerCall = true;

	DinoInfo[CTYPE_TREX].Name = "T-Rex";
	DinoInfo[CTYPE_TREX].Mass = 6.f;
	DinoInfo[CTYPE_TREX].Length = 12.f;
	DinoInfo[CTYPE_TREX].Radius = 400.f;
	DinoInfo[CTYPE_TREX].Health0 = 1024;
	DinoInfo[CTYPE_TREX].BaseScore = 20;
	DinoInfo[CTYPE_TREX].SmellK = 0.85f;
	DinoInfo[CTYPE_TREX].HearK = 0.8f;
	DinoInfo[CTYPE_TREX].LookK = 0.8f;
	DinoInfo[CTYPE_TREX].ShDelta = 168;
	DinoInfo[CTYPE_TREX].DangerCall = true;
}


CarnivoresEngine::CarnivoresEngine()
{
	m_Loop = true;
	g_FullScreen = true;
	WATERANI = true;
	NODARKBACK = true;
	LoDetailSky = true;
	CORRECTION = true;
	FOGON = true;
	FOGENABLE = true;
	Clouds = true;
	SKY = true;
	GOURAUD = true;
	MODELS = true;
	//TIMER        = true;
	BITMAPP = false;
	MIPMAP = true;
	NOCLIP = false;
	CLIP3D = true;

	DEBUG = false;
	SLOW = false;
	LOWRESTX = false;
	MORPHP = true;
	MORPHA = true;

	GameState = 0;

	RadarMode = false;

	fnt_BIG = CreateFont(
		23, 10, 0, 0,
		600, 0, 0, 0,
#ifdef __rus
		RUSSIAN_CHARSET,
#else
		ANSI_CHARSET,
#endif				
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);




	fnt_Small = CreateFont(
		14, 5, 0, 0,
		100, 0, 0, 0,
#ifdef __rus
		RUSSIAN_CHARSET,
#else
		ANSI_CHARSET,
#endif		        
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);


	fnt_Midd = CreateFont(
		16, 7, 0, 0,
		550, 0, 0, 0,
#ifdef __rus
		RUSSIAN_CHARSET,
#else
		ANSI_CHARSET,
#endif		        
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);

	WaterR = 10;
	WaterG = 38;
	WaterB = 46;
	WaterA = 10;
	TargetDino = 0;
	MessageList.timeleft = 0;

	InitGameInfo();

	InitMenu();

	// TODO : Replace with Audio
	InitAudioSystem(hwndMain);

	CreateVideoDIB();
	CreateFadeTab();
	CreateDivTable();
	InitClips();

	MenuState = -1;

	//    MenuState = 0;
	TrophyRoom.PlayerName = "";
	TrophyRoom.RegNumber = 0;
	LoadTrophy();

	PlayerX = (ctMapSize / 2) * 256;
	PlayerZ = (ctMapSize / 2) * 256;
	ProjectName = "hunt";

	ProcessCommandLine();

	ctViewR = 36;
	Soft_Persp_K = 1.5f;
	HeadY = 220;

	FogsList[0].fogRGB = 0x000000;
	FogsList[0].YBegin = 0;
	//  FogsList[0].YMax = 0;
	FogsList[0].Transp = 000;
	FogsList[0].FLimit = 000;

	FogsList[127].fogRGB = 0x604500;
	FogsList[127].Mortal = false;
	FogsList[127].Transp = 450;
	FogsList[127].FLimit = 160;

	std::memset(FogsMap, 0, sizeof(FogsMap));
	std::cout << "Init Engine: Ok." << std::endl;
}


CarnivoresEngine::~CarnivoresEngine()
{
	if (hdcMain) {
		ReleaseDC(hwndMain, hdcMain);
		hdcMain = nullptr;
	}

	ShutdownAudioSystem();

	ShowCursor(true);
}


void ProcessSyncro()
{
	RealTime = TimerGetMS();// timeGetTime();
	srand((unsigned)RealTime);
	if (SLOW) RealTime /= 4;
	TimeDt = RealTime - PrevTime;
	if (TimeDt < 0) TimeDt = 10;
	if (TimeDt > 10000) TimeDt = 10;
	if (TimeDt > 1000) TimeDt = 1000;
	PrevTime = RealTime;
	Takt++;
	if (!PAUSE)
		if (MyHealth) MyHealth += TimeDt * 4;
	if (MyHealth > MAX_HEALTH) MyHealth = MAX_HEALTH;
}


void MakeCall()
{
	if (ObservMode || TrophyMode) return;
	if (CallLockTime) return;

	CallLockTime = 1024 * 3;

	NextCall += (RealTime % 2) + 1;
	NextCall %= 3;

	AddVoice(fxCall[NextCall].length, fxCall[NextCall].lpData);

	float dmin = 200 * 256;
	int ai = -1;

	for (int c = 0; c < ChCount; c++) {
		TCharacter* cptr = &Characters[c];

		if (DinoInfo[TargetDino + CTYPE_PARA].DangerCall)
			if (cptr->CType < CTYPE_PARA) {
				cptr->State = 2;
				cptr->AfraidTime = (10 + rRand(5)) * 1024;
			}

		if (cptr->CType != CTYPE_PARA + TargetDino) continue;
		if (cptr->AfraidTime) continue;
		if (cptr->State) continue;

		float d = VectorLength(SubVectors(PlayerPos, cptr->pos));
		if (d < 98 * 256) {
			if (rRand(128) > 32)
				if (d < dmin) { dmin = d; ai = c; }
			cptr->tgx = PlayerX + siRand(1800);
			cptr->tgz = PlayerZ + siRand(1800);
		}
	}

	if (ai != -1) {
		answpos = SubVectors(Characters[ai].pos, PlayerPos);
		answpos.x /= -3.f; answpos.y /= -3.f; answpos.z /= -3.f;
		answpos = SubVectors(PlayerPos, answpos);
		answtime = 2000 + rRand(2000);
	}

}


void MakeShot(float ax, float ay, float az, float bx, float by, float bz)
{
	int sres;
	TrophyRoom.Last.smade++;
	if (TargetWeapon != 1)
		sres = TraceShot(ax, ay, az, bx, by, bz);
	else {
		Vector3d dl;
		float dy = 40;
		dl.x = (bx - ax) / 3;
		dl.y = (by - ay) / 3;
		dl.z = (bz - az) / 3;
		bx = ax + dl.x;
		by = ay + dl.y - dy / 2;
		bz = az + dl.z;
		sres = TraceShot(ax, ay, az, bx, by, bz);
		if (sres != -1) goto ENDTRACE;
		ax = bx; ay = by; az = bz;

		bx = ax + dl.x;
		by = ay + dl.y - dy * 3;
		bz = az + dl.z;
		sres = TraceShot(ax, ay, az, bx, by, bz);
		if (sres != -1) goto ENDTRACE;
		ax = bx; ay = by; az = bz;

		bx = ax + dl.x;
		by = ay + dl.y - dy * 5;
		bz = az + dl.z;
		sres = TraceShot(ax, ay, az, bx, by, bz);
		if (sres != -1) goto ENDTRACE;
		ax = bx; ay = by; az = bz;
	}

ENDTRACE:
	if (sres == -1) return;

	AddExplosion(bx, by, bz);

	int mort = (sres & 0xFF00) && (Characters[ShotDino].Health);
	sres &= 0xFF;

	if (sres != 3) return;
	if (!Characters[ShotDino].Health) return;

	if (mort) Characters[ShotDino].Health = 0;
	else Characters[ShotDino].Health--;


	if (sres == 3)
		if (!Characters[ShotDino].Health) {
			DemoPoint.pos = Characters[ShotDino].pos;
			DemoPoint.pos.y;
			if (Characters[ShotDino].CType >= CTYPE_PARA) {
				TrophyRoom.Last.ssucces++;
				AddShipTask(ShotDino);
			}

			if (Characters[ShotDino].CType < CTYPE_PARA)
				Characters_AddSecondaryOne(Characters[ShotDino].CType);

			DemoPoint.CIndex = ShotDino;
		}
		else {
			Characters[ShotDino].AfraidTime = 60 * 1000;
			if (Characters[ShotDino].CType != CTYPE_TREX || Characters[ShotDino].State == 0)
				Characters[ShotDino].State = 2;
		}

	if (Characters[ShotDino].CType == CTYPE_TREX)
		if (Characters[ShotDino].State)
			Characters[ShotDino].State = 5;
		else
			Characters[ShotDino].State = 1;
}


void RemoveCharacter(int index)
{
	if (index == -1) return;
	memcpy(&Characters[index], &Characters[index + 1], (255 - index) * sizeof(TCharacter));
	ChCount--;

	if (DemoPoint.CIndex > index) DemoPoint.CIndex--;

	for (int c = 0; c < ShipTask.tcount; c++)
		if (ShipTask.clist[c] > index) ShipTask.clist[c]--;
}


void AnimateShip()
{
	if (Ship.State == -1) {
		SetAmbient3d(0, 0, 0, 0, 0);
		if (!ShipTask.tcount) return;
		InitShip(ShipTask.clist[0]);
		memcpy(&ShipTask.clist[0], &ShipTask.clist[1], 250 * 4);
		ShipTask.tcount--;
		return;
	}


	SetAmbient3d(ShipModel.SoundFX[0].length,
		ShipModel.SoundFX[0].lpData,
		Ship.pos.x, Ship.pos.y, Ship.pos.z);

	int _TimeDt = TimeDt;

	//====== get up/down time acceleration ===========//
	if (Ship.FTime) {
		int am = ShipModel.Animation[0].AniTime;
		if (Ship.FTime < 500) _TimeDt = TimeDt * (Ship.FTime + 48) / 548;
		if (am - Ship.FTime < 500) _TimeDt = TimeDt * (am - Ship.FTime + 48) / 548;
		if (_TimeDt < 2) _TimeDt = 2;
	}


	Ship.tgalpha = FindVectorAlpha(Ship.tgpos.x - Ship.pos.x, Ship.tgpos.z - Ship.pos.z);
	float currspeed;
	float dalpha = AngleDifference(Ship.tgalpha, Ship.alpha);


	float L = VectorLength(SubVectors(Ship.tgpos, Ship.pos));
	Ship.pos.y += 0.3f * std::cosf(RealTime / 256.f);


	if (Ship.State)
		if (Ship.speed > 1)
			if (L < 4000)
				if (VectorLength(SubVectors(PlayerPos, Ship.pos)) < (ctViewR + 2) * 256) {
					Ship.tgpos.x += std::cosf(Ship.alpha) * 256 * 6.f;
					Ship.tgpos.z += std::sinf(Ship.alpha) * 256 * 6.f;
					Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z) + Ship.DeltaY;
				}


	//========= animate down ==========//
	if (Ship.State == 3) {
		Ship.FTime += _TimeDt;
		if (Ship.FTime >= ShipModel.Animation[0].AniTime) {
			Ship.FTime = ShipModel.Animation[0].AniTime - 1;
			Ship.State = 2;
			AddVoice(ShipModel.SoundFX[4].length,
				ShipModel.SoundFX[4].lpData);
			AddVoice3d(ShipModel.SoundFX[1].length, ShipModel.SoundFX[1].lpData,
				Ship.pos.x, Ship.pos.y, Ship.pos.z);
		}
		return;
	}


	//========= get body on board ==========//
	if (Ship.State) {
		if (Ship.cindex != -1) {
			DeltaFunc(Characters[Ship.cindex].pos.y, Ship.pos.y - 650 - (Ship.DeltaY - 2048), _TimeDt / 3.f);
			DeltaFunc(Characters[Ship.cindex].beta, 0, TimeDt / 4048.f);
			DeltaFunc(Characters[Ship.cindex].gamma, 0, TimeDt / 4048.f);
		}

		if (Ship.State == 2) {
			Ship.FTime -= _TimeDt;
			if (Ship.FTime < 0) Ship.FTime = 0;

			if (Ship.FTime == 0)
				if (std::fabs(Characters[Ship.cindex].pos.y - (Ship.pos.y - 650 - (Ship.DeltaY - 2048))) < 1.f) {
					Ship.State = 1;
					AddVoice(ShipModel.SoundFX[5].length,
						ShipModel.SoundFX[5].lpData);
					AddVoice3d(ShipModel.SoundFX[2].length, ShipModel.SoundFX[2].lpData,
						Ship.pos.x, Ship.pos.y, Ship.pos.z);
				}
			return;
		}
	}

	//====== speed ===============//  
	float vspeed = 1.f + L / 128.f;
	if (vspeed > 24) vspeed = 24;
	if (Ship.State) vspeed = 24;
	if (std::fabs(dalpha) > 0.4) vspeed = 0.f;
	float _s = Ship.speed;
	if (vspeed > Ship.speed) DeltaFunc(Ship.speed, vspeed, TimeDt / 200.f);
	else Ship.speed = vspeed;

	if (Ship.speed > 0 && _s == 0)
		AddVoice3d(ShipModel.SoundFX[2].length, ShipModel.SoundFX[2].lpData,
			Ship.pos.x, Ship.pos.y, Ship.pos.z);

	//====== fly ===========//
	float l = TimeDt * Ship.speed / 16.f;

	if (std::fabs(dalpha) < 0.4)
		if (l < L) {
			Ship.pos.x += std::cosf(Ship.alpha) * l;
			Ship.pos.z += std::sinf(Ship.alpha) * l;
		}
		else {
			if (Ship.State) {
				Ship.State = -1;
				RemoveCharacter(Ship.cindex);
				return;
			}
			else {
				Ship.pos = Ship.tgpos;
				Ship.State = 3;
				Ship.FTime = 1;
				Ship.tgpos = Ship.retpos;
				Characters[Ship.cindex].StateF = 0xFF;
				AddVoice3d(ShipModel.SoundFX[1].length, ShipModel.SoundFX[1].lpData,
					Ship.pos.x, Ship.pos.y, Ship.pos.z);
			}
		}

	//======= y movement ============//
	float h = GetLandUpH(Ship.pos.x, Ship.pos.z);
	DeltaFunc(Ship.pos.y, Ship.tgpos.y, TimeDt / 8.f);
	if (Ship.pos.y < h + 1024) Ship.pos.y = h + 1024;



	//======= rotation ============//
	if (Ship.tgalpha > Ship.alpha) currspeed = 0.1f + std::fabs(dalpha) / 2.f;
	else currspeed = -0.1f - std::fabs(dalpha) / 2.f;

	if (std::fabs(dalpha) > Math::pi) currspeed *= -1;

	DeltaFunc(Ship.rspeed, currspeed, (float)TimeDt / 420.f);

	float rspd = Ship.rspeed * TimeDt / 1024.f;
	if (std::fabs(dalpha) < std::fabs(rspd)) { Ship.alpha = Ship.tgalpha; Ship.rspeed /= 2; }
	else {
		Ship.alpha += rspd;
		if (Ship.State)
			if (Ship.cindex != -1)
				Characters[Ship.cindex].alpha += rspd;
	}

	if (Ship.alpha < 0) Ship.alpha += Math::pi * 2;
	if (Ship.alpha > Math::pi * 2) Ship.alpha -= Math::pi * 2;
	//======== move body ===========//
	if (Ship.State) {
		if (Ship.cindex != -1) {
			Characters[Ship.cindex].pos.x = Ship.pos.x;
			Characters[Ship.cindex].pos.z = Ship.pos.z;
		}
		if (L > 1000) Ship.tgpos.y += TimeDt / 12.f;
	}
	else {
		Ship.tgpos.x = Characters[Ship.cindex].pos.x;
		Ship.tgpos.z = Characters[Ship.cindex].pos.z;
		Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z) + Ship.DeltaY;
	}
}


void ProcessTrophy()
{
	TrophyBody = -1;

	for (int c = 0; c < ChCount; c++) {
		Vector3d p = Characters[c].pos;
		p.x += Characters[c].lookx * 256 * 2.5f;
		p.z += Characters[c].lookz * 256 * 2.5f;

		if (VectorLength(SubVectors(p, PlayerPos)) < 148)
			TrophyBody = c;
	}

	if (TrophyBody == -1) return;

	TrophyBody = Characters[TrophyBody].State;
}


void AnimateProcesses()
{
	Wind.speed += siRand(1600) / 6400.f;
	if (Wind.speed < 0.f) Wind.speed = 0.f;
	if (Wind.speed > 20.f) Wind.speed = 20.f;

	Wind.nv.x = std::sinf(Wind.alpha);
	Wind.nv.z =-std::cosf(Wind.alpha);
	Wind.nv.y = 0.f;


	if (answtime) {
		answtime -= TimeDt;
		if (answtime <= 0) {
			answtime = 0;
			int r = rRand(128) % 3;
			AddVoice3d(fxCall[r].length, fxCall[r].lpData,
				answpos.x, answpos.y, answpos.z);
		}
	}



	if (CallLockTime) {
		CallLockTime -= TimeDt;
		if (CallLockTime < 0) CallLockTime = 0;
	}

	CheckAfraid();
	AnimateShip();
	if (TrophyMode)
		ProcessTrophy();

	for (int e = 0; e < ExpCount; e++) {
		Explosions[e].FTime += TimeDt;
		if (Explosions[e].FTime >= ExplodInfo.Animation[0].AniTime) {
			memcpy(&Explosions[e], &Explosions[e + 1], sizeof(TExplosion) * (ExpCount + 1 - e));
			e--;
			ExpCount--;
		}
	}

	if (ExitTime) {
		ExitTime -= TimeDt;
		if (ExitTime <= 0) {
			TrophyRoom.Total.time += TrophyRoom.Last.time;
			TrophyRoom.Total.smade += TrophyRoom.Last.smade;
			TrophyRoom.Total.ssucces += TrophyRoom.Last.ssucces;
			TrophyRoom.Total.path += TrophyRoom.Last.path;
			if (!ShotsLeft)
				if (!TrophyRoom.Last.ssucces) {
					TrophyRoom.Score--;
					if (TrophyRoom.Score < 0) TrophyRoom.Score = 0;
				}

			if (MyHealth) SaveTrophy();
			else LoadTrophy();
			GameState = 0;
		}
	}
}


void RemoveCurrentTrophy()
{
	int p = 0;
	int& ctype = TrophyRoom.Body[TrophyBody].ctype;

	if (!TrophyMode || !ctype) return;

	std::cout << "Trophy removed: " << DinoInfo[ctype].Name << std::endl;

	for (int c = 0; c < TrophyBody; c++) {
		if (ctype)
			p++;
	}

	ctype = 0;

	if (TrophyMode) {
		memcpy(&Characters[p], &Characters[p + 1], (250 - p) * sizeof(TCharacter));
		ChCount--;
	}

	TrophyTime = 0;
	TrophyBody = -1;
}


void LoadTrophy()
{
	int pr = TrophyRoom.RegNumber;
	TrophyRoom.reset();
	TrophyRoom.RegNumber = pr;

	std::stringstream fname;
	int rn = TrophyRoom.RegNumber;
	fname << "trophy" << std::setfill('0') << std::setw(2) << TrophyRoom.RegNumber << ".sav";

	std::cout << "LoadTrophy( '" << fname.str() << "' )" << std::endl;

	std::ifstream file(fname.str(), std::ios::binary);
	if (!file.is_open()) {
		std::cout << "===> Error loading trophy!" << std::endl;
		return;
	}

	char player_name[128];
	std::memset(player_name, 0, 128);

	file.read((char*)player_name, 128);
	TrophyRoom.PlayerName = player_name;

	//file.read((char*)&TrophyRoom.RegNumber, sizeof(int));
	//file.read((char*)&TrophyRoom.Score, sizeof(int));
	//file.read((char*)&TrophyRoom.Rank, sizeof(int));
	////file.read((char*)&TrophyRoom, sizeof(TrophyRoom));
	file.read((char*)&TrophyRoom.RegNumber, 44 + (sizeof(TTrophyItem) * 24));

	file.read((char*)&OptAgres, 4);
	file.read((char*)&OptDens, 4);
	file.read((char*)&OptSens, 4);

	file.read((char*)&OptRes, 4); std::cout << "\t OptRes = " << OptRes << std::endl;
	file.read((char*)&FOGENABLE, 4);
	file.read((char*)&OptText, 4);
	file.read((char*)&SHADOWS3D, 4);

	file.read((char*)&KeyMap, sizeof(KeyMap));
	file.read((char*)&REVERSEMS, 4);

	file.read((char*)&ScentMode, 4);
	file.read((char*)&CamoMode, 4);
	file.read((char*)&RadarMode, 4);
	file.read((char*)&Tranq, 4);

	file.read((char*)&OptSys, 4);

	SetupRes();

	file.clear();
	OPT_ALPHA_COLORKEY = TrophyRoom.Body[0].r4;
	TrophyRoom.RegNumber = rn;
	std::cout << "Trophy Loaded." << std::endl;
	//	TrophyRoom.Score = 299;
}


void SaveTrophy()
{
	std::stringstream fname;
	fname << "trophy" << std::setfill('0') << std::setw(2) << TrophyRoom.RegNumber << ".sav";

	int r = TrophyRoom.Rank;
	TrophyRoom.Rank = 0;
	if (TrophyRoom.Score >= 100) TrophyRoom.Rank = RANK_ADVANCED;
	if (TrophyRoom.Score >= 300) TrophyRoom.Rank = RANK_MASTER;

	if (r != TrophyRoom.Rank) {
		if (TrophyRoom.Rank == RANK_ADVANCED) MenuState = 112;
		if (TrophyRoom.Rank == RANK_MASTER) MenuState = 113;
	}

	TrophyRoom.Body[0].r4 = OPT_ALPHA_COLORKEY;

	std::ofstream file(fname.str(), std::ios::binary);
	if (!file.is_open()) {
		std::cout << "==>> Error saving trophy!" << std::endl;
		return;
	}

	size_t sz = TrophyRoom.PlayerName.size();
	if (sz >= 127) sz = 126;
	char pname[128];
	std::memset(pname, 0, 128);
	std::memcpy(pname, TrophyRoom.PlayerName.data(), sz);
	file.write(pname, 128);

	file.write((char*)&TrophyRoom.RegNumber, 44 + (sizeof(TTrophyItem) * 24));

	file.write((char*)&OptAgres, 4);
	file.write((char*)&OptDens, 4);
	file.write((char*)&OptSens, 4);

	file.write((char*)&OptRes, 4);
	file.write((char*)&FOGENABLE, 4);
	file.write((char*)&OptText, 4);
	file.write((char*)&SHADOWS3D, 4);

	file.write((char*)&KeyMap, sizeof(KeyMap));
	file.write((char*)&REVERSEMS, 4);

	file.write((char*)&ScentMode, 4);
	file.write((char*)&CamoMode, 4);
	file.write((char*)&RadarMode, 4);
	file.write((char*)&Tranq, 4);

	file.write((char*)&OptSys, 4);

	file.close();
	std::cout << "Trophy Saved." << std::endl;
}


void LoadPlayersInfo()
{
	for (int p = 0; p < 16; p++) {
		PlayerR[p].reset();

		std::stringstream fname("");
		fname << "trophy" << p << ".sav";

		std::ifstream file(fname.str(), std::ios::binary);

		if (file.is_open()) {
			char player_name[128];
			std::memset(player_name, 0, 128);

			file.read((char*)player_name, 128);
			file.read((char*)&PlayerR[p].RegNumber, sizeof(int));
			file.read((char*)&PlayerR[p].Score, sizeof(int));
			file.read((char*)&PlayerR[p].Rank, sizeof(int));

			PlayerR[p].PName = player_name;
		}

		file.close();
	}
}