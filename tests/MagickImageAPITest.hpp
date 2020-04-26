#ifndef SDL_CRT_FILTER_MAGICKAPITESTS_HPP
#define SDL_CRT_FILTER_MAGICKAPITESTS_HPP

#include <loaders/MagickLoader.hpp>
#include <BaseApp.hpp>

TEST_CASE( "SDL2 Magick++ API", "[App][SDL2][Magick++]") {

    SECTION( "load file, save blob, load blob, compare blob" ) {
        Config cfg;
        MagickLoader loader;
        cfg.initResources(loader);
        SDL_Surface* sample = nullptr;
        sample = MagickLoader::AllocateSurface  ( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( sample != nullptr );
        loader.GetSurface(sample);

    }
}

#endif //SDL_CRT_FILTER_MAGICKAPITESTS_HPP
