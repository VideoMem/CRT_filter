//Using SDL and standard IO
#include <SDL2/SDL.h>
#include <ctime>
#include <prngs.h>
#include <loaders/MagickLoader.hpp>
#include <filters/Noise.hpp>
#include <filters/BCS.hpp>
#include <filters/Sync.hpp>
#include <filters/Deflection.hpp>
#include <generators/MagickOSD.hpp>
#include <BaseApp.hpp>
#include <chrono>
#include <loaders/ZMQVideoPipe.hpp>
#include <loaders/ZMQLoader.hpp>

using namespace std::chrono;
#define rand() xorshift()

#define PERSISTENCE_ALPHA 0xA1
#define PLANE_SPEED 1000

class CRTApp : public BaseApp {
    public:
        explicit CRTApp(Loader &l);
        ~CRTApp();

    void setRipple(double r) { ripple = r; }
    void setNoise(double r) { gnoise = r; }
    void setPerfStats(bool f) { perf_stats = f; }
    void noise(bool f) { addNoise = f; }
    void setVRipple(bool f) { addVRipple = f; }
    void setHRipple(bool f) { addHRipple = f; }
    void setBlend(bool f) { addBlend = f; }
    void setGhost(bool f) { addGhost =f; }
    void setSupply(double v) { supplyV = v; }
    void setBrightness(double v) { brightness = v; }
    void setContrast(double v) { contrast = v; }
    void setFocus(double v) { focus = v; }
    void setColor(double v) { color = v; }
    void setLoop(bool l) { loop = l; }
    bool getLoop() { return loop; }

    void setPlane() { }
    inline double frameRate(double *seconds);
    inline Uint32 wTime() { double seconds; worldTime = 1000 * warp / frameRate(&seconds); return worldTime; }
    inline void blank(SDL_Surface* surface);
    void shutdown();
    double getSupply() { return supplyV; }
    int  randomSlip() { return (rand() & 3) == 0? -3: 1; }
    void resetFrameStats();

    void invert( SDL_Surface *surface);
    void desaturate(SDL_Surface *surface, SDL_Surface *dest );
    void noise( SDL_Surface *surface, SDL_Surface *dest  );
    void bcs( SDL_Surface *surface, SDL_Surface *dest );
    double rippleBias(int sync);
    //double rippleBias() { return rippleBias(warp); }
    void HRipple( SDL_Surface *surface, SDL_Surface *dest, int warp );
    void VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp );
    void ghost( SDL_Surface *surface, SDL_Surface *dest, int delay, Uint8 power );
    void blend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest );

    void strech( SDL_Surface *surface, double scale);
    void strech( double scale ) { strech( gScreenSurface, scale );}
    void dot(SDL_Surface* surface);
    void dot() { dot(gScreenSurface); }
    void focusNoise(SDL_Surface* surface);
    void focusNoise() { focusNoise(gScreenSurface); }
    void fade( SDL_Surface* surface );
    void fade() { fade( gScreenSurface ); }

    void logStats();
    void update();
    void update(ZMQVideoPipe *pipe, ZMQLoader *zmqloader);
    void update(SDL_Surface *recover_frame, ZMQVideoPipe *zPipe);
    bool loadMedia();


    void upSpeed() { ++ripplesync; }
    void dwSpeed() { if(ripplesync > 1) --ripplesync; }

    void getCode(SDL_Surface *dst);
    void getFrame(SDL_Surface *dst);
    void pushCode(SDL_Surface *src);

protected:
    volatile bool hold;
    void plane(Uint8* delay, Uint8* power);

    void initOSD();
    void showOSD();
    static void blitLine(SDL_Surface* src, SDL_Surface* dst, int line, int dstline);
    static void blitLineScaled(SDL_Surface* src, SDL_Surface* dst, int line, double scale);

    SDL_Surface* gFrame  = nullptr;
    SDL_Surface* gBuffer = nullptr;
    SDL_Surface* gBlank  = nullptr;
    SDL_Surface* gBack   = nullptr;
    SDL_Surface* gAux    = nullptr;
    SDL_Surface* gCode   = nullptr;

    time_t frameTimer;
    time_t execTimer;
    bool createBuffers();
    volatile bool initialized = false;
    void postInit() {
        if(!initialized) {
            createBuffers();
            loadMedia();
            osdFilter.clear();
            initOSD();
            noiseFilter = new NoiseFilter<SDL_Surface>( *gScreenSurface->format );
            deflectionFilter = new DeflectionFilter<SDL_Surface> ( *gScreenSurface->format );
            initialized=true;
        }
    }
    void init();
    void close();

    int warp;
    double ripple;
    double gnoise;
    bool perf_stats;
    bool addNoise;
    bool addVRipple;
    bool addHRipple;
    bool addBlend;
    bool addGhost;
    double supplyV;
    double lastR;
    int channel;
    double brightness;
    double contrast;
    double color;
    double focus;
    int planeidx;
    Uint32 worldTime;
    NoiseFilter<SDL_Surface>* noiseFilter;
    BCSFilter<SDL_Surface> bcsFilter;
    SyncFilter<SDL_Surface> syncFilter;
    bool loop;
   // volatile bool updateLock;
    DeflectionFilter<SDL_Surface>* deflectionFilter;
    MagickOSD osdFilter;


    double ripplesync = 1;


    void updateScreen();



};

CRTApp::CRTApp(Loader &l) : BaseApp(l) {
    init();
}

CRTApp::~CRTApp() {
    close();
}

bool CRTApp::createBuffers() {
    SDL_Surface* aux_gBuffer = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, *gScreenSurface->format );
    gBuffer = SDL_ConvertSurface(aux_gBuffer, gScreenSurface->format, 0);
    gBlank  = SDL_ConvertSurface(aux_gBuffer, gScreenSurface->format, 0);
    gBack   = SDL_ConvertSurface(aux_gBuffer, gScreenSurface->format, 0);
    gFrame  = SDL_ConvertSurface(aux_gBuffer, gScreenSurface->format, 0);
    gAux    = SDL_ConvertSurface(aux_gBuffer, gScreenSurface->format, 0);
    gCode = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    SDL_FreeSurface(aux_gBuffer);

    if (gBuffer == nullptr || gBlank == nullptr || gBack == nullptr || gFrame == nullptr || gAux == nullptr || gCode == nullptr )  {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
        assert(false && "Cannot allocate required memory buffers");
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

void CRTApp::strech(SDL_Surface *surface, double scale) {
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

void CRTApp::desaturate(SDL_Surface *surface, SDL_Surface *dest ) {

    Loader::blank(dest);
    Uint32 B, G, R, pixel, npx;
    double luma;
    for(int x=0; x< Config::SCREEN_WIDTH; ++x)
        for(int y=0; y< Config::SCREEN_HEIGHT; ++y) {
            pixel = Loader::get_pixel32(surface, x, y);
            Loader::comp(&pixel, &R, &G, &B);
            Loader::toLuma(&luma, &R, &G, &B);
            Uint32 iluma = Loader::toChar(&luma);
            Loader::toPixel(&npx, &iluma, &iluma, &iluma);
            Loader::put_pixel32(dest, x, y,
                        npx);
        }
}

void CRTApp::noise( SDL_Surface *surface, SDL_Surface *dest  ) {

    if (addNoise) {
        noiseFilter->run(surface, dest, gnoise);
    } else
        Loader::SurfacePixelsCopy( surface, dest );

}

void CRTApp::bcs( SDL_Surface *surface, SDL_Surface *dest  ) {

    BCSFilterParams params;
    params.contrast = contrast;
    params.saturation = color;
    params.brightness = brightness;
    params.supply_voltage = supplyV;
    params.ripple = ripple;
    params.frame_sync = warp * ripplesync;

    bcsFilter.run(surface, dest , params);

}


double CRTApp::rippleBias(int sync) {
    int line = sync % Config::SCREEN_HEIGHT;
    double screenpos = (double) line / Config::SCREEN_HEIGHT;
    double vt = abs(sin(M_PI *  screenpos));
    double ret = vt > lastR? vt: lastR;
    double correct = 1 - (10e-4 * ripple);
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
    SDL_BlitSurface(src, &srcrect, dst, &dstrect);
}

inline void CRTApp::blitLineScaled(SDL_Surface *src, SDL_Surface* dst, int line, double scale) {
    SDL_Rect srcrect;
    SDL_Rect dstrect;
    srcrect.x = 0;
    srcrect.y = line;
    srcrect.w = Config::SCREEN_WIDTH;
    srcrect.h = 1;
    int width = round((double)Config::SCREEN_WIDTH * scale);
    int center = round( (double) (Config::SCREEN_WIDTH - width) / 2);
    dstrect.x = center;
    dstrect.y = line;
    dstrect.w = width;
    dstrect.h = 1;
    SDL_BlitScaled(src, &srcrect, dst, &dstrect);
}

void CRTApp::HRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    DeflectionFilterParams params;
    params.Hcomp = addHRipple;
    params.Vcomp = addVRipple;
    params.ripple = ripple;
    params.vsupply = supplyV;
    params.warp = warp * ripplesync;
    deflectionFilter->run( surface, dest, params );
/*        Loader::blank(dest);
        int sync = warp;
        for(int y=0; y< Config::SCREEN_HEIGHT; ++y) {
            double scale = ((0.337 * supplyV ) + 0.663) * rippleBias(sync);
            ++sync;
            blitLineScaled(surface, dest, y, scale);
        }*/

}

void CRTApp::VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    if(addVRipple) {
        Loader::blank(dest);
        int noiseSlip = round(((rand() & 0xFF) / 0xF0) * 0.3 );
        int sync = warp;
        int newy = 0 , last_blitY = 0;
        for(int y=0; y< Config::SCREEN_HEIGHT; ++y) {
            double scale = (supplyV * rippleBias(sync)); ++sync;
            int height = round((double) Config::SCREEN_HEIGHT * scale);
            int center = round((double) (Config::SCREEN_HEIGHT - height) / 2);
            last_blitY = newy;
            newy = round(y * scale) + center + noiseSlip;
            if (newy > 0 && newy < Config::SCREEN_HEIGHT) {
                Loader::blitLine( surface, dest, y, newy );
            }
            if(newy > last_blitY && last_blitY != 0)
                for(int i = last_blitY; i < newy; ++i)
                    Loader::blitLine( surface, dest, y, i );
            else
                for(int i = newy; i < last_blitY; ++i)
                    Loader::blitLine( surface, dest, y, i );
        }
    } else
        Loader::SurfacePixelsCopy( surface, dest );
}

void CRTApp::fade(SDL_Surface* surface) {

    SDL_BlitSurface(gBack, NULL, gBuffer, NULL);
/*
    for (int y = 0; y < Config::SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < Config::SCREEN_WIDTH; ++x) {
            int persist = Loader::get_pixel32(gBuffer, x, y);
            int pxno = (persist & 0x000FFFFFF) | PERSISTENCE_ALPHA;
            Loader::put_pixel32(gBuffer, x, y, pxno);
        }
    }
*/
    SDL_SetSurfaceAlphaMod(gBuffer, PERSISTENCE_ALPHA / 2);
    SDL_SetSurfaceBlendMode(gBuffer, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    SDL_Rect clipRect, sRect;
    SDL_GetClipRect(gBuffer, &sRect);
    SDL_GetClipRect(surface, &clipRect);

    SDL_BlitScaled (gBuffer, &sRect, surface, &clipRect);
    SDL_BlitSurface(gBuffer, NULL, gBack, NULL);

    SDL_SetSurfaceAlphaMod(gBuffer, 0xFF);
    SDL_SetSurfaceBlendMode(gBuffer, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    ++warp;
}

void CRTApp::blend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest) {
    if (addBlend) {
        Loader::blank(dest);
        SDL_SetSurfaceAlphaMod(last, PERSISTENCE_ALPHA / 2);
        SDL_SetSurfaceBlendMode(last, SDL_BLENDMODE_BLEND);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(last, nullptr, dest, nullptr);
        SDL_BlitSurface(surface, nullptr, dest, nullptr);
        SDL_SetSurfaceAlphaMod(last, 0xFF);
        //SDL_SetSurfaceBlendMode(last, SDL_BLENDMODE_NONE);
        //SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    } else {
        SDL_BlitSurface(surface, nullptr, dest, nullptr);
    }
}

void CRTApp::plane(Uint8 *delay, Uint8 *power) {
    int mag = PLANE_SPEED;
    int position = (warp % mag) - (mag /2);
    double height2 = pow(mag/2,2);
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
    return loader->GetSurface( gFrame, *gScreenSurface->format );
}

void CRTApp::resetFrameStats() {
    warp = 0;
    time(&frameTimer);
}

void CRTApp::init() {
    hold=false;
    //updateLock = false;
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
    setBlend(false);
    setSupply(1.0);
    setGhost(false);
    SDL_BlitSurface(gFrame, NULL, gBack, NULL);
    resetFrameStats();
    SDL_SetSurfaceBlendMode(gFrame , SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(gBlank , SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(gBuffer, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(gBack  , SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(gAux   , SDL_BLENDMODE_BLEND);

}

void CRTApp::close() {
    //Deallocate surfaces
    delete(noiseFilter);
    delete(deflectionFilter);
    SDL_FreeSurface( gBuffer );
    SDL_FreeSurface( gBlank );
    SDL_FreeSurface( gBack );
	SDL_FreeSurface( gFrame );
    SDL_FreeSurface( gAux );

	gFrame  = nullptr;
    gBuffer = nullptr;
    gBlank  = nullptr;
    gBack   = nullptr;
    gAux    = nullptr;
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

void CRTApp::getCode(SDL_Surface *dst) {
    while(!initialized);
    loader->GetSurface(gFrame);
    Magickable::blitScaled( dst, gFrame );
}

void CRTApp::pushCode(SDL_Surface *src) {
    Magickable::blitScaled( gFrame, src );
    updateScreen();
}

void CRTApp::getFrame(SDL_Surface *dst) {
    while(!initialized);
    Magickable::blitScaled( dst, gBack );
}

void CRTApp::updateScreen() {
    auto s0 = high_resolution_clock::now();
    Loader::SurfacePixelsCopy(gFrame, gBack);
    publish(gFrame);
    return;
    // getCode();
    //Uint8 power = 0, delay = 0;
    //if(addGhost) plane(&delay, &power);
    if(gnoise > 0.5) { color = 0; }
    //ghost(gAux, gBlank, delay, power);

    if(!loop) syncFilter.run(gFrame, gAux, gnoise);
    else      syncFilter.run(gBack , gAux, gnoise);

    noise(gAux, gBlank);

    auto start = high_resolution_clock::now();
    bcs( gBlank, gBuffer );
    auto stop = high_resolution_clock::now();

    HRipple(gBuffer, gBlank, warp);

    //VRipple(gBlank , gBuffer, warp);
    //blend(gAux, gBack, gBlank);
    //Apply the image
    publish(gBlank);


    Loader::SurfacePixelsCopy(gBlank, gBack);
    ++warp;

    auto s1 = high_resolution_clock::now();
    if((warp % 100) == 0) {
        auto d0 = duration_cast<microseconds>(s1 - s0);
        auto duration = duration_cast<microseconds>(stop - start);
        SDL_Log("BCS loop %ld µs, total loop %ld µs", duration.count(), d0.count() );
        logStats();
    }
}

void CRTApp::update()  {

    if(!initialized) postInit();
    //while(updateLock);
    updateScreen();

}

void CRTApp::initOSD() {
    Loader::SurfacePixelsCopy(gFrame, gBuffer);
    //SDL_BlitSurface( gFrame, nullptr , gBuffer, nullptr );
    SDL_Rect position = { 0, Config::SCREEN_HEIGHT / 2, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT };
    osdFilter.centerXtxt( &position , "> INITIALIZING <");
    SDL_SetSurfaceAlphaMod( gAux, 0xFF );
    osdFilter.getSurface(gAux);
    SDL_Surface* osdOverlay = SDL_ConvertSurface(gAux, gScreenSurface->format, 0 );
    SDL_SetColorKey( osdOverlay, SDL_TRUE,
            SDL_MapRGB( osdOverlay->format, 0, 0, 0 ) );
    SDL_SetSurfaceBlendMode( osdOverlay, SDL_BLENDMODE_BLEND );

    SDL_BlitSurface ( osdOverlay, nullptr, gBuffer, nullptr );
    SDL_BlitScaled( gBuffer, nullptr, gScreenSurface, nullptr);
    SDL_FreeSurface(osdOverlay);
    redraw();
}

void CRTApp::showOSD() {

}



