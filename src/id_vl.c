#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <string.h>
#include <time.h>

#include "wl_def.h"

#pragma hdrstop

// Uncomment the following line, if you get destination out of bounds
// assertion errors and want to ignore them during debugging
//#define IGNORE_BAD_DEST

#ifdef IGNORE_BAD_DEST
#undef assert
#define assert(x)                                                              \
	if (!(x))                                                                  \
	return
#define assert_ret(x)                                                          \
	if (!(x))                                                                  \
	return 0
#else
#define assert_ret(x) assert(x)
#endif

int screenWidth;
int screenHeight;
int screenBits = 32;
SDL_Surface *screenSurface = NULL;
SDL_Surface *rgbaSurface = NULL;
int bufferPitch;
int scaleFactor;

boolean usedoublebuffering = true;
boolean screenfaded;

uint32_t *ylookup;
SDL_Color palette1[256], palette2[256];
SDL_Color curpal[256];

#define CASSERT(x) extern int ASSERT_COMPILE[((x) != 0) * 2 - 1];
#define RGB(r, g, b)                                                           \
	{                                                                          \
		(r) * 255 / 63, (g)*255 / 63, (b)*255 / 63, 0                          \
	}

SDL_Color gamepal[] = {
#ifdef SPEAR
#include "sodpal.inc"
#else

#include "wolfpal.inc"

#endif
};

CASSERT(lengthof(gamepal) == 256)

static int SetupSDLVideo(const char *title);
static void ShutdownSDLVideo();

void VL_Shutdown(void)
{
	ShutdownSDLVideo();

	free(ylookup);
	free(pixelangle);
	free(wallheight);
	screenSurface = NULL;
	rgbaSurface = NULL;
	ylookup = NULL;
	pixelangle = NULL;
	wallheight = NULL;
}

void VL_SetVGAPlaneMode(void)
{
	int i;
	uint32_t a, r, g, b;

#ifdef SPEAR
	const char *title = "Spear of Destiny";
#else
	const char *title = "Wolfenstein 3D";
#endif

	if (SetupSDLVideo(title) != 0) {
		Quit("Failed to setup SDL video");
	}

	VL_SetPaletteColors(gamepal, 0, 256);

	bufferPitch = screenSurface->pitch;

	scaleFactor = screenWidth / 320;
	if (screenHeight / 200 < scaleFactor)
		scaleFactor = screenHeight / 200;

	ylookup = SafeMalloc(screenHeight * sizeof(*ylookup));
	pixelangle = SafeMalloc(screenWidth * sizeof(*pixelangle));
	wallheight = SafeMalloc(screenWidth * sizeof(*wallheight));

	for (i = 0; i < screenHeight; i++)
		ylookup[i] = i * bufferPitch;
}

/*
=============================================================================

                        PALETTE OPS

        To avoid snow, do a WaitVBL BEFORE calling these

=============================================================================
*/

void VL_ConvertPalette(byte *srcpal, SDL_Color *destpal, int numColors)
{
	int i;

	for (i = 0; i < numColors; i++) {
		destpal[i].r = *srcpal++ * 255 / 63;
		destpal[i].g = *srcpal++ * 255 / 63;
		destpal[i].b = *srcpal++ * 255 / 63;
	}
}

void VL_FillPalette(int red, int green, int blue)
{
	int i;
	SDL_Color pal[256];

	for (i = 0; i < 256; i++) {
		pal[i].r = red;
		pal[i].g = green;
		pal[i].b = blue;
	}

	VL_SetPalette(pal, true);
}

void VL_SetColor(int color, int red, int green, int blue)
{
	SDL_Color col = { (Uint8)red, (Uint8)green, (Uint8)blue };
	curpal[color] = col;

	VL_SetPaletteColors(&col, color, 1);
	VL_SwapSurfaces();
	VL_Present();
}

void VL_GetColor(int color, int *red, int *green, int *blue)
{
	SDL_Color *col = &curpal[color];
	*red = col->r;
	*green = col->g;
	*blue = col->b;
}

void VL_SetPalette(SDL_Color *palette, boolean forceupdate)
{
	memcpy(curpal, palette, sizeof(SDL_Color) * 256);

	VL_SetPaletteColors(palette, 0, 256);
	if (forceupdate) {
		VL_SwapSurfaces();
		VL_Present();
	}
}

void VL_GetPalette(SDL_Color *palette)
{
	memcpy(palette, curpal, sizeof(SDL_Color) * 256);
}

void VL_FadeOut(int start, int end, int red, int green, int blue, int steps)
{
	int i, j, orig, delta;
	SDL_Color *origptr, *newptr;

	red = red * 255 / 63;
	green = green * 255 / 63;
	blue = blue * 255 / 63;

	VL_WaitVBL(1);
	VL_GetPalette(palette1);
	memcpy(palette2, palette1, sizeof(SDL_Color) * 256);

	//
	// fade through intermediate frames
	//
	for (i = 0; i < steps; i++) {
		origptr = &palette1[start];
		newptr = &palette2[start];
		for (j = start; j <= end; j++) {
			orig = origptr->r;
			delta = red - orig;
			newptr->r = orig + delta * i / steps;
			orig = origptr->g;
			delta = green - orig;
			newptr->g = orig + delta * i / steps;
			orig = origptr->b;
			delta = blue - orig;
			newptr->b = orig + delta * i / steps;
			origptr++;
			newptr++;
		}

		if (!usedoublebuffering || screenBits == 8)
			VL_WaitVBL(1);
		VL_SetPalette(palette2, true);
	}

	// final color
	VL_FillPalette(red, green, blue);

	screenfaded = true;
}

void VL_FadeIn(int start, int end, SDL_Color *palette, int steps)
{
	int i, j, delta;

	VL_WaitVBL(1);
	VL_GetPalette(palette1);
	memcpy(palette2, palette1, sizeof(SDL_Color) * 256);

	// fade through intermediate frames
	for (i = 0; i < steps; i++) {
		for (j = start; j <= end; j++) {
			delta = palette[j].r - palette1[j].r;
			palette2[j].r = palette1[j].r + delta * i / steps;
			delta = palette[j].g - palette1[j].g;
			palette2[j].g = palette1[j].g + delta * i / steps;
			delta = palette[j].b - palette1[j].b;
			palette2[j].b = palette1[j].b + delta * i / steps;
		}

		if (!usedoublebuffering || screenBits == 8)
			VL_WaitVBL(1);
		VL_SetPalette(palette2, true);
	}

	// final color
	VL_SetPalette(palette, true);
	screenfaded = false;
}

/*
=============================================================================

                            PIXEL OPS

=============================================================================
*/

byte *VL_LockSurface(SDL_Surface *surface)
{
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0)
			return NULL;
	}
	return (byte *)surface->pixels;
}

void VL_UnlockSurface(SDL_Surface *surface)
{
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

void VL_Plot(int x, int y, int color)
{
	byte *dest;

	assert(x >= 0 && (unsigned)x < screenWidth && y >= 0 &&
		   (unsigned)y < screenHeight && "VL_Plot: Pixel out of bounds!");

	dest = VL_LockSurface(screenSurface);
	if (dest == NULL)
		return;

	dest[ylookup[y] + x] = color;

	VL_UnlockSurface(screenSurface);
}

byte VL_GetPixel(int x, int y)
{
	byte col;

	assert_ret(x >= 0 && (unsigned)x < screenWidth && y >= 0 &&
			   (unsigned)y < screenHeight &&
			   "VL_GetPixel: Pixel out of bounds!");

	if (!VL_LockSurface(screenSurface))
		return 0;

	col = ((byte *)screenSurface->pixels)[ylookup[y] + x];

	VL_UnlockSurface(screenSurface);

	return col;
}

void VL_Hlin(unsigned x, unsigned y, unsigned width, int color)
{
	byte *dest;

	assert(x >= 0 && x + width <= screenWidth && y >= 0 && y < screenHeight &&
		   "VL_Hlin: Destination rectangle out of bounds!");

	dest = VL_LockSurface(screenSurface);
	if (dest == NULL)
		return;

	memset(dest + ylookup[y] + x, color, width);

	VL_UnlockSurface(screenSurface);
}

void VL_Vlin(int x, int y, int height, int color)
{
	byte *dest;

	assert(x >= 0 && (unsigned)x < screenWidth && y >= 0 &&
		   (unsigned)y + height <= screenHeight &&
		   "VL_Vlin: Destination rectangle out of bounds!");

	dest = VL_LockSurface(screenSurface);
	if (dest == NULL)
		return;

	dest += ylookup[y] + x;

	while (height--) {
		*dest = color;
		dest += bufferPitch;
	}

	VL_UnlockSurface(screenSurface);
}

void VL_Bar(int x, int y, int width, int height, int color)
{
	VL_BarScaledCoord(scaleFactor * x, scaleFactor * y, scaleFactor * width,
					  scaleFactor * height, color);
}

void VL_BarScaledCoord(int scx, int scy, int scwidth, int scheight, int color)
{
	byte *dest;

	assert(scx >= 0 && (unsigned)scx + scwidth <= screenWidth && scy >= 0 &&
		   (unsigned)scy + scheight <= screenHeight &&
		   "VL_BarScaledCoord: Destination rectangle out of bounds!");

	dest = VL_LockSurface(screenSurface);
	if (dest == NULL)
		return;

	dest += ylookup[scy] + scx;

	while (scheight--) {
		memset(dest, color, scwidth);
		dest += bufferPitch;
	}
	VL_UnlockSurface(screenSurface);
}

/*
============================================================================

                            MEMORY OPS

============================================================================
*/

void VL_DePlaneVGA(byte *source, int width, int height)
{
	int x, y, plane;
	word size, pwidth;
	byte *temp, *dest, *srcline;

	size = width * height;

	if (width & 3)
		Quit("DePlaneVGA: width not divisible by 4!");

	temp = SafeMalloc(size);

	//
	// munge pic into the temp buffer
	//
	srcline = source;
	pwidth = width >> 2;

	for (plane = 0; plane < 4; plane++) {
		dest = temp;

		for (y = 0; y < height; y++) {
			for (x = 0; x < pwidth; x++)
				*(dest + (x << 2) + plane) = *srcline++;

			dest += width;
		}
	}

	// copy the temp buffer back into the original source
	memcpy(source, temp, size);

	free(temp);
}

void VL_MemToScreen(byte *source, int width, int height, int x, int y)
{
	VL_MemToScreenScaledCoord(source, width, height, scaleFactor * x,
							  scaleFactor * y);
}

void VL_MemToScreenScaledCoord(byte *source, int width, int height, int destx,
							   int desty)
{
	byte *dest;
	int i, j, sci, scj;
	unsigned m, n;

	assert(destx >= 0 && destx + width * scaleFactor <= screenWidth &&
		   desty >= 0 && desty + height * scaleFactor <= screenHeight &&
		   "VL_MemToScreenScaledCoord: Destination rectangle out of bounds!");

	dest = VL_LockSurface(screenSurface);
	if (dest == NULL)
		return;

	for (j = 0, scj = 0; j < height; j++, scj += scaleFactor) {
		for (i = 0, sci = 0; i < width; i++, sci += scaleFactor) {
			byte col = source[(j * width) + i];
			for (m = 0; m < scaleFactor; m++) {
				for (n = 0; n < scaleFactor; n++) {
					dest[ylookup[scj + m + desty] + sci + n + destx] = col;
				}
			}
		}
	}
	VL_UnlockSurface(screenSurface);
}

/*
=================
=
= VL_MemToScreenScaledCoord
=
= Draws a part of a block of data to the screen.
= The block has the size origwidth*origheight.
= The part at (srcx, srcy) has the size width*height
= and will be painted to (destx, desty) with scaling according to scaleFactor.
=
=================
*/

void VL_MemToScreenScaledCoord2(byte *source, int origwidth, int origheight,
								int srcx, int srcy, int destx, int desty,
								int width, int height)
{
	byte *dest;
	int i, j, sci, scj;
	unsigned m, n;

	assert(destx >= 0 && destx + width * scaleFactor <= screenWidth &&
		   desty >= 0 && desty + height * scaleFactor <= screenHeight &&
		   "VL_MemToScreenScaledCoord: Destination rectangle out of bounds!");

	dest = VL_LockSurface(screenSurface);
	if (dest == NULL)
		return;

	for (j = 0, scj = 0; j < height; j++, scj += scaleFactor) {
		for (i = 0, sci = 0; i < width; i++, sci += scaleFactor) {
			byte col = source[((j + srcy) * origwidth) + (i + srcx)];

			for (m = 0; m < scaleFactor; m++) {
				for (n = 0; n < scaleFactor; n++) {
					dest[ylookup[scj + m + desty] + sci + n + destx] = col;
				}
			}
		}
	}
	VL_UnlockSurface(screenSurface);
}

void VL_ScreenToScreen(SDL_Surface *source, SDL_Surface *dest)
{
	SDL_BlitSurface(source, NULL, dest, NULL);
}

/*
=============================================================================

                        SDL Video Functions

=============================================================================
*/

#define ORIGINAL_SCREENWIDTH 320
#define ORIGINAL_SCREENHEIGHT 200
#define ORIGINAL_SCREENHEIGHT_4_3 240
#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
// 4k resolution 4096x2160 is 8,847,360
#define MAX_SCREEN_TEXTURE_PIXELS 8847360

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_RendererInfo rendererInfo;
static SDL_Texture *intermediateTexture = NULL;
static SDL_Texture *upscaledTexture = NULL;

static int actualScreenHeight;
static int windowScale;
static boolean fullscreen;
static boolean aspectRatioCorrection;
static boolean highRes;
static int lastTextureUpscaleW = 0;
static int lastTextureUpscaleH = 0;
static boolean windowFocused = false;

static int SetupScreen();
static int SetupUpscaleTexture();

static int SetupSDLVideo(const char *title)
{
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
							  SDL_WINDOWPOS_CENTERED, ORIGINAL_SCREENWIDTH,
							  ORIGINAL_SCREENHEIGHT,
							  SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
	if (!window) {
		LOG_Error("Unable to create SDL_Window: %s", SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(
			window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		LOG_Error("Unable to create SDL_Renderer: %s", SDL_GetError());
		return -1;
	}

	if (SDL_GetRendererInfo(renderer, &rendererInfo) != 0) {
		LOG_Error("Unable to get SDL_RenderInfo: %s", SDL_GetError());
		return -1;
	}

	LOG_Info("SDL video initialized with driver: %s", rendererInfo.name);

	VL_SetWindowScale((int)ConfigOptionWindowScale);
	VL_SetHighRes(ConfigOptionHighRes);
	VL_SetAspectRatioCorrection(ConfigOptionAspectRatioCorrection);
	VL_SetFullscreen(ConfigOptionFullscreen);

	if (highRes) {
		screenWidth = ORIGINAL_SCREENWIDTH * 2;
		screenHeight = ORIGINAL_SCREENHEIGHT * 2;
		actualScreenHeight = aspectRatioCorrection ?
									   ORIGINAL_SCREENHEIGHT_4_3 * 2 :
									   screenHeight;
	} else {
		screenWidth = ORIGINAL_SCREENWIDTH;
		screenHeight = ORIGINAL_SCREENHEIGHT;
		actualScreenHeight = aspectRatioCorrection ? ORIGINAL_SCREENHEIGHT_4_3 :
													   screenHeight;
	}

	if (SetupScreen() != 0) {
		LOG_Error("Failed to setup the screen for drawing!");
		return -1;
	}

	if (VL_Apply() != 0) {
		LOG_Error("Failed to apply video settings!");
		return -1;
	}

	SDL_ShowWindow(window);
	windowFocused = true;

	return 0;
}

static void ShutdownSDLVideo()
{
	if (upscaledTexture)
		SDL_DestroyTexture(upscaledTexture);

	if (intermediateTexture)
		SDL_DestroyTexture(intermediateTexture);

	if (rgbaSurface)
		SDL_FreeSurface(rgbaSurface);

	if (screenSurface)
		SDL_FreeSurface(screenSurface);

	if (renderer)
		SDL_DestroyRenderer(renderer);

	if (window)
		SDL_DestroyWindow(window);
}

void VL_HandleWindowEvent(SDL_Event *sdlevent)
{
	switch (sdlevent->type) {
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		windowFocused = true;
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		windowFocused = false;
		break;
	}
}

void VL_SetPaletteColors(SDL_Color *colors, int first, int numColors)
{
	SDL_SetPaletteColors(screenSurface->format->palette, colors, first,
						 numColors);
}

void VL_SwapSurfaces()
{
	SDL_BlitSurface(screenSurface, NULL, rgbaSurface, NULL);
}

void VL_Present()
{
	SDL_UpdateTexture(intermediateTexture, NULL, rgbaSurface->pixels,
					  rgbaSurface->pitch);
	SDL_RenderClear(renderer);

	// Render the CRT texture to the upscaled texture using 'nearest' scaling.
	SDL_SetRenderTarget(renderer, upscaledTexture);
	SDL_RenderCopy(renderer, intermediateTexture, NULL, NULL);

	// Render the upscaled texture to the window using 'linear' scaling.
	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, upscaledTexture, NULL, NULL);

	SDL_RenderPresent(renderer);
}

boolean VL_GetWindowFocus()
{
	return windowFocused;
}

void VL_SetWindowScale(int newWindowScale)
{
	if (newWindowScale <= 0) {
		newWindowScale = 1;
	}

	windowScale = newWindowScale;
}

void VL_SetHighRes(boolean highResOn)
{
	highRes = highResOn;
}

void VL_SetAspectRatioCorrection(boolean aspectRatioCorrectionOn)
{
	aspectRatioCorrection = aspectRatioCorrectionOn;
}

boolean VL_GetFullscreen()
{
	return fullscreen;
}

void VL_SetFullscreen(boolean fullscreenOn)
{
	fullscreen = fullscreenOn;
}

int VL_Apply()
{
	if (aspectRatioCorrection) {
		actualScreenHeight = highRes ? ORIGINAL_SCREENHEIGHT_4_3 * 2 :
										 ORIGINAL_SCREENHEIGHT_4_3;
		if (SDL_RenderSetLogicalSize(renderer, screenWidth,
									 actualScreenHeight) != 0) {
			LOG_Error("Failed to set the renderer logical size to %dx%d! %s",
					  screenWidth, actualScreenHeight, SDL_GetError());
		}
	} else {
		actualScreenHeight =
				highRes ? ORIGINAL_SCREENHEIGHT * 2 : ORIGINAL_SCREENHEIGHT;
		SDL_RenderSetLogicalSize(renderer, 0, 0);
	}

	if (fullscreen) {
		SDL_DisplayMode dm;
		SDL_GetCurrentDisplayMode(0, &dm);
		SDL_SetWindowSize(window, dm.w, dm.h);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	} else {
		int w = screenWidth * windowScale;
		int h = actualScreenHeight * windowScale;
		SDL_SetWindowSize(window, w, h);
		SDL_SetWindowFullscreen(window, false);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,
							  SDL_WINDOWPOS_CENTERED);
	}

	if (SetupUpscaleTexture() != 0) {
		return -1;
	}

	VL_Present();

	return 0;
}

void VL_GetWindowSize(int *outW, int *outH)
{
	SDL_GetWindowSize(window, outW, outH);
}

void VL_WarpMouseInWindow(int x, int y)
{
	SDL_WarpMouseInWindow(window, x, y);
}

void VL_Screenshot(const char *configDir)
{
	char imagePath[300];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	if (configDir[0])
		snprintf(imagePath, sizeof(imagePath),
				 "%s/%s_%d-%02d-%02dT%02d-%02d-02d.png", configDir,
				 "screenshot", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				 tm.tm_hour, tm.tm_min, tm.tm_sec);
	else
		snprintf(imagePath, sizeof(imagePath),
				 "%s_%d-%02d-%02dT%02d-%02d-%02d.png", "screenshot",
				 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
				 tm.tm_min, tm.tm_sec);

	int comp = 3;
	int w, h;
	if (SDL_GetRendererOutputSize(renderer, &w, &h) != 0) {
		LOG_Error("Failed to get the renderer output size: %s", SDL_GetError());
		return;
	}

	int stride = w * comp;
	unsigned char *pixels = malloc(h * stride);
	if (!pixels) {
		LOG_Error("Failed to get malloc pixel buffer for screenshot");
		return;
	}

	if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB24, pixels,
							 stride) != 0) {
		LOG_Error("Failed to read renderer pixels: %s", SDL_GetError());
		return;
	}

	if (stbi_write_png(imagePath, w, h, comp, pixels, stride) == 0)
		LOG_Error("Failed to save screenshot");
	else
		LOG_Info("Screenshot saved to %s!", imagePath);

	free(pixels);
}

static int SetupScreen()
{
	// Create the indexed screen surface which the game will draw into using a
	// color palette.
	screenSurface =
			SDL_CreateRGBSurface(0, screenWidth, screenHeight, 8, 0, 0, 0, 0);
	if (!screenSurface) {
		LOG_Error("Failed to create screen surface: %s!", SDL_GetError());
		return -1;
	}

	// Create the screen surface which will contain a 32bit ARGB version of the
	// indexed screen.
	uint32_t rmask, gmask, bmask, amask;
	int bpp;
	SDL_PixelFormatEnumToMasks(PIXEL_FORMAT, &bpp, &rmask, &gmask, &bmask,
							   &amask);
	rgbaSurface = SDL_CreateRGBSurface(0, screenWidth, screenHeight, bpp, rmask,
									   gmask, bmask, amask);
	if (!rgbaSurface) {
		LOG_Error("Failed to create rgba surface: %s!", SDL_GetError());
		return -1;
	}

	// Create the intermediate texture that we render the rgba surface into.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	intermediateTexture = SDL_CreateTexture(renderer, PIXEL_FORMAT,
											SDL_TEXTUREACCESS_STREAMING,
											screenWidth, screenHeight);
	if (!intermediateTexture) {
		LOG_Error("Failed to create intermediate texture: %s!", SDL_GetError());
		return -1;
	}

	LOG_Info(
			"Crispy video screen initialized!, Surface size: %dx%d, Pixel format: %s, Logical size: %dx%d",
			screenWidth, screenHeight, SDL_GetPixelFormatName(PIXEL_FORMAT),
			screenWidth, actualScreenHeight);

	return 0;
}

static int LimitUpscaleTextureSize(int *widthUpscale, int *heightUpscale)
{
	int maxTextureWidth = rendererInfo.max_texture_width;
	int maxTextureHeight = rendererInfo.max_texture_height;

	while (*widthUpscale * screenWidth > maxTextureWidth) {
		--*widthUpscale;
	}
	while (*heightUpscale * screenHeight > maxTextureHeight) {
		--*heightUpscale;
	}

	if ((*widthUpscale < 1 && maxTextureWidth > 0) ||
		(*heightUpscale < 1 && maxTextureHeight > 0)) {
		LOG_Error(
				"Unable to create a texture big enough for the whole screen! Maximum texture size %dx%d",
				maxTextureWidth, maxTextureWidth);
		return -1;
	}

	// We limit the amount of texture memory used for the screen texture,
	// since beyond a certain point there are diminishing returns. Also,
	// depending on the hardware there may be performance problems with very
	// huge textures.
	while (*widthUpscale * *heightUpscale * screenWidth * screenHeight >
		   MAX_SCREEN_TEXTURE_PIXELS) {
		if (*widthUpscale > *heightUpscale) {
			--*widthUpscale;
		} else {
			--*heightUpscale;
		}
	}

	return 0;
}

static int GetTextureUpscale(int *widthUpscale, int *heightUpscale)
{
	int w, h;
	if (SDL_GetRendererOutputSize(renderer, &w, &h) != 0) {
		LOG_Error("Failed to get the renderer output size: %s", SDL_GetError());
		return -1;
	}

	if (w > h) {
		// Wide window.
		w = h * (screenWidth / actualScreenHeight);
	} else {
		// Tall window.
		h = w * (actualScreenHeight / screenWidth);
	}

	*widthUpscale = (w + screenWidth - 1) / screenWidth;
	*heightUpscale = (h + screenHeight - 1) / screenHeight;

	if (*widthUpscale < 1)
		*widthUpscale = 1;
	if (*heightUpscale < 1)
		*heightUpscale = 1;

	return LimitUpscaleTextureSize(widthUpscale, heightUpscale);
}

static int SetupUpscaleTexture()
{
	// Create the final upscaled texture that is an integer scaled up version
	// of the intermediate texture. The scale is determined by the window size.
	// If the window aspect ratio differs from the CRT ratio (640 / 480) then
	// the screen texture will be scaled down to fit on the screen using
	// 'linear' smooth scaling.
	//
	// For example, on a window of size 1920x1080, the screen texture would be
	// size 1920x1440 (scale of 3). This would then be scaled down smoothly to
	// fit into the window.
	// Credit to crispy-doom for this strategy:
	// https://github.com/fabiangreffrath/crispy-doom/blob/master/src/i_video.c
	int widthUpscale = 1, heightUpscale = 1;
	if (GetTextureUpscale(&widthUpscale, &heightUpscale) != 0) {
		return -1;
	}

	if (widthUpscale == lastTextureUpscaleW &&
		heightUpscale == lastTextureUpscaleH) {
		return 0;
	}

	LOG_Info(
			"Texture upscale has changed! Initializing the upscale texture...");

	int screenTextureW = screenWidth * widthUpscale;
	int screenTextureH = screenHeight * heightUpscale;
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_Texture *newUpscaledTexture =
			SDL_CreateTexture(renderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_TARGET,
							  screenTextureW, screenTextureH);
	if (!newUpscaledTexture) {
		LOG_Error("Failed to create upscaled texture: %s!", SDL_GetError());
		return -1;
	}

	// Destroy the upscaled texture if it already exists.
	if (upscaledTexture) {
		SDL_DestroyTexture(upscaledTexture);
	}

	upscaledTexture = newUpscaledTexture;

	lastTextureUpscaleW = screenTextureW;
	lastTextureUpscaleH = screenTextureH;

	LOG_Info(
			"SDL video upscale texture initialized! Size: %dx%d, upscale: %dx%d",
			screenTextureW, screenTextureH, widthUpscale, heightUpscale);

	return 0;
}
