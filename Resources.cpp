#include "Hunt.h"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "Targa.h"


HANDLE hfile;
DWORD  l, HeapAllocated, HeapReleased;

void GenerateModelMipMaps(TModel&);
void GenerateAlphaFlags(TModel&);
void CalcLights(TModel& model); // Defined in mathematics.cpp


void AddMessage(const std::string &mt)
{
	//MessageList.timeleft = timeGetTime() + 2 * 1000;
	MessageList.timeleft = TimerGetMS() + 2 * 1000;
	MessageList.mtext = mt;
}

void PlaceHunter()
{
	if (LockLanding) return;

	if (TrophyMode) {
		PlayerX = 76 * 256 + 128;
		PlayerZ = 70 * 256 + 128;
		PlayerY = GetLandQH(PlayerX, PlayerZ);
		return;
	}

	//int p = (timeGetTime() % LandingList.PCount);
	int p = (TimerGetMS() % LandingList.PCount);
	PlayerX = (float)LandingList.list[p].x * 256 + 128;
	PlayerZ = (float)LandingList.list[p].y * 256 + 128;
	PlayerY = GetLandQH(PlayerX, PlayerZ);
}

int DitherHi(int C)
{
	int d = C & 255;
	C = C / 256;
	if (rand() * 255 / RAND_MAX < d) C++;
	if (C > 31) C = 31;
	return C;
}


int DitherHiC(int C)
{
	int d = C & 7;
	C = C / 8;
	if (rand() * 8 / RAND_MAX < d) C++;
	if (C > 31) C = 31;
	return C;
}




int DizeredHiColor(int R, int G, int B)
{
	return (DitherHiC(R) << 10) +
		(DitherHiC(G) << 5) +
		(DitherHiC(B));
}


void CreateWaterTab()
{
	for (int c = 0; c < 0x8000; c++) {
		int R = (c >> 10);
		int G = (c >> 5) & 31;
		int B = c & 31;
		R = 1 + (R * 8) / 28; if (R > 31) R = 31;
		G = 2 + (G * 18) / 28; if (G > 31) G = 31;
		B = 3 + (B * 22) / 28; if (B > 31) B = 31; /*
		R =  (WaterR*WaterA + R * WaterR) / 64; if (R>31) R=31;
		G =  (WaterR*WaterA + G * WaterG) / 64; if (G>31) G=31;
		B =  (WaterR*WaterA + B * WaterB) / 64; if (B>31) B=31; */
		FadeTab[64][c] = HiColor(R, G, B);
	}
}

void CreateFadeTab()
{
#ifdef _soft
	for (int l = 0; l < 64; l++)
		for (int c = 0; c < 0x8000; c++) {
			int R = (c >> 10);
			int G = (c >> 5) & 31;
			int B = c & 31;
			//if (l<16) ll=64
			R = (int)((float)R * (64.f - l) / 60.f + (float)rand() * 0.2f / RAND_MAX); if (R > 31) R = 31;
			G = (int)((float)G * (64.f - l) / 60.f + (float)rand() * 0.2f / RAND_MAX); if (G > 31) G = 31;
			B = (int)((float)B * (64.f - l) / 60.f + (float)rand() * 0.2f / RAND_MAX); if (B > 31) B = 31;
			FadeTab[l][c] = HiColor(R, G, B);
		}

	CreateWaterTab();
#endif
}


void CreateDivTable()
{
	DivTbl[0] = 0x7fffffff;
	DivTbl[1] = 0x7fffffff;
	DivTbl[2] = 0x7fffffff;
	for (int i = 3; i < 10240; i++)
		DivTbl[i] = (int)((float)0x100000000 / i);

	for (int y = 0; y < 32; y++)
		for (int x = 0; x < 32; x++)
			RandomMap[y][x] = rand() * 1024 / RAND_MAX;
}


void CreateVideoDIB()
{
	hdcMain = GetDC(hwndMain);
	hdcCMain = CreateCompatibleDC(hdcMain);

	SelectObject(hdcMain, fnt_Midd);
	SelectObject(hdcCMain, fnt_Midd);

	BITMAPINFOHEADER bmih;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = 1024;
	bmih.biHeight = -768;
	bmih.biPlanes = 1;
	bmih.biBitCount = 16;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 400;
	bmih.biYPelsPerMeter = 400;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;

	BITMAPINFO binfo;
	binfo.bmiHeader = bmih;
	hbmpVideoBuf = CreateDIBSection(hdcMain, &binfo, DIB_RGB_COLORS, &lpVideoBuf, NULL, 0);


	bmih.biWidth = 800;
	bmih.biHeight = -600;
	binfo.bmiHeader = bmih;
	hbmpMenuBuf = CreateDIBSection(hdcMain, &binfo, DIB_RGB_COLORS, &lpMenuBuf, NULL, 0);
	hbmpMenuBuf2 = CreateDIBSection(hdcMain, &binfo, DIB_RGB_COLORS, &lpMenuBuf2, NULL, 0);
}



int GetDepth(int y, int x)
{
	int h = HMap[y][x] + 48 + 1;
	float lmin = 17.2f;
	float l;
	BOOL Done = FALSE;
	for (int yy = -12; yy <= 12; yy++)
		for (int xx = -12; xx <= 12; xx++) {
			if (!(FMap[(y + yy) & 511][(x + xx) & 511] & fmWater)) {
				l = (float)sqrt((xx * xx) + (yy * yy));
				if (l < lmin) lmin = l;
			}
		}

	//if (lmin>16.f) lmin = 16.f; 
	float ladd = 0;
	if (lmin > 6) { ladd = lmin - 6; lmin = 6; }
	h -= (int)(lmin * 4 + ladd * 2.4);
	if (h < 0) h = 0;
	return h;
}


int GetObjectH(int x, int y, int R)
{
	x = (x << 8) + 128;
	y = (y << 8) + 128;
	float hr, h;
	hr = GetLandH((float)x, (float)y);
	h = GetLandH((float)x + R, (float)y); if (h < hr) hr = h;
	h = GetLandH((float)x - R, (float)y); if (h < hr) hr = h;
	h = GetLandH((float)x, (float)y + R); if (h < hr) hr = h;
	h = GetLandH((float)x, (float)y - R); if (h < hr) hr = h;
	hr += 15;
	return  (int)(hr / ctHScale + 48);
}


int GetObjectHWater(int x, int y)
{
	if (FMap[y][x] & fmReverse)
		return (int)(HMap[y][x + 1] + HMap[y + 1][x]) / 2 + 48;
	else
		return (int)(HMap[y][x] + HMap[y + 1][x + 1]) / 2 + 48;
}



void CreateTMap()
{
	int x, y;
	LandingList.PCount = 0;
	for (y = 0; y < 512; y++)
		for (x = 0; x < 512; x++) {
			if (TMap1[y][x] == 255) TMap1[y][x] = 1;
			if (TMap2[y][x] == 255) TMap2[y][x] = 1;
		}


	for (y = 0; y < ctMapSize; y++)
		for (x = 0; x < ctMapSize; x++) {

			if (OMap[y][x] == 254) {
				LandingList.list[LandingList.PCount].x = x;
				LandingList.list[LandingList.PCount].y = y;
				LandingList.PCount++;
				OMap[y][x] = 255;
			}

			int ob = OMap[y][x];
			if (ob == 255) { HMapO[y][x] = 48; continue; }

			//HMapO[y][x] = GetObjectH(x,y); 
			if (MObjects[ob].info.flags & ofPLACEUSER)   HMapO[y][x] += 48;
			if (MObjects[ob].info.flags & ofPLACEGROUND) HMapO[y][x] = GetObjectH(x, y, MObjects[ob].info.GrRad);
			if (MObjects[ob].info.flags & ofPLACEWATER)  HMapO[y][x] = GetObjectHWater(x, y);

		}

	if (!LandingList.PCount) {
		LandingList.list[LandingList.PCount].x = 256;
		LandingList.list[LandingList.PCount].y = 256;
		LandingList.PCount = 1;
	}

	if (TrophyMode) {
		LandingList.PCount = 0;
		for (x = 0; x < 6; x++) {
			LandingList.list[LandingList.PCount].x = 69 + x * 3;
			LandingList.list[LandingList.PCount].y = 66;
			LandingList.PCount++;
		}

		for (y = 0; y < 6; y++) {
			LandingList.list[LandingList.PCount].x = 87;
			LandingList.list[LandingList.PCount].y = 69 + y * 3;
			LandingList.PCount++;
		}

		for (x = 0; x < 6; x++) {
			LandingList.list[LandingList.PCount].x = 84 - x * 3;
			LandingList.list[LandingList.PCount].y = 87;
			LandingList.PCount++;
		}

		for (y = 0; y < 6; y++) {
			LandingList.list[LandingList.PCount].x = 66;
			LandingList.list[LandingList.PCount].y = 84 - y * 3;
			LandingList.PCount++;
		}
	}


}


/*****************************************************************************************************/
/* TTexture Method Definitions */

void TTexture::copy(int w, int h, uint16_t* d) {
	if (data) {
		delete[] data;
	}

	if (mipmap) {
		delete mipmap;
	}

	W = w;
	H = h;
	data = new uint16_t[w * h];

	if (d != nullptr) {
		std::memcpy(data, d, w * h * sizeof(uint16_t));
	}
}

void TTexture::copy(const TTexture& t) {
	if (data) {
		delete[] data;
	}

	if (mipmap) {
		delete mipmap;
	}

	W = t.W;
	H = t.H;
	data = new uint16_t[W * H];

	std::memcpy(data, t.data, W * H * sizeof(uint16_t));

	if (t.mipmap != nullptr) {
		mipmap = new TTexture();
		mipmap->copy(*t.mipmap);
	}
}

void TTexture::generateMipMap() {
	this->mipmap = new TTexture();
	this->mipmap->W = this->W / 2;
	this->mipmap->H = this->H / 2;
	this->mipmap->data = new uint16_t[this->mipmap->W * this->mipmap->H];
	this->mipmap->mipmap = nullptr;

	for (int y = 0; y < this->mipmap->H; y++) {
		for (int x = 0; x < this->mipmap->W; x++) {
			int C1 = *(this->data + x * 2 + (y * 2 + 0) * 2 * this->mipmap->W);
			int C2 = *(this->data + x * 2 + 1 + (y * 2 + 0) * 2 * this->mipmap->W);
			int C3 = *(this->data + x * 2 + (y * 2 + 1) * 2 * this->mipmap->H);
			int C4 = *(this->data + x * 2 + 1 + (y * 2 + 1) * 2 * this->mipmap->H);
			int B = (((C1 >> 0) & 31) + ((C2 >> 0) & 31) + ((C3 >> 0) & 31) + ((C4 >> 0) & 31) + 2) >> 2;
			int G = (((C1 >> 5) & 31) + ((C2 >> 5) & 31) + ((C3 >> 5) & 31) + ((C4 >> 5) & 31) + 2) >> 2;
			int R = (((C1 >> 10) & 31) + ((C2 >> 10) & 31) + ((C3 >> 10) & 31) + ((C4 >> 10) & 31) + 2) >> 2;
			*(this->mipmap->data + x + y * this->mipmap->W) = HiColor(R, G, B);
		}
	}
}


int CalcColorSum(uint16_t* A, int L)
{
	int R = 0, G = 0, B = 0;
	for (int x = 0; x < L; x++) {
		B += (*(A + x) >> 0) & 31;
		G += (*(A + x) >> 5) & 31;
		R += (*(A + x) >> 10) & 31;
	}
	return HiColor(R / L, G / L, B / L);
}


void GenerateShadedMipMap(uint16_t* src, uint16_t* dst, int L)
{
	for (int x = 0; x < 16 * 16; x++) {
		int B = (*(src + x) >> 0) & 31;
		int G = (*(src + x) >> 5) & 31;
		int R = (*(src + x) >> 10) & 31;
		R = DitherHi(SkyR * L / 8 + R * (256 - L) + 6);
		G = DitherHi(SkyG * L / 8 + G * (256 - L) + 6);
		B = DitherHi(SkyB * L / 8 + B * (256 - L) + 6);
		*(dst + x) = HiColor(R, G, B);
	}
}


void GenerateShadedSkyMipMap(uint16_t* src, uint16_t* dst, int L)
{
	for (int x = 0; x < 128 * 128; x++) {
		int B = (*(src + x) >> 0) & 31;
		int G = (*(src + x) >> 5) & 31;
		int R = (*(src + x) >> 10) & 31;
		R = DitherHi(SkyR * L / 8 + R * (256 - L) + 6);
		G = DitherHi(SkyG * L / 8 + G * (256 - L) + 6);
		B = DitherHi(SkyB * L / 8 + B * (256 - L) + 6);
		*(dst + x) = HiColor(R, G, B);
	}
}


void DATASHIFT(uint16_t* d, int cnt)
{
#ifdef _soft
	cnt >>= 1;
	for (int l = 0; l < cnt; l++)
		*(d + l) *= 2;
#endif
}



void ApplyAlphaFlags(WORD* tptr, int cnt)
{
#ifdef _d3d
	for (int w = 0; w < cnt; w++)
		*(tptr + w) |= 0x8000;
#endif
}

void LoadTexture(std::ifstream& rsc, int width, int height, int byte_count, TTexture& texture)
{
	if ((byte_count / 2) > width * height)
		return;

	uint16_t* tdata = new uint16_t[width * height];
	std::memset(tdata, 0, (width * sizeof(uint16_t)) * height);

	rsc.read((char*)tdata, byte_count);

	/*for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (!tdata[y * width + x]) {
				tdata[y * width + x] = 1;
			}
		}
	}*/

	texture.copy(width, height, tdata);
	delete[] tdata;

	//texture.generateMipMap();
	//texture.getMipmap().generateMipMap();
	//texture.getMipmap().getMipmap().generateMipMap();
}

void LoadTexture(std::ifstream& rsc, TTexture& texture)
{
	LoadTexture(rsc, 128, 128, 256 * 128, texture);
	/*uint16_t *tdata = new uint16_t[128 * 128];

	rsc.read((char*)tdata, 128 * 128 * sizeof(uint16_t));

	for (int y = 0; y < 128; y++) {
		for (int x = 0; x < 128; x++) {
			if (!tdata[y * 128 + x]) {
				tdata[y * 128 + x] = 1;
			}
		}
	}

	texture.copy(128, 128, tdata);
	delete[] tdata;

	texture.generateMipMap();
	texture.getMipmap().generateMipMap();
	texture.getMipmap().getMipmap().generateMipMap();

	// RH : Used by RenderSoft for something, TODO: will implement later
	//std::memcpy(T->SDataC[0], T->DataC, 32 * 32 * 2);
	//std::memcpy(T->SDataC[1], T->DataC, 32 * 32 * 2);

	//DATASHIFT((unsigned short*)T, sizeof(TEXTURE));
	//for (int w = 0; w < 32 * 32; w++)
	//	T->SDataC[1][w] = FadeTab[16][T->SDataC[1][w] >> 1];

	ApplyAlphaFlags(texture.getData(), 128 * 128);
	ApplyAlphaFlags(texture.getMipmap().getData(), 64 * 64);
	ApplyAlphaFlags(texture.getMipmap().getMipmap().getData(), 32 * 32);*/
}




void LoadSky(std::ifstream& file)
{
	file.read((char*)SkyPic, 256 * 256 * 2);

	for (int y = 0; y < 128; y++)
		for (int x = 0; x < 128; x++)
			SkyFade[0][y * 128 + x] = SkyPic[y * 2 * 256 + x * 2];

	for (int l = 1; l < 8; l++)
		GenerateShadedSkyMipMap(SkyFade[0], SkyFade[l], l * 32 - 16);
	GenerateShadedSkyMipMap(SkyFade[0], SkyFade[8], 250);
	ApplyAlphaFlags(SkyPic, 256 * 256);
}


void LoadSkyMap(std::ifstream& file)
{
	file.read((char*)SkyMap, 128 * 128);
}


void fp_conv(void* d)
{
	return;
	int i;
	float f;
	memcpy(&i, d, 4);
	f = ((float)i) / 256.f;
	memcpy(d, &f, 4);
}



void CorrectModel(TModel& model)
{
	// TODO: change to vector I guess
	TFace *tface = new TFace[model.FCount];

	for (int f = 0; f < model.FCount; f++) {
		if (!(model.gFace[f].Flags & sfDoubleSide))
			model.gFace[f].Flags |= sfNeedVC;
#if defined(_d3d) || defined(_d3d11)
		fp_conv(&model.gFace[f].tax);
		fp_conv(&model.gFace[f].tay);
		fp_conv(&model.gFace[f].tbx);
		fp_conv(&model.gFace[f].tby);
		fp_conv(&model.gFace[f].tcx);
		fp_conv(&model.gFace[f].tcy);
#else
		model.gFace[f].tax = (model.gFace[f].tax << 16) + 0x8000;
		model.gFace[f].tay = (model.gFace[f].tay << 16) + 0x8000;
		model.gFace[f].tbx = (model.gFace[f].tbx << 16) + 0x8000;
		model.gFace[f].tby = (model.gFace[f].tby << 16) + 0x8000;
		model.gFace[f].tcx = (model.gFace[f].tcx << 16) + 0x8000;
		model.gFace[f].tcy = (model.gFace[f].tcy << 16) + 0x8000;
#endif
	}

	int fp = 0;
	for (int f = 0; f < model.FCount; f++)
		if ((model.gFace[f].Flags & (sfOpacity | sfTransparent)) == 0)
		{
			tface[fp] = model.gFace[f];
			fp++;
		}

	for (int f = 0; f < model.FCount; f++)
		if ((model.gFace[f].Flags & (sfOpacity | sfTransparent)) != 0)
		{
			tface[fp] = model.gFace[f];
			fp++;
		}

	// WARNING: Gotta get rid of this
	memcpy(model.gFace.data(), tface, model.gFace.size() * sizeof(TFace));
	delete[] tface;
}

/*

*/
void LoadModel(std::ifstream& file, TModel& model)
{
	model.reset();

	if (!file.is_open()) {
		std::cout << "! ERROR :: LoadModel(...) !\n\tifstream is not open!" << std::endl;
		return;
	}

	file.read((char*)&model.VCount, 4);
	file.read((char*)&model.FCount, 4);
	file.read((char*)&OCount, 4);
	file.read((char*)&model.TextureSize, 4);

	//model.gFace.resize(model.FCount);
	for (int f = 0; f < model.FCount; ++f) {
		TFaceI face;
		file.read((char*)&face, sizeof(TFaceI));
		model.gFace.push_back(face);
	}

	model.VLight.resize(model.VCount);
	model.gVertex.resize(model.VCount);
	for (size_t v = 0; v < model.gVertex.size(); ++v) {
		file.read((char*)&model.gVertex.at(v), sizeof(TPoint3d));
	}

	file.seekg(48 * OCount, std::ios::cur);

	if (HARD3D) CalcLights(model);

	int ts = model.TextureSize;

	if (ts > 256 * (256 * sizeof(uint16_t))) {
		std::cout << "! WARNING !\n\tLoadModel():TModel(" << model.VCount << ", " << model.FCount << ") - TextureSize is greater than 256x256!" << std::endl;
		std::cout << "\tWidth: " << (256) << ", Height: " << ((ts / 2) / 256) << std::endl;
	}
	else if (ts < 256 * (256 * sizeof(uint16_t))) {
		std::cout << "! WARNING !\n\tLoadModel():TModel(" << model.VCount << ", " << model.FCount << ") - TextureSize is less than 256x256!" << std::endl;
		std::cout << "\tWidth: " << (256) << ", Height: " << ((ts / 2) / 256) << std::endl;
	}

	if (HARD3D)
		model.TextureHeight = 256;
	else 
		model.TextureHeight = model.TextureSize >> 9;

	model.TextureSize = model.TextureHeight * 512;

	LoadTexture(file, 256, 256, ts, model.texture);

	for (size_t v = 0; v < model.gVertex.size(); v++) {
		model.gVertex[v].x *= 2.f;
		model.gVertex[v].z *= -2.f;
	}

	CorrectModel(model);
}



void LoadAnimation(std::ifstream& file, TVTL& vtl)
{
	int vc;

	file.read((char*)&vc, 4);
	std::cout << "LoadAnimation() vc=" << vc << std::endl;
	file.read((char*)&vc, 4);
	std::cout << "LoadAnimation() vc=" << vc << std::endl;
	file.read((char*)&vtl.aniKPS, 4);
	file.read((char*)&vtl.FramesCount, 4);
	vtl.FramesCount++;

	vtl.AniTime = (vtl.FramesCount * 1000) / vtl.aniKPS;
	//vtl.aniDataLength = vc * vtl.FramesCount * 3;
	vtl.aniData = new int16_t[vc * vtl.FramesCount * 3];
	file.read((char*)vtl.aniData, (vc * vtl.FramesCount * 6));

}



void LoadModelEx(TModel& model, const std::string& FName)
{
	std::ifstream file(FName, std::ios::binary);

	if (!file.is_open()) {
		char sz[512];
		wsprintf(sz, "Error opening file\n%s.", FName.c_str());
		DoHalt(sz);
	}

	model.reset();

	file.read((char*)&model.VCount, 4);
	file.read((char*)&model.FCount, 4);
	file.read((char*)&OCount, 4);
	file.read((char*)&model.TextureSize, 4);

	model.gFace.resize(model.FCount);
	for (size_t f = 0; f < model.gFace.size(); ++f) {
		file.read((char*)&model.gFace.at(f), sizeof(TFace));
	}

	model.VLight.resize(model.VCount);
	model.gVertex.resize(model.VCount);
	for (size_t v = 0; v < model.gVertex.size(); ++v) {
		file.read((char*)&model.gVertex.at(v), sizeof(TPoint3d));
	}

	file.seekg(48 * OCount, std::ios::cur);

	int ts = model.TextureSize;
	if (HARD3D)
		model.TextureHeight = 256;
	else
		model.TextureHeight = (model.TextureSize / 2) / 256;
	model.TextureSize = model.TextureHeight * 512;

	LoadTexture(file, 256, 256, ts, model.texture);

	file.close();

	for (size_t v = 0; v < model.gVertex.size(); v++) {
		model.gVertex[v].x *= 2.f;
		model.gVertex[v].y *= 2.f;
		model.gVertex[v].z *= -2.f;
	}

	CorrectModel(model);
	//GenerateModelMipMaps(model);
	GenerateAlphaFlags(model);
}




void LoadWav(const std::string& FName, TSoundFX& sfx)
{
	DWORD l;

	HANDLE hfile = CreateFile(FName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		char sz[512];
		wsprintf(sz, "Error opening file\n%s.", FName.c_str());
		DoHalt(sz);
	}

	if (sfx.lpData) {
		delete[] sfx.lpData;
		sfx.lpData = nullptr;
	}

	SetFilePointer(hfile, 36, NULL, FILE_BEGIN);

	char c[5]; c[4] = 0;

	for (; ; ) {
		ReadFile(hfile, c, 1, &l, NULL);
		if (c[0] == 'd') {
			ReadFile(hfile, &c[1], 3, &l, NULL);
			if (!lstrcmp(c, "data")) break;
			else SetFilePointer(hfile, -3, NULL, FILE_CURRENT);
		}
	}

	ReadFile(hfile, &sfx.length, 4, &l, NULL);

	sfx.lpData = new int16_t[sfx.length];

	ReadFile(hfile, sfx.lpData, sfx.length, &l, NULL);
	CloseHandle(hfile);
}


WORD conv_565(WORD c)
{
	return (c & 31) + ((c & 0xFFE0) << 1);
}

void conv_pic(TPicture& pic)
{
	if (!HARD3D)
		return;
	
	for (int y = 0; y < pic.H; y++)
		for (int x = 0; x < pic.W; x++)
			*(pic.lpImage + x + y * pic.W) = conv_565(*(pic.lpImage + x + y * pic.W));
}



/*
Load a 24-bit RGB Bitmap into TPicture class as a 16-bit RGB TARGA
*/
void LoadPicture(TPicture& pic, const std::string& pname)
{
	int C;
	byte fRGB[800][3];
	BITMAPFILEHEADER bmpFH;
	BITMAPINFOHEADER bmpIH;

	std::ifstream f(pname, std::ios::binary);

	if (!f.is_open()) {
		std::stringstream sz;
		sz << "Error opening file\n" << pname;
		DoHalt(sz.str());
		return;
	}

	f.read((char*)&bmpFH, sizeof(BITMAPFILEHEADER));
	f.read((char*)&bmpIH, sizeof(BITMAPINFOHEADER));

	if (pic.lpImage) {
		delete[] pic.lpImage;
		pic.lpImage = nullptr;
	}

	pic.W = bmpIH.biWidth;
	pic.H = bmpIH.biHeight;
	pic.lpImage = new uint16_t[pic.W * pic.H];

	for (int y = 0; y < pic.H; y++) {
		f.read((char*)fRGB, pic.W * 3);
		for (int x = 0; x < pic.W; x++) {
			C = ((int)fRGB[x][2] / 8 << 10) + ((int)fRGB[x][1] / 8 << 5) + ((int)fRGB[x][0] / 8);
			*(pic.lpImage + (pic.H - y - 1) * pic.W + x) = C;
		}
	}

	f.close();
}

/*
Load a 16-bit RGB TARGA into TPicture class
*/
void LoadPictureTGA(TPicture& pic, const std::string& pname)
{
	std::ifstream f(pname, std::ios::binary);

	if (!f.is_open()) {
		std::stringstream sz;
		sz << "Error opening file\n" << pname;
		DoHalt(sz.str());
		return;
	}

	TARGAINFOHEADER tga;
	f.read((char*)&tga, sizeof(TARGAINFOHEADER));

	if (tga.tgaIdentSize != 0) {
		f.close();
		std::cout << "! ERROR !\n\tLoadPictureTGA(" << pname << ") TGA has an ident field!" << std::endl;
		return;
	}

	if (tga.tgaColorMapType != 0) {
		f.close();
		std::cout << "! ERROR !\n\tLoadPictureTGA(" << pname << ") TGA has a colour map!" << std::endl;
		return;
	}

	if (tga.tgaImageType != 2) {
		f.close();
		std::cout << "! ERROR !\n\tLoadPictureTGA(" << pname << ") image type is not RGB!" << std::endl;
		return;
	}

	if (tga.tgaBits != 16) {
		f.close();
		std::cout << "! ERROR !\n\tLoadPictureTGA(" << pname << ") incorrect bitdepth of TGA file!" << std::endl;
		return;
	}

	pic.W = tga.tgaWidth;
	pic.H = tga.tgaHeight;

	if (pic.lpImage)
		delete[] pic.lpImage;
	pic.lpImage = new uint16_t[pic.W * pic.H];
	
	//f.read((char*)pic.lpImage, pic.W * sizeof(uint16_t) * pic.H);
	for (int y = 0; y < pic.H; y++) {
		int y0 = (pic.H - 1) - y;
		//f.read((char*)pic.lpImage + ((pic.W * sizeof(uint16_t)) * y0), pic.W * sizeof(uint16_t));
		f.read((char*)&pic.lpImage[pic.W * y0], pic.W * sizeof(uint16_t));
	}

	f.close();
}



void LoadTextFile(TText& txt, const std::string& FName)
{
	std::fstream f(FName, std::ios::in);
	txt.Lines = 0;

	if (f.is_open()) {

		while (!f.eof()) {
			std::getline(f, txt.Text[txt.Lines]);
			txt.Lines++;
		}

		f.close();
	}
}


void CreateMipMapMT(WORD* dst, WORD* src, int H)
{
	for (int y = 0; y < H; y++)
		for (int x = 0; x < 127; x++) {
			int C1 = *(src + (x * 2 + 0) + (y * 2 + 0) * 256);
			int C2 = *(src + (x * 2 + 1) + (y * 2 + 0) * 256);
			int C3 = *(src + (x * 2 + 0) + (y * 2 + 1) * 256);
			int C4 = *(src + (x * 2 + 1) + (y * 2 + 1) * 256);

			if (!HARD3D) { C1 >>= 1; C2 >>= 1; C3 >>= 1; C4 >>= 1; }

			/*if (C1 == 0 && C2!=0) C1 = C2;
			if (C1 == 0 && C3!=0) C1 = C3;
			if (C1 == 0 && C4!=0) C1 = C4;*/

			if (C1 == 0) { *(dst + x + y * 128) = 0; continue; }

			//C4 = C1; 

			if (!C2) C2 = C1;
			if (!C3) C3 = C1;
			if (!C4) C4 = C1;

			int B = (((C1 >> 0) & 31) + ((C2 >> 0) & 31) + ((C3 >> 0) & 31) + ((C4 >> 0) & 31) + 2) >> 2;
			int G = (((C1 >> 5) & 31) + ((C2 >> 5) & 31) + ((C3 >> 5) & 31) + ((C4 >> 5) & 31) + 2) >> 2;
			int R = (((C1 >> 10) & 31) + ((C2 >> 10) & 31) + ((C3 >> 10) & 31) + ((C4 >> 10) & 31) + 2) >> 2;
			if (!HARD3D) *(dst + x + y * 128) = HiColor(R, G, B) * 2;
			else *(dst + x + y * 128) = HiColor(R, G, B);
		}
}



void CreateMipMapMT2(WORD* dst, WORD* src, int H)
{
	for (int y = 0; y < H; y++)
		for (int x = 0; x < 63; x++) {
			int C1 = *(src + (x * 2 + 0) + (y * 2 + 0) * 128);
			int C2 = *(src + (x * 2 + 1) + (y * 2 + 0) * 128);
			int C3 = *(src + (x * 2 + 0) + (y * 2 + 1) * 128);
			int C4 = *(src + (x * 2 + 1) + (y * 2 + 1) * 128);

			if (!HARD3D) { C1 >>= 1; C2 >>= 1; C3 >>= 1; C4 >>= 1; }

			if (C1 == 0) { *(dst + x + y * 64) = 0; continue; }

			//C2 = C1; 

			if (!C2) C2 = C1;
			if (!C3) C3 = C1;
			if (!C4) C4 = C1;

			int B = (((C1 >> 0) & 31) + ((C2 >> 0) & 31) + ((C3 >> 0) & 31) + ((C4 >> 0) & 31) + 2) >> 2;
			int G = (((C1 >> 5) & 31) + ((C2 >> 5) & 31) + ((C3 >> 5) & 31) + ((C4 >> 5) & 31) + 2) >> 2;
			int R = (((C1 >> 10) & 31) + ((C2 >> 10) & 31) + ((C3 >> 10) & 31) + ((C4 >> 10) & 31) + 2) >> 2;
			if (!HARD3D) *(dst + x + y * 64) = HiColor(R, G, B) * 2;
			else *(dst + x + y * 64) = HiColor(R, G, B);
		}
}



void GetObjectCaracteristics(TModel* mptr, int& ylo, int& yhi)
{
	ylo = 10241024;
	yhi = -10241024;
	for (int v = 0; v < mptr->VCount; v++) {
		if (mptr->gVertex[v].y < ylo) ylo = (int)mptr->gVertex[v].y;
		if (mptr->gVertex[v].y > yhi) yhi = (int)mptr->gVertex[v].y;
	}
	if (yhi < ylo) yhi = ylo + 1;
}



void GenerateAlphaFlags(TModel& model)
{
#ifdef _d3d  
	bool usesOpacityMap = false;
	uint16_t* tptr = model.texture.getData();

	for (size_t w = 0; w < model.gFace.size(); w++) {
		if ((model.gFace[w].Flags & sfOpacity) > 0)
			usesOpacityMap = true;
	}

	if (usesOpacityMap) {
		for (int w = 0; w < 256 * 256; w++) {
			if (tptr[w] > 0)
				tptr[w] |= 0x8000;
		}
	}
	else {
		for (int w = 0; w < 256 * 256; w++) {
			tptr[w] |= 0x8000;
		}
	}

	if (!model.texture.hasMipmap()) return;
	tptr = model.texture.getMipmap().getData();

	if (usesOpacityMap) {
		for (int w = 0; w < 128 * 128; w++)
			if ((tptr[w]) > 0) tptr[w] += 0x8000;
	}
	else
		for (int w = 0; w < 128 * 128; w++)
			tptr[w] |= 0x8000;

	if (!model.texture.getMipmap().hasMipmap()) return;
	tptr = model.texture.getMipmap().getMipmap().getData();

	if (usesOpacityMap) {
		for (int w = 0; w < 64 * 64; w++)
			if (tptr[w] > 0) tptr[w] += 0x8000;
	}
	else
		for (int w = 0; w < 64 * 64; w++)
			tptr[w] |= 0x8000;

#endif  
}




void GenerateModelMipMaps(TModel& model)
{
	if (!model.texture.hasMipmap()) {
		model.texture.generateMipMap();
		model.texture.getMipmap().generateMipMap();
	}
}


void GenerateMapImage()
{
	TTexture* mm1, * mm2;
	int YShift = 23;
	int XShift = 11;
	int lsw = MapPic.W;

	for (int y = 0; y < 256; y++)
		for (int x = 0; x < 256; x++) {
			size_t t = TMap1[y << 1][x << 1];
			if (t >= Textures.size()) {
				continue;
			}

			uint16_t c = 0x0000;
			mm1 = &Textures.at(t).getMipmap().getMipmap();
			mm2 = &Textures.at(t).getMipmap().getMipmap().getMipmap();

			if (t)
				c = mm1->getData()[(y & 31) * 32 + (x & 31)];
			else
				c = mm2->getData()[(y & 15) * 16 + (x & 15)];

			if (!HARD3D)
				c = c >> 1;
			else
				c = conv_565(c);

			*((uint16_t*)MapPic.lpImage + (y + YShift) * lsw + x + XShift) = c;
		}
}



void ReleaseResources()
{
	Textures.clear();
	MObjects.clear();
	Ambient.clear();
	RandSound.clear();
}



void LoadResources()
{
	int tc = 0, mc = 0;
	std::stringstream sz;

	sz << ProjectName << ".map";
	std::string MapName = sz.str();
	sz.str("");  sz.clear();
	sz << ProjectName << ".rsc";
	std::string RscName = sz.str();
	sz.str(""); sz.clear();

	ReleaseResources();

	std::cout << "LoadResources(\'" << RscName << "\')" << std::endl;

	std::ifstream file(RscName, std::ios::binary);

	if (!file.is_open()) {
		sz << "Error opening resource file: \'" << RscName << "\'";
		throw dohalt(sz.str(), __FUNCTION__, __FILE__, __LINE__);
		return;
	}

	file.read((char*)&tc, 4);
	file.read((char*)&mc, 4);

	file.read((char*)&SkyR, 4);
	file.read((char*)&SkyG, 4);
	file.read((char*)&SkyB, 4);

	file.read((char*)&SkyTR, 4);
	file.read((char*)&SkyTG, 4);
	file.read((char*)&SkyTB, 4);

	for (int tt = 0; tt < tc; tt++) {
		TTexture tex;
		LoadTexture(file, tex);
		for (unsigned i = 0; i < 128 * 128; i++)
			if (!tex.getData()[i]) tex.getData()[i] = 1;
		ApplyAlphaFlags(tex.getData(), 128 * 128);
		ApplyAlphaFlags(tex.getMipmap().getData(), 64 * 64);
		ApplyAlphaFlags(tex.getMipmap().getMipmap().getData(), 32 * 32);
		Textures.push_back(tex); // This uses a C++ Move constructor
	}


	if (mc < 256)
	for (int mm = 0; mm < mc; mm++) {
		TObject object;

		file.read((char*)&object.info, 64);
		object.info.Radius *= 2;
		object.info.YLo *= 2;
		object.info.YHi *= 2;
		object.info.linelenght = (object.info.linelenght / 128) * 128;
		LoadModel(file, object.model);
		if (object.info.flags & ofANIMATED)
			LoadAnimation(file, object.vtl);
		//GenerateModelMipMaps(object.model);
		GenerateAlphaFlags(object.model);

		MObjects.push_back(object);
	}

	LoadSky(file);
	LoadSkyMap(file);

	int FgCount;
	file.read((char*)&FgCount, 4);
	file.read((char*)&FogsList[1], FgCount * sizeof(TFogEntity));

	int RdCount, AmbCount;

	std::cout << "Loading random sounds...";
	file.read((char*)&RdCount, 4);
	for (int r = 0; r < RdCount; r++) {
		TSoundFX sfx;

		file.read((char*)&sfx.length, 4);
		sfx.lpData = new int16_t[sfx.length];
		file.read((char*)sfx.lpData, sfx.length);

		RandSound.push_back(sfx);
	}
	std::cout << "Ok!" << std::endl;

	std::cout << "Loading ambient sounds...";
	file.read((char*)&AmbCount, 4);
	for (int a = 0; a < AmbCount; a++) {
		TAmbient amb;

		file.read((char*)&amb.sfx.length, 4);
		amb.sfx.lpData = new int16_t[amb.sfx.length];
		file.read((char*)amb.sfx.lpData, amb.sfx.length);

		file.read((char*)amb.rdata, sizeof(amb.rdata));
		file.read((char*)&amb.RSFXCount, 4L);
		file.read((char*)&amb.AVolume, 4);

		if (amb.RSFXCount)
			amb.RndTime = (amb.rdata[0].RFreq / 2 + rRand(amb.rdata[0].RFreq)) * 1000;

		Ambient.push_back(amb);
	}
	std::cout << "Ok!" << std::endl;

	file.close();

	if (!Textures.empty()) {
		CopyTexture.copy(Textures.at(0));
	}

	//================ Load MAPs file ==================//
	file.open(MapName, std::ios::binary);

	std::cout << "Loading map file" << std::endl;

	if (!file.is_open()) {
		std::stringstream ss;
		ss << "Error opening map file: \'" << MapName << "\'";
		throw dohalt(ss.str(), __FUNCTION__, __FILE__, __LINE__);
		return;
	}

	file.read((char*)HMap, 512 * 512);
	file.read((char*)TMap1, 512 * 512);
	file.read((char*)TMap2, 512 * 512);
	file.read((char*)OMap, 512 * 512);
	file.read((char*)FMap, 512 * 512);
	file.read((char*)LMap, 512 * 512);
	file.read((char*)HMap2, 512 * 512);
	file.read((char*)HMapO, 512 * 512);
	file.read((char*)FogsMap, 256 * 256);
	file.read((char*)AmbMap, 256 * 256);

	file.close();

	if (FogsList[1].YBegin > 1.f)
		for (int x = 0; x < 255; x++)
			for (int y = 0; y < 255; y++)
				if (!FogsMap[y][x])
					if (HMap[y * 2 + 0][x * 2 + 0] < FogsList[1].YBegin || HMap[y * 2 + 0][x * 2 + 1] < FogsList[1].YBegin || HMap[y * 2 + 0][x * 2 + 2] < FogsList[1].YBegin ||
						HMap[y * 2 + 1][x * 2 + 0] < FogsList[1].YBegin || HMap[y * 2 + 1][x * 2 + 1] < FogsList[1].YBegin || HMap[y * 2 + 1][x * 2 + 2] < FogsList[1].YBegin ||
						HMap[y * 2 + 2][x * 2 + 0] < FogsList[1].YBegin || HMap[y * 2 + 2][x * 2 + 1] < FogsList[1].YBegin || HMap[y * 2 + 2][x * 2 + 2] < FogsList[1].YBegin)
						FogsMap[y][x] = 1;


	//======== load calls ==============//
	std::cout << "Loading animal call sounds...";
	char name[128];
	wsprintf(name, "HUNTDAT/SOUNDFX/CALLS/call%d_a.wav", TargetDino + 1);
	LoadWav(name, fxCall[0]);

	wsprintf(name, "HUNTDAT/SOUNDFX/CALLS/call%d_b.wav", TargetDino + 1);
	LoadWav(name, fxCall[1]);

	wsprintf(name, "HUNTDAT/SOUNDFX/CALLS/call%d_c.wav", TargetDino + 1);
	LoadWav(name, fxCall[2]);

	std::cout << "Ok!" << std::endl;

	switch (TargetWeapon) {
	case 0:
		LoadCharacterInfo(Weapon.chinfo, "HUNTDAT/WEAPON1.CAR");
		LoadPictureTGA(BulletPic, "HUNTDAT/MENU/WEPPIC/bullet1.tga");
		break;
	case 1:
		LoadCharacterInfo(Weapon.chinfo, "HUNTDAT/WEAPON2.CAR");
		LoadPictureTGA(BulletPic, "HUNTDAT/MENU/WEPPIC/bullet2.tga");
		break;
	case 2:
		LoadCharacterInfo(Weapon.chinfo, "HUNTDAT/WEAPON3.CAR");
		LoadPictureTGA(BulletPic, "HUNTDAT/MENU/WEPPIC/bullet3.tga");
		break;
	case 3:
		LoadCharacterInfo(Weapon.chinfo, "HUNTDAT/WEAPON4.CAR");
		LoadPictureTGA(BulletPic, "HUNTDAT/MENU/WEPPIC/bullet4.tga");
		break;
	}


	conv_pic(BulletPic);

	//======= Post load rendering ==============//
	CreateTMap();
	RenderLightMap();
	PlaceHunter();

	LoadPictureTGA(MapPic, "HUNTDAT/MENU/mapframe.tga");
	conv_pic(MapPic);

	if (TrophyMode)	PlaceTrophy();
	else PlaceCharacters();
	GenerateMapImage();

	if (TrophyMode) LoadPictureTGA(TrophyPic, "HUNTDAT/MENU/trophy.tga");
	else LoadPictureTGA(TrophyPic, "HUNTDAT/MENU/trophy_g.tga");
	conv_pic(TrophyPic);



	LockLanding = FALSE;
	Wind.alpha = rRand(1024) * 2.f * pi / 1024.f;
	Wind.speed = 10;
	MyHealth = MAX_HEALTH;
	ShotsLeft = WeapInfo[TargetWeapon].Shots;
	if (ObservMode) ShotsLeft = 0;

	Weapon.state = 0;
	Weapon.FTime = 0;
	PlayerAlpha = 0;
	PlayerBeta = 0;

	ExpCount = 0;
	BINMODE = false;
	OPTICMODE = false;
	EXITMODE = false;
	PAUSE = false;

	Ship.pos.x = PlayerX;
	Ship.pos.z = PlayerZ;
	Ship.pos.y = GetLandUpH(Ship.pos.x, Ship.pos.z) + 2048;
	Ship.State = -1;
	Ship.tgpos.x = Ship.pos.x;
	Ship.tgpos.z = Ship.pos.z + 60 * 256;
	Ship.cindex = -1;
	Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z) + 2048;
	ShipTask.tcount = 0;

	if (!TrophyMode) {
		TrophyRoom.Last.smade = 0;
		TrophyRoom.Last.ssucces = 0;
		TrophyRoom.Last.path = 0;
		TrophyRoom.Last.time = 0;
	}

	DemoPoint.DemoTime = 0;
	RestartMode = false;
	TrophyTime = 0;
	answtime = 0;
	ExitTime = 0;
}


TModel::~TModel()
{
	this->reset();
}


void TModel::reset()
{
	this->VCount = 0;
	this->FCount = 0;
	this->TextureSize = 0;
	this->TextureHeight = 0;
	this->gVertex.clear();
	this->gFace.clear();
	this->texture.reset();
	this->VLight.clear();
}


TCharacterInfo::~TCharacterInfo()
{
	this->reset();
}


void TCharacterInfo::reset()
{
	this->ModelName = "";
	this->AniCount = 0;
	this->SfxCount = 0;
	this->Model.reset();
	this->Animation.clear();
	this->SoundFX.clear();
}

void LoadCharacterInfo(TCharacterInfo& chinfo, const std::string& FName)
{
	chinfo.reset();

	std::ifstream file(FName, std::ios::binary);
	if (!file.is_open()) {
		std::stringstream sz;
		sz << "Error opening character file: " << FName;
		throw dohalt(sz.str(), __FUNCTION__, __FILE__, __LINE__);
		return;
	}

	char mname[32];

	file.read(mname, 32);
	chinfo.ModelName = mname; // Convert 32-byte c-string to C++ string

	//============= read model =================//

	file.read((char*)&chinfo.AniCount, 4);
	file.read((char*)&chinfo.SfxCount, 4);
	file.read((char*)&chinfo.Model.VCount, 4);
	file.read((char*)&chinfo.Model.FCount, 4);
	file.read((char*)&chinfo.Model.TextureSize, 4);

	// -- Faces
	chinfo.Model.gFace.resize(chinfo.Model.FCount);
	file.read((char*)chinfo.Model.gFace.data(), chinfo.Model.gFace.size() * sizeof(TFace));

	// -- Vertices
	chinfo.Model.VLight.resize(chinfo.Model.VCount);
	chinfo.Model.gVertex.resize(chinfo.Model.VCount);
	file.read((char*)chinfo.Model.gVertex.data(), chinfo.Model.gVertex.size() * sizeof(TPoint3d));

	CorrectModel(chinfo.Model);

	int ts = chinfo.Model.TextureSize;

	if (HARD3D)
		chinfo.Model.TextureHeight = 256;
	else
		chinfo.Model.TextureHeight = (chinfo.Model.TextureSize / 2) / 256;

	chinfo.Model.TextureSize = chinfo.Model.TextureHeight * 512;

	LoadTexture(file, 256, 256, ts, chinfo.Model.texture);

	//GenerateModelMipMaps(chinfo.Model);
	GenerateAlphaFlags(chinfo.Model);
	//ApplyAlphaFlags(chinfo.mptr->lpTexture, 256*256);
	//ApplyAlphaFlags(chinfo.mptr->lpTexture2, 128*128);
//============= read animations =============//
	//chinfo.Animation.reserve(chinfo.AniCount);
	//fread((char*)chinfo.Animation.data(), chinfo.AniCount * sizeof(TAni));
	try {
		for (int a = 0; a < chinfo.AniCount; a++) {
			TAni animation;

			file.read(animation.aniName, 32);
			file.read((char*)&animation.aniKPS, 4);
			file.read((char*)&animation.FramesCount, 4);

			if (animation.aniKPS != 0)
				animation.AniTime = (animation.FramesCount * 1000) / animation.aniKPS;
			else
				animation.AniTime = 0;
		
			animation.aniDataLength = (chinfo.Model.VCount * 3) * animation.FramesCount;
		
			animation.aniData = new int16_t[animation.aniDataLength];

			file.read((char*)animation.aniData, ((chinfo.Model.VCount * 6) * animation.FramesCount));

			chinfo.Animation.push_back(animation);
		}
	}
	catch (std::exception& e) {
		std::stringstream ess;
		ess << "C++ Exception: " << e.what();
		MessageBox(HWND_DESKTOP, ess.str().c_str(), "Exception!", MB_OK | MB_ICONERROR);
	}

	//============= read sound fx ==============//
	char tmp[32];
	for (int s = 0; s < chinfo.SfxCount; s++) {
		TSoundFX sfx;

		file.read((char*)tmp, 32);
		file.read((char*)&sfx.length, 4);
		sfx.lpData = new int16_t[sfx.length];
		file.read((char*)sfx.lpData, sfx.length);

		chinfo.SoundFX.push_back(sfx);
	}

	for (int v = 0; v < chinfo.Model.VCount; v++) {
		chinfo.Model.gVertex[v].x *= 2.f;
		chinfo.Model.gVertex[v].y *= 2.f;
		chinfo.Model.gVertex[v].z *= -2.f;
	}

	file.read((char*)chinfo.Anifx.data(), 64 * sizeof(int32_t));

	file.close();
}


void ScrollWater()
{
#if defined(_soft)
	int WaterShift = RealTime / 40;
	int xpos = (-WaterShift) & 127;

	for (int y = 0; y < 128; y++) {
		int ypos = (y - WaterShift * 2) & 127;

		memcpy((void*)&(Textures.at(0).getData())[y * 128],
			(void*)&(CopyTexture.getData())[ypos * 128 + xpos], (128 - xpos) * 2);

		if (xpos)
			memcpy((void*)&(Textures.at(0).getData())[y * 128 + (128 - xpos)],
			(void*)&(CopyTexture.getData())[ypos * 128], xpos * 2);
	}

	xpos /= 2;

	for (int y = 0; y < 64; y++) {
		int ypos = (y * 2 - WaterShift * 2) & 127;
		ypos /= 2;

		memcpy((void*)&Textures[0]->DataB[y * 64],
			(void*)&Textures[255]->DataB[ypos * 64 + xpos], (64 - xpos) * 2);

		if (xpos)
			memcpy((void*)&Textures[0]->DataB[y * 64 + (64 - xpos)],
			(void*)&Textures[255]->DataB[ypos * 64], xpos * 2);
	}

	xpos /= 2;

	for (int y = 0; y < 32; y++) {
		int ypos = (y * 4 - WaterShift * 2) & 127;
		ypos /= 4;

		memcpy((void*)&Textures[0]->DataC[y * 32],
			(void*)&Textures[255]->DataC[ypos * 32 + xpos], (32 - xpos) * 2);

		if (xpos)
			memcpy((void*)&Textures[0]->DataC[y * 32 + (32 - xpos)],
			(void*)&Textures[255]->DataC[ypos * 32], xpos * 2);
	}
#endif
}


//================ light map ========================//

void FillVector(int x, int y, Vector3d& v)
{
	v.x = (float)x * 256;
	v.z = (float)y * 256;
	v.y = (float)((int)HMap[y][x]) * ctHScale;
}

BOOL TraceVector(Vector3d v, Vector3d lv)
{
	v.y += 4;
	NormVector(lv, 64);
	for (int l = 0; l < 32; l++) {
		v.x -= lv.x; v.y -= lv.y / 6; v.z -= lv.z;
		if (v.y > 255 * ctHScale) return TRUE;
		if (GetLandH(v.x, v.z) > v.y) return FALSE;
	}
	return TRUE;
}


void AddShadow(int x, int y, int d)
{
	if (x < 0 || y < 0 || x>511 || y>511) return;
	LMap[y][x] += d;
	if (LMap[y][x] > 56) LMap[y][x] = 56;
}

void RenderShadowCircle(int x, int y, int R, int D)
{
	int cx = x / 256;
	int cy = y / 256;
	int cr = 1 + R / 256;
	for (int yy = -cr; yy <= cr; yy++)
		for (int xx = -cr; xx <= cr; xx++) {
			int tx = (cx + xx) * 256;
			int ty = (cy + yy) * 256;
			int r = (int)sqrt((tx - x) * (tx - x) + (ty - y) * (ty - y));
			if (r > R) continue;
			AddShadow(cx + xx, cy + yy, D * (R - r) / R);
		}
}

void RenderLightMap()
{

	Vector3d lv;
	int x, y;

	lv.x = -412;
	lv.z = -412;
	lv.y = -1024;
	NormVector(lv, 1.0f);

	for (y = 1; y < ctMapSize - 1; y++)
		for (x = 1; x < ctMapSize - 1; x++) {
			int ob = OMap[y][x];
			if (ob == 255) continue;

			int l = MObjects[ob].info.linelenght / 128;
			if (l > 0) RenderShadowCircle(x * 256 + 128, y * 256 + 128, 256, MObjects[ob].info.lintensity / 2);
			for (int i = 1; i < l; i++) {
				AddShadow(x + i, y + i, MObjects[ob].info.lintensity);
			}

			l = MObjects[ob].info.linelenght * 2;
			RenderShadowCircle(x * 256 + 128 + l, y * 256 + 128 + l,
				MObjects[ob].info.circlerad * 2,
				MObjects[ob].info.cintensity);
		}

}





void SaveScreenShot()
{

	HANDLE hf;                  /* file handle */
	BITMAPFILEHEADER hdr;       /* bitmap file-header */
	BITMAPINFOHEADER bmi;       /* bitmap info-header */
	DWORD dwTmp;


	//MessageBeep(0xFFFFFFFF);
	CopyHARDToDIB();

	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biWidth = WinW;
	bmi.biHeight = WinH;
	bmi.biPlanes = 1;
	bmi.biBitCount = 24;
	bmi.biCompression = BI_RGB;

	bmi.biSizeImage = WinW * WinH * 3;
	bmi.biClrImportant = 0;
	bmi.biClrUsed = 0;



	hdr.bfType = 0x4d42;
	hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) +
		bmi.biSize + bmi.biSizeImage);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) +
		bmi.biSize;


	char t[12];
	wsprintf(t, "HUNT%004d.BMP", ++_shotcounter);
	hf = CreateFile(t,
		GENERIC_READ | GENERIC_WRITE,
		(DWORD)0,
		(LPSECURITY_ATTRIBUTES)NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)NULL);



	WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), (LPDWORD)&dwTmp, (LPOVERLAPPED)NULL);

	WriteFile(hf, &bmi, sizeof(BITMAPINFOHEADER), (LPDWORD)&dwTmp, (LPOVERLAPPED)NULL);

	byte fRGB[1024][3];

	for (int y = 0; y < WinH; y++) {
		for (int x = 0; x < WinW; x++) {
			WORD C = *((WORD*)lpVideoBuf + (WinEY - y) * 1024 + x);
			fRGB[x][0] = (C & 31) << 3;
			if (HARD3D) {
				fRGB[x][1] = ((C >> 5) & 63) << 2;
				fRGB[x][2] = ((C >> 11) & 31) << 3;
			}
			else {
				fRGB[x][1] = ((C >> 5) & 31) << 3;
				fRGB[x][2] = ((C >> 10) & 31) << 3;
			}
		}
		WriteFile(hf, fRGB, 3 * WinW, &dwTmp, NULL);
	}

	CloseHandle(hf);
	//MessageBeep(0xFFFFFFFF);
}



std::streambuf* cout_prev = nullptr;
std::ofstream log_file;

void CreateLog()
{
#if !defined(_DEBUG)
	log_file.open("carnivores.log", std::ios::out | std::ios::trunc);

	if (!log_file.is_open())
	{
		std::cerr << "Failed to open the \'carnivores.log\' file for writing!" << std::endl;
		throw std::runtime_error("Unable to create/open the 'carnivores.log' file for logging.");
		return;
	}

	cout_prev = std::cout.rdbuf(log_file.rdbuf());
#endif

#if defined(_d3d)
	std::cout << "Carnivores D3D7. Build v1." << CARN_BUILDVERSION << ". " << __DATE__ << std::endl;
#elif defined(_3dfx)
	std::cout << "Carnivores 3DFX. Build v1." << CARN_BUILDVERSION << ". " << __DATE__ << std::endl;
#elif defined(_soft)
	std::cout << "Carnivores Soft. Build v1." << CARN_BUILDVERSION << ". " << __DATE__ << std::endl;
#endif
}

void CloseLog()
{
#if defined(_DEBUG)
	std::cout.rdbuf(cout_prev);
#endif //_DEBUG

	if (log_file.is_open()) {
		log_file.close();
	}
}
