#ifndef SDL_CRT_FILTER_BCS_HPP
#define SDL_CRT_FILTER_BCS_HPP
#include <filters/Filter.hpp>
#include <generators/Ripple.hpp>
#include <glm/glm.hpp>

struct BCSFilterParams {
    float saturation, brightness, contrast, supply_voltage, ripple;
    int frame_sync;
};

typedef glm::mat<Config::SCREEN_HEIGHT, Config::SCREEN_WIDTH, float, glm::highp> screenMat;
typedef glm::vec<Config::SCREEN_WIDTH, float, glm::highp> screenRow;

template <typename A>
class BCSFilter: public Filter<A> {
public:
    static void run(A* surface, A* dest, BCSFilterParams&);
protected:
    static void surfaceRGB(A* surface, screenMat& R, screenMat& G, screenMat& B );
    static void surfaceYrBrCr(A* surface, screenMat& luma, screenMat& chroma, BCSFilterParams&);
    Ripple rippleGen;
};

template <typename A>
void BCSFilter<A>::run(A *surface, A *dest, BCSFilterParams& ctrl) {
    SDL_FillRect(dest, nullptr, 0x000000);
    static Uint32 pixel, R, G, B, BiasR, BiasG, BiasB, pixelOut;
    static float luma, Db, Dr;
    for (int y = 0; y < Config::SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < Config::SCREEN_WIDTH; ++x) {
            pixel = Loader::get_pixel32(surface, x, y) & Loader::cmask;
            Loader::comp(&pixel, &R, &G, &B);
            Loader::toLuma(&luma, &R, &G, &B);
            Loader::toChroma(&Db, &Dr, &R, &G, &B);
            luma += (1 - ctrl.brightness);
            luma *= ctrl.contrast;
            Dr *= ctrl.saturation * ctrl.contrast;
            Db *= ctrl.saturation * ctrl.contrast;

            Loader::toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);*/
            Loader::toPixel(&pixelOut, &BiasR, &BiasG, &BiasB);
            Loader::put_pixel32(dest, x , y, pixelOut);
        }
    }
}

template<typename A>
void BCSFilter<A>::surfaceRGB(A *surface, screenMat &Rc, screenMat &Gc, screenMat &Bc) {
    Uint32 pixel, R, G, B;

    for (int y = 0; y < Config::SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < Config::SCREEN_WIDTH; ++x) {
            pixel = Loader::get_pixel32(surface, x, y) & Loader::cmask;
            Loader::comp(&pixel, &R, &G, &B);

        }
    }
}

template<typename A>
void BCSFilter<A>::surfaceYrBrCr(A *surface, screenMat &luma, screenMat& chroma, BCSFilterParams &) {
    glm::mat4 mat;
    Uint32 pixel, R, G, B;
}


#endif //SDL_CRT_FILTER_BCS_HPP
