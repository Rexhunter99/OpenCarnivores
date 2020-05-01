#if defined(_opengl3)

#include "Hunt.h"

#include <gl.h>
#include <glext.h>
#include <wglext.h>


void Init3DHardware() {
	HARD3D = true;
}

void Activate3DHardware() {

}

void ShutDown3DHardware() {

}

void ClearVideoBuf() {
	// Only used by RenderSoft
}

void ClipVector(CLIPPLANE& C, int vn) {

}

void CopyHARDToDIB() {

}

void DrawHMap() {

}

void DrawPicture(int x, int y, TPicture& pic) {

}

void DrawTPlane() {

}

void DrawTPlaneClip() {

}

void DrawTrophyText() {

}

void Hardware_ZBuffer(bool bl) {

}

void ProcessMap(...) {
}

void Render3DHardwarePosts() {
}

void RenderCharacter() {
	// Only RenderSoft
}

void RenderExplosion() {
	// Only RenderSoft
}

Render_LifeInfo()

void RenderModel(...) {

}

void RenderModelClip(...) {

}

void RenderModelClipWater(...) {
	// Only RenderSoft
}

void RenderNearModel(...) {

}

void RenderShip() {
	// Only RenderSoft
}

void RenderSkyPlane() {

}

void ShowControlElements() {

}

void ShowVideo() {

}

#endif