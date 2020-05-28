//
// Created by sebastian on 22/5/20.
//

#ifndef SDL_CRT_FILTER_TRANSCODERTESTS_HPP
#define SDL_CRT_FILTER_TRANSCODERTESTS_HPP
#include <transcoders/Surfaceable.hpp>
#include <transcoders/Waveable.hpp>
#include <loaders/fmt_tools/WaveFile.hpp>
#include <transcoders/Magickable.hpp>

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

        REQUIRE(memcmp(img->pixels, copy->pixels, Waveable::conversion_size( img ) * sizeof(Uint32) ) == 0);
        REQUIRE(memcmp(img->pixels, resized->pixels, Waveable::conversion_size( img ) * sizeof(Uint32) ) == 0);

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
        REQUIRE(required_size == (size_t) img->w * img->h);
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
        REQUIRE(required_size == (size_t) img->w * img->h);
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

    SECTION("Pixelable idempotent RGB colorspace conversion check") {
        Uint32 pixel = 0, R = 0, G = 0, B = 0, A = 0;
        for( Uint32 r = 0; r <= 0xFF; r+=8 )
            for( Uint32 g = 0; g <= 0xFF; g+=8 )
                for( Uint32 b = 0; b <= 0xFF; b+=8 )
                    for( Uint32 a = 0; a <= 0xFF; a+=8 ) {
                        Pixelable::fromComponents( &pixel, &r , &g , &b, &a );
                        Pixelable::toComponents( &pixel, &R, &G, &B, &A );
                        REQUIRE( (r == R && g == G && b == B && a == A) );
                    }
    }

    SECTION("Magick colorspace <-> surface check") {
        SDL_Surface * test = SDL_LoadBMP( "resources/images/testCardRGB32.bmp" );
        SDL_Surface * img = Surfaceable::AllocateSurface( test );
        REQUIRE( img != nullptr );

        auto test_converted = SDL_ConvertSurfaceFormat(test, SDL_PIXELFORMAT_RGBA32 , 0 );
        SDL_SaveBMP( test_converted , "magick_direct_normalized_surface.bmp" );
        Image* image = Magickable::Allocate( test_converted );
        Magickable::decode( image, test_converted );
        image->write("magick_direct_decoded.bmp");
        Magickable::encode( img, image );

        SDL_SaveBMP( img , "magick_direct_to_sdl_test.bmp" );
        REQUIRE( memcmp( img->format, test_converted->format, sizeof(SDL_PixelFormat) ) == 0 );
        REQUIRE( memcmp( img->pixels, test_converted->pixels, Waveable::conversion_size( img ) * sizeof(Uint32) ) == 0);
        delete image;

        SDL_FreeSurface(img);
        SDL_FreeSurface(test_converted);
    }

    SECTION("Magick to surface and surface to Magick conversions") {
        SDL_Surface * bmp = SDL_LoadBMP( "resources/images/testCardRGB.bmp" );
        auto img = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_RGBA32, 0);
        REQUIRE( img != nullptr );
        SDL_Surface * copy = Surfaceable::AllocateSurface( img );
        REQUIRE( copy != nullptr );
        SDL_Surface * doubled = Surfaceable::AllocateSurface( img, 2 );
        REQUIRE( doubled != nullptr );
        SDL_Surface * thumb = Surfaceable::AllocateSurface( img, 0.5 );
        REQUIRE( thumb != nullptr );
        SDL_Surface * rescale = Surfaceable::AllocateSurface( img );
        REQUIRE( rescale != nullptr );

        Image* magick = Magickable::Allocate( img );
        Image* magick_doubled = Magickable::Allocate( img, 2 );
        Magickable::decode(magick, img);
        Magickable::encode(copy, magick);
        SDL_SaveBMP(img, "magic_testsurface_raw.bmp");
        SDL_SaveBMP(copy, "magic_recover.bmp");
        REQUIRE(memcmp(img->pixels, copy->pixels, Waveable::conversion_size( img ) * sizeof(Uint32)) == 0);

        Magickable::decode(magick, img);
        Magickable::encode(doubled, magick);
        SDL_SaveBMP(doubled, "magic_recover_doubled.bmp");

        Magickable::decode(magick_doubled, doubled);
        Magickable::encode(rescale, magick_doubled);
        SDL_SaveBMP(rescale, "magic_recover_rescaled.bmp");

        Magickable::blitScaled(thumb, img);
        SDL_SaveBMP(thumb, "magic_recover_halved.bmp");

        delete(magick);
        delete(magick_doubled);
        SDL_FreeSurface( copy );
        SDL_FreeSurface( rescale );
        SDL_FreeSurface( doubled );
        SDL_FreeSurface( thumb );
        SDL_FreeSurface( img );
    }


}

#endif //SDL_CRT_FILTER_TRANSCODERTESTS_HPP
