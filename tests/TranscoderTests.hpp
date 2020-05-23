//
// Created by sebastian on 22/5/20.
//

#ifndef SDL_CRT_FILTER_TRANSCODERTESTS_HPP
#define SDL_CRT_FILTER_TRANSCODERTESTS_HPP
#include <transcoders/Surfaceable.hpp>
#include <transcoders/Waveable.hpp>
#include <loaders/fmt_tools/WaveFile.hpp>

TEST_CASE( "Type Conversions", "[SDL2][Transcodeable]") {

    SECTION("Surface size conversions") {
        Surfaceable transcoder;
        SDL_Surface * img = SDL_LoadBMP( "resources/images/standby640.bmp" );
        REQUIRE( img != nullptr );
        SDL_Surface * copy = Surfaceable::AllocateSurface( img );
        SDL_Surface * resized = Surfaceable::AllocateSurface( img );
        SDL_Surface * halved = Surfaceable::AllocateSurface( img, 0.5 );
        SDL_Surface * doubled = Surfaceable::AllocateSurface( img, 2 );
        REQUIRE( copy != nullptr );
        REQUIRE( halved != nullptr );
        REQUIRE( doubled != nullptr );

        transcoder.encode( copy, img );
        transcoder.encode( halved, img );
        transcoder.encode( doubled, img );
        transcoder.encode( resized, doubled );

        REQUIRE(memcmp(img->pixels, copy->pixels, img->w * img->h ) == 0);
        REQUIRE(memcmp(img->pixels, resized->pixels, img->w * img->h ) == 0);

        SDL_FreeSurface( copy );
        SDL_FreeSurface( resized );
        SDL_FreeSurface( halved );
        SDL_FreeSurface( doubled );
    }


    SECTION("Wave to surface and surface to wave conversions (pixel median space)") {
        Waveable transcoder;
        WaveIO waveio; waveio.setFM();
        SDL_Surface * img = SDL_LoadBMP( "resources/images/standby640.bmp" );
        REQUIRE( img != nullptr );
        SDL_Surface * copy = Surfaceable::AllocateSurface( img );
        REQUIRE( copy != nullptr );

        size_t required_size = Waveable::conversion_size( img );
        REQUIRE(required_size == img->w * img->h);
        auto wave = new uint8_t[required_size];
        auto file = new uint8_t[required_size];

        transcoder.decode(wave, img);
        waveio.write("standby640.wav", wave, required_size );
        waveio.read("standby640.wav", file );
        transcoder.encode(copy, file);

        REQUIRE(memcmp(img->pixels, copy->pixels, required_size ) == 0);

        delete [] wave;
        delete [] file;
        SDL_FreeSurface( copy );
    }

    SECTION("Wave to surface and surface to wave conversions (luma space)") {
        WaveLuma transcoder;
        WaveIO waveio; waveio.setFM();
        SDL_Surface * img = SDL_LoadBMP( "resources/images/standby640.bmp" );
        REQUIRE( img != nullptr );
        SDL_Surface * copy = Surfaceable::AllocateSurface( img );
        REQUIRE( copy != nullptr );

        size_t required_size = Waveable::conversion_size( img );
        REQUIRE(required_size == img->w * img->h);
        auto wave = new uint8_t[required_size];
        auto file = new uint8_t[required_size];

        transcoder.decode(wave, img);
        waveio.write("luma_standby640.wav", wave, required_size );
        waveio.read("luma_standby640.wav", file );
        transcoder.encode(copy, file);
        REQUIRE(memcmp(img->pixels, copy->pixels, required_size ) == 0);

        delete [] wave;
        delete [] file;
        SDL_FreeSurface( copy );
    }

}

#endif //SDL_CRT_FILTER_TRANSCODERTESTS_HPP
