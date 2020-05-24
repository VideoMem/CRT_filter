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

    inline static double luma(Uint32 *R, Uint32 *G, Uint32 *B) {
        return 0.299 * fromChar(R) + 0.587 * fromChar(G) + 0.114 * fromChar(B);
    }

    inline static void RGB(const double *luma, Uint32 *R, Uint32 *G, Uint32 *B) {
        const double DbDr = 0.0;
        RGB(luma, &DbDr, &DbDr, R, G, B);
    }

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

};

#endif //SDL_CRT_FILTER_PIXELABLE_HPP
