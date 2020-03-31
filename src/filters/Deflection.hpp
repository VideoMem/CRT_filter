#ifndef SDL_CRT_FILTER_DEFLECTION_HPP
#define SDL_CRT_FILTER_DEFLECTION_HPP
#include <filters/Filter.hpp>
#include <generators/Ripple.hpp>
#include <prngs.h>

#define rand() xorshift()

struct DeflectionFilterParams {
    double ripple;
    double vsupply;
    int warp;
    bool Hcomp = true;
    bool Vcomp = true;
};

template <typename A>
class DeflectionFilter: public Filter<A> {
public:
    DeflectionFilter( SDL_PixelFormat& format ) {
        gBack = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, format);
    }

    ~DeflectionFilter() {
        SDL_FreeSurface(gBack);
    }

    void run( A* surface, A* dest, DeflectionFilterParams& par ) {
        params = par; run(surface, dest, par.ripple, par.vsupply, par.warp);
    }
    void run(A* surface, A* dest, double& ripple, double& vsupply, int& warp);

protected:
    inline void HRipple( A *surface, A *dest,
                         double ripple,
                         double supplyV,
                         int sync
    );
    inline void VRipple( A *surface, A *dest,
            double ripple,
            double supplyV,
            int sync
            );
    Ripple rippleGen;
    SDL_Surface* gBack;
    DeflectionFilterParams params;
};

template<typename A>
void DeflectionFilter<A>::run( A *surface, A *dest, double& ripple, double &supplyV, int& warp ) {
    Loader::blank( dest );
    Loader::blank( gBack);
    int sync( warp );
    if(params.Hcomp) {
        HRipple(surface, gBack, ripple, supplyV, sync);
        if (params.Vcomp) VRipple(gBack, dest, ripple, supplyV, sync);
        else Loader::SurfacePixelsCopy( gBack, dest );
    } else {
        if (params.Vcomp) VRipple(surface, dest, ripple, supplyV, sync);
        else Loader::SurfacePixelsCopy( surface, dest );
    }
}

template<typename A>
void DeflectionFilter<A>::HRipple(A *surface, A *dest, double ripple, double supplyV, int sync) {
    for( int y = 0; y < Config::SCREEN_HEIGHT; ++y ) {
        double scale = ((0.337 * supplyV ) + 0.663) * rippleGen.get(sync, ripple);
        ++sync;
        Loader::blitLineScaled( surface, dest, y, scale );
    }
}


template<typename A>
void DeflectionFilter<A>::VRipple( A *surface, A *dest, double ripple, double supplyV, int sync ) {
    int frameSlip = round(((rand() & 0xFF) / 0xF0) * 0.3 );
    int newy = 0, last_blitY = 0;


    for(int y=0; y< Config::SCREEN_HEIGHT; ++y) {
        double scale = (supplyV * rippleGen.get(sync, ripple)); ++sync;
        int height = round((double) Config::SCREEN_HEIGHT * scale);
        int center = round((double) (Config::SCREEN_HEIGHT - height) / 2);

        last_blitY = newy;
        newy = round(y * scale) + center + frameSlip;
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
}

/*
void VRipple( SDL_Surface *surface, SDL_Surface *dest, int warp, ) {
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
        SDL_BlitSurface(surface, NULL, dest, NULL);
}
*/

#endif //SDL_CRT_FILTER_DEFLECTION_HPP


