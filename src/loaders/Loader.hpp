//
// Created by sebastian on 20/2/20.
//

#ifndef SDL_CRT_FILTER_LOADER_HPP
#define SDL_CRT_FILTER_LOADER_HPP
#include <ResourceRoller.hpp>
#include <SDL2/SDL.h>

class Loader: public ResourceRoller {
public:
    virtual bool GetSurface(SDL_Surface*) {};
    static SDL_Surface* AllocateSurface(int w, int h);
    static SDL_Rect BiggestSurfaceClipRect(SDL_Surface* src, SDL_Surface* dst);
    static bool CompareSurface(SDL_Surface* src, SDL_Surface* dst);

protected:
    static Uint32 get_pixel32( SDL_Surface *surface, int x, int y);
    static void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel);

    //TODO: bit endianess
    inline static void comp(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B) {
        *B = (*pixel & bmask) >> 16;
        *G = (*pixel & gmask) >> 8;
        *R = *pixel & rmask ;
    }

    //TODO: bit endianess
    inline static void toPixel(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B) {
        *pixel = ((*B << 16) + (*G << 8) + *R) | amask;
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
};

SDL_Surface* Loader::AllocateSurface(int w, int h) {
    return SDL_CreateRGBSurface(0, w, h, 32,
                                rmask, gmask, bmask, amask);
}

bool Loader::CompareSurface(SDL_Surface *src, SDL_Surface *dst) {
    if (src->format->format == dst->format->format &&
        src->w == dst->w && src->h == dst->h) {
        bool error = false;
        SDL_LockSurface(src);
        SDL_LockSurface(dst);
        for(int x=0; x < src->w && !error; ++x)
            for(int y=0; y < src->h; ++y)
                if(get_pixel32(src, x, y) != get_pixel32(dst, x, y)) {
                    error = true;
                    break;
                }
        SDL_UnlockSurface(src);
        SDL_UnlockSurface(dst);
        return !error;
    }
    return false;
}

Uint32 Loader::get_pixel32(SDL_Surface *surface, int x, int y) {
    //Convert the pixels to 32 bit
    auto *pixels = (Uint32 *)surface->pixels;

    //Get the requested pixel
    return pixels[ ( y * surface->w ) + x ];
}

void Loader::put_pixel32(SDL_Surface *surface, int x, int y, Uint32 pixel) {
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

#endif //SDL_CRT_FILTER_LOADER_HPP
