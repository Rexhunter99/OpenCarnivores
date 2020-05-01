#include <Windows.h>
#include <cmath>
#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#define _AUDIO_
#include "audio.h"


std::thread audio_thread;
WAVEFORMATEX wf;
HWND hwndApp;
bool AudioNeedRestore = false;
bool audio_shutdown = false;


std::string dsoundGetErrorString(HRESULT hres);
void dsoundPrintBufferCaps(DSBCAPS* caps);


/* TODO:
Update to utilise dynamic libraries like Carnivores 2/IA does, possibly even able to use Carnivores 2's libraries 1:1
*/


void ProcessAudioThread()
{
	for (audio_shutdown = false; !audio_shutdown;)
	{
		if (iSoundActive)
		{
			ProcessAudio();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(70));
	}
}



void Init_SetCooperative()
{

}


int InitDirectSound(HWND hwnd)
{
	std::cout << std::endl;
	std::cout << "==Init Direct Sound==" << std::endl;

	HRESULT hres = DS_OK;
	iTotalSoundDevices = 0;
	hSoundHeap = HeapCreate(0, 1024000, 4000000);
	if (!hSoundHeap)
		return 0;

	std::cout << "SFX heap created" << std::endl;

	sdd = (SOUNDDEVICEDESC*)HeapAlloc(hSoundHeap, HEAP_ZERO_MEMORY, sizeof(SOUNDDEVICEDESC) * MAX_SOUND_DEVICE);
	if (!sdd)
		return 0;

	lpSoundBuffer = (char*)HeapAlloc(hSoundHeap, HEAP_ZERO_MEMORY, 4096 * 2);
	if (!lpSoundBuffer)
		return 0;

	std::cout << "Back Sound Buffer created." << std::endl;

	for (int i = 0; i < MAX_SOUND_DEVICE; i++) {
		sdd[i].lpDSC = (LPDSCAPS)HeapAlloc(hSoundHeap, HEAP_ZERO_MEMORY, sizeof(DSCAPS));
		if (!sdd[i].lpDSC)
			return 0;
		sdd[i].lpDSC->dwSize = sizeof(DSCAPS);
	}

	hres = DirectSoundEnumerate((LPDSENUMCALLBACK)EnumerateSoundDevice, NULL);
	if (hres != DS_OK) {
		std::cout << "DirectSoundEnumerate Error: " << std::hex << hres << "\n";
		std::cout << dsoundGetErrorString(hres) << std::endl;
		return 0;
	}
	std::cout << "DirectSoundEnumerate: Ok" << std::endl;

	iTotal16SD = 0;
	for (int i = 0; i < iTotalSoundDevices; i++)
	{
		LPDIRECTSOUND lpds;
		if (DirectSoundCreate(sdd[i].lpGuid, &lpds, NULL) != DS_OK)
			continue;

		if (lpds->GetCaps(sdd[i].lpDSC) != DS_OK)
			continue;

		if (sdd[i].lpDSC->dwFlags & (DSCAPS_PRIMARY16BIT | DSCAPS_PRIMARYSTEREO | DSCAPS_SECONDARY16BIT | DSCAPS_SECONDARYSTEREO))
		{
			sdd[i].status = 1;
			iTotal16SD++;
			std::cout << "Acceptable device: " << i << std::endl;
		}
	}

	if (!iTotal16SD)
		return 0;

	iCurrentDriver = 0;

	while (!sdd[iCurrentDriver].status)
		iCurrentDriver++;

	std::cout << "Device selected: " << iCurrentDriver << std::endl;

	hres = DirectSoundCreate(sdd[iCurrentDriver].lpGuid, &lpDS, NULL);
	if ((hres != DS_OK) || (!lpDS)) {
		std::cout << "DirectSoundCreate Error: " << std::hex << hres << "\n";
		std::cout << dsoundGetErrorString(hres) << std::endl;
		return 0;
	}
	std::cout << "DirectSoundCreate: Ok" << std::endl;

	std::cout << "Attempting to set DSSCL_WRITEPRIMARY CooperativeLevel:\n";
	hres = lpDS->SetCooperativeLevel(hwnd, DSSCL_WRITEPRIMARY);
	if (hres != DS_OK)
	{
		std::cout << "\tSetCooperativeLevel Error: " << std::hex << hres << "\n";
		std::cout << dsoundGetErrorString(hres) << std::endl;
		PrimaryMode = false;
	}
	else
	{
		std::cout << "\tSet CooperativeLevel: Ok" << std::endl;
		PrimaryMode = true;
	}

	if (!PrimaryMode)
	{
		std::cout << "Attempting to set DSSCL_EXCLUSIVE CooperativeLevel:\n";
		hres = lpDS->SetCooperativeLevel(hwnd, DSSCL_EXCLUSIVE);
		if (hres != DS_OK)
		{
			std::cout << "\tSetCooperativeLevel Error: " << std::hex << hres << "\n";
			std::cout << dsoundGetErrorString(hres) << std::endl;
			return 0;
		}
		std::cout << "Set Cooperative  : Ok" << std::endl;
	}


	// -- Creating primary buffer
	std::memcpy(&WaveFormat, &wf, sizeof(WAVEFORMATEX));

	DSBUFFERDESC dsbd;
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;
	dsbd.dwReserved = 0;

	std::cout << "Attempting to create primary sound buffer:\n";
	hres = lpDS->CreateSoundBuffer(&dsbd, &lpdsPrimary, NULL);
	if (hres != DS_OK) {
		std::cout << "\tCreatePrimarySoundBuffer Error: " << std::hex << hres << "\n";
		std::cout << dsoundGetErrorString(hres) << std::endl;
		return 0;
	}

	std::cout << "\tCreatePrimarySoundBuffer: Ok" << std::endl;
	lpdsWork = lpdsPrimary;

	hres = lpdsPrimary->SetFormat(&wf);
	if (hres != DS_OK) {
		std::cout << "SetFormat Error: " << std::hex << hres << "\n";
		std::cout << dsoundGetErrorString(hres) << std::endl;
		return 0;
	}
	std::cout << "SetFormat: Ok" << std::endl;

	if (!PrimaryMode) {
		// -- Creating Secondary
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwFlags = 0;
		dsbd.dwBufferBytes = 2 * 8192;
		dsbd.lpwfxFormat = &wf;
		dsbd.dwReserved = 0;

		std::cout << "Creating secondary sound buffer:\n";
		hres = lpDS->CreateSoundBuffer(&dsbd, &lpdsSecondary, NULL);
		if (hres != DS_OK) {
			std::cout << "\tCreateSecondarySoundBuffer Error: " << std::hex << hres << "\n";
			std::cout << dsoundGetErrorString(hres) << std::endl;
			return 0;
		}
		std::cout << "\tCreateSecondarySoundBuffer: Ok" << std::endl;
		lpdsWork = lpdsSecondary;
	}

	DSBCAPS dsbc;
	dsbc.dwSize = sizeof(DSBCAPS);
	lpdsWork->GetCaps(&dsbc);
	iBufferLength = dsbc.dwBufferBytes;
	iBufferLength /= 8192;

	// -- Share buffer capabilities
	dsoundPrintBufferCaps(&dsbc);

	std::cout << "Testing audio playback:\n";
	hres = lpdsWork->Play(0, 0, DSBPLAY_LOOPING);
	if (hres != DS_OK)
	{
		std::cout << "\tPlayback Error: " << std::hex << hres << "\n";
		std::cout << dsoundGetErrorString(hres) << std::endl;
		return 0;
	}
	std::cout << "\tPlayback: Ok" << std::endl;

	std::memset(channel, 0, sizeof(CHANNEL) * MAX_CHANNEL);
	ambient.iLength = 0;

	iSoundActive = true;

	audio_thread = std::thread(ProcessAudioThread);

	std::cout << "DirectSound activated." << std::endl;
	return 1;
}


void InitAudioSystem(HWND hw)
{
	hwndApp = hw;
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 2;
	wf.nSamplesPerSec = 11025 * 2;
	wf.nAvgBytesPerSec = 44100 * 2;
	wf.nBlockAlign = 4;
	wf.wBitsPerSample = 16;
	wf.cbSize = 0;

	if (!InitDirectSound(hwndApp))
	{
		iSoundActive = false;
		std::cout << "Audio System failed" << std::endl;
	}
}


void ShutdownAudioSystem()
{
	iSoundActive = false;
	if (!audio_shutdown && audio_thread.joinable()) {
		audio_shutdown = true;
		audio_thread.join();
	}
}


void AudioStop()
{
	std::memset(&ambient, 0, sizeof(ambient));
	std::memset(&ambient2, 0, sizeof(ambient2));
	std::memset(channel, 0, sizeof(CHANNEL) * MAX_CHANNEL);
	AudioNeedRestore = false;
}


void AudioSetCameraPos(float cx, float cy, float cz, float ca)
{
	xCamera = (int)cx;
	yCamera = (int)cy;
	zCamera = (int)cz;
	alphaCamera = ca;
	cosa = std::cosf(ca);
	sina = std::sinf(ca);
}


void SetAmbient(int length, short int* lpdata, int av)
{
	if (!iSoundActive)
		return;
	if (ambient.lpData == lpdata)
		return;

	ambient2 = ambient;

	ambient.iLength = length;
	ambient.lpData = lpdata;
	ambient.iPosition = 0;
	ambient.volume = 0;
	ambient.avolume = av;
}


void SetAmbient3d(int length, short int* lpdata, float cx, float cy, float cz)
{
	if (!iSoundActive) return;

	if (mambient.iPosition >= length)
		mambient.iPosition = 0;

	mambient.iLength = length;
	mambient.lpData = lpdata;
	mambient.x = cx;
	mambient.y = cy;
	mambient.z = cz;
}



int AddVoice3dv(int length, short int* lpdata, float cx, float cy, float cz, int vol)
{
	if (!iSoundActive) return 0;
	if (lpdata == 0) return 0;

	for (int i = 0; i < MAX_CHANNEL; i++)
		if (!channel[i].status) {
			channel[i].iLength = length;
			channel[i].lpData = lpdata;
			channel[i].iPosition = 0;

			channel[i].x = (int)cx;
			channel[i].y = (int)cy;
			channel[i].z = (int)cz;

			channel[i].status = 1;
			channel[i].volume = vol;

			return 1;
		}
	return 0;
}


int AddVoice3d(int length, short int* lpdata, float cx, float cy, float cz)
{
	return AddVoice3dv(length, lpdata, cx, cy, cz, 256);
}


int AddVoicev(int length, short int* lpdata, int v)
{
	return AddVoice3dv(length, lpdata, 0, 0, 0, v);
}



int AddVoice(int length, short int* lpdata)
{
	return AddVoice3dv(length, lpdata, 0, 0, 0, 256);
}




BOOL CALLBACK EnumerateSoundDevice(GUID* lpGuid, LPSTR lpstrDescription, LPSTR lpstrModule, LPVOID lpContext)
{
	if (lpGuid == NULL)
	{
		if (!iTotalSoundDevices)
			return true;
		else
			return false;
	}

	std::cout << "Device" << iTotalSoundDevices << ": " << lpstrDescription << "/" << lpstrModule << std::endl;
	sdd[iTotalSoundDevices].lpGuid = (LPGUID)HeapAlloc(hSoundHeap, HEAP_ZERO_MEMORY, sizeof(GUID));

	if (!sdd[iTotalSoundDevices].lpGuid)
		return false;

	std::memcpy(sdd[iTotalSoundDevices].lpGuid, lpGuid, sizeof(GUID));
	/*
	   int len = lstrlen( lpstrDescription );
	   sdd[iTotalSoundDevices].lpstrDescription = (char*)HeapAlloc( hSoundHeap, 0, len );
	   if( sdd[iTotalSoundDevices].lpstrDescription == NULL )
		  return FALSE;
	   lstrcpy( sdd[iTotalSoundDevices].lpstrDescription, lpstrDescription );

	   len = lstrlen( lpstrModule );
	   sdd[iTotalSoundDevices].lpstrModule = (char*)HeapAlloc( hSoundHeap, 0, len );
	   if( sdd[iTotalSoundDevices].lpstrModule == NULL )
		  return FALSE;
	   lstrcpy( sdd[iTotalSoundDevices].lpstrModule, lpstrModule ); */

	iTotalSoundDevices++;

	return true;
}


void Audio_Restore()
{
	if (iSoundActive)
	{
		lpdsWork->Stop();
		lpdsWork->Restore();

		HRESULT hres = lpdsWork->Play(0, 0, DSBPLAY_LOOPING);

		if (hres != DS_OK)
			AudioNeedRestore = true;
		else
			AudioNeedRestore = false;

		if (!AudioNeedRestore)
			std::cout << "Audio restored." << std::endl;
	}
}


int ProcessAudio()
{
	LPVOID lpStart1;
	LPVOID lpStart2;
	DWORD  len1, len2;
	HRESULT hres;
	static int PrevBlock = 1;

	if (AudioNeedRestore)
		Audio_Restore();
	if (AudioNeedRestore)
		return 1;

	hres = lpdsWork->GetCurrentPosition(&len1, &dwWritePos);
	if (hres != DS_OK)
	{
		AudioNeedRestore = true;
		return 0;
	}

	hres = lpdsWork->GetCurrentPosition(&len2, &dwWritePos);
	if (hres != DS_OK)
	{
		AudioNeedRestore = true;
		return 0;
	}

	if ((len1 > len2) || (len1 < len2 + 16))
	{
		hres = lpdsWork->GetCurrentPosition(&len2, &dwWritePos);
	}

	if (len1 + len2 == 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		lpdsWork->GetCurrentPosition(&len2, &dwWritePos);
		if (!len2)
		{
			AudioNeedRestore = true;
			return 0;
		}
	}

	dwPlayPos = len2;

	int CurBlock = dwPlayPos / (4096 * 2);
	if (CurBlock == PrevBlock)
		return 1;

	if ((int)dwPlayPos < CurBlock * 4096 * 2 + 2)
		return 1; // it's no time to put info

	std::memset(lpSoundBuffer, 0, MAX_BUFFER_LENGTH * 2);
	Audio_MixChannels();
	Audio_MixAmbient();
	Audio_MixAmbient3d();

	PrevBlock = CurBlock;
	int NextBlock = (CurBlock + 1) % iBufferLength;
	hres = lpdsWork->Lock(NextBlock * 4096 * 2, 4096 * 2, &lpStart1, &len1, &lpStart2, &len2, 0);
	if (hres != DS_OK)
	{
		return 0;
	}

	CopyMemory(lpStart1, lpSoundBuffer, 4096 * 2);

	hres = lpdsWork->Unlock(lpStart1, len1, lpStart2, len2);
	if (hres != DS_OK)
		return 0;

	return 1;
}


void CalcLRVolumes(int v0, int x, int y, int z, int& lv, int& rv)
{
	if (x == 0)
	{
		lv = v0 * 180;
		rv = v0 * 180;
		return;
	}

	v0 *= 200;
	x -= xCamera;
	y -= yCamera;
	z -= zCamera;
	float xx = (float)x * cosa + (float)z * sina;
	float yy = (float)y;
	float zz = (float)fabs((float)z * cosa - (float)x * sina);

	float xa = (float)fabs(xx);
	float l = 0.8f;
	float r = 0.8f;
	float d = (float)sqrt(xx * xx + yy * yy + zz * zz) - MIN_RADIUS;
	float k;
	if (d <= 0)
		k = 1.f;
	//else k = (MAX_RADIUS - d) / MAX_RADIUS;
	else
		k = 1224.f / (1224 + d);

	if (d > 6000)
	{
		d -= 6000;
		k = k * (4000 - d) / (4000);
	}
	if (k < 0)
		k = 0.f;

	float fi = (float)atan2(xa, zz);
	r = 0.7f + 0.3f * fi / (3.141593f / 2.f);
	l = 0.7f - 0.6f * fi / (3.141593f / 2.f);

	if (xx > 0)
	{
		lv = (int)(v0 * l * k); rv = (int)(v0 * r * k);
	}
	else
	{
		lv = (int)(v0 * r * k);
		rv = (int)(v0 * l * k);
	}
}


void Audio_MixChannels()
{
	int iMixLen;
	int srcofs;
	int LV, RV;

	for (int i = 0; i < MAX_CHANNEL; i++)
	{
		if (channel[i].status)
		{
			if (channel[i].iPosition + 2048 * 2 >= channel[i].iLength)
				iMixLen = (channel[i].iLength - channel[i].iPosition) >> 1;
			else
				iMixLen = 1024 * 2;

			srcofs = (int)channel[i].lpData + channel[i].iPosition;
			CalcLRVolumes(channel[i].volume, channel[i].x, channel[i].y, channel[i].z, LV, RV);

			if (LV || RV)
				Audio_MixSound((int)lpSoundBuffer, srcofs, iMixLen, LV, RV);

			if (channel[i].iPosition + 2048 * 2 >= channel[i].iLength)
				channel[i].status = 0;
			else
				channel[i].iPosition += 2048 * 2;
		}
	}
}


void Audio_DoMixAmbient(AMBIENT& ambient)
{
	if (ambient.lpData == 0)
		return;
	int iMixLen, srcofs;
	int v = (32000 * ambient.volume * ambient.avolume) / 256 / 256;

	if (ambient.iPosition + 2048 * 2 >= ambient.iLength)
		iMixLen = (ambient.iLength - ambient.iPosition) >> 1;
	else
		iMixLen = 1024 * 2;

	srcofs = (int)ambient.lpData + ambient.iPosition;
	Audio_MixSound((int)lpSoundBuffer, srcofs, iMixLen, v, v);

	if (ambient.iPosition + 2048 * 2 >= ambient.iLength)
		ambient.iPosition = 0;
	else
		ambient.iPosition += 2048 * 2;

	if (iMixLen < 1024 * 2)
	{
		Audio_MixSound((int)lpSoundBuffer + iMixLen * 4, (int)ambient.lpData, 1024 * 2 - iMixLen, v, v);
		ambient.iPosition += (1024 * 2 - iMixLen) * 2;
	}
}


void Audio_MixAmbient()
{
	Audio_DoMixAmbient(ambient);
	if (ambient.volume < 256)
		ambient.volume = min(ambient.volume + 16, 256);

	if (ambient2.volume)
		Audio_DoMixAmbient(ambient2);
	if (ambient2.volume > 0)
		ambient2.volume = max(ambient2.volume - 16, 0);
}


void Audio_MixAmbient3d()
{
	if (mambient.lpData == 0)
		return;

	int iMixLen, srcofs;
	int LV, RV;

	CalcLRVolumes(256, (int)mambient.x, (int)mambient.y, (int)mambient.z, LV, RV);
	if (!(LV || RV))
		return;


	if (mambient.iPosition + 2048 * 2 >= mambient.iLength)
		iMixLen = (mambient.iLength - mambient.iPosition) >> 1;
	else
		iMixLen = 1024 * 2;

	srcofs = (int)mambient.lpData + mambient.iPosition;
	Audio_MixSound((int)lpSoundBuffer, srcofs, iMixLen, LV, RV);

	if (mambient.iPosition + 2048 * 2 >= mambient.iLength)
		mambient.iPosition = 0;
	else
		mambient.iPosition += 2048 * 2;

	if (iMixLen < 1024 * 2)
	{
		Audio_MixSound((int)lpSoundBuffer + iMixLen * 4, (int)mambient.lpData, 1024 * 2 - iMixLen, LV, RV);
		mambient.iPosition += (1024 * 2 - iMixLen) * 2;
	}
}


void Audio_MixSound(int DestAddr, int SrcAddr, int MixLen, int LVolume, int RVolume)
{
	_asm {
		mov      edi, DestAddr
		mov      ecx, MixLen
		mov      esi, SrcAddr
	}

SOUND_CYCLE:

	_asm {
		movsx    eax, word ptr[esi]
		imul     LVolume
		sar      eax, 16
		mov      bx, word ptr[edi]

		add      ax, bx
		jo       LEFT_CHECK_OVERFLOW
		mov      word ptr[edi], ax
		jmp      CYCLE_RIGHT
	}
LEFT_CHECK_OVERFLOW:
	__asm {
		cmp      bx, 0
		js       LEFT_MAX_NEGATIVE
		mov      ax, 32767
		mov      word ptr[edi], ax
		jmp      CYCLE_RIGHT
	}
LEFT_MAX_NEGATIVE:
	__asm  mov      word ptr[edi], -32767




CYCLE_RIGHT:
	__asm {
		movsx    eax, word ptr[esi]
		imul     dword ptr RVolume
		sar      eax, 16
		mov      bx, word ptr[edi + 2]

		add      ax, bx
		jo       RIGHT_CHECK_OVERFLOW
		mov      word ptr[edi + 2], ax
		jmp      CYCLE_CONTINUE
	}
RIGHT_CHECK_OVERFLOW:
	__asm {
		cmp      bx, 0
		js       RIGHT_MAX_NEGATIVE
		mov      word ptr[edi + 2], 32767
		jmp      CYCLE_CONTINUE
	}
RIGHT_MAX_NEGATIVE:
	__asm  mov      word ptr[edi + 2], -32767

	CYCLE_CONTINUE :
		__asm {
		add      edi, 4
		add      esi, 2
		dec      ecx
		jnz      SOUND_CYCLE
	}
}

std::string dsoundGetErrorString(HRESULT hres)
{
	switch (hres) {
	case DS_NO_VIRTUALIZATION:
		return "The buffer was created, but another 3D algorithm was substituted.";
		//case DS_INCOMPLETE:
		//	return "The method succeeded, but not all the optional effects were obtained.";
	case DSERR_ACCESSDENIED:
		return "The request failed because access was denied.";
	case DSERR_ALLOCATED:
		return "The request failed because resources, such as a priority level, were already in use by another caller.";
	case DSERR_ALREADYINITIALIZED:
		return "The object is already initialized.";
	case DSERR_BADFORMAT:
		return "The specified wave format is not supported.";
	case DSERR_BADSENDBUFFERGUID:
		return "The GUID specified in an audiopath file does not match a valid mix - in buffer.";
	case DSERR_BUFFERLOST:
		return "The buffer memory has been lost and must be restored.";
	case DSERR_BUFFERTOOSMALL:
		return "The buffer size is not great enough to enable effects processing.";
	case DSERR_CONTROLUNAVAIL:
		return "The buffer control(volume, pan, and so on) requested by the caller is not available.Controls must be specified when the buffer is created, using the dwFlags member of DSBUFFERDESC.";
	case DSERR_DS8_REQUIRED:
		return "A DirectSound object of class CLSID_DirectSound8 or later is required for the requested functionality.For more information, see IDirectSound8 Interface.";
	case DSERR_FXUNAVAILABLE:
		return "The effects requested could not be found on the system, or they are in the wrong order or in the wrong location; for example, an effect expected in hardware was found in software.";
	case DSERR_GENERIC:
		return "An undetermined error occurred inside the DirectSound subsystem.";
	case DSERR_INVALIDCALL:
		return "This function is not valid for the current state of this object.";
	case DSERR_INVALIDPARAM:
		return "An invalid parameter was passed to the returning function.";
	case DSERR_NOAGGREGATION:
		return "The object does not support aggregation.";
	case DSERR_NODRIVER:
		return "No sound driver is available for use, or the given GUID is not a valid DirectSound device ID.";
	case DSERR_NOINTERFACE:
		return "The requested COM interface is not available.";
	case DSERR_OBJECTNOTFOUND:
		return "The requested object was not found.";
	case DSERR_OTHERAPPHASPRIO:
		return "Another application has a higher priority level, preventing this call from succeeding.";
	case DSERR_OUTOFMEMORY:
		return "The DirectSound subsystem could not allocate sufficient memory to complete the caller's request.";
	case DSERR_PRIOLEVELNEEDED:
		return "A cooperative level of DSSCL_PRIORITY or higher is required.";
	case DSERR_SENDLOOP:
		return "A circular loop of send effects was detected.";
	case DSERR_UNINITIALIZED:
		return "The IDirectSound8::Initialize method has not been called or has not been called successfully before other methods were called.";
	case DSERR_UNSUPPORTED:
		return "The function called is not supported at this time.";
	default: return "The method succeeded."; // DS_OK
	}
}

void dsoundPrintBufferCaps(DSBCAPS* caps)
{
	if (!caps)
		return;
	if (caps->dwSize != sizeof(DSBCAPS))
		return;

	/* MSDN Article: https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee416818(v%3Dvs.85)
	*/
	std::cout << "Buffer capabilities:\n";
	if (caps->dwFlags & DSBCAPS_CTRL3D) std::cout << "\tThe buffer has 3D control capability.\n";
	if (caps->dwFlags & DSBCAPS_CTRLFREQUENCY) std::cout << "\tThe buffer has frequency control capability.\n";
	if (caps->dwFlags & DSBCAPS_CTRLFX) std::cout << "\tThe buffer supports effects processing.\n";
	if (caps->dwFlags & DSBCAPS_CTRLPAN) std::cout << "\tThe buffer has pan control capability.\n";
	if (caps->dwFlags & DSBCAPS_CTRLVOLUME) std::cout << "\tThe buffer has volume control capability.\n";
	if (caps->dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY) std::cout << "\tThe buffer has position notification capability.\n";
	if (caps->dwFlags & DSBCAPS_GETCURRENTPOSITION2) std::cout << "\tThe buffer uses the new behavior of the play cursor when IDirectSoundBuffer8::GetCurrentPosition is called.\n";
	if (caps->dwFlags & DSBCAPS_GLOBALFOCUS) std::cout << "\tThe buffer is a global sound buffer.\n";
	if (caps->dwFlags & DSBCAPS_LOCDEFER) std::cout << "\tThe buffer can be assigned to a hardware or software resource at play time, or when IDirectSoundBuffer8::AcquireResources is called.\n";
	if (caps->dwFlags & DSBCAPS_LOCHARDWARE) std::cout << "\tThe buffer uses hardware mixing.\n";
	if (caps->dwFlags & DSBCAPS_LOCSOFTWARE) std::cout << "\tThe buffer is in software memory and uses software mixing.\n";
	if (caps->dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE) std::cout << "\tThe sound is reduced to silence at the maximum distance.\n";
	if (caps->dwFlags & DSBCAPS_PRIMARYBUFFER) std::cout << "\t	The buffer is a primary buffer.\n";
	if (caps->dwFlags & DSBCAPS_STATIC) std::cout << "\tThe buffer is in on-board hardware memory.\n";
	if (caps->dwFlags & DSBCAPS_STICKYFOCUS) std::cout << "\tThe buffer has sticky focus.\n";
	if (caps->dwFlags & DSBCAPS_TRUEPLAYPOSITION) std::cout << "\tForce IDirectSoundBuffer8::GetCurrentPosition to return the buffer's true play position.\n";
}