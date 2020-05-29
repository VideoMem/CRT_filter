//
// Created by sebastian on 25/5/20.
//

#ifndef SDL_CRT_FILTER_LIBAVABLE_HPP
#define SDL_CRT_FILTER_LIBAVABLE_HPP

#include <cstdlib>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include "transcoders/helpers/testyuv_cvt.c"
}
#include "transcoders/Magickable.hpp"

/* 422 (YUY2, etc) formats are the largest */
#define MAX_YCBCR_SURFACE_SIZE(W, H, P)  (H*4*(W+P+1)/2)

class LibAVable: public Magickable {
public:
    struct AVthings_t {
        AVCodec *codec;
        AVCodecContext *c;
        AVFrame *frame;
        AVPacket *pkt;
        AVCodecParserContext *parser;
    };

    static void encode(void* dst, void* src);
    static void decode(void* dst, void* src);

    static LibAVable::AVthings_t *init_state(SDL_Surface* reference, std::string codec_name) {
        return init_state(codec_name, 0, reference->w, reference->h );
    }
    static AVthings_t *
    init_state(std::string codec_name, int decode_flag = 0, int x = 320, int y = 240, int framerate = 25, int bitrate = 800000);
    static size_t readfile(LibAVable::AVthings_t *state, FILE *infile);
    static void writefile(LibAVable::AVthings_t *state, FILE *outfile) {
        int ret;

        /* send the frame to the encoder */
        if ( state->frame )
            printf("Send frame %3" PRId64"\n", state->frame->pts );

        ret = avcodec_send_frame( state->c, state->frame );
        if (ret < 0) {
            fprintf(stderr, "Error sending a frame for encoding\n");
            exit(1);
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet( state->c, state->pkt );
            if ( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                return;
            else if ( ret < 0 ) {
                fprintf( stderr, "Error during encoding\n" );
                exit(1 );
            }

            printf("Write packet %3" PRId64" (size=%5d)\n", state->pkt->pts, state->pkt->size );
            fwrite( state->pkt->data, 1, state->pkt->size, outfile );
            av_packet_unref( state->pkt );
        }
    }

public:

    static void init_codec( AVthings_t *state, const std::string codec_name ) {
        avcodec_register_all();

        /* find the encoder */
        state->codec = avcodec_find_encoder_by_name( codec_name.c_str() );
        if ( !state->codec ) {
            fprintf(stderr, "Codec '%s' not found\n", codec_name.c_str());
            exit(1);
        }

        state->c = avcodec_alloc_context3( state->codec );
        if ( !state->c ) {
            fprintf(stderr, "Could not allocate video codec context\n");
            exit(1);
        }

    }


    static void init_decode( AVthings_t *state, const std::string codec_name ) {
        avcodec_register_all();

        /* find the encoder */
        state->codec = avcodec_find_decoder_by_name( codec_name.c_str() );
        if ( !state->codec ) {
            fprintf(stderr, "Codec '%s' not found\n", codec_name.c_str());
            exit(1);
        }

        state->c = avcodec_alloc_context3( state->codec );
        if ( !state->c ) {
            fprintf(stderr, "Could not allocate video codec context\n");
            exit(1);
        }

    }



    static void dummy_image(int idx, AVFrame& frame) {
        /* prepare a dummy image */
        /* Y */
        for ( int y = 0; y < frame.height; y++ ) {
            for ( int x = 0; x < frame.width; x++ ) {
                frame.data[0][y * frame.linesize[0] + x] = x + y + idx * 3;
            }
        }

        /* Cb and Cr */
        for ( int y = 0; y < frame.height/2; y++ ) {
            for ( int x = 0; x < frame.width/2; x++ ) {
                frame.data[1][y * frame.linesize[1] + x] = 128 + y + idx * 2;
                frame.data[2][y * frame.linesize[2] + x] = 64 + x + idx * 5;
            }
        }

        frame.pts = idx;
    }

    static int test_ycbcr(int pattern_size, int extra_pitch);
    static SDL_Surface* generate_test_pattern(int pattern_size);

    static SDL_bool verify_ycbcr_data(Uint32 format, const Uint8 *yuv, int yuv_pitch, SDL_Surface *surface);

    static SDL_bool is_packed_ycbcr_format(Uint32 format);

    //from RGB32 to YCrCb
    static void toYCbCr(uint8_t *yuv, SDL_Surface *pattern, int format);
    static void toFrame(AVFrame *frame, const Uint8 *ycbcr);
    static void fromYCbCr(SDL_Surface *pattern, uint8_t *yuv, int format);
    static void fromFrame(Uint8 *ycbcr, AVFrame *frame );

};

void LibAVable::fromFrame(Uint8 *ycbcr, AVFrame *frame ) {
    int fid = 0, cid = 0, yid = 0, nyid = 0;

    //YVYU Luminance
    for (int y = 0; y < frame->height ; y++) {
        for (int x = 0; x < frame->width; x++) {
            fid = y * frame->linesize[0] + x;
            ycbcr[ fid * 2 ] = frame->data[0][ fid ];
        }
    }

    /* Cb and Cr */
    for (int y = 0; y < frame->height/2; y++) {
        for (int x = 0; x < frame->width/2; x++) {
            cid = y * frame->linesize[1] + x;
            yid  = 4 * ( 2 * y * frame->linesize[1] + x );
            nyid = round(4 * ( ( 2 * y - 1 ) * frame->linesize[1] + x ));
            ycbcr[ yid + 3 ] = frame->data[1][ cid ]; // Cb <-> U;
            ycbcr[ yid + 1 ] = frame->data[2][ cid ]; // Cr <-> V;
            if( y > 0 ) {
                ycbcr[nyid + 3] = frame->data[1][cid]; // Cb <-> U;
                ycbcr[nyid + 1] = frame->data[2][cid]; // Cr <-> V;
            }
        }
    }

    //last line
    int y = (frame->height/2) - 1;
    for (int x = 0; x < frame->width/2; x++) {
        nyid = round(4 * ( ( 2 * y + 1 ) * frame->linesize[1] + x ));
        cid = y * frame->linesize[1] + x;
        ycbcr[nyid + 3] = frame->data[1][cid]; // Cb <-> U;
        ycbcr[nyid + 1] = frame->data[2][cid]; // Cr <-> V;
    }
}

void LibAVable::toFrame(AVFrame *frame, const Uint8 *ycbcr ) {
    int fid = 0, cid = 0, yid = 0, nyid = 0;;

    //YVYU Luminance
    for (int y = 0; y < frame->height ; y++) {
        for (int x = 0; x < frame->width; x++) {
            fid = y * frame->linesize[0] + x;
            frame->data[0][ fid ] = ycbcr[ fid * 2 ];
        }
    }

    /* Cb and Cr */
    for (int y = 0; y < frame->height/2; y++) {
        for (int x = 0; x < frame->width/2; x++) {
            cid = y * frame->linesize[1] + x;
            yid = 4 * ( 2 * y * frame->linesize[1] + x );
            nyid = round(4 * ( ( 2 * y - 1 ) * frame->linesize[1] + x ));
            frame->data[1][ cid ] = y == 0? ycbcr[ yid + 3 ]: (ycbcr[ yid + 3 ] + ycbcr[ nyid + 3 ]) / 2 ; // Cb <-> U;
            frame->data[2][ cid ] = y == 0? ycbcr[ yid + 1 ]: (ycbcr[ yid + 1 ] + ycbcr[ nyid + 1 ]) / 2 ; // Cr <-> V;
        }
    }
}

void LibAVable::decode(void *dst, void *src) {
    auto source_frame = static_cast<SDL_Surface*>(src);
    auto dst_frame = static_cast<AVFrame*>(dst);
    const int yuv_len = MAX_YCBCR_SURFACE_SIZE(source_frame->w, source_frame->h, 0);
    auto yuv = new Uint8[yuv_len];

    auto format = SDL_PIXELFORMAT_YVYU;

    toYCbCr(yuv, source_frame, format);
    toFrame(dst_frame, yuv);

    delete [] yuv;
}

void LibAVable::encode(void *dst, void *src) {
    auto source_frame = static_cast<AVFrame*>(src);
    auto dst_frame = static_cast<SDL_Surface*>(dst);
    const int yuv_len = MAX_YCBCR_SURFACE_SIZE(dst_frame->w, dst_frame->h, 0);
    auto yuv = new Uint8[yuv_len];

    auto format = SDL_PIXELFORMAT_YVYU;

    fromFrame( yuv, source_frame );
    fromYCbCr(dst_frame, yuv, format);

    delete [] yuv;
}


void LibAVable::toYCbCr(uint8_t *yuv, SDL_Surface *pattern, int format) {
    auto yuv_pitch = CalculateYCbCrPitch(format, pattern->w);
    if (SDL_ConvertPixels(pattern->w, pattern->h, pattern->format->format, pattern->pixels, pattern->pitch, format, yuv, yuv_pitch) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(pattern->format->format), SDL_GetPixelFormatName(format), SDL_GetError());
        assert(false && "Error during format conversion");
    }
}

void LibAVable::fromYCbCr(SDL_Surface *pattern, uint8_t *yuv, int format) {
    auto yuv_pitch = CalculateYCbCrPitch(format, pattern->w);
    if (SDL_ConvertPixels(pattern->w, pattern->h, format, yuv, yuv_pitch, pattern->format->format, pattern->pixels, pattern->pitch ) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(pattern->format->format), SDL_GetPixelFormatName(format), SDL_GetError());
        assert(false && "Error during format conversion");
    }
}


int LibAVable::test_ycbcr(int pattern_size, int extra_pitch) {
    const Uint32 formats[] = {
            SDL_PIXELFORMAT_YV12,
            SDL_PIXELFORMAT_IYUV,
            SDL_PIXELFORMAT_NV12,
            SDL_PIXELFORMAT_NV21,
            SDL_PIXELFORMAT_YUY2,
            SDL_PIXELFORMAT_UYVY,
            SDL_PIXELFORMAT_YVYU
    };
    int i,j;
    SDL_Surface *pattern = generate_test_pattern(pattern_size);
    const int yuv_len = MAX_YCBCR_SURFACE_SIZE(pattern->w, pattern->h, extra_pitch);

    auto yuv1 = new Uint8[yuv_len];
    auto yuv2 = new Uint8[yuv_len];
    int yuv1_pitch, yuv2_pitch;
    int result = 0;

    if ( pattern == nullptr || pattern->format == nullptr ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't allocate test surfaces");
        assert(false);
    }

    /* Verify conversion from YUV formats */
    for (i = 0; i < (int) SDL_arraysize(formats); ++i) {
        if (!ConvertRGBtoYUV(formats[i], (uint8_t*) pattern->pixels, pattern->pitch, yuv1, pattern->w, pattern->h, SDL_GetYUVConversionModeForResolution(pattern->w, pattern->h), 0, 100)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ConvertRGBtoYUV() doesn't support converting to %s\n", SDL_GetPixelFormatName(formats[i]));
            result = -1;
            break;
        }
        yuv1_pitch = CalculateYCbCrPitch(formats[i], pattern->w);
        if (!verify_ycbcr_data(formats[i], yuv1, yuv1_pitch, pattern)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed conversion from %s to RGB\n", SDL_GetPixelFormatName(formats[i]));
            result = -1;
            break;
        }
    }

    /* Verify conversion to YUV (YCbCr) formats */
    for (i = 0; i < (int) SDL_arraysize(formats); ++i) {
        yuv1_pitch = CalculateYCbCrPitch(formats[i], pattern->w) + extra_pitch;
        if (SDL_ConvertPixels(pattern->w, pattern->h, pattern->format->format, pattern->pixels, pattern->pitch, formats[i], yuv1, yuv1_pitch) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(pattern->format->format), SDL_GetPixelFormatName(formats[i]), SDL_GetError());
            result = -1;
            break;
        }
        if (!verify_ycbcr_data(formats[i], yuv1, yuv1_pitch, pattern)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed conversion from RGB to %s\n", SDL_GetPixelFormatName(formats[i]));
            result = -1;
            break;
        }
    }


    /* Verify conversion between YUV formats */
    for (i = 0; i < (int) SDL_arraysize(formats); ++i) {
        for (j = 0; j < (int) SDL_arraysize(formats); ++j) {
            yuv1_pitch = CalculateYCbCrPitch(formats[i], pattern->w) + extra_pitch;
            yuv2_pitch = CalculateYCbCrPitch(formats[j], pattern->w) + extra_pitch;
            if (SDL_ConvertPixels(pattern->w, pattern->h, pattern->format->format, pattern->pixels, pattern->pitch, formats[i], yuv1, yuv1_pitch) < 0) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(pattern->format->format), SDL_GetPixelFormatName(formats[i]), SDL_GetError());
                result = -1;
                break;
            }
            if (SDL_ConvertPixels(pattern->w, pattern->h, formats[i], yuv1, yuv1_pitch, formats[j], yuv2, yuv2_pitch) < 0) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(formats[i]), SDL_GetPixelFormatName(formats[j]), SDL_GetError());
                result = -1;
                break;
            }
            if (!verify_ycbcr_data(formats[j], yuv2, yuv2_pitch, pattern)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed conversion from %s to %s\n", SDL_GetPixelFormatName(formats[i]), SDL_GetPixelFormatName(formats[j]));
                result = -1;
                break;
            }
        }
    }

    /* Verify conversion between YUV formats in-place */
    for (i = 0; i < (int) SDL_arraysize(formats); ++i) {
        for (j = 0; j < (int) SDL_arraysize(formats); ++j) {
            if (is_packed_ycbcr_format(formats[i]) != is_packed_ycbcr_format(formats[j])) {
                // Can't change plane vs packed pixel layout in-place
                continue;
            }

            yuv1_pitch = CalculateYCbCrPitch(formats[i], pattern->w) + extra_pitch;
            yuv2_pitch = CalculateYCbCrPitch(formats[j], pattern->w) + extra_pitch;
            if (SDL_ConvertPixels(pattern->w, pattern->h, pattern->format->format, pattern->pixels, pattern->pitch, formats[i], yuv1, yuv1_pitch) < 0) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(pattern->format->format), SDL_GetPixelFormatName(formats[i]), SDL_GetError());
                 result = -1;
                 break;
            }
            if (SDL_ConvertPixels(pattern->w, pattern->h, formats[i], yuv1, yuv1_pitch, formats[j], yuv1, yuv2_pitch) < 0) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(formats[i]), SDL_GetPixelFormatName(formats[j]), SDL_GetError());
                result = -1;
                break;
            }
            if (!verify_ycbcr_data(formats[j], yuv1, yuv2_pitch, pattern)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed conversion from %s to %s\n", SDL_GetPixelFormatName(formats[i]), SDL_GetPixelFormatName(formats[j]));
                result = -1;
                break;
            }
        }
    }

    if( pattern->format != nullptr ) {
        SDL_SaveBMP(pattern, "testyuv.bmp");
        SDL_FreeSurface(pattern);
        SDL_Log("Test passed");
    } else {
        SDL_Log("Test failed!");
    }
    delete [] yuv1;
    delete [] yuv2;
    return result;
}

/* Return true if the YUV format is packed pixels */
SDL_bool LibAVable::is_packed_ycbcr_format(Uint32 format) {
    return static_cast<SDL_bool>(format == SDL_PIXELFORMAT_YUY2 ||
                                 format == SDL_PIXELFORMAT_UYVY ||
                                 format == SDL_PIXELFORMAT_YVYU);
}

SDL_Surface *LibAVable::generate_test_pattern(int pattern_size) {
    SDL_Surface *pattern = SDL_CreateRGBSurfaceWithFormat(0, pattern_size, pattern_size, 0, SDL_PIXELFORMAT_RGB24 );

    if (pattern != nullptr && pattern->format != nullptr ) {
        int i, x, y;
        Uint8 *p, c;
        const int thickness = 2;    /* Important so 2x2 blocks of color are the same, to avoid Cr/Cb interpolation over pixels */

        /* R, G, B in alternating horizontal bands */
        for (y = 0; y + thickness < pattern->h; y += thickness) {
            for (i = 0; i < thickness; ++i) {
                p = (Uint8 *)pattern->pixels + (y + i) * pattern->pitch + ((y/thickness) % 3);
                for (x = 0; x < pattern->w; ++x) {
                    *p = 0xFF;
                    p += 3;
                }
            }
        }

        /* Black and white in alternating vertical bands */
        c = 0xFF;
        for (x = 1*thickness; x < pattern->w; x += 2*thickness) {
            for (i = 0; i < thickness; ++i) {
                p = (Uint8 *)pattern->pixels + (x + i)*3;
                for (y = 0; y < pattern->h; ++y) {
                    SDL_memset(p, c, 3);
                    p += pattern->pitch;
                }
            }
            if (c) {
                c = 0x00;
            } else {
                c = 0xFF;
            }
        }

        return pattern;
    } else {
        return nullptr;
    }

}

SDL_bool LibAVable::verify_ycbcr_data(Uint32 format, const Uint8 *yuv, int yuv_pitch, SDL_Surface *surface) {
    const int tolerance = 20;
    const int size = (surface->h * surface->pitch);

    SDL_bool result = SDL_FALSE;

    auto rgb = new Uint8[size];

    if (SDL_ConvertPixels(surface->w, surface->h, format, yuv, yuv_pitch, surface->format->format, rgb, surface->pitch) == 0) {
        int x, y;
        result = SDL_TRUE;
        for (y = 0; y < surface->h; ++y) {
            const Uint8 *actual = rgb + y * surface->pitch;
            const Uint8 *expected = (const Uint8 *)surface->pixels + y * surface->pitch;
            for (x = 0; x < surface->w; ++x) {
                int deltaR = (int)actual[0] - expected[0];
                int deltaG = (int)actual[1] - expected[1];
                int deltaB = (int)actual[2] - expected[2];
                int distance = (deltaR * deltaR + deltaG * deltaG + deltaB * deltaB);
                if (distance > tolerance) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pixel at %d,%d was 0x%.2x,0x%.2x,0x%.2x, expected 0x%.2x,0x%.2x,0x%.2x, distance = %d\n", x, y, actual[0], actual[1], actual[2], expected[0], expected[1], expected[2], distance);
                    result = SDL_FALSE;
                }
                actual += 3;
                expected += 3;
            }
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(format), SDL_GetPixelFormatName(surface->format->format), SDL_GetError());
    }

    delete[] rgb;

    return result;
}

LibAVable::AVthings_t * LibAVable::init_state(std::string codec_name, int decode_flag, int x, int y, int framerate, int bitrate) {
    static auto state = new AVthings_t;

    if(decode_flag == 0)
        init_codec( state, codec_name );
    else
        init_decode( state, codec_name );

    state->pkt = av_packet_alloc();
    state->c->bit_rate = bitrate;
    state->c->width = x;
    state->c->height = y;
    state->c->time_base = (AVRational){1, framerate};
    state->c->framerate = (AVRational){framerate, 1};

    /* emit one intra frame every ten frames
    * check frame pict_type before passing frame
    * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
    * then gop_size is ignored and the output of encoder
    * will always be I frame irrespective to gop_size
    */
    state->c->gop_size = 100;
    state->c->max_b_frames = 0;
    state->c->pix_fmt = AV_PIX_FMT_YUV420P;

    if ( state->codec->id == AV_CODEC_ID_H264 )
        av_opt_set( state->c->priv_data, "preset", "slow", 0 );

    /* open it */
    int ret = avcodec_open2( state->c, state->codec, nullptr );
    if ( ret < 0 ) {
        // std::string err =  av_err2str(ret);
        // fprintf( stderr, "Could not open codec: %s\n", err.c_str() );
        assert( false && "Cannot open codec error" );
    }

    state->frame = av_frame_alloc();
    if ( !state->frame ) {
        fprintf( stderr, "Could not allocate video frame\n" );
        assert( false && "Cannot allocate video frame" );
    }

    state->frame->format = state->c->pix_fmt;
    state->frame->width  = state->c->width;
    state->frame->height = state->c->height;

    ret = av_frame_get_buffer(state->frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        assert( false && "Cannot allocate video frame data" );
    }

    /* make sure the frame data is writable */
    ret = av_frame_make_writable( state->frame );
    if (ret < 0)
        assert( false && "Frame not writable" );

    if(decode_flag == 1) {
        state->parser = av_parser_init(state->codec->id);
        if (!state->parser) {
            fprintf(stderr, "parser not found\n");
            assert(false && "Parser not found");
        }
    }

    return state;
}

size_t LibAVable::readfile( LibAVable::AVthings_t *state, FILE *infile ) {

    int ret = avcodec_send_packet( state->c, state->pkt );

    if(ret < 0 ) {
        switch (ret) {
            case AVERROR(EAGAIN):
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                             "AVERROR(EAGAIN) input is not accepted in the current state - user must read output with avcodec_receive_frame() (once all output is read, the packet should be resent, and the call will not fail with EAGAIN).");
                goto end_case;
            case AVERROR_EOF:
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                             "AVERROR_EOF the decoder has been flushed, and no new packets can be sent to it (also returned if more than 1 flush packet is sent) ");
                goto end_case;
            case AVERROR(EINVAL):
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                             "AVERROR(EINVAL) codec not opened, it is an encoder, or requires flush");
                goto end_case;
            case AVERROR(ENOMEM):
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                             "AVERROR(ENOMEM) failed to add packet to internal queue, or similar");
                goto end_case;
            default:
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Legitimate decoding errors");
            end_case:
                fprintf(stderr, "Error sending a packet for decoding, avcodec_send_packet returned %d\n", ret);
                return -1;
        }
    }

    ret = avcodec_receive_frame( state->c , state->frame );
    if ( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
        return -1;
    else if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        assert( false && "Error decoding video" );
    }

    printf("decoded frame %3d\n", state->c->frame_number );
    fflush(stdout);
    return ret;
}


#endif //SDL_CRT_FILTER_LIBAVABLE_HPP
