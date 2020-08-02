//
// Created by sebastian on 25/5/20.
//
#ifndef SDL_CRT_FILTER_LIBAVTESTS_HPP
#define SDL_CRT_FILTER_LIBAVTESTS_HPP
#include <transcoders/libAVable.hpp>
#include <BaseApp.hpp>

#define INBUF_SIZE 4096

void test_codec( const std::string file_name, const std::string codec_name ) {
    SDL_Surface* still_image = SDL_ConvertSurfaceFormat(SDL_LoadBMP("resources/images/standby640.bmp" ), SDL_PIXELFORMAT_RGBA32, 0 );
    auto state = LibAVable::init_state( still_image, codec_name );
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    FILE* f = fopen( file_name.c_str() , "wb");
    if (!f) {
        fprintf( stderr, "Could not open %s\n", file_name.c_str() );
        exit(1);
    }

    /* encode 10 seconds of video */
    for ( int i = 0; i < 250; i++ ) {
        fflush(stdout);

        if(i % 10  == 0 ) LibAVable::decode( state->frame, still_image );
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

    avcodec_free_context( &state->c );
    av_frame_free( &state->frame );
    av_packet_free( &state->pkt );
    //delete state;
    SDL_FreeSurface( still_image );
}

void test_decode( const std::string file_name, const std::string codec_name ) {
    auto state = LibAVable::init_state( codec_name, 1, 640, 480 );
    uint8_t *data;
    size_t   data_size;

    FILE* f = fopen( file_name.c_str() , "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", file_name.c_str() );
        exit(1);
    }

    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    while (!feof(f)) {
        fflush(stdout);
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            int ret = av_parser_parse2(state->parser, state->c, &state->pkt->data, &state->pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;

            if ( state->pkt->size )
                if ( LibAVable::readfile( state, f ) < 0 ) data_size = -1;
        }
    }

    auto recovered_surface = Surfaceable::AllocateSurface( state->frame->width , state->frame->height );
    LibAVable::encode( recovered_surface, state->frame );
    SDL_SaveBMP(recovered_surface, "libav_recovered_from_video.bmp");

    //flush
    state->pkt = nullptr;
    LibAVable::readfile( state, f );

    av_parser_close(state->parser);
    avcodec_free_context(&state->c);
    av_packet_free(&state->pkt);
    SDL_FreeSurface( recovered_surface );
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

    SECTION("Read from file") {
        test_decode("test.mpg", "mpeg1video");
        test_decode("test_mpg2.mpg", "mpeg2video");
    }

    SECTION("Log_conv idempotency ") {
        for(int i=100; i > 0; --i) {
            double luma = (double) 1/i;
            double log_luma = LibAVable::log_conv( luma );
            double rluma = LibAVable::log_unconv( log_luma );
            printf("luma: %lf, log_luma: %lf, rluma: %lf\n", luma, log_luma, rluma );
            REQUIRE ( abs(luma - rluma) < 0.01 );
        }

        for(int i=10; i >= -10; --i) {
            double luma = i != 0?  (double) 1/i : 0;
            double log_luma = LibAVable::log_conv( abs(luma) );
            double pack_logluma = LibAVable::pack_logluma( luma );
            Uint32 pixel = Pixelable::direct_from_luma( pack_logluma ) ;
            double pixel_luma = Pixelable::direct_to_luma( pixel );
            double unlog_luma = LibAVable::pack_unlogluma( pixel_luma );
            printf("pixel: %d px_luma: %lf\n", pixel & 0xFF, pixel_luma );
            printf("luma: %lf, unlog_luma: %lf, log_conv %lf, pack_logluma: %lf\n", luma, unlog_luma, log_luma, pack_logluma );
            REQUIRE ( abs(luma - unlog_luma) < 0.01 );
        }

    }

    SECTION("Pack interlace") {
        auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto interlaced = Surfaceable::AllocateSurface( frame );
        auto deinterlaced = Surfaceable::AllocateSurface( frame );

        REQUIRE( frame != nullptr );
        REQUIRE( interlaced != nullptr );
        REQUIRE( deinterlaced != nullptr );


        LibAVable::pack_doubleinterlace( interlaced, frame );
        LibAVable::pack_doubledeinterlace( deinterlaced, interlaced );

        SDL_SaveBMP( interlaced, "libav_pack_interlaced_test.bmp");
        SDL_SaveBMP( deinterlaced, "libav_pack_deinterlaced_test.bmp");

        SDL_FreeSurface( deinterlaced );
        SDL_FreeSurface( interlaced );
        SDL_FreeSurface( frame );
    }

    SECTION("Spiral scan pattern test ") {
        auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/standby640.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto copy = Surfaceable::AllocateSurface( frame );
        auto recover = Surfaceable::AllocateSurface( frame );
        LibAVable::pack_spiral( copy, frame, 40 );
        LibAVable::pack_despiral( recover, copy, 40 );
        SDL_SaveBMP( copy,  "libav_pack_spiral.bmp" );
        SDL_SaveBMP( recover,  "libav_pack_despiral.bmp" );

        REQUIRE(Loader::CompareSurface(frame, recover));
        SDL_FreeSurface(frame);
        SDL_FreeSurface(copy);
        SDL_FreeSurface(recover);
    }

    SECTION("Miniraster scan pattern test ") {
        auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto copy = Surfaceable::AllocateSurface( frame );
        auto recover = Surfaceable::AllocateSurface( frame );
        LibAVable::pack_miniraster( copy, frame, 40 );
        LibAVable::unpack_miniraster( recover, copy, 40 );
        SDL_SaveBMP( copy,  "libav_pack_miniraster.bmp" );
        SDL_SaveBMP( recover,  "libav_unpack_miniraster.bmp" );

        REQUIRE(Loader::CompareSurface(frame, recover));
        SDL_FreeSurface(frame);
        SDL_FreeSurface(copy);
        SDL_FreeSurface(recover);
    }

    SECTION("Minizigzag scan pattern test ") {
        auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto copy = Surfaceable::AllocateSurface( frame );
        auto recover = Surfaceable::AllocateSurface( frame );
        LibAVable::pack_minizigzag( copy, frame, 40 );
        LibAVable::unpack_minizigzag( recover, copy, 40 );
        SDL_SaveBMP( copy,  "libav_pack_minizigzag.bmp" );
        SDL_SaveBMP( recover,  "libav_unpack_minizigzag.bmp" );

        REQUIRE(Loader::CompareSurface(frame, recover));
        SDL_FreeSurface(frame);
        SDL_FreeSurface(copy);
        SDL_FreeSurface(recover);
    }

    SECTION("Diagonal scan pattern test ") {
        auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto copy = Surfaceable::AllocateSurface( frame );
        auto recover = Surfaceable::AllocateSurface( frame );
        LibAVable::pack_diagonal(copy, frame, 40);
        LibAVable::unpack_diagonal( recover, copy, 40 );
        SDL_SaveBMP( copy,  "libav_pack_diagonal.bmp" );
        SDL_SaveBMP( recover,  "libav_unpack_diagonal.bmp" );

        REQUIRE(Loader::CompareSurface(frame, recover));
        SDL_FreeSurface(frame);
        SDL_FreeSurface(copy);
        SDL_FreeSurface(recover);
    }

    /* both commented due invalidread/write in valgrind
           SECTION("Mirrored diagonal scan pattern test ") {
               auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                      SDL_PIXELFORMAT_RGBA32 , 0 );
               auto copy = Surfaceable::AllocateSurface( frame );
               auto recover = Surfaceable::AllocateSurface( frame );
               LibAVable::pack_diagonal_mirror(copy, frame, 40);
               LibAVable::unpack_diagonal_mirror( recover, copy, 40 );
               SDL_SaveBMP( copy,  "libav_pack_diagonal_mirror.bmp" );
               SDL_SaveBMP( recover,  "libav_unpack_diagonal_mirror.bmp" );

               REQUIRE(Loader::CompareSurface(frame, recover));
               SDL_FreeSurface(frame);
               SDL_FreeSurface(copy);
               SDL_FreeSurface(recover);
           }

          SECTION("Flipped diagonal scan pattern test ") {
               auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                      SDL_PIXELFORMAT_RGBA32 , 0 );
               auto copy = Surfaceable::AllocateSurface( frame );
               auto recover = Surfaceable::AllocateSurface( frame );
               LibAVable::pack_diagonal_flip( copy, frame, 40);
               LibAVable::unpack_diagonal_flip( recover, copy, 40 );
               SDL_SaveBMP( copy,  "libav_pack_diagonal_flip.bmp" );
               SDL_SaveBMP( recover,  "libav_unpack_diagonal_flip.bmp" );

               REQUIRE(Loader::CompareSurface(frame, recover));
               SDL_FreeSurface(frame);
               SDL_FreeSurface(copy);
               SDL_FreeSurface(recover);
           }
       */
    SECTION("All scan patterns test ") {
        auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto copy = Surfaceable::AllocateSurface( frame );
        auto recover = Surfaceable::AllocateSurface( frame );
        LibAVable::pack_all( copy, frame, 40 );
        LibAVable::unpack_all( recover, copy, 40 );
        SDL_SaveBMP( copy,  "libav_pack_all.bmp" );
        SDL_SaveBMP( recover,  "libav_unpack_all.bmp" );

        REQUIRE(Loader::CompareSurface(frame, recover));
        SDL_FreeSurface(frame);
        SDL_FreeSurface(copy);
        SDL_FreeSurface(recover);
    }

    SECTION("All recursive scan patterns test ") {
        auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto copy = Surfaceable::AllocateSurface( frame );
        auto recover = Surfaceable::AllocateSurface( frame );
        LibAVable::pack_all_recursive( copy, frame, 40 );
        LibAVable::unpack_all_recursive( recover, copy, 40 );
        SDL_SaveBMP( copy,  "libav_pack_all_recursive.bmp" );
        SDL_SaveBMP( recover,  "libav_unpack_all_recursive.bmp" );

        REQUIRE(Loader::CompareSurface(frame, recover));
        SDL_FreeSurface(frame);
        SDL_FreeSurface(copy);
        SDL_FreeSurface(recover);
    }

    SECTION("Hypersurface stack test ") {
        auto frame = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto step = 40;
        int depth = frame->w / step;
        auto blank = Surfaceable::AllocateSurface( frame );
        Loader::blank( blank );
        auto hs = new LibAVable_hypersurface_t();
        auto hc = new LibAVable_hypersurface_t();
        SDL_Surface* copy = nullptr;
        hs->step = step;
        hs->depth = depth;
        for ( int i=0; i <= depth + 1; ++i ) {
            copy = LibAVable::hs_stack( hs, hc, frame );
            if ( i <= depth )
                REQUIRE( Loader::CompareSurface( copy, blank ) );
            else {
                SDL_SaveBMP( copy,  "libav_hs_stack_out.bmp" );
                REQUIRE( Loader::CompareSurface( copy, frame ) );
            }
            SDL_FreeSurface(copy);
        }

        SDL_FreeSurface( frame );
        SDL_FreeSurface( blank );
        LibAVable::hs_free( hs );
        LibAVable::hs_free( hc );
        delete hs;
        delete hc;
    }

    SECTION("Hypersurface transpose test ") {
        auto frame = Surfaceable::AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
        auto image = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                               SDL_PIXELFORMAT_RGBA32 , 0 );
        auto step = 40;
        LibAVable::macroblock_test( frame, 0, step );
        SDL_SaveBMP( frame,  "libav_macrotest.bmp" );
        size_t depth = frame->w / step;
        auto blank = Surfaceable::AllocateSurface( frame );
        Loader::blank( blank );
        auto hs = new LibAVable_hypersurface_t();
        auto hc = new LibAVable_hypersurface_t();
        auto his = new LibAVable_hypersurface_t();
        auto hic = new LibAVable_hypersurface_t();
        SDL_Surface* copy = nullptr;
        hs->step = step;
        hs->depth = depth;
        his->step = step;
        his->depth = depth;
        while ( hc->surfaces.size() == 0 ) {
            copy = LibAVable::hs_stack( hs, hc, frame );
            SDL_FreeSurface(copy);
            copy = LibAVable::hs_stack( his, hic, image );
            SDL_FreeSurface(copy);
        }
        REQUIRE( hc->surfaces.size() == depth );
        LibAVable::hs_free( hs );
        LibAVable::hs_transpose(hc, hs, 0);

        int i = 0;
        for ( auto & surface: hs->surfaces ) {
            char buff[100] = { 0 };
            snprintf(buff, sizeof(buff), "libav_hs_stack_out-%02d.bmp", i);
            SDL_SaveBMP( surface, buff );
            ++i;
        }

        LibAVable::hs_free( his );
        LibAVable::hs_transpose( hic, his );
        for ( auto & surface: his->surfaces ) {
            char buff[100] = { 0 };
            snprintf(buff, sizeof(buff), "libav_hs_stack_image_out-%02d.bmp", i);
            SDL_SaveBMP( surface, buff );
            ++i;
        }

        LibAVable::hs_free( hic );
        LibAVable::hs_untranspose( his, hic );

        for ( auto & surface: hic->surfaces ) {
            char buff[100] = { 0 };
            snprintf(buff, sizeof(buff), "libav_hs_unstack_image_out-%02d.bmp", i);
            SDL_SaveBMP( surface, buff );
            ++i;
        }

        SDL_FreeSurface( frame );
        SDL_FreeSurface( blank );
        LibAVable::hs_free( hs );
        LibAVable::hs_free( hc );
        LibAVable::hs_free( his );
        LibAVable::hs_free( hic );
        delete hs;
        delete hc;
        delete hic;
        delete his;
    }

}

struct observation_t {
    unsigned long module;
    float comp_factor;
    std::string sha256;
};

static bool obs_exists ( std::list<observation_t> &observations, std::string& sha ) {
    auto match = std::find_if(observations.cbegin(), observations.cend(), [&sha] (const observation_t& s) {
        return s.sha256 == sha;
    });
    return match->sha256 == sha;
}


static void dummy_module_search(std::list<observation_t> &observations, std::string name, void (*funct)(SDL_Surface*, SDL_Surface*, int), SDL_Surface* image ) {
    LazyLoader loader;
    BaseApp app(loader);
    app.Standby();
    auto frame = Surfaceable::AllocateSurface( image );
    auto copy = Surfaceable::AllocateSurface( image );
    auto step = 40;
    Magickable::blitScaled( copy, image );
    unsigned long module=1;
    float min_ratio = 1, max_ratio=0;
    char fname[100];
    bool abort = false;
    while ( !Loader::CompareSurface( image, frame ) && !abort ) {
        funct( frame, copy, step );
        Magickable::blitScaled( copy, frame );
        auto ratio = LibAVable::compressibility( frame );

        if ( max_ratio < ratio ) max_ratio = ratio;
        if ( ratio <= min_ratio ) {
            SDL_Log("Found good compression at iteration %lu: Last ratio: %02f -> new: %02f", module, min_ratio, ratio );
            min_ratio = ratio;
            app.publish(frame);
            std::sprintf( fname , "libav_test-%s-%lu", name.c_str(), module);
            SDL_SaveBMP(frame, fname);
        } else if( module % 100 == 0 ) {
            SDL_Log( "%s, iteration %d, current_ratio: %02f (%02f min ~ %02f max)", name.c_str(), module, ratio, min_ratio, max_ratio );
            app.publish(frame);
        }
        auto sha256 = Loader::sha256Log((uint8_t*) frame->pixels, Pixelable::pixels( frame ) );
        observation_t reg = {
            module,
            ratio,
            sha256
        };
        if (! obs_exists( observations , sha256 ) ) {
            observations.push_back(reg);
        } else {
            SDL_Log( "Repeated observation at %d", module );
            abort = true;
        }
        module++;
    }

    SDL_FreeSurface(frame);
    SDL_FreeSurface(copy);
}


static void compander_search(std::list<observation_t> &observations, std::string name, void (*comp)(SDL_Surface*, SDL_Surface*, int), void (*expand)(SDL_Surface*, SDL_Surface*, int), SDL_Surface* image ) {
    LazyLoader loader;
    BaseApp app(loader);
    app.Standby();
    auto frame = Surfaceable::AllocateSurface( image );
    auto copy = Surfaceable::AllocateSurface( image );
    auto step = 16;
    Magickable::blitScaled( copy, image );
    unsigned long module=1;
    float min_ratio = 100, max_ratio=0;
    char fname[100];
    bool abort = false;
    while ( !Loader::CompareSurface( image, frame ) && !abort ) {
        comp( frame, copy, step );
        expand( copy, frame, step );
        Magickable::flip_vertical( frame, copy );
        Magickable::blitScaled( copy, frame );
        auto ratio = Pixelable::psnr( image, frame );

        if ( ratio < min_ratio ) min_ratio = ratio;
        if ( ratio > max_ratio ) {
            SDL_Log("Found good pnsr at iteration %lu: Last ratio: %02f -> new: %02f", module, min_ratio, ratio );
            max_ratio = ratio;
            app.publish(frame);
            std::sprintf( fname , "libav_test-%s-%lu", name.c_str(), module);
            SDL_SaveBMP(frame, fname);
        } else if( module % 100 == 0 ) {
            SDL_Log( "%s, iteration %d, current_ratio: %02f (%02f min ~ %02f max)", name.c_str(), module, ratio, min_ratio, max_ratio );
            app.publish(frame);
        }
        auto sha256 = Loader::sha256Log((uint8_t*) frame->pixels, Pixelable::pixels( frame ) );
        observation_t reg = {
                module,
                ratio,
                sha256
        };
        if (! obs_exists( observations , sha256 ) ) {
            observations.push_back(reg);
        } else {
            SDL_Log( "Repeated observation at %d", module );
            abort = true;
        }
        module++;
    }

    SDL_FreeSurface(frame);
    SDL_FreeSurface(copy);
}




typedef observation_t obs_minmax_t[2];

static void minmax_obs ( obs_minmax_t& dst, std::list<observation_t>& observations) {

    float min = 1;
    float max = 0;
    for (auto & observation : observations) {
        if (observation.comp_factor < min) {
            min = observation.comp_factor;
            dst[0].comp_factor = observation.comp_factor;
            dst[0].module = observation.module;
            dst[0].sha256 = observation.sha256;
        }
        if (observation.comp_factor > max) {
            max = observation.comp_factor;
            dst[1].comp_factor = observation.comp_factor;
            dst[1].module = observation.module;
            dst[1].sha256 = observation.sha256;
        }
    }
}

static void display_observation(std::string name, std::list<observation_t> &observations ) {
    SDL_Log("%s Found function module: %lu", name.c_str(), observations.end()->module );
    obs_minmax_t eval;
    minmax_obs(eval, observations);
    SDL_Log("%s MAX ratio: %02f", name.c_str(), eval[0].comp_factor );
    SDL_Log("%s MAX iteration: %lu", name.c_str(), eval[0].module );
    SDL_Log("%s MAX sha256: %s", name.c_str(), eval[0].sha256.c_str() );
    SDL_Log("%s MIN ratio: %02f", name.c_str(), eval[1].comp_factor );
    SDL_Log("%s MIN iteration: %lu", name.c_str(), eval[1].module );
    SDL_Log("%s MIN sha256: %s", name.c_str(), eval[1].sha256.c_str() );
}


static void observe(std::string name, void (*funct)(SDL_Surface*, SDL_Surface*, int), SDL_Surface* image) {
    std::list<observation_t> observations;

    SDL_Log("Observing %s() repetition module", name.c_str());
    dummy_module_search( observations, name, funct, image);
    display_observation( name, observations );
    observations.clear();

}

static void observe(std::string name, void (*comp)(SDL_Surface*, SDL_Surface*, int), void (*expand)(SDL_Surface*, SDL_Surface*, int),  SDL_Surface* image) {
    std::list<observation_t> observations;

    SDL_Log("Observing %s() repetition module", name.c_str());
    compander_search( observations, name, comp, expand, image);
    display_observation( name, observations );
    observations.clear();

}


TEST_CASE("LibAV, Observe a function's repetition module", "[LibAV]") {

    auto image = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                           SDL_PIXELFORMAT_RGBA32 , 0 );


    //observe( "miniraster & minizigzag", &LibAVable::pack_miniraster, &LibAVable::pack_minizigzag, image);
    //observe( "spiral & miniraster", &LibAVable::pack_spiral, &LibAVable::pack_miniraster, image);
    //observe( "pack_spiral & diagonal", &LibAVable::pack_spiral, &LibAVable::pack_diagonal, image);


    SDL_FreeSurface(image);
}

#endif //SDL_CRT_FILTER_LIBAVTESTS_HPP
