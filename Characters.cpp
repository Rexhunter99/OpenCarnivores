#include "Hunt.h"
#include <cstdio>

bool NewPhase = false;

#define fx_DIE    0


enum AlloStatesEnum {
	RAP_RUN = 0,
	RAP_WALK = 1,
	RAP_SWIM = 2,
	RAP_SLIDE = 3,
	RAP_JUMP = 4,
	RAP_DIE = 5,
	RAP_EAT = 6,
	RAP_SLP = 7
};

#define REX_RUN    0
#define REX_WALK   1
#define REX_SCREAM 2
#define REX_SWIM   3
#define REX_SEE    4
#define REX_SEE1   5
#define REX_SMEL   6
#define REX_SMEL1  7
#define REX_DIE    8
#define REX_EAT    9
#define REX_SLP    10

#define VEL_RUN    0
#define VEL_WALK   1
#define VEL_SWIM   3
#define VEL_SLIDE  2
#define VEL_JUMP   4
#define VEL_DIE    5
#define VEL_EAT    6
#define VEL_SLP    7

#define MOS_RUN    0
#define MOS_WALK   1
#define MOS_DIE    2
#define MOS_IDLE1  3 
#define MOS_IDLE2  4
#define MOS_SLP    5

#define STG_RUN    0
#define STG_WALK   1
#define STG_DIE    2
#define STG_IDLE1  3 
#define STG_IDLE2  4
#define STG_SLP    5

#define GAL_RUN    0
#define GAL_WALK   1
#define GAL_SLIDE  2
#define GAL_DIE    3
#define GAL_IDLE1  4 
#define GAL_IDLE2  5
#define GAL_SLP    6

#define TRI_RUN    0
#define TRI_WALK   1
#define TRI_IDLE1  2 
#define TRI_IDLE2  3
#define TRI_IDLE3  4
#define TRI_DIE    5
#define TRI_SLP    6


#define PAC_WALK   0
#define PAC_RUN    1
#define PAC_SLIDE  2
#define PAC_DIE    3
#define PAC_IDLE1  4
#define PAC_IDLE2  5
#define PAC_SLP    6


#define PAR_WALK   0
#define PAR_RUN    1
#define PAR_IDLE1  2
#define PAR_IDLE2  3
#define PAR_DIE    4
#define PAR_SLP    5


#define DIM_FLY    0
#define DIM_FLYP   1
#define DIM_FALL   2
#define DIM_DIE    3

int CurDino;

void SetNewTargetPlace(TCharacter* cptr, float R);


void ProcessPrevPhase(TCharacter& chr)
{
	chr.PPMorphTime += TimeDt;
	if (chr.PPMorphTime > PMORPHTIME) chr.PrevPhase = chr.Phase;

	chr.PrevPFTime += TimeDt;
	chr.PrevPFTime %= chr.pinfo->Animation[chr.PrevPhase].AniTime;
}


void ActivateCharacterFx(TCharacter* cptr)
{
	if (cptr->CType != CTYPE_HUNTER)
		if (UNDERWATER) return;
	int fx = cptr->pinfo->Anifx[cptr->Phase];
	if (fx == -1) return;

	if (VectorLength(SubVectors(PlayerPos, cptr->pos)) > 30 * 256) return;

	AddVoice3d(cptr->pinfo->SoundFX[fx].length,
		cptr->pinfo->SoundFX[fx].lpData,
		cptr->pos.x, cptr->pos.y, cptr->pos.z);
}


void ResetCharacter(TCharacter* cptr)
{
	cptr->pinfo = &ChInfo[cptr->CType];
	cptr->State = 0;
	cptr->StateF = 0;
	cptr->Phase = 0;
	cptr->FTime = 0;
	cptr->PrevPhase = 0;
	cptr->PrevPFTime = 0;
	cptr->PPMorphTime = 0;
	cptr->beta = 0;
	cptr->gamma = 0;
	cptr->tggamma = 0;
	cptr->bend = 0;
	cptr->rspeed = 0;
	cptr->AfraidTime = 0;

	cptr->lookx = std::cosf(cptr->alpha);
	cptr->lookz = std::sinf(cptr->alpha);

	cptr->Health = DinoInfo[cptr->CType].Health0;

	cptr->scale = (float)(DinoInfo[cptr->CType].Scale0 + rRand(DinoInfo[cptr->CType].ScaleA)) / 1000.f;
}


void AddDeadBody(TCharacter* cptr, int phase)
{
	if (!MyHealth) return;

	if (ExitTime)
		AddMessage("Transportation cancelled.");
	ExitTime = 0;

	OPTICMODE = FALSE;
	BINMODE = FALSE;
	Characters[ChCount].CType = CTYPE_HUNTER;
	Characters[ChCount].alpha = CameraAlpha;
	ResetCharacter(&Characters[ChCount]);

	int v = rRand(3);
	if (phase != HUNT_BREATH)
		AddVoice(fxScream[r].length, fxScream[r].lpData);

	Characters[ChCount].Health = 0;
	MyHealth = 0;
	if (cptr) {
		float pl = 170.f;
		if (cptr->CType == CTYPE_TREX) pl = 0;
		Characters[ChCount].pos.x = cptr->pos.x + cptr->lookx * pl * cptr->scale;
		Characters[ChCount].pos.z = cptr->pos.z + cptr->lookz * pl * cptr->scale;
		Characters[ChCount].pos.y = GetLandQH(Characters[ChCount].pos.x, Characters[ChCount].pos.z);
	}
	else {
		Characters[ChCount].pos.x = PlayerX;
		Characters[ChCount].pos.z = PlayerZ;
		Characters[ChCount].pos.y = PlayerY;
	}

	Characters[ChCount].Phase = phase;
	Characters[ChCount].PrevPhase = phase;

	ActivateCharacterFx(&Characters[ChCount]);


	DemoPoint.pos = Characters[ChCount].pos;
	DemoPoint.DemoTime = 1;
	DemoPoint.CIndex = ChCount;

	ChCount++;
}



float AngleDifference(float a, float b)
{
	a -= b;
	a = (float)fabs(a);
	if (a > pi) a = 2 * pi - a;
	return a;
}

void ThinkY_Beta_Gamma(TCharacter* cptr, float blook, float glook, float blim, float glim)
{
	cptr->pos.y = GetLandH(cptr->pos.x, cptr->pos.z);

	//=== beta ===//
	float hlook = GetLandH(cptr->pos.x + cptr->lookx * blook, cptr->pos.z + cptr->lookz * blook);
	float hlook2 = GetLandH(cptr->pos.x - cptr->lookx * blook, cptr->pos.z - cptr->lookz * blook);
	DeltaFunc(cptr->beta, (hlook2 - hlook) / (blook * 3.2f), TimeDt / 800.f);

	if (cptr->beta > blim) cptr->beta = blim;
	if (cptr->beta < -blim) cptr->beta = -blim;

	//=== gamma ===//
	hlook = GetLandH(cptr->pos.x + cptr->lookz * glook, cptr->pos.z - cptr->lookx * glook);
	hlook2 = GetLandH(cptr->pos.x - cptr->lookz * glook, cptr->pos.z + cptr->lookx * glook);
	cptr->tggamma = (hlook - hlook2) / (glook * 3.2f);
	if (cptr->tggamma > glim) cptr->tggamma = glim;
	if (cptr->tggamma < -glim) cptr->tggamma = -glim;
	/*
		if (DEBUG) cptr->tggamma = 0;
		if (DEBUG) cptr->beta    = 0;
		*/
}




int CheckPlaceCollisionP(Vector3d& v)
{
	int ccx = (int)v.x / 256;
	int ccz = (int)v.z / 256;

	if (ccx < 4 || ccz < 4 || ccx>508 || ccz>508) return 1;

	int F = (FMap[ccz][ccx - 1] | FMap[ccz - 1][ccx] | FMap[ccz - 1][ccx - 1] |
		FMap[ccz][ccx] |
		FMap[ccz + 1][ccx] | FMap[ccz][ccx + 1] | FMap[ccz + 1][ccx + 1]);

	if (F & (fmWater + fmNOWAY)) return 1;


	float h = GetLandH(v.x, v.z);
	v.y = h;

	float hh = GetLandH(v.x - 164, v.z - 164); if (fabs(hh - h) > 200) return 1;
	hh = GetLandH(v.x + 164, v.z - 164); if (fabs(hh - h) > 200) return 1;
	hh = GetLandH(v.x - 164, v.z + 164); if (fabs(hh - h) > 200) return 1;
	hh = GetLandH(v.x + 164, v.z + 164); if (fabs(hh - h) > 200) return 1;

	for (int z = -2; z <= 2; z++)
		for (int x = -2; x <= 2; x++)
			if (OMap[ccz + z][ccx + x] != 255) {
				int ob = OMap[ccz + z][ccx + x];
				if (MObjects[ob].info.Radius < 10) continue;
				float CR = (float)MObjects[ob].info.Radius + 64;

				float oz = (ccz + z) * 256.f + 128.f;
				float ox = (ccx + x) * 256.f + 128.f;

				float r = (float)sqrt((ox - v.x) * (ox - v.x) + (oz - v.z) * (oz - v.z));
				if (r < CR) return 1;
			}

	return 0;
}




int CheckPlaceCollision(Vector3d& v, BOOL wc, BOOL mc)
{
	int ccx = (int)v.x / 256;
	int ccz = (int)v.z / 256;

	if (ccx < 4 || ccz < 4 || ccx>508 || ccz>508) return 1;

	if (wc)
		if ((FMap[ccz][ccx - 1] | FMap[ccz - 1][ccx] | FMap[ccz - 1][ccx - 1] |
			FMap[ccz][ccx] |
			FMap[ccz + 1][ccx] | FMap[ccz][ccx + 1] | FMap[ccz + 1][ccx + 1]) & fmWater)
			return 1;


	float h = GetLandH(v.x, v.z);
	if (!(FMap[ccz][ccx] & fmWater))
		if (fabs(h - v.y) > 64) return 1;

	v.y = h;

	float hh = GetLandH(v.x - 64, v.z - 64); if (fabs(hh - h) > 124) return 1;
	hh = GetLandH(v.x + 64, v.z - 64); if (fabs(hh - h) > 124) return 1;
	hh = GetLandH(v.x - 64, v.z + 64); if (fabs(hh - h) > 124) return 1;
	hh = GetLandH(v.x + 64, v.z + 64); if (fabs(hh - h) > 124) return 1;

	if (mc)
		for (int z = -2; z <= 2; z++)
			for (int x = -2; x <= 2; x++)
				if (OMap[ccz + z][ccx + x] != 255) {
					int ob = OMap[ccz + z][ccx + x];
					if (MObjects[ob].info.Radius < 10) continue;
					float CR = (float)MObjects[ob].info.Radius + 64;

					float oz = (ccz + z) * 256.f + 128.f;
					float ox = (ccx + x) * 256.f + 128.f;

					float r = (float)sqrt((ox - v.x) * (ox - v.x) + (oz - v.z) * (oz - v.z));
					if (r < CR) return 1;
				}

	return 0;
}






int CheckPlaceCollision2(Vector3d& v, BOOL wc)
{
	int ccx = (int)v.x / 256;
	int ccz = (int)v.z / 256;

	if (ccx < 4 || ccz < 4 || ccx>508 || ccz>508) return 1;

	if (wc)
		if ((FMap[ccz][ccx - 1] | FMap[ccz - 1][ccx] | FMap[ccz - 1][ccx - 1] |
			FMap[ccz][ccx] |
			FMap[ccz + 1][ccx] | FMap[ccz][ccx + 1] | FMap[ccz + 1][ccx + 1]) & fmWater)
			return 1;

	float h = GetLandH(v.x, v.z);
	/*if (! (FMap[ccz][ccx] & fmWater) )
	  if (fabs(h - v.y) > 64) return 1;*/
	v.y = h;

	float hh = GetLandH(v.x - 64, v.z - 64); if (fabs(hh - h) > 124) return 1;
	hh = GetLandH(v.x + 64, v.z - 64); if (fabs(hh - h) > 124) return 1;
	hh = GetLandH(v.x - 64, v.z + 64); if (fabs(hh - h) > 124) return 1;
	hh = GetLandH(v.x + 64, v.z + 64); if (fabs(hh - h) > 124) return 1;

	return 0;
}



int CheckPossiblePath(TCharacter* cptr, BOOL wc, BOOL mc)
{
	Vector3d p = cptr->pos;
	float lookx = (float)cos(cptr->tgalpha);
	float lookz = (float)sin(cptr->tgalpha);
	int c = 0;
	for (int t = 0; t < 20; t++) {
		p.x += lookx * 64.f;
		p.z += lookz * 64.f;
		if (CheckPlaceCollision(p, wc, mc)) c++;
	}
	return c;
}


void LookForAWay(TCharacter* cptr, BOOL wc, BOOL mc)
{
	float alpha = cptr->tgalpha;
	float dalpha = 15.f;
	float afound = alpha;
	int maxp = 16;
	int curp;

	if (!CheckPossiblePath(cptr, wc, mc)) { cptr->NoWayCnt = 0; return; }

	cptr->NoWayCnt++;
	for (int i = 0; i < 12; i++) {
		cptr->tgalpha = alpha + dalpha * pi / 180.f;
		curp = CheckPossiblePath(cptr, wc, mc) + (i >> 1);
		if (!curp) return;
		if (curp < maxp) {
			maxp = curp;
			afound = cptr->tgalpha;
		}

		cptr->tgalpha = alpha - dalpha * pi / 180.f;
		curp = CheckPossiblePath(cptr, wc, mc) + (i >> 1);
		if (!curp) return;
		if (curp < maxp) {
			maxp = curp;
			afound = cptr->tgalpha;
		}

		dalpha += 15.f;
	}

	cptr->tgalpha = afound;
}




BOOL ReplaceCharacterForward(TCharacter* cptr)
{
	float al = CameraAlpha + (float)siRand(2048) / 2048.f;
	float sa = (float)sin(al);
	float ca = (float)cos(al);
	Vector3d p;
	p.x = PlayerX + sa * (36 + rRand(10)) * 256;
	p.z = PlayerZ - ca * (36 + rRand(10)) * 256;
	p.y = GetLandH(p.x, p.z);

	if (p.x < 16 * 256) return FALSE;
	if (p.z < 16 * 256) return FALSE;
	if (p.x > 500 * 256) return FALSE;
	if (p.z > 500 * 256) return FALSE;

	if (CheckPlaceCollisionP(p)) return FALSE;

	cptr->State = 0;
	cptr->pos = p;
	//cptr->tgx = cptr->pos.x + siRand(2048);
	//cptr->tgz = cptr->pos.z + siRand(2048);	 
	SetNewTargetPlace(cptr, 2048);
	if (cptr->CType == CTYPE_DIMOR)
		cptr->pos.y += 1048.f;
	return TRUE;
}


void Characters_AddSecondaryOne(int ctype)
{
	if (ChCount > 64) return;
	Characters[ChCount].CType = ctype;
replace1:
	Characters[ChCount].pos.x = PlayerX + siRand(20040);
	Characters[ChCount].pos.z = PlayerZ + siRand(20040);
	Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
		Characters[ChCount].pos.z);

	if (CheckPlaceCollisionP(Characters[ChCount].pos)) goto replace1;

	if (fabs(Characters[ChCount].pos.x - PlayerX) +
		fabs(Characters[ChCount].pos.z - PlayerZ) < 256 * 40)
		goto replace1;

	Characters[ChCount].tgx = Characters[ChCount].pos.x;
	Characters[ChCount].tgz = Characters[ChCount].pos.z;
	Characters[ChCount].tgtime = 0;

	ResetCharacter(&Characters[ChCount]);
	ChCount++;
}



void MoveCharacter(TCharacter* cptr, float dx, float dz, BOOL wc, BOOL mc)
{
	Vector3d p = cptr->pos;

	if (CheckPlaceCollision2(p, wc)) {
		cptr->pos.x += dx / 2;
		cptr->pos.z += dz / 2;
		return;
	}

	p.x += dx;
	p.z += dz;
	if (!CheckPlaceCollision2(p, wc)) {
		cptr->pos = p;
		return;
	}

	p = cptr->pos;
	p.x += dx / 2;
	p.z += dz / 2;
	if (!CheckPlaceCollision2(p, wc)) cptr->pos = p;
	p = cptr->pos;

	p.x += dx / 4;
	//if (!CheckPlaceCollision2(p)) cptr->pos = p; 	   
	p = cptr->pos;

	p.z += dz / 4;
	//if (!CheckPlaceCollision2(p)) cptr->pos = p; 	   
	p = cptr->pos;
}




void MoveCharacter2(TCharacter* cptr, float dx, float dz)
{
	cptr->pos.x += dx;
	cptr->pos.z += dz;
}




void SetNewTargetPlace(TCharacter* cptr, float R)
{
	Vector3d p;
	int tr = 0;
replace:
	p.x = cptr->pos.x + siRand((int)R);
	p.z = cptr->pos.z + siRand((int)R);
	p.y = GetLandH(p.x, p.z);
	tr++;
	if (fabs(p.x - cptr->pos.x) + fabs(p.z - cptr->pos.z) < R / 2.f) goto replace;
	//if (tr>16) return;
	R += 512;

	if (CheckPlaceCollisionP(p)) goto replace;

	cptr->tgtime = 0;
	cptr->tgx = p.x;
	cptr->tgz = p.z;
}



void AnimateHuntDead(TCharacter* cptr)
{
	if (!cptr)
		return;

	//if (!cptr->FTime) ActivateCharacterFx(cptr);

	ProcessPrevPhase(*cptr);
	bool NewPhase = false;

	cptr->FTime += TimeDt;
	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		NewPhase = true;
		if (cptr->Phase == 2)
			cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
		else
			cptr->FTime = 0;

		if (cptr->Phase == 1) {
			cptr->FTime = 0;
			cptr->Phase = 2;
		}

		ActivateCharacterFx(cptr);
	}


	float h = GetLandH(cptr->pos.x, cptr->pos.z);
	DeltaFunc(cptr->pos.y, h, TimeDt / 5.f);

	if (cptr->Phase == 2)
		if (cptr->pos.y > h + 3) {
			cptr->FTime = 0;
			//MessageBeep(0xFFFFFFFF);
		}


	if (cptr->pos.y < h + 256) {
		//=== beta ===//
		float blook = 256;
		float hlook = GetLandH(cptr->pos.x + cptr->lookx * blook, cptr->pos.z + cptr->lookz * blook);
		float hlook2 = GetLandH(cptr->pos.x - cptr->lookx * blook, cptr->pos.z - cptr->lookz * blook);
		DeltaFunc(cptr->beta, (hlook2 - hlook) / (blook * 3.2f), TimeDt / 1800.f);

		if (cptr->beta > 0.4f) cptr->beta = 0.4f;
		if (cptr->beta < -0.4f) cptr->beta = -0.4f;

		//=== gamma ===//
		float glook = 256;
		hlook = GetLandH(cptr->pos.x + cptr->lookz * glook, cptr->pos.z - cptr->lookx * glook);
		hlook2 = GetLandH(cptr->pos.x - cptr->lookz * glook, cptr->pos.z + cptr->lookx * glook);
		cptr->tggamma = (hlook - hlook2) / (glook * 3.2f);
		if (cptr->tggamma > 0.4f) cptr->tggamma = 0.4f;
		if (cptr->tggamma < -0.4f) cptr->tggamma = -0.4f;
		DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1800.f);
	}


	TCharacter& cptr2 = Characters[DemoPoint.CIndex];
	if (cptr2.CType == CTYPE_TREX) {
		cptr->pos = cptr2.pos;
		cptr->FTime = cptr2.FTime;
		cptr->beta = cptr2.beta;
		cptr->gamma = cptr2.gamma;
	}
}



void AnimateRaptorDead(TCharacter* cptr)
{

	if (cptr->Phase != RAP_DIE && cptr->Phase != RAP_SLP) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = RAP_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if (Tranq) {
				cptr->FTime = 0;
				cptr->Phase = RAP_SLP;
				ActivateCharacterFx(cptr);
			}
			else
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}



void AnimateVeloDead(TCharacter* cptr)
{

	if (cptr->Phase != VEL_DIE && cptr->Phase != VEL_SLP) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = VEL_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if (Tranq) {
				cptr->FTime = 0;
				cptr->Phase = VEL_SLP;
				ActivateCharacterFx(cptr);
			}
			else
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}



void AnimateTRexDead(TCharacter* cptr)
{

	if (cptr->Phase != REX_DIE) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = REX_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 200, 196, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}



void AnimateMoshDead(TCharacter* cptr)
{

	if (cptr->Phase != MOS_DIE && cptr->Phase != MOS_SLP) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = MOS_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if (Tranq) {
				cptr->FTime = 0;
				cptr->Phase = MOS_SLP;
				ActivateCharacterFx(cptr);
			}
			else
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateGallDead(TCharacter* cptr)
{

	if (cptr->Phase != GAL_DIE && cptr->Phase != GAL_SLP) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = GAL_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if (Tranq) {
				cptr->FTime = 0;
				cptr->Phase = GAL_SLP;
				ActivateCharacterFx(cptr);
			}
			else
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}



void AnimateDimorDead(TCharacter* cptr)
{

	if (cptr->Phase != DIM_FALL && cptr->Phase != DIM_DIE) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = DIM_FALL;
		cptr->rspeed = 0;
		ActivateCharacterFx(cptr);
		return;
	}

	ProcessPrevPhase(*cptr);

	cptr->FTime += TimeDt;
	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		if (cptr->Phase == DIM_DIE)
			cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
		else
			cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;


	//======= movement ===========//
	if (cptr->Phase == DIM_DIE)
		DeltaFunc(cptr->vspeed, 0, TimeDt / 400.f);
	else
		DeltaFunc(cptr->vspeed, 0, TimeDt / 1200.f);

	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	if (cptr->Phase == DIM_FALL) {
		cptr->pos.y += cptr->rspeed * TimeDt / 1024;
		cptr->rspeed -= TimeDt * 5;

		if (cptr->pos.y < GetLandH(cptr->pos.x, cptr->pos.z)) {
			cptr->pos.y = GetLandH(cptr->pos.x, cptr->pos.z);

			if (cptr->PPMorphTime > 128) {
				cptr->PrevPhase = cptr->Phase;
				cptr->PrevPFTime = cptr->FTime;
				cptr->PPMorphTime = 0;
			}

			cptr->Phase = DIM_DIE;
			cptr->FTime = 0;
			ActivateCharacterFx(cptr);
		}
	}
	else {
		ThinkY_Beta_Gamma(cptr, 140, 126, 0.6f, 0.5f);
		DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
	}
}





void AnimateTricDead(TCharacter* cptr)
{

	if (cptr->Phase != TRI_DIE && cptr->Phase != TRI_SLP) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = TRI_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if (Tranq) {
				cptr->FTime = 0;
				cptr->Phase = TRI_SLP;
				ActivateCharacterFx(cptr);
			}
			else
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}




void AnimatePacDead(TCharacter* cptr)
{

	if (cptr->Phase != PAC_DIE && cptr->Phase != PAC_SLP) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = PAC_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if (Tranq) {
				cptr->FTime = 0;
				cptr->Phase = PAC_SLP;
				ActivateCharacterFx(cptr);
			}
			else
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}



void AnimateStegDead(TCharacter* cptr)
{

	if (cptr->Phase != STG_DIE && cptr->Phase != STG_SLP) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = STG_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if (Tranq) {
				cptr->FTime = 0;
				cptr->Phase = STG_SLP;
				ActivateCharacterFx(cptr);
			}
			else
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}



void AnimateParDead(TCharacter* cptr)
{

	if (cptr->Phase != PAR_DIE && cptr->Phase != PAR_SLP) {
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = PAR_DIE;
		ActivateCharacterFx(cptr);
	}
	else {
		ProcessPrevPhase(*cptr);

		cptr->FTime += TimeDt;
		if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if (Tranq) {
				cptr->FTime = 0;
				cptr->Phase = PAR_SLP;
				ActivateCharacterFx(cptr);
			}
			else
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}
























void AnimateRaptor(TCharacter* cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = std::sqrtf(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 100 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 100 * cptr->scale;
	float pdist = std::sqrtf(playerdx * playerdx + playerdz * playerdz);
	if (cptr->State == 2) { if (cptr->Phase != RAP_JUMP) NewPhase = TRUE; cptr->State = 1; }


	if (GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) > 180 * cptr->scale)
		cptr->StateF |= csONWATER; else
		cptr->StateF &= (!csONWATER);

	if (cptr->Phase == RAP_EAT) goto NOTHINK;

	//============================================//   
	if (!MyHealth) cptr->State = 0;
	if (cptr->State) {
		if (pdist > (16 + OptAgres * 6) * 256) {
			nv.x = playerdx; nv.z = playerdz; nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
			cptr->AfraidTime -= TimeDt;
			if (cptr->AfraidTime <= 0) {
				cptr->AfraidTime = 0; cptr->State = 0;
			}

		}
		else {
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}

		if (!(cptr->StateF & csONWATER))
			if (pdist < 1324 * cptr->scale && pdist>900 * cptr->scale)
				if (AngleDifference(cptr->alpha, FindVectorAlpha(playerdx, playerdz)) < 0.2f)
					cptr->Phase = RAP_JUMP;

		if (pdist < 256)
			if (fabs(PlayerY - cptr->pos.y - 160) < 256) {
				if (!(cptr->StateF & csONWATER)) {
					cptr->vspeed /= 8.0f;
					cptr->State = 1;
					cptr->Phase = RAP_EAT;
				}
				AddDeadBody(cptr, HUNT_EAT);
			}
	}

	if (!cptr->State) {
		if (tdist < 456) {
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}

NOTHINK:
	if (pdist < 2048) cptr->NoFindCnt = 0;
	if (cptr->NoFindCnt) cptr->NoFindCnt--; else
	{
		cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
		if (cptr->State && pdist > 1648) {
			cptr->tgalpha += (float)sin(RealTime / 824.f) / 2.f;
			if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
			if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;
		}
	}

	LookForAWay(cptr, FALSE, TRUE);
	if (cptr->NoWayCnt > 12) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 16 + rRand(20); }


	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);


	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (cptr->Phase == RAP_EAT)  goto ENDPSELECT;
	if (NewPhase && _Phase == RAP_JUMP) { cptr->Phase = RAP_RUN; goto ENDPSELECT; }


	if (cptr->Phase == RAP_JUMP) goto ENDPSELECT;

	if (!cptr->State) cptr->Phase = RAP_WALK; else
		if (fabs(cptr->tgalpha - cptr->alpha) < 1.0 ||
			fabs(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0)
			cptr->Phase = RAP_RUN; else cptr->Phase = RAP_WALK;

	if (cptr->StateF & csONWATER) cptr->Phase = RAP_SWIM;
	if (cptr->Slide > 40) cptr->Phase = RAP_SLIDE;


ENDPSELECT:

	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		//==== set proportional FTime for better morphing =//       
		if (MORPHP)
			if (_Phase <= 3 && cptr->Phase <= 3)
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = std::fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;

	if (cptr->Phase == RAP_JUMP || cptr->Phase == RAP_EAT) goto SKIPROT;

	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.6f + drspd * 1.2f;
		else currspeed = -0.6f - drspd * 1.2f;
	else currspeed = 0;
	if (cptr->AfraidTime) currspeed *= 2.5;

	if (dalpha > pi) currspeed *= -1;
	if ((cptr->StateF & csONWATER) || cptr->Phase == RAP_WALK) currspeed /= 1.4f;

	if (cptr->AfraidTime) DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 160.f);
	else DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 180.f);

	tgbend = drspd / 2;
	if (tgbend > pi / 5) tgbend = pi / 5;

	tgbend *= SGN(currspeed);
	if (std::fabs(tgbend) > std::fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 600.f);


	rspd = cptr->rspeed * TimeDt / 1024.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//======= set slide mode ===========//
	if (!cptr->Slide && cptr->vspeed > 0.6 && cptr->Phase != RAP_JUMP)
		if (AngleDifference(cptr->tgalpha, cptr->alpha) > pi / 2.f) {
			cptr->Slide = (int)(cptr->vspeed * 700.f);
			cptr->slidex = cptr->lookx;
			cptr->slidez = cptr->lookz;
			cptr->vspeed = 0;
		}



	//========== movement ==============================//
	cptr->lookx = std::cosf(cptr->alpha);
	cptr->lookz = std::sinf(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == RAP_RUN) curspeed = 1.2f;
	if (cptr->Phase == RAP_JUMP) curspeed = 1.1f;
	if (cptr->Phase == RAP_WALK) curspeed = 0.428f;
	if (cptr->Phase == RAP_SWIM) curspeed = 0.4f;
	if (cptr->Phase == RAP_EAT) curspeed = 0.0f;

	if (cptr->Phase == RAP_RUN && cptr->Slide) {
		curspeed /= 8;
		if (drspd > pi / 2.f) curspeed = 0; else
			if (drspd > pi / 4.f) curspeed *= 2.f - 4.f * drspd / pi;
	}
	else
		if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 500.f);

	if (cptr->Phase == RAP_JUMP) cptr->vspeed = 1.1f;

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
		cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, FALSE, TRUE);


	//========== slide ==============//
	if (cptr->Slide) {
		MoveCharacter(cptr, cptr->slidex * cptr->Slide / 600.f * TimeDt * cptr->scale,
			cptr->slidez * cptr->Slide / 600.f * TimeDt * cptr->scale, FALSE, TRUE);

		cptr->Slide -= TimeDt;
		if (cptr->Slide < 0) cptr->Slide = 0;
	}


	//============ Y movement =================//   
	if (cptr->StateF & csONWATER) {
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 200 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}
	else {
		ThinkY_Beta_Gamma(cptr, 48, 24, 0.5f, 0.4f);
	}

	//=== process to tggamma ===//   
	if (cptr->Phase == RAP_WALK) cptr->tggamma += cptr->rspeed / 10.0f;
	else cptr->tggamma += cptr->rspeed / 8.0f;
	if (cptr->Phase == RAP_JUMP) cptr->tggamma = 0;

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1624.f);


	//==================================================//   

}











void AnimateVelo(TCharacter* cptr)
{
	NewPhase = FALSE;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 108;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 108;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);
	if (cptr->State == 2) { if (cptr->Phase != RAP_JUMP) NewPhase = TRUE; cptr->State = 1; }


	if (GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) > 140 * cptr->scale)
		cptr->StateF |= csONWATER; else
		cptr->StateF &= (!csONWATER);

	if (cptr->Phase == VEL_EAT) goto NOTHINK;

	//============================================//   
	if (!MyHealth) cptr->State = 0;
	if (cptr->State) {
		if (pdist > (18 + OptAgres * 10) * 256) {
			nv.x = playerdx; nv.z = playerdz; nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
			cptr->AfraidTime -= TimeDt;
			if (cptr->AfraidTime <= 0) {
				cptr->AfraidTime = 0; cptr->State = 0;
			}
		}
		else {
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}

		if (!(cptr->StateF & csONWATER))
			if (pdist < 1324 * cptr->scale && pdist>900 * cptr->scale)
				if (AngleDifference(cptr->alpha, FindVectorAlpha(playerdx, playerdz)) < 0.2f)
					cptr->Phase = VEL_JUMP;

		if (pdist < 256)
			if (std::fabs(PlayerY - cptr->pos.y - 120) < 256) {
				if (!(cptr->StateF & csONWATER)) {
					cptr->vspeed /= 8.0f;
					cptr->State = 1;
					cptr->Phase = VEL_EAT;
				}

				AddDeadBody(cptr, HUNT_EAT);
			}
	}

	if (!cptr->State) {
		cptr->AfraidTime = 0;
		if (tdist < 456) {
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}

NOTHINK:
	if (pdist < 2048) cptr->NoFindCnt = 0;
	if (cptr->NoFindCnt) cptr->NoFindCnt--; else
	{
		cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
		if (cptr->State && pdist > 1648) {
			cptr->tgalpha += (float)sin(RealTime / 824.f) / 2.f;
			if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
			if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;
		}
	}

	LookForAWay(cptr, FALSE, TRUE);
	if (cptr->NoWayCnt > 12) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 16 + rRand(20); }


	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);


	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (cptr->Phase == VEL_EAT)  goto ENDPSELECT;
	if (NewPhase && _Phase == VEL_JUMP) { cptr->Phase = VEL_RUN; goto ENDPSELECT; }


	if (cptr->Phase == VEL_JUMP) goto ENDPSELECT;

	if (!cptr->State) cptr->Phase = VEL_WALK; else
		if (std::fabs(cptr->tgalpha - cptr->alpha) < 1.0 ||
			std::fabs(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0)
			cptr->Phase = VEL_RUN; else cptr->Phase = VEL_WALK;

	if (cptr->StateF & csONWATER) cptr->Phase = VEL_SWIM;
	if (cptr->Slide > 40) cptr->Phase = VEL_SLIDE;


ENDPSELECT:

	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		//==== set proportional FTime for better morphing =//       
		if (MORPHP)
			if (_Phase <= 3 && cptr->Phase <= 3)
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = std::fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;

	if (cptr->Phase == VEL_JUMP || cptr->Phase == VEL_EAT) goto SKIPROT;

	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.6f + drspd * 1.2f;
		else currspeed = -0.6f - drspd * 1.2f;
	else currspeed = 0;
	if (cptr->AfraidTime) currspeed *= 2.5;

	if (dalpha > pi) currspeed *= -1;
	if ((cptr->StateF & csONWATER) || cptr->Phase == VEL_WALK) currspeed /= 1.4f;

	if (cptr->AfraidTime) DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 160.f);
	else DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 180.f);

	tgbend = drspd / 3;
	if (tgbend > pi / 5) tgbend = pi / 5;

	tgbend *= SGN(currspeed);
	if (std::fabs(tgbend) > std::fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 600.f);


	rspd = cptr->rspeed * TimeDt / 1024.f;
	if (drspd < std::fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//======= set slide mode ===========//
	if (!cptr->Slide && cptr->vspeed > 0.6 && cptr->Phase != VEL_JUMP)
		if (AngleDifference(cptr->tgalpha, cptr->alpha) > pi / 2.f) {
			cptr->Slide = (int)(cptr->vspeed * 700.f);
			cptr->slidex = cptr->lookx;
			cptr->slidez = cptr->lookz;
			cptr->vspeed = 0;
		}



	//========== movement ==============================//
	cptr->lookx = std::cosf(cptr->alpha);
	cptr->lookz = std::sinf(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == VEL_RUN) curspeed = 1.2f;
	if (cptr->Phase == VEL_JUMP) curspeed = 1.1f;
	if (cptr->Phase == VEL_WALK) curspeed = 0.428f;
	if (cptr->Phase == VEL_SWIM) curspeed = 0.4f;
	if (cptr->Phase == VEL_EAT) curspeed = 0.0f;

	if (cptr->Phase == VEL_RUN && cptr->Slide) {
		curspeed /= 8;
		if (drspd > pi / 2.f) curspeed = 0; else
			if (drspd > pi / 4.f) curspeed *= 2.f - 4.f * drspd / pi;
	}
	else
		if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 500.f);

	if (cptr->Phase == VEL_JUMP) cptr->vspeed = 1.1f;

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
		cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, FALSE, TRUE);


	//========== slide ==============//
	if (cptr->Slide) {
		MoveCharacter(cptr, cptr->slidex * cptr->Slide / 600.f * TimeDt * cptr->scale,
			cptr->slidez * cptr->Slide / 600.f * TimeDt * cptr->scale, FALSE, TRUE);

		cptr->Slide -= TimeDt;
		if (cptr->Slide < 0) cptr->Slide = 0;
	}


	//============ Y movement =================//   
	if (cptr->StateF & csONWATER) {
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 160 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}
	else {
		ThinkY_Beta_Gamma(cptr, 48, 24, 0.5f, 0.4f);
	}

	//=== process to tggamma ===//   
	if (cptr->Phase == VEL_WALK) cptr->tggamma += cptr->rspeed / 7.0f;
	else cptr->tggamma += cptr->rspeed / 5.0f;
	if (cptr->Phase == VEL_JUMP) cptr->tggamma = 0;

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1624.f);


	//==================================================//   

}









void AnimateTRex(TCharacter* cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	bool LookMode = false;



TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = std::sqrtf(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 108;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 108;
	float pdist = std::sqrtf(playerdx * playerdx + playerdz * playerdz);
	float palpha = FindVectorAlpha(playerdx, playerdz);
	//if (cptr->State==2) { NewPhase=TRUE; cptr->State=1; }      
	if (cptr->State == 5) {
		NewPhase = true; cptr->State = 1; cptr->Phase = REX_WALK; cptr->FTime = 0;
		cptr->tgx = PlayerX;
		cptr->tgz = PlayerZ;
		goto TBEGIN;
	}

	if (GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) > 560 * cptr->scale)
		cptr->StateF |= csONWATER; else
		cptr->StateF &= (!csONWATER);

	if (cptr->Phase == REX_EAT) goto NOTHINK;

	//============================================//   
	if (!MyHealth) cptr->State = 0;
	if (cptr->State) {
		cptr->tgx = PlayerX;
		cptr->tgz = PlayerZ;
		cptr->tgtime = 0;
		if (cptr->State > 1)
			if (AngleDifference(cptr->alpha, palpha) < 0.4f) {
				if (cptr->State == 2) cptr->Phase = REX_SEE1 + rRand(1);
				else cptr->Phase = REX_SMEL + rRand(1);
				cptr->State = 1;
				cptr->rspeed = 0;
			}

		if (pdist < 256)
			if (std::fabs(PlayerY - cptr->pos.y) < 256) {
				cptr->vspeed /= 8.0f;
				cptr->State = 1;
				cptr->Phase = REX_EAT;
				AddDeadBody(cptr, HUNT_KILL);
				Characters[ChCount - 1].scale = cptr->scale;
				Characters[ChCount - 1].alpha = cptr->alpha;
				cptr->bend = 0;
				DemoPoint.CIndex = CurDino;
			}
	}

	if (!cptr->State)
		if (tdist < 1224) {
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}


NOTHINK:
	if (pdist < 2048) cptr->NoFindCnt = 0;
	if (cptr->NoFindCnt) cptr->NoFindCnt--; else
	{
		cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
		if (cptr->State && pdist > 5648) {
			cptr->tgalpha += std::sinf(RealTime / 824.f) / 6.f;
			if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
			if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;
		}
	}

	LookForAWay(cptr, FALSE, !cptr->State);
	if (cptr->NoWayCnt > 12) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 16 + rRand(20); }


	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);


	//======== select new phase =======================//
	if (cptr->Phase == REX_SEE || cptr->Phase == REX_SEE1 ||
		cptr->Phase == REX_SMEL || cptr->Phase == REX_SMEL1)   LookMode = TRUE;

	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (cptr->Phase == REX_EAT)    goto ENDPSELECT;

	if (!NewPhase)
		if (cptr->Phase == REX_SCREAM) goto ENDPSELECT;

	if (!cptr->State)
		if (NewPhase)
			if (rRand(128) > 110) {
				if (rRand(128) > 64) cptr->Phase = REX_SEE1 + rRand(1);
				else cptr->Phase = REX_SMEL + rRand(1);
				goto ENDPSELECT;
			}


	if (!NewPhase) if (LookMode) goto ENDPSELECT;

	if (cptr->State)
		if (NewPhase && LookMode) {
			cptr->Phase = REX_SCREAM;
			goto ENDPSELECT;
		}

	if (!cptr->State || cptr->State > 1) cptr->Phase = REX_WALK; else
		if (fabs(cptr->tgalpha - cptr->alpha) < 1.0 ||
			fabs(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0)
			cptr->Phase = REX_RUN; else cptr->Phase = REX_WALK;

	if (cptr->StateF & csONWATER) cptr->Phase = REX_SWIM;

ENDPSELECT:

	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		//==== set proportional FTime for better morphing =//       
		if (MORPHP)
			if (_Phase <= 1 && cptr->Phase <= 1)
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;

	if (cptr->Phase == REX_SCREAM || cptr->Phase == REX_EAT) goto SKIPROT;
	if (LookMode) goto SKIPROT;

	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.7f + drspd * 1.4f;
		else currspeed = -0.7f - drspd * 1.4f;
	else currspeed = 0;
	if (cptr->AfraidTime) currspeed *= 2.5;

	if (dalpha > pi) currspeed *= -1;

	if (cptr->State) DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 440.f);
	else DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 620.f);

	tgbend = drspd / 2;
	if (tgbend > pi / 6.f) tgbend = pi / 6.f;

	tgbend *= SGN(currspeed);
	DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1800.f);




	rspd = cptr->rspeed * TimeDt / 1024.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == REX_RUN) curspeed = 2.49f;
	if (cptr->Phase == REX_WALK) curspeed = 0.76f;
	if (cptr->Phase == REX_SWIM) curspeed = 0.70f;

	if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 200.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
		cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, FALSE, TRUE);

	//============ Y movement =================//      
	if (cptr->StateF & csONWATER) {
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 540 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}
	else {
		ThinkY_Beta_Gamma(cptr, 348, 324, 0.5f, 0.4f);
	}



	//=== process to tggamma ===//   
	if (cptr->Phase == REX_WALK) cptr->tggamma += cptr->rspeed / 16.0f;
	else cptr->tggamma += cptr->rspeed / 12.0f;

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2024.f);


	//==================================================//   

}














void AnimateMosh(TCharacter* cptr)
{
	NewPhase = FALSE;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	if (cptr->AfraidTime) cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	if (cptr->State == 2) { NewPhase = TRUE; cptr->State = 1; }

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if (cptr->State) {
		if (!cptr->AfraidTime) {
			if (pdist < 2048.f) cptr->AfraidTime = (5 + rRand(5)) * 1024;

			if (!cptr->AfraidTime)
				if (pdist > 4096.f) {
					cptr->State = 0;
					SetNewTargetPlace(cptr, 2048.f);
					goto TBEGIN;
				}
		}


		nv.x = playerdx; nv.z = playerdz; nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}

	if (pdist > 13240)
		if (ReplaceCharacterForward(cptr)) goto TBEGIN;


	//======== exploring area ===============//
	if (!cptr->State) {
		cptr->AfraidTime = 0;
		if (pdist < 812.f) {
			cptr->State = 1;
			cptr->AfraidTime = (5 + rRand(5)) * 1024;
			cptr->Phase = MOS_RUN;
			goto TBEGIN;
		}


		if (tdist < 456) {
			SetNewTargetPlace(cptr, 2048.f);
			goto TBEGIN;
		}
	}


	//============================================//        

	if (cptr->NoFindCnt) cptr->NoFindCnt--;
	else cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
	LookForAWay(cptr, TRUE, TRUE);
	if (cptr->NoWayCnt > 8) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 8 + rRand(80); }

	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (NewPhase)
		if (!cptr->State) {
			if (cptr->Phase == MOS_IDLE1 || cptr->Phase == MOS_IDLE2) {
				if (rRand(128) > 76 && cptr->Phase == MOS_IDLE2)
					cptr->Phase = MOS_WALK;
				else cptr->Phase = MOS_IDLE1 + rRand(3) / 3;
				goto ENDPSELECT;
			}
			if (rRand(128) > 120) cptr->Phase = MOS_IDLE1; else cptr->Phase = MOS_WALK;
		}
		else
			if (cptr->AfraidTime) cptr->Phase = MOS_RUN;
			else cptr->Phase = MOS_WALK;

ENDPSELECT:

	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		if (_Phase <= 1 && cptr->Phase <= 1)
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;


	if (cptr->Phase == MOS_IDLE1 || cptr->Phase == MOS_IDLE2) goto SKIPROT;
	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.8f + drspd * 1.4f;
		else currspeed = -0.8f - drspd * 1.4f;
	else currspeed = 0;

	if (cptr->AfraidTime) currspeed *= 1.5;
	if (dalpha > pi) currspeed *= -1;
	if ((cptr->State & csONWATER) || cptr->Phase == MOS_WALK) currspeed /= 1.4f;

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 260.f);

	tgbend = drspd / 2;
	if (tgbend > pi / 2) tgbend = pi / 2;

	tgbend *= SGN(currspeed);
	if (fabs(tgbend) > fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);


	rspd = cptr->rspeed * TimeDt / 1024.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == MOS_RUN) curspeed = 0.6f;
	if (cptr->Phase == MOS_WALK) curspeed = 0.3f;

	if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
		cptr->lookz * cptr->vspeed * TimeDt, TRUE, TRUE);

	ThinkY_Beta_Gamma(cptr, 64, 32, 0.7f, 0.4f);
	if (cptr->Phase == MOS_WALK) cptr->tggamma += cptr->rspeed / 12.0f;
	else cptr->tggamma += cptr->rspeed / 8.0f;
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}











void AnimateTric(TCharacter* cptr)
{
	NewPhase = FALSE;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	if (cptr->AfraidTime) cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	if (cptr->State == 2) { NewPhase = TRUE; cptr->State = 1; }

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;

	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if (cptr->State) {
		if (pdist < 6000) cptr->AfraidTime = 8000;

		if (!cptr->AfraidTime) {
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if (pdist > (10 + OptAgres * 6) * 256) {
			nv.x = playerdx; nv.z = playerdz; nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}
		else {
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if (MyHealth)
		if (pdist < 300)
			if (fabs(PlayerY - cptr->pos.y - 160) < 256) {
				cptr->State = 0;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if (!cptr->State) {
		cptr->AfraidTime = 0;

		if (tdist < 456) {
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//   

	if (cptr->NoFindCnt) cptr->NoFindCnt--;
	else {
		cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
		if (cptr->AfraidTime) {
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;
			if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
			if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;
		}
	}


	LookForAWay(cptr, TRUE, TRUE);
	if (cptr->NoWayCnt > 8) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 48 + rRand(80); }

	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (NewPhase)
		if (!cptr->State) {
			if (cptr->Phase == TRI_IDLE1 || cptr->Phase == TRI_IDLE2 || cptr->Phase == TRI_IDLE3) {
				if (rRand(128) > 64 && cptr->Phase == TRI_IDLE3)
					cptr->Phase = TRI_WALK;
				else cptr->Phase = TRI_IDLE1 + rRand(2);
				goto ENDPSELECT;
			}
			if (rRand(128) > 124) cptr->Phase = TRI_IDLE1; else cptr->Phase = TRI_WALK;
		}
		else
			if (cptr->AfraidTime) cptr->Phase = TRI_RUN;
			else cptr->Phase = TRI_WALK;

ENDPSELECT:

	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		if (_Phase <= 1 && cptr->Phase <= 1)
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;


	if (cptr->Phase == TRI_IDLE1 || cptr->Phase == TRI_IDLE2 || cptr->Phase == TRI_IDLE3) goto SKIPROT;
	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.2f + drspd * 1.0f;
		else currspeed = -0.2f - drspd * 1.0f;
	else currspeed = 0;

	if (cptr->AfraidTime) currspeed *= 1.5;
	if (dalpha > pi) currspeed *= -1;
	if ((cptr->State & csONWATER) || cptr->Phase == TRI_WALK) currspeed /= 1.4f;

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;
	if (tgbend > pi / 2.f) tgbend = pi / 2.f;

	tgbend *= SGN(currspeed);
	if (fabs(tgbend) > fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);


	rspd = cptr->rspeed * TimeDt / 612.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == TRI_RUN) curspeed = 1.2f;
	if (cptr->Phase == TRI_WALK) curspeed = 0.30f;

	if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//
	curspeed *= cptr->scale;
	if (curspeed > cptr->vspeed) DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	else DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
		cptr->lookz * cptr->vspeed * TimeDt, TRUE, TRUE);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);
	if (cptr->Phase == MOS_WALK) cptr->tggamma += cptr->rspeed / 12.0f;
	else cptr->tggamma += cptr->rspeed / 8.0f;
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}






void AnimatePac(TCharacter* cptr)
{
	NewPhase = FALSE;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	if (cptr->AfraidTime) cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	if (cptr->State == 2) { NewPhase = TRUE; cptr->State = 1; }

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if (cptr->State) {

		if (!cptr->AfraidTime) {
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx; nv.z = playerdz; nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if (!cptr->State) {
		cptr->AfraidTime = 0;
		if (pdist < 1024.f) {
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = PAC_RUN;
			goto TBEGIN;
		}


		if (tdist < 456) {
			SetNewTargetPlace(cptr, 6048.f);
			goto TBEGIN;
		}
	}


	//============================================//   

	if (cptr->NoFindCnt) cptr->NoFindCnt--;
	else {
		cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
		if (cptr->AfraidTime) {
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;
			if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
			if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;
		}
	}


	LookForAWay(cptr, TRUE, TRUE);
	if (cptr->NoWayCnt > 12) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 32 + rRand(60); }

	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (NewPhase)
		if (!cptr->State) {
			if (cptr->Phase == PAC_IDLE1 || cptr->Phase == PAC_IDLE2) {
				if (rRand(128) > 64 && cptr->Phase == PAC_IDLE2)
					cptr->Phase = PAC_WALK;
				else cptr->Phase = PAC_IDLE1 + rRand(1);
				goto ENDPSELECT;
			}
			if (rRand(128) > 120) cptr->Phase = PAC_IDLE1; else cptr->Phase = PAC_WALK;
		}
		else
			if (cptr->AfraidTime) cptr->Phase = PAC_RUN;
			else cptr->Phase = PAC_WALK;

ENDPSELECT:

	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		if (_Phase <= 2 && cptr->Phase <= 2)
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;


	if (cptr->Phase == PAC_IDLE1 || cptr->Phase == PAC_IDLE2) goto SKIPROT;
	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.2f + drspd * 1.0f;
		else currspeed = -0.2f - drspd * 1.0f;
	else currspeed = 0;

	if (cptr->AfraidTime) currspeed *= 1.5;
	if (dalpha > pi) currspeed *= -1;
	if ((cptr->State & csONWATER) || cptr->Phase == PAC_WALK) currspeed /= 1.4f;

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.0f;
	if (tgbend > pi / 2.f) tgbend = pi / 2.f;

	tgbend *= SGN(currspeed);
	if (fabs(tgbend) > fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);


	rspd = cptr->rspeed * TimeDt / 612.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == PAC_RUN) curspeed = 1.6f;
	if (cptr->Phase == PAC_WALK) curspeed = 0.40f;

	if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//
	curspeed *= cptr->scale;
	if (curspeed > cptr->vspeed) DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	else DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
		cptr->lookz * cptr->vspeed * TimeDt, TRUE, TRUE);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);
	if (cptr->Phase == PAC_WALK) cptr->tggamma += cptr->rspeed / 12.0f;
	else cptr->tggamma += cptr->rspeed / 8.0f;
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}




void AnimateSteg(TCharacter* cptr)
{
	NewPhase = FALSE;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	if (cptr->AfraidTime) cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	if (cptr->State == 2) { NewPhase = TRUE; cptr->State = 1; }

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if (cptr->State) {

		if (!cptr->AfraidTime) {
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx; nv.z = playerdz; nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if (!cptr->State) {
		cptr->AfraidTime = 0;
		if (pdist < 1024.f) {
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = STG_RUN;
			goto TBEGIN;
		}


		if (tdist < 456) {
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//   

	if (cptr->NoFindCnt) cptr->NoFindCnt--;
	else {
		cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
		if (cptr->AfraidTime) {
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;
			if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
			if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;
		}
	}


	LookForAWay(cptr, TRUE, TRUE);
	if (cptr->NoWayCnt > 12) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 32 + rRand(60); }

	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (NewPhase)
		if (!cptr->State) {
			if (cptr->Phase == STG_IDLE1 || cptr->Phase == STG_IDLE2) {
				if (rRand(128) > 64 && cptr->Phase == STG_IDLE2)
					cptr->Phase = STG_WALK;
				else cptr->Phase = STG_IDLE1 + rRand(1);
				goto ENDPSELECT;
			}
			if (rRand(128) > 120) cptr->Phase = STG_IDLE1; else cptr->Phase = STG_WALK;
		}
		else
			if (cptr->AfraidTime) cptr->Phase = STG_RUN;
			else cptr->Phase = STG_WALK;

ENDPSELECT:

	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		if (_Phase <= 2 && cptr->Phase <= 2)
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;


	if (cptr->Phase == STG_IDLE1 || cptr->Phase == STG_IDLE2) goto SKIPROT;
	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.2f + drspd * 1.0f;
		else currspeed = -0.2f - drspd * 1.0f;
	else currspeed = 0;

	if (cptr->AfraidTime) currspeed *= 1.5;
	if (dalpha > pi) currspeed *= -1;
	if ((cptr->State & csONWATER) || cptr->Phase == STG_WALK) currspeed /= 1.4f;

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 2.f;
	if (tgbend > pi / 3.f) tgbend = pi / 3.f;

	tgbend *= SGN(currspeed);
	DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 2000.f);



	rspd = cptr->rspeed * TimeDt / 612.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == STG_RUN) curspeed = 0.96f;
	if (cptr->Phase == STG_WALK) curspeed = 0.36f;

	if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//
	curspeed *= cptr->scale;
	if (curspeed > cptr->vspeed) DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	else DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
		cptr->lookz * cptr->vspeed * TimeDt, TRUE, TRUE);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);
	if (cptr->Phase == STG_WALK) cptr->tggamma += cptr->rspeed / 16.0f;
	else cptr->tggamma += cptr->rspeed / 10.0f;
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}




void AnimatePar(TCharacter* cptr)
{
	NewPhase = FALSE;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	if (cptr->AfraidTime) cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	if (cptr->State == 2) { NewPhase = TRUE; cptr->State = 1; }

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if (cptr->State) {

		if (!cptr->AfraidTime) {
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx; nv.z = playerdz; nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if (!cptr->State) {
		cptr->AfraidTime = 0;
		if (pdist < 1024.f) {
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = PAR_RUN;
			goto TBEGIN;
		}


		if (tdist < 456) {
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//   

	if (cptr->NoFindCnt) cptr->NoFindCnt--;
	else {
		cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
		if (cptr->AfraidTime) {
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;
			if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
			if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;
		}
	}


	LookForAWay(cptr, TRUE, TRUE);
	if (cptr->NoWayCnt > 8) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 44 + rRand(80); }

	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (NewPhase)
		if (!cptr->State) {
			if (cptr->Phase == PAR_IDLE1 || cptr->Phase == PAR_IDLE2) {
				if (rRand(128) > 64 && cptr->Phase == PAR_IDLE2)
					cptr->Phase = PAR_WALK;
				else cptr->Phase = PAR_IDLE1 + rRand(1);
				goto ENDPSELECT;
			}
			if (rRand(128) > 120) cptr->Phase = PAR_IDLE1; else cptr->Phase = PAR_WALK;
		}
		else
			if (cptr->AfraidTime) cptr->Phase = PAR_RUN;
			else cptr->Phase = PAR_WALK;

ENDPSELECT:

	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		if (_Phase <= 2 && cptr->Phase <= 2)
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;


	if (cptr->Phase == PAR_IDLE1 || cptr->Phase == PAR_IDLE2) goto SKIPROT;
	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.2f + drspd * 1.0f;
		else currspeed = -0.2f - drspd * 1.0f;
	else currspeed = 0;

	if (cptr->AfraidTime) currspeed *= 1.5;
	if (dalpha > pi) currspeed *= -1;
	if ((cptr->State & csONWATER) || cptr->Phase == PAR_WALK) currspeed /= 1.4f;

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.0f;
	if (tgbend > pi / 2.f) tgbend = pi / 2.f;

	tgbend *= SGN(currspeed);
	if (fabs(tgbend) > fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);


	rspd = cptr->rspeed * TimeDt / 612.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == PAR_RUN) curspeed = 1.6f;
	if (cptr->Phase == PAR_WALK) curspeed = 0.40f;

	if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//
	curspeed *= cptr->scale;
	if (curspeed > cptr->vspeed) DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	else DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
		cptr->lookz * cptr->vspeed * TimeDt, TRUE, TRUE);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);
	if (cptr->Phase == PAR_WALK) cptr->tggamma += cptr->rspeed / 12.0f;
	else cptr->tggamma += cptr->rspeed / 8.0f;
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}







void AnimateGall(TCharacter* cptr)
{
	NewPhase = FALSE;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	if (cptr->AfraidTime) cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	if (cptr->State == 2) { NewPhase = TRUE; cptr->State = 1; }

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if (cptr->State) {
		if (!cptr->AfraidTime) {
			if (pdist < 2048.f) { cptr->State = 1;  cptr->AfraidTime = (5 + rRand(5)) * 1024; }
			if (pdist > 4096.f) {
				cptr->State = 0;
				SetNewTargetPlace(cptr, 2048.f);
				goto TBEGIN;
			}
		}

		nv.x = playerdx; nv.z = playerdz; nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}

	if (pdist > 13240)
		if (ReplaceCharacterForward(cptr)) goto TBEGIN;



	//======== exploring area ===============//
	if (!cptr->State) {
		cptr->AfraidTime = 0;
		if (pdist < 812.f) {
			cptr->State = 1;
			cptr->AfraidTime = (5 + rRand(5)) * 1024;
			cptr->Phase = MOS_RUN;
			goto TBEGIN;
		}

		if (tdist < 456) {
			SetNewTargetPlace(cptr, 2048.f);
			goto TBEGIN;
		}
	}


	//============================================//   

	if (cptr->NoFindCnt > 0) cptr->NoFindCnt--;
	else cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
	LookForAWay(cptr, TRUE, TRUE);
	if (cptr->NoWayCnt > 8) { cptr->NoWayCnt = 0; cptr->NoFindCnt = 8 + rRand(40); }

	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (NewPhase)
		if (!cptr->State) {
			if (cptr->Phase == GAL_IDLE1 || cptr->Phase == GAL_IDLE2) {
				if (rRand(128) > 76 && cptr->Phase == GAL_IDLE2)
					cptr->Phase = GAL_WALK;
				else cptr->Phase = GAL_IDLE1 + rRand(3) / 3;
				goto ENDPSELECT;
			}
			if (rRand(128) > 120) cptr->Phase = GAL_IDLE1; else cptr->Phase = GAL_WALK;
		}
		else
			if (cptr->AfraidTime) cptr->Phase = GAL_RUN;
			else cptr->Phase = GAL_WALK;

ENDPSELECT:

	//====== process phase changing ===========//

	if ((_Phase != cptr->Phase) || NewPhase)
		ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		if (_Phase <= 2 && cptr->Phase <= 2)
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		else if (!NewPhase) cptr->FTime = 0;

		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;

	if (cptr->Phase == GAL_IDLE1 || cptr->Phase == GAL_IDLE2) goto SKIPROT;
	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.8f + drspd * 1.4f;
		else currspeed = -0.8f - drspd * 1.4f;
	else currspeed = 0;
	if (cptr->AfraidTime) currspeed *= 1.5;

	if (dalpha > pi) currspeed *= -1;
	if ((cptr->State & csONWATER) || cptr->Phase == GAL_WALK) currspeed /= 1.4f;

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 260.f);

	tgbend = drspd / 3;
	if (tgbend > pi / 2) tgbend = pi / 2;

	tgbend *= SGN(currspeed);
	if (fabs(tgbend) > fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);


	rspd = cptr->rspeed * TimeDt / 1024.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == GAL_RUN) curspeed = 0.9f;
	if (cptr->Phase == GAL_WALK) curspeed = 0.36f;

	if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
		cptr->lookz * cptr->vspeed * TimeDt, TRUE, TRUE);

	ThinkY_Beta_Gamma(cptr, 64, 32, 0.7f, 0.4f);
	if (cptr->Phase == GAL_WALK) cptr->tggamma += cptr->rspeed / 12.0f;
	else cptr->tggamma += cptr->rspeed / 8.0f;
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}






void AnimateDimor(TCharacter* cptr)
{
	NewPhase = FALSE;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//   

	if (pdist > 13240)
		if (ReplaceCharacterForward(cptr)) goto TBEGIN;


	//======== exploring area ===============//   
	if (tdist < 1024) {
		SetNewTargetPlace(cptr, 4048.f);
		goto TBEGIN;
	}


	//============================================//        


	cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);
	if (cptr->tgalpha < 0) cptr->tgalpha += 2 * pi;
	if (cptr->tgalpha > 2 * pi) cptr->tgalpha -= 2 * pi;

	//===============================================//

	ProcessPrevPhase(*cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime) {
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = TRUE;
	}

	if (NewPhase) {
		if (cptr->Phase == DIM_FLY)
			if (cptr->pos.y > GetLandH(cptr->pos.x, cptr->pos.z) + 2800)
				cptr->Phase = DIM_FLYP;
			else;
		else
			if (cptr->Phase == DIM_FLYP)
				if (cptr->pos.y < GetLandH(cptr->pos.x, cptr->pos.z) + 1800)
					cptr->Phase = DIM_FLY;
	}




	//====== process phase changing ===========//
	if ((_Phase != cptr->Phase) || NewPhase)
		if ((rand() & 1023) > 980)
			ActivateCharacterFx(cptr);

	if (_Phase != cptr->Phase) {
		if (!NewPhase) cptr->FTime = 0;
		if (cptr->PPMorphTime > 128) {
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime = _FTime;
			cptr->PPMorphTime = 0;
		}
	}


	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;


	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha; if (drspd > pi) drspd = 2 * pi - drspd;



	if (drspd > 0.02)
		if (cptr->tgalpha > cptr->alpha) currspeed = 0.6f + drspd * 1.2f;
		else currspeed = -0.6f - drspd * 1.2f;
	else currspeed = 0;

	if (dalpha > pi) currspeed *= -1;
	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 460.f);

	tgbend = drspd / 2.f;
	if (tgbend > pi / 2) tgbend = pi / 2;

	tgbend *= SGN(currspeed);
	if (fabs(tgbend) > fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);


	rspd = cptr->rspeed * TimeDt / 1024.f;
	if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
	else cptr->alpha += rspd;


	if (cptr->alpha > pi * 2) cptr->alpha -= pi * 2;
	if (cptr->alpha < 0) cptr->alpha += pi * 2;

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;
	if (cptr->Phase == DIM_FLY) curspeed = 1.5f;
	if (cptr->Phase == DIM_FLYP) curspeed = 1.3f;

	if (drspd > pi / 2.f) curspeed *= 2.f - 2.f * drspd / pi;

	if (cptr->Phase == DIM_FLY)
		DeltaFunc(cptr->pos.y, GetLandH(cptr->pos.x, cptr->pos.z) + 4048, TimeDt / 6.f);
	else
		DeltaFunc(cptr->pos.y, GetLandH(cptr->pos.x, cptr->pos.z), TimeDt / 16.f);

	if (cptr->pos.y < GetLandH(cptr->pos.x, cptr->pos.z) + 236)
		cptr->pos.y = GetLandH(cptr->pos.x, cptr->pos.z) + 256;


	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 2024.f);

	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	cptr->tggamma = cptr->rspeed / 4.0f;
	if (cptr->tggamma > pi / 6.f) cptr->tggamma = pi / 6.f;
	if (cptr->tggamma < -pi / 6.f) cptr->tggamma = -pi / 6.f;
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}





void AnimateCharacters()
{
	if (TrophyMode)
		return;

	for (CurDino = 0; CurDino < ChCount; CurDino++) {
		TCharacter& cref = Characters[CurDino];
		if (cref.StateF == 0xFF) continue;
		cref.tgtime += TimeDt;

		if (cref.tgtime > 30 * 1000) SetNewTargetPlace(&cref, 2048);

		switch (cref.CType) {
		case CTYPE_HUNTER: AnimateHuntDead(&cref);
			break;
			
		case CTYPE_MOSCH: if (cref.Health) AnimateMosh(&cref); else AnimateMoshDead(&cref);
			break;
		case CTYPE_GALLI: if (cref.Health) AnimateGall(&cref); else AnimateGallDead(&cref);
			break;
		case CTYPE_DIMOR: if (cref.Health) AnimateDimor(&cref); else AnimateDimorDead(&cref);
			break;


		case CTYPE_PARA: if (cref.Health) AnimatePar(&cref);    else AnimateParDead(&cref);
			break;
		case CTYPE_PACHY: if (cref.Health) AnimatePac(&cref);    else AnimatePacDead(&cref);
			break;
		case CTYPE_STEGO: if (cref.Health) AnimateSteg(&cref);   else AnimateStegDead(&cref);
			break;
		case CTYPE_ALLO: if (cref.Health) AnimateRaptor(&cref); else AnimateRaptorDead(&cref);
			break;
		case CTYPE_TRIKE: if (cref.Health) AnimateTric(&cref);   else AnimateTricDead(&cref);
			break;
		case CTYPE_VELOCI: if (cref.Health) AnimateVelo(&cref);   else AnimateVeloDead(&cref);
			break;
		case CTYPE_TREX: if (cref.Health) AnimateTRex(&cref);  else AnimateTRexDead(&cref);
			break;
		}
	}
}




void MakeNoise(Vector3d pos, float range)
{
	for (int c = 0; c < ChCount; c++) {
		TCharacter& cref = Characters[c];

		if (!cref.Health)
			continue;

		float l = VectorLength(SubVectors(cref.pos, pos));
		if (l > range)
			continue;

		if (cref.CType == CTYPE_TREX)
			if (!cref.State) cref.State = 2;

		if (cref.CType != CTYPE_TREX || cref.CType == CTYPE_MOSCH) {
			cref.AfraidTime = (int)(10.f + (range - l) / 256.f) * 1024;
			cref.State = 2;
			cref.NoFindCnt = 0;
		}
	}
}


void CheckAfraid()
{
	if (!MyHealth) return;
	if (TrophyMode) return;

	Vector3d ppos, plook, clook, wlook, rlook;
	ppos = PlayerPos;

	if (DEBUG || UNDERWATER || ObservMode) return;

	plook.y = 0;
	plook.x = std::sinf(CameraAlpha);
	plook.z =-std::cosf(CameraAlpha);

	wlook = Wind.nv;

	float kR, kwind, klook, kstand;

	float kmask = 1.0f;
	float kskill = 1.0f;
	float kscent = 1.0f;

	if (CamoMode)  kmask *= 1.5;
	if (ScentMode) kscent *= 1.5;

	for (int c = 0; c < ChCount; c++) {
		TCharacter& cref = Characters[c];
		if (!cref.Health) continue;
		if (cref.CType < CTYPE_PARA) continue;
		if (cref.AfraidTime || cref.State == 1) continue;

		rlook = SubVectors(ppos, cref.pos);
		kR = VectorLength(rlook) / 256.f / 30.f;
		NormVector(rlook, 1.0f);

		kR *= 2.5f / (float)(1.5 + OptSens);
		if (kR > 2.0f) continue;

		clook.x = cref.lookx;
		clook.y = 0;
		clook.z = cref.lookz;

		MulVectorsScal(wlook, rlook, kwind); kwind *= Wind.speed / 10;
		MulVectorsScal(clook, rlook, klook); klook *= -1.f;

		if (HeadY > 180) kstand = 1.0f; else kstand = 1.4f;

		//============= reasons ==============//

		float kALook = kR * ((klook + 3.f) / 3.f) * kstand * kmask;
		if (klook > 0.3) kALook *= 2.0;
		if (klook > 0.8) kALook *= 2.0;
		kALook /= DinoInfo[cref.CType].LookK;
		if (kALook < 1.0)
			if (TraceLook(cref.pos.x, cref.pos.y + 220, cref.pos.z, PlayerX, PlayerY + HeadY / 2.f, PlayerZ))
				kALook *= 1.3f;

		if (kALook < 1.0)
			if (TraceLook(cref.pos.x, cref.pos.y + 220, cref.pos.z, PlayerX, PlayerY + HeadY, PlayerZ))
				kALook = 2.0;
		kALook *= (1.f + (float)ObjectsOnLook / 4.f);

		/*
		  if (kR<1.0f) {
			  char t[32];
		   wsprintf(t,"%d", ObjectsOnLook);
		   AddMessage(t);
		   kALook = 20.f;
		  }
		  */

		float kASmell = kR * ((kwind + 2.5f) / 2.5F) * ((klook + 4.f) / 4.f) * kscent;
		if (kwind > 0) kASmell *= 2.0;
		kASmell /= DinoInfo[cref.CType].SmellK;

		float kRes = min(kALook, kASmell);

		if (kRes < 1.0) {
			/*      MessageBeep(0xFFFFFFFF);
					char t[128];
					if (kALook<kASmell)
					 sprintf(t, "LOOK: KR: %f  Tr: %d  K: %f", kR, ObjectsOnLook, kALook);
					else
					 sprintf(t, "SMELL: KR: %f  Tr: %d  K: %f", kR, ObjectsOnLook, kASmell);
					AddMessage(t); */

			kRes = min(kRes, kR);
			cref.AfraidTime = (int)(1.0 / (kRes + 0.1) * 10.f * 1000.f);
			cref.State = 2;
			if (cref.CType == CTYPE_TREX)
				if (kALook > kASmell) cref.State = 3;
			cref.NoFindCnt = 0;
		}
	}
}





void PlaceTrophy()
{
	ChCount = 0;

	for (int c = 0; c < 24; c++) {
		if (!TrophyRoom.Body[c].ctype) continue;
		Characters[ChCount].CType = TrophyRoom.Body[c].ctype;

		if (c < 6) Characters[ChCount].alpha = pi / 2; else
			if (c < 12) Characters[ChCount].alpha = pi; else
				if (c < 18) Characters[ChCount].alpha = pi * 3 / 2; else
					Characters[ChCount].alpha = 0;

		ResetCharacter(&Characters[ChCount]);

		Characters[ChCount].State = c;
		Characters[ChCount].scale = TrophyRoom.Body[c].scale;
		Characters[ChCount].pos.x = LandingList.list[c].x * 256.f + 128.f;
		Characters[ChCount].pos.z = LandingList.list[c].y * 256.f + 128.f;

		Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
			Characters[ChCount].pos.z);
		ChCount++;
	}
}



void PlaceCharacters()
{
	int c;
	ChCount = 0;

	//return;    	

	// -- Generate Ambient creatures
	for (c = 0; c < 5 + OptDens; c++) {
		Characters[ChCount].CType = CTYPE_MOSCH + rRand(2);

	replace1:
		Characters[ChCount].pos.x = PlayerX + siRand(10040);
		Characters[ChCount].pos.z = PlayerZ + siRand(10040);
		Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
			Characters[ChCount].pos.z);

		if (CheckPlaceCollisionP(Characters[ChCount].pos)) goto replace1;

		if (Characters[ChCount].CType == CTYPE_DIMOR)
			Characters[ChCount].pos.y += 2048.f;

		Characters[ChCount].tgx = Characters[ChCount].pos.x;
		Characters[ChCount].tgz = Characters[ChCount].pos.z;
		Characters[ChCount].tgtime = 0;

		ResetCharacter(&Characters[ChCount]);
		ChCount++;
	}

	int MC = 10 + OptDens * 2;
	if (TargetDino == 6) MC = 5 + OptDens;
	//======== main =========//
	for (c = 0; c < MC; c++) {
		if (c < 4) Characters[ChCount].CType = CTYPE_PARA + rRand(3);
		else Characters[ChCount].CType = CTYPE_PARA + TargetDino;

		//Characters[ChCount].CType = CTYPE_PARA + TargetDino;     	 
	replace2:
		Characters[ChCount].pos.x = 256 * 256 + siRand(20 * 256) * 10.f;
		Characters[ChCount].pos.z = 256 * 256 + siRand(20 * 256) * 10.f;
		Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
			Characters[ChCount].pos.z);
		if (fabs(Characters[ChCount].pos.x - PlayerX) +
			fabs(Characters[ChCount].pos.z - PlayerZ) < 256 * 40)
			goto replace2;

		if (CheckPlaceCollisionP(Characters[ChCount].pos)) goto replace2;

		Characters[ChCount].tgx = Characters[ChCount].pos.x;
		Characters[ChCount].tgz = Characters[ChCount].pos.z;
		Characters[ChCount].tgtime = 0;

		ResetCharacter(&Characters[ChCount]);
		ChCount++;
	}

	PlayerY = GetLandQH(PlayerX, PlayerZ);
	DemoPoint.DemoTime = 0;
}








void CreateChMorphedModel(TCharacter& chr)
{

	if (chr.Phase >= chr.pinfo->Animation.size()) {
		return;
	}
	if (chr.PrevPhase >= chr.pinfo->Animation.size()) {
		return;
	}

	TAni* aptr = nullptr;
	TAni* paptr = nullptr;

	try {
		aptr = &chr.pinfo->Animation.at(chr.Phase);
		paptr = &chr.pinfo->Animation.at(chr.PrevPhase);
	}
	catch (std::exception& e) {
		throw dohalt(e.what(), __FUNCTION__, __FILE__, __LINE__);
	}

	int CurFrame = 0, SplineD = 0, PCurFrame = 0, PSplineD = 0;
	float scale = chr.scale;

	if (chr.FTime > (aptr->AniTime/2))
		return;

	CurFrame = ((aptr->FramesCount - 1) * chr.FTime * 256) / aptr->AniTime;
	SplineD = CurFrame & 0xFF;
	CurFrame >>= 8;


	bool PMorph = (chr.Phase != chr.PrevPhase) && (chr.PPMorphTime < PMORPHTIME) && (MORPHP);

	if (PMorph) {
		PCurFrame = ((paptr->FramesCount - 1) * chr.PrevPFTime * 256) / paptr->AniTime;
		PSplineD = PCurFrame & 0xFF;
		PCurFrame = (PCurFrame >> 8);
	}

	if (!MORPHA) { SplineD = 0; PSplineD = 0; }

	float k1, k2, pk1, pk2, pmk1, pmk2;

	k2 = (float)(SplineD) / 256.f;
	k1 = 1.0f - k2;
	k1 /= 8.f; k2 /= 8.f;

	if (PMorph) {
		pk2 = (float)(PSplineD) / 256.f;
		pk1 = 1.0f - pk2;
		pk1 /= 8.f; pk2 /= 8.f;
		pmk1 = (float)chr.PPMorphTime / PMORPHTIME;
		pmk2 = 1.f - pmk1;
	}

	int VCount = chr.pinfo->Model.gVertex.size();
	int16_t* adptr = aptr->aniData + CurFrame * VCount * 3;
	int16_t* padptr = paptr->aniData + PCurFrame * VCount * 3;

	float sb = std::sinf(chr.beta) * scale;
	float cb = std::cosf(chr.beta) * scale;
	float sg = std::sinf(chr.gamma);
	float cg = std::cosf(chr.gamma);

	for (int v = 0; v < VCount; v++) {
		k1 = 1.0f; k2 = 0.0f;
		float x = *(adptr + v * 3 + 0) * k1 + *(adptr + (v + VCount) * 3 + 0) * k2;
		float y = *(adptr + v * 3 + 1) * k1 + *(adptr + (v + VCount) * 3 + 1) * k2;
		float z = -(*(adptr + v * 3 + 2) * k1 + *(adptr + (v + VCount) * 3 + 2) * k2);
		continue;

		if (PMorph) {
			float px = *(padptr + v * 3 + 0) * pk1 + *(padptr + (v + VCount) * 3 + 0) * pk2;
			float py = *(padptr + v * 3 + 1) * pk1 + *(padptr + (v + VCount) * 3 + 1) * pk2;
			float pz = -(*(padptr + v * 3 + 2) * pk1 + *(padptr + (v + VCount) * 3 + 2) * pk2);

			x = x * pmk1 + px * pmk2;
			y = y * pmk1 + py * pmk2;
			z = z * pmk1 + pz * pmk2;
		}


		float zz = z;
		float xx = cg * x - sg * y;
		float yy = cg * y + sg * x;

		float fi;
		if (z > 0) {
			fi = z / 240.f;
			if (fi > 1.f) fi = 1.f;
		}
		else {
			fi = z / 380.f;
			if (fi < -1.f) fi = -1.f;
		}

		fi *= chr.bend;

		float bendc = std::cosf(fi);
		float bends = std::sinf(fi);

		float bx = bendc * xx - bends * zz;
		float bz = bendc * zz + bends * xx;
		zz = bz;
		xx = bx;

		chr.pinfo->Model.gVertex[v].x = xx * scale;
		chr.pinfo->Model.gVertex[v].y = cb * yy - sb * zz;
		chr.pinfo->Model.gVertex[v].z = cb * zz + sb * yy;
	}
}



void CreateMorphedModel(TModel* mptr, TAni* aptr, int FTime)
{
	bool Tweening = false;

	int CurFrame = ((aptr->FramesCount - 1) * FTime * 256) / aptr->AniTime;

	int SplineD = CurFrame & 0xFF;
	CurFrame = (CurFrame >> 8);

	if (CurFrame > aptr->FramesCount - 6)
		return;

	if (Tweening) {
		float k2 = (float)(SplineD) / 256.f;
		float k1 = 1.0f - k2;
		k1 /= 8.f; k2 /= 8.f;

		int VCount = mptr->VCount;
		int16_t* adptr = &(aptr->aniData[CurFrame * VCount * 3]);
		for (int v = 0; v < VCount; v++) {
			mptr->gVertex[v].x = *(adptr + (v * 3) + 0) * k1 + *(adptr + (v + VCount) * 3 + 0) * k2;
			mptr->gVertex[v].y = *(adptr + (v * 3) + 1) * k1 + *(adptr + (v + VCount) * 3 + 1) * k2;
			mptr->gVertex[v].z = -*(adptr + (v * 3) + 2) * k1 - *(adptr + (v + VCount) * 3 + 2) * k2;
		}
	}
	else {
		int VCount = mptr->VCount;
		int16_t* adptr = &(aptr->aniData[CurFrame * VCount * 3]);
		for (int v = 0; v < VCount; v++) {
			mptr->gVertex[v].x = *(adptr + (v * 3) + 0) * 0.125f;
			mptr->gVertex[v].y = *(adptr + (v * 3) + 1) * 0.125f;
			mptr->gVertex[v].z = -*(adptr + (v * 3) + 2) * 0.125f;
		}
	}
}




void CreateMorphedObject(TModel* mptr, TVTL& vtl, int FTime)
{
	int CurFrame = ((vtl.FramesCount - 1) * FTime * 256) / vtl.AniTime;

	int SplineD = CurFrame & 0xFF;
	CurFrame = (CurFrame >> 8);

	float k2 = (float)(SplineD) / 256.f;
	float k1 = 1.0f - k2;
	k1 /= 8.f; k2 /= 8.f;

	int VCount = mptr->VCount;
	short int* adptr = &(vtl.aniData[CurFrame * VCount * 3]);
	for (int v = 0; v < VCount; v++) {
		mptr->gVertex[v].x = *(adptr + v * 3 + 0) * k1 + *(adptr + (v + VCount) * 3 + 0) * k2;
		mptr->gVertex[v].y = *(adptr + v * 3 + 1) * k1 + *(adptr + (v + VCount) * 3 + 1) * k2;
		mptr->gVertex[v].z = -*(adptr + v * 3 + 2) * k1 - *(adptr + (v + VCount) * 3 + 2) * k2;
	}
}