//
// Created by sebastian on 22/2/20.
//

#ifndef SDL_CRT_FILTER_NOISE_HPP
#define SDL_CRT_FILTER_NOISE_HPP
#include <filters/Filter.hpp>
#include <prngs.h>
#include <deque>

#define rand() xorshift()

template <typename A>
class NoiseFilter: public Filter<A> {
public:
    void fill(A* dest);
    void run(A* surface, A* dest, double& gnoise);
    NoiseFilter(SDL_PixelFormat& format) {
        fmt = format;
//        SDL_Surface* background = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, format);
//        Loader::blank(background);
        count = 0;
        for(int i = 0; i != frames; ++i) {
            gBack.insert(gBack.end(), Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, format));
            fill(gBack.back());
            SDL_SetSurfaceBlendMode(gBack.back(), SDL_BLENDMODE_BLEND );
        }
//        SDL_FreeSurface(background);
    }
    ~NoiseFilter() {
        for (int i = 0; i != frames; ++i) {
            SDL_FreeSurface(gBack.back());
            gBack.pop_back();
        }
    }

protected:
    static const size_t frames = 60;
    std::vector<A*> gBack;
    SDL_PixelFormat fmt;
    size_t count = 0;
    static double simpleLoPass(std::deque<double>& samples, size_t range ) {
        auto it = samples.begin();
        double sum = 0;
        while(samples.size() > range) {
            samples.pop_front();
        }
        while (it != samples.end()) {
            sum+= *it++;
        }

        return sum / samples.size();
    }

};

template<typename A>
void NoiseFilter<A>::fill(A *dest) {
    SDL_FillRect(dest, nullptr, 0x000000);
    Uint32 BiasR, BiasG, BiasB, pxno;
    int32_t rndL, rndDb, rndDr;
    double luma = 0, Db = 0, Dr = 0;
    std::deque<double> avgY, avgDb, avgDr;
    rndL  = rand() & 0xFF;
    rndDb = rand() & 0xFF;
    rndDr = rand() & 0xFF;
    avgY.push_front(Loader::fromChar(&rndL));
    avgDb.push_front(Loader::fromChar(&rndDb));
    avgDr.push_front(Loader::fromChar(&rndDr));

    SDL_Surface* noiseSurface = Loader::AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);
    for (int y = 0; y < Config::NKERNEL_HEIGHT; ++y) {
        for (int x = 0; x < Config::NKERNEL_WIDTH; ++x) {
            rndL  = (rand() & 0xFF);
            rndDb = (rand() & 0xFF);
            rndDr = (rand() & 0xFF);

            luma = Loader::fromChar(&rndL);
            Db   = (Loader::fromChar(&rndDb) * 2.666) - 1.333;
            Dr   = (Loader::fromChar(&rndDr) * 2.666) - 1.333;
            avgY.push_back(luma);
            avgDb.push_back(Db);
            avgDr.push_back(Dr);
            luma = 2 * abs(simpleLoPass(avgY, 32)  - luma);
            Db   = simpleLoPass(avgDb, 64);
            Dr   = simpleLoPass(avgDr, 64);

            Loader::toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);
            Loader::toPixel(&pxno, &BiasR, &BiasG, &BiasB);
            Loader::put_pixel32(noiseSurface, x, y,
                                pxno);
        }
    }
    SDL_Surface* optimizedNoise = SDL_ConvertSurface(noiseSurface, &fmt, 0);
    SDL_SetSurfaceAlphaMod(optimizedNoise, 0xFF);
    SDL_Rect dstRect;
    SDL_Rect srcRect;
    SDL_GetClipRect(dest, &dstRect);
    SDL_GetClipRect(optimizedNoise, &srcRect);
    SDL_BlitScaled(optimizedNoise, &srcRect, dest, &dstRect);
    SDL_FreeSurface(noiseSurface);
    SDL_FreeSurface(optimizedNoise);
}

template<typename A>
void NoiseFilter<A>::run(A *surface, A *dest, double& gnoise) {
    size_t max = gBack.size();
    size_t select = count;
    Loader::SurfacePixelsCopy( surface, dest );
    SDL_SetSurfaceAlphaMod(gBack[select], 0xFF * gnoise);
    SDL_BlitSurface(gBack[select], nullptr, dest, nullptr);
    ++count;
    if(count == max) count = 0;
}


#endif //SDL_CRT_FILTER_NOISE_HPP


