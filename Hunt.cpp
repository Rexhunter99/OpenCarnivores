#define _MAIN_
#include "Hunt.h"

#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>


std::chrono::steady_clock::duration g_TimeStart;

float rav = 0;
float rbv = 0;
/* Tracks the need to reset video mode */
bool NeedRVM;
float BinocularPower = 2.5f;

void ShowMenuVideo();
void HideWeapon();

char cheatcode[16] = "DEBUGON";
int  cheati = 0;

class TModelListItem {
public:
	TModel* mptr;
	float x0, y0, z0;
	int light;
	float al, bt;
	int gl, ts;
	Vector3d waterbase;
};

TModelListItem rmlistA[128], rmlistB[128];
int rmlistselector, rmcountA, rmcountB;


class TCharListItem {
public:
	int CType, Index;
};


class TCharListLine {
public:
	int ICount;
	TCharListItem Items[256];
};

TCharListLine ChRenderList[64];



/*
Replacement for timeGetTime() that uses the std::chrono library
*/
int64_t TimerGetMS()
{
	auto t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch() - g_TimeStart);
	return t.count();
}

void ResetMousePos()
{
	if (g_FullScreen)
		SetCursorPos(VideoCX, VideoCY);
}

void InsertModelList(TModel* mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	if (rmlistselector) {
		rmlistA[rmcountA].mptr = mptr;
		rmlistA[rmcountA].x0 = x0;
		rmlistA[rmcountA].y0 = y0;
		rmlistA[rmcountA].z0 = z0;
		rmlistA[rmcountA].light = light;
		rmlistA[rmcountA].al = al;
		rmlistA[rmcountA].bt = bt;
		rmlistA[rmcountA].gl = GlassL;
		rmlistA[rmcountA].ts = ts;
		if (ts & 0x8000) rmlistA[rmcountA].waterbase = v[1];
		rmcountA++;
	}
	else {
		rmlistB[rmcountB].mptr = mptr;
		rmlistB[rmcountB].x0 = x0;
		rmlistB[rmcountB].y0 = y0;
		rmlistB[rmcountB].z0 = z0;
		rmlistB[rmcountB].light = light;
		rmlistB[rmcountB].al = al;
		rmlistB[rmcountB].bt = bt;
		rmlistB[rmcountB].gl = GlassL;
		rmlistB[rmcountB].ts = ts;
		if (ts & 0x8000) rmlistB[rmcountB].waterbase = v[1];
		rmcountB++;
	}
}


void RenderMList()
{
	rmlistselector = 1 - rmlistselector;

	if (rmlistselector) {
		for (int rr = 0; rr < rmcountA; rr++) {
			GlassL = rmlistA[rr].gl;
			ts = rmlistA[rr].ts;
			waterclip = (ts & 0x8000);

			if (waterclip)
				waterclipbase = rmlistA[rr].waterbase;
			ts &= 0xFFF;

			int d = (int)fabs(rmlistA[rr].z0);
			if (CLIP3D && (d < BackViewRR))
				RenderModelClip(rmlistA[rr].mptr, rmlistA[rr].x0, rmlistA[rr].y0, rmlistA[rr].z0, rmlistA[rr].light, rmlistA[rr].al, rmlistA[rr].bt);
			else
				if (waterclip)
					RenderModelClipWater(rmlistA[rr].mptr, rmlistA[rr].x0, rmlistA[rr].y0, rmlistA[rr].z0, rmlistA[rr].light, rmlistA[rr].al, rmlistA[rr].bt);
				else
					RenderModel(rmlistA[rr].mptr, rmlistA[rr].x0, rmlistA[rr].y0, rmlistA[rr].z0, rmlistA[rr].light, rmlistA[rr].al, rmlistA[rr].bt);
		}
		rmcountA = 0;
	}
	else {
		for (int rr = 0; rr < rmcountB; rr++) {
			GlassL = rmlistB[rr].gl;
			ts = rmlistB[rr].ts;
			waterclip = (ts & 0x8000);

			if (waterclip)
				waterclipbase = rmlistB[rr].waterbase;
			ts &= 0xFFF;

			int d = (int)fabs(rmlistB[rr].z0);
			if (CLIP3D && (d < BackViewRR))
				RenderModelClip(rmlistB[rr].mptr, rmlistB[rr].x0, rmlistB[rr].y0, rmlistB[rr].z0, rmlistB[rr].light, rmlistB[rr].al, rmlistB[rr].bt);
			else
				if (waterclip)
					RenderModelClipWater(rmlistB[rr].mptr, rmlistB[rr].x0, rmlistB[rr].y0, rmlistB[rr].z0, rmlistB[rr].light, rmlistB[rr].al, rmlistB[rr].bt);
				else
					RenderModel(rmlistB[rr].mptr, rmlistB[rr].x0, rmlistB[rr].y0, rmlistB[rr].z0, rmlistB[rr].light, rmlistB[rr].al, rmlistB[rr].bt);
		}
		rmcountB = 0;
	}

}


float CalcFogLevel(Vector3d v)
{
	if (!FOGON)
		return 0.f;

	bool vinfog = true;
	int cf = FogsMap[((int)(v.z + CameraZ)) >> 9][((int)(v.x + CameraX)) >> 9];
	
	if ((!cf) && CAMERAINFOG) {
		cf = CameraFogI;
		vinfog = true;
	}

	if (UNDERWATER) {
		cf = 127;
	}

	if (!(CAMERAINFOG && cf)) return 0;
	TFogEntity* fptr;
	fptr = &FogsList[cf];
	CurFogColor = fptr->fogRGB;

	float d = VectorLength(v);

	v.y += CameraY;

	float fla = -(v.y - fptr->YBegin * ctHScale) / ctHScale;
	if (!vinfog && fla > 0) {
		fla = 0;
	}

	float flb = -(CameraY - fptr->YBegin * ctHScale) / ctHScale;
	
	if (!CAMERAINFOG && flb > 0) {
		flb = 0;
	}

	if (fla < 0 && flb < 0) {
		return 0;
	}

	if (fla < 0) { d *= flb / (flb - fla); fla = 0; }
	if (flb < 0) { d *= fla / (fla - flb); flb = 0; }

	float fl = (fla + flb);

	fl *= (d + (fptr->Transp / 2)) / fptr->Transp;

	return min(fl, fptr->FLimit);
}


void PreCashGroundModel()
{
	SKYDTime = RealTime >> 1;
	int x, y;
	int kx = SKYDTime & 255;
	int ky = SKYDTime & 255;
	int SKYDT = SKYDTime >> 8;
	BOOL FogFound = FALSE;

	MapMinY = 10241024;

	for (y = -(ctViewR + 2); y < (ctViewR + 2); y++)
		for (x = -(ctViewR + 2); x < (ctViewR + 2); x++) {
			int xx = (CCX + x) & 511;
			int yy = (CCY + y) & 511;

			v[0].x = xx * 256 - CameraX;
			v[0].z = yy * 256 - CameraZ;
			if (UNDERWATER)  v[0].y = (float)((int)HMap2[yy][xx] - 48) * ctHScale - CameraY;
			else v[0].y = (float)((int)HMap[yy][xx]) * ctHScale - CameraY;

			bool wtr = false;
			float wdelta;

			if (WATERANI && !UNDERWATER) wtr = (FMap[yy][xx] & fmWater);

			if (wtr) {
				v[0].x += std::sinf(xx + yy + SKYDTime / 124.f) * 16.f;
				v[0].z += std::sinf(Math::pi / 2.f + xx + yy + SKYDTime / 124.f) * 16.f;
			}


			if (HARD3D) {
				VMap[64 + y][64 + x].Fog = CalcFogLevel(v[0]);
			}

			v[0] = RotateVector(v[0]);

			if (v[0].z < 1024)
				if (FOGENABLE)
					if (FogsMap[yy >> 1][xx >> 1]) FogFound = TRUE;

			if (v[0].z < -ctViewR * 128) {
				if (UNDERWATER)
					if (HMap2[yy][xx] - 48 < MapMinY)
						MapMinY = HMap2[yy][xx] - 48;
					else if (HMap[yy][xx] < MapMinY)
						MapMinY = HMap[yy][xx];
			}


			VMap[64 + y][64 + x].v = v[0];

			int  DF = 0;
			int  db = 0;

			if (v[0].z < 256) {
				if (Clouds) {
					int shmx = (xx + SKYDT) & 127;
					int shmy = (yy + SKYDT) & 127;

					int db1 = SkyMap[shmy * 128 + shmx];
					int db2 = SkyMap[shmy * 128 + ((shmx + 1) & 127)];
					int db3 = SkyMap[((shmy + 1) & 127) * 128 + shmx];
					int db4 = SkyMap[((shmy + 1) & 127) * 128 + ((shmx + 1) & 127)];
					db = (db1 * (256 - kx) + db2 * kx) * (256 - ky) +
						(db3 * (256 - kx) + db4 * kx) * ky;
					db >>= 19;
					db = db - 10;
					if (db < 0) db = 0;
					if (db > 12) db = 12;
				}

				int clt = 6;
				if (GOURAUD) clt = min(62, (LMap[yy][xx] + db));


				if (wtr | UNDERWATER) {
					if (UNDERWATER) {
						wdelta = (float)sin(-Math::pi / 2 + RandomMap[yy & 31][xx & 31] / 512.f + RealTime / 400.f);
						clt += (int)(6 + wdelta * 8.f);
					}
					else {
						wdelta = (float)sin(-Math::pi / 2 + RandomMap[yy & 31][xx & 31] / 512.f + RealTime / (400.f + RandomMap[yy & 31][xx & 31] / 512.f)) * 6;
						clt += (int)(wdelta);
					}

					if (clt < 6) clt = 6;
					if (clt > 44) clt = 44;
				}

				VMap[64 + y][64 + x].Light = clt;
			}

			if (v[0].z > -256.0) DF += 128; else {

				VMap[64 + y][64 + x].scrx = VideoCX - (int)(v[0].x / v[0].z * CameraW);
				VMap[64 + y][64 + x].scry = VideoCY + (int)(v[0].y / v[0].z * CameraH);

				if (VMap[64 + y][64 + x].scrx < 0)     DF += 1;
				if (VMap[64 + y][64 + x].scrx > WinEX) DF += 2;
				if (VMap[64 + y][64 + x].scry < 0)     DF += 4;
				if (VMap[64 + y][64 + x].scry > WinEY) DF += 8;
			}

			VMap[64 + y][64 + x].DFlags = DF;
		}

	FOGON = FogFound || UNDERWATER;
}


void PreCashWaterModel()
{
	int x, y;
	for (y = -(ctViewR + 2); y < (ctViewR + 2); y++)
		for (x = -(ctViewR + 2); x < (ctViewR + 2); x++) {
			int xx = (CCX + x) & 511;
			int yy = (CCY + y) & 511;

			if (HMap2[yy][xx] == HMap[yy][xx] + 48) {
				VMap2[64 + y][64 + x].DFlags = 0xFFFF;
				continue;
			}

			v[0].x = xx * 256 - CameraX;
			v[0].z = yy * 256 - CameraZ;
			v[0].y = (float)((int)HMap[yy][xx]) * ctHScale - CameraY;


			BOOL wtr = FALSE;

			if (WATERANI) wtr = (FMap[yy][xx] & fmWater);

			if (wtr) {
				//float wdelta = (float)sin(xx*2-yy + RealTime/256.f);              
				//wdelta = (float)sin(-pi/2 + RandomMap[yy & 31][xx & 31]/512.f + RealTime/400.f) * 16;
				//v[0].y+=(float) wdelta; 
				v[0].x += std::sinf(xx + yy + RealTime / 256.f) * 20.f;
				v[0].z += std::sinf(Math::pi / 2.f + xx + yy + RealTime / 256.f) * 20.f;
			}

			v[0] = RotateVector(v[0]);
			VMap2[64 + y][64 + x].v = v[0];

			int  DF = 0;

			VMap2[64 + y][64 + x].Light = 0;

			if (v[0].z > -256.0) DF += 128; else {
				VMap2[64 + y][64 + x].scrx = VideoCX + (int)(v[0].x / (-v[0].z) * CameraW);
				VMap2[64 + y][64 + x].scry = VideoCY - (int)(v[0].y / (-v[0].z) * CameraH);

				if (VMap2[64 + y][64 + x].scrx < 0)     DF += 1;
				if (VMap2[64 + y][64 + x].scrx > WinEX) DF += 2;
				if (VMap2[64 + y][64 + x].scry < 0)     DF += 4;
				if (VMap2[64 + y][64 + x].scry > WinEY) DF += 8;
			}

			VMap2[64 + y][64 + x].DFlags = DF;
		}
}


void AddShadowCircle(int x, int y, int R, int D)
{
	if (UNDERWATER) return;

	int cx = x / 256;
	int cy = y / 256;
	int cr = 1 + R / 256;
	for (int yy = -cr; yy <= cr; yy++)
		for (int xx = -cr; xx <= cr; xx++) {
			int tx = (cx + xx) * 256;
			int ty = (cy + yy) * 256;
			int r = (int)sqrt((tx - x) * (tx - x) + (ty - y) * (ty - y));
			if (r > R) continue;
			VMap[cy + yy - CCY + 64][cx + xx - CCX + 64].Light += D * (R - r) / R;
			if (VMap[cy + yy - CCY + 64][cx + xx - CCX + 64].Light > 62)
				VMap[cy + yy - CCY + 64][cx + xx - CCX + 64].Light = 62;
		}
}


void CreateChRenderList()
{
	//=========== ship ================//   
	Ship.rpos.x = Ship.pos.x - CameraX;
	Ship.rpos.y = Ship.pos.y - CameraY;
	Ship.rpos.z = Ship.pos.z - CameraZ;
	float r = (float)max(fabs(Ship.rpos.x), fabs(Ship.rpos.z));
	int ri = -1 + (int)(r / 256.f + 1.6f);

	if (Ship.State != -1)
		if (ri < ctViewR - 6) {
			int h = (int)((Ship.pos.y - GetLandUpH(Ship.pos.x, Ship.pos.z)) / 1.8);
			AddShadowCircle((int)Ship.pos.x + h, (int)Ship.pos.z + h, 1200, 24);
		}



	if (HARD3D) return;
	for (int c = 0; c <= ctViewR; c++)
		ChRenderList[c].ICount = 0;


	//=========== ship ================//   

	if (Ship.State == -1)
		goto NOSHIP;
	if (ri < 0)
		ri = 0;

	if (ri < ctViewR) {
		Ship.rpos = RotateVector(Ship.rpos);
		if (Ship.rpos.z > BackViewR) goto NOSHIP;
		if (std::fabs(Ship.rpos.x) > -Ship.rpos.z + BackViewR) goto NOSHIP;

		int i = ChRenderList[ri].ICount++;
		ChRenderList[ri].Items[i].CType = CTYPE_DIMOR+1; // RH: WHAT!? The Ship has a Creature Type?
	}
NOSHIP:;


	//============= Dinosaurs ====================//
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

		cptr->rpos = RotateVector(cptr->rpos);

		float br = BackViewR + DinoInfo[cptr->CType].Radius;
		if (cptr->rpos.z > br) continue;
		if (fabs(cptr->rpos.x) > -cptr->rpos.z + br) continue;
		if (fabs(cptr->rpos.y) > -cptr->rpos.z + br) continue;

		/*
			  if (cptr->rpos.z > BackViewR + ) continue;
			  if ( fabs(cptr->rpos.x) > -cptr->rpos.z + BackViewR ) continue;
		*/
		AddShadowCircle((int)cptr->pos.x + 100, (int)cptr->pos.z + 100, 360, 16);

		int i = ChRenderList[ri].ICount++;
		ChRenderList[ri].Items[i].CType = CTYPE_MOSCH;
		ChRenderList[ri].Items[i].Index = c;
	}

	//============= Explosions =================//
	TExplosion* eptr;
	for (int c = 0; c < ExpCount; c++) {

		eptr = &Explosions[c];
		eptr->rpos.x = eptr->pos.x - CameraX;
		eptr->rpos.y = eptr->pos.y - CameraY;
		eptr->rpos.z = eptr->pos.z - CameraZ;


		float r = (float)max(fabs(eptr->rpos.x), fabs(eptr->rpos.z));
		int ri = -1 + (int)(r / 256.f + 0.2f);
		if (ri < 0) ri = 0;
		if (ri > ctViewR) continue;

		eptr->rpos = RotateVector(eptr->rpos);

		if (eptr->rpos.z > BackViewR) continue;
		if (fabs(eptr->rpos.x) > -eptr->rpos.z + BackViewR) continue;

		int i = ChRenderList[ri].ICount++;
		ChRenderList[ri].Items[i].CType = CTYPE_DIMOR;
		ChRenderList[ri].Items[i].Index = c;
	}



}


void RenderChList(int r)
{
	if (HARD3D) return;
	for (int c = 0; c < ChRenderList[r].ICount; c++) {
		if (ChRenderList[r].Items[c].CType == CTYPE_MOSCH) {
			RenderCharacter(ChRenderList[r].Items[c].Index);
		}
		else if (ChRenderList[r].Items[c].CType == CTYPE_DIMOR) {
			RenderExplosion(ChRenderList[r].Items[c].Index);
		}
		else if (ChRenderList[r].Items[c].CType == CTYPE_DIMOR+1) {
			RenderShip();
		}
	}
}


void DrawScene()
{
	dFacesCount = 0;

	ca = std::cosf(CameraAlpha);
	sa = std::sinf(CameraAlpha);

	cb = std::cosf(CameraBeta);
	sb = std::sinf(CameraBeta);

	ClipW.nv.x = 0;
	ClipW.nv.y = cb;
	ClipW.nv.z = sb;

	CCX = (int)CameraX / 256;
	CCY = (int)CameraZ / 256;

	if (UNDERWATER) {
		PreCashWaterModel();
	}
	
	PreCashGroundModel();

	CreateChRenderList();

	if (NeedWater && WATERANI && !HARD3D)
		ScrollWater();

	NeedWater = false;

	if (SKY) {
		RenderSkyPlane();
	}
	else {
		ClearVideoBuf();
	}

	cb = std::cosf(CameraBeta);
	sb = std::sinf(CameraBeta);

	rmlistselector = 0;
	rmcountA = 0;
	rmcountB = 0;

	for (r = ctViewR; r > 0; r--) {

		for (int x = r; x > 0; x--) {
			ProcessMap(CCX - x, CCY + r, r);
			ProcessMap(CCX + x, CCY + r, r);
			ProcessMap(CCX - x, CCY - r, r);
			ProcessMap(CCX + x, CCY - r, r);
		}

		ProcessMap(CCX, CCY - r, r);
		ProcessMap(CCX, CCY + r, r);

		for (int y = r - 1; y > 0; y--) {
			ProcessMap(CCX + r, CCY - y, r);
			ProcessMap(CCX + r, CCY + y, r);
			ProcessMap(CCX - r, CCY + y, r);
			ProcessMap(CCX - r, CCY - y, r);
		}
		ProcessMap(CCX - r, CCY, r);
		ProcessMap(CCX + r, CCY, r);

		RenderMList();
		RenderChList(r);
	}

	ProcessMap(CCX, CCY, r);
	RenderMList();
	RenderMList();
	RenderChList(0);
	Render3DHardwarePosts();
}


void DrawOpticCross(int v)
{

	int sx = VideoCX + (int)(rVertex[v].x / (-rVertex[v].z) * CameraW);
	int sy = VideoCY - (int)(rVertex[v].y / (-rVertex[v].z) * CameraH);

	if ((fabs(VideoCX - sx) > WinW / 2) ||
		(fabs(VideoCY - sy) > WinH / 4)) return;


	Render_Cross(sx, sy);
}


void ScanLifeForms()
{
	int li = -1;
	float dm = (float)(ctViewR + 2) * 256;
	for (int c = 0; c < ChCount; c++) {
		TCharacter* cptr = &Characters[c];
		if (!cptr->Health) continue;
		if (cptr->rpos.z > -512) continue;
		float d = (float)sqrt(cptr->rpos.x * cptr->rpos.x + cptr->rpos.y * cptr->rpos.y + cptr->rpos.z * cptr->rpos.z);
		if (d > ctViewR * 256) continue;
		float r = (float)(fabs(cptr->rpos.x) + fabs(cptr->rpos.y)) / d;
		if (r > 0.15) continue;
		if (d < dm)
			if (!TraceLook(cptr->pos.x, cptr->pos.y + 220, cptr->pos.z,
				PlayerX, PlayerY + HeadY, PlayerZ)) {

				dm = d;
				li = c;
			}

	}

	if (li == -1) return;
	Render_LifeInfo(li);
}

/**
Render the weapon and HUD elements
**/
void DrawPostObjects()
{
	float b;
	TWeapon* wptr = &Weapon;

	Hardware_ZBuffer(FALSE);

	if (DemoPoint.DemoTime) goto SKIPWEAPON;
	/*
	  if (BINMODE || OPTICMODE) {
	   CameraW = (float)VideoCX*1.25f;
	   CameraH = CameraW;
	  }
	*/
	if (BINMODE) {
		RenderNearModel(&Binocular, 0, 0, 2 * (216 - 72 * BinocularPower), 0, 0, 0);
		ScanLifeForms();
		MapMode = FALSE;
	}


	if (BINMODE || OPTICMODE) goto SKIPWIND;

	if (!TrophyMode)
		if (!(KeyboardState[VK_CAPITAL] & 1)) {
			BOOL lr = LOWRESTX;
			LOWRESTX = TRUE;
			VideoCX = WinW / 5;
			VideoCY = WinH - (WinH / 3);
			VideoCY = WinH - (WinH * 10 / 23);
			CreateMorphedModel(&WindModel.Model, &WindModel.Animation[0], (int)(Wind.speed * 50.f));
			RenderNearModel(&WindModel.Model, -10, -37, -96, 0, CameraAlpha - Wind.alpha, 0);

			VideoCX = WinW - (WinW / 5);
			VideoCY = WinH - (WinH * 10 / 23);
			RenderNearModel(&CompasModel, +8, -38, -96, 0, CameraAlpha, 0);

			VideoCX = WinW / 2;
			VideoCY = WinH / 2;
			LOWRESTX = lr;
		}

SKIPWIND:


	if (wptr->state == 0) goto SKIPWEAPON;

	MapMode = FALSE;

	wptr->shakel += TimeDt / 10000.f;
	if (wptr->shakel > 4.0f) wptr->shakel = 4.0f;
	//if (DEBUG) wptr->shakel = 0.0f;

	if (wptr->state == 1) {
		wptr->FTime += TimeDt;
		if (wptr->FTime >= wptr->chinfo.Animation[0].AniTime) {
			wptr->FTime = 0;
			wptr->state = 2;
		}
	}

	if (wptr->state == 2 && wptr->FTime > 0) {
		wptr->FTime += TimeDt;
		if (wptr->FTime >= wptr->chinfo.Animation[1].AniTime) {
			wptr->FTime = 0;
			wptr->state = 2;
		}
	}

	if (wptr->state == 3) {
		wptr->FTime += TimeDt;
		if (wptr->FTime >= wptr->chinfo.Animation[2].AniTime) {
			wptr->FTime = 0;
			wptr->state = 0;
			goto SKIPWEAPON;
		}
	}

	if (!ShotsLeft) HideWeapon();

	CreateMorphedModel(&wptr->chinfo.Model, &wptr->chinfo.Animation[wptr->state - 1], wptr->FTime);

	b = (float)sin((float)RealTime / 300.f) / 100.f;
	wpnDAlpha = wptr->shakel * std::sinf((float)RealTime / 300.f + Math::pi / 2) / 200.f;
	wpnDBeta = wptr->shakel * std::sinf((float)RealTime / 300.f) / 400.f;
	nv.z = 0;

	RenderNearModel(&wptr->chinfo.Model, 0, 0, 0, 0, -wpnDAlpha, -wpnDBeta);

	if (TargetWeapon == 2) {
		DrawOpticCross(wptr->chinfo.Model.gVertex.size() - 1);
	}

SKIPWEAPON:


	/*
	  if (BINMODE || OPTICMODE) {
	   CameraW*=BinocularPower;
	   CameraH*=BinocularPower;
	  }
	*/
	Hardware_ZBuffer(TRUE);

	if (Weapon.state)
		for (int bl = 0; bl < ShotsLeft; bl++)
			DrawPicture(6 + bl * BulletPic.W, 5, BulletPic);


	if (TrophyMode)
		DrawPicture(VideoCX - TrophyExit.W / 2, 2, TrophyExit);

	if (EXITMODE)
		DrawPicture((WinW - ExitPic.W) / 2, (WinH - ExitPic.H) / 2, ExitPic);

	if (PAUSE)
		DrawPicture((WinW - PausePic.W) / 2, (WinH - PausePic.H) / 2, PausePic);

	if (TrophyMode || TrophyTime)
		if (TrophyBody != -1) {
			int x0 = WinW - TrophyPic.W - 16;
			int y0 = WinH - TrophyPic.H - 12;
			if (!TrophyMode)
				x0 = VideoCX - TrophyPic.W / 2;

			DrawPicture(x0, y0, TrophyPic);
			DrawTrophyText(x0, y0);

			if (TrophyTime) {
				TrophyTime -= TimeDt;
				if (TrophyTime < 0) {
					TrophyTime = 0;
					TrophyBody = -1;
				}
			}
		}
}


void SwitchMode(const std::string& lps, bool& b)
{
	b = !b;
	std::stringstream ss;

	if (b)
		ss << lps << " is ON";
	else
		ss << lps << " is OFF";

	MessageBeep(0xFFFFFFFF);
	AddMessage(ss.str());
}


void ChangeViewR(int d)
{
	char buf[200];
	ctViewR += d;
	if (ctViewR < 10) ctViewR = 10;
	if (ctViewR > 60) ctViewR = 60;
	wsprintf(buf, "ViewR = %d", ctViewR);
	MessageBeep(0xFFFFFFFF);
	AddMessage(buf);
}


void ToggleBinocular()
{
	if (!GameState) return;
	if (Weapon.state) return;
	if (UNDERWATER) return;
	if (!MyHealth) return;
	BINMODE = !BINMODE;
	MapMode = FALSE;
}


void ToggleRunMode()
{
	RunMode = !RunMode;
	if (RunMode) AddMessage("Run mode is ON");
	else AddMessage("Run mode is OFF");
}


void ToggleMapMode()
{
	if (!MyHealth) return;
	if (BINMODE) return;
	if (Weapon.state) return;
	MapMode = !MapMode;
}


void AcceptNewKey()
{
	if (GetKeyboardState(KeyboardState)) {

		if (KeyboardState[VK_ESCAPE] & 128)
			*((int*)(&KeyMap) + WaitKey) = 0;
		else
			for (int k = 0; k < 255; k++) {
				if (KeyboardState[k] & 128) {

					for (int t = 0; t < 16; t++)
						if (*((int*)(&KeyMap) + t) == k)
							*((int*)(&KeyMap) + t) = 0;

					*((int*)(&KeyMap) + WaitKey) = k;
					wait_mouse_release();
					WaitKey = -1;
					return;
					//break;
				}
			}
	}

	WaitKey = -1;
}


void ProcessNameEdit(int key)
{
	std::string& s = TrophyRoom.PlayerName;

	// RH: keycode 8 is backspace?
	if (key == 8) {
		if (!s.empty()) s.pop_back();
		return;
	}
	else if (s.length() >= 10) {
		return;
	}

	if (key >= 32 || key <= 128) {
		s.push_back(static_cast<char>(key));
	}
}


LONG APIENTRY MainWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	bool A = (GetActiveWindow() == hWnd);

	if (A != blActive) {
		blActive = A;

		if (GameState) {
			if (blActive) {
				Activate3DHardware();
				NeedRVM = true;
			}
			else {
				ShutDown3DHardware();
			}
		}

		if (blActive) {
			Audio_Restore();
		}
	}

	if (WaitKey != -1)
		if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN ||
			message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN)
			AcceptNewKey();

	if (!GameState && MenuState == -1) {
		if (message == WM_CHAR) {
			ProcessNameEdit((int)wParam);
		}
	}

	if (GameState)
		if (message == WM_KEYDOWN) {
			if ((int)wParam == KeyMap.fkBinoc) ToggleBinocular();
			if ((int)wParam == KeyMap.fkRun) ToggleRunMode();
			if ((int)wParam == cheatcode[cheati]) {
				cheati++;
				if (cheati > 6) {
					cheati = 0;
					SwitchMode("Debug mode", DEBUG);
				}
			}
			else cheati = 0;
		}


	switch (message) {
	case WM_CREATE: return 0;
	case WM_SYSKEYDOWN:
		if ((int)wParam == VK_RETURN)
			SetFullScreen();
		return 0;

	case WM_KEYDOWN: {
		bool CTRL = (GetKeyState(VK_SHIFT) & 0x8000);
		switch ((int)wParam) {
		case 219: if (DEBUG) ChangeViewR(-2); break;
		case 221: if (DEBUG) ChangeViewR(+2); break;

		case 'S': if (DEBUG && GameState && CTRL) SwitchMode("Slow mode", SLOW);
			break;
		case 'T': if (DEBUG && GameState && CTRL) SwitchMode("Timer", TIMER);
			break;

			/*
			case 'M': if (CTRL) SwitchMode("Draw 3D models",MODELS);
					  break;
			case 'P': if (CTRL) SwitchMode("Perspective correction",CORRECTION);
					  break;
			case 'F': if (CTRL) SwitchMode("V.Fog",FOGENABLE);
					  break;
			case 'C': if (CTRL) SwitchMode("Clouds shadow",Clouds);
					  break;
			case 'H': if (CTRL) SwitchMode("Clouds shadow",SHADOWS3D);
					  break;
			*/
		case VK_TAB:
			if (GameState && !TrophyMode) ToggleMapMode();
			//MapMode=!MapMode;
			break;

		case VK_PAUSE:
			if (GameState) {
				PAUSE = !PAUSE; EXITMODE = FALSE; ResetMousePos();
			}
			break;

		case 'N':
			if (EXITMODE) EXITMODE = FALSE;
			break;

		case VK_ESCAPE:
			if (GameState)
				if (TrophyMode) {
					GameState = 0;
					SaveTrophy();
				}
				else {
					if (PAUSE) PAUSE = false;
					else EXITMODE = !EXITMODE;
					if (ExitTime) EXITMODE = false;
					ResetMousePos();
				}
			break;

		case 'Y':
		case VK_RETURN:
			if (GameState && EXITMODE) {
				if (MyHealth) ExitTime = 4000; else ExitTime = 1;
				EXITMODE = false;
			}
			break;

		case 'R':
			if (GameState && TrophyBody != -1) RemoveCurrentTrophy();
			if (GameState && EXITMODE) {
				LoadTrophy();
				RestartMode = false;
				GameState = 0;
				ShowMenuVideo();
			}
			break;

		case VK_F9:
			DestroyWindow(hwndMain);
			break;
		case VK_F12: SaveScreenShot();              break;

		}   // switch  
		break;     }

	case WM_SIZE: return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
		//case WM_ERASEBKGND:
		//case WM_NCPAINT   : break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC  hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);

		if (!GameState) ShowMenuVideo();
		return 0;
	}
	default:
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return 0;
}


bool CreateMainWindow()
{
	std::cout << "Creating main window... " << std::endl;

	std::string window_title = "Carnivores - ";

#ifdef _soft
	window_title += "3D:Soft";
#elif _3dfx
	window_title += "3D:3DFX Glide";
#elif _d3d
	window_title += "3D:Direct3D 7.0";
#elif _d3d11
	window_title += "3D:Direct3D 11";
#elif _opengl
	window_title += "3D:OpenGL 2.1";
#endif

	WNDCLASS wc;
	std::memset(&wc, 0, sizeof(WNDCLASS));

	wc.style = CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = static_cast<HINSTANCE>(hInst);
	wc.hIcon = wc.hIcon = LoadIcon((HINSTANCE)hInst, "ACTION");
	wc.hCursor = NULL;
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "HuntWindow";

	if (!RegisterClass(&wc))
		return false;

	hwndMain = CreateWindow(
		"HuntWindow", window_title.c_str(),
		WS_VISIBLE | WS_BORDER | WS_CAPTION | WS_POPUP,
		0, 0, 800, 600, NULL, NULL, (HINSTANCE)hInst, NULL);

	if (IsWindow(hwndMain))
		std::cout << "Ok." << std::endl;
	else {
		std::cout << "Fail!" << std::endl;
		return false;
	}

	int WX = (GetSystemMetrics(SM_CXSCREEN) / 2) - 400;
	int WY = (GetSystemMetrics(SM_CYSCREEN) / 2) - 300;
	RECT rc = { WX, WY, WX + 800, WY + 600 };
	AdjustWindowRect (&rc, GetWindowLong(hwndMain, GWL_STYLE), false);
	SetWindowPos (hwndMain, HWND_TOP, rc.left, rc.top, rc.right  - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);

	UpdateWindow(hwndMain);

	return true;
}


void HideWeapon()
{
	if (ObservMode || TrophyMode) return;
	TWeapon* wptr = &Weapon;
	if (wptr->state == 0) {
		if (!ShotsLeft) return;
		if (TargetWeapon == 2) OPTICMODE = true;
		AddVoice(wptr->chinfo.SoundFX[0].length,
			wptr->chinfo.SoundFX[0].lpData);
		wptr->FTime = 0;
		wptr->state = 1;
		BINMODE = false;
		MapMode = false;
		wptr->shakel = 0.2f;
		return;
	}

	if (wptr->state != 2 || wptr->FTime != 0) return;
	AddVoice(wptr->chinfo.SoundFX[2].length,
		wptr->chinfo.SoundFX[2].lpData);
	wptr->state = 3;
	wptr->FTime = 0;
	OPTICMODE = false;
	return;
}


void ProcessShoot()
{
	if (HeadBackR) return;
	if (!ShotsLeft) return;

	TWeapon* wptr = &Weapon;
	if (wptr->state == 2 && wptr->FTime == 0) {
		wptr->FTime = 1;
		HeadBackR = 64;

		AddVoice(wptr->chinfo.SoundFX[1].length,
			wptr->chinfo.SoundFX[1].lpData);

		float ca = (float)cos(PlayerAlpha + wpnDAlpha);
		float sa = (float)sin(PlayerAlpha + wpnDAlpha);
		float cb = (float)cos(PlayerBeta + wpnDBeta);
		float sb = (float)sin(PlayerBeta + wpnDBeta);

		nv.x = sa;
		nv.y = 0;
		nv.z = -ca;

		nv.x *= cb;
		nv.y = -sb;
		nv.z *= cb;

		MakeShot(PlayerX, PlayerY + HeadY, PlayerZ,
			PlayerX + nv.x * 256 * ctViewR,
			PlayerY + nv.y * 256 * ctViewR + HeadY,
			PlayerZ + nv.z * 256 * ctViewR);

		Vector3d v;
		v.x = PlayerX;
		v.y = PlayerY;
		v.z = PlayerZ;
		MakeNoise(v, 18 * 256 / WeapInfo[TargetWeapon].Loud);
		ShotsLeft--;
	}
}


void ProcessSlide()
{
	if (NOCLIP || UNDERWATER) return;
	float ch = GetLandQHNoObj(PlayerX, PlayerZ);
	float mh = ch;
	float chh;
	int   sd = 0;

	chh = GetLandQHNoObj(PlayerX - 16, PlayerZ); if (chh < mh) { mh = chh; sd = 1; }
	chh = GetLandQHNoObj(PlayerX + 16, PlayerZ); if (chh < mh) { mh = chh; sd = 2; }
	chh = GetLandQHNoObj(PlayerX, PlayerZ - 16); if (chh < mh) { mh = chh; sd = 3; }
	chh = GetLandQHNoObj(PlayerX, PlayerZ + 16); if (chh < mh) { mh = chh; sd = 4; }

	chh = GetLandQHNoObj(PlayerX - 12, PlayerZ - 12); if (chh < mh) { mh = chh; sd = 5; }
	chh = GetLandQHNoObj(PlayerX + 12, PlayerZ - 12); if (chh < mh) { mh = chh; sd = 6; }
	chh = GetLandQHNoObj(PlayerX - 12, PlayerZ + 12); if (chh < mh) { mh = chh; sd = 7; }
	chh = GetLandQHNoObj(PlayerX + 12, PlayerZ + 12); if (chh < mh) { mh = chh; sd = 8; }

	if (!NOCLIP)
		if (mh < ch - 16) {
			float delta = (ch - mh) / 4;
			if (sd == 1) { PlayerX -= delta; }
			if (sd == 2) { PlayerX += delta; }
			if (sd == 3) { PlayerZ -= delta; }
			if (sd == 4) { PlayerZ += delta; }

			delta *= 0.7f;
			if (sd == 5) { PlayerX -= delta; PlayerZ -= delta; }
			if (sd == 6) { PlayerX += delta; PlayerZ -= delta; }
			if (sd == 7) { PlayerX -= delta; PlayerZ += delta; }
			if (sd == 8) { PlayerX += delta; PlayerZ += delta; }
		}
}


void ProcessPlayerMovement()
{

	POINT ms;
	if (g_FullScreen) {
		GetCursorPos(&ms);
		if (REVERSEMS) ms.y = -ms.y + VideoCY * 2;
		rav += (float)(ms.x - VideoCX) / 600.f;
		rbv += (float)(ms.y - VideoCY) / 600.f;
		if (KeyFlags & kfStrafe)
			SSpeed += (float)rav * 10; else
			PlayerAlpha += rav;
		PlayerBeta += rbv;

		rav /= (2.f + (float)TimeDt / 20.f);
		rbv /= (2.f + (float)TimeDt / 20.f);
		ResetMousePos();
	}


	if (!(KeyFlags & (kfForward | kfBackward)))
		if (VSpeed > 0) VSpeed = max(0, VSpeed - DeltaT * 2);
		else VSpeed = min(0, VSpeed + DeltaT * 2);

	if (!(KeyFlags & (kfSLeft | kfSRight)))
		if (SSpeed > 0) SSpeed = max(0, SSpeed - DeltaT * 2);
		else SSpeed = min(0, SSpeed + DeltaT * 2);

	if (KeyFlags & kfForward)  if (VSpeed > 0) VSpeed += DeltaT; else VSpeed += DeltaT * 4;
	if (KeyFlags & kfBackward) if (VSpeed < 0) VSpeed -= DeltaT; else VSpeed -= DeltaT * 4;

	if (KeyFlags & kfSRight)  if (SSpeed > 0) SSpeed += DeltaT; else SSpeed += DeltaT * 4;
	if (KeyFlags & kfSLeft)  if (SSpeed < 0) SSpeed -= DeltaT; else SSpeed -= DeltaT * 4;


	if (SWIM) {
		if (VSpeed > 0.25f) VSpeed = 0.25f;
		if (VSpeed < -0.25f) VSpeed = -0.25f;
		if (SSpeed > 0.25f) SSpeed = 0.25f;
		if (SSpeed < -0.25f) SSpeed = -0.25f;
	}
	if (RunMode && (HeadY == 220.f) && (Weapon.state == 0)) {
		if (VSpeed > 0.7f) VSpeed = 0.7f;
		if (VSpeed < -0.7f) VSpeed = -0.7f;
		if (SSpeed > 0.7f) SSpeed = 0.7f;
		if (SSpeed < -0.7f) SSpeed = -0.7f;
	}
	else {
		if (VSpeed > 0.3f) VSpeed = 0.3f;
		if (VSpeed < -0.3f) VSpeed = -0.3f;
		if (SSpeed > 0.30f) SSpeed = 0.30f;
		if (SSpeed < -0.30f) SSpeed = -0.30f;
	}

	if (KeyboardState[KeyMap.fkFire] & 128) ProcessShoot();

	if (!UNDERWATER)
		if (KeyboardState[KeyMap.fkShow] & 128) HideWeapon();

	if (BINMODE) {
		if (KeyboardState[VK_ADD] & 128) BinocularPower += BinocularPower * TimeDt / 4000.f;
		if (KeyboardState[VK_SUBTRACT] & 128) BinocularPower -= BinocularPower * TimeDt / 4000.f;
		if (BinocularPower < 1.5f) BinocularPower = 1.5f;
		if (BinocularPower > 3.0f) BinocularPower = 3.0f;
	}

	if (KeyFlags & kfCall) MakeCall();

	if (DEBUG)
		if (KeyboardState[VK_CONTROL] & 128)
			if (KeyFlags & kfBackward) VSpeed = -4; else VSpeed = 4;

	if (KeyFlags & kfJump)
		if (YSpeed == 0 && !SWIM) {
			YSpeed = 600 + (float)fabs(VSpeed) * 600;
			AddVoice(fxJump.length, fxJump.lpData);
		}

	//=========  rotation =========//   
	if (KeyFlags & kfRight)  PlayerAlpha += DeltaT * 1.5f;
	if (KeyFlags & kfLeft)  PlayerAlpha -= DeltaT * 1.5f;
	if (KeyFlags & kfLookUp) PlayerBeta -= DeltaT;
	if (KeyFlags & kfLookDn) PlayerBeta += DeltaT;


	//========= movement ==========//

	ca = (float)cos(PlayerAlpha);
	sa = (float)sin(PlayerAlpha);
	cb = (float)cos(PlayerBeta);
	sb = (float)sin(PlayerBeta);

	nv.x = sa;
	nv.y = 0;
	nv.z = -ca;


	PlayerNv = nv;
	if (UNDERWATER) {
		nv.x *= cb;
		nv.y = -sb;
		nv.z *= cb;
		PlayerNv = nv;
	}
	else {
		PlayerNv.x *= cb;
		PlayerNv.y = -sb;
		PlayerNv.z *= cb;
	}

	Vector3d sv = nv;
	nv.x *= (float)TimeDt * VSpeed;
	nv.y *= (float)TimeDt * VSpeed;
	nv.z *= (float)TimeDt * VSpeed;

	sv.x *= (float)TimeDt * SSpeed;
	sv.y = 0;
	sv.z *= (float)TimeDt * SSpeed;

	if (!TrophyMode) {
		TrophyRoom.Last.path += (TimeDt * VSpeed) / 128.f;
		TrophyRoom.Last.time += TimeDt / 1000.f;
	}

	//if (SWIM & (VSpeed>0.1) & (sb>0.60)) HeadY-=40;

	int mvi = 1 + TimeDt / 16;

	for (int mvc = 0; mvc < mvi; mvc++) {
		PlayerX += nv.x / mvi;
		PlayerY += nv.y / mvi;
		PlayerZ += nv.z / mvi;

		PlayerX -= sv.z / mvi;
		PlayerZ += sv.x / mvi;

		if (!NOCLIP) CheckCollision(PlayerX, PlayerZ);

		if (PlayerY <= GetLandQHNoObj(PlayerX, PlayerZ) + 16) {
			ProcessSlide();
			ProcessSlide();
		}
	}

	if (PlayerY <= GetLandQHNoObj(PlayerX, PlayerZ) + 16) {
		ProcessSlide();
		ProcessSlide();
	}
	//===========================================================      
}


void ProcessDemoMovement()
{
	BINMODE = FALSE;

	PAUSE = FALSE;
	MapMode = FALSE;

	if (DemoPoint.DemoTime > 6 * 1000)
		if (!PAUSE) {
			EXITMODE = TRUE;
			ResetMousePos();
		}

	if (DemoPoint.DemoTime > 12 * 1000) {
		ResetMousePos();
		DemoPoint.DemoTime = 0;
		LoadTrophy();
		GameState = 0;
		return;
	}

	VSpeed = 0.f;

	DemoPoint.pos = Characters[DemoPoint.CIndex].pos;
	DemoPoint.pos.y += 256;
	if (Characters[DemoPoint.CIndex].CType == CTYPE_TREX)
		DemoPoint.pos.y += 512;

	Vector3d nv = SubVectors(DemoPoint.pos, CameraPos);
	Vector3d pp = DemoPoint.pos;
	pp.y = CameraPos.y;
	float l = VectorLength(SubVectors(pp, CameraPos));
	float base = 824;
	if (Characters[DemoPoint.CIndex].CType == CTYPE_TREX) base = 1424;

	if (DemoPoint.DemoTime == 1)
		if (l < base) DemoPoint.DemoTime = 2;
	NormVector(nv, 1.0f);

	if (DemoPoint.DemoTime == 1) {
		DeltaFunc(CameraX, DemoPoint.pos.x, (float)fabs(nv.x) * TimeDt * 3.f);
		DeltaFunc(CameraZ, DemoPoint.pos.z, (float)fabs(nv.z) * TimeDt * 3.f);
	}
	else {
		DemoPoint.DemoTime += TimeDt;
		CameraAlpha += TimeDt / 1224.f;
		ca = (float)cos(CameraAlpha);
		sa = (float)sin(CameraAlpha);
		//float k = (base - l) / 350.f;
		DeltaFunc(CameraX, DemoPoint.pos.x - sa * base, (float)TimeDt);
		DeltaFunc(CameraZ, DemoPoint.pos.z + ca * base, (float)TimeDt);
	}

	float b = FindVectorAlpha((float)
		sqrt((DemoPoint.pos.x - CameraX) * (DemoPoint.pos.x - CameraX) +
		(DemoPoint.pos.z - CameraZ) * (DemoPoint.pos.z - CameraZ)),
		DemoPoint.pos.y - CameraY - 400.f);
	if (b > Math::pi) b = b - 2 * pi;
	DeltaFunc(CameraBeta, -b, TimeDt / 4000.f);



	float h = GetLandQH(CameraX, CameraZ);
	DeltaFunc(CameraY, h + 128, TimeDt / 8.f);
	if (CameraY < h + 80) CameraY = h + 80;
}


void ProcessControls()
{
	int _KeyFlags = KeyFlags;
	KeyFlags = 0;
	
	if (GetKeyboardState(KeyboardState)) {

		if (KeyboardState[KeyMap.fkStrafe] & 128) KeyFlags += kfStrafe;

		if (KeyboardState[KeyMap.fkForward] & 128) KeyFlags += kfForward;
		if (KeyboardState[KeyMap.fkBackward] & 128) KeyFlags += kfBackward;
		if (KeyboardState[KeyMap.fkCrouch] & 128) KeyFlags += kfDown;

		if (KeyboardState[KeyMap.fkUp] & 128)  KeyFlags += kfLookUp;
		if (KeyboardState[KeyMap.fkDown] & 128)  KeyFlags += kfLookDn;

		if (KeyFlags & kfStrafe) {
			if (KeyboardState[KeyMap.fkLeft] & 128)  KeyFlags += kfSLeft;
			if (KeyboardState[KeyMap.fkRight] & 128) KeyFlags += kfSRight;
		}
		else {
			if (KeyboardState[KeyMap.fkLeft] & 128)  KeyFlags += kfLeft;
			if (KeyboardState[KeyMap.fkRight] & 128) KeyFlags += kfRight;
		}

		if (KeyboardState[KeyMap.fkSLeft] & 128) KeyFlags += kfSLeft;
		if (KeyboardState[KeyMap.fkSRight] & 128) KeyFlags += kfSRight;


		if (KeyboardState[KeyMap.fkJump] & 128) KeyFlags += kfJump;

		if (KeyboardState[KeyMap.fkCall] & 128)
			if (!(_KeyFlags & kfCall)) KeyFlags += kfCall;
	}

	DeltaT = (float)TimeDt / 1000.f;

	if (DemoPoint.DemoTime) ProcessDemoMovement();
	if (!DemoPoint.DemoTime) ProcessPlayerMovement();


	//======= Y movement ===========//   
	HeadAlpha = HeadBackR / 20000;
	HeadBeta = -HeadBackR / 10000;
	if (HeadBackR) {
		HeadBackR -= DeltaT * (80 + (32 - (float)fabs(HeadBackR - 32)) * 4);
		if (HeadBackR <= 0) { HeadBackR = 0; HeadBSpeed = 0; }
	}

	if ((KeyFlags & kfDown) && (UNDERWATER)) {
		if (HeadY < 110.f) HeadY = 110.f;
		HeadY -= DeltaT * (60 + (HeadY - 110) * 5);
		if (HeadY < 110.f) HeadY = 110.f;
	}
	else {
		if (HeadY > 220.f) HeadY = 220.f;
		HeadY += DeltaT * (60 + (220 - HeadY) * 5);
		if (HeadY > 220.f) HeadY = 220.f;
	}


	float h = GetLandQH(PlayerX, PlayerZ);
	float hwater = GetLandUpH(PlayerX, PlayerZ);

	if (DemoPoint.DemoTime) goto SKIPYMOVE;

	if (!UNDERWATER) {
		if (PlayerY > h) YSpeed -= DeltaT * 3000;
	}
	else
		if (YSpeed < 0) {
			YSpeed += DeltaT * 4000;
			if (YSpeed > 0) YSpeed = 0;
		}

	PlayerY += YSpeed * DeltaT;
	if (PlayerY <= h) {
		if (YSpeed < -800) HeadY += YSpeed / 100;
		if (PlayerY + 80 < h) PlayerY = h - 80;
		PlayerY += (h - PlayerY + 32) * DeltaT * 4;
		if (PlayerY > h) PlayerY = h;
		if (YSpeed < -600)
			AddVoicev(fxStep[(RealTime % 3)].length,
				fxStep[(RealTime % 3)].lpData, 64);
		YSpeed = 0;
	}

SKIPYMOVE:

	SWIM = FALSE;
	if (!UNDERWATER && (KeyFlags & kfJump))
		if (PlayerY < hwater - 148) { SWIM = TRUE; PlayerY = hwater - 148; YSpeed = 0; }

	float _s = stepdy;

	if (SWIM) stepdy = (float)sin((float)RealTime / 360) * 20;
	else stepdy = (float)min(1.f, fabs(VSpeed) + (float)fabs(SSpeed)) * (float)sin((float)RealTime / 80.f) * 22.f;
	float d = stepdy - _s;

	if (PlayerY < h + 64)
		if (d < 0 && stepdd >= 0)
			AddVoicev(fxStep[(RealTime % 3)].length,
				fxStep[(RealTime % 3)].lpData, 24 + (int)(VSpeed * 50.f));
	stepdd = d;

	if (PlayerBeta > 1.46f) PlayerBeta = 1.46f;
	if (PlayerBeta < -1.26f) PlayerBeta = -1.26f;


	//======== set camera pos ===================//

	if (!DemoPoint.DemoTime) {
		CameraAlpha = PlayerAlpha + HeadAlpha;
		CameraBeta = PlayerBeta + HeadBeta;

		CameraX = PlayerX - sa * HeadBackR;
		CameraY = PlayerY + HeadY + stepdy;
		CameraZ = PlayerZ + ca * HeadBackR;
	}

	if (CLIP3D) {
		if (sb < 0) BackViewR = 320.f - 1024.f * sb;
		else BackViewR = 320.f + 512.f * sb;
		BackViewRR = 380 + (int)(1024 * fabs(sb));
		if (UNDERWATER) BackViewR -= 512.f * (float)min(0, sb);
	}
	else {
		BackViewR = 300;
		BackViewRR = 380;
	}


	//==================== SWIM & UNDERWATER =========================//
	if (UNDERWATER) {
		UNDERWATER = (GetLandUpH(CameraX, CameraZ) - 4 >= CameraY) | (FLY);
		if (!UNDERWATER) {
			HeadY += 20; CameraY += 20;
			AddVoice(fxWaterOut.length, fxWaterOut.lpData);
		}
	}
	else {
		UNDERWATER = (GetLandUpH(CameraX, CameraZ) + 28 >= CameraY) | (FLY);
		if (UNDERWATER) {
			HeadY -= 20; CameraY -= 20;
			BINMODE = FALSE;
			AddVoice(fxWaterIn.length, fxWaterIn.lpData);
		}
	}

	if (MyHealth)
		if (UNDERWATER) {
			MyHealth -= TimeDt * 12;
			if (MyHealth <= 0)
				AddDeadBody(NULL, HUNT_BREATH);
		}

	if (UNDERWATER)
		if (Weapon.state) HideWeapon();

	if (!UNDERWATER) UnderWaterT = 0;
	else if (UnderWaterT < 512) UnderWaterT += TimeDt; else UnderWaterT = 512;

	if (UNDERWATER) {
		CameraW = (float)VideoCX * (1.25f + (1.f + (float)cos(RealTime / 180.f)) / 30 + (1.f - (float)sin(UnderWaterT / 512.f * pi / 2)) / 1.5f);
		CameraH = (float)VideoCX * (1.25f + (1.f + (float)sin(RealTime / 180.f)) / 30 - (1.f - (float)sin(UnderWaterT / 512.f * pi / 2)) / 16.f);

		CameraAlpha += (float)cos(RealTime / 360.f) / 120;
		CameraBeta += (float)sin(RealTime / 360.f) / 100;
		CameraY -= (float)sin(RealTime / 360.f) * 4;
		FogsList[127].YBegin = (GetLandUpH(CameraX, CameraZ) / ctHScale) + 8;
	}
	else {
		CameraW = (float)VideoCX * 1.25f;
		CameraH = CameraW;
	}


	ctViewR = 36;
	if (BINMODE) {
		ctViewR = 40;
		CameraW *= BinocularPower;
		CameraH *= BinocularPower;
	}
	else if (OPTICMODE) {
		ctViewR = 40;
		CameraW *= 3.0f;
		CameraH *= 3.0f;
	}

	if (SWIM) {
		CameraBeta -= (float)cos(RealTime / 360.f) / 80;
		PlayerX += DeltaT * 32;
		PlayerZ += DeltaT * 32;
	}


	CameraFogI = FogsMap[((int)CameraZ) >> 9][((int)CameraX) >> 9];
	if (FogsList[CameraFogI].YBegin * ctHScale > CameraY)
		CAMERAINFOG = (CameraFogI > 0);
	else
		CAMERAINFOG = FALSE;

	if (CAMERAINFOG)
		if (MyHealth)
			if (FogsList[CameraFogI].Mortal) {
				if (MyHealth > 100000) MyHealth = 100000;
				MyHealth -= TimeDt * 64;
				if (MyHealth <= 0)
					AddDeadBody(NULL, HUNT_EAT);
			}

	int CameraAmb = AmbMap[((int)CameraZ) >> 9][((int)CameraX) >> 9];


	if (UNDERWATER) SetAmbient(fxUnderwater.length,
		fxUnderwater.lpData,
		240); else
	{
		SetAmbient(Ambient[CameraAmb].sfx.length,
			Ambient[CameraAmb].sfx.lpData,
			Ambient[CameraAmb].AVolume);

		if (Ambient[CameraAmb].RSFXCount) {
			Ambient[CameraAmb].RndTime -= TimeDt;
			if (Ambient[CameraAmb].RndTime <= 0) {
				Ambient[CameraAmb].RndTime = (Ambient[CameraAmb].rdata[0].RFreq / 2 + rRand(Ambient[CameraAmb].rdata[0].RFreq)) * 1000;
				int rr = (rand() % Ambient[CameraAmb].RSFXCount);
				int r = Ambient[CameraAmb].rdata[rr].RNumber;
				AddVoice3dv(RandSound[r].length, RandSound[r].lpData,
					CameraX + siRand(4096),
					CameraY + siRand(4096),
					CameraZ + siRand(256),
					Ambient[CameraAmb].rdata[rr].RVolume);
			}
		}
	}


	if (NOCLIP) CameraY += 1024;
	//======= results ==========//
	if (CameraBeta > 1.46f) CameraBeta = 1.46f;
	if (CameraBeta < -1.26f) CameraBeta = -1.26f;

	PlayerPos.x = PlayerX;
	PlayerPos.y = PlayerY;
	PlayerPos.z = PlayerZ;

	CameraPos.x = CameraX;
	CameraPos.y = CameraY;
	CameraPos.z = CameraZ;

}


void ProcessGame()
{
	// -- This checks if the game state is initialising, then does Game::Init() stuff
	if (_GameState != GameState) {
		std::cout << "\tEntered game" << std::endl;
		SaveTrophy();
		LoadResources();
		AudioStop();
		SetVideoMode(WinW, WinH);
		while (ShowCursor(false) >= 0);
		Activate3DHardware();
	}

	_GameState = GameState;

	if (NeedRVM)
	{
		SetVideoMode(WinW, WinH);
		NeedRVM = false;
	}


	ProcessSyncro();
	/*
	if (KeyboardState[VK_CAPITAL] & 1) {
			 SaveScreenShot();
			 TimeDt = 66;
	}         */

	if (!PAUSE || !MyHealth)
	{
		ProcessControls();
		AudioSetCameraPos(CameraX, CameraY, CameraZ, CameraAlpha);
		AnimateCharacters();
		AnimateProcesses();
	}

	if (!GameState)
		return;

	if (DEBUG || ObservMode || TrophyMode)
	{
		if (MyHealth)
			MyHealth = MAX_HEALTH;
	}

	if (DEBUG)
	{
		ShotsLeft = WeapInfo[TargetWeapon].Shots;
	}

	DrawScene();

	if (!TrophyMode && MapMode)
	{
		DrawHMap();
	}

	DrawPostObjects();

	ShowControlElements();

	ShowVideo();
}


#if defined(_DEBUG)
int main(int argc, char* argv[]) {
	hInst = GetModuleHandle(0);
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow) {
	hInst = hInstance;
#endif
	MSG msg = MSG();

	// Initialise the 
	g_TimeStart = std::chrono::steady_clock::now().time_since_epoch();

	CreateLog();

	try {
		CreateMainWindow();
		Init3DHardware();
		CarnivoresEngine cengine; // Replaces `InitEngine()`

		std::cout << "== Loading resources ==" << std::endl;
		hcArrow = LoadCursor(NULL, IDC_ARROW);
		//hcHand  = LoadCursor(hInst, "CROSSH");

		LoadCharacterInfo(ChInfo[CTYPE_HUNTER], "huntdat/hunter1.car");

		LoadCharacterInfo(ChInfo[CTYPE_MOSCH], "huntdat/mosh.car");
		LoadCharacterInfo(ChInfo[CTYPE_GALLI], "huntdat/gall.car");
		LoadCharacterInfo(ChInfo[CTYPE_DIMOR], "huntdat/dimor2.car");

		LoadCharacterInfo(ChInfo[CTYPE_PARA], "huntdat/par2.car");
		LoadCharacterInfo(ChInfo[CTYPE_PACHY], "huntdat/pach.car");
		LoadCharacterInfo(ChInfo[CTYPE_STEGO], "huntdat/stego.car");
		LoadCharacterInfo(ChInfo[CTYPE_ALLO], "huntdat/allo.car");
		LoadCharacterInfo(ChInfo[CTYPE_TRIKE], "huntdat/tricer.car");
		LoadCharacterInfo(ChInfo[CTYPE_VELOCI], "huntdat/velo2.car");
		LoadCharacterInfo(ChInfo[CTYPE_TREX], "huntdat/tirex.car");

		LoadModelEx(SunModel, "huntdat/sun2.3df");
		LoadModelEx(CompasModel, "huntdat/compas.3df");
		LoadModelEx(Binocular, "huntdat/binocul.3df");
		LoadCharacterInfo(ShipModel, "huntdat/ship2a.car");

		LoadCharacterInfo(WindModel, "huntdat/wind.car");
		LoadCharacterInfo(ExplodInfo, "huntdat/explo.car");

		LoadWav("huntdat/soundfx/menugo.wav", fxMenuGo);
		LoadWav("huntdat/soundfx/menumov.wav", fxMenuMov);

		LoadWav("huntdat/soundfx/menuamb.wav", fxMenuAmb);
		LoadWav("huntdat/soundfx/a_underw.wav", fxUnderwater);
		LoadWav("huntdat/soundfx/steps/hwalk1.wav", fxStep[0]);
		LoadWav("huntdat/soundfx/steps/hwalk2.wav", fxStep[1]);
		LoadWav("huntdat/soundfx/steps/hwalk3.wav", fxStep[2]);

		LoadWav("huntdat/SOUNDFX/hum_die1.wav", fxScream[0]);
		LoadWav("huntdat/SOUNDFX/hum_die2.wav", fxScream[1]);
		LoadWav("huntdat/SOUNDFX/hum_die3.wav", fxScream[2]);
		LoadWav("huntdat/SOUNDFX/hum_die4.wav", fxScream[3]);

		LoadPictureTGA(PausePic, "huntdat/MENU/pause.tga");         conv_pic(PausePic);
		LoadPictureTGA(ExitPic, "huntdat/MENU/exit.tga");           conv_pic(ExitPic);
		LoadPictureTGA(TrophyExit, "huntdat/MENU/trophy_e.tga");    conv_pic(TrophyExit);


		LoadPictureTGA(MapPic, "huntdat/MENU/mapframe.tga");        conv_pic(MapPic);

		LoadTextFile(ObserText, "huntdat/MENU/WEPPIC/observe.nfo");
		LoadTextFile(TranqText, "huntdat/MENU/WEPPIC/tranq.nfo");
		LoadTextFile(ComfText, "huntdat/MENU/WEPPIC/camoflag.nfo");
		LoadTextFile(RadarText, "huntdat/MENU/WEPPIC/radar.nfo");
		LoadTextFile(ScentText, "huntdat/MENU/WEPPIC/scent.nfo");

		LoadPlayersInfo();

		std::cout << "Loading resources: Done." << std::endl;

		ProcessSyncro();
		blActive = true;
		_GameState = -1;

		std::cout << "Entering messages loop." << std::endl;
		while (cengine.GetLoopStatus())
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					break;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else if (blActive)
			{
				if (GameState)
					ProcessGame();
				else
					ProcessMenu();

			}
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		ShutDown3DHardware(); // <- Redundant, ~CarnivoresEngine() calls this
		std::cout << "Game normal shutdown." << std::endl;
	}
	catch (std::runtime_error& e) {
		// We want to catch these first as they are usually thrown by Carnivores and not the C++ Standard Libary
		std::cout << "\n\n!! FATAL RUNTIME EXCEPTION !!" << e.what() << std::endl;

		std::stringstream ess;
		ess << "Fatal Runtime Exception!\n" << e.what() << "\n";
		MessageBox(HWND_DESKTOP, ess.str().c_str(), "Carnivores - Exception", MB_OK);
	}
	catch (std::exception& e) {
		// Catch C++ Standard Library Exceptions, because it means we didnt try/catch it ourselves so the entire game is probably borked anyway
		std::cout << "\n\n!! FATAL C++ STD::EXCEPTION !!" << e.what() << std::endl;

		std::stringstream ess;
		ess << "Fatal C++ Exception!\n" << e.what() << "\n";
		MessageBox(HWND_DESKTOP, ess.str().c_str(), "Carnivores - Exception", MB_OK);
	}
	catch (...) {
		// God help us all if we end up here somehow...
		std::cout << "\n\n!! Fatal Unknown Exception !!" << std::endl;
	}

	CloseLog();

	return msg.wParam;
}
