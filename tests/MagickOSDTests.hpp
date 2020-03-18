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
#endif //SDL_CRT_FILTER_MAGICKOSDTESTS_HPP