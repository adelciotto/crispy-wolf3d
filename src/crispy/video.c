#include "video.h"

#include <stdbool.h>
#include <time.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#include "log.h"
#include "config.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define HIGHRES_SCREEN_WIDTH 640
#define HIGHRES_SCREEN_HEIGHT 400

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888

// 4k resolution 4096x2160 is 8,847,360
#define MAX_SCREEN_TEXTURE_PIXELS 8847360

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_RendererInfo rendererInfo;

static SDL_Surface *palettedSurface = NULL;
static SDL_Surface *rgbaSurface = NULL;

static SDL_Texture *intermediateTexture = NULL;
static SDL_Texture *screenTexture = NULL;

static int screenWidth, screenHeight;
static int actualScreenHeight;

static bool takingScreenshot = false;

static int getScreenTextureUpscale(int *widthUpscale, int *heightUpscale);

int CrispyVideoStart(const char *title)
{
    if (CrispyConfigHighRes)
    {
        screenWidth = HIGHRES_SCREEN_WIDTH;
        screenHeight = HIGHRES_SCREEN_HEIGHT;
    }
    else
    {
        screenWidth = SCREEN_WIDTH;
        screenHeight = SCREEN_HEIGHT;
    }

    if (CrispyConfigAspectRatioCorrection)
    {
        actualScreenHeight = screenWidth * 3.0 / 4.0;
    }
    else
    {
        actualScreenHeight = screenHeight;
    }

    int winScale = CrispyConfigWindowScale;
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth * winScale,
                              actualScreenHeight * winScale, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
    {
        CrispyLogError("Unable to create SDL_Window %ix%i: %s", screenWidth, actualScreenHeight, SDL_GetError());
        return -1;
    }

    SDL_ShowCursor(SDL_DISABLE);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        CrispyLogError("Unable to create SDL_Renderer: %s", SDL_GetError());
        return -1;
    }

    if (SDL_GetRendererInfo(renderer, &rendererInfo) != 0)
    {
        CrispyLogError("Unable to get SDL_RenderInfo: %s", SDL_GetError());
        return -1;
    }

    // Create the indexed screen surface which the game will draw into using a color palette.
    palettedSurface = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 8, 0, 0, 0, 0);
    if (!palettedSurface)
    {
        CrispyLogError("Unable to create paletted surface: %s", SDL_GetError());
        return -1;
    }

    // Create the screen surface which will contain a 32bit ARGB version of the indexed screen.
    uint32_t rmask, gmask, bmask, amask;
    int bpp;
    SDL_PixelFormatEnumToMasks(PIXEL_FORMAT, &bpp, &rmask, &gmask, &bmask, &amask);
    rgbaSurface = SDL_CreateRGBSurface(0, screenWidth, screenHeight, bpp, rmask, gmask, bmask, amask);
    if (!rgbaSurface)
    {
        CrispyLogError("Unable to create rgba surface: %s", SDL_GetError());
        return -1;
    }

    // Create the intermediate texture that we render the rgba surface into.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    intermediateTexture =
        SDL_CreateTexture(renderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);
    if (!intermediateTexture)
    {
        CrispyLogError("Unable to create intermediate texture: %s", SDL_GetError());
        return -1;
    }

    if (CrispyConfigAspectRatioCorrection)
    {
        SDL_RenderSetLogicalSize(renderer, screenWidth, actualScreenHeight);
    }

    if (CrispyConfigFullscreen)
    {
        SDL_DisplayMode dm;
        SDL_GetCurrentDisplayMode(0, &dm);
        SDL_SetWindowSize(window, dm.w, dm.h);

        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }

    // Create the final screen texture that is an integer scaled up version of the intermediate texture. The scale is
    // determined by the window size. If the window aspect ratio differs from the CRT ratio (640 / 480) then the screen
    // texture will be scaled down to fit on the screen using 'linear' smooth scaling.
    //
    // For example, on a window of size 1920x1080, the screen texture would be size 1920x1440 (scale of 3).
    // This would then be scaled down smoothly to fit into the window.
    // Credit to crispy-doom for this strategy: https://github.com/fabiangreffrath/crispy-doom/blob/master/src/i_video.c
    int widthUpscale = 1, heightUpscale = 1;
    if (getScreenTextureUpscale(&widthUpscale, &heightUpscale) != 0)
    {
        return -1;
    }
    int screenTextureW = screenWidth * widthUpscale;
    int screenTextureH = screenHeight * heightUpscale;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    screenTexture = SDL_CreateTexture(renderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_TARGET, screenTextureW, screenTextureH);
    if (!screenTexture)
    {
        CrispyLogError("Unable to create screen texture: %s", SDL_GetError());
        return -1;
    }

    CrispyLogInfo("Crispy video initialized with driver: %s", rendererInfo.name);
    CrispyLogInfo(
        "Surface size: %dx%d, Pixel format: %s, Logical size: %dx%d, Screen texture [size: %dx%d, upscale: %dx%d]",
        screenWidth, screenHeight, SDL_GetPixelFormatName(PIXEL_FORMAT), screenWidth, actualScreenHeight,
        screenTextureW, screenTextureH, widthUpscale, heightUpscale);

    return 0;
}

void CrispyVideoShutdown()
{
    if (screenTexture)
        SDL_DestroyTexture(screenTexture);
    if (intermediateTexture)
        SDL_DestroyTexture(intermediateTexture);
    if (rgbaSurface)
        SDL_FreeSurface(rgbaSurface);
    if (palettedSurface)
        SDL_FreeSurface(palettedSurface);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);

    CrispyLogInfo("Crispy video resources destroyed");
}

SDL_Surface *CrispyVideoGetRGBASurface()
{
    // TODO(Anthony): Assert this is not NULL
    return rgbaSurface;
}

SDL_Surface *CrispyVideoGetPalettedSurface()
{
    // TODO(Anthony): Assert this is not NULL
    return palettedSurface;
}

void CrispyVideoGetScreenSize(int *w, int *h)
{
    *w = screenWidth;
    *h = screenHeight;
}

void CrispyVideoSetPaletteColors(SDL_Color *colors, int first, int numColors)
{
    SDL_SetPaletteColors(palettedSurface->format->palette, colors, first, numColors);
}

void CrispyVideoSwapSurfaces()
{
    SDL_BlitSurface(palettedSurface, NULL, rgbaSurface, NULL);
}

void CrispyVideoPresent()
{
    SDL_UpdateTexture(intermediateTexture, NULL, rgbaSurface->pixels, rgbaSurface->pitch);
    SDL_RenderClear(renderer);

    // Render the CRT texture to the screen texture using 'nearest' scaling.
    SDL_SetRenderTarget(renderer, screenTexture);
    SDL_RenderCopy(renderer, intermediateTexture, NULL, NULL);

    // Render the screen texture to the window using 'linear' scaling.
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);

    SDL_RenderPresent(renderer);
}

void CrispyVideoScreenshot(const char *configDir)
{
    if (takingScreenshot)
        return;

    takingScreenshot = true;

    char imagePath[300];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    if (configDir[0])
        snprintf(imagePath, sizeof(imagePath), "%s/%s_%d-%02d-%02dT%02d-%02d-02d.png", configDir, "screenshot",
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    else
        snprintf(imagePath, sizeof(imagePath), "%s_%d-%02d-%02dT%02d-%02d-%02d.png", "screenshot", tm.tm_year + 1900,
                 tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    int comp = 3;
    int w, h;
    if (SDL_GetRendererOutputSize(renderer, &w, &h) != 0)
    {
        CrispyLogError("Failed to get the renderer output size: %s", SDL_GetError());
        return;
    }

    int stride = w * comp;
    unsigned char *pixels = malloc(h * stride);
    if (!pixels)
    {
        CrispyLogError("Failed to get malloc pixel buffer for screenshot");
        return;
    }

    if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB24, pixels, stride) != 0)
    {
        CrispyLogError("Failed to read renderer pixels: %s", SDL_GetError());
        return;
    }

    if (stbi_write_png(imagePath, w, h, comp, pixels, stride) == 0)
        CrispyLogError("Failed to save screenshot");
    else
        CrispyLogInfo("Screenshot saved to %s!", imagePath);

    free(pixels);

    takingScreenshot = false;
}

static int limitScreenTextureSize(int *widthUpscale, int *heightUpscale)
{
    int maxTextureWidth = rendererInfo.max_texture_width;
    int maxTextureHeight = rendererInfo.max_texture_height;

    while (*widthUpscale * screenWidth > maxTextureWidth)
    {
        --*widthUpscale;
    }
    while (*heightUpscale * screenHeight > maxTextureHeight)
    {
        --*heightUpscale;
    }

    if ((*widthUpscale < 1 && maxTextureWidth > 0) || (*heightUpscale < 1 && maxTextureHeight > 0))
    {
        CrispyLogError("Unable to create a texture big enough for the whole screen! Maximum texture size %dx%d",
                       maxTextureWidth, maxTextureWidth);
        return -1;
    }

    // We limit the amount of texture memory used for the screen texture,
    // since beyond a certain point there are diminishing returns. Also,
    // depending on the hardware there may be performance problems with very huge textures.
    while (*widthUpscale * *heightUpscale * screenWidth * screenHeight > MAX_SCREEN_TEXTURE_PIXELS)
    {
        if (*widthUpscale > *heightUpscale)
        {
            --*widthUpscale;
        }
        else
        {
            --*heightUpscale;
        }
    }

    return 0;
}

int getScreenTextureUpscale(int *widthUpscale, int *heightUpscale)
{
    int w, h;
    if (SDL_GetRendererOutputSize(renderer, &w, &h) != 0)
    {
        CrispyLogError("Failed to get the renderer output size: %s", SDL_GetError());
        return -1;
    }

    if (w > h)
    {
        // Wide window.
        w = h * (screenWidth / actualScreenHeight);
    }
    else
    {
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