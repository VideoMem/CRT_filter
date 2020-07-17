//
// Created by sebastian on 08/07/2020.
//

#ifndef SDL_CRT_FILTER_TURBOFEC_HPP
#define SDL_CRT_FILTER_TURBOFEC_HPP
#include <SDL2/SDL.h>
extern "C" {
    #include <turbofec/turbo.h>
}

/* Maximum LTE code block size of 6144 */
#define LEN		TURBO_MAX_K
#define DEFAULT_ITER		4

class TurboFEC {
public:
    static uint8_t** Allocate( int buflen );
    static uint8_t** Allocate( SDL_Surface* ref );
    static void free ( uint8_t** b );
    static void encode( uint8_t** buff, SDL_Surface* src );
    static void decode( SDL_Surface* dst, uint8_t** buff );
    static inline int term_size() { return 4; };
    static inline size_t bits ( size_t bytes ) { return bytes * 8 * sizeof(uint8_t); }
    static inline size_t bytes( size_t bits ) { return bits / 8 * sizeof(uint8_t); }

    //nearest surface size from input frame size
    static inline int conv_size( SDL_Rect* ref ) {

        auto scale = bits(1);
        auto bits = ref->w * ref->h * scale;
        bits += term_size() * bits / LEN;
        auto bit_remainder = bits % scale;
        bits += ( scale - bit_remainder );
        assert( bits % scale == 0 && "Incompatible frame size bit expansion" );

        auto bytes = bits / scale;
        auto remainder = bytes % ref->w;
        int ret_size = (ref->w - remainder) + bytes;
        if ( ret_size % ref->w != 0 ) {
            SDL_Log("Non partitionable in screen lines! %d % %d != 0", ret_size, ref->w);
            assert(false);
        }
        return ret_size;
    }

    //size bytes
    static inline int conv_size( SDL_Surface* ref ) {
        SDL_Rect rect;
        SDL_GetClipRect( ref, &rect );
        return conv_size(&rect);
    }

    //size frame
    static inline SDL_Rect conv_rect( SDL_Surface* ref ) {
        SDL_Rect rect;
        SDL_GetClipRect( ref, &rect );
        size_t nearest = conv_size(&rect);
        return {
            .x=0,
            .y=0,
            .w=ref->w,
            .h= static_cast<int>(nearest / ref->w)
        };
    }

    //size bytes
    static inline int conv_size( size_t size, size_t width=Config::SCREEN_WIDTH ) {
        auto height = size / width;
        SDL_Rect rect { .x = 0, .y= 0, .w = static_cast<int>(width), .h = static_cast<int>(height) };
        return conv_size( &rect );
    }


    static inline int conv_size_bits( SDL_Surface * ref ) { return bits ( conv_size( ref ) ); }
    static void tobits( uint8_t *dst, const uint8_t *b, int n );
    static void frombits( uint8_t *dst, uint8_t *b, int n );

public:
    static const struct lte_turbo_code lte_turbo() {
        return {
                .n = 2,
                .k = 4,
                .len = LEN,
                .rgen = 013,
                .gen = 015
        };
    }
};


uint8_t** TurboFEC::Allocate(SDL_Surface *ref) {
    return Allocate ( conv_size_bits(ref) );
}

void TurboFEC::free( uint8_t** b ) {
    for( int i = 0; i < 3; ++i ) {
        delete [] b[i];
    }
}

uint8_t **TurboFEC::Allocate(int buflen) {
    uint8_t** b = new uint8_t*[3] {
            new uint8_t[buflen],
            new uint8_t[buflen],
            new uint8_t[buflen]
    };
    memset( b[0], 0, sizeof( uint8_t ) * buflen );
    memset( b[1], 0, sizeof( uint8_t ) * buflen );
    memset( b[2], 0, sizeof( uint8_t ) * buflen );
    return b;
}

//src surface -> lumachannelmatrix -> tobits -> turbo encoder -> buff[] (??)
void TurboFEC::encode(uint8_t **buff, SDL_Surface *src) {

    auto in = Pixelable::AsLumaChannelMatrix( src );
    auto in_bits = new uint8_t[ conv_size_bits( src ) ];
    static const int bit_chunks = conv_size_bits( src ) / LEN;
    auto lte_t = lte_turbo();

    tobits( in_bits, in, conv_size_bits( src ) );
    for ( size_t chunk_pos=0; chunk_pos < bit_chunks; chunk_pos ++ ) {
        auto in_bit_offset = chunk_pos * LEN;
        auto bit_offset = chunk_pos * (LEN + term_size());
        lte_turbo_encode( &lte_t,
                &in_bits[ in_bit_offset ],
                buff[0] + bit_offset,
                buff[1] + bit_offset,
                buff[2] + bit_offset );
    }
    delete [] in_bits;
    delete [] in;
}

//buff[] -> turbo decode -> frombits -> applylumachannelmatrix -> dst surface
void TurboFEC::decode( SDL_Surface *dst, uint8_t **buff ) {
    auto out_bits = new uint8_t[ conv_size_bits( dst ) ];
    static const int bit_chunks = conv_size_bits( dst ) / LEN;
    struct tdecoder *tdec = alloc_tdec();
    int8_t *bs0, *bs1, *bs2;
    bs0 = (int8_t*) buff[0];
    bs1 = (int8_t*) buff[1];
    bs2 = (int8_t*) buff[2];
    auto iter = DEFAULT_ITER;
    for ( size_t chunk_pos=0; chunk_pos < bit_chunks; chunk_pos ++ ) {
        auto in_bit_offset = chunk_pos * LEN;
        auto bit_offset = chunk_pos * (LEN + term_size());
        lte_turbo_decode_unpack(tdec, LEN, iter,
                &out_bits[in_bit_offset],
                bs0 + bit_offset,
                bs1 + bit_offset,
                bs2 + bit_offset);
    }
    auto out = new uint8_t [ conv_size( dst ) ];
    frombits( out, out_bits, conv_size_bits( dst ) );
    Pixelable::ApplyLumaChannelMatrix( dst, out );
    delete [] out;
    delete [] out_bits;
}

//n bits to uint8_t array b[0] = 0xFF as, dst[0] = 1, dst[1] = 1, dst[2] = 1 and so on, up to n
void TurboFEC::tobits( uint8_t *dst, const uint8_t *b, int n ) {
    int m = sizeof(uint8_t) * 8;
    uint8_t r = b[0];

    int j = 0;
    for ( int i = 0; i < n; i++ ) {
        if ( i % m == 0 ) {
            r = b[j];
            ++j;
        }
        dst[i] = (r >> (i % m)) & 0x01;
    }

}

//
void TurboFEC::frombits( uint8_t *dst, uint8_t *b, int n ) {
    int m = sizeof(uint8_t) * 8;
    uint8_t r = 0;

    int j = 0;
    for ( int i = 0; i < n; i++ ) {
        if ( i != 0 && i % m == 0 ) {
            dst[j] = r;
            ++j;
            r = 0;
        }
        r |= (b[i] & 0x01) << (i % m);
    }

}


#endif //SDL_CRT_FILTER_TURBOFEC_HPP
