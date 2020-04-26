#ifndef SDL_CRT_FILTER_MAGICKAPITESTS_HPP
#define SDL_CRT_FILTER_MAGICKAPITESTS_HPP

#include <loaders/MagickLoader.hpp>
#include <BaseApp.hpp>

TEST_CASE( "SDL2 Magick++ API", "[App][SDL2][Magick++]") {

    SECTION( "SHA256Tests" ) {
        Config cfg;
        MagickLoader loader;
        cfg.initResources(loader);
        REQUIRE(
                loader.sha256Log("The quick brown fox jumps over the lazy dog") ==
                "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
        REQUIRE(
                loader.sha256Log("The quick brown fox jumps over the lazy dog.") ==
                "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c");
    }

    SECTION( "Convert between SDL surface and Magick++ image" ) {
        Config cfg;
        MagickLoader loader;
        cfg.initResources(loader);
        SDL_Surface* sample = nullptr;
        sample = MagickLoader::AllocateSurface  ( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( sample != nullptr );
        loader.GetSurface(sample);
        SDL_Surface* conv =
                MagickLoader::AllocateSurface  ( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( conv != nullptr );
        using namespace Magick;
        Image image{
                Geometry( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT ),
                Color()
        };
        loader.surface2image( sample, image );
        loader.image2surface( image, conv );
        REQUIRE(loader.CompareSurface(sample, conv));
        SDL_FreeSurface( sample );
        SDL_FreeSurface( conv );
    }


    SECTION( "Save and load blob" ) {
        Config cfg;
        MagickLoader loader, saver;
        cfg.initResources(loader);
        cfg.initResources(saver);

        SDL_Surface*
                sample = MagickLoader::AllocateSurface  ( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( sample != nullptr );
        SDL_Surface*
                remote = MagickLoader::AllocateSurface  ( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        REQUIRE( remote != nullptr );
        saver.GetSurface(sample);
        using namespace Magick;

        saver.saveBlob( sample, "/tmp/sample.blob" );

        Image image {
                loader.readBlob("/tmp/sample.blob")
        };

        saver.image2surface( image, remote );
        SDL_SaveBMP( remote, "/tmp/remote.bmp");

        REQUIRE( Loader::CompareSurface( sample, remote ) );

        unlink("/tmp/sample.blob");
        unlink("/tmp/remote.bmp");

        SDL_FreeSurface( remote );
        SDL_FreeSurface( sample );
    }


}

#endif //SDL_CRT_FILTER_MAGICKAPITESTS_HPP
