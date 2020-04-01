#ifndef SDL_CRT_FILTER_BCS_HPP
#define SDL_CRT_FILTER_BCS_HPP
#include <filters/Filter.hpp>
#include <generators/Ripple.hpp>

//YDbDr colorspace adjust filter

struct BCSFilterParams {
    double saturation, brightness, contrast, supply_voltage, ripple;
    int frame_sync;
};

template <typename A>
class BCSFilter: public Filter<A> {
public:
    static void run(A* surface, A* dest, BCSFilterParams&);
protected:
    inline static Uint32 pixelFilter(Uint32 &px, BCSFilterParams &ctrl);
    inline void toYDbDr(A* surface, BCSFilterParams& ctrl);
    Ripple rippleGen;
};


template <typename A>
void BCSFilter<A>::run(A *surface, A *dest, BCSFilterParams& ctrl) {
    static const int div = 13;
    SDL_FillRect(dest, nullptr, 0x000000);
    static Uint32 pixel[div], pixelOut[div];
    int offset=0;

    //duff device makes it slower
    for ( int y = 0; y < Config::SCREEN_HEIGHT; ++y ) {
        for ( int x = 0; x < Config::SCREEN_WIDTH; ++x ) {
            pixel[offset] = Loader::get_pixel32(surface, x, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x , y, pixelOut[offset]);
        }
    }
}

template<typename A>
Uint32 BCSFilter<A>::pixelFilter(Uint32 &pixel, BCSFilterParams &ctrl) {
    static double luma, Db, Dr;
    static Uint32 R, G, B, BiasR, BiasG, BiasB, pixelOut;

    Loader::comp(&pixel, &R, &G, &B);
    Loader::toLuma(&luma, &R, &G, &B);
    Loader::toChroma(&Db, &Dr, &R, &G, &B);
    luma += (1 - ctrl.brightness);
    luma *= ctrl.contrast;
    Dr *= ctrl.saturation * ctrl.contrast;
    Db *= ctrl.saturation * ctrl.contrast;
    Loader::toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);
    Loader::toPixel(&pixelOut, &BiasR, &BiasG, &BiasB);

    return pixelOut;
}

#endif //SDL_CRT_FILTER_BCS_HPP
