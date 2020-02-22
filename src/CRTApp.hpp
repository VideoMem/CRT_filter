//Using SDL and standard IO
#include <SDL2/SDL.h>
#include <ctime>
#include <prngs.h>
#include <loaders/MagickLoader.hpp>
#include <BaseApp.hpp>

#define rand() xorshift()
#define PERSISTENCE_ALPHA 0xA1000000
#define PLANE_SPEED 1000

class CRTApp : public BaseApp {
    public:
        explicit CRTApp(Loader& l);
        ~CRTApp();

    void setRipple(float r) { ripple = r; }
    void setNoise(float r) { gnoise = r; }
    void setPerfStats(bool f) { perf_stats = f; }
    void noise(bool f) { addNoise = f; }
    void setVRipple(bool f) { addVRipple = f; }
    void setHRipple(bool f) { addHRipple = f; }
    void setBlend(bool f) { addBlend = f; }
    void setGhost(bool f) { addGhost =f; }
    void setSupply(float v) { supplyV = v; }
    void setBrightness(float v) { brightness = v; }
    void setContrast(float v) { contrast = v; }
    void setFocus(float v) { focus = v; }
    void setColor(float v) { color = v; }
    void setPlane() { }
    inline double frameRate(double *seconds);
    inline Uint32 wTime() { double seconds; worldTime = 1000 * warp / frameRate(&seconds); return worldTime; }
    inline void blank(SDL_Surface* surface);
    void shutdown();
    float getSupply() { return supplyV; }
    int  randomSlip() { return (rand() & 3) == 0? -3: 1; }
    void resetFrameStats();

    void invert( SDL_Surface *surface);
    void desaturate(SDL_Surface *surface, SDL_Surface *dest );
    void noise( SDL_Surface *surface, SDL_Surface *dest  );
    float rippleBias(int sync);
    float rippleBias() { return rippleBias(warp); }
    void HRipple( SDL_Surface *surface, SDL_Surface *dest, int warp );
    void VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp );
    void ghost( SDL_Surface *surface, SDL_Surface *dest, int delay, Uint8 power );
    void blend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest );

    void strech( SDL_Surface *surface, float scale);
    void strech( float scale ) { strech( gScreenSurface, scale );}
    void dot(SDL_Surface* surface);
    void dot() { dot(gScreenSurface); }
    void focusNoise(SDL_Surface* surface);
    void focusNoise() { focusNoise(gScreenSurface); }
    void fade( SDL_Surface* surface );
    void fade() { fade( gScreenSurface ); }

    void logStats();
    void update();
    bool loadMedia();

    protected:
    void plane(Uint8* delay, Uint8* power);

    static void blitLine(SDL_Surface* src, SDL_Surface* dst, int line, int dstline);
    void blitLineScaled(SDL_Surface* src, SDL_Surface* dst, int line, float scale);
    inline void toLuma  (float* luma , Uint32* R, Uint32* G, Uint32* B);
    inline void toChroma(float* Db, float* Dr , Uint32* R, Uint32* G, Uint32* B);
    inline void toRGB   (float* luma , float* Db, float* Dr, Uint32* R, Uint32* G, Uint32* B);
    inline Uint32 toChar (float* comp) { return *comp < 1? round(0xFF **comp): 0xFF; }
    inline float  fromChar(Uint32* c) { return (float) *c / 0xFF; }

    SDL_Surface* gFrame  = nullptr;
    SDL_Surface* gBuffer = nullptr;
    SDL_Surface* gBlank  = nullptr;
    SDL_Surface* gBack   = nullptr;
    SDL_Surface* gAux    = nullptr;

    time_t frameTimer;
    time_t execTimer;
    bool createBuffers();
    bool initialized = false;
    void postInit() { if(!initialized) { createBuffers(); loadMedia(); initialized=true; } }
    void init();
    void close();

    int warp;
    float ripple;
    float gnoise;
    bool perf_stats;
    bool addNoise;
    bool addVRipple;
    bool addHRipple;
    bool addBlend;
    bool addGhost;
    float supplyV;
    float lastR;
    int channel;
    float brightness;
    float contrast;
    float color;
    float focus;
    int planeidx;
    Uint32 worldTime;
};

CRTApp::CRTApp(Loader& l):BaseApp(l) {
    init();
}

CRTApp::~CRTApp() {
    close();
}

bool CRTApp::createBuffers() {
    gBuffer = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    gBlank  = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    gBack   = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    gFrame  = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    gAux    = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );

    if (gBuffer == nullptr || gBlank == nullptr || gBack == nullptr || gFrame == nullptr || gAux == nullptr)  {
        SDL_Log("SDL_CreateRGBSurface() failed: %prngState", SDL_GetError());
        return false;
    }
    return true;
}



void CRTApp::focusNoise(SDL_Surface *surface) {
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
    Loader::blank(gBlank);
    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(gBack , SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(gBack, &tsize, gBlank, &tsize);
    SDL_FillRect(gBlank, &size, Loader::cmask | PERSISTENCE_ALPHA /4 );
    SDL_BlitScaled(gBlank, &tsize, surface, &osize);

    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBack , SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBlank, SDL_BLENDMODE_NONE);
    SDL_BlitSurface(gBlank, &tsize, gBack, &tsize);
}

void CRTApp::dot(SDL_Surface *surface) {
    Loader::blank(surface);
    SDL_Rect size;
    SDL_Rect osize;
    SDL_GetClipRect(gBack, &osize);
    size.x = osize.w /2;
    size.y = osize.h /2;
    size.w =2;
    size.h =2;
    SDL_FillRect(gBack, &size, Loader::cmask);
}

void CRTApp::strech(SDL_Surface *surface, float scale) {
    SDL_Rect src, dst;
    SDL_GetClipRect(gBack, &src);
    SDL_GetClipRect(surface, &dst);
    Loader::blank(gBuffer);
    for(int line =0; line < src.h; ++line) {
        blitLineScaled(gBack, gBuffer, line, scale);
    }
    SDL_BlitScaled(gBuffer, &src, surface, &dst);
    SDL_BlitSurface(gBuffer, NULL, gBack, NULL);
}

void CRTApp::shutdown() {
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

void CRTApp::invert( SDL_Surface *surface ) {
    for(int x=0; x < Config::SCREEN_WIDTH; ++x)
        for(int y=0; y < Config::SCREEN_HEIGHT; ++y) {
            Loader::put_pixel32(surface, x, y,
                        (0x00FFFFFF - (Loader::get_pixel32(surface, x, y) & 0x00FFFFFF)) | Loader::amask );
        }
}

inline void CRTApp::toLuma(float *luma, Uint32 *R, Uint32 *G, Uint32 *B) {
    *luma = 0.299 * fromChar(R) + 0.587 * fromChar(G) + 0.114 * fromChar(B);
}

inline void CRTApp::toChroma(float *Db, float *Dr, Uint32 *R, Uint32 *G, Uint32 *B) {
    *Db = -0.450 * fromChar(R) - 0.883 * fromChar(G) + 1.333 * fromChar(B);
    *Dr = -1.333 * fromChar(R) + 1.116 * fromChar(G) + 0.217 * fromChar(B);
}

inline void CRTApp::toRGB(float *luma, float *Db, float *Dr, Uint32 *R, Uint32 *G, Uint32 *B) {
    float fR = *luma + 0.000092303716148 * *Db - 0.525912630661865 * *Dr;
    float fG = *luma - 0.129132898890509 * *Db + 0.267899328207599 * *Dr;
    float fB = *luma + 0.664679059978955 * *Db - 0.000079202543533 * *Dr;
    *R = toChar(&fR);
    *G = toChar(&fG);
    *B = toChar(&fB);
}

void CRTApp::desaturate(SDL_Surface *surface, SDL_Surface *dest ) {
    SDL_FillRect(dest, NULL, 0x000000);
    Uint32 B, G, R, pixel, npx;
    float luma;
    for(int x=0; x< Config::SCREEN_WIDTH; ++x)
        for(int y=0; y< Config::SCREEN_HEIGHT; ++y) {
            pixel = Loader::get_pixel32(surface, x, y);
            Loader::comp(&pixel, &R, &G, &B);
            toLuma(&luma, &R, &G, &B);
            Uint32 iluma = toChar(&luma);
            Loader::toPixel(&npx, &iluma, &iluma, &iluma);
            Loader::put_pixel32(dest, x, y,
                        npx);
        }
}

void CRTApp::noise( SDL_Surface *surface, SDL_Surface *dest  ) {
    if(addNoise) {
        SDL_FillRect(dest, NULL, 0x000000);
        Uint32 rnd, snow, pixel, R, G, B, BiasR, BiasG, BiasB, pxno, chrnoise;
        float luma, Db, Dr, noise;
        for (int y = 0; y < Config::SCREEN_HEIGHT; ++y) {
            int noiseSlip = round(((rand() & 0xFF) / 0x50) * gnoise);
            for (int x = 0; x < Config::SCREEN_WIDTH; ++x) {
                pixel = Loader::get_pixel32(surface, x, y) & Loader::cmask;
                rnd = rand() & 0xFF;
                noise = fromChar(&rnd) * gnoise;
                chrnoise = toChar(&noise);
                Loader::toPixel(&snow, &chrnoise, &chrnoise, &chrnoise);
                Loader::comp(&pixel, &R, &G, &B);
                toLuma(&luma, &R, &G, &B);
                toChroma(&Db, &Dr, &R, &G, &B);
                luma = luma ==0? fromChar(&rnd) / 20: luma;
                luma *= (1 - noise) * contrast;
                luma += (1 - brightness) * contrast;
                Dr *= (1 - noise) * color * contrast;
                Db *= (1 - noise) * color * contrast;

                toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);
                Loader::toPixel(&pxno, &BiasR, &BiasG, &BiasB);
                Loader::put_pixel32(dest, x + noiseSlip, y,
                            pxno);
            }
        }
        if(gnoise > 1) { desaturate(dest, gBlank); SDL_BlitSurface(gBlank, NULL, dest, NULL); }
    } else {
        SDL_BlitSurface(surface, NULL, dest, NULL);
    }
}

float CRTApp::rippleBias(int sync) {
    int line = sync % Config::SCREEN_HEIGHT;
    float screenpos = (float) line / Config::SCREEN_HEIGHT;
    float vt = abs(sin(M_PI *  screenpos));
    float ret = vt > lastR? vt: lastR;
    float correct = 1 - (10e-4 * ripple);
    lastR = ret * correct;
    return ret ;
}

inline void CRTApp::blitLine(SDL_Surface *src, SDL_Surface *dst, int line, int dstline) {
    SDL_Rect srcrect;
    SDL_Rect dstrect;
    SDL_GetClipRect(src, &srcrect);
    SDL_GetClipRect(src, &dstrect);
    srcrect.y = line;
    dstrect.y = dstline;
    srcrect.h = 1;
    dstrect.h = 1;
    SDL_BlitScaled(src, &srcrect, dst, &dstrect);
}

inline void CRTApp::blitLineScaled(SDL_Surface *src, SDL_Surface* dst, int line, float scale) {
    SDL_Rect srcrect;
    SDL_Rect dstrect;
    srcrect.x = 0;
    srcrect.y = line;
    srcrect.w = Config::SCREEN_WIDTH;
    srcrect.h = 1;
    int width = round((float)Config::SCREEN_WIDTH * scale);
    int center = round( (float) (Config::SCREEN_WIDTH - width) / 2);
    dstrect.x = center;
    dstrect.y = line;
    dstrect.w = width;
    dstrect.h = 1;
    SDL_BlitScaled(src, &srcrect, dst, &dstrect);
}

void CRTApp::HRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    if(addHRipple) {
        Loader::blank(dest);
        int sync = warp;
        for(int y=0; y< Config::SCREEN_HEIGHT; ++y) {
            float scale = ((0.337 * supplyV ) + 0.663) * rippleBias(sync);
            ++sync;
            blitLineScaled(surface, dest, y, scale);
        }
    } else
        SDL_BlitSurface(surface, NULL, dest, NULL);
}

void CRTApp::VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    if(addVRipple) {
        Loader::blank(dest);
        int noiseSlip = round(((rand() & 0xFF) / 0xF0) * 0.3);
        int sync = warp;
        int newy = 0 , last_blitY = 0;
        for(int y=0; y< Config::SCREEN_HEIGHT; ++y) {
            float scale = (supplyV * rippleBias(sync)); ++sync;
            int height = round((float) Config::SCREEN_HEIGHT * scale);
            int center = round((float) (Config::SCREEN_HEIGHT - height) / 2);
            last_blitY = newy;
            newy = round(y * scale) + center + noiseSlip;
            if (newy > 0 && newy < Config::SCREEN_HEIGHT) {
                blitLine(surface, dest, y, newy);
            }
            if(newy > last_blitY && last_blitY != 0)
                for(int i = last_blitY; i < newy; ++i)
                    blitLine(surface, dest, y, i);
            else
                for(int i = newy; i < last_blitY; ++i)
                    blitLine(surface, dest, y, i);
        }
    } else
        SDL_BlitSurface(surface, NULL, dest, NULL);
}

void CRTApp::fade(SDL_Surface* surface) {

    SDL_BlitSurface(gBack, NULL, gBuffer, NULL);
    for (int y = 0; y < Config::SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < Config::SCREEN_WIDTH; ++x) {
            int persist = Loader::get_pixel32(gBuffer, x, y);
            int pxno = (persist & 0x000FFFFFF) | PERSISTENCE_ALPHA;
            Loader::put_pixel32(gBuffer, x, y, pxno);
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
    ++warp;
}

void CRTApp::blend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest) {
    if (addBlend) {
        Loader::blank(dest);

        for (int y = 0; y < Config::SCREEN_HEIGHT; ++y) {
            for (int x = 0; x < Config::SCREEN_WIDTH; ++x) {
                int persist = Loader::get_pixel32(last, x, y);
                int pxno = (persist & Loader::cmask) | PERSISTENCE_ALPHA;
                Loader::put_pixel32(last, x, y, pxno);
            }
        }

        SDL_SetSurfaceBlendMode(last, SDL_BLENDMODE_BLEND);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(last, nullptr, dest, nullptr);
        SDL_BlitSurface(surface, nullptr, dest, nullptr);
        SDL_SetSurfaceBlendMode(last, SDL_BLENDMODE_NONE);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    } else {
        SDL_BlitSurface(surface, nullptr, dest, nullptr);
    }
}

void CRTApp::plane(Uint8 *delay, Uint8 *power) {
    int mag = PLANE_SPEED;
    int position = (warp % mag) - (mag /2);
    float height2 = pow(mag/2,2);
    auto distance = sqrt(height2 + pow(position, 2));
    auto fpow = 1/pow(distance/ sqrt(height2),2);
    *power = fpow * 64;
    *delay = round(50.0 * distance / sqrt(height2)) - 50;
}

void CRTApp::ghost( SDL_Surface *surface, SDL_Surface *dest, int delay, Uint8 power) {
    if(addGhost) {
        Loader::blank(dest);
        Loader::blank(gAux);
        SDL_Rect src, dst;
        SDL_GetClipRect(surface, &src);
        SDL_GetClipRect(surface, &dst);
        dst.x = delay;
        SDL_BlitSurface(surface, &src, gAux, &dst);
        SDL_SetSurfaceAlphaMod(surface, 0xFF);
        SDL_BlitSurface(surface, &src, dest, &src);

        SDL_SetSurfaceAlphaMod(gAux, power);
        SDL_SetSurfaceBlendMode(gAux, SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(gAux, &src, dest, &src);
        SDL_SetSurfaceBlendMode(gAux, SDL_BLENDMODE_NONE);
    } else {
        SDL_BlitSurface(surface, NULL, dest, NULL);
    }
}

bool CRTApp::loadMedia() {
    return loader->GetSurface( gFrame );
}

void CRTApp::resetFrameStats() {
    warp = 0;
    time(&frameTimer);
}

void CRTApp::init() {
    srand(time(0));
    channel = 0;
    worldTime = 0;
    lastR =0;
    prngState[0] = time(0);
    prngState[1] = time(0);
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
    setGhost(false);
    SDL_BlitSurface(gFrame, NULL, gBack, NULL);
    resetFrameStats();
    SDL_SetSurfaceBlendMode(gFrame , SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBlank , SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBuffer, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gBack  , SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(gAux   , SDL_BLENDMODE_NONE);

}

void CRTApp::close() {
    //Deallocate surfaces
    SDL_FreeSurface( gBuffer );
    SDL_FreeSurface( gBlank );
    SDL_FreeSurface( gBack );
	SDL_FreeSurface( gFrame );
    SDL_FreeSurface( gAux );

	gFrame  = NULL;
    gBuffer = NULL;
    gBlank  = NULL;
    gBack   = NULL;
    gAux    = NULL;
}

inline double CRTApp::frameRate(double *seconds) {
    time(&execTimer);
    *seconds = difftime(execTimer, frameTimer);
    double framerate = 0;
    if(*seconds > 0) { framerate = warp / *seconds; }
    return framerate;
}

void CRTApp::logStats() {
    double seconds = 0;
    if (!perf_stats)
        return;
    SDL_Log("%.02f frames/s: %.f seconds elapsed, %d World milli Seconds", frameRate(&seconds), seconds, wTime());
}

void CRTApp::update()  {
    if(!initialized) postInit();
    Uint8 power = 0, delay = 0;
    if(addGhost) plane(&delay, &power);
    ghost(gFrame, gBlank, delay, power);
    noise(gBlank, gBuffer);
    HRipple(gBuffer, gBlank, warp);
    VRipple(gBlank , gBuffer, warp);
    blend(gBuffer, gBack, gBlank);


    //Apply the image
    SDL_Rect srcsize;
    SDL_Rect dstsize;
    SDL_GetClipRect(gBlank, &srcsize);
    SDL_GetClipRect(gScreenSurface, &dstsize);
    dstsize.w = Config::TARGET_WIDTH;
    dstsize.x = srcsize.w - dstsize.w > 0? (srcsize.w - dstsize.w ) / 2: 0;
    dstsize.h = Config::TARGET_HEIGHT;
    dstsize.y = srcsize.h - dstsize.h > 0? (srcsize.h - dstsize.h ) / 2: 0;

    SDL_BlitScaled(gBlank, &srcsize, gScreenSurface, &dstsize);
    SDL_BlitSurface(gBlank, nullptr, gBack, nullptr);
    redraw();
    ++warp;
    if((warp % 100) == 0) logStats();
}


