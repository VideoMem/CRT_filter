//
// Created by sebastian on 25/5/20.
//

#ifndef SDL_CRT_FILTER_LIBAVTESTS_HPP
#define SDL_CRT_FILTER_LIBAVTESTS_HPP
#include <transcoders/libAVable.hpp>

int test_mpeg(const std::string file_name, const std::string codec_name ) {
    const char *filename;
    AVCodec *codec = nullptr;
    AVCodecContext *c= nullptr;
    AVFrame *frame;
    AVPacket *pkt;

    int i, ret;
    FILE *f;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    filename = file_name.c_str();
    LibAVable::init_codec( codec, c, codec_name );

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    SDL_Surface* still = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"), SDL_PIXELFORMAT_RGBA32, 0 );
    //SDL_Surface* still = SDL_LoadBMP("resources/images/standby640.bmp");

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = still->w;
    c->height = still->h;
    /* frames per second */
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    ret = avcodec_open2(c, codec, nullptr );
    if (ret < 0) {
        std::string err("Commented error");
        //std::string err(av_err2str(ret));
        fprintf(stderr, "Could not open codec: %s\n", err.c_str() );
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    /* encode 1 second of video */
    /* make sure the frame data is writable */
    ret = av_frame_make_writable(frame);
    if (ret < 0)
        exit(1);

    LibAVable::decode(frame, still);

    for (i = 0; i < 250; i++) {
        fflush(stdout);

        //LibAVable::dummy_image( i, *frame, *c );
        frame->pts = i;

        /* encode the image */
        LibAVable::encode(c, frame, pkt, f);
    }

    /* flush the encoder */
    LibAVable::encode(c, nullptr, pkt, f);

    /* add sequence end code to have a real MPEG file */
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}

TEST_CASE("LibAV tests","[LibAV]") {
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
            REQUIRE(LibAVable::test_yuv(automated_test_param.pattern_size, automated_test_param.extra_pitch) == 0);
        }
    }

    SECTION("Encode to file") {
       // LibAVable::test_yuv(2,0);
        test_mpeg( "test.mpg", "mpeg1video" );
    }
}

#endif //SDL_CRT_FILTER_LIBAVTESTS_HPP
