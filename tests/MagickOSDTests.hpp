//
// Created by sebastian on 16/3/20.
//

#ifndef SDL_CRT_FILTER_MAGICKOSDTESTS_HPP
#define SDL_CRT_FILTER_MAGICKOSDTESTS_HPP

#include <Config.hpp>
#include <generators/MagickOSD.hpp>

TEST_CASE( "SDL2 Magick++ OSD", "[OSD][SDL2][Magick++]") {

    SECTION( "OSD, create / destroy" ) {
        MagickOSD* osd = new MagickOSD();
        delete(osd);
    }

    SECTION( "OSD, self test" ) {
        MagickOSD osd;
        osd.test();
        SDL_Surface* sample = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        osd.getSurface( sample );
        SDL_SaveBMP( sample, "test_OSD_Self_Test.bmp" );
        SDL_FreeSurface( sample );
    }


}

TEST_CASE( "SDL2 Magick++ OSD Benchmarks", "[OSD][SDL2][Magick++][Benchmaking]") {

    SECTION( "OSD, text rendering speed" ) {
        const int iterations = 10e3;
        MagickOSD osd;

        time_t times[2];
        time(&times[0]);
        for(int i = 0; i < iterations; ++i ) {
            osd.text(0,0, "This is only a test");
        }
        time(&times[1]);
        double  blits = (double) iterations / (double) (times[1] - times[0]);
        SDL_Log("Test Result (MagickOSD::text) %.02lf lines/s", blits );
    }

    SECTION( "OSD, shadow text rendering speed" ) {
        const int iterations = 10e3;
        MagickOSD osd;

        time_t times[2];
        time(&times[0]);
        for(int i = 0; i < iterations; ++i ) {
            osd.shadowText(0,0, "This is only a test");
        }
        time(&times[1]);
        double  blits = (double) iterations / (double) (times[1] - times[0]);
        SDL_Log("Test Result (MagickOSD::shadowText) %.02lf lines/s", blits );
        SDL_Surface* sample = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        osd.getSurface( sample );
        SDL_SaveBMP( sample, "test_OSD_shadowText.bmp" );
        SDL_FreeSurface( sample );
    }

}

#endif //SDL_CRT_FILTER_MAGICKOSDTESTS_HPP