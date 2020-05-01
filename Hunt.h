
#include "targetver.h"

#include <Windows.h>
#include <winuser.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <fstream>
#include <chrono>

#include "resource.h"

#include <ddraw.h>

#if defined(_d3d)
#include <d3d.h>
#elif defined(_d3d9)
#include <d3d9.h>
#elif defined(_d3d11)
#include <d3d11.h>
#elif defined(_opengl)
#include <gl.h>
#endif

#define CARN_BUILDVERSION 4
#define ctHScale  32
#define PMORPHTIME 256
#define HiColor(R,G,B) ( ((R)<<10) + ((G)<<5) + (B) )

#define TCMAX ((128<<16)-62024)
#define TCMIN ((000<<16)+62024)

#define kfForward     0x00000001
#define kfBackward    0x00000002
#define kfLeft        0x00000004
#define kfRight       0x00000008
#define kfLookUp      0x00000010
#define kfLookDn      0x00000020
#define kfJump        0x00000040
#define kfDown        0x00000080
#define kfCall        0x00000100

#define kfSLeft       0x00001000
#define kfSRight      0x00002000
#define kfStrafe      0x00004000

#define fmWater   0x80
#define fmReverse 0x40
#define fmNOWAY   0x20

#define sfDoubleSide         1
#define sfDarkBack           2
#define sfOpacity            4
#define sfTransparent        8
#define sfMortal        0x0010

#define sfNeedVC        0x0080
#define sfDark          0x8000

#define ofPLACEWATER       1
#define ofPLACEGROUND      2
#define ofPLACEUSER        4
#define ofANIMATED         0x80000000

#define csONWATER          0x00010000
#define MAX_HEALTH         128000

enum PlayerRankEnum {
	RANK_NOVICE = 0,
	RANK_ADVANCED = 1,
	RANK_MASTER = 2
};

enum CreatureTypeEnum {
	CTYPE_HUNTER = 0,
	CTYPE_MOSCH = 1,
	CTYPE_GALLI = 2,
	CTYPE_DIMOR = 3,
	//CTYPE_SHIP = 4,
	CTYPE_PARA = 5,
	CTYPE_PACHY = 6,
	CTYPE_STEGO = 7,
	CTYPE_ALLO = 8,
	CTYPE_TRIKE = 9,
	CTYPE_VELOCI = 10,
	CTYPE_TREX = 11,
	CTYPE_MAX
};

#define HUNT_EAT      0
#define HUNT_BREATH   1
#define HUNT_FALL     2
#define HUNT_KILL     3 


#ifdef _MAIN_
#define _EXTORNOT 
#else
#define _EXTORNOT extern
#endif

#define ctMapSize 512



class dohalt : public std::exception {
private:
	std::string _funcname;
	std::string _file;
	size_t _line;
	std::string _what;

public:
	dohalt(std::string what = "", std::string funcname = "", std::string file = "", size_t line = 0) :
		_funcname(funcname),
		_file(file),
		_line(line),
		_what(what)
	{
	}

	dohalt(const dohalt& o) :
		_funcname(o._funcname),
		_file(o._file),
		_line(o._line),
		_what(o._what)
	{
	}

	std::string function() {
		return _funcname;
	}

	std::string file() {
		return _file;
	}

	size_t line() {
		return _line;
	}

	std::string what() {
		return _what;
	}
};


class CarnivoresEngine {
public:

	CarnivoresEngine();
	~CarnivoresEngine();
};

class RenderEngine {
public:

	RenderEngine() {}
	~RenderEngine() {}
};

class AudioEngine {
public:

	AudioEngine() {}
	~AudioEngine() {}
};


typedef struct tagMessageList {
	int64_t timeleft;
	std::string mtext;
} TMessageList;

typedef struct tagTRGB {
	uint8_t B;
	uint8_t G;
	uint8_t R;
} TRGB;

class TAni {
public:
	char		aniName[32];
	int			aniKPS,
				FramesCount,
				AniTime;
	int16_t*	aniData;
	size_t		aniDataLength;

	TAni() {
		std::memset(aniName, 0, 32);
		aniKPS = 0;
		FramesCount = 0;
		AniTime = 0;
		aniData = nullptr;
		aniDataLength = 0;
	}

	TAni(const TAni& rhs) noexcept {
		std::memcpy(aniName, rhs.aniName, 32);
		aniKPS = rhs.aniKPS;
		FramesCount = rhs.FramesCount;
		AniTime = rhs.AniTime;
		aniData = new int16_t[rhs.aniDataLength];
		aniDataLength = rhs.aniDataLength;

		std::memcpy(aniData, rhs.aniData, aniDataLength);
	}

	TAni(TAni&& rhs) noexcept {
		std::memcpy(aniName, rhs.aniName, 32);
		aniKPS = rhs.aniKPS;
		FramesCount = rhs.FramesCount;
		AniTime = rhs.AniTime;
		aniData = rhs.aniData;
		aniDataLength = rhs.aniDataLength;
		rhs.aniData = nullptr;
	}

	~TAni() {
		if (aniData) {
			delete[] aniData;
		}
	}
};

class TVTL {
public:
	int aniKPS;
	int FramesCount;
	int AniTime;
	int16_t* aniData;
	size_t aniDataLength;

	TVTL() : aniKPS(0), FramesCount(0), AniTime(0), aniData(nullptr), aniDataLength(0) {}

	TVTL(const TVTL& rhs) noexcept {
		aniKPS = rhs.aniKPS;
		FramesCount = rhs.FramesCount;
		AniTime = rhs.AniTime;
		aniDataLength = rhs.aniDataLength;
		if (rhs.aniData != nullptr) {
			aniData = new int16_t[FramesCount * (sizeof(int16_t) * 3)];
			std::memcpy(aniData, rhs.aniData, FramesCount * (sizeof(int16_t) * 3));
		}
		else {
			aniData = nullptr;
		}
	}

	TVTL(TVTL&& rhs) noexcept {
		aniKPS = rhs.aniKPS;
		FramesCount = rhs.FramesCount;
		AniTime = rhs.AniTime;
		aniData = rhs.aniData;
		aniDataLength = rhs.aniDataLength;
		rhs.aniData = nullptr;
	}

	~TVTL() {
		if (aniData) {
			delete[] aniData;
		}
	}
};

class TSoundFX {
public:

	int  length;
	int16_t* lpData;

	TSoundFX() : length(0), lpData(nullptr) {}
	
	TSoundFX(const TSoundFX& rhs) {
		length = rhs.length;
		lpData = new int16_t[rhs.length];
		
		std::memcpy(lpData, rhs.lpData, length);
	}

	TSoundFX(TSoundFX&& rhs) noexcept {
		length = rhs.length;
		lpData = rhs.lpData;
		rhs.lpData = nullptr;
	}

	~TSoundFX() {
		if (lpData)
			delete[] lpData;
	}
};

class TText {
public:
	int  Lines;
	std::array<std::string, 64>	Text;
};


class TRD {
public:
	int RNumber, RVolume, RFreq, RTmp;

	TRD() : RNumber(0), RVolume(0), RFreq(0), RTmp(0) {}

	TRD(const TRD& rhs) {
		RNumber = rhs.RNumber;
		RVolume = rhs.RVolume;
		RFreq = rhs.RFreq;
		RTmp = rhs.RTmp;
	}
};

class TAmbient {
public:
	TSoundFX sfx;
	TRD  rdata[16];
	int  RSFXCount;
	int  AVolume;
	int  RndTime;

	TAmbient() {
		sfx.length = 0;
		sfx.lpData = nullptr;
		RSFXCount = 0;
		AVolume = 0;
		RndTime = 0;
	}

	TAmbient(const TAmbient& rhs) noexcept {
		sfx.length = rhs.sfx.length;
		sfx.lpData = new int16_t[sfx.length];
		std::memcpy(sfx.lpData, rhs.sfx.lpData, sfx.length);
		for (int r = 0; r < 16; ++r) {
			rdata[r].RNumber = rhs.rdata[r].RNumber;
			rdata[r].RVolume = rhs.rdata[r].RVolume;
			rdata[r].RFreq = rhs.rdata[r].RFreq;
			rdata[r].RTmp = rhs.rdata[r].RTmp;
		}
		RSFXCount = rhs.RSFXCount;
		AVolume = rhs.AVolume;
		RndTime = rhs.RndTime;
	}

	TAmbient(TAmbient&& rhs) noexcept {
		sfx.length = rhs.sfx.length;
		sfx.lpData = rhs.sfx.lpData;
		rhs.sfx.lpData = nullptr;
		for (int r = 0; r < 16; ++r) {
			rdata[r].RNumber = rhs.rdata[r].RNumber;
			rdata[r].RVolume = rhs.rdata[r].RVolume;
			rdata[r].RFreq = rhs.rdata[r].RFreq;
			rdata[r].RTmp = rhs.rdata[r].RTmp;
		}
		RSFXCount = rhs.RSFXCount;
		AVolume = rhs.AVolume;
		RndTime = rhs.RndTime;
	}
};


class TTexture {
protected:

	int W, H;
	uint16_t* data;
	TTexture* mipmap; // NOTE: Needed for RenderSoft

public:

	TTexture(int w = 0, int h = 0, uint16_t* d = nullptr) : W(w), H(h), data(d), mipmap(nullptr) {}
	
	TTexture(const TTexture& rhs) noexcept : W(0), H(0), data(nullptr), mipmap(nullptr) { copy(rhs); }
	
	TTexture(TTexture&& rhs) noexcept : W(rhs.W), H(rhs.H), data(rhs.data), mipmap(rhs.mipmap) {
		rhs.data = nullptr;
		rhs.mipmap = nullptr;
	}
	
	~TTexture() {
		reset();
	}

	void reset() {
		if (data) {
			delete[] data;
		}
		if (mipmap) {
			delete mipmap;
		}

		W = 0;
		H = 0;
		data = nullptr;
		mipmap = nullptr;
	}

	/*
	Store the width and height in the two parameter references
	*/
	void getDimensions(int& w, int& h) {
		w = W;
		h = H;
	}

	int getWidth() {
		return W;
	}

	int getHeight() {
		return H;
	}

	/*
	UNSAFE!
	Return a pointer to the internal data buffer
	*/
	uint16_t* getData() {
		return data;
	}
	
	TTexture& getMipmap() {
		if (!mipmap)
			return *this;
		else
			return *mipmap;
	}

	bool hasMipmap() {
		return (mipmap != nullptr);
	}

	/*
	Copy a buffer of uint16_t's into the instance
	*/
	void copy(int w, int h, uint16_t* d);

	/*
	Copy an existing TTexture object's contents to this instance, including mipmaps if they exist.
	*/
	void copy(const TTexture& t);

	/*
	Generate mipmap data from this texture
	*/
	void generateMipMap();
};


#include "Math.h"

/*
! DEPRECATED !
A class that is used to deal with Vectors typical for 3D mathematics.
NOTE: This is deprecated and should not be used. See `vec3<T>` instead.
*/
class Vector3d {
public:
	float x, y, z;

	Vector3d(float X = 0.0f, float Y = 0.0f, float Z = 0.0f) : x(X), y(Y), z(Z) {}
	Vector3d(const Vector3d& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) {}

	void operator= (const Vector3d& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
	}

	Vector3d operator+ (const Vector3d& rhs) {
		Vector3d v(x + rhs.x, y + rhs.y, z + rhs.z);
		return v;
	}

	Vector3d operator- (const Vector3d& rhs) {
		Vector3d v(x - rhs.x, y - rhs.y, z - rhs.z);
		return v;
	}

	Vector3d operator* (float rhs) {
		Vector3d v(x * rhs, y * rhs, z * rhs);
		return v;
	}
};

class TPoint3di {
public:
	int32_t x, y, z;

	TPoint3di(int32_t x = 0, int32_t y = 0, int32_t z = 0) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	TPoint3di(const TPoint3di& v) : x(v.x), y(v.y), z(v.z) {
	}

	void operator= (const TPoint3di& v) {
		x = v.x;
		y = v.y;
		z = v.z;
	}

	TPoint3di operator+ (const TPoint3di& v) {
		TPoint3di v2(x + v.x, y + v.y, z + v.z);
		return v2;
	}

	TPoint3di operator- (const TPoint3di& v) {
		TPoint3di v2(x - v.x, y - v.y, z - v.z);
		return v2;
	}
};

typedef struct TagVector2di {
	int x, y;
} Vector2di;

typedef struct TagVector2df {
	float x, y;
} Vector2df;


typedef struct TagScrPoint {
	int x, y, tx, ty,
		Light, z, r2, r3;
} ScrPoint;

typedef struct TagMScrPoint {
	int x, y, tx, ty;
} MScrPoint;

typedef struct tagClipPlane {
	Vector3d v1, v2, nv;
} CLIPPLANE;





typedef struct TagEPoint {
	Vector3d v;
	int  DFlags, scrx, scry, Light;
	float Fog;
} EPoint;


typedef struct TagClipPoint {
	EPoint ev;
	float tx, ty;
} ClipPoint;


//================= MODEL ========================
class TPoint3d {
public:
	float x;
	float y;
	float z;
	int16_t owner;
	int16_t hide; // Ignored in game, only used in Editors

	TPoint3d() : x(0.0f), y(0.0f), z(0.0f), owner(-1), hide(0) {
	}
};


class TFaceI {
public:
	int32_t		v1, v2, v3;
	int32_t		tax, tbx, tcx, tay, tby, tcy;
	uint16_t	Flags,
				DMask;
	int32_t		Distant,
				Next,
				group;
	uint8_t		reserv[12];
};


class TFace {
public:
	int32_t		v1, v2, v3;
	float		tax, tbx, tcx, tay, tby, tcy; // RenderSoft needs these as int32_t types, they are generally 0-256 instead of 0.0f-1.0f so scale floats by 256.f
	uint16_t	Flags,
				DMask;
	int32_t		Distant,
				Next,
				group;
	uint8_t		reserv[12];

	TFace(int V1 = 0, int V2 = 0, int V3 = 0, float TAX = 0.f, float TBX = 0.f, float TCX = 0.f, float TAY = 0.f, float TBY = 0.f, float TCY = 0.f, uint16_t flags = 0, uint16_t dmask = 0, int distant = -1, int next_face = -1, int Group = 0) {
		this->v1 = V1;
		this->v2 = V2;
		this->v3 = V3;
		this->tax = TAX;
		this->tbx = TBX;
		this->tcx = TCX;
		this->tay = TAY;
		this->tby = TBY;
		this->tcy = TCY;
		this->Flags = flags;
		this->DMask = dmask;
		this->Distant = distant;
		this->Next = next_face;
		this->group = Group;
		for (int i = 0; i < 12; ++i)
			reserv[i] = 0;
	}

	TFace(const TFace& f) {
		v1 = f.v1;
		v2 = f.v2;
		v3 = f.v3;
		tax = f.tax;
		tbx = f.tbx;
		tcx = f.tcx;
		tay = f.tay;
		tby = f.tby;
		tcy = f.tcy;
		Flags = f.Flags;
		DMask = f.DMask;
		Distant = f.Distant;
		Next = f.Next;
		group = f.group;
		for (int i = 0; i < 12; ++i)
			reserv[i] = 0;
	}

	TFace(const TFaceI& f) {
		v1 = f.v1;
		v2 = f.v2;
		v3 = f.v3;
		tax = ((float)f.tax) / 256.f;
		tbx = ((float)f.tbx) / 256.f;
		tcx = ((float)f.tcx) / 256.f;
		tay = ((float)f.tay) / 256.f;
		tby = ((float)f.tby) / 256.f;
		tcy = ((float)f.tcy) / 256.f;
		Flags = f.Flags;
		DMask = f.DMask;
		Distant = f.Distant;
		Next = f.Next;
		group = f.group;
		for (int i = 0; i < 12; ++i)
			reserv[i] = 0;
	}
};


typedef struct _Obj {
	std::string OName; // 32-bytes
	float ox;
	float oy;
	float oz;
	int16_t owner;
	int16_t hide;
} TObj;


class TModel {
public:
	int VCount,
		FCount,
		TextureSize,
		TextureHeight;
	std::vector<TPoint3d>	gVertex;
	std::vector<TFace>		gFace;
	TTexture				texture;
#ifdef _d3d
	std::vector<int> VLight;
#else
	std::vector<float> VLight;
#endif

	TModel() {
		VCount = 0;
		FCount = 0;
		TextureSize = 0;
		TextureHeight = 0;
	}

	~TModel();

	void reset();
};


//=========== END MODEL ==============================//


class TObjInfo {
public:
	int  Radius;
	int  YLo, YHi;
	int  linelenght, lintensity;
	int  circlerad, cintensity;
	int  flags;
	int  GrRad;
	int  LastAniTime;
	uint8_t res[24];
};

class TObject {
public:
	TObjInfo	info;
	TModel		model;
	TVTL		vtl;
};


class TCharacterInfo {
public:
	std::string				ModelName;
	int						AniCount,
							SfxCount;
	TModel					Model;
	std::vector<TAni>		Animation;
	std::vector<TSoundFX>	SoundFX;
	std::array<int, 64>		Anifx; // TODO: std::pair<TSFX,int> in SoundFX to reduce vector overhead.

	~TCharacterInfo();

	void reset();
};

typedef struct _TWeapon {
	TCharacterInfo chinfo;
	int state, FTime;
	float shakel;
} TWeapon;



typedef struct _TExplosion {
	Vector3d pos, rpos;
	int EType, FTime;
} TExplosion;




class TCharacter {
public:
	int CType;
	TCharacterInfo* pinfo;
	int StateF;
	int State;
	int NoWayCnt, NoFindCnt, AfraidTime, tgtime;
	int PPMorphTime, PrevPhase, PrevPFTime, Phase, FTime;

	float vspeed, rspeed, bend, scale;
	int Slide;
	float slidex, slidez;
	float tgx, tgz;

	Vector3d pos, rpos;
	float tgalpha, alpha, beta,
		tggamma, gamma,
		lookx, lookz;
	int Health;
};



class TPlayer {
public:
	bool Active;
	unsigned int IPaddr;
	Vector3d pos;
	float alpha, beta, vspeed;
	int kbState;
	std::string NickName;
};


typedef struct _TDemoPoint {
	Vector3d pos;
	int DemoTime,
		CIndex;
} TDemoPoint;

// NOTE: Unused, left over from pre-release
class TLevelDef {
public:

	std::string FileName;
	std::string MapName;
	uint32_t DinosAvail;
	uint16_t* lpMapImage;

	TLevelDef() : FileName(""), MapName(""), DinosAvail(0), lpMapImage(nullptr) {}
	~TLevelDef() {
		if (lpMapImage)
			delete[] lpMapImage;
	}
};


typedef struct tagShipTask {
	int tcount;
	int clist[255];
} TShipTask;

typedef struct tagShip {
	Vector3d pos, rpos, tgpos, retpos;
	float alpha, tgalpha, speed, rspeed, DeltaY;
	int State, cindex, FTime;
} TShip;


typedef struct tagLandingList {
	int PCount;
	std::array<Vector2di, 64> list;
} TLandingList;


class TPlayerR {
public:
	std::string PName; // Limit to 128
	int		RegNumber;
	int		Score,
			Rank;

	TPlayerR() : PName(""), RegNumber(0), Score(0), Rank(0) {}
	~TPlayerR() {
		reset();
	}

	void reset() {
		PName = "";
		RegNumber = 0;
		Score = 0;
		Rank = 0;
	}
};

typedef struct _TTrophyItem {
	int ctype, weapon, phase,
		height, weight, score,
		date, time;
	float scale, range;
	int r1, r2, r3, r4;
} TTrophyItem;


typedef struct _TStats {
	int smade, ssucces;
	float path, time;
} TStats;


class TTrophyRoom { // [128] + 44 + (sizeof(TTrophyItem) * 24)
public:
	std::string PlayerName; // Limit to 128
	int  RegNumber;
	int  Score,
			Rank;

	TStats Last,
		Total;

	TTrophyItem Body[24];

	TTrophyRoom() {
		reset();
	}

	void reset() {
		PlayerName = "";
		RegNumber = -1;
		Score = 0;
		Rank = RANK_NOVICE;
		Last.path = 0.0f;
		Last.time = 0.0f;
		Last.smade = 0;
		Last.ssucces = 0;
		Total.path = 0.0f;
		Total.time = 0.0f;
		Total.smade = 0;
		Total.ssucces = 0;
		for (int b = 0; b < 24; ++b) {
			std::memset(&Body[b], 0, sizeof(TTrophyItem));
		}
	}
};



class TDinoInfo {
public:
	// TODO: Make member variables protected
	std::string Name;
	int Health0;
	bool DangerCall;
	float Mass,
		Length,
		Radius,
		SmellK,
		HearK,
		LookK,
		ShDelta;
	int   Scale0,
		ScaleA,
		BaseScore;

	TDinoInfo() :
		Name(""),
		Health0(0),
		DangerCall(false),
		Mass(0.f),
		Length(0.f),
		Radius(0.f),
		SmellK(1.f),
		HearK(1.f),
		LookK(1.f),
		ShDelta(0.f),
		Scale0(800),
		ScaleA(600),
		BaseScore(0)
	{}
	~TDinoInfo() {}

	const std::string& getName() { return Name; }
	const int& getBaseHealth() { return Health0;  }
	const int& getBaseScale() { return Scale0;  }
	const int& getAddScale() { return ScaleA;  }
	int getMaxScale() { return Scale0 + ScaleA; } // Helper function
	bool isDangerous() { return DangerCall; }
	const float& getMass() { return Mass; }
	const float& getLength() { return Length;  }
	const float& getRadius() { return Radius; }
	const float& getSenseSmell() { return SmellK; }
	const float& getSenseHearing() { return HearK; }
	const float& getSenseSight() { return LookK; }
	const float& getShipDelta() { return ShDelta; }
};


class TWeapInfo {
public:
	std::string Name;
	float Power,
		Prec,
		Loud,
		Rate;
	int Shots;
};



typedef struct _TFogEntity {
	int fogRGB;
	float YBegin;
	bool  Mortal;
	float Transp, FLimit;
} TFogEntity;


typedef struct _TWind {
	float alpha;
	float speed;
	Vector3d nv;
} TWind;


class TPicture {
public:
	int W, H;
	uint16_t* lpImage;

	TPicture() : W(0), H(0), lpImage(nullptr) {}
	~TPicture() {
		if (lpImage) {
			delete[] lpImage;
		}
	}
};




//============= functions ==========================//

// Assembly functions:
void HLineTxB(void);
void HLineTxC(void);
void HLineTxGOURAUD(void);


void HLineTxModel25(void);
void HLineTxModel75(void);
void HLineTxModel50(void);

void HLineTxModel3(void);
void HLineTxModel2(void);
void HLineTxModel(void);

void HLineTDGlass75(void);
void HLineTDGlass50(void);
void HLineTDGlass25(void);
void HLineTBGlass25(void);
// End of Assembly functions

void SetFullScreen();
void SetVideoMode(int, int);
void SetMenuVideoMode();
void CreateDivTable();
void DrawTexturedFace();
int GetTextW(HDC, const char*);
int GetTextW(HDC, const std::string&);
void InitMenu();
void wait_mouse_release();

//============================== render =================================//
void ShowControlElements();
void InsertModelList(TModel* mptr, float x0, float y0, float z0, int light, float al, float bt);
void ProcessMap(int x, int y, int r);

void DrawTPlane(bool);
void DrawTPlaneClip(bool);
void ClearVideoBuf();
void DrawTrophyText(int, int);
void DrawHMap();
void RenderCharacter(int);
void RenderExplosion(int);
void RenderShip();
void RenderPlayer(int);
void RenderSkyPlane();
void RenderHealthBar();
void Render_Cross(int, int);
void Render_LifeInfo(int);

void RenderModel(TModel*, float, float, float, int, float, float);
void RenderModelClipWater(TModel*, float, float, float, int, float, float);
void RenderModelClip(TModel*, float, float, float, int, float, float);
void RenderNearModel(TModel*, float, float, float, int, float, float);
void DrawPicture(int x, int y, TPicture &pic);

void InitClips();
void WaitRetrace();

//============= Characters =======================
void Characters_AddSecondaryOne(int ctype);
void AddDeadBody(TCharacter* cptr, int);
void PlaceCharacters();
void PlaceTrophy();
void AnimateCharacters();
void MakeNoise(Vector3d, float);
void CheckAfraid();
void CreateChMorphedModel(TCharacter&);
void CreateMorphedObject(TModel* mptr, TVTL& vtl, int FTime);
void CreateMorphedModel(TModel* mptr, TAni* aptr, int FTime);

//=============================== Math ==================================//
void  NormVector(Vector3d&, float);
float SGN(float);
void  DeltaFunc(float& a, float b, float d);
float  MulVectorsScal(Vector3d, Vector3d, float&);
Vector3d  MulVectorsVect(Vector3d, Vector3d, Vector3d&);
Vector3d SubVectors(Vector3d, Vector3d);
Vector3d AddVectors(Vector3d, Vector3d);
Vector3d RotateVector(Vector3d&);
float VectorLength(Vector3d);
int   siRand(int);
int   rRand(int);
void  CalcHitPoint(CLIPPLANE&, Vector3d&, Vector3d&, Vector3d&);
void  ClipVector(CLIPPLANE& C, int vn);
float FindVectorAlpha(float, float);
float AngleDifference(float a, float b);

int   TraceShot(float ax, float ay, float az, float& bx, float& by, float& bz);
int   TraceLook(float ax, float ay, float az, float bx, float by, float bz);


void CheckCollision(float&, float&);
float CalcFogLevel(Vector3d v);
//=================================================================//
void AddMessage(const std::string& mt);
void CreateTMap();
void ScrollWater();

void LoadSky(std::ifstream&);
void LoadSkyMap(std::ifstream&);
void LoadTexture(std::ifstream&, TTexture&);
void LoadTexture(std::ifstream&, int, int, int, TTexture&);
void LoadWav(const std::string& FName, TSoundFX &sfx);
void LoadTextFile(TText &txt, const std::string& FName);

WORD conv_565(WORD c);
void conv_pic(TPicture& pic);
void LoadPicture(TPicture& pic, const std::string& pname);
void LoadPictureTGA(TPicture& pic, const std::string& pname);
void LoadCharacterInfo(TCharacterInfo&, const std::string&);
void LoadModelEx(TModel& mptr, const std::string& FName);
void LoadModel(TModel&);
void LoadResources();


void SaveScreenShot();
void CreateWaterTab();
void CreateFadeTab();
void CreateVideoDIB();
void RenderLightMap();

//============ game ===========================//
float GetLandH(float, float);
float GetLandOH(int, int);
float GetLandUpH(float, float);
float GetLandQH(float, float);
float GetLandQHNoObj(float, float);
float GetLandHObj(float, float);

void ProcessSyncro();
void ProcessMenu();
void AddShipTask(int);
void LoadTrophy();
void LoadPlayersInfo();
void SaveTrophy();
void RemoveCurrentTrophy();
void MakeCall();
void MakeShot(float ax, float ay, float az, float bx, float by, float bz);

void AnimateProcesses();
void DoHalt(const char*);
void DoHalt(const std::string&);
void CreateLog();
void CloseLog();
int64_t TimerGetMS();

_EXTORNOT   float BackViewR;
_EXTORNOT   int   BackViewRR;
_EXTORNOT   int   UnderWaterT;



//========== common ==================//
_EXTORNOT   HWND    hwndMain;
_EXTORNOT   HANDLE  hInst;
_EXTORNOT   HDC     hdcMain, hdcCMain;
_EXTORNOT   bool    blActive;
_EXTORNOT   uint8_t KeyboardState[256];
_EXTORNOT   int     KeyFlags, MenuSelect, LastMenuSelect, _shotcounter;

_EXTORNOT   TMessageList MessageList;
_EXTORNOT   std::string	ProjectName; // Limit 128
_EXTORNOT   int     GameState, _GameState, MenuState;
_EXTORNOT   TSoundFX    fxMenuGo, fxMenuMov, fxCall[3], fxScream[4];
_EXTORNOT   TSoundFX    fxMenuAmb, fxUnderwater, fxWaterIn, fxWaterOut, fxJump, fxStep[3], fxRun[3];
//========== map =====================//
_EXTORNOT   uint8_t HMap[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t HMap2[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t HMapO[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t FMap[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t LMap[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t TMap1[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t TMap2[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t OMap[ctMapSize][ctMapSize];

_EXTORNOT   uint8_t FogsMap[256][256];
_EXTORNOT   uint8_t AmbMap[256][256];

_EXTORNOT   TFogEntity  FogsList[256];
_EXTORNOT   TWind       Wind;
_EXTORNOT   TShip       Ship;
_EXTORNOT   TShipTask   ShipTask;

_EXTORNOT   int SkyR, SkyG, SkyB, WaterR, WaterG, WaterB, WaterA,
SkyTR, SkyTG, SkyTB, CurFogColor;
_EXTORNOT   int RandomMap[32][32];

_EXTORNOT   uint16_t SkyPic[256 * 256];
_EXTORNOT   uint16_t SkyFade[9][128 * 128];
_EXTORNOT   uint8_t SkyMap[128 * 128];

_EXTORNOT   std::vector<TTexture>	Textures; // Limit of 256
_EXTORNOT   std::vector<TAmbient>	Ambient; // Limit of 256
_EXTORNOT   std::vector<TSoundFX>	RandSound; // Limit of 256
_EXTORNOT	TTexture				CopyTexture;

//========= GAME ====================//
_EXTORNOT int TargetDino, TargetArea, TargetWeapon,
TrophyTime, ObservMode, Tranq, ShotsLeft, ObjectsOnLook;

_EXTORNOT Vector3d answpos;
_EXTORNOT int answtime;

_EXTORNOT bool
	ScentMode,
	CamoMode,
	RadarMode,
	LockLanding,
	TrophyMode;
_EXTORNOT TTrophyRoom TrophyRoom;
_EXTORNOT TPlayerR PlayerR[16];
_EXTORNOT TPicture LandPic, DinoPic, DinoPicM, MapPic, WepPic;
_EXTORNOT TText DinoText, LandText, WepText,
RadarText, ScentText, ComfText, TranqText, ObserText;
_EXTORNOT HFONT fnt_BIG, fnt_Small, fnt_Midd;
_EXTORNOT TLandingList LandingList;

//======== MODEL ======================//
_EXTORNOT std::vector<TObject>  MObjects; // Limit of 256
_EXTORNOT TModel* mptr; // NOTE: Remove this ASAP!
_EXTORNOT TWeapon Weapon;


_EXTORNOT int   OCount, iModelFade, iModelBaseFade, Current;
_EXTORNOT Vector3d  rVertex[1024];
_EXTORNOT TObj      gObj[1024];
_EXTORNOT Vector2di gScrp[1024];

//============= Characters ==============//
_EXTORNOT TPicture  PausePic, ExitPic, TrophyExit, TrophyPic, BulletPic;
_EXTORNOT TModel SunModel;
_EXTORNOT TModel CompasModel;
_EXTORNOT TModel Binocular;
_EXTORNOT std::array<TDinoInfo, 16> DinoInfo;
_EXTORNOT std::array<TWeapInfo, 8> WeapInfo;
_EXTORNOT TCharacterInfo ShipModel;
_EXTORNOT int ChCount, ExpCount, ShotDino, TrophyBody;
_EXTORNOT TCharacterInfo WindModel;
_EXTORNOT TCharacterInfo ExplodInfo;
_EXTORNOT TCharacterInfo PlayerInfo;
_EXTORNOT std::array<TCharacterInfo, 64> ChInfo;
_EXTORNOT std::array<TCharacter, 256> Characters;
_EXTORNOT std::array<TExplosion, 256> Explosions;
_EXTORNOT TDemoPoint     DemoPoint;

_EXTORNOT std::array<TPlayer, 16> Players[16];
_EXTORNOT Vector3d       PlayerPos, CameraPos;

//========== Render ==================//
_EXTORNOT   LPDIRECTDRAW lpDD;
_EXTORNOT   LPDIRECTDRAW2 lpDD2;
//_EXTORNOT   LPDIRECTINPUT lpDI;

_EXTORNOT   void *lpVideoRAM;
_EXTORNOT   LPDIRECTDRAWSURFACE lpddsPrimary;
_EXTORNOT   bool g_FullScreen, RestartMode;
_EXTORNOT   bool LoDetailSky;
/* The Width and Height of the client area in pixels */
_EXTORNOT   int  WinW, WinH;
/* The right and bottom edge of the client area in pixels */
_EXTORNOT   int  WinEX, WinEY;
/* The center of the client area in pixels*/
_EXTORNOT   int  VideoCX, VideoCY;
_EXTORNOT   int  iBytesPerLine, ts, r, MapMinY;
_EXTORNOT   float CameraW, CameraH;
_EXTORNOT	float Soft_Persp_K, stepdy, stepdd;
_EXTORNOT   CLIPPLANE ClipA, ClipB, ClipC, ClipD, ClipZ, ClipW;
_EXTORNOT   int u, vused;
/* Camera Cell X/Y*/
_EXTORNOT	int CCX, CCY;

_EXTORNOT   uint32_t Mask1, Mask2;


/* [GLOBAL] PreCache Vertex Map */
_EXTORNOT   EPoint VMap[128][128];
_EXTORNOT   EPoint VMap2[128][128];
_EXTORNOT   EPoint ev[3];

_EXTORNOT   ClipPoint cp[16];
_EXTORNOT   ClipPoint hleft, hright;

_EXTORNOT   void *HLineT;
_EXTORNOT   int   rTColor;
_EXTORNOT   int	SKYMin, SKYDTime, FadeL, GlassL;

/* [GLOBAL] View Range */
_EXTORNOT	int ctViewR;
_EXTORNOT	int dFacesCount,
				ReverseOn,
				TDirection;
_EXTORNOT   uint16_t	FadeTab[65][0x8000];

_EXTORNOT   uint8_t		MenuMap[300][400];


_EXTORNOT   int     PrevTime, TimeDt, T, Takt, RealTime, StepTime, MyHealth, ExitTime,
CallLockTime, NextCall;
_EXTORNOT   float   DeltaT;
_EXTORNOT   float   CameraX, CameraY, CameraZ, CameraAlpha, CameraBeta;
_EXTORNOT   float   PlayerX, PlayerY, PlayerZ, PlayerAlpha, PlayerBeta,
HeadY, HeadBackR, HeadBSpeed, HeadAlpha, HeadBeta,
SSpeed, VSpeed, RSpeed, YSpeed;
_EXTORNOT   Vector3d PlayerNv;

_EXTORNOT   float   ca, sa, cb, sb, wpnDAlpha, wpnDBeta;
_EXTORNOT   void *lpVideoBuf, *lpMenuBuf, *lpMenuBuf2, *lpTextureAddr;
_EXTORNOT   HBITMAP hbmpVideoBuf, hbmpMenuBuf, hbmpMenuBuf2;
_EXTORNOT   HCURSOR hcArrow;
_EXTORNOT   int     DivTbl[10240];

_EXTORNOT   Vector3d  v[3];
_EXTORNOT   ScrPoint  scrp[3];
_EXTORNOT   MScrPoint mscrp[3];
_EXTORNOT   Vector3d  nv, waterclipbase;


struct structKeyMap {
	int	fkForward,
		fkBackward,
		fkUp,
		fkDown,
		fkLeft,
		fkRight,
		fkFire,
		fkShow,
		fkSLeft,
		fkSRight,
		fkStrafe,
		fkJump,
		fkRun,
		fkCrouch,
		fkCall,
		fkBinoc;
};

_EXTORNOT structKeyMap KeyMap;

_EXTORNOT bool
	WATERANI,
	Clouds,
	SKY,
	GOURAUD,
	MODELS,
	TIMER,
	BITMAPP,
	MIPMAP,
	NOCLIP,
	CLIP3D,
	NODARKBACK,
	CORRECTION,
	LOWRESTX,
	FOGENABLE,
	FOGON,
	CAMERAINFOG,
	WATERREVERSE,
	waterclip,
	UNDERWATER,
	NeedWater,
	SWIM,
	FLY,
	PAUSE,
	OPTICMODE,
	BINMODE,
	EXITMODE,
	MapMode,
	RunMode,
	SHADOWS3D,
	REVERSEMS,
	SLOW,
	DEBUG,
	MORPHP,
	MORPHA;

_EXTORNOT int  CameraFogI;
_EXTORNOT int OptAgres, OptDens, OptSens, OptRes, OptText, OptSys, WaitKey, OPT_ALPHA_COLORKEY;

extern std::chrono::steady_clock::duration g_time_start;






//========== for audio ==============//
int  AddVoice(int, short int*);
int  AddVoicev(int, short int*, int);
int  AddVoice3dv(int, short int*, float, float, float, int);
int  AddVoice3d(int, short int*, float, float, float);

void SetAmbient3d(int, short int*, float, float, float);
void SetAmbient(int, short int*, int);
void AudioSetCameraPos(float, float, float, float);
void InitAudioSystem(HWND);
void ShutdownAudioSystem();
void Audio_Restore();
void AudioStop();

//========== for 3d hardware =============//
_EXTORNOT bool HARD3D;
void ShowVideo();
void Init3DHardware();
bool Activate3DHardware();
void ShutDown3DHardware();
void Render3DHardwarePosts();
void CopyHARDToDIB();
void Hardware_ZBuffer(bool zb);

#ifdef _MAIN_
_EXTORNOT char KeysName[256][24] = {
"...",
"Esc",
"1",
"2",
"3",
"4",
"5",
"6",
"7",
"8",
"9",
"0",
"-",
"=",
"BSpace",
"Tab",
"Q",
"W",
"E",
"R",
"T",
"Y",
"U",
"I",
"O",
"P",
"[",
"]",
"Enter",
"Ctrl",
"A",
"S",
"D",
"F",
"G",
"H",
"J",
"K",
"L",
";",
"'",
"~",
"Shift",
"\\",
"Z",
"X",
"C",
"V",
"B",
"N",
"M",
",",
".",
"/",
"Shift",
"*",
"Alt",
"Space",
"CLock",
"F1",
"F2",
"F3",
"F4",
"F5",
"F6",
"F7",
"F8",
"F9",
"F10",
"NLock",
"SLock",
"Home",
"Up",
"PgUp",
"-",
"Left",
"Midle",
"Right",
"+",
"End",
"Down",
"PgDn",
"Ins",
"Del",
"",
"",
"",
"F11",
"F12",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"Mouse1",
"Mouse2",
"Mouse3",
"<?>",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};
#else
_EXTORNOT char KeysName[128][24];
#endif