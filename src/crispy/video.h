#ifndef CRISPY_WOLF3D_VIDEO_H
#define CRISPY_WOLF3D_VIDEO_H

#include "SDL.h"

#include <stdbool.h>

int crispyVideoStart();
void crispyVideoShutdown();
bool crispyVideoGetFullscreen();
void crispyVideoSetFullscreen(bool toggleOn);
int crispyVideoGetWindowScale();
void crispyVideoSetWindowScale(int scale);
bool crispyVideoGetAspectRatioCorrection();
void crispyVideoSetAspectRatioCorrection(bool toggleOn);
void crispyVideoSetHighRes(bool toggleOn);
int crispyVideoApply();

SDL_Surface *crispyVideoGetRGBASurface();
SDL_Surface *crispyVideoGetPaletteSurface();
void crispyVideoGetScreenSize(int *w, int *h);
void crispyVideoSetPaletteColors(SDL_Color *colors, int first, int numColors);
void crispyVideoSwapSurfaces();

void crispyVideoPresent();

void crispyVideoScreenshot(const char *configDir);

#endif // CRISPY_WOLF3D_VIDEO_H
