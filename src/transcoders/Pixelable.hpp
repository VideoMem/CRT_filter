//
// Created by sebastian on 23/5/20.
//

#ifndef SDL_CRT_FILTER_PIXELABLE_HPP
#define SDL_CRT_FILTER_PIXELABLE_HPP

#include "Surfaceable.hpp"

class Pixelable: public Surfaceable {
public:
    //TODO: bit endianess
    inline static void toComponents(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B, Uint32 *A) {
        *A = (*pixel & mask_t::a) >> 24;
        toComponents( pixel, R, G, B );
    }

    inline static void toComponents(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B) {
        *B = (*pixel & mask_t::b) >> 16;
        *G = (*pixel & mask_t::g) >> 8;
        *R = *pixel & mask_t::r ;
    }

    //TODO: bit endianess
    inline static void fromComponents(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B) {
        *pixel = ((*B << 16) + (*G << 8) + *R) | mask_t::a;
    }

    inline static void fromComponents(Uint32 *pixel, Uint32 *R, Uint32 *G, Uint32 *B, Uint32 *A) {
        *pixel = ((*A << 24) + (*B << 16) + (*G << 8) + *R);
    }

    inline static Uint32 get32(SDL_Surface *surface, int x, int y) {
        assert(surface->format->BitsPerPixel == 32 && "Bits per pixel mismatch, 32 required");
        auto *pixels = (Uint32 *)surface->pixels;
        return pixels[ ( y * surface->w ) + x ];
    }

    inline static void put32(SDL_Surface *surface, int x, int y, Uint32 pixel) {
        assert(surface->format->BitsPerPixel == 32 && "Bits per pixel mismatch, 32 required");
        auto *pixels = (Uint32 *)surface->pixels;
        pixels[ ( y * surface->w ) + x ] = pixel;
    }

    inline static void copy32( SDL_Surface *dst, SDL_Surface *src, int x, int y ) {
        copy32( dst, src, x, y, x, y );
    }

    inline static void copy32( SDL_Surface *dst, SDL_Surface *src, int x, int y, int x0, int y0 ) {
        Uint32 pixel;
        pixel = get32( src , x, y );
        put32( dst, x0, y0, pixel );
    }

    inline static double  fromChar(Uint32* c)  { return (double) *c / 0xFF; }
    inline static Uint32 toChar (double* comp) { return *comp < 1? 0xFF * *comp: 0xFF; }

    inline static void RGB(const double *luma, Uint32 *R, Uint32 *G, Uint32 *B) {
        const double DbDr = 0.0;
        RGB(luma, &DbDr, &DbDr, R, G, B);
    }

    inline static double luma(Uint32 *R, Uint32 *G, Uint32 *B) {
        return 0.299 * fromChar(R) + 0.587 * fromChar(G) + 0.114 * fromChar(B);
    }

    //wolfram alpha: invert {{0.299, 0.587, 0.114},{-0.450, -0.883, 1.333},{-1.333,1.116,0.217}}
    inline static void RGB(const double *luma, const double *Db, const double *Dr, Uint32 *R, Uint32 *G, Uint32 *B) {
        double fR = *luma + 0.0000923037 * *Db - 0.525913          * *Dr;
        double fG = *luma - 0.129133     * *Db + 0.267899328207599 * *Dr;
        double fB = *luma + 0.664679     * *Db - 0.0000792025      * *Dr;
        *R = toChar(&fR);
        *G = toChar(&fG);
        *B = toChar(&fB);
    }

    inline static double direct_to_luma( Uint32 pixel ){
        Uint32 R = 0, G = 0, B = 0;
        toComponents( &pixel, &R, &G, &B);
        return luma( &R, &G, &B );
    }

    inline static double direct_to_luma( SDL_Surface* src, int x, int y ){
        Uint32 pixel = get32(src, x, y);
        return direct_to_luma( pixel );
    }

    inline static Uint32 direct_from_luma( double luma ){
        Uint32 pixel = 0, R = 0, G = 0, B = 0;
        RGB( &luma, &R, &G, &B );
        fromComponents( &pixel, &R, &G, &B);
        return pixel;
    }


    inline static void chroma(double *Db, double *Dr, Uint32 *R, Uint32 *G, Uint32 *B) {
        *Db = -0.450 * fromChar(R) - 0.883 * fromChar(G) + 1.333 * fromChar(B);
        *Dr = -1.333 * fromChar(R) + 1.116 * fromChar(G) + 0.217 * fromChar(B);
    }

    static double surface_diff( SDL_Surface *sample, SDL_Surface* copy ) {
        double diff = 0;
        double integrated_error = 0;
        int pixels = 0;
        Uint32 R[2] = { 0 }, G[2] = { 0 }, B[2] = { 0 };
        Uint32 pixel[2] = { 0 };
        double luminance[2] = { 0 };
        for ( int y = 0; y < sample->h; ++y ) {
            for( int x= 0; x < sample->w; ++x ) {
                pixel[0] = get32( sample, x, y );
                pixel[1] = get32( copy  , x, y );
                toComponents( &pixel[0], &R[0], &G[0], &B[0] );
                toComponents( &pixel[1], &R[1], &G[1], &B[1] );
                luminance[0] = luma( &R[0], &G[0], &B[0] );
                luminance[1] = luma( &R[1], &G[1], &B[1] );
                integrated_error += luminance[0] - luminance[1];
                pixels++;
            }
        }
        diff = integrated_error / pixels;

        return diff;
    }

    static double psnr(SDL_Surface *original, SDL_Surface *copy) {
        double sum = 0;
        for( int y = 0; y < copy->h; ++y )
            for( int x = 0; x < copy->w; ++x ) {
                sum += pow( ( direct_to_luma( original , x, y ) - direct_to_luma( copy , x, y ) ), 2 );
            }
        double media = sum / ( copy->w * copy->h );
        if (media <= 0.001) return 100;
        double PIXEL_MAX = 255.0;
        return 20 * log( PIXEL_MAX / sqrt(media) ) / log(10);
    }

    /*
     *  3.1. Overall PSNR
        PSNR is a traditional signal quality metric, measured in decibels. It is directly drived from mean square error (MSE), or its square root (RMSE). The formula used is:
        20 * log10 ( MAX / RMSE )
        or, equivalently:
        10 * log10 ( MAX^2 / MSE )
        where the error is computed over all the pixels in the video, which is the method used in the dump_psnr.c reference implementation.
        This metric may be applied to both the luma and chroma planes, with all planes reported separately.

     *  https://tools.ietf.org/id/draft-ietf-netvc-testing-06.html
    */

    static void psnr(SDL_Surface *original, SDL_Surface *copy, SDL_Surface *error) {
        double err = 0;
        double max = 0;
        double min = 1e6;
        for( int y = 0; y < copy->h; ++y )
            for( int x = 0; x < copy->w; ++x ) {
                err = pow( ( direct_to_luma( original , x, y ) - direct_to_luma( copy , x, y ) ), 2 );
                if ( err > max ) max = err;
                if ( err < min ) min = err;
            }

        double PIXEL_MAX = 255.0;
        for( int y = 0; y < copy->h; ++y )
            for( int x = 0; x < copy->w; ++x ) {
                err = pow( ( direct_to_luma( original , x, y ) - direct_to_luma( copy , x, y ) ), 2 );
                Uint32 pixel = mask_t::a + mask_t::c;
                if (err > 0.001) {
                    double err_normal = (err - min) / (max - min);
                    double snr = 20 * log( PIXEL_MAX / sqrt(err_normal) ) / log(10);
                    pixel = direct_from_luma( snr / 100 );
                }
                put32(error, x, y, pixel);
            }
    }

    static void DrawLine(SDL_Surface* dst, float x1, float y1, float x2, float y2, Uint32 color) {
        // Bresenham's line algorithm
        const bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
        if(steep) {
            std::swap(x1, y1);
            std::swap(x2, y2);
        }

        if(x1 > x2) {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }

        const float dx = x2 - x1;
        const float dy = fabs(y2 - y1);

        float error = dx / 2.0f;
        const int ystep = (y1 < y2) ? 1 : -1;
        int y = (int)y1;

        const int maxX = (int)x2;

        for(int x=(int)x1; x<maxX; x++) {
            if(steep) {
                put32( dst, y, x, color);
            }
            else {
                put32( dst, x, y, color);
            }

            error -= dy;
            if(error < 0) {
                y += ystep;
                error += dx;
            }
        }
    }

    static void DrawRect(SDL_Surface* dst, SDL_Rect* box, Uint32 color) {
        float x0 = box->x;
        float y0 = box->y;
        float x1 = box->x + box->w - 1;
        float y1 = y0;
        float x2 = x1;
        float y2 = box->y + box->h - 1;
        float x3 = x0;
        float y3 = y2;
        DrawLine( dst, x0, y0, x1, y1, color );
        DrawLine( dst, x1, y1, x2, y2, color );
        DrawLine( dst, x2 + 1, y2, x3, y3, color );
        DrawLine( dst, x3, y3, x0, y0, color );
    }

    static size_t pixels( SDL_Surface* ref ) { return ref->w * ref->h; }

    static Uint8* AllocateChannelMatrix ( SDL_Surface* ref ) {
        size_t buflen = pixels( ref );
        return (uint8_t *) malloc(sizeof(uint8_t) * buflen);
    }

    static double* AllocateFloatMatrix ( SDL_Surface* ref ) {
        size_t buflen = pixels ( ref );
        return new double[buflen];
    }

    static double* AsLumaFloatMatrix ( SDL_Surface* ref ) {
        auto fm = AllocateFloatMatrix( ref );
        size_t pos = 0;
        for( int y = 0; y < ref->h; ++y )
            for( int x = 0; x < ref->w; ++x ) {
                fm[pos] = direct_to_luma( get32( ref, x, y ) );
                pos++;
            }

        return fm;
    }

    // -1.0 to 1.0 range
    static inline uint8_t double_to_uint8( double real ) {
        if ( real >= 1 )  return 0xFF;
        if ( real <= -1 ) return 0;
        double shift = (real + 1) / 2;
        return shift * 0x100;
    }

    // -1.0 to 1.0 range
    static inline double uint8_to_double( uint8_t quant ) {
        double dequant = (double) quant / 0xFF;
        return (dequant * 2) - 1;
    }

    // 0.0 to 1.1 range
    static inline uint8_t float_to_uint8( float real ) {
        if ( real >= 1 )  return 0xFF;
        if ( real <= 0 ) return 0;
        return real * 0x100;
    }

    // 0.0 to 1.0 range
    static inline float uint8_to_float( uint8_t quant ) {
        return (float) quant / 0xFF;
    }

    static uint8_t* AsLumaChannelMatrix ( SDL_Surface* ref ) {
        size_t buflen = pixels( ref );
        auto cm = new Uint8[buflen];
        auto fm = AsLumaFloatMatrix( ref );
        size_t pos = 0;
        for( int y = 0; y < ref->h; ++y )
            for( int x = 0; x < ref->w; ++x ) {
                cm[pos] = float_to_uint8(fm[pos]);
                pos++;
            }

        delete [] fm;
        return cm;
    }

    static void ApplyLumaChannelMatrix ( SDL_Surface* ref, Uint8* cm ) {
        size_t pos = 0;
        for( int y = 0; y < ref->h; ++y )
            for( int x = 0; x < ref->w; ++x ) {
                put32( ref, x, y, direct_from_luma(uint8_to_float(cm[pos])) );
                pos++;
            }

    }

};

#endif //SDL_CRT_FILTER_PIXELABLE_HPP
