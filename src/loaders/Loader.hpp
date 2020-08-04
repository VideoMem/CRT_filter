//
// Created by sebastian on 20/2/20.
//

#ifndef SDL_CRT_FILTER_LOADER_HPP
#define SDL_CRT_FILTER_LOADER_HPP
//#include <ResourceRoller.hpp>
#include <SDL2/SDL.h>
#include <fstream>
#include <picosha2.h>
#include <transcoders/Surfaceable.hpp>
#include <transcoders/Waveable.hpp>
#include <transcoders/Pixelable.hpp>

#define MAX_WHITE_LEVEL 255

class Loader: public ResourceRoller {
public:
    virtual void GetRAWSurface(SDL_Surface *pSurface);
    virtual bool GetSurface(SDL_Surface*) { return false; };
    virtual bool GetSurface(SDL_Surface*, SDL_PixelFormat&) { return false; };
    virtual void pushCache(SDL_Surface* frame );
    virtual bool frameEvent() { return false; };
    static SDL_Surface* AllocateSurface(int w, int h);
    static SDL_Surface* AllocateSurface(int w, int h, SDL_PixelFormat &format);
    static SDL_Rect BiggestSurfaceClipRect(SDL_Surface* src, SDL_Surface* dst);
    static SDL_Rect SmallerBlitArea( SDL_Surface* src, SDL_Surface* dst);
    inline static void SurfacePixelsCopy( SDL_Surface* src, SDL_Surface* dst );
    static bool CompareSurface(SDL_Surface* src, SDL_Surface* dst);

    static inline Uint32 get_pixel32( SDL_Surface *surface, int x, int y);
    static inline void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel);

    static inline void blank(SDL_Surface *surface) {
        SDL_FillRect(surface, nullptr, amask);
    }

    static bool testFile( std::string path ) {
        using namespace  std;
        ofstream myfile;
        myfile.open ( path, ios::in | ios::binary );
        if (myfile.is_open()) {
            myfile.close();
            return true;
        } else
            return false;
    }

    //TODO: bit endianess
    inline static void comp(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B, Uint32 *A) {
        *A = 0xFF - ((*pixel & amask) >> 24);
        comp( pixel, R, G, B );
    }
    inline static void comp(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B) {
        *B = (*pixel & bmask) >> 16;
        *G = (*pixel & gmask) >> 8;
        *R = *pixel & rmask ;
    }

    //TODO: bit endianess
    inline static void toPixel(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B) {
        *pixel = ((*B << 16) + (*G << 8) + *R) | amask;
    }

    inline static void toPixel(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B, Uint32 *A) {
        *pixel = 0;
        *pixel = ((*A << 24) + (*B << 16) + (*G << 8) + *R);
        //if (*A == 0) * pixel = 0x01F000F0;
    }

    inline static double  fromChar(int32_t* c) { return (double) *c / 0xFF; }
    inline static double  fromChar(Uint32* c)  { return (double) *c / 0xFF; }
    inline static Uint32 toChar (double* comp) { return *comp < 1? 0xFF * *comp: 0xFF; }

    inline static void toLuma(double *luma, Uint32 *R, Uint32 *G, Uint32 *B) {
        *luma = Pixelable::luma(R, G, B);
    }

    inline static void toChroma(double *Db, double *Dr, Uint32 *R, Uint32 *G, Uint32 *B) {
        return Pixelable::chroma(Db, Dr, R, G, B);
    }

    inline static void toRGB(const double *luma, Uint32 *R, Uint32 *G, Uint32 *B) {
        const double DbDr = 0.0;
        toRGB(luma, &DbDr, &DbDr, R, G, B);
    }

    inline static void toRGB(const double *luma, const double *Db, const double *Dr, Uint32 *R, Uint32 *G, Uint32 *B) {
        return Pixelable::RGB(luma, Db, Dr, R, G, B);
    }

    inline static void blitLineScaled(SDL_Surface *src, SDL_Surface* dst, int& line, double& scale);

    inline static void blitLine(SDL_Surface *src, SDL_Surface *dst, int& line, int& dstline) {
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

    size_t surface_to_wave(SDL_Surface *surface, uint8_t *wav);
    void wave_to_surface(uint8_t *wav, SDL_Surface *surface);
    static std::string sha256Log(uint8_t data[], size_t len);

    void wave_to_surface(uint8_t *wav, SDL_Surface *surface, int flag);


};

//deprecated
size_t Loader::surface_to_wave( SDL_Surface *surface, uint8_t *wav ) {
    WaveLuma::decode( wav, surface );
    return ( surface->w * surface->h );
}

//deprecated
void Loader::wave_to_surface(uint8_t *wav, SDL_Surface* surface, int flag ) {
    size_t size = Config::NKERNEL_WIDTH * Config::NKERNEL_HEIGHT;
    SDL_Log("DEBUG: wave_to_surface, input sha256: %s", sha256Log(wav, size).c_str());
    wave_to_surface(wav, surface);
}

//deprecated
void Loader::wave_to_surface(uint8_t *wav, SDL_Surface* surface ) {
    return WaveLuma::encode( surface, wav );
}

SDL_Surface* Loader::AllocateSurface(int w, int h, SDL_PixelFormat& format) {
    return Surfaceable::AllocateSurface( w, h, format );
    SDL_Surface* ns = SDL_CreateRGBSurface(0, w, h, 32,
                                rmask, gmask, bmask, amask);
    SDL_Surface* optimized = SDL_ConvertSurface(ns, &format, 0);
    SDL_FreeSurface(ns);
    return optimized;
}

SDL_Surface *Loader::AllocateSurface(int w, int h) {
    return Surfaceable::AllocateSurface( w, h );
    return SDL_CreateRGBSurface(0, w, h, 32,
            rmask, gmask, bmask, amask);
}



bool Loader::CompareSurface(SDL_Surface *src, SDL_Surface *dst) {

    struct pxfmt_t {
        Uint8 r;
        Uint8 g;
        Uint8 b;
        Uint8 a;
    };

    union pxu_t {
        pxfmt_t comp;
        Uint32  px;
    } pxa, pxb;

    if (src->format->format == dst->format->format &&
        src->w == dst->w && src->h == dst->h) {
        bool error = false;
        SDL_LockSurface(src);
        SDL_LockSurface(dst);
        for(int x=0; x < src->w && !error; ++x)
            for(int y=0; y < src->h; ++y) {
                pxa.px = get_pixel32(src, x, y);
                pxb.px = get_pixel32(dst, x, y);;
                if(get_pixel32(src, x, y) != get_pixel32(dst, x, y)) {
                    if ( pxa.comp.r != pxb.comp.r || pxa.comp.g != pxb.comp.g || pxa.comp.b != pxb.comp.b ) {
                        error = true;
                        /*SDL_Log( "CompareSurface:(%d,%d) -> Expected RGBA (%u, %u, %u, %u), got (%u, %u, %u, %u)",
                                 x, y,
                                 pxa.comp.r,
                                 pxa.comp.g,
                                 pxa.comp.b,
                                 pxa.comp.a,
                                 pxb.comp.r,
                                 pxb.comp.g,
                                 pxb.comp.b,
                                 pxb.comp.a);*/
                        break;
                    }
                }
            }
        SDL_UnlockSurface(src);
        SDL_UnlockSurface(dst);
        return !error;
    }
    return false;
}

//deprecated
Uint32 Loader::get_pixel32(SDL_Surface *surface, int x, int y) {
    return Pixelable::get32( surface, x, y );
    //Convert the pixels to 32 bit
    auto *pixels = (Uint32 *)surface->pixels;

    //Get the requested pixel
    return pixels[ ( y * surface->w ) + x ];
}

//deprecated
void Loader::put_pixel32(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    return Pixelable::put32( surface, x, y , pixel );
    //Convert the pixels to 32 bit
    auto *pixels = (Uint32 *)surface->pixels;
    //Set the pixel
    pixels[ ( y * surface->w ) + x ] = pixel;
}

SDL_Rect Loader::BiggestSurfaceClipRect(SDL_Surface *src, SDL_Surface *dst) {
    SDL_Rect srcsize;
    SDL_Rect dstsize;
    SDL_GetClipRect(src, &srcsize);
    SDL_GetClipRect(dst, &dstsize);
    //dstsize.w = Config::TARGET_WIDTH;
    //dstsize.h = Config::TARGET_HEIGHT;
    //dstsize.x = srcsize.w < dstsize.w ? abs((srcsize.w - dstsize.w )) / 2: 0;
    //dstsize.y = srcsize.h < dstsize.h ? abs((srcsize.h - dstsize.h )) / 2: 0;
    return dstsize;
}

inline void Loader::blitLineScaled(SDL_Surface *src, SDL_Surface* dst, int& line, double& scale) {
    SDL_Rect srcrect;
    SDL_Rect dstrect;
    srcrect.x = 0;
    srcrect.y = line;
    srcrect.w = Config::SCREEN_WIDTH;
    srcrect.h = 1;
    int width  = round( (double) Config::SCREEN_WIDTH * scale );
    int center = round( (double) (Config::SCREEN_WIDTH - width) / 2 );
    dstrect.x = center;
    dstrect.y = line;
    dstrect.w = width;
    dstrect.h = 1;
    SDL_BlitScaled(src, &srcrect, dst, &dstrect);
}

SDL_Rect Loader::SmallerBlitArea(SDL_Surface *src, SDL_Surface *dst) {
    SDL_Rect srcsize;
    SDL_Rect dstsize;
    SDL_Rect retsize;
    SDL_GetClipRect(src, &srcsize);
    SDL_GetClipRect(dst, &dstsize);
    retsize.x = srcsize.x < dstsize.x ? srcsize.x: dstsize.x;
    retsize.y = srcsize.y < dstsize.y ? srcsize.y: dstsize.y;

    return retsize;
}

//deprecated
void Loader::SurfacePixelsCopy(SDL_Surface *src, SDL_Surface *dst) {
    //Surfaceable::encode(dst, src);
    size_t area = src->w * src->h * sizeof(Uint32);
    memcpy( dst->pixels , src->pixels, area );
}

std::string Loader::sha256Log(uint8_t *data, size_t len) {
    picosha2::hash256_one_by_one hasher;
    hasher.process(data, &data[len]);
    hasher.finish();
    std::string hex_str = picosha2::get_hash_hex_string(hasher);
    return hex_str;
}

void Loader::GetRAWSurface(SDL_Surface *pSurface) {
}

void Loader::pushCache(SDL_Surface* frame) {
}


#endif //SDL_CRT_FILTER_LOADER_HPP
