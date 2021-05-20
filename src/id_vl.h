// ID_VL.H

#ifndef __ID_VL_H_
#define __ID_VL_H_

void Quit(const char *error, ...);

extern SDL_Surface *screenSurface;
extern SDL_Surface *rgbaSurface;
extern int screenWidth, screenHeight;
extern int bufferPitch;
extern int screenBits;
extern int scaleFactor;

extern boolean usedoublebuffering;
extern boolean screenfaded;
extern uint32_t *ylookup;
extern SDL_Color gamepal[256];

//===========================================================================

#define VL_WaitVBL(a) SDL_Delay((a)*8)
#define VL_ClearScreen(c) SDL_FillRect(screenSurface, NULL, (c))

void VL_DePlaneVGA(byte *source, int width, int height);

void VL_SetVGAPlaneMode(void);

void VL_Shutdown(void);

void VL_ConvertPalette(byte *srcpal, SDL_Color *destpal, int numColors);

void VL_FillPalette(int red, int green, int blue);

void VL_SetPalette(SDL_Color *palette, boolean forceupdate);

void VL_GetPalette(SDL_Color *palette);

void VL_FadeOut(int start, int end, int red, int green, int blue, int steps);

void VL_FadeIn(int start, int end, SDL_Color *palette, int steps);

byte *VL_LockSurface(SDL_Surface *surface);

void VL_UnlockSurface(SDL_Surface *surface);

byte VL_GetPixel(int x, int y);

void VL_Plot(int x, int y, int color);

void VL_Hlin(unsigned x, unsigned y, unsigned width, int color);

void VL_Vlin(int x, int y, int height, int color);

void VL_BarScaledCoord(int scx, int scy, int scwidth, int scheight, int color);

void VL_Bar(int x, int y, int width, int height, int color);

void VL_ScreenToScreen(SDL_Surface *source, SDL_Surface *dest);

void VL_MemToScreenScaledCoord(byte *source, int width, int height, int scx,
							   int scy);

void VL_MemToScreenScaledCoord2(byte *source, int origwidth, int origheight,
								int srcx, int srcy, int destx, int desty,
								int width, int height);

void VL_MemToScreen(byte *source, int width, int height, int x, int y);

void VL_HandleWindowEvent(SDL_Event *sdlevent);

void VL_SetPaletteColors(SDL_Color *colors, int first, int numColors);

void VL_SwapSurfaces();

void VL_Present();

boolean VL_GetWindowFocus();

void VL_SetWindowScale(int newWindowScale);

void VL_SetHighRes(boolean highResOn);

void VL_SetAspectRatioCorrection(boolean aspectRatioCorrectionOn);

boolean VL_GetFullscreen();

void VL_SetFullscreen(boolean fullscreenOn);

int VL_Apply();

void VL_GetWindowSize(int *outW, int *outH);

void VL_WarpMouseInWindow(int x, int y);

void VL_Screenshot(const char *configDir);

#endif
