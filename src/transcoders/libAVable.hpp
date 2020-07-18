//
// Created by sebastian on 25/5/20.
//

#ifndef SDL_CRT_FILTER_LIBAVABLE_HPP
#define SDL_CRT_FILTER_LIBAVABLE_HPP

#include <cstdlib>
#include <zlib.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include "transcoders/helpers/testyuv_cvt.c"
}
#include "transcoders/Magickable.hpp"
#include "generators/MagickOSD.hpp"

/* 422 (YUY2, etc) formats are the largest */
#define MAX_YCBCR_SURFACE_SIZE(W, H, P)  (H*4*(W+P+1)/2)

struct LibAVable_hypersurface_t {
    std::vector<SDL_Surface*> surfaces;
    size_t depth;
    size_t step;
};

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
    init_state(std::string codec_name, int decode_flag = 0, int x = 320, int y = 240, int framerate = 30, int bitrate = 800000);
    static size_t readfile(LibAVable::AVthings_t *state, FILE *infile);
    static void writefile(LibAVable::AVthings_t *state, FILE *outfile) {
        int ret;

        /* send the frame to the encoder */
        //if ( state->frame )
          //  printf("Send frame %3" PRId64"\n", state->frame->pts );

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

            //printf("Write packet %3" PRId64" (size=%5d)\n", state->pkt->pts, state->pkt->size );
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

    static void pack_interlace(SDL_Surface *dst, SDL_Surface *src);
    static void pack_deinterlace(SDL_Surface *dst, SDL_Surface *src);

    static double log3_2(double x);
    static double log_conv(double x);
    static double pack_logluma(double luma_diff);
    static double log_unconv(double x);
    static double pack_unlogluma(double luma_diff);
    static void FlipSurfaceVertical(SDL_Surface *flip, SDL_Surface *src);
    static void pack_doubleinterlace(SDL_Surface* dst, SDL_Surface* src);
    static void pack_doubledeinterlace(SDL_Surface* dst, SDL_Surface* src);

    static AVFrame *AllocateFrame(int w, int h, int format);

    static void pack_spiral( SDL_Surface *dst, SDL_Surface *src, int step );
    static void pack_despiral( SDL_Surface *dst, SDL_Surface *src, int step );

    static void pack_miniraster( SDL_Surface *dst, SDL_Surface *src, int step );
    static void unpack_miniraster( SDL_Surface *dst, SDL_Surface *src, int step );
    static void pack_minizigzag( SDL_Surface *dst, SDL_Surface *src, int step );
    static void unpack_minizigzag( SDL_Surface *dst, SDL_Surface *src, int step );
    static void pack_all( SDL_Surface *dst, SDL_Surface *src, int step );
    static void unpack_all( SDL_Surface *dst, SDL_Surface *src, int step );
    static void pack_all_recursive( SDL_Surface *dst, SDL_Surface *src, int step );
    static void unpack_all_recursive( SDL_Surface *dst, SDL_Surface *src, int step );
    static SDL_Rect* diagonal_sort( int step );
    static SDL_Rect* box_raster( SDL_Surface* ref, int step );
    static void pack_diagonal( SDL_Surface* dst, SDL_Surface* src, int step );
    static void unpack_diagonal( SDL_Surface* dst, SDL_Surface* src, int step );
    static void pack_diagonal_mirror( SDL_Surface* dst, SDL_Surface* src, int step );
    static void unpack_diagonal_mirror( SDL_Surface* dst, SDL_Surface* src, int step );
    static void pack_diagonal_flip( SDL_Surface* dst, SDL_Surface* src, int step );
    static void unpack_diagonal_flip( SDL_Surface* dst, SDL_Surface* src, int step );
    static void pixelsort( SDL_Surface* dst, SDL_Surface*  src, SDL_Rect* macro_id, SDL_Rect* block_id);
    static void pixelunsort( SDL_Surface* dst, SDL_Surface*  src, SDL_Rect* macro_id, SDL_Rect* block_id);
    static SDL_Rect* mirror_sort( SDL_Rect* macro_id );
    static SDL_Rect* flip_sort( SDL_Rect* macro_id );

    static void swirl_pattern( SDL_Rect *pattern, int step );

    static LibAVable_hypersurface_t* hs_add( LibAVable_hypersurface_t* hs, SDL_Surface* src );
    static void hs_copy(LibAVable_hypersurface_t *hypsfc_source, LibAVable_hypersurface_t *hypsfc_destin );
    static void hs_transpose(LibAVable_hypersurface_t *hs, LibAVable_hypersurface_t *hc, int direction);
    static void hs_transpose(LibAVable_hypersurface_t *hs, LibAVable_hypersurface_t *hc) {
        hs_transpose( hs, hc, 0 );
    }
    static void hs_untranspose(LibAVable_hypersurface_t *hs, LibAVable_hypersurface_t *hc) {
        hs_transpose( hs, hc, 1 );
    }

    static void hs_itranspose(LibAVable_hypersurface_t *hs, LibAVable_hypersurface_t *hc, int times ) {
        auto hst = new LibAVable_hypersurface_t();
        hs_copy(hs, hc);
        for (int i = 0; i < times; i++ ) {
            hs_transpose(hc, hst, 0);
            hs_free(hc);
            hs_copy( hst, hc );
            hs_free( hst );
        }
        delete hst;
    }

    static void hs_iuntranspose(LibAVable_hypersurface_t *hs, LibAVable_hypersurface_t *hc, int times ) {
        auto hst = new LibAVable_hypersurface_t();
        hs_copy(hs, hc);
        for (int i = 0; i < times; i++ ) {
            hs_transpose(hc, hst, 1);
            hs_free(hc);
            hs_copy( hst, hc );
            hs_free( hst );
        }
        delete hst;
    }

    static void hs_free( LibAVable_hypersurface_t* src );
    static SDL_Surface* hs_stack( LibAVable_hypersurface_t *hs, LibAVable_hypersurface_t *hc, SDL_Surface* src );
    static void macroblock_test( SDL_Surface* dst, int start, int step );

    static void compress_memory(void *in_data, size_t in_data_size, std::vector<uint8_t> &out_data );
    static float compressibility( SDL_Surface* src );
};

void LibAVable::fromFrame( Uint8 *ycbcr, AVFrame *frame ) {
    int fid = 0, cid = 0, yid = 0, nyid = 0;

    //YVYU Luminance
    for ( int y = 0; y < frame->height ; y++ ) {
        for ( int x = 0; x < frame->width; x++ ) {
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
    const int yuv_len = MAX_YCBCR_SURFACE_SIZE(pattern->w, pattern->h, extra_pitch );

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

AVFrame* LibAVable::AllocateFrame( int w, int h, int format ) {
    auto frame = av_frame_alloc();
    if ( !frame ) {
        fprintf( stderr, "Could not allocate video frame\n" );
        assert( false && "Cannot allocate video frame" );
    }

    frame->format = format;
    frame->width  = w;
    frame->height = h;

    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        assert( false && "Cannot allocate video frame data" );
    }

    /* make sure the frame data is writable */
    ret = av_frame_make_writable( frame );
    if (ret < 0)
        assert( false && "Frame not writable" );

    return frame;
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
    state->c->gop_size = 10;
    state->c->max_b_frames = 10;
    state->c->pix_fmt = AV_PIX_FMT_YUV420P;

    if ( state->codec->id == AV_CODEC_ID_H264 ) {
        av_opt_set(state->c->priv_data, "preset", "medium", 0);
        av_opt_set(state->c->priv_data, "tune", "animation", 0);
        av_opt_set(state->c->priv_data, "crf", "0", 0);
        av_opt_set(state->c->priv_data, "crf_max", "0", 0);
    }
    /* open it */
    int ret = avcodec_open2( state->c, state->codec, nullptr );
    if ( ret < 0 ) {
        // std::string err =  av_err2str(ret);
        // fprintf( stderr, "Could not open codec: %s\n", err.c_str() );
        assert( false && "Cannot open codec error" );
    }
    state->frame = AllocateFrame( state->c->width, state->c->height, state->c->pix_fmt );
    /*
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

    // make sure the frame data is writable
    ret = av_frame_make_writable( state->frame );
    if (ret < 0)
        assert( false && "Frame not writable" );

 */
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

double LibAVable::log3_2( double x ) {
    return log(x) / log ( 1.5 );
}

double LibAVable::log_conv( double x ) {
    return log3_2((x + 2) / 2 );
}

double LibAVable::log_unconv( double x ) {
    return (- pow(2, (1 - x) ) * ( pow( 2, x ) - pow( 3, x ) ));
}

double LibAVable::pack_logluma( double luma_diff ) {
    double log_diff = log_conv( abs( luma_diff ) );
    if ( luma_diff >= 0 ) {
        return 0.5 + log_diff / 2;
    } else {
        return 0.5 - log_diff / 2;
    }
}

double LibAVable::pack_unlogluma( double luma_diff ) {
    double real_luma_diff = (luma_diff - 0.5) * 2;
    if ( luma_diff >= 0.5 ) {
        return ( log_unconv( abs(real_luma_diff) ) ) ;
    } else {
        return ( -log_unconv( abs(real_luma_diff) ) ) ;;
    }
}

void LibAVable::pack_interlace(SDL_Surface *dst, SDL_Surface *src) {
    auto line_pair = new double[src->w][2];
    for( int y = 0; y < src->h; y+=2 ) {
        for ( int x = 0; x < src->w; ++x ) {
            line_pair[x][0] = Pixelable::direct_to_luma(src, x, y) + Pixelable::direct_to_luma(src, x, y + 1) - 1;
            line_pair[x][1] = Pixelable::direct_to_luma(src, x, y) - Pixelable::direct_to_luma(src, x, y + 1);
            Uint32 pixel[2] = {
                    Pixelable::direct_from_luma( pack_logluma(line_pair[x][0]) ),
                    Pixelable::direct_from_luma( pack_logluma(line_pair[x][1]) )
            };
            Pixelable::put32(dst, x, y, pixel[0]);
            Pixelable::put32(dst, x, y+1, pixel[1]);
        }
    }
    delete[] line_pair;
}

void LibAVable::pack_deinterlace(SDL_Surface *dst, SDL_Surface *src) {
    auto line_pair = new double[src->w][2];
    for( int y = 0; y < src->h ; y+=2 ) {
        for ( int x = 0; x < src->w; ++x ) {
            double gQ0 = pack_unlogluma( Pixelable::direct_to_luma(src, x, y) );
            double gQ1 = pack_unlogluma( Pixelable::direct_to_luma(src, x, y + 1) );
            line_pair[x][0] = ( gQ1 + gQ0 + 1 ) / 2;
            line_pair[x][1] = ( gQ0 - gQ1 + 1 ) / 2;
            Uint32 pixel[2] = {
                    Pixelable::direct_from_luma( line_pair[x][0] ),
                    Pixelable::direct_from_luma( line_pair[x][1] )
            };
            Pixelable::put32(dst, x, y, pixel[0]);
            Pixelable::put32(dst, x, y+1, pixel[1]);
        }
    }
    delete[] line_pair;
}

void LibAVable::FlipSurfaceVertical(SDL_Surface* flip, SDL_Surface* src ) {
    for(int x = 0; x < src->w; x++ )
        for(int y = 0; y < src->h; y++ ) {
            Uint32 pixel = Pixelable::get32( src, x, y );
            Pixelable::put32( flip, x, src->h - y - 1, pixel );
        }
}

void LibAVable::pack_doubleinterlace( SDL_Surface* dst, SDL_Surface* src ) {
    auto swap = AllocateSurface( src );
    pack_interlace( dst, src );
    FlipSurfaceVertical( swap, dst );
    pack_interlace( dst, swap );
    SDL_FreeSurface( swap );
}

void LibAVable::pack_doubledeinterlace( SDL_Surface *dst, SDL_Surface *src ) {
    auto swap = AllocateSurface( src );
    pack_deinterlace( dst, src );
    FlipSurfaceVertical( swap, dst );
    pack_deinterlace( dst, swap );
    SDL_FreeSurface( swap );
}


//It generates a swirl pattern macroblock pixel index LUT
void LibAVable::swirl_pattern( SDL_Rect pattern[], int step ) {
    int y0 = 0, x0 = 0, pos = 0;

    while ( pos < (int) pow(step,2) ) {

        for (int x = x0; x < step - x0; ++x) {
            pattern[pos] = { x, y0, step, step };
            ++pos;
        }

        for (int y = y0 + 1; y < step - y0; ++y) {
            pattern[pos] = {step - x0 - 1, y, step, step };
            ++pos;
        }

        for (int x = step - x0 - 2; x >= x0; --x) {
            pattern[pos] = { x, step - y0 - 1, step, step };
            ++pos;
        }

        for (int y = step - y0 - 2; y > y0; --y) {
            pattern[pos] = { x0, y, step, step };
            ++pos;
        }

        x0++;
        y0++;
    }
}

//same as spiral pattern
void LibAVable::pack_spiral( SDL_Surface *dst, SDL_Surface *src, int step ) {
    assert(src->w % step == 0 && "Incompatible step");
    assert(src->h % step == 0 && "Incompatible step");
    auto pattern = new SDL_Rect[ (int) pow( step, 2 ) ];
    swirl_pattern( pattern, step );
    int srcpos = 0;
    for ( int y = 0; y < src->h; y+=step ) {
        for ( int x = 0; x < src->w; x+=step ) {
            for ( int pos = 0; pos < step * step; ++pos ) {
                int sx = srcpos % src->w;
                int sy = srcpos / src->w;
                Pixelable::copy32( dst, src, sx, sy, x + pattern[pos].x, y + pattern[pos].y );
                srcpos++;
            }
        }
    }

    delete[] pattern;
}

//inverse of spiral one
void LibAVable::pack_despiral(SDL_Surface *dst, SDL_Surface *src, int step) {
    assert(src->w % step == 0 && "Incompatible step");
    assert(src->h % step == 0 && "Incompatible step");
    auto pattern = new SDL_Rect[ (int) pow( step, 2 ) ];
    swirl_pattern( pattern, step );
    int srcpos = 0;
    for ( int y = 0; y < src->h; y+=step ) {
        for ( int x = 0; x < src->w; x+=step ) {
            for ( int pos = 0; pos < step * step; ++pos ) {
                int sx = srcpos % src->w;
                int sy = srcpos / src->w;
                Pixelable::copy32( dst, src,x + pattern[pos].x, y + pattern[pos].y, sx, sy );
                srcpos++;
            }
        }
    }
    delete[] pattern;
}

// mini macroblock left to right raster
void LibAVable::pack_miniraster( SDL_Surface *dst, SDL_Surface *src, int step ) {
    assert(src->w % step == 0 && "Incompatible step");
    assert(src->h % step == 0 && "Incompatible step");
    int pos = 0, dby = -step, dbx = 1, dbz;
    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {
            int block = pos % (int) pow(step, 2);
            int db = pos / (int) pow(step, 2);
            dbz = dbx;
            dbx = db * step % src->w;
            if (dbx == 0 && dbz != dbx) dby += step;
            int dy = dby + block / step;
            int dx = dbx + block % step;
            Pixelable::copy32( dst, src, x, y, dx, dy );
            ++pos;
        }
    }
}

//inverse of the above
void LibAVable::unpack_miniraster( SDL_Surface *dst, SDL_Surface *src, int step ) {
    assert(src->w % step == 0 && "Incompatible step");
    assert(src->h % step == 0 && "Incompatible step");
    int pos = 0, dby = -step, dbx = 1, dbz;
    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {
            int block = pos % (int) pow(step, 2);
            int db = pos / (int) pow(step, 2);
            dbz = dbx;
            dbx = db * step % src->w;
            if (dbx == 0 && dbz != dbx) dby += step;
            int dy = dby + block / step;
            int dx = dbx + block % step;
            Pixelable::copy32( dst, src, dx, dy, x, y );
            ++pos;
        }
    }
}

// same as miniraster but with zig zag line ordering
void LibAVable::pack_minizigzag( SDL_Surface *dst, SDL_Surface *src, int step ) {
    assert(src->w % step == 0 && "Incompatible step");
    assert(src->h % step == 0 && "Incompatible step");
    int pos = 0, dby = -step, dbx = 1, dbz;
    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {
            int block = pos % (int) pow(step, 2);
            int db = pos / (int) pow(step, 2);
            dbz = dbx;
            dbx = db * step % src->w;
            if (dbx == 0 && dbz != dbx) dby += step;
            int dy = dby + block / step;
            int dx = dy % 2 == 0? dbx - 1 + step - block % step: dbx + block % step;
            Pixelable::copy32( dst, src, x, y, dx, dy );
            ++pos;
        }
    }
}

//inverse of pack_zigzag
void LibAVable::unpack_minizigzag( SDL_Surface *dst, SDL_Surface *src, int step ) {
    assert(src->w % step == 0 && "Incompatible step");
    assert(src->h % step == 0 && "Incompatible step");
    int pos = 0, dby = -step, dbx = 1, dbz;
    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {
            int block = pos % (int) pow(step, 2);
            int db = pos / (int) pow(step, 2);
            dbz = dbx;
            dbx = db * step % src->w;
            if (dbx == 0 && dbz != dbx) dby += step;
            int dy = dby + block / step;
            int dx = dy % 2 == 0? dbx -1 + step - block % step: dbx + block % step;
            Pixelable::copy32( dst, src, dx, dy, x, y );
            ++pos;
        }
    }
}

//it stacks various transforms
void LibAVable::pack_all(SDL_Surface *dst, SDL_Surface *src, int step) {
    auto frame = Surfaceable::AllocateSurface( src );
    pack_miniraster ( frame, src, step );
    deverticalize( dst, frame );
    unpack_minizigzag ( frame, dst, step );
    verticalize( dst, frame );
    pack_spiral ( frame, dst, step );
    deverticalize( dst, frame );
    unpack_diagonal( frame, dst, step );
    verticalize( dst, frame );
    pack_minizigzag ( frame, dst, step );
    deverticalize( dst, frame );
    SDL_FreeSurface( frame );
}

//inverse of te all transforms
void LibAVable::unpack_all(SDL_Surface *dst, SDL_Surface *src, int step) {
    auto frame = Surfaceable::AllocateSurface( src );
    verticalize( frame, src );
    unpack_minizigzag ( dst, frame, step );
    deverticalize( frame, dst );
    pack_diagonal ( dst, frame, step );
    verticalize( frame, dst );
    pack_despiral ( dst, frame, step );
    deverticalize( frame, dst );
    pack_minizigzag ( dst, frame, step );
    verticalize( frame, dst );
    unpack_miniraster( dst, frame, step );
    SDL_FreeSurface( frame );
}

//it does the transform on the sub macroblocks from 4 to step
void LibAVable::pack_all_recursive(SDL_Surface *dst, SDL_Surface *src, int step) {
    auto frame = Surfaceable::AllocateSurface( src );
    auto copy = Surfaceable::AllocateSurface( src );
    Loader::SurfacePixelsCopy( src, copy );
    int ustep = step;
    while( ustep % 2 == 0 && ustep > 4 ) {
        ustep /= 2;
    }

    while( ustep <= step ) {
        pack_all( frame, copy, ustep );
        Loader::SurfacePixelsCopy( frame, copy );
        //printf( "ustep: %d\n", ustep );
        ustep *= 2;
    }
    Loader::SurfacePixelsCopy( frame, dst );
    SDL_FreeSurface( frame );
}

//inverse of the above
void LibAVable::unpack_all_recursive(SDL_Surface *dst, SDL_Surface *src, int step) {
    auto frame = Surfaceable::AllocateSurface( src );
    auto copy = Surfaceable::AllocateSurface( src );
    Loader::SurfacePixelsCopy( src, copy );

    while( step % 2 == 0 && step > 4 ) {
        unpack_all(frame, copy, step);
        Loader::SurfacePixelsCopy(frame, copy);
        //printf( "pstep: %d\n", step );
        step /= 2;
    }
    //printf( "pstep: %d\n", step );
    unpack_all( frame, copy, step );

    Loader::SurfacePixelsCopy( frame, dst );
    SDL_FreeSurface( frame );
}

//It generates diagonal sort macroblock pixel order LUT
SDL_Rect *LibAVable::diagonal_sort( int m ) {
    int square_size = (int) pow(m, 2);
    SDL_Rect* idx_rel = new SDL_Rect[ square_size ];

    int pos = 0;
    for ( int box = 1; box < m ; ++box ) {
        for ( int x = 0; x < box; ++x ) {
            idx_rel[pos] = { x, box - x - 1, m, m };
            //printf( "pos %d: %d, %d\n", pos, x, idx_rel[pos].y );
            ++pos;
        }
    }
    //printf( "midway ---- here --- \n" );
    for ( int box = m; box > 0; --box ) {
        int y = m - 1;
        for ( int x = m - box; x < m; ++x ) {
            idx_rel[pos] = { x, y--, m, m };
            //printf( "pos %d: %d, %d\n", pos, x, idx_rel[pos].y );
            assert( pos < square_size && "Pattern overflow" );
            ++pos;
        }
    }

    return idx_rel;
}

//It generates box macroblock raster origin offsets LUT to use with the inner macroblock pixel index LUT on the final sorting algorithm
SDL_Rect *LibAVable::box_raster( SDL_Surface* ref, int m ) {
    int square_size = (int) pow( m, 2 );
    int area_size = ref->w * ref->h;
    assert ( area_size % square_size == 0 && "Incompatible Step" );
    auto idx_rel = new SDL_Rect[ area_size / square_size ];

    int pos = 0;
    while ( pos < area_size ) {
        int dx = m * pos / square_size % ref->w / m ;
        int dy = m * ( (int) pos / ( square_size * ref->w / m ) );
        idx_rel[ pos / square_size ] = { m * dx, dy, m, m };
        //printf( "box pos %d: %d, %d\n", pos, m * dx, dy );
        pos+=square_size;
    }

    return idx_rel;
}


//it applies the transform from src to dst with macro_id as the source LUT and block_id as dst LUT
void LibAVable::pixelsort( SDL_Surface *dst, SDL_Surface *src, SDL_Rect *macro_id, SDL_Rect *block_id ) {
    int square_size = (int) pow( macro_id[0].w, 2 );
    int pos = 0;
    for ( int y = 0; y < src->h; y++ )
        for ( int x = 0; x < src->w; x++ ) {
            int dx = macro_id[ pos / square_size ].x + block_id[ pos % square_size ].x;
            int dy = macro_id[ pos / square_size ].y + block_id[ pos % square_size ].y;
            Pixelable::copy32( dst, src, x, y, dx, dy );
            ++pos;
        }
}

//inverse of the pixelsort
void LibAVable::pixelunsort(SDL_Surface *dst, SDL_Surface *src, SDL_Rect *macro_id, SDL_Rect *block_id) {
    int square_size = (int) pow( macro_id[0].w, 2 );
    int pos = 0;
    for ( int y = 0; y < src->h; y++ )
        for ( int x = 0; x < src->w; x++ ) {
            int dx = macro_id[ pos / square_size ].x + block_id[ pos % square_size ].x;
            int dy = macro_id[ pos / square_size ].y + block_id[ pos % square_size ].y;
            Pixelable::copy32( dst, src, dx, dy, x, y );
            ++pos;
        }
}

SDL_Rect* LibAVable::mirror_sort( SDL_Rect* macro_id ) {
    int square_size = (int) pow( macro_id[0].w , 2);

    for ( int pos = 0; pos < square_size; ++pos )
        macro_id[ pos ].x = macro_id[ pos ].w - macro_id[ pos ].x;

    return macro_id;
}

SDL_Rect *LibAVable::flip_sort(SDL_Rect *macro_id) {
    int square_size = (int) pow( macro_id[0].w , 2);

    for ( int pos = 0; pos < square_size; ++pos )
        macro_id[ pos ].y = macro_id[ pos ].h - macro_id[ pos ].y;

    return macro_id;
}

void LibAVable::pack_diagonal( SDL_Surface* dst, SDL_Surface* src, int step ) {
    auto block_id = diagonal_sort( step );
    auto macro_id = box_raster( src, step );

    pixelsort( dst, src, macro_id, block_id );
    delete [] macro_id;
    delete [] block_id;
}

void LibAVable::unpack_diagonal( SDL_Surface* dst, SDL_Surface* src, int step ) {
    auto block_id = diagonal_sort( step );
    auto macro_id = box_raster( src, step );

    pixelunsort( dst, src, macro_id, block_id );
    delete [] macro_id;
    delete [] block_id;
}

void LibAVable::pack_diagonal_mirror( SDL_Surface* dst, SDL_Surface* src, int step ) {
    auto block_id = mirror_sort( diagonal_sort( step ) );
    auto macro_id = box_raster( src, step );

    pixelsort( dst, src, macro_id, block_id );
    delete [] macro_id;
    delete [] block_id;
}

void LibAVable::unpack_diagonal_mirror( SDL_Surface* dst, SDL_Surface* src, int step ) {
    auto block_id = mirror_sort( diagonal_sort( step ) );
    auto macro_id = box_raster( src, step );

    pixelunsort( dst, src, macro_id, block_id );
    delete [] macro_id;
    delete [] block_id;
}

void LibAVable::pack_diagonal_flip( SDL_Surface* dst, SDL_Surface* src, int step ) {
    auto block_id = flip_sort( diagonal_sort( step ) );
    auto macro_id = box_raster( src, step );

    pixelsort( dst, src, macro_id, block_id );
    delete [] macro_id;
    delete [] block_id;
}

void LibAVable::unpack_diagonal_flip( SDL_Surface* dst, SDL_Surface* src, int step ) {
    auto block_id = flip_sort( diagonal_sort( step ) );
    auto macro_id = box_raster( src, step );

    pixelunsort( dst, src, macro_id, block_id );
    delete [] macro_id;
    delete [] block_id;
}

//allocates, then adds a pointer to a copy into the surfaces stack (hypersurface)
LibAVable_hypersurface_t *LibAVable::hs_add( LibAVable_hypersurface_t *hs, SDL_Surface *src ) {
    auto frame = Surfaceable::AllocateSurface( src );
    Magickable::blitScaled( frame, src );
    hs->surfaces.push_back( frame );
    return hs;
}

//allocates a copy of the source surfaces stack into destination surfaces stack, domain: (hypersurfaces) -> (hypersurfaces)
void LibAVable::hs_copy(LibAVable_hypersurface_t *hypsfc_source, LibAVable_hypersurface_t *hypsfc_destin ) {
    hypsfc_destin->step  = hypsfc_source->step;
    hypsfc_destin->depth = hypsfc_source->depth;
    for (auto & surface : hypsfc_source->surfaces) {
        hs_add(hypsfc_destin, surface );
    }
}

void LibAVable::hs_free( LibAVable_hypersurface_t *src ) {
    for ( auto & surface : src->surfaces )
        SDL_FreeSurface(surface);
    src->surfaces.clear();
}

SDL_Surface *LibAVable::hs_stack(LibAVable_hypersurface_t *hs, LibAVable_hypersurface_t *hc, SDL_Surface* src ) {
    auto frame = Surfaceable::AllocateSurface( src );
    Loader::blank( frame );

    if( hc != nullptr && hc->surfaces.size() > 0 ) { //there are remaining frames to publish
        auto surface = hc->surfaces.back();
        Loader::SurfacePixelsCopy( surface , frame );
        SDL_FreeSurface( surface );
        hc->surfaces.pop_back();
    }

    if ( hs->surfaces.size() >= hs->depth ) { //hypersurface filled
        if( hc != nullptr ) {
            hs_free( hc );
        }
        hs_copy( hs, hc );
        hs_free( hs );
    }

    hs_add( hs, src );
    return frame;

}

void LibAVable::hs_transpose(LibAVable_hypersurface_t *hs, LibAVable_hypersurface_t *hc, int direction) {
    auto macro_id = box_raster( hs->surfaces.back(), hs->step );
    hs_copy( hs, hc );
    auto depth = hs->depth;
    auto count = hs->surfaces.back()->w * hs->surfaces.back()->h / (hs->step * hs->step);
    size_t pos = 0, bi = 0;

    if ( direction == 0 ) {
        for (auto &surface : hs->surfaces) {
            bi = 0;
            while (bi < count) {
                for (auto &dst: hc->surfaces) {
                    auto dz = (pos / depth) % count;
                    SDL_BlitSurface(surface, &macro_id[bi], dst, &macro_id[dz]);
                    ++bi;
                    ++pos;
                }
            }
        }
    } else {
        for (auto &dst : hc->surfaces) {
            bi = 0;
            while (bi < count) {
                for (auto &surface: hs->surfaces) {
                    auto dz = (pos / depth) % count;
                    SDL_BlitSurface(surface, &macro_id[dz], dst, &macro_id[bi]);
                    ++bi;
                    ++pos;
                }
            }
        }
    }

    delete macro_id;
}

void LibAVable::macroblock_test(SDL_Surface* dst, int start, int step ) {
    auto osd = new MagickOSD();
    auto black = Surfaceable::AllocateSurface( dst );
    auto frame = Surfaceable::AllocateSurface( dst );
    Loader::blank(black);
    osd->setFontSize( osd->getFontSize() / 2 );
    auto macroblocks = box_raster( dst, step );
    auto count = dst->w * dst->h / ( step * step );
    for( int i = 0; i < count; i++ ) {
        char buff[100] = { 0 };
        snprintf(buff, sizeof(buff), "%03d", i + start );
        osd->centerXtxt( &macroblocks[i], buff );

        Pixelable::DrawRect( black,
                &macroblocks[i],
                0xFFFFFFFF
                );
    }
    osd->getSurface( frame );
    SDL_BlitSurface ( frame, nullptr, black, nullptr );
    Loader::SurfacePixelsCopy( black, dst );
    SDL_FreeSurface( black );
    SDL_FreeSurface( frame );
    delete osd;
}

float LibAVable::compressibility( SDL_Surface* src ) {
    auto cm = Pixelable::AsLumaChannelMatrix( src );
    auto pixels = Pixelable::pixels( src );
    if ( pixels > 0x100 * 1024 ) pixels = 0x100 * 1024;
    std::vector<uint8_t> out_data;
    compress_memory( cm, pixels, out_data );
    assert( out_data.size() > 0 && "empty compressor result ");
    auto ratio = (float) out_data.size() / pixels;
    return ratio;
}

void LibAVable::compress_memory(void *in_data, size_t in_data_size, std::vector<uint8_t> &out_data) {
    std::vector<uint8_t> buffer;

    const size_t BUFSIZE = 128 * 1024;
    uint8_t temp_buffer[BUFSIZE];

    z_stream strm;
    strm.zalloc = 0;
    strm.zfree = 0;
    strm.next_in = reinterpret_cast<uint8_t *>(in_data);
    strm.avail_in = in_data_size;
    strm.next_out = temp_buffer;
    strm.avail_out = BUFSIZE;

    deflateInit(&strm, Z_BEST_COMPRESSION);

    while (strm.avail_in != 0)
    {
        int res = deflate(&strm, Z_NO_FLUSH);
        assert(res == Z_OK);
        if (strm.avail_out == 0)
        {
            buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
            strm.next_out = temp_buffer;
            strm.avail_out = BUFSIZE;
        }
    }

    int deflate_res = Z_OK;
    while (deflate_res == Z_OK)
    {
        if (strm.avail_out == 0)
        {
            buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
            strm.next_out = temp_buffer;
            strm.avail_out = BUFSIZE;
        }
        deflate_res = deflate(&strm, Z_FINISH);
    }

    assert(deflate_res == Z_STREAM_END);
    buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE - strm.avail_out);
    deflateEnd(&strm);

    out_data.swap(buffer);
}

#endif //SDL_CRT_FILTER_LIBAVABLE_HPP
