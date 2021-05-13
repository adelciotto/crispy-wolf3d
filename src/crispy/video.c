#include "video.h"

#include <stdbool.h>
#include <time.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#include "log.h"
#include "config.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_HEIGHT_4_3 240

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888

// 4k resolution 4096x2160 is 8,847,360
#define MAX_SCREEN_TEXTURE_PIXELS 8847360

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_RendererInfo rendererInfo;
static SDL_Surface *screenSurface = NULL;
static SDL_Surface *rgbaSurface = NULL;
static SDL_Texture *intermediateTexture = NULL;
static SDL_Texture *upscaledTexture = NULL;

static int screenWidth;
static int screenHeight;
static int actualScreenHeight;
static int windowScale;
static bool fullscreen;
static bool aspectRatioCorrection;
static bool highRes;

static int lastTextureUpscaleW = 0;
static int lastTextureUpscaleH = 0;

static int setupScreen();

int crispyVideoStart(const char *title)
{
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
							  SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight,
							  SDL_WINDOW_ALLOW_HIGHDPI);
	if (!window) {
		crispyLogError("Unable to create SDL_Window %ix%i: %s", screenWidth,
					   actualScreenHeight, SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(
			window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		crispyLogError("Unable to create SDL_Renderer: %s", SDL_GetError());
		return -1;
	}

	if (SDL_GetRendererInfo(renderer, &rendererInfo) != 0) {
		crispyLogError("Unable to get SDL_RenderInfo: %s", SDL_GetError());
		return -1;
	}

	crispyLogInfo("Crispy video initialized with driver: %s",
				  rendererInfo.name);

	crispyVideoSetWindowScale((int)g_crispyConfigWindowScale);
	crispyVideoSetHighRes(g_crispyConfigHighRes);
	crispyVideoSetAspectRatioCorrection(g_crispyConfigAspectRatioCorrection);
	crispyVideoSetFullscreen(g_crispyConfigFullscreen);

	if (highRes) {
		screenWidth = SCREEN_WIDTH * 2;
		screenHeight = SCREEN_HEIGHT * 2;
		actualScreenHeight =
				aspectRatioCorrection ? SCREEN_HEIGHT_4_3 * 2 : screenHeight;
	} else {
		screenWidth = SCREEN_WIDTH;
		screenHeight = SCREEN_HEIGHT;
		actualScreenHeight =
				aspectRatioCorrection ? SCREEN_HEIGHT_4_3 : screenHeight;
	}

	if (setupScreen() != 0) {
		crispyLogError("Failed to setup the screen for drawing!");
		return -1;
	}

	if (crispyVideoApply() != 0) {
		crispyLogError("Failed to apply crispy video settings!");
		return -1;
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_ShowWindow(window);

	return 0;
}

void crispyVideoShutdown()
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

	crispyLogInfo("Crispy video resources destroyed");
}

bool crispyVideoGetFullscreen()
{
	return fullscreen;
}

void crispyVideoSetFullscreen(bool toggleOn)
{
	fullscreen = toggleOn;
}

int crispyVideoGetWindowScale()
{
	return windowScale;
}

void crispyVideoSetWindowScale(int scale)
{
	if (scale <= 0) {
		scale = 1;
	}

	windowScale = scale;
}

bool crispyVideoGetAspectRatioCorrection()
{
	return aspectRatioCorrection;
}

void crispyVideoSetAspectRatioCorrection(bool toggleOn)
{
	aspectRatioCorrection = toggleOn;
}

void crispyVideoSetHighRes(bool toggleOn)
{
	highRes = toggleOn;
}

static int setupUpscaleTexture();

int crispyVideoApply()
{
	if (aspectRatioCorrection) {
		actualScreenHeight =
				highRes ? SCREEN_HEIGHT_4_3 * 2 : SCREEN_HEIGHT_4_3;
		if (SDL_RenderSetLogicalSize(renderer, screenWidth,
									 actualScreenHeight) != 0) {
			crispyLogError(
					"Failed to set the renderer logical size to %dx%d! %s",
					screenWidth, actualScreenHeight, SDL_GetError());
		}
	} else {
		actualScreenHeight = highRes ? SCREEN_HEIGHT * 2 : SCREEN_HEIGHT;
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

	if (setupUpscaleTexture() != 0) {
		return -1;
	}

	crispyVideoPresent();

	return 0;
}

SDL_Surface *crispyVideoGetRGBASurface()
{
	return rgbaSurface;
}

SDL_Surface *crispyVideoGetPaletteSurface()
{
	return screenSurface;
}

void crispyVideoGetScreenSize(int *w, int *h)
{
	*w = screenWidth;
	*h = screenHeight;
}

void crispyVideoSetPaletteColors(SDL_Color *colors, int first, int numColors)
{
	SDL_SetPaletteColors(screenSurface->format->palette, colors, first,
						 numColors);
}

void crispyVideoSwapSurfaces()
{
	SDL_BlitSurface(screenSurface, NULL, rgbaSurface, NULL);
}

void crispyVideoPresent()
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

void crispyVideoScreenshot(const char *configDir)
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
		crispyLogError("Failed to get the renderer output size: %s",
					   SDL_GetError());
		return;
	}

	int stride = w * comp;
	unsigned char *pixels = malloc(h * stride);
	if (!pixels) {
		crispyLogError("Failed to get malloc pixel buffer for screenshot");
		return;
	}

	if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB24, pixels,
							 stride) != 0) {
		crispyLogError("Failed to read renderer pixels: %s", SDL_GetError());
		return;
	}

	if (stbi_write_png(imagePath, w, h, comp, pixels, stride) == 0)
		crispyLogError("Failed to save screenshot");
	else
		crispyLogInfo("Screenshot saved to %s!", imagePath);

	free(pixels);
}

static int setupScreen()
{
	// Create the indexed screen surface which the game will draw into using a
	// color palette.
	screenSurface =
			SDL_CreateRGBSurface(0, screenWidth, screenHeight, 8, 0, 0, 0, 0);
	if (!screenSurface) {
		crispyLogError("Failed to create screen surface: %s!", SDL_GetError());
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
		crispyLogError("Failed to create rgba surface: %s!", SDL_GetError());
		return -1;
	}

	// Create the intermediate texture that we render the rgba surface into.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	intermediateTexture = SDL_CreateTexture(renderer, PIXEL_FORMAT,
											SDL_TEXTUREACCESS_STREAMING,
											screenWidth, screenHeight);
	if (!intermediateTexture) {
		crispyLogError("Failed to create intermediate texture: %s!",
					   SDL_GetError());
		return -1;
	}

	crispyLogInfo(
			"Crispy video screen initialized!, Surface size: %dx%d, Pixel format: %s, Logical size: %dx%d",
			screenWidth, screenHeight, SDL_GetPixelFormatName(PIXEL_FORMAT),
			screenWidth, actualScreenHeight);

	return 0;
}

static int limitScreenTextureSize(int *widthUpscale, int *heightUpscale)
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
		crispyLogError(
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

static int getScreenTextureUpscale(int *widthUpscale, int *heightUpscale)
{
	int w, h;
	if (SDL_GetRendererOutputSize(renderer, &w, &h) != 0) {
		crispyLogError("Failed to get the renderer output size: %s",
					   SDL_GetError());
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

	return limitScreenTextureSize(widthUpscale, heightUpscale);
}

static int setupUpscaleTexture()
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
	if (getScreenTextureUpscale(&widthUpscale, &heightUpscale) != 0) {
		return -1;
	}

	if (widthUpscale == lastTextureUpscaleW &&
		heightUpscale == lastTextureUpscaleH) {
		return 0;
	}

	crispyLogInfo(
			"Texture upscale has changed! Initializing the upscale texture...");

	int screenTextureW = screenWidth * widthUpscale;
	int screenTextureH = screenHeight * heightUpscale;
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_Texture *newUpscaledTexture =
			SDL_CreateTexture(renderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_TARGET,
							  screenTextureW, screenTextureH);
	if (!newUpscaledTexture) {
		crispyLogError("Failed to create upscaled texture: %s!",
					   SDL_GetError());
		return -1;
	}

	// Destroy the upscaled texture if it already exists.
	if (upscaledTexture) {
		SDL_DestroyTexture(upscaledTexture);
	}

	upscaledTexture = newUpscaledTexture;

	lastTextureUpscaleW = screenTextureW;
	lastTextureUpscaleH = screenTextureH;

	crispyLogInfo(
			"Crispy video upscale texture initialized! Size: %dx%d, upscale: %dx%d",
			screenTextureW, screenTextureH, widthUpscale, heightUpscale);

	return 0;
}
