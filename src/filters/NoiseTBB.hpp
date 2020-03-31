//
// Created by sebastian on 22/2/20.
//

#ifndef SDL_CRT_FILTER_NOISE_HPP
#define SDL_CRT_FILTER_NOISE_HPP
#include <filters/Filter.hpp>
#include <prngs.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/tbb.h>
using namespace tbb;

class ApplyFilter {
private:
    SDL_Surface *const src;
    SDL_Surface *const dst;

public:
    void operator()( const blocked_range<size_t>& r ) const {
        SDL_Surface *a = src;
        SDL_Surface *b = dst;

        Uint32 rnd, snow, pixel, R, G, B, BiasR, BiasG, BiasB, pxno, chrnoise;
        double luma, Db, Dr, noise;
        double gnoise = 0.3, color = 1, brightness =1, contrast = 1;

        //int noiseSlip = round(((rand() & 0xFF) / 0x50) * gnoise);
        for( size_t i=r.begin(); i!=r.end(); ++i ) {
            int top= r.end();
            int str= r.begin();
            int res = top + str;
            int x = i % Config::SCREEN_WIDTH;
            int y = i / Config::SCREEN_WIDTH;
            pixel = Loader::get_pixel32(a, x, y) & Loader::cmask;
            rnd = rand() & 0xFF;
            noise = Loader::fromChar(&rnd) * gnoise;
            chrnoise = Loader::toChar(&noise);
            Loader::toPixel(&snow, &chrnoise, &chrnoise, &chrnoise);
            Loader::comp(&pixel, &R, &G, &B);
            Loader::toLuma(&luma, &R, &G, &B);
            Loader::toChroma(&Db, &Dr, &R, &G, &B);
            luma = luma ==0? Loader::fromChar(&rnd) / 20: luma;
            luma *= (1 - noise) * contrast;
            luma += (1 - brightness) * contrast;
            Dr *= (1 - noise) * color * contrast;
            Db *= (1 - noise) * color * contrast;

            Loader::toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);
            Loader::toPixel(&pxno, &BiasR, &BiasG, &BiasB);
            Loader::put_pixel32(b, x, y,
                                pxno);
        }
    }
    ApplyFilter( SDL_Surface* s, SDL_Surface* d ) :
            src(s), dst(d)
    {}

    ApplyFilter() : dst(nullptr), src(nullptr) {}
};

template <typename A>
class NoiseFilter: public Filter<A>, ApplyFilter {
public:
    void parlapply(A* src, A* dst) {
        static affinity_partitioner ap;
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, Config::SCREEN_WIDTH * Config::SCREEN_HEIGHT),
            ApplyFilter(src, dst), ap);
    }
    void fill(A* surface, A* dest);
    void run(A* surface, A* dest, double& gnoise);
    NoiseFilter() {
        SDL_Surface* background = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_FillRect(background, nullptr, 0x000000);
        count = 0;
        for(int i = 0; i != frames; ++i) {
            gBack.insert(gBack.end(), Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT));
            fill(background, gBack.back());
        }
    }
    ~NoiseFilter() {
        for (int i = 0; i != frames; ++i) {
            SDL_FreeSurface(gBack.back());
            gBack.pop_back();
        }
    }

protected:
    static const size_t frames = 25;
    std::vector<A*> gBack;
    size_t count = 0;
};

template<typename A>
void NoiseFilter<A>::fill(A *surface, A *dest) {
    SDL_FillRect(dest, nullptr, 0x000000);
    Uint32 BiasR, BiasG, BiasB, pxno;
    int32_t rndL, rndDb, rndDr;
    Uint32 pixel, snow, R, G, B, chrnoise;
    double luma = 0, Db = 0, Dr = 0;
    for (int y = 0; y < Config::SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < Config::SCREEN_WIDTH; ++x) {
            rndL  = (rand() & 0xFF);
            rndDb = (rand() & 0xFF);
            rndDr = (rand() & 0xFF);

            luma = Loader::fromChar(&rndL);
            Dr   = Loader::fromChar(&rndDr) / 80;
            Db   = Loader::fromChar(&rndDb) / 80;

            Loader::toRGB(&luma, &Db, &Dr, &BiasR, &BiasG, &BiasB);
            Loader::toPixel(&pxno, &BiasR, &BiasG, &BiasB);
            Loader::put_pixel32(dest, x, y,
                                pxno);
        }
    }
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_ADD );
    SDL_BlitSurface(surface, nullptr, dest, nullptr);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE );
    //if(gnoise > 1) { desaturate(dest, gBlank); SDL_BlitSurface(gBlank, NULL, dest, NULL); }
}

template<typename A>
void NoiseFilter<A>::run(A *surface, A *dest, double& gnoise) {
    //uint64_t rnd = rand();
    size_t max = gBack.size();
    size_t select = count; //rnd * max / std::numeric_limits<uint64_t>::max();
    SDL_BlitSurface(surface, nullptr, dest, nullptr);
    SDL_SetSurfaceBlendMode(gBack[select], SDL_BLENDMODE_BLEND );
    SDL_SetSurfaceAlphaMod(gBack[select], 0xFF * gnoise);
    SDL_BlitSurface(gBack[select], nullptr, dest, nullptr);
    SDL_SetSurfaceBlendMode(gBack[select], SDL_BLENDMODE_NONE );
    ++count;
    if(count == max) count = 0;
}


#endif //SDL_CRT_FILTER_NOISE_HPP


