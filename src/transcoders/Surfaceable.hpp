//
// Created by sebastian on 22/5/20.
//

#ifndef SDL_CRT_FILTER_SURFACEABLE_HPP
#define SDL_CRT_FILTER_SURFACEABLE_HPP
#include <Config.hpp>
#include <transcoders/Transcodeable.hpp>
#include <SDL2/SDL.h>

class Surfaceable: public Transcodeable {
public:
    static void encode(void* dst, void* src);
    static void decode(void* dst, void* src) { encode(dst, src); }
    static SDL_Surface* AllocateSurface(int w, int h);
    static SDL_Surface* AllocateSurface(int w, int h, SDL_PixelFormat &format);
    static SDL_Surface* AllocateSurface(SDL_Surface* mock) { return AllocateSurface(mock, 1.0); }
    static SDL_Surface* AllocateSurface(SDL_Surface* mock, double scale );

    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    static struct mask_t {
            static const Uint32 type = 1;
            static const Uint32 r = 0xff000000;
            static const Uint32 g = 0x00ff0000;
            static const Uint32 b = 0x0000ff00;
            static const Uint32 a = 0x000000ff;
            static const Uint32 c = 0xffffff00;
        } mask;
    #else
        static struct mask_t {
            static const Uint32 type = 0;
            static const Uint32 r = 0x000000ff;
            static const Uint32 g = 0x0000ff00;
            static const Uint32 b = 0x00ff0000;
            static const Uint32 a = 0xff000000;
            static const Uint32 c = 0x00ffffff;
        } mask;
    #endif

protected:
    struct Dimension_t {
        SDL_Rect src;
        SDL_Rect dst;
    };
    inline static Dimension_t getDimensions(SDL_Surface* dst, SDL_Surface* src);
};

void Surfaceable::encode(void *dst, void *src) {
    auto src_frame = static_cast<SDL_Surface*>( src );
    auto dst_frame = static_cast<SDL_Surface*>( dst );
    Dimension_t sizes = getDimensions( dst_frame, src_frame );
    if( memcmp( &sizes.src, &sizes.dst, sizeof(SDL_Rect) ) == 0 ) {
        memcpy ( dst_frame, src_frame, sizeof (SDL_Surface) );
        memcpy ( &dst_frame->clip_rect, &src_frame->clip_rect, sizeof (SDL_Rect) );
        memcpy ( &dst_frame->format, &src_frame->format, sizeof (SDL_PixelFormat) );
        size_t area = sizes.src.w * sizes.src.h * sizeof(Uint32);
        memcpy( dst_frame->pixels , src_frame->pixels, area );
    } else {
        SDL_BlitScaled(src_frame, &sizes.src, dst_frame, &sizes.dst );
    }
}

Surfaceable::Dimension_t Surfaceable::getDimensions(SDL_Surface *dst, SDL_Surface *src) {
    Dimension_t sizes = {{0,0, 0, 0}, {0, 0, 0, 0 }};
    SDL_GetClipRect( src, &sizes.src );
    SDL_GetClipRect( dst, &sizes.dst );
    return sizes;
}

SDL_Surface* Surfaceable::AllocateSurface( int w, int h, SDL_PixelFormat& format ) {
    SDL_Surface* ns = SDL_CreateRGBSurface(0, w, h, 32,
                                           mask_t::r, mask_t::g, mask_t::b, mask_t::a );
    SDL_Surface* optimized = SDL_ConvertSurface(ns, &format, 0);
    SDL_FreeSurface(ns);
    return optimized;
}

SDL_Surface* Surfaceable::AllocateSurface( int w, int h ) {
    return SDL_CreateRGBSurface(0, w, h, 32,
                                mask_t::r, mask_t::g, mask_t::b, mask_t::a );
}

SDL_Surface *Surfaceable::AllocateSurface( SDL_Surface *mock, double scale ) {
    assert(mock != nullptr && "Reference surface cannot be nullptr");
    SDL_Rect mock_size;
    SDL_GetClipRect( mock, &mock_size );
    mock_size.w = static_cast<int>( round(mock_size.w * scale ) );
    mock_size.h = static_cast<int>( round(mock_size.h * scale ) );
    return AllocateSurface( mock_size.w, mock_size.h, *mock->format );
}


#endif //SDL_CRT_FILTER_SURFACEABLE_HPP
