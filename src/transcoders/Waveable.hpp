//
// Created by sebastian on 23/5/20.
//

#ifndef SDL_CRT_FILTER_WAVEABLE_HPP
#define SDL_CRT_FILTER_WAVEABLE_HPP

#include <transcoders/Transcodeable.hpp>
#include <transcoders/Pixelable.hpp>

class Waveable: public Transcodeable {
protected:
    static const uint8_t MAX_WHITE_LEVEL = 200;
    static inline uint8_t wave_median(Uint32& px);
    static inline uint8_t to_wave(Uint32 px) { return wave_median(px); }

    static inline Uint32 px_median(uint8_t& wav);
    static inline Uint32 to_pixel( uint8_t wav )  { return px_median(wav); }
public:
    static void encode(void* dst, void* src);
    static void decode(void* dst, void* src);
    static size_t conversion_size( SDL_Surface* src ) { return src->h * src->w; }
};

class WaveLuma: public Waveable {
protected:
    static inline uint8_t wave_luma(Uint32& px);
    static inline Uint32 px_luma(uint8_t& wav);
    static inline Uint32 to_pixel( uint8_t wav )  { return px_luma(wav); }
    static inline uint8_t to_wave(Uint32 px) { return wave_luma(px); }
};


uint8_t Waveable::wave_median(Uint32 &px) {
    Uint32 R, G, B;
    Pixelable::toComponents(&px, &R, &G, &B);
    auto media = static_cast<Uint32>(R + G + B) / 3;
    return (uint8_t) media;
}

uint8_t WaveLuma::wave_luma(Uint32 &px) {
    Uint32 R, G, B;
    Pixelable::toComponents(&px, &R, &G, &B);
    double luma = Pixelable::luma( &R, &G , &B);
    return (uint8_t) MAX_WHITE_LEVEL * luma;
}

Uint32 Waveable::px_median(uint8_t &wav) {
    auto comp = static_cast<Uint32>(wav);
    Uint32 pixel = 0;
    Pixelable::fromComponents( &pixel, &comp, &comp, &comp );
    return pixel;
}

Uint32 WaveLuma::px_luma(uint8_t &wav) {
    double luma = (double) wav / MAX_WHITE_LEVEL;
    Uint32 R = 0, G = 0, B = 0;
    Pixelable::RGB(&luma, &R, &G, &B);
    Uint32 pixel = 0;
    Pixelable::fromComponents( &pixel, &R, &G, &B );
    return pixel;
}

//surface to wave
void Waveable::decode(void *dst, void* src) {
    auto source_surface = static_cast<SDL_Surface*>(src);
    auto destination_wave = static_cast<uint8_t*>(dst);

    for (int x = 0; x < source_surface->w; ++x)
        for (int y = 0; y < source_surface->h; ++y) {
            destination_wave[( y * source_surface->w ) + x] =
                    to_wave( Pixelable::get32(source_surface, x, y) );
        }
}

//wave to surface
void Waveable::encode(void *dst, void *src) {
    auto source_wave = static_cast<uint8_t *>(src);
    auto destination_surface = static_cast<SDL_Surface*>(dst);
    for (int x = 0; x < destination_surface->w; ++x) {
        for (int y = 0; y < destination_surface->h; ++y) {
            Pixelable::put32(
                    destination_surface, x, y,
                    to_pixel( source_wave[( y * destination_surface->w ) + x] )
            );
        }
    }
}


#endif //SDL_CRT_FILTER_WAVEABLE_HPP
