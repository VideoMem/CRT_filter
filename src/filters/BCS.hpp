#ifndef SDL_CRT_FILTER_BCS_HPP
#define SDL_CRT_FILTER_BCS_HPP
#include <filters/Filter.hpp>
#include <prngs.h>
#include <generators/Ripple.hpp>

struct BCSFilterParams {
    float saturation, brightness, contrast, supply_voltage, ripple;
    int frame_sync;
};

template <typename A>
class BCSFilter: public Filter<A> {
public:
    static void run(A* surface, A* dest, BCSFilterParams&);

protected:
    Ripple rippleGen;
};

template <typename A>
void BCSFilter<A>::run(A *surface, A *dest, BCSFilterParams& ctrl) {
    SDL_FillRect(dest, nullptr, 0x000000);
    Uint32 pixel, R, G, B, BiasR, BiasG, BiasB, pixelOut;
    float luma, Db, Dr;
    for (int y = 0; y < Config::SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < Config::SCREEN_WIDTH; ++x) {
            pixel = Loader::get_pixel32(surface, x, y) & Loader::cmask;
            Loader::comp(&pixel, &R, &G, &B);
            Loader::toLuma(&luma, &R, &G, &B);
            Loader::toChroma(&Db, &Dr, &R, &G, &B);
            luma *= ctrl.contrast;
            luma += (1 - ctrl.brightness) * ctrl.contrast;
            Dr *= ctrl.saturation * ctrl.contrast;
            Db *= ctrl.saturation * ctrl.contrast;
            Loader::toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);
            Loader::toPixel(&pixelOut, &BiasR, &BiasG, &BiasB);
            Loader::put_pixel32(dest, x , y, pixelOut);
        }
    }
}

#endif //SDL_CRT_FILTER_BCS_HPP
