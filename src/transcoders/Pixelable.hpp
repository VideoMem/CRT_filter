//
// Created by sebastian on 23/5/20.
//

#ifndef SDL_CRT_FILTER_PIXELABLE_HPP
#define SDL_CRT_FILTER_PIXELABLE_HPP

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

};

#endif //SDL_CRT_FILTER_PIXELABLE_HPP
