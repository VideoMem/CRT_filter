//
// Created by sebastian on 25/3/20.
//

#ifndef SDL_CRT_FILTER_BLITTERBENCHS_HPP
#define SDL_CRT_FILTER_BLITTERBENCHS_HPP

#include <loaders/Loader.hpp>

void SDL_Blitter(SDL_Surface* src, SDL_Surface* dst, long iterations, std::string msg ) {
    for(int i= 0; i < 1000; ++i)
        SDL_BlitSurface(src, nullptr, dst, nullptr);
    time_t times[2];
    time(&times[0]);
    for(int i= 0; i < iterations; ++i)
        SDL_BlitSurface(src, nullptr, dst, nullptr);
    time(&times[1]);
    double  blits = (double) iterations / (double) (times[1] - times[0]);
    SDL_Log("Test Result %.02lf blits/s (%s)", blits, msg.c_str() );
}

TEST_CASE( "SDL2 BlitSurface Bench", "[SDL2][BenchMark]") {

    SECTION("SDL_BlitSuface BLENDMODE_BLEND") {
        SDL_Surface* src = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_Surface* dst = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_BLEND );
        SDL_SetSurfaceBlendMode(dst, SDL_BLENDMODE_BLEND );
        SDL_Blitter(src, dst, 2e4, "blend");
        SDL_FreeSurface(src);
        SDL_FreeSurface(dst);
    }

    SECTION("SDL_BlitSuface BLENDMODE_NONE") {
        SDL_Surface* src = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_Surface* dst = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE );
        SDL_SetSurfaceBlendMode(dst, SDL_BLENDMODE_NONE );
        SDL_Blitter(src, dst, 2e4, "none");
        SDL_FreeSurface(src);
        SDL_FreeSurface(dst);
    }


    SECTION("SDL_BlitSuface BLENDMODE_MOD") {
        SDL_Surface* src = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_Surface* dst = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_MOD );
        SDL_SetSurfaceBlendMode(dst, SDL_BLENDMODE_MOD );
        SDL_Blitter(src, dst, 1e3, "mod");
        SDL_FreeSurface(src);
        SDL_FreeSurface(dst);
    }

    SECTION("SDL_BlitSuface BLENDMODE_ADD") {
        SDL_Surface* src = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_Surface* dst = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_ADD );
        SDL_SetSurfaceBlendMode(dst, SDL_BLENDMODE_ADD );
        SDL_Blitter(src, dst, 1e3, "add");
        SDL_FreeSurface(src);
        SDL_FreeSurface(dst);
    }

}


TEST_CASE( "SDL2 ConvertSurface Bench", "[SDL2][BenchMark]") {

    SECTION("SDL_ConvertSurface Blit") {
        SDL_Surface* src = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        std::vector<SDL_Surface*> dsts;

        for( int i=0; i < 1e3 ; ++i ) {
            SDL_Surface* dst = SDL_ConvertSurfaceFormat(src, src->format->format, src->flags );
            dsts.push_back( dst );
        }

        for( auto& x : dsts ) {
            SDL_FreeSurface( x );
        }
        dsts.clear();

        time_t times[2];
        time(&times[0]);

        int iterations = 2100;
        for( int i=0; i < iterations ; ++i ) {
            SDL_Surface* dst = SDL_ConvertSurfaceFormat(src, src->format->format, src->flags );
            dsts.push_back( dst );
        }

        time(&times[1]);
        double  blits = (double) iterations / (double) (times[1] - times[0]);
        SDL_Log("Test Result (Convert) %.02lf blits/s", blits );

        for(auto& x : dsts) {
            SDL_FreeSurface( x );
        }
    }

}



TEST_CASE( "SDL2 Surface memcpy Bench", "[SDL2][BenchMark]") {

    SECTION("SDL_ConvertSurface Blit") {
        SDL_Surface* src = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        SDL_Surface* dst = Loader::AllocateSurface(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);

        time_t times[2];
        time(&times[0]);
        int iterations = 2e5;
        for(int i = 0; i < iterations; ++i ) {
            Loader::SurfacePixelsCopy( src, dst );
        }
        time(&times[1]);
        double  blits = (double) iterations / (double) (times[1] - times[0]);
        SDL_Log("Test Result (memcpy) %.02lf blits/s", blits );

        SDL_FreeSurface( src );
        SDL_FreeSurface( dst );
    }

}

#endif //SDL_CRT_FILTER_BLITTERBENCHS_HPP
