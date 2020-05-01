#pragma once

#include <ddraw.h>
#include <string>

std::string ddrawGetErrorString(HRESULT hErr)
{
	std::string out = "";

	switch (hErr)
	{
	case DDERR_DDSCAPSCOMPLEXREQUIRED: out = "DDERR_DDSCAPSCOMPLEXREQUIRED: New for DirectX 7.0. The surface requires the DDSCAPS_COMPLEX flag."; break;
	case DDERR_DEVICEDOESNTOWNSURFACE: out = "DDERR_DEVICEDOESNTOWNSURFACE: Surfaces created by one DirectDraw device cannot be used directly by another DirectDraw device."; break;
	case DDERR_EXPIRED: out = "DDERR_EXPIRED: The data has expired and is therefore no longer valid."; break;
	case DDERR_INVALIDSTREAM: out = "DDERR_INVALIDSTREAM: The specified stream contains invalid data."; break;
	case DDERR_MOREDATA: out = "DDERR_MOREDATA: There is more data available than the specified buffer size can hold."; break;
	case DDERR_NEWMODE: out = "DDERR_NEWMODE: New for DirectX 7.0. When IDirectDraw7::StartModeTest is called with the DDSMT_ISTESTREQUIRED flag, it may return this value to denote that some or all of the resolutions can and should be tested. IDirectDraw7::EvaluateMode returns this value to indicate that the test has switched to a new display mode."; break;
	case DDERR_NODRIVERSUPPORT: out = "DDERR_NODRIVERSUPPORT: New for DirectX 7.0. Testing cannot proceed because the display adapter driver does not enumerate refresh rates."; break;
	case DDERR_NOFOCUSWINDOW: out = "DDERR_NOFOCUSWINDOW: An attempt was made to create or set a device window without first setting the focus window."; break;
	case DDERR_NOMONITORINFORMATION: out = "DDERR_NOMONITORINFORMATION: New for DirectX 7.0. Testing cannot proceed because the monitor has no associated EDID data."; break;
	case DDERR_NONONLOCALVIDMEM: out = "DDERR_NONONLOCALVIDMEM: An attempt was made to allocate nonlocal video memory from a device that does not support nonlocal video memory."; break;
	case DDERR_NOOPTIMIZEHW: out = "DDERR_NOOPTIMIZEHW: The device does not support optimized surfaces."; break;
	case DDERR_NOSTEREOHARDWARE: out = "DDERR_NOSTEREOHARDWARE: There is no stereo hardware present or available."; break;
	case DDERR_NOSURFACELEFT: out = "DDERR_NOSURFACELEFT: There is no hardware present that supports stereo surfaces."; break;
	case DDERR_NOTLOADED: out = "DDERR_NOTLOADED: The surface is an optimized surface, but it has not yet been allocated any memory."; break;
	case DDERR_OVERLAPPINGRECTS: out = "DDERR_OVERLAPPINGRECTS: The source and destination rectangles are on the same surface and overlap each other."; break;
	case DDERR_TESTFINISHED: out = "DDERR_TESTFINISHED: New for DirectX 7.0. When returned by the IDirectDraw7::StartModeTest method, this value means that no test could be initiated because all the resolutions chosen for testing already have refresh rate information in the registry. When returned by IDirectDraw7::EvaluateMode, the value means that DirectDraw has completed a refresh rate test."; break;
	case DDERR_VIDEONOTACTIVE: out = "DDERR_VIDEONOTACTIVE: The video port is not active."; break;
	case DDERR_ALREADYINITIALIZED: out = "DDERR_ALREADYINITIALIZED: The object has already been initialized."; break;
	case DDERR_CANNOTATTACHSURFACE: out = "DDERR_CANNOTATTACHSURFACE: A surface cannot be attached to another requested surface."; break;
	case DDERR_CANNOTDETACHSURFACE: out = "DDERR_CANNOTDETACHSURFACE: A surface cannot be detached from another requested surface."; break;
	case DDERR_CURRENTLYNOTAVAIL: out = "DDERR_CURRENTLYNOTAVAIL: No support is currently available"; break;
	case DDERR_EXCEPTION: out = "DDERR_EXCEPTION: An exception was encountered while performing the requested operation."; break;
	case DDERR_GENERIC: out = "DDERR_GENERIC: There is an undefined error condition."; break;
	case DDERR_HEIGHTALIGN: out = "DDERR_HEIGHTALIGN: The height of the provided rectangle is not a multiple of the required alignment."; break;
	case DDERR_INCOMPATIBLEPRIMARY: out = "DDERR_INCOMPATIBLEPRIMARY: The primary surface creation request does not match the existing primary surface."; break;
	case DDERR_INVALIDCAPS: out = "DDERR_INVALIDCAPS: One or more of the capability bits passed to the callback function are incorrect."; break;
	case DDERR_INVALIDCLIPLIST: out = "DDERR_INVALIDCLIPLIST: DirectDraw does not support the provided clip list."; break;
	case DDERR_INVALIDMODE: out = "DDERR_INVALIDMODE: DirectDraw does not support the requested mode."; break;
	case DDERR_INVALIDOBJECT: out = "DDERR_INVALIDOBJECT: DirectDraw received a pointer that was an invalid DirectDraw object."; break;
	case DDERR_INVALIDPARAMS: out = "DDERR_INVALIDPARAMS: One or more of the parameters passed to the method are incorrect."; break;
	case DDERR_INVALIDPIXELFORMAT: out = "DDERR_INVALIDPIXELFORMAT: The pixel format was invalid as specified."; break;
	case DDERR_INVALIDRECT: out = "DDERR_INVALIDRECT: The provided rectangle was invalid."; break;
	case DDERR_LOCKEDSURFACES: out = "DDERR_LOCKEDSURFACES: One or more surfaces are locked, causing the failure of the requested operation."; break;
	case DDERR_NO3D: out = "DDERR_NO3D: No 3-D hardware or emulation is present."; break;
	case DDERR_NOALPHAHW: out = "DDERR_NOALPHAHW: No alpha-acceleration hardware is present or available, causing the failure of the requested operation."; break;
	case DDERR_NOCLIPLIST: out = "DDERR_NOCLIPLIST: No clip list is available."; break;
	case DDERR_NOCOLORCONVHW: out = "DDERR_NOCOLORCONVHW: No color-conversion hardware is present or available."; break;
	case DDERR_NOCOOPERATIVELEVELSET: out = "DDERR_NOCOOPERATIVELEVELSET: A create function was called without the IDirectDraw7::SetCooperativeLevel method."; break;
	case DDERR_NOCOLORKEY: out = "DDERR_NOCOLORKEY: The surface does not currently have a color key."; break;
	case DDERR_NOCOLORKEYHW: out = "DDERR_NOCOLORKEYHW: There is no hardware support for the destination color key."; break;
	case DDERR_NODIRECTDRAWSUPPORT: out = "DDERR_NODIRECTDRAWSUPPORT: DirectDraw support is not possible with the current display driver."; break;
	case DDERR_NOEXCLUSIVEMODE: out = "DDERR_NOEXCLUSIVEMODE: The operation requires the application to have exclusive mode, but the application does not have exclusive mode."; break;
	case DDERR_NOFLIPHW: out = "DDERR_NOFLIPHW: Flipping visible surfaces is not supported."; break;
	case DDERR_NOGDI: out = "DDERR_NOGDI: No GDI is present."; break;
	case DDERR_NOMIRRORHW: out = "DDERR_NOMIRRORHW: No mirroring hardware is present or available."; break;
	case DDERR_NOTFOUND: out = "DDERR_NOTFOUND: The requested item was not found."; break;
	case DDERR_NOOVERLAYHW: out = "DDERR_NOOVERLAYHW: No overlay hardware is present or available."; break;
	case DDERR_NORASTEROPHW: out = "DDERR_NORASTEROPHW: No appropriate raster-operation hardware is present or available."; break;
	case DDERR_NOROTATIONHW: out = "DDERR_NOROTATIONHW: No rotation hardware is present or available."; break;
	case DDERR_NOSTRETCHHW: out = "DDERR_NOSTRETCHHW: There is no hardware support for stretching."; break;
	case DDERR_NOT4BITCOLOR: out = "DDERR_NOT4BITCOLOR: The DirectDrawSurface object is not using a 4-bit color palette, and the requested operation requires a 4-bit color palette."; break;
	case DDERR_NOT4BITCOLORINDEX: out = "DDERR_NOT4BITCOLORINDEX: The DirectDrawSurface object is not using a 4-bit color index palette, and the requested operation requires a 4-bit color index palette."; break;
	case DDERR_NOT8BITCOLOR: out = "DDERR_NOT8BITCOLOR: The DirectDrawSurface object is not using an 8-bit color palette, and the requested operation requires an 8-bit color palette."; break;
	case DDERR_NOTEXTUREHW: out = "DDERR_NOTEXTUREHW: The operation cannot be carried out because no texture-mapping hardware is present or available."; break;
	case DDERR_NOVSYNCHW: out = "DDERR_NOVSYNCHW: There is no hardware support for vertical blank synchronized operations."; break;
	case DDERR_NOZBUFFERHW: out = "DDERR_NOZBUFFERHW: The operation to create a z-buffer in display memory or to perform a blit, using a z-buffer cannot be carried out because there is no hardware support for z-buffers."; break;
	case DDERR_NOZOVERLAYHW: out = "DDERR_NOZOVERLAYHW: The overlay surfaces cannot be z-layered, based on the z-order because the hardware does not support z-ordering of overlays."; break;
	case DDERR_OUTOFCAPS: out = "DDERR_OUTOFCAPS: The hardware needed for the requested operation has already been allocated."; break;
	case DDERR_OUTOFMEMORY: out = "DDERR_OUTOFMEMORY: DirectDraw does not have enough memory to perform the operation."; break;
	case DDERR_OUTOFVIDEOMEMORY: out = "DDERR_OUTOFVIDEOMEMORY: DirectDraw does not have enough display memory to perform the operation."; break;
	case DDERR_OVERLAYCANTCLIP: out = "DDERR_OVERLAYCANTCLIP: The hardware does not support clipped overlays."; break;
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: out = "DDERR_OVERLAYCOLORKEYONLYONEACTIVE: An attempt was made to have more than one color key active on an overlay."; break;
	case DDERR_PALETTEBUSY: out = "DDERR_PALETTEBUSY: Access to this palette is refused because the palette is locked by another thread."; break;
	case DDERR_COLORKEYNOTSET: out = "DDERR_COLORKEYNOTSET: No source color key is specified for this operation."; break;
	case DDERR_SURFACEALREADYATTACHED: out = "DDERR_SURFACEALREADYATTACHED: An attempt was made to attach a surface to another surface to which it is already attached."; break;
	case DDERR_SURFACEALREADYDEPENDENT: out = "DDERR_SURFACEALREADYDEPENDENT: An attempt was made to make a surface a dependency of another surface on which it is already dependent."; break;
	case DDERR_SURFACEBUSY: out = "DDERR_SURFACEBUSY: Access to the surface is refused because the surface is locked by another thread."; break;
	case DDERR_CANTLOCKSURFACE: out = "DDERR_CANTLOCKSURFACE: Access to this surface is refused because an attempt was made to lock the primary surface without DCI support."; break;
	case DDERR_SURFACEISOBSCURED: out = "DDERR_SURFACEISOBSCURED: Access to the surface is refused because the surface is obscured."; break;
	case DDERR_SURFACELOST: out = "DDERR_SURFACELOST: Access to the surface is refused because the surface memory is gone. Call the IDirectDrawSurface7::Restore method on this surface to restore the memory associated with it."; break;
	case DDERR_SURFACENOTATTACHED: out = "DDERR_SURFACENOTATTACHED: The requested surface is not attached."; break;
	case DDERR_TOOBIGHEIGHT: out = "DDERR_TOOBIGHEIGHT: The height requested by DirectDraw is too large."; break;
	case DDERR_TOOBIGSIZE: out = "DDERR_TOOBIGSIZE: The size requested by DirectDraw is too large. However, the individual height and width are valid sizes."; break;
	case DDERR_TOOBIGWIDTH: out = "DDERR_TOOBIGWIDTH: The width requested by DirectDraw is too large."; break;
	case DDERR_UNSUPPORTED: out = "DDERR_UNSUPPORTED: The operation is not supported."; break;
	case DDERR_UNSUPPORTEDFORMAT: out = "DDERR_UNSUPPORTEDFORMAT: The pixel format requested is not supported by DirectDraw."; break;
	case DDERR_UNSUPPORTEDMASK: out = "DDERR_UNSUPPORTEDMASK: The bitmask in the pixel format requested is not supported by DirectDraw."; break;
	case DDERR_VERTICALBLANKINPROGRESS: out = "DDERR_VERTICALBLANKINPROGRESS: A vertical blank is in progress."; break;
	case DDERR_WASSTILLDRAWING: out = "DDERR_WASSTILLDRAWING: The previous blit operation that is transferring information to or from this surface is incomplete."; break;
	case DDERR_XALIGN: out = "DDERR_XALIGN: The provided rectangle was not horizontally aligned on a required boundary."; break;
	case DDERR_INVALIDDIRECTDRAWGUID: out = "DDERR_INVALIDDIRECTDRAWGUID: The globally unique identifier (GUID) passed to the DirectDrawCreate function is not a valid DirectDraw driver identifier."; break;
	case DDERR_DIRECTDRAWALREADYCREATED: out = "DDERR_DIRECTDRAWALREADYCREATED: A DirectDraw object representing this driver has already been created for this process."; break;
	case DDERR_NODIRECTDRAWHW: out = "DDERR_NODIRECTDRAWHW: Hardware-only DirectDraw object creation is not possible; the driver does not support any hardware."; break;
	case DDERR_PRIMARYSURFACEALREADYEXISTS: out = "DDERR_PRIMARYSURFACEALREADYEXISTS: This process has already created a primary surface."; break;
	case DDERR_NOEMULATION: out = "DDERR_NOEMULATION: Software emulation is not available."; break;
	case DDERR_REGIONTOOSMALL: out = "DDERR_REGIONTOOSMALL: The region passed to the IDirectDrawClipper::GetClipList method is too small."; break;
	case DDERR_CLIPPERISUSINGHWND: out = "DDERR_CLIPPERISUSINGHWND: An attempt was made to set a clip list for a DirectDrawClipper object that is already monitoring a window handle."; break;
	case DDERR_NOCLIPPERATTACHED: out = "DDERR_NOCLIPPERATTACHED: No DirectDrawClipper object is attached to the surface object."; break;
	case DDERR_NOHWND: out = "DDERR_NOHWND: Clipper notification requires a window handle, or no window handle has been previously set as the cooperative level window handle."; break;
	case DDERR_HWNDSUBCLASSED: out = "DDERR_HWNDSUBCLASSED: DirectDraw is prevented from restoring state because the DirectDraw cooperative-level window handle has been subclassed."; break;
	case DDERR_HWNDALREADYSET: out = "DDERR_HWNDALREADYSET: The DirectDraw cooperative-level window handle has already been set. It cannot be reset while the process has surfaces or palettes created."; break;
	case DDERR_NOPALETTEATTACHED: out = "DDERR_NOPALETTEATTACHED: No palette object is attached to this surface."; break;
	case DDERR_NOPALETTEHW: out = "DDERR_NOPALETTEHW: There is no hardware support for 16- or 256-color palettes."; break;
	case DDERR_BLTFASTCANTCLIP: out = "DDERR_BLTFASTCANTCLIP: A DirectDrawClipper object is attached to a source surface that has passed into a call to the IDirectDrawSurface7::BltFast method."; break;
	case DDERR_NOBLTHW: out = "DDERR_NOBLTHW: No blitter hardware is present."; break;
	case DDERR_NODDROPSHW: out = "DDERR_NODDROPSHW: No DirectDraw raster-operation (ROP) hardware is available."; break;
	case DDERR_OVERLAYNOTVISIBLE: out = "DDERR_OVERLAYNOTVISIBLE: The IDirectDrawSurface7::GetOverlayPosition method was called on a hidden overlay."; break;
	case DDERR_NOOVERLAYDEST: out = "DDERR_NOOVERLAYDEST: The IDirectDrawSurface7::GetOverlayPosition method is called on an overlay that the IDirectDrawSurface7::UpdateOverlay method has not been called on to establish as a destination."; break;
	case DDERR_INVALIDPOSITION: out = "DDERR_INVALIDPOSITION: The position of the overlay on the destination is no longer legal."; break;
	case DDERR_NOTAOVERLAYSURFACE: out = "DDERR_NOTAOVERLAYSURFACE: An overlay component is called for a nonoverlay surface."; break;
	case DDERR_EXCLUSIVEMODEALREADYSET: out = "DDERR_EXCLUSIVEMODEALREADYSET: An attempt was made to set the cooperative level when it was already set to exclusive."; break;
	case DDERR_NOTFLIPPABLE: out = "DDERR_NOTFLIPPABLE: An attempt was made to flip a surface that cannot be flipped."; break;
	case DDERR_CANTDUPLICATE: out = "DDERR_CANTDUPLICATE: Primary and 3-D surfaces, or surfaces that are implicitly created, cannot be duplicated."; break;
	case DDERR_NOTLOCKED: out = "DDERR_NOTLOCKED: An attempt was made to unlock a surface that was not locked."; break;
	case DDERR_CANTCREATEDC: out = "DDERR_CANTCREATEDC: Windows cannot create any more device contexts (DCs), or a DC has requested a palette-indexed surface when the surface had no palette and the display mode was not palette-indexed (in this case DirectDraw cannot select a proper palette into the DC)."; break;
	case DDERR_NODC: out = "DDERR_NODC: No device context (DC) has ever been created for this surface."; break;
	case DDERR_WRONGMODE: out = "DDERR_WRONGMODE: This surface cannot be restored because it was created in a different mode."; break;
	case DDERR_IMPLICITLYCREATED: out = "DDERR_IMPLICITLYCREATED: The surface cannot be restored because it is an implicitly created surface."; break;
	case DDERR_NOTPALETTIZED: out = "DDERR_NOTPALETTIZED: The surface being used is not a palette-based surface."; break;
	case DDERR_UNSUPPORTEDMODE: out = "DDERR_UNSUPPORTEDMODE: The display is currently in an unsupported mode."; break;
	case DDERR_NOMIPMAPHW: out = "DDERR_NOMIPMAPHW: No mipmap-capable texture mapping hardware is present or available."; break;
	case DDERR_INVALIDSURFACETYPE: out = "DDERR_INVALIDSURFACETYPE: The surface was of the wrong type."; break;
	case DDERR_DCALREADYCREATED: out = "DDERR_DCALREADYCREATED: A device context (DC) has already been returned for this surface. Only one DC can be retrieved for each surface."; break;
	case DDERR_CANTPAGELOCK: out = "DDERR_CANTPAGELOCK: An attempt to page-lock a surface failed. Page lock does not work on a display-memory surface or an emulated primary surface."; break;
	case DDERR_CANTPAGEUNLOCK: out = "DDERR_CANTPAGEUNLOCK: An attempt to page-unlock a surface failed. Page unlock does not work on a display-memory surface or an emulated primary surface."; break;
	case DDERR_NOTPAGELOCKED: out = "DDERR_NOTPAGELOCKED: An attempt was made to page-unlock a surface with no outstanding page locks."; break;
	case DDERR_NOTINITIALIZED: out = "DDERR_NOTINITIALIZED: An attempt was made to call an interface method of a DirectDraw object created by CoCreateInstance before the object was initialized."; break;

	default: out = "Unknown Error"; break;
	}

	//sprintf(string, "DirectDraw Error %s\n", dderr);
	//OutputDebugString(string);

	return out;
}