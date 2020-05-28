//
// Created by sebastian on 25/5/20.
//

#ifndef SDL_CRT_FILTER_LIBAVABLE_HPP
#define SDL_CRT_FILTER_LIBAVABLE_HPP

///#include <stdio.h>
#include <cstdlib>
//#include <string.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include "transcoders/helpers/testyuv_cvt.c"
}
#include "transcoders/Magickable.hpp"

/* 422 (YUY2, etc) formats are the largest */
#define MAX_YUV_SURFACE_SIZE(W, H, P)  (H*4*(W+P+1)/2)

class LibAVable: public Magickable {
public:
    static void encode(void* dst, void* src);
    static void decode(void* dst, void* src);
    static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                       FILE *outfile) {
        int ret;

        /* send the frame to the encoder */
        if (frame)
            printf("Send frame %3" PRId64"\n", frame->pts);

        ret = avcodec_send_frame(enc_ctx, frame);
        if (ret < 0) {
            fprintf(stderr, "Error sending a frame for encoding\n");
            exit(1);
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            else if (ret < 0) {
                fprintf(stderr, "Error during encoding\n");
                exit(1);
            }

            printf("Write packet %3" PRId64" (size=%5d)\n", pkt->pts, pkt->size);
            fwrite(pkt->data, 1, pkt->size, outfile);
            av_packet_unref(pkt);
        }
    }

public:

    static void init_codec( AVCodec*& codec, AVCodecContext*& c,  const std::string codec_name ) {
        avcodec_register_all();

        /* find the encoder */
        AVCodec* codecptr = avcodec_find_encoder_by_name( codec_name.c_str() );
        if ( !codecptr ) {
            fprintf(stderr, "Codec '%s' not found\n", codec_name.c_str());
            exit(1);
        }

        AVCodecContext* cptr = avcodec_alloc_context3( codecptr );
        if ( !cptr ) {
            fprintf(stderr, "Could not allocate video codec context\n");
            exit(1);
        }

        codec = codecptr;
        c = cptr;
    }

    static void dummy_image(int idx, AVFrame& frame) {
        /* prepare a dummy image */
        /* Y */
        for (int y = 0; y < frame.height; y++) {
            for (int x = 0; x < frame.width; x++) {
                frame.data[0][y * frame.linesize[0] + x] = x + y + idx * 3;
            }
        }

        /* Cb and Cr */
        for (int y = 0; y < frame.height/2; y++) {
            for (int x = 0; x < frame.width/2; x++) {
                frame.data[1][y * frame.linesize[1] + x] = 128 + y + idx * 2;
                frame.data[2][y * frame.linesize[2] + x] = 64 + x + idx * 5;
            }
        }

        frame.pts = idx;
    }



    static int test_yuv(int pattern_size, int extra_pitch);
    static SDL_Surface* generate_test_pattern(int pattern_size);

    static SDL_bool verify_yuv_data(Uint32 format, const Uint8 *yuv, int yuv_pitch, SDL_Surface *surface);

    static SDL_bool is_packed_yuv_format(Uint32 format);

    //from RGB32 to YCrCb
    static void toYCrBr(uint8_t *yuv, SDL_Surface *pattern, int format);
    static void toFrame(AVFrame *frame, const Uint8 *ycrcb, size_t ycrcb_len);

};

void LibAVable::toFrame(AVFrame *frame, const Uint8 *ycrcb, size_t ycrcb_pitch) {
    int fid = 0, cid = 0, pxid = 0;

    //YVYU
    for (int y = 0; y < frame->height ; y++) {
        for (int x = 0; x < frame->width; x++) {
            fid = y * frame->linesize[0] + x;
            frame->data[0][ fid ] = ycrcb[ fid * 2 ];
//            if (pxid % 4 == 0) {
  //              cid = (int) SDL_floor((double) pxid / 4 ); // 4 Uint8 contains two Y values, one V and one U
                //cid = (y/2) * frame->linesize[1] + (x / 2);
    //            frame->data[1][ cid ] = ycrcb[ 8 * cid + 3 ]; // Cb <-> U
      //          frame->data[2][ cid ] = ycrcb[ 8 * cid + 1 ]; // Cr <-> V
        //    }
            pxid++;
        }
    }

    pxid = 0;
    /* Cb and Cr */
    for (int y = 0; y < frame->height/2; y++) {
        for (int x = 0; x < frame->width/2; x++) {
            cid = y * frame->linesize[1] + x;
            frame->data[1][ cid ] = ycrcb[ 4 * cid + 3 ]; // Cb <-> U;
            frame->data[2][ cid ] = ycrcb[ 4 * cid + 1 ]; // Cr <-> V;
            pxid++;
        }
    }


    //memcpy( frame->data[0], ycrcb, frame->height * frame->width );
    //memset(frame->data[1], 128, frame->height * frame->width / 4 );
    //memset(frame->data[2], 128, frame->height * frame->width / 4 );

}

void LibAVable::decode(void *dst, void *src) {
    auto source_frame = static_cast<SDL_Surface*>(src);
    auto dst_frame = static_cast<AVFrame*>(dst);
    const int yuv_len = MAX_YUV_SURFACE_SIZE(source_frame->w, source_frame->h, 0);
    auto yuv = new Uint8[yuv_len];

    auto format = SDL_PIXELFORMAT_YVYU;

    auto yuv_pitch = CalculateYUVPitch(format, source_frame->w);
    toYCrBr( yuv, source_frame, format );
    toFrame( dst_frame, yuv, yuv_pitch );

    delete [] yuv;
}

void LibAVable::toYCrBr(uint8_t *yuv, SDL_Surface *pattern, int format) {
    auto yuv_pitch = CalculateYUVPitch(format, pattern->w);
    if (SDL_ConvertPixels(pattern->w, pattern->h, pattern->format->format, pattern->pixels, pattern->pitch, format, yuv, yuv_pitch) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(pattern->format->format), SDL_GetPixelFormatName(format), SDL_GetError());
        assert(false && "Error during format conversion");
    }
}

int LibAVable::test_yuv(int pattern_size, int extra_pitch) {
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
    const int yuv_len = MAX_YUV_SURFACE_SIZE(pattern->w, pattern->h, extra_pitch);

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
        yuv1_pitch = CalculateYUVPitch(formats[i], pattern->w);
        if (!verify_yuv_data(formats[i], yuv1, yuv1_pitch, pattern)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed conversion from %s to RGB\n", SDL_GetPixelFormatName(formats[i]));
            result = -1;
            break;
        }
    }

    /* Verify conversion to YUV formats */
    for (i = 0; i < (int) SDL_arraysize(formats); ++i) {
        yuv1_pitch = CalculateYUVPitch(formats[i], pattern->w) + extra_pitch;
        if (SDL_ConvertPixels(pattern->w, pattern->h, pattern->format->format, pattern->pixels, pattern->pitch, formats[i], yuv1, yuv1_pitch) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't convert %s to %s: %s\n", SDL_GetPixelFormatName(pattern->format->format), SDL_GetPixelFormatName(formats[i]), SDL_GetError());
            result = -1;
            break;
        }
        if (!verify_yuv_data(formats[i], yuv1, yuv1_pitch, pattern)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed conversion from RGB to %s\n", SDL_GetPixelFormatName(formats[i]));
            result = -1;
            break;
        }
    }


    /* Verify conversion between YUV formats */
    for (i = 0; i < (int) SDL_arraysize(formats); ++i) {
        for (j = 0; j < (int) SDL_arraysize(formats); ++j) {
            yuv1_pitch = CalculateYUVPitch(formats[i], pattern->w) + extra_pitch;
            yuv2_pitch = CalculateYUVPitch(formats[j], pattern->w) + extra_pitch;
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
            if (!verify_yuv_data(formats[j], yuv2, yuv2_pitch, pattern)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed conversion from %s to %s\n", SDL_GetPixelFormatName(formats[i]), SDL_GetPixelFormatName(formats[j]));
                result = -1;
                break;
            }
        }
    }

    /* Verify conversion between YUV formats in-place */
    for (i = 0; i < (int) SDL_arraysize(formats); ++i) {
        for (j = 0; j < (int) SDL_arraysize(formats); ++j) {
            if (is_packed_yuv_format(formats[i]) != is_packed_yuv_format(formats[j])) {
                // Can't change plane vs packed pixel layout in-place
                continue;
            }

            yuv1_pitch = CalculateYUVPitch(formats[i], pattern->w) + extra_pitch;
            yuv2_pitch = CalculateYUVPitch(formats[j], pattern->w) + extra_pitch;
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
            if (!verify_yuv_data(formats[j], yuv1, yuv2_pitch, pattern)) {
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
SDL_bool LibAVable::is_packed_yuv_format(Uint32 format) {
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

SDL_bool LibAVable::verify_yuv_data(Uint32 format, const Uint8 *yuv, int yuv_pitch, SDL_Surface *surface) {
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



#endif //SDL_CRT_FILTER_LIBAVABLE_HPP
