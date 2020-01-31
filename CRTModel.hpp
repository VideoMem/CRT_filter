/*This source code copyrighted by Lazy Foo' Productions (2004-2020)
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

class CRTModel {
    public:
        CRTModel();
        ~CRTModel();

    void setRipple(float r) { ripple = r; }
    void setNoise(float r) { gnoise = r; }

    Uint32 get_pixel32( SDL_Surface *surface, int x, int y);
    void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel);

    void invert( SDL_Surface *surface);
    void desaturate(SDL_Surface *surface, SDL_Surface *dest );
    void noise( SDL_Surface *surface, SDL_Surface *dest  );
    void HRipple( SDL_Surface *surface, SDL_Surface *dest, int warp );
    void VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp );
    void ghost( SDL_Surface *surface, SDL_Surface *dest, int delay );
    void blend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest );

    void update(SDL_Surface *gScreenSurface);

    protected:
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        const Uint32 rmask = 0xff000000;
        const Uint32 gmask = 0x00ff0000;
        const Uint32 bmask = 0x0000ff00;
        const Uint32 amask = 0x000000ff;
    #else
        const Uint32 rmask = 0x000000ff;
        const Uint32 gmask = 0x0000ff00;
        const Uint32 bmask = 0x00ff0000;
        const Uint32 amask = 0xff000000;
    #endif

    SDL_Surface* gFrame = NULL;
    SDL_Surface* gBuffer = NULL;
    SDL_Surface* gBlank = NULL;
    SDL_Surface* gBack = NULL;
    bool createBuffers();
    void init();
    void close();
    bool loadMedia();
    int warp;
    float ripple;
    float gnoise;
};

CRTModel::CRTModel() {
    init();
}

CRTModel::~CRTModel() {
    close();
}

bool CRTModel::createBuffers() {
    gBuffer = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                   rmask, gmask, bmask, amask);
    gBlank = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                   rmask, gmask, bmask, amask);
    gBack  = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                  rmask, gmask, bmask, amask);

    if (gBuffer == NULL || gBlank == NULL || gBack == NULL) {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

Uint32 CRTModel::get_pixel32( SDL_Surface *surface, int x, int y ) {
    //Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;

    //Get the requested pixel
    return pixels[ ( y * surface->w ) + x ];
}

void CRTModel::put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel ) {
    //Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;
    //Set the pixel
    pixels[ ( y * surface->w ) + x ] = pixel;
}

void CRTModel::invert( SDL_Surface *surface ) {
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            put_pixel32(surface, x, y,
                        (0x00FFFFFF - (get_pixel32(surface, x, y) & 0x00FFFFFF)) | 0xFF000000);
        }
}

void CRTModel::desaturate(SDL_Surface *surface, SDL_Surface *dest ) {
    SDL_FillRect(dest, NULL, 0x000000);
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            int pixel = get_pixel32(surface, x, y);
            int R = (pixel & 0xFF0000) >> 16;
            int G = (pixel & 0xFF00) >> 8;
            int B = pixel & 0xFF;
            int luma = round((R + G + B) /3);
            int npx = ((luma << 16) + (luma << 8) + luma) | 0xFF000000;
            put_pixel32(dest, x, y,
                        npx);
        }
}

void CRTModel::noise( SDL_Surface *surface, SDL_Surface *dest  ) {
    SDL_FillRect(dest, NULL, 0x000000);
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            int noise = round((rand() & 0x50) * gnoise);
            int snow  = (noise << 16) + (noise << 8) + noise;
            int pixel = get_pixel32(surface, x, y) & 0x00FFFFFF;
            int biasR = (pixel & 0xFF0000) > (snow & 0xFF0000)? (pixel & 0xFF0000) - (snow & 0xFF0000): (snow & 0xFF0000) - (pixel & 0xFF0000);
            int biasG = (pixel & 0xFF00) > (snow & 0xFF00)? (pixel & 0xFF00) - (snow & 0xFF00): (snow & 0xFF00) - (pixel & 0xFF00);
            int biasB = (pixel & 0xFF) > (snow & 0xFF)? (pixel & 0xFF) - (snow & 0xFF): (snow & 0xFF) - (pixel & 0xFF);

            int pxno  = (biasR + biasG + biasB) | 0xFF000000;
            put_pixel32(dest, x, y,
                        pxno );
        }
}

void CRTModel::HRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    SDL_FillRect(dest, NULL, 0x000000);
    for(int y=0; y< SCREEN_HEIGHT; ++y) {
        int noiseSlip = round(((rand() & 0xFF) / 0x50) * gnoise);
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int pixel = get_pixel32(surface, x, y);
            float screenpos = ((float) y + warp) / SCREEN_HEIGHT;
            int slip = round((sin((M_PI * 4) * screenpos)) * 6 * ripple) + noiseSlip;
            int newx = x + slip;
            if (newx > 0) put_pixel32(dest, newx, y, pixel);
        }
    }
}

void CRTModel::VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    SDL_FillRect(dest, NULL, 0x000000);
    int noiseSlip = round(((rand() & 0xFF) / 0xF0) * gnoise);
    for(int y=0; y< SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int pixel = get_pixel32(surface, x, y);
            float screenpos = ((float) y + warp) / SCREEN_HEIGHT;
            int slip = round((cos((M_PI * 2) * screenpos)) * 10 * ripple) + noiseSlip;
            int newy = y + slip;
            if (newy > 0 && newy < SCREEN_HEIGHT) put_pixel32(dest, x, newy, pixel);
        }
    }
}

void CRTModel::blend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest) {
    SDL_FillRect(dest, NULL, 0x000000);

    for(int y=0; y< SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int persist  = get_pixel32(last, x, y);
            int pxno = (persist & 0x000FFFFFF) | 0xF0000000;
            put_pixel32(last, x, y, pxno);
        }
    }
    SDL_SetSurfaceBlendMode(last, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(last, NULL, dest, NULL);
    SDL_BlitSurface(surface, NULL, dest, NULL);
}

void CRTModel::ghost( SDL_Surface *surface, SDL_Surface *dest, int delay) {
    SDL_FillRect(dest, NULL, 0x000000);

    for(int y=0; y< SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int pixel  = get_pixel32(surface, x, y);
            int pixeld = get_pixel32(surface, x + delay, y);
            put_pixel32(dest, x + delay, y, ((pixel & 0xFFFFFF) + (pixeld & 0xFFFFFF)) | 0xFF000000);
        }
    }
}

bool CRTModel::loadMedia() {
    //Loading success flag
    bool success = true;

    //Load splash image
    gFrame = SDL_LoadBMP( "testCardRGB.bmp" );
    if( gFrame == NULL )
    {
        printf( "Unable to load image %s! SDL Error: %s\n", "standby640.bmp", SDL_GetError() );
        success = false;
    }
    return success;
}

void CRTModel::init() {
    srand(time(0));
    createBuffers();
    loadMedia();
    warp = 0;
    setRipple(1.0);
    setNoise(1.0);
    SDL_BlitSurface(gFrame, NULL, gBack, NULL);
}

void CRTModel::close() {
    //Deallocate surfaces
    SDL_FreeSurface( gBuffer );
    SDL_FreeSurface( gBlank );
    SDL_FreeSurface( gBack );
	SDL_FreeSurface( gFrame );
	gFrame  = NULL;
    gBuffer = NULL;
    gBlank  = NULL;
    gBack   = NULL;
}

void CRTModel::update(SDL_Surface * gScreenSurface)  {
    SDL_FillRect(gScreenSurface, NULL, 0x000000);
    SDL_BlitSurface(gFrame, NULL, gBlank, NULL);
    //if((rand() & 1)) testDesaturate(gBuffer, gBlank); else SDL_BlitSurface(gBuffer, NULL, gBlank, NULL);
    noise(gBlank, gBuffer);
    HRipple(gBuffer, gBlank, warp);
    VRipple(gBlank , gBuffer, warp);
    blend(gBuffer, gBack, gBlank);

    //Apply the image
    SDL_BlitSurface(gBlank, NULL, gScreenSurface, NULL);
    SDL_BlitSurface(gScreenSurface, NULL, gBack, NULL);

    ++warp;
}


