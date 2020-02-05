//Using SDL and standard IO
#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>
#include <string>

#define rand() xorshift128plus()
#define PERSISTENCE_ALPHA 0xA1000000


std::string channels[] = { "standby640.bmp", "testCardRGB.bmp", "marcosvtar.bmp", "alf.bmp" };

static uint64_t s[2];
static inline uint64_t
xoroshiro128plus() {
    uint64_t s0 = s[0];
    uint64_t s1 = s[1];
    uint64_t result = s0 + s1;
    s1 ^= s0;
    s[0] = ((s0 << 55) | (s0 >> 9)) ^ s1 ^ (s1 << 14);
    s[1] = (s1 << 36) | (s1 >> 28);
    return result;
}

static inline uint64_t
xorshift64star() {
    uint64_t x = s[0];
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    s[0] = x;
    return x * UINT64_C(0x2545f4914f6cdd1d);
}

static inline uint64_t
xorshift128plus() {
    uint64_t x = s[0];
    uint64_t y = s[1];
    s[0] = y;
    x ^= x << 23;
    s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return s[1] + y;
}

//Screen dimension constants
const int SCREEN_WIDTH  = 720; //720;
const int SCREEN_HEIGHT = 540; //540;
const int TARGET_WIDTH  = 640; //720;
const int TARGET_HEIGHT = 480; //540;

class CRTModel {
    public:
        CRTModel();
        ~CRTModel();

    void setRipple(float r) { ripple = r; }
    void setNoise(float r) { gnoise = r; }
    void setPerfStats(bool f) { perf_stats = f; }
    void noise(bool f) { addNoise = f; }
    void setVRipple(bool f) { addVRipple = f; }
    void setHRipple(bool f) { addHRipple = f; }
    void setBlend(bool f) { addBlend = f; }
    void setSupply(float v) { supplyV = v; }
    void setBrightness(float v) { brightness = v; }
    void setContrast(float v) { contrast = v; }
    void setFocus(float v) { focus = v; }
    void setColor(float v) { color = v; }

    void blank(SDL_Surface* surface);
    void dot(SDL_Surface* surface);
    void focusNoise(SDL_Surface* surface);
    void shutdown();
    void fade( SDL_Surface* surface );
    float getSupply() { return supplyV; }
    int  randomSlip() { return (rand() & 3) == 0? -3: 1; }
    void resetFrameStats();

    Uint32 get_pixel32( SDL_Surface *surface, int x, int y);
    void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel);

    void invert( SDL_Surface *surface);
    void desaturate(SDL_Surface *surface, SDL_Surface *dest );
    void noise( SDL_Surface *surface, SDL_Surface *dest  );
    float rippleBias(int sync);
    float rippleBias() { return rippleBias(warp); }
    void HRipple( SDL_Surface *surface, SDL_Surface *dest, int warp );
    void VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp );
    void ghost( SDL_Surface *surface, SDL_Surface *dest, int delay );
    void blend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest );
    void strech( SDL_Surface *surface, float scale);
    void channelUp() { int size = sizeof(channels) / sizeof(channels[0]); ++channel; if(channel >= size) channel = 0; loadMedia(); }
    void channelDw() { int size = sizeof(channels) / sizeof(channels[0]);--channel; if(channel < 0 ) channel = size -1; loadMedia(); }

    void logStats();
    void update(SDL_Surface *gScreenSurface);

    protected:
    void blitLine(SDL_Surface* src, SDL_Surface* dst, int line, int dstline);
    void blitLineScaled(SDL_Surface* src, SDL_Surface* dst, int line, float scale);
    inline void comp    (Uint32* pixel, Uint32* R, Uint32* G, Uint32* B);
    inline void toPixel (Uint32* pixel, Uint32* R, Uint32* G, Uint32* B);
    inline void toLuma  (float* luma , Uint32* R, Uint32* G, Uint32* B);
    inline void toChroma(float* Db, float* Dr , Uint32* R, Uint32* G, Uint32* B);
    inline void toRGB   (float* luma , float* Db, float* Dr, Uint32* R, Uint32* G, Uint32* B);
    inline Uint32 toChar (float* comp) { return *comp < 1? round(0xFF **comp): 0xFF; }
    inline float  fromChar(Uint32* c) { return (float) *c / 0xFF; }

    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        static const Uint32 rmask = 0xff000000;
        static const Uint32 gmask = 0x00ff0000;
        static const Uint32 bmask = 0x0000ff00;
        static const Uint32 amask = 0x000000ff;
        static const Uint32 cmask = 0xffffff00;
    #else
        static const Uint32 rmask = 0x000000ff;
        static const Uint32 gmask = 0x0000ff00;
        static const Uint32 bmask = 0x00ff0000;
        static const Uint32 amask = 0xff000000;
        static const Uint32 cmask = 0x00ffffff;
    #endif

    SDL_Surface* gFrame = NULL;
    SDL_Surface* gBuffer = NULL;
    SDL_Surface* gBlank = NULL;
    SDL_Surface* gBack = NULL;

    time_t frameTimer;
    time_t execTimer;
    bool createBuffers();
    void init();
    void close();
    bool loadMedia();
    int warp;
    float ripple;
    float gnoise;
    bool perf_stats;
    bool addNoise;
    bool addVRipple;
    bool addHRipple;
    bool addBlend;
    float supplyV;
    float lastR;
    int channel;
    float brightness;
    float contrast;
    float color;
    float focus;
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
    gBlank  = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                   rmask, gmask, bmask, amask);
    gBack   = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                  rmask, gmask, bmask, amask);
    gFrame  = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                  rmask, gmask, bmask, amask);

    if (gBuffer == NULL || gBlank == NULL || gBack == NULL || gFrame == NULL)  {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

void CRTModel::blank(SDL_Surface *surface) {
    SDL_FillRect(surface, NULL, amask);
}

void CRTModel::focusNoise(SDL_Surface *surface) {
    SDL_Rect tsize;
    SDL_Rect size;
    SDL_Rect osize;
    SDL_GetClipRect(surface, &osize);
    SDL_GetClipRect(gBack, &tsize);
    int spread = 4;
    int psize = 2;
    int slip = (rand() % spread) - (spread /2);
    size.x = rand() & 1? slip + tsize.w /2: -slip + tsize.w /2 ;
    size.y = rand() & 1? slip + tsize.h /2: -slip + tsize.h /2 ;
    size.x -= psize /2;
    size.y -= psize /2;
    size.w = psize;
    size.h = psize;
    blank(gBlank);
    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(gBack , SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(gBack, &tsize, gBlank, &tsize);
    SDL_FillRect(gBlank, &size, cmask | PERSISTENCE_ALPHA /4 );
    SDL_BlitScaled(gBlank, &tsize, surface, &osize);

    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBack , SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_NONE);
    SDL_BlitSurface(gBlank, &tsize, gBack, &tsize);
}

void CRTModel::dot(SDL_Surface *surface) {
    blank(surface);
    SDL_Rect size;
    SDL_Rect osize;
    SDL_GetClipRect(gBack, &osize);
    size.x = osize.w /2;
    size.y = osize.h /2;
    size.w =2;
    size.h =2;
    SDL_FillRect(gBack, &size, cmask);
}

void CRTModel::strech(SDL_Surface *surface, float scale) {
    SDL_Rect src, dst;
    SDL_GetClipRect(gBack, &src);
    SDL_GetClipRect(surface, &dst);
    blank(gBuffer);
    for(int line =0; line < src.h; ++line) {
        blitLineScaled(gBack, gBuffer, line, scale);
    }
    SDL_BlitScaled(gBuffer, &src, surface, &dst);
    SDL_BlitSurface(gBuffer, NULL, gBack, NULL);
}

void CRTModel::shutdown() {
    if(supplyV < 0.5) {
        setNoise(1);
        desaturate(gFrame, gBuffer);
        SDL_BlitSurface(gBuffer, NULL, gFrame, NULL);
    }
    if(supplyV < 0.2) {
        setNoise(0);
        SDL_FillRect(gFrame, NULL, 0xFFFFFFFF);
    }
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
                        (0x00FFFFFF - (get_pixel32(surface, x, y) & 0x00FFFFFF)) | amask );
        }
}

inline void CRTModel::comp(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B) {
    *B = (*pixel & bmask) >> 16;
    *G = (*pixel & gmask) >> 8;
    *R = *pixel & rmask ;
}

inline void CRTModel::toLuma(float *luma, Uint32 *R, Uint32 *G, Uint32 *B) {
    *luma = 0.299 * fromChar(R) + 0.587 * fromChar(G) + 0.114 * fromChar(B);
}


inline void CRTModel::toPixel(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B) {
    *pixel = ((*B << 16) + (*G << 8) + *R) | amask;
}

inline void CRTModel::toChroma(float *Db, float *Dr, Uint32 *R, Uint32 *G, Uint32 *B) {
    *Db = -0.450 * fromChar(R) - 0.883 * fromChar(G) + 1.333 * fromChar(B);
    *Dr = -1.333 * fromChar(R) + 1.116 * fromChar(G) + 0.217 * fromChar(B);
}

inline void CRTModel::toRGB(float *luma, float *Db, float *Dr, Uint32 *R, Uint32 *G, Uint32 *B) {
    float fR = *luma + 0.000092303716148 * *Db - 0.525912630661865 * *Dr;
    float fG = *luma - 0.129132898890509 * *Db + 0.267899328207599 * *Dr;
    float fB = *luma + 0.664679059978955 * *Db - 0.000079202543533 * *Dr;
    *R = toChar(&fR);
    *G = toChar(&fG);
    *B = toChar(&fB);
}

void CRTModel::desaturate(SDL_Surface *surface, SDL_Surface *dest ) {
    SDL_FillRect(dest, NULL, 0x000000);
    Uint32 B, G, R, pixel, npx;
    float luma;
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            pixel = get_pixel32(surface, x, y);
            comp(&pixel, &R, &G, &B);
            toLuma(&luma, &R, &G, &B);
            Uint32 iluma = toChar(&luma);
            toPixel(&npx, &iluma, &iluma, &iluma);
            put_pixel32(dest, x, y,
                        npx);
        }
}

void CRTModel::noise( SDL_Surface *surface, SDL_Surface *dest  ) {
    if(addNoise) {
        SDL_FillRect(dest, NULL, 0x000000);
        Uint32 rnd, snow, pixel, R, G, B, BiasR, BiasG, BiasB, pxno, chrnoise;
        float luma, Db, Dr, noise;
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            int noiseSlip = round(((rand() & 0xFF) / 0x50) * gnoise);
            for (int x = 0; x < SCREEN_WIDTH; ++x) {
                pixel = get_pixel32(surface, x, y) & cmask;
                rnd = rand() & 0xFF;
                noise = fromChar(&rnd) * gnoise;
                chrnoise = toChar(&noise);
                toPixel(&snow, &chrnoise, &chrnoise, &chrnoise);
                comp(&pixel, &R, &G, &B);
                toLuma(&luma, &R, &G, &B);
                toChroma(&Db, &Dr, &R, &G, &B);
                luma = luma ==0? fromChar(&rnd) / 20: luma;
                luma *= (1 - noise) * contrast;
                luma += (1 - brightness) * contrast;
                Dr *= (1 - noise) * color * contrast;
                Db *= (1 - noise) * color * contrast;

                toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);
                toPixel(&pxno, &BiasR, &BiasG, &BiasB);
                put_pixel32(dest, x + noiseSlip, y,
                            pxno);
            }
        }
        if(gnoise > 1) { desaturate(dest, gBlank); SDL_BlitSurface(gBlank, NULL, dest, NULL); }
    } else {
        SDL_BlitSurface(surface, NULL, dest, NULL);
    }
}

float CRTModel::rippleBias(int sync) {
    int line = sync % SCREEN_HEIGHT;
    float screenpos = (float) line / SCREEN_HEIGHT;
    float vt = abs(sin(M_PI *  screenpos));
    float ret = vt > lastR? vt: lastR;
    float correct = 1 - (10e-4 * ripple);
    lastR = ret * correct;
    return ret ;
}

inline void CRTModel::blitLine(SDL_Surface *src, SDL_Surface *dst, int line, int dstline) {
    SDL_Rect srcrect;
    SDL_Rect dstrect;
    SDL_GetClipRect(src, &srcrect);
    SDL_GetClipRect(src, &dstrect);
    srcrect.y = line;
    dstrect.y = dstline;
    srcrect.h = 1;
    dstrect.h = 4;
    SDL_BlitScaled(src, &srcrect, dst, &dstrect);
}

inline void CRTModel::blitLineScaled(SDL_Surface *src, SDL_Surface* dst, int line, float scale) {
    SDL_Rect srcrect;
    SDL_Rect dstrect;
    srcrect.x = 0;
    srcrect.y = line;
    srcrect.w = SCREEN_WIDTH;
    srcrect.h = 1;
    int width = round((float)SCREEN_WIDTH * scale);
    int center = round( (float) (SCREEN_WIDTH - width) / 2);
    dstrect.x = center;
    dstrect.y = line;
    dstrect.w = width;
    dstrect.h = 1;
    SDL_BlitScaled(src, &srcrect, dst, &dstrect);
}

void CRTModel::HRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    if(addHRipple) {
        SDL_FillRect(dest, NULL, 0x000000);
        int sync = warp;
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            float scale = ((0.337 * supplyV ) + 0.663) * rippleBias(sync);
            ++sync;
            blitLineScaled(surface, dest, y, scale);
        }
    } else
        SDL_BlitSurface(surface, NULL, dest, NULL);
}

void CRTModel::VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    if(addVRipple) {
        SDL_FillRect(dest, NULL, 0x000000);
        int noiseSlip = round(((rand() & 0xFF) / 0xF0) * gnoise);
        int sync = warp;
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            float scale = (supplyV * rippleBias(sync));
            int height = round((float) SCREEN_HEIGHT * scale);
            int center = round((float) (SCREEN_HEIGHT - height) / 2);
            ++sync;
            int newy = round(y * scale) + center + noiseSlip;
            if (newy > 0 && newy < SCREEN_HEIGHT) {
                blitLine(surface, dest, y, newy);
            }
        }
    } else
        SDL_BlitSurface(surface, NULL, dest, NULL);
}

void CRTModel::fade(SDL_Surface* surface) {

    SDL_BlitSurface(gBack, NULL, gBuffer, NULL);
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int persist = get_pixel32(gBuffer, x, y);
            int pxno = (persist & 0x000FFFFFF) | PERSISTENCE_ALPHA;
            put_pixel32(gBuffer, x, y, pxno);
        }
    }

    SDL_SetSurfaceBlendMode(gBuffer, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    SDL_Rect clipRect, sRect;
    SDL_GetClipRect(gBuffer, &sRect);
    SDL_GetClipRect(surface, &clipRect);

    SDL_BlitScaled (gBuffer, &sRect, surface, &clipRect);
    SDL_BlitSurface(gBuffer, NULL, gBack, NULL);

    SDL_SetSurfaceBlendMode(gBuffer, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

}


void CRTModel::blend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest) {
    if (addBlend) {
        blank(dest);

        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            for (int x = 0; x < SCREEN_WIDTH; ++x) {
                int persist = get_pixel32(last, x, y);
                int pxno = (persist & cmask) | PERSISTENCE_ALPHA;
                put_pixel32(last, x, y, pxno);
            }
        }

        SDL_SetSurfaceBlendMode(last, SDL_BLENDMODE_BLEND);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(last, NULL, dest, NULL);
        SDL_BlitSurface(surface, NULL, dest, NULL);
        SDL_SetSurfaceBlendMode(last, SDL_BLENDMODE_NONE);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    } else {
        SDL_BlitSurface(surface, NULL, dest, NULL);
    }
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
    SDL_Rect srcrect;
    SDL_Rect dstrect;
    SDL_Surface * gX;
    dstrect.x = 0; dstrect.y = 0;
    dstrect.w = SCREEN_WIDTH; dstrect.h = SCREEN_HEIGHT;

    //Load splash image
    gX = SDL_LoadBMP( channels[channel].c_str() );

    if( gX == NULL )
    {
        printf( "Unable to load image %s! SDL Error: %s\n", "standby640.bmp", SDL_GetError() );
        success = false;
    }

    SDL_GetClipRect(gX, &srcrect);
    SDL_Log("Loaded image size: %dx%d", srcrect.w, srcrect.h);
    SDL_FillRect(gFrame, &dstrect, 0x0);
    SDL_BlitScaled(gX, &srcrect, gFrame, &dstrect);
    SDL_Log("Target image size: %dx%d", dstrect.w, dstrect.h);
    SDL_FreeSurface( gX );

    return success;
}

void CRTModel::resetFrameStats() {
    warp = 0;
    time(&frameTimer);
}

void CRTModel::init() {
    srand(time(0));
    s[0] = time(0);
    s[1] = time(0);
    createBuffers();
    loadMedia();
    setRipple(0.1);
    setNoise(0.2);
    setColor(1);
    setBrightness(1);
    setContrast(1);
    setFocus(1);
    setPerfStats(true);
    noise(true);
    setHRipple(true);
    setVRipple(true);
    setBlend(true);
    setSupply(1.0);
    lastR =0;
    SDL_BlitSurface(gFrame, NULL, gBack, NULL);
    resetFrameStats();
    SDL_SetSurfaceBlendMode(gFrame, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBuffer, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBack, SDL_BLENDMODE_NONE);
    channel = 0;
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

void CRTModel::logStats() {
    if (!perf_stats)
        return;
    time(&execTimer);
    double seconds = difftime(execTimer, frameTimer);
    double framerate = 0;
    if(seconds > 0) { framerate = warp / seconds; }
    SDL_Log("%.02f frames/s: %.f seconds elapsed", framerate, seconds);
}

void CRTModel::update(SDL_Surface * gScreenSurface)  {
    SDL_FillRect(gScreenSurface, NULL, 0x000000);
    SDL_BlitSurface(gFrame, NULL, gBlank, NULL);
    noise(gBlank, gBuffer);
    HRipple(gBuffer, gBlank, warp);
    VRipple(gBlank , gBuffer, warp);
    blend(gBuffer, gBack, gBlank);

    //Apply the image
    SDL_Rect srcsize;
    SDL_Rect dstsize;
    SDL_GetClipRect(gBlank, &srcsize);
    SDL_GetClipRect(gScreenSurface, &dstsize);
    dstsize.w = TARGET_WIDTH;
    dstsize.x = srcsize.w - dstsize.w > 0? (srcsize.w - dstsize.w ) / 2: 0;
    dstsize.h = TARGET_HEIGHT;
    dstsize.y = srcsize.h - dstsize.h > 0? (srcsize.h - dstsize.h ) / 2: 0;

    SDL_BlitScaled(gBlank, &srcsize, gScreenSurface, &dstsize);
    SDL_BlitSurface(gBlank, NULL, gBack, NULL);

    ++warp;
    if((warp % 100) == 0) logStats();
}


