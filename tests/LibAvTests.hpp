//
// Created by sebastian on 25/5/20.
//

#ifndef SDL_CRT_FILTER_LIBAVTESTS_HPP
#define SDL_CRT_FILTER_LIBAVTESTS_HPP
#include <transcoders/libAVable.hpp>

void test_codec( const std::string file_name, const std::string codec_name ) {
    SDL_Surface* still_image = SDL_ConvertSurfaceFormat(SDL_LoadBMP("resources/images/standby640.bmp" ), SDL_PIXELFORMAT_RGBA32, 0 );
    auto state = LibAVable::init_state( still_image, codec_name );
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    FILE* f = fopen( file_name.c_str() , "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", file_name.c_str() );
        exit(1);
    }

    /* encode 10 seconds of video */
    for (int i = 0; i < 250; i++) {
        fflush(stdout);

        if(i % 10  == 0 ) LibAVable::decode(state->frame, still_image );
        else LibAVable::dummy_image( i, *state->frame );

        state->frame->pts = i;
        /* encode the image */
        LibAVable::writefile( state, f );
    }

    /* flush the encoder */
    state->frame->pts++;
    LibAVable::writefile( state, f );

    /* add sequence end code to have a real MPEG file */
    if (state->codec->id == AV_CODEC_ID_MPEG1VIDEO || state->codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_free_context(&state->c);
    av_frame_free(&state->frame);
    av_packet_free(&state->pkt);
    //delete state;

}

TEST_CASE("LibAV tests","[LibAV]") {

    SECTION("Basic encode/decode tests") {
        auto surface = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                        SDL_PIXELFORMAT_RGBA32 , 0 );
        REQUIRE( surface != nullptr );
        auto copy = Surfaceable::AllocateSurface(surface);
        auto avframe = av_frame_alloc();
        avframe->format = AV_PIX_FMT_YUV420P;
        avframe->width = surface->w;
        avframe->height = surface->h;
        REQUIRE( av_frame_get_buffer( avframe, 0 ) >= 0 );
        LibAVable::decode( avframe, surface );
        LibAVable::encode( copy, avframe );
        auto ycbcr = SDL_ConvertSurfaceFormat( surface,
                                              SDL_PIXELFORMAT_RGBA32 , 0 );
        REQUIRE( ycbcr != nullptr );
        auto recover = Surfaceable::AllocateSurface( ycbcr );
        REQUIRE( recover != nullptr );

        const int yuv_len = MAX_YCBCR_SURFACE_SIZE(ycbcr->w, ycbcr->h, 0 );
        auto yuv = new Uint8[yuv_len];
        LibAVable::toYCbCr( yuv, ycbcr, SDL_PIXELFORMAT_YVYU );
        LibAVable::fromYCbCr( recover, yuv , SDL_PIXELFORMAT_YVYU );

        SDL_SaveBMP( surface, "libav_basic_frameraw.bmp" );
        SDL_SaveBMP( recover, "libav_basic_ycbcr.bmp" );
        SDL_SaveBMP( copy, "libav_basic_frameconv.bmp" );
        av_frame_free( &avframe );
        SDL_FreeSurface( surface );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( ycbcr );
        SDL_FreeSurface( recover );
        delete [] yuv;
    }

    SECTION ( "Color bleed test" ) {
        auto rgb = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        REQUIRE( rgb != nullptr );

        const int yuv_len = MAX_YCBCR_SURFACE_SIZE(rgb->w, rgb->h, 0 );
        auto yuv = new Uint8[yuv_len];

        for(int i = 30; i > 0; --i ) {
            LibAVable::toYCbCr(yuv, rgb, SDL_PIXELFORMAT_YVYU);
            LibAVable::fromYCbCr(rgb, yuv, SDL_PIXELFORMAT_YVYU);
        }

        SDL_SaveBMP(rgb, "libav_colorbleed.bmp");
        SDL_FreeSurface( rgb );
        delete[] yuv;
    }

    SECTION("RGB <-> YUV conversions") {
        struct {
            SDL_bool enable_intrinsics;
            int pattern_size;
            int extra_pitch;
        } automated_test_params[] = {
                /* Test: even width and height */
                { SDL_FALSE, 2, 0 },
                { SDL_FALSE, 4, 0 },
                /* Test: odd width and height */
                { SDL_FALSE, 1, 0 },
                { SDL_FALSE, 3, 0 },
                /* Test: even width and height, extra pitch */
                { SDL_FALSE, 2, 3 },
                { SDL_FALSE, 4, 3 },
                /* Test: odd width and height, extra pitch */
                { SDL_FALSE, 1, 3 },
                { SDL_FALSE, 3, 3 },
                /* Test: even width and height with intrinsics */
                { SDL_TRUE, 32, 0 },
                /* Test: odd width and height with intrinsics */
                { SDL_TRUE, 33, 0 },
                { SDL_TRUE, 37, 0 },
                /* Test: even width and height with intrinsics, extra pitch */
                { SDL_TRUE, 32, 3 },
                /* Test: odd width and height with intrinsics, extra pitch */
                { SDL_TRUE, 33, 3 },
                { SDL_TRUE, 37, 3 },
        };

        for (auto & automated_test_param : automated_test_params) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running automated test, pattern size %d, extra pitch %d, intrinsics %s\n",
                        automated_test_param.pattern_size,
                        automated_test_param.extra_pitch,
                        automated_test_param.enable_intrinsics ? "enabled" : "disabled");
            REQUIRE(LibAVable::test_ycbcr(automated_test_param.pattern_size, automated_test_param.extra_pitch) == 0);
        }
    }

    SECTION("Encode to file") {
        test_codec("test.mpg", "mpeg1video");
        test_codec("test_mpg2.mpg", "mpeg2video");
    }
}

#endif //SDL_CRT_FILTER_LIBAVTESTS_HPP
