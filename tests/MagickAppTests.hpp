#ifndef SDL_CRT_FILTER_MAGICKAPPTESTS_HPP
#define SDL_CRT_FILTER_MAGICKAPPTESTS_HPP

#include <loaders/MagickLoader.hpp>
#include <BaseApp.hpp>

TEST_CASE( "SDL2 Magick++ App", "[App][SDL2][Magick++]") {

    SECTION("MagickLoader, Surface Allocation") {
        Config cfg;
        MagickLoader loader;
        cfg.initResources(loader);
        SDL_Surface* sample = nullptr;
        SDL_Surface* copy = nullptr;
        sample = MagickLoader::AllocateSurface  ( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        copy   = MagickLoader::AllocateSurface  ( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( sample != nullptr );
        REQUIRE( copy   != nullptr );

        loader.GetSurface(sample);
        loader.GetSurface(copy);
        SDL_Rect srcrect;
        SDL_GetClipRect( sample, &srcrect );
        REQUIRE( MagickLoader::CompareSurface(sample, copy) );
        SDL_FreeSurface(sample);
        SDL_FreeSurface(copy);
    }

    SECTION("MagickLoader Surface Mismatch") {
        Config cfg;
        MagickLoader loader;
        cfg.initResources(loader);
        SDL_Surface* sample = nullptr;
        SDL_Surface* copy = nullptr;
        sample = MagickLoader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        copy   = MagickLoader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( sample != nullptr );
        REQUIRE( copy   != nullptr );

        loader.GetSurface(sample);
        loader.Up();
        loader.GetSurface(copy);
        SDL_Rect srcrect;
        SDL_GetClipRect( sample, &srcrect );
        REQUIRE( !MagickLoader::CompareSurface(sample, copy) );
        SDL_FreeSurface(sample);
        SDL_FreeSurface(copy);
    }

    SECTION("Magick++ App Instantiation") {
        MagickLoader loader = MagickLoader();
        BaseApp app = BaseApp(loader);
        app.Standby();
        //SDL_Delay(3000);
    }

}

#endif //SDL_CRT_FILTER_MAGICKAPPTESTS_HPP
