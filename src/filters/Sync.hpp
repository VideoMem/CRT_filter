//
// Created by sebastian on 22/2/20.
//

#ifndef SDL_CRT_FILTER_SYNC_HPP
#define SDL_CRT_FILTER_SYNC_HPP
#include <filters/Filter.hpp>
#include <prngs.h>

template <typename A>
class SyncFilter: public Filter<A> {
public:
    void run(A* surface, A* dest, float& gnoise);

protected:
    int vdrift = 0;
    int hdrift = 0;
};

template<typename A>
void SyncFilter<A>::run(A *surface, A *dest, float& gnoise) {
    Loader::blank( dest );
    SDL_Rect srcRect, dstRect;
    SDL_GetClipRect( surface, &srcRect );
    SDL_GetClipRect( dest   , &dstRect );
    srcRect.h = 1;
    dstRect.h = 1;
    Uint32 rnd = rand() & 0xFF;
    double vDelay = (Loader::fromChar(&rnd) - 0.5) * gnoise;
    vdrift += round(vDelay * 10 );

    double lockLevel = 0.6 + (rnd & 1) / 10.0;
    const int vertHdrift = 16;

    if(gnoise < lockLevel) {
        hdrift /= 1.3;
        vdrift /= 1.3;
    } else if (gnoise > lockLevel + 0.1)
        vdrift -= vertHdrift;

    //# 40ms ___ 625lines
    //# x   ____ 610lines
    // x = 610 * 40 / 625 = 39.04

    int vband = Config::SCREEN_HEIGHT * 40 / 35;
    int hband = Config::SCREEN_WIDTH  * 64.0 / 52.0;

    for( int y = 0; y < Config::SCREEN_HEIGHT; ++y ) {
        if(gnoise > lockLevel)
            hdrift += 1;
        srcRect.y = y;
        int newy = ( y + vdrift ) % vband;

        dstRect.y = newy > 0 ? newy: vband + newy;

        Uint32 xrnd = rand() & 0xFF;
        double hDelay = ( Loader::fromChar(&xrnd) - 0.5 ) * gnoise;
        hdrift += round(hDelay );

        int newx = ( hdrift ) % hband;

        dstRect.x = newx; // > 0 ? newx: hband + newx; //round(((rand() & 0xFF) / 0x20) * gnoise );
        SDL_BlitSurface( surface, &srcRect, dest, &dstRect );
        dstRect.x = newx - hband;
        SDL_BlitSurface( surface, &srcRect, dest, &dstRect );
        dstRect.x = newx + hband;
        SDL_BlitSurface( surface, &srcRect, dest, &dstRect );
    }

}


#endif //SDL_CRT_FILTER_SYNC_HPP


