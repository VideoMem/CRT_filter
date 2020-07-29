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
#define LEN		        TURBO_MAX_K
#define DEFAULT_ITER    4
#define DEFAULT_BITDEPTH 4

typedef std::vector<Pixelable_ch_t> Turbofec_bitvect_t;

struct TurboFEC_bitbuff_t {
    uint8_t ** turbo;
    size_t size;
};

struct TurboFEC_mtu_t {
    int input_bits;
    int padding;
    int output_bits;
};

struct TurboFEC_framemetadata_t {
    unsigned long long frame_id;
};

class TurboFEC {
public:
    static TurboFEC_bitbuff_t& AllocateC( size_t buflen );
    static uint8_t** Allocate( int buflen );
    static TurboFEC_bitbuff_t & Allocate( SDL_Surface* ref );
    static void free ( uint8_t** b );
    static void encode(uint8_t** buff, uint8_t* in_bits, size_t len_bits );
    static void encode( uint8_t** buff, SDL_Surface* src );
    static void decode( uint8_t* dst_bits, uint8_t** buff, size_t len_bits );
    static void decode( SDL_Surface* dst, uint8_t** buff );
    static inline int term_size() { return 4; };
    static inline size_t bits ( size_t bytes ) { return bytes * 8 * sizeof(uint8_t); }
    static inline size_t bytes( size_t bits ) { return bits / 8 * sizeof(uint8_t); }
    //changes from uint8_t bit representation to n bit length rearrange
    static size_t bitdownquant( uint8_t **dst, uint8_t **src, int bitlen, size_t read_size );
    static void single_bitdownquant(uint8_t *dst, uint8_t *src, int bitlen, size_t read_size );
    static void bitupquant( uint8_t **dst, uint8_t **src, int bitlen, size_t write_size );
    static void single_bitupquant( uint8_t *dst, uint8_t *src, int bitlen, size_t write_size );

    //all size units bits
    static size_t bigger_size(int bitlen, size_t read_size ) { return  bits(1) * read_size / bitlen; };
    static size_t smaller_size(int bitlen, size_t int_count ) { return int_count * bitlen / bits(1); };
    static size_t downquant_size(int bitlen, size_t input_size ) {
        return bigger_size(bitlen, input_size);
    }
    static size_t upquant_size(int bitlen, size_t int_count ) {
        auto size = smaller_size(bitlen, int_count);
        if( size % bits(1) > bits(1) / 2 )
            return size + bits(1) - size % bits(1);
        else
            return size - bits(1) + size % bits(1);
    }

    static void AsChannelMatrix(uint8_t *cm, TurboFEC_bitbuff_t &b);
    static void FromChannelMatrix(TurboFEC_bitbuff_t &b, uint8_t *cm);
    //nearest surface size in bytes from input rect size
    static inline int conv_size( SDL_Rect* ref );
    //size bytes
    static inline int conv_size( SDL_Surface* ref );
    //size frame
    static inline SDL_Rect conv_rect( SDL_Surface* ref );
    //size bytes
    static inline int conv_size( size_t size, size_t width=Config::NKERNEL_WIDTH );

    static inline int conv_size_bits( SDL_Surface * ref ) { return bits ( conv_size( ref ) ); }
    static void tobits(Pixelable_ch_t &dst, Pixelable_ch_t &src );
    static void tobits( uint8_t *dst, const uint8_t *b, int n );
    static void frombits(Pixelable_ch_t &dst, Pixelable_ch_t &src );
    static void frombits( uint8_t *dst, uint8_t *b, int n );
    static inline int output_bits_per_packet() { return 3 * ( LEN + term_size() ); };
    static inline int packets( int ref ) { return floor((double)ref / output_bits_per_packet() ); };

    //it returns how many input bits are for those output bit space
    // output_bits = 3 * ( input_bits  + term_size ) //as found in turbo_test.c from turbofec library
    // (output_bits/3) - 4 = input_bits
    static inline int input_bits( int output_bits ) {
        if ( output_bits < output_bits_per_packet() ) {
            auto retvalue = (output_bits/3) -  term_size();
            if ( retvalue % 8 != 0 )
                return input_bits(--output_bits);
            else
                return retvalue;
        } else {
            return LEN * packets( output_bits );
        }
    }

    static inline int input_bytes( int output_bytes ) {
        return bytes( input_bits( bits(output_bytes) ) );
    }

    static inline int output_bits ( int input_bits ) {
        return 3 * (input_bits + term_size());
    }

    // maximum transfer unit in bits for a given size in ref bits
    // for N len bits as ref
    // output_bits_per_packet = LEN * 3 + 3 * term_size;
    static TurboFEC_mtu_t mtu( size_t ref, size_t max ) {
        TurboFEC_mtu_t size = {};
        size.input_bits = input_bits(ref);
        size.output_bits = ref;
        size.padding = (int)max - size.output_bits;

        if( size.padding % 8 != 0 || size.output_bits % 8 != 0 || size.input_bits % 8 != 0
           || size.output_bits % 3 != 0 || size.output_bits % output_bits_per_packet() != 0 ) {
            //"multiplier check failed" );
            return mtu( --ref, max );
        } else {
            return {size.input_bits, size.padding, size.output_bits };
        }
    }

    static TurboFEC_mtu_t mtu( size_t ref ) {
        return mtu( ref, ref );
    }

    static TurboFEC_mtu_t mtu_downquant( size_t available_bits, size_t bitlen ) {
        return mtu( TurboFEC::upquant_size( bitlen, available_bits ) );
    }

    // Recursive function to return gcd of a and b
    static int gcd(int a, int b)    {
        // Everything divides 0
        if (a == 0)
            return b;
        if (b == 0)
            return a;

        // base case
        if (a == b)
            return a;

        // a is greater
        if (a > b)
            return gcd(a-b, b);
        return gcd(a, b-a);
    }

    static void add_frame_metadata(uint8_t *cm, TurboFEC_framemetadata_t &md, int padsize);
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


    static void dump_partition(uint8_t *in, size_t start=0, size_t end=64);
    static int verbose_memcmp(uint8_t *p1, uint8_t *p2, size_t size );
};


TurboFEC_bitbuff_t & TurboFEC::Allocate(SDL_Surface *ref ) {
    return AllocateC( conv_size_bits(ref) );
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

//size bytes
TurboFEC_bitbuff_t& TurboFEC::AllocateC( size_t buflen ) {
    auto bitbuff = new TurboFEC_bitbuff_t {
        Allocate( buflen ),
        buflen
    };
    return *bitbuff;
}


// bits -> turbo encoder -> buff[] (turbo)
void TurboFEC::encode( uint8_t** buff, uint8_t* in_bits, size_t len_bits ) {
    static const unsigned int packets = floor((double)len_bits / LEN);
    auto lte_t = lte_turbo();

    if(packets > 0 ) {
        for (size_t packet = 0; packet < packets; packet++) {
            auto in_bit_offset = packet * LEN;
            auto bit_offset = packet * (LEN + term_size());
            lte_turbo_encode(&lte_t,
                             &in_bits[in_bit_offset],
                             &buff[0][bit_offset],
                             &buff[1][bit_offset],
                             &buff[2][bit_offset]);
        }
    } else {
        lte_t.len = len_bits;
        lte_turbo_encode(&lte_t,
                         &in_bits[0],
                         &buff[0][0],
                         &buff[1][0],
                         &buff[2][0]);
    }

}

//src surface ->  lumachannelmatrix -> tobits -> above encoder
void TurboFEC::encode(uint8_t **buff, SDL_Surface *src) {

    auto in = Pixelable::AsLumaChannelMatrix( src );
    auto in_bits = new uint8_t[ conv_size_bits( src ) ];
    //static const int bit_chunks = conv_size_bits( src ) / LEN;

    tobits( in_bits, in, conv_size_bits( src ) );
    encode( buff, in_bits, conv_size_bits( src ) );
    delete [] in_bits;
    delete [] in;
}

//turbo decode from buff to out_bits by len_bits
void TurboFEC::decode( uint8_t* out_bits, uint8_t** buff, size_t len_bits ) {
    static const unsigned packets = len_bits / LEN;
    struct tdecoder *tdec = alloc_tdec();
    int8_t *bs0, *bs1, *bs2;
    bs0 = (int8_t*) buff[0];
    bs1 = (int8_t*) buff[1];
    bs2 = (int8_t*) buff[2];
    auto iter = DEFAULT_ITER;

    if(packets > 0 ) {
        for (size_t packet=0; packet < packets; packet ++ ) {
            auto in_bit_offset = packet * LEN;
            auto bit_offset = packet * (LEN + term_size());
            lte_turbo_decode_unpack(tdec, LEN, iter,
                                    &out_bits[in_bit_offset],
                                    &bs0[ bit_offset ],
                                    &bs1[ bit_offset ],
                                    &bs2[ bit_offset ]);
        }
    } else {
        lte_turbo_decode_unpack(tdec, len_bits, iter,
                                &out_bits[0],
                                &bs0[0],
                                &bs1[0],
                                &bs2[0]);
    }
    free_tdec(tdec);
}

//turbo decode -> frombits -> applylumachannelmatrix -> dst surface
void TurboFEC::decode( SDL_Surface *dst, uint8_t **buff ) {
    auto out = new uint8_t [ conv_size( dst ) ];
    auto out_bits = new uint8_t[ conv_size_bits( dst ) ];
    decode( out_bits, buff, conv_size_bits( dst ) );
    frombits( out, out_bits, conv_size_bits( dst ) );
    Pixelable::ApplyLumaChannelMatrix( dst, out );
    delete [] out;
    delete [] out_bits;
}


void TurboFEC::tobits(Pixelable_ch_t &dst, Pixelable_ch_t &src ) {
    Pixelable_ch_t temp(src.size() * bytes(1), 0);
    auto srcp = &src[0];
    auto dstp = &temp[0];
    tobits(dstp, srcp, src.size());
    dst.clear();
//    std::copy(temp.begin(), temp.end(), dst);
}

// Convert n bits to uint8_t array
// b[0] = 0xFF as, dst[0] = 1, dst[1] = 1, dst[2] = 1, etc ..
// up to n (usually 8)
void TurboFEC::tobits( uint8_t *dst, const uint8_t *b, int n ) {
    int m = sizeof(uint8_t) * bits(1);
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

void TurboFEC::frombits(Pixelable_ch_t &dst, Pixelable_ch_t &src ) {
    Pixelable_ch_t temp(src.size() * bytes(1), 0);
    auto srcp = &src[0];
    auto dstp = &temp[0];
    frombits(dstp, srcp, src.size());
    dst.clear();
//    std::copy(temp.begin(), temp.end(), dst);
}

//same as above but in reverse
void TurboFEC::frombits( uint8_t *dst, uint8_t *b, int n ) {
    int m = sizeof(uint8_t) * bits(1);
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


void TurboFEC::single_bitdownquant(uint8_t *dst, uint8_t *src, int bitlen, size_t read_size) {
    auto shift = bits(1) - bitlen;
    for (size_t i = 0; i < read_size; i += bitlen) {
        auto block = i / bitlen;
        auto bi = block * bits(1);
        for (int b = 0; b < bitlen; b++) {
            auto bit = src[i + b] & 0x01;
            dst[shift + bi + b] = bit;
        }
    }
}

// From src array to dst array (read_size) elements.
// Quantize each to unsigned bitlen integer, then store it on
// dst as uint8_t, read size as bits
size_t TurboFEC::bitdownquant(uint8_t **dst, uint8_t **src, int bitlen, size_t read_size) {
    size_t int_slots = bigger_size(bitlen, read_size);
    for(int q=0; q < 3; ++q )
        single_bitdownquant(dst[q], src[q], bitlen, read_size );
    auto log = downquant_size( bitlen, read_size );
    //SDL_Log( "int_slots %zu, downquant %zu", int_slots, log );
    assert( int_slots == log );
    return int_slots;
}

//same as above, but in reverse
void TurboFEC::single_bitupquant(uint8_t *dst, uint8_t *src, int bitlen, size_t write_size) {
    auto shift = bits(1) - bitlen;
    for(size_t i=0; i < write_size; i+=bitlen ) {
        auto block = i / bitlen;
        auto bi = block * bits(1);
        for (int b = 0; b < bitlen; b++) {
            auto bit = src[shift + bi + b] & 0x01;
            dst[i + b] = bit;
        }
    }
}

void TurboFEC::bitupquant(uint8_t **dst, uint8_t **src, int bitlen, size_t write_size) {
    for(int q=0; q < 3; ++q ) {
        single_bitupquant(dst[q], src[q], bitlen, write_size );
    }
}

//It rounds up a byte size to the nearest byte size surface as big as supplied in ref
int TurboFEC::conv_size(SDL_Rect *ref) {

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

int TurboFEC::conv_size(SDL_Surface *ref) {
    SDL_Rect rect;
    SDL_GetClipRect( ref, &rect );
    return conv_size( &rect );
}

SDL_Rect TurboFEC::conv_rect(SDL_Surface *ref) {
    SDL_Rect rect;
    SDL_GetClipRect( ref, &rect );
    size_t nearest = conv_size( &rect );
    return {
            .x=0,
            .y=0,
            .w=ref->w,
            .h= static_cast<int>(nearest / ref->w)
    };
}

int TurboFEC::conv_size(size_t size, size_t width) {
    auto height = size / width;
    SDL_Rect rect { .x = 0, .y= 0, .w = static_cast<int>(width), .h = static_cast<int>(height) };
    return conv_size( &rect );
}

void TurboFEC::AsChannelMatrix(uint8_t *cm, TurboFEC_bitbuff_t &b ) {
    size_t size_bits = b.size;
    auto in = Allocate( bytes(size_bits) );
    frombits( &in[0][0], b.turbo[0], b.size );
    frombits( &in[1][0], b.turbo[1], b.size );
    frombits( &in[2][0], b.turbo[2], b.size );
    //size_t pos[3] = { 0, bytes( size_bits ) / 3, 2 * bytes( size_bits ) / 3 };
    for(size_t i=0; i < bytes( input_bits(size_bits) ); i++ ) {
        auto id_dst = i * 3;
        cm[id_dst]     = in[0][i];
        cm[id_dst + 1] = in[1][i];
        cm[id_dst + 2] = in[2][i];
    }

    delete [] in;
}

void TurboFEC::FromChannelMatrix( TurboFEC_bitbuff_t &b, uint8_t *cm ) {
    size_t size_bits = b.size;
    auto in = Allocate( bytes(size_bits) );
    //size_t pos[3] = { 0, bytes( size_bits ) / 3, 2 * bytes( size_bits ) / 3 };
    for(size_t i=0; i < bytes( input_bits(size_bits) ) ; i++) {
        auto id_dst = i * 3;
        in[0][i] = cm[id_dst];
        in[1][i] = cm[id_dst + 1];
        in[2][i] = cm[id_dst + 2];
    }
    tobits( &b.turbo[0][0], in[0], b.size );
    tobits( &b.turbo[1][0], in[1], b.size );
    tobits( &b.turbo[2][0], in[2], b.size );

    delete [] in;
}

//this commented log
void TurboFEC::add_frame_metadata(uint8_t *cm, TurboFEC_framemetadata_t &md, int padsize_bits ) {
    auto padsize = input_bits( padsize_bits );
    SDL_Log("adding %d padsize (input) bits at end of the channel matrix buffer cm", padsize );
    assert( padsize <= LEN && "frame metadata is expected to occupy less than a whole packet" );
    assert( padsize > 0 && "no space left for frame metadata");
    assert( bytes(padsize) > sizeof( TurboFEC_framemetadata_t ) && " metadata overflow ");
    auto inp = new uint8_t[ output_bits_per_packet() ];
    memset(inp, 0, padsize );
    auto du = Surfaceable::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    auto b = Allocate( du );
    SDL_FreeSurface( du );
    auto padding_buf = new uint8_t[bytes(output_bits_per_packet())];
    memset(padding_buf, 0, bytes(output_bits_per_packet()));
    memcpy( padding_buf, &md, sizeof(TurboFEC_framemetadata_t));

    tobits(inp, padding_buf, padsize );
    dump_partition( inp );
    encode(b.turbo , inp, padsize );

    auto filler = new uint8_t[bytes(padsize_bits)];
    memset(filler, 0,  bytes(padsize_bits) );
    auto step_size = padsize + term_size();
    frombits(filler, b.turbo[0], step_size );
    frombits(&filler[bytes(step_size)], b.turbo[1], step_size);
    frombits(&filler[2*bytes(step_size)], b.turbo[2], step_size);
    memcpy( cm, filler, bytes( padsize_bits ) );
    TurboFEC::free(b.turbo);
    delete[] b.turbo;
    delete[] padding_buf;
    delete[] inp;
    delete[] filler;

}

void TurboFEC::dump_partition( uint8_t* in, size_t start, size_t end) {
    char buff[1024];
    memset( buff,0, 1024 );
    for ( auto i = start; i < end; i ++ ) {
        sprintf( buff+i-start, "%u", in[i] );
    }
    SDL_Log( "Dump partition [%03zu~%03zu]: %s", start, end, buff );
}

int TurboFEC::verbose_memcmp( uint8_t *p1, uint8_t *p2, size_t size ) {
    auto check = memcmp(p1,p2, size);
    if(check != 0)
        for(size_t i =0; i < size; i++)
            if ( p1[i] != p2[i] ) {
                SDL_Log("Found non congruent memory, two partitions source and badcopy");
                dump_partition(p1,i,i+64);
                dump_partition(p2,i,i+64);
                return check;
            }
    return check;
}


#endif //SDL_CRT_FILTER_TURBOFEC_HPP
