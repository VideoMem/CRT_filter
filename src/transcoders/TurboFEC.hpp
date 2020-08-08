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
#define DEFAULT_BITDEPTH 3

typedef std::vector<Pixelable_ch_t> TurboFEC_bitvect_t;

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
    static uint8_t** Allocate(size_t buflen );
    static TurboFEC_bitbuff_t & Allocate( SDL_Surface* ref );
    static void free ( uint8_t** b );
    static void free ( TurboFEC_bitbuff_t& b );
    static void encode(uint8_t** buff, uint8_t* in_bits, size_t len_bits );
    static void encode( uint8_t** buff, SDL_Surface* src );
    static void encode(TurboFEC_bitvect_t &dst, SDL_Surface* src );
    static TurboFEC_bitvect_t encode(SDL_Surface* src, size_t wordbitlen );
    static void encode(TurboFEC_bitvect_t &dst, Pixelable_ch_t &in_bits );

    static void decode( uint8_t* dst_bits, uint8_t** buff, size_t len_bits );
//    static void decode( SDL_Surface* dst, uint8_t** buff );
    static void decode( SDL_Surface* dst, TurboFEC_bitvect_t &src );
    static void decode( SDL_Surface* dst, TurboFEC_bitvect_t &src, size_t wordbitlen );

    static void decode( Pixelable_ch_t &dst, TurboFEC_bitvect_t &src );
    static inline int term_size() { return 4; };
    static inline size_t bits ( size_t bytes ) { return bytes * 8 * sizeof(uint8_t); }
    static inline size_t bytes( size_t bits ) { return bits / 8 * sizeof(uint8_t); }
    //changes from uint8_t bit representation to n bit length rearrange
    static size_t bitdownquant( uint8_t **dst, uint8_t **src, int bitlen, size_t read_size );
    static void bitdownquant( TurboFEC_bitvect_t &dst, TurboFEC_bitvect_t &src, int bitlen );
    static void single_bitdownquant(uint8_t *dst, uint8_t *src, int bitlen, size_t read_size );
    static void single_bitdownquant(Pixelable_ch_t &dst, Pixelable_ch_t &src, size_t bitlen );
    static void bitupquant( uint8_t **dst, uint8_t **src, int bitlen, size_t write_size );
    static void bitupquant( TurboFEC_bitvect_t &dst, TurboFEC_bitvect_t &src, int bitlen );
    static void single_bitupquant( uint8_t *dst, uint8_t *src, int bitlen, size_t write_size );
    static void single_bitupquant(Pixelable_ch_t &dst, Pixelable_ch_t &src, size_t bitlen );
    static TurboFEC_bitvect_t init_bitvect() { return { Pixelable_ch_t(), Pixelable_ch_t(), Pixelable_ch_t() }; };

    static size_t required_size( size_t );
    static size_t linear_required_size( size_t, size_t );
    static size_t linear_required_size( size_t );
    static size_t unrequired_size( size_t );

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
            return size + size % bits(1);
    }

    static void AsChannelMatrix(uint8_t *cm, TurboFEC_bitbuff_t &b, size_t bitslen);
    static Pixelable_ch_t AsChannelVector(TurboFEC_bitvect_t &b);
    static void FromChannelMatrix(TurboFEC_bitbuff_t &b, uint8_t *cm, size_t bytelen);
    static TurboFEC_bitvect_t FromChannelVector(Pixelable_ch_t &cv);
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
    static Pixelable_ch_t tobits( Pixelable_ch_t &src );
    static void tobits( uint8_t *dst, const uint8_t *b, int n );
    static void frombits( Pixelable_ch_t &dst, Pixelable_ch_t &src );
    static void frombits( uint8_t *dst, uint8_t *b, int n );
    static Pixelable_ch_t frombits( Pixelable_ch_t &src );

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

    // maximum transfer unit in bits for a given size in ref bits
    // for N len bits as ref
    // output_bits_per_packet = LEN * 3 + 3 * term_size;
    static TurboFEC_mtu_t mtu( size_t ref, size_t max ) {
        TurboFEC_mtu_t size = {};
        size.input_bits = input_bits( ref );
        size.output_bits = required_size( size.input_bits );
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
    static int gcd( int a, int b )    {
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

void TurboFEC::free( TurboFEC_bitbuff_t& b ) {
    free(b.turbo);
    delete [] b.turbo;
    delete &b;
}

uint8_t **TurboFEC::Allocate(size_t buflen) {
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

size_t TurboFEC::required_size( size_t input_bits ) {
    return input_bits + term_size() * input_bits / LEN;
}

size_t TurboFEC::linear_required_size( size_t input_bits, size_t wordbitlen ) {
    return 3 * downquant_size( wordbitlen, required_size( input_bits ) );
}

size_t TurboFEC::linear_required_size( size_t input_bits ) {
    return 3 * required_size( input_bits );
}

// bits -> turbo encoder -> buff[] (turbo)
void TurboFEC::encode( uint8_t** buff, uint8_t* in_bits, size_t len_bits ) {
    Pixelable_ch_t in_vect(in_bits, in_bits + len_bits);
    TurboFEC_bitvect_t bv;
    encode(bv, in_vect);
    for (int i = 0; i < 3; i++) {
        memcpy(buff[i], &bv[i][0], bv[i].size());
        assert(verbose_memcmp(&bv[i][0], buff[i], bv[i].size()) == 0);
    }
}

void TurboFEC::encode(TurboFEC_bitvect_t &dst, Pixelable_ch_t &in_bits ) {
    //SDL_Log("Encoding %zu input as %zu output bits ", in_bits.size(), required_size( in_bits.size() ));
    auto packets = in_bits.size() / LEN;
    auto lte_t = lte_turbo();
    auto &buff = AllocateC( required_size(in_bits.size()) );
    size_t bitssum = 0;
    for (size_t packet = 0; packet < packets; packet++) {
        auto in_bit_offset = packet * LEN;
        auto bit_offset = packet * (LEN + term_size());
        if ( bit_offset > required_size( in_bits.size() ) ) {
            SDL_Log("Error at iteration %zu, from %zu total packets", packet, packets );
            assert( false &&  "Buffer overflow error" );
        } else {
            bitssum += lte_turbo_encode(&lte_t,
                    &in_bits[in_bit_offset],
                    &buff.turbo[0][bit_offset],
                    &buff.turbo[1][bit_offset],
                    &buff.turbo[2][bit_offset]);
        }
    }

    if( bitssum / 3 != required_size(in_bits.size()) ) {
        SDL_Log("resulting size, %zu is not expected size, %zu", bitssum / 3, required_size(in_bits.size()));
        assert(false);
    }

    TurboFEC_bitvect_t bv( {
        Pixelable_ch_t(buff.turbo[0], buff.turbo[0] + required_size(in_bits.size()) ),
        Pixelable_ch_t(buff.turbo[1], buff.turbo[1] + required_size(in_bits.size()) ),
        Pixelable_ch_t(buff.turbo[2], buff.turbo[2] + required_size(in_bits.size()) )
    } );

    assert( bv[0].size() == required_size(in_bits.size()) );

    for ( int i = 0; i < 3; i++ )
        assert( verbose_memcmp( &bv[i][0], buff.turbo[i], bv[i].size() ) == 0 );

    dst = bv;
    free( buff );
}

//src surface ->  lumachannelmatrix -> tobits -> above encoder
void TurboFEC::encode( uint8_t **buff, SDL_Surface *src ) {
    TurboFEC_bitvect_t bv = init_bitvect();
    encode ( bv, src );
    for(int i =0; i < 3; i++)
        memcpy( buff[i], &bv[i][0], bv[i].size() );
}

void TurboFEC::encode(TurboFEC_bitvect_t &dst, SDL_Surface* src ) {
    auto m = mtu( bits( Pixelable::pixels(src) ) );
    auto in_bytes = Pixelable::AsLumaChannelVector( src );
    in_bytes.resize( bytes( m.input_bits ) );
    auto in_bits = tobits( in_bytes );
    encode ( dst, in_bits );
}

TurboFEC_bitvect_t TurboFEC::encode(SDL_Surface* src, size_t wordbitlen ) {
    auto m = mtu_downquant(bits( Pixelable::pixels(src) ), wordbitlen );
    auto in_bytes = Pixelable::AsLumaChannelVector( src );
    in_bytes.resize( bytes( m.input_bits ) );
    auto in_bits = tobits( in_bytes );
    auto src_bv = init_bitvect();
    encode ( src_bv, in_bits );
    TurboFEC_bitvect_t dst;
    bitdownquant(dst, src_bv, wordbitlen );
    return dst;
}

//returns input bits from output bits
size_t TurboFEC::unrequired_size( size_t output_bits ) {
    size_t us = LEN * output_bits / ( LEN + term_size() );
    assert( output_bits == required_size( us ) );
    return us;
}

void TurboFEC::decode( uint8_t* out_bits, uint8_t** buff, size_t len_bits ) {
    TurboFEC_bitvect_t bv = {
            Pixelable_ch_t(buff[0], buff[0] + required_size(len_bits)),
            Pixelable_ch_t(buff[1], buff[1] + required_size(len_bits)),
            Pixelable_ch_t(buff[2], buff[2] + required_size(len_bits))
    };
    Pixelable_ch_t cv;
    decode( cv, bv );
    memcpy(out_bits, &cv[0], cv.size() );
}

//turbo decode from buff to out_bits by len_bits
void TurboFEC::decode( Pixelable_ch_t &dst, TurboFEC_bitvect_t &src ) {
    auto packets = unrequired_size( src[0].size() ) / LEN;
    struct tdecoder *tdec = alloc_tdec();

    int8_t *bs0, *bs1, *bs2;
    bs0 = (int8_t*) &src[0][0];
    bs1 = (int8_t*) &src[1][0];
    bs2 = (int8_t*) &src[2][0];
    auto out_bits = new uint8_t[ unrequired_size(src[0].size()) ];

    auto iter = DEFAULT_ITER;

    for ( size_t packet=0; packet < packets; packet ++ ) {
        auto in_bit_offset = packet * LEN;
        auto bit_offset = packet * (LEN + term_size());
        lte_turbo_decode_unpack(tdec, LEN, iter,
                                &out_bits[in_bit_offset],
                                &bs0[ bit_offset ],
                                &bs1[ bit_offset ],
                                &bs2[ bit_offset ]);
    }
    dst = Pixelable_ch_t (out_bits,out_bits + unrequired_size( src[0].size() ) );

    delete[] out_bits;
    free_tdec(tdec);
}

void TurboFEC::decode( SDL_Surface* dst, TurboFEC_bitvect_t &src ) {
    Pixelable_ch_t bv;
    decode( bv, src );
    auto out_bytes_cv = frombits( bv );
    out_bytes_cv.resize( Pixelable::pixels(dst) );
    Pixelable::ApplyLumaChannelVector( dst, out_bytes_cv );
}

void TurboFEC::decode( SDL_Surface* dst, TurboFEC_bitvect_t &src, size_t wordbitlen ) {
    TurboFEC_bitvect_t bv_downconverted = init_bitvect();
    bitupquant( bv_downconverted, src, wordbitlen );
    Pixelable_ch_t bv;
    decode( bv, bv_downconverted );
    auto out_bytes_cv = frombits( bv );
    out_bytes_cv.resize( Pixelable::pixels(dst) );
    Pixelable::ApplyLumaChannelVector( dst, out_bytes_cv );
}


// Convert n bits to uint8_t array
// b[0] = 0xFF as, dst[0] = 1, dst[1] = 1, dst[2] = 1, etc ..
// up to n (usually 8)
void TurboFEC::tobits( uint8_t *dst, const uint8_t *b, int n ) {
    assert( n % bits(1) == 0 && "bit length is not multiple of 8" );
    Pixelable_ch_t src(&b[0], &b[0] + bytes(n) );
    Pixelable_ch_t ddr;
    tobits(ddr, src);
    memcpy(dst, &ddr[0], n );
}

Pixelable_ch_t TurboFEC::tobits(Pixelable_ch_t &src) {
    Pixelable_ch_t m;
    tobits(m, src);
    return m;
}

void TurboFEC::tobits(Pixelable_ch_t &dst, Pixelable_ch_t &src ) {
    int m = sizeof(uint8_t) * bits(1);

    for ( auto srcp : src ) {
        for(int i = 0; i < m; i++ ) {
            auto b = srcp >> i & 0x01;
            dst.push_back ( b );
        }
    }
}

Pixelable_ch_t TurboFEC::frombits( Pixelable_ch_t &src ) {
    Pixelable_ch_t m;
    frombits(m, src);
    return m;
}

void TurboFEC::frombits( Pixelable_ch_t &dst, Pixelable_ch_t &src ) {
    int m = sizeof src[0] * bits(1);

    for ( int b=0; b < (int)src.size(); b+=m ) {
        uint8_t r = 0;
        for(int i = 0; i < m; i++ )
            r |= (src[i+b] & 0x01) << i;
        dst.push_back(r);
    }
}

//same as above but in reverse
void TurboFEC::frombits( uint8_t *dst, uint8_t *b, int n ) {
    assert( n % bits(1) == 0 && "bit length is not multiple of 8");
    Pixelable_ch_t src(b, b + n / sizeof b[0]);
    Pixelable_ch_t ddr;
    frombits(ddr, src);
    memcpy(dst, &ddr[0], n / bits(1));
}


void TurboFEC::single_bitdownquant(uint8_t *dst, uint8_t *src, int bitlen, size_t read_size) {
    Pixelable_ch_t dst_v;
    Pixelable_ch_t src_v( src, src + read_size );
    single_bitdownquant( dst_v, src_v, bitlen );
    memcpy( dst, &dst_v[0], dst_v.size() );
}

void TurboFEC::single_bitdownquant( Pixelable_ch_t &dst, Pixelable_ch_t &src, size_t bitlen ) {
    auto shift = bits(1) - bitlen;
    std::deque<uint8_t> srcq;
    std::move( src.begin(), src.end(), std::back_inserter(srcq) );
    while ( ! srcq.empty() ) {
        for (size_t b = 0; b < shift; b++)
            dst.push_back( 0 );
        for (size_t b = 0; b < bitlen && !srcq.empty(); b++) {
            auto bit = srcq.front() % 2;
            dst.push_back( bit );
            srcq.pop_front();
        }
    }
    //std::reverse( dst.begin(), dst.end() );
}

// From src array to dst array (read_size) elements.
// Quantize each to unsigned bitlen integer, then store it on
// dst as uint8_t, read size as bits
size_t TurboFEC::bitdownquant(uint8_t **dst, uint8_t **src, int bitlen, size_t read_size) {
    TurboFEC_bitvect_t srcv = {
            Pixelable_ch_t(src[0], src[0] + read_size),
            Pixelable_ch_t(src[1], src[1] + read_size),
            Pixelable_ch_t(src[2], src[2] + read_size),
    };
    TurboFEC_bitvect_t dstv = init_bitvect();

    for(int q=0; q < 3; q++ ) {
        single_bitdownquant(dstv[q], srcv[q], bitlen);
        memcpy( dst[q], &dstv[q][0], dstv[q].size() );
    }
    return downquant_size( bitlen, read_size ) + 1;
}

void TurboFEC::bitdownquant( TurboFEC_bitvect_t &dst, TurboFEC_bitvect_t &src, int bitlen ) {
    for( auto srcp: src ) {
        Pixelable_ch_t dstch;
        single_bitdownquant(dstch, srcp, bitlen);
        dst.push_back( dstch );
    }
}

//same as above, but in reverse
void TurboFEC::single_bitupquant(uint8_t *dst, uint8_t *src, int bitlen, size_t write_size) {
    Pixelable_ch_t dst_v;
    Pixelable_ch_t src_v(&src[0], &src[0] + write_size );
    single_bitupquant( dst_v, src_v, bitlen );
    memcpy( dst, &dst_v[0], dst_v.size() );
}

void TurboFEC::single_bitupquant(Pixelable_ch_t &dst, Pixelable_ch_t &src, size_t bitlen ) {
    auto shift = bits(1) - bitlen;
    std::deque<uint8_t> srcq;
    std::move( src.begin(), src.end(), std::back_inserter(srcq) );
    while( ! srcq.empty() ) {
        for (size_t b = 0; b < shift && !srcq.empty(); b++)
            srcq.pop_front();
        for (size_t b = 0; b < bitlen && !srcq.empty(); b++) {
            auto bit = srcq.front() % 2;
            dst.push_back(bit);
            srcq.pop_front();
        }
    }
}

void TurboFEC::bitupquant( TurboFEC_bitvect_t &dst, TurboFEC_bitvect_t &src, int bitlen ) {
    for(int q=0; q < 3; q++ )
        single_bitupquant(dst[q], src[q], bitlen );
}

void TurboFEC::bitupquant(uint8_t **dst, uint8_t **src, int bitlen, size_t write_size) {
    TurboFEC_bitvect_t srcv = {
            Pixelable_ch_t(src[0], src[0] + write_size),
            Pixelable_ch_t(src[1], src[1] + write_size),
            Pixelable_ch_t(src[2], src[2] + write_size),
    };
    TurboFEC_bitvect_t dstv = init_bitvect();

    for(int q=0; q < 3; q++ ) {
        single_bitupquant(dstv[q], srcv[q], bitlen);
        memcpy( dst[q], &dstv[q][0], dstv[q].size() );
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
        SDL_Log("Non partitionable in screen lines! %d % %d != 0", ret_size, ref->w );
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

void TurboFEC::AsChannelMatrix( uint8_t *cm, TurboFEC_bitbuff_t &b, size_t bitslen ) {
    TurboFEC_bitvect_t bv( {
        Pixelable_ch_t(b.turbo[0], b.turbo[0] + bitslen ),
        Pixelable_ch_t(b.turbo[1], b.turbo[1] + bitslen ),
        Pixelable_ch_t(b.turbo[2], b.turbo[2] + bitslen )
    } );

    auto cv = AsChannelVector(bv);
    memcpy(cm, &cv[0], cv.size() );
}

Pixelable_ch_t TurboFEC::AsChannelVector(TurboFEC_bitvect_t &b) {
    Pixelable_ch_t cp;
    TurboFEC_bitvect_t in = {
            frombits( b[0] ),
            frombits( b[1] ),
            frombits( b[2] )
    };

    Pixelable_ch_t cv;

    for(auto i : in[0]) {
        cv.push_back(i);
    }
    for(auto i : in[1]) {
        cv.push_back(i);
    }
    for(auto i : in[2]) {
        cv.push_back(i);
    }

    return cv;
}

void TurboFEC::FromChannelMatrix(TurboFEC_bitbuff_t &b, uint8_t *cm, size_t bytelen ) {
    assert( bytelen % 3 == 0 && "ChannelMatrix bytelen must be multiple of 3" );
    Pixelable_ch_t cv(cm, cm + bytelen / sizeof cm[0] );
    TurboFEC_bitvect_t bv = FromChannelVector(cv);
    b.size = bv[0].size();
    for( int s = 0; s < 3; s++ ) {
        int j = 0;
        for (auto i : bv[s]) {
            b.turbo[s][j] = i;
            j++;
        }
    }
}

TurboFEC_bitvect_t TurboFEC::FromChannelVector(Pixelable_ch_t &cv) {
    Pixelable_ch_t ch;
    TurboFEC_bitvect_t c = init_bitvect();
    TurboFEC_bitvect_t b = init_bitvect();
    assert( cv.size() % 3 == 0 && "ChannelVector size must be multiple of 3" );
    size_t step = cv.size() / 3;
    for(size_t i=0; i < step ; i++) {
        c[0].push_back( cv[ i ] );
        c[1].push_back( cv[ i + step ] );
        c[2].push_back( cv[ i + 2 * step ] );
    }
    tobits(b[0], c[0]);
    tobits(b[1], c[1]);
    tobits(b[2], c[2]);

    return b;
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
