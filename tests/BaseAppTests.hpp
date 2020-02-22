#ifndef SDL_CRT_FILTER_BASEAPPTESTS_HPP
#define SDL_CRT_FILTER_BASEAPPTESTS_HPP

#include <BaseApp.hpp>

TEST_CASE( "SDL2 Basic App", "[App][SDL2]") {

    SECTION("Loader, Surface Allocation") {
        Config cfg;
        LazyLoader loader;
        cfg.initResources(loader);
        SDL_Surface* sample = nullptr;
        SDL_Surface* copy = nullptr;
        sample = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        copy   = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( sample != nullptr );
        REQUIRE( copy   != nullptr );

        loader.GetSurface(sample);
        loader.GetSurface(copy);
        SDL_Rect srcrect;
        SDL_GetClipRect( sample, &srcrect );
        REQUIRE(Loader::CompareSurface(sample, copy));
        SDL_FreeSurface(sample);
        SDL_FreeSurface(copy);
    }

    SECTION("Loader Surface Mismatch") {
        Config cfg;
        LazyLoader loader;
        cfg.initResources(loader);
        SDL_Surface* sample = nullptr;
        SDL_Surface* copy = nullptr;
        sample = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        copy   = Loader::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( sample != nullptr );
        REQUIRE( copy   != nullptr );

        loader.GetSurface(sample);
        loader.Up();
        loader.GetSurface(copy);
        SDL_Rect srcrect;
        SDL_GetClipRect( sample, &srcrect );
        REQUIRE(!Loader::CompareSurface(sample, copy));
        SDL_FreeSurface(sample);
        SDL_FreeSurface(copy);
    }

    SECTION("Base App Instantiation") {
        LazyLoader loader;
        BaseApp app(loader);
        app.Standby();
        SDL_Delay(1000);
    }

}

#endif //SDL_CRT_FILTER_BASEAPPTESTS_HPP
