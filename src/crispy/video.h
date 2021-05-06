#ifndef CRISPY_WOLF3D_VIDEO_H
#define CRISPY_WOLF3D_VIDEO_H

#include "SDL.h"

int CrispyVideoStart();
void CrispyVideoShutdown();

SDL_Surface *CrispyVideoGetRGBASurface();
SDL_Surface *CrispyVideoGetPalettedSurface();
void CrispyVideoGetScreenSize(int *w, int *h);
void CrispyVideoSetPaletteColors(SDL_Color *colors, int first, int numColors);
void CrispyVideoSwapSurfaces();

void CrispyVideoPresent();

void CrispyVideoScreenshot(const char *configDir);

#endif // CRISPY_WOLF3D_VIDEO_H
