#ifndef SDL_CRT_FILTER_BCS_HPP
#define SDL_CRT_FILTER_BCS_HPP
#include <filters/Filter.hpp>
#include <generators/Ripple.hpp>
#include <glm/glm.hpp>

//YDbDr colorspace adjust filter

struct BCSFilterParams {
    double saturation, brightness, contrast, supply_voltage, ripple;
    int frame_sync;
};

typedef glm::mat<Config::SCREEN_HEIGHT, Config::SCREEN_WIDTH, double, glm::highp> screenMat;
typedef glm::vec<Config::SCREEN_WIDTH, double, glm::highp> screenRow;

struct bcsCache {
    static double Y [Config::SCREEN_WIDTH][Config::SCREEN_HEIGHT];
    static double Db[Config::SCREEN_WIDTH][Config::SCREEN_HEIGHT];
    static double Dr[Config::SCREEN_WIDTH][Config::SCREEN_HEIGHT];
};

template <typename A>
class BCSFilter: public Filter<A> {
public:
    static void run(A* surface, A* dest, BCSFilterParams&);
protected:
    inline void toYDbDr(A* surface, BCSFilterParams& ctrl);
    static void surfaceRGB(A* surface, screenMat& R, screenMat& G, screenMat& B );
    static void surfaceYrBrCr(A* surface, screenMat& luma, screenMat& chroma, BCSFilterParams&);
    Ripple rippleGen;
    bcsCache cache;
};

template <typename A>
void BCSFilter<A>::run(A *surface, A *dest, BCSFilterParams& ctrl) {
    SDL_FillRect(dest, nullptr, 0x000000);
    static Uint32 pixel, R, G, B, BiasR, BiasG, BiasB, pixelOut;
    static double luma, Db, Dr;
    //toYDbDr(surface, ctrl);
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

            Loader::toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);
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

template<typename A>
void BCSFilter<A>::toYDbDr(A *surface, BCSFilterParams &ctrl) {
    static double luma, Db, Dr;
    static Uint32 pixel, R, G, B;
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
            cache.Y [x][y] = luma;
            cache.Dr[x][y] = Dr;
            cache.Db[x][y] = Db;
        }
    }
}



#endif //SDL_CRT_FILTER_BCS_HPP
