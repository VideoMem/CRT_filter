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
    inline static Uint32 pixelFilter(Uint32 &px, BCSFilterParams &ctrl);
    inline void toYDbDr(A* surface, BCSFilterParams& ctrl);
    static void surfaceRGB(A* surface, screenMat& R, screenMat& G, screenMat& B );
    static void surfaceYrBrCr(A* surface, screenMat& luma, screenMat& chroma, BCSFilterParams&);
    Ripple rippleGen;
    bcsCache cache;
};



template <typename A>
void BCSFilter<A>::run(A *surface, A *dest, BCSFilterParams& ctrl) {
    static const int div = 13;
    SDL_FillRect(dest, nullptr, 0x000000);
    static Uint32 pixel[div], pixelOut[div];
    int offset=0;
    //duff device?
    for ( int y = 0; y < Config::SCREEN_HEIGHT; ++y ) {
        for ( int x = 0; x < Config::SCREEN_WIDTH; ++x ) {
            pixel[offset] = Loader::get_pixel32(surface, x, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x , y, pixelOut[offset]);
            /*
            offset = 0;
            //0
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //1
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //2
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //3
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //4
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //5
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //6
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //7
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //8
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //9
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //10
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //11
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]);
            ++offset;
            //12
            pixel[offset] = Loader::get_pixel32(surface, x+offset, y) & Loader::cmask;
            pixelOut[offset] = pixelFilter( pixel[offset], ctrl );
            Loader::put_pixel32(dest, x+offset , y, pixelOut[offset]); */
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
