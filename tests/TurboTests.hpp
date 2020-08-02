//
// Created by sebastian on 08/07/2020.
//

#ifndef SDL_CRT_FILTER_TURBOTESTS_HPP
#define SDL_CRT_FILTER_TURBOTESTS_HPP
#include <transcoders/TurboFEC.hpp>

extern "C" {
    #include "noise.c"
};

#define DEFAULT_NUM_PKTS	1000
#define MAX_LEN_BITS		32768
#define DEFAULT_SNR	8.0
#define DEFAULT_AMP	32.0

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

struct lte_test_vector {
    const char *name;
    const char *spec;
    const struct lte_turbo_code *code;
    int in_len;
    int out_len;
};

static void fill_random(uint8_t *b, int n) {
    int i, r, m, c;

    c = 0;
    r = rand();
    m = sizeof(int) - 1;

    for ( i = 0; i < n; i++ ) {
        if ( c++ == m ) {
            r = rand();
            c = 0;
        }

        b[i] = (r >> (i % m)) & 0x01;
    }
}

static void tobits( uint8_t *dst, const uint8_t *b, int n ) {
    TurboFEC::tobits( dst, b, n );
}

static void frombits( uint8_t *dst, uint8_t *b, int n ) {
    TurboFEC::frombits( dst, b, n );
}

/* Generate soft bits with ~2.3% crossover probability */
static int uint8_to_err(int8_t *dst, uint8_t *src, int n, float snr) {
    int i, err = 0;

    add_noise(src, dst, n, snr, DEFAULT_AMP);

    for (i = 0; i < n; i++) {
        if ((!src[i] && (dst[i] >= 0)) || (src[i] && (dst[i] <= 0)))
            err++;
    }

    return err;
}

/* Output error input/output error rates */
static void print_error_results(const struct lte_test_vector *test,
                                int iber, int ober, int fer, int num_pkts)
{
    printf("[..] Input BER.......................... %f\n",
           (float) iber / (num_pkts * test->out_len));
    printf("[..] Output BER......................... %f\n",
           (float) ober / (num_pkts * test->in_len));
    printf("[..] Output FER......................... %f\n",
           (float) fer / num_pkts);
}


// len as bytes, cp as bits
static void dump( SDL_Surface *output_surface, uint8_t *cp, size_t len_bits ) {
    Loader::blank( output_surface );
    auto out = new uint8_t[ TurboFEC::bytes(len_bits) + 1 ];
    frombits( out, cp, len_bits );
    Pixelable::ApplyLumaChannelMatrix( output_surface, out );
    delete[] out;
}

// len as bytes, cp as bits
static void dump( SDL_Surface *output_surface, uint8_t *cp, std::string name, size_t len_bits) {
    dump(output_surface, cp, len_bits);
    SDL_SaveBMP( output_surface,  name.c_str() );
}

static int error_test( const struct lte_test_vector *test,
                      int num_pkts, int iter, float snr ) {
    // dump buffers as surfaces
    auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
    auto testcard = SDL_ConvertSurfaceFormat( image ,
                                             SDL_PIXELFORMAT_RGBA32 , 0 );
    static const int width = testcard->w;
    static const int size = TurboFEC::conv_size( TurboFEC::bytes(test->in_len) );
    static const int height = size / width;

    auto input_surface = Surfaceable::AllocateSurface( width, height );
    auto output_surface = Surfaceable::AllocateSurface( width, height );

    //start of the 'original' test code
    int i, n, l, iber = 0, ober = 0, fer = 0;
    int8_t *bs0, *bs1, *bs2;
    uint8_t *in_bits;

    in_bits  = new uint8_t[test->in_len];

    struct tdecoder *tdec = alloc_tdec();
    SDL_Rect sub { .w = width, .h = height };
    SDL_BlitSurface( testcard, &sub, input_surface, &sub );
    auto in = Pixelable::AsLumaChannelMatrix( input_surface );

    SDL_SaveBMP( input_surface, "turbo_error_test_in.bmp" );
    //dump( in, output_surface, "turbo_error_test_in.bmp"  );
    //uint8_t* in_min = std::min_element(in_bits, in_bits + test->in_len  );
    //uint8_t* in_max = std::max_element(in_bits, in_bits + test->in_len  );
    //SDL_Log( "in min / max: %d/%d ",  *in_min, *in_max );

    tobits( in_bits, in, test->in_len );

    auto bu = TurboFEC::Allocate( input_surface ); // <-- each uint8_t represents a bit not a byte!!
    auto bs = TurboFEC::Allocate( input_surface ); // there is four bit terminator for each channel
    bs0 = (int8_t*) bs.turbo[0];
    bs1 = (int8_t*) bs.turbo[1];
    bs2 = (int8_t*) bs.turbo[2];

    for (i = 0; i < num_pkts; i++) {
        //fill_random(in, test->in_len);
        l = lte_turbo_encode(test->code, in_bits, bu.turbo[0], bu.turbo[1], bu.turbo[2]);
        if (l != test->out_len) {
            printf("ERROR !\n");
            fprintf(stderr, "[!] Failed encoding length check (%i)\n",
                    l);
            return -1;
        }

        iber += uint8_to_err(bs0, bu.turbo[0], test->in_len + 4, snr);
        iber += uint8_to_err(bs1, bu.turbo[1], test->in_len + 4, snr);
        iber += uint8_to_err(bs2, bu.turbo[2], test->in_len + 4, snr);

        lte_turbo_decode_unpack(tdec, test->in_len, iter, bu.turbo[0], bs0, bs1, bs2);

        for (n = 0; n < test->in_len; n++) {
            if (in_bits[n] != bu.turbo[0][n])
                ober++;
        }

        if (memcmp(in_bits, bu.turbo[0], test->in_len))
            fer++;

    }


    dump(output_surface, bu.turbo[0], "turbo_error_test_out.bmp", bu.size);

    dump(output_surface, bs.turbo[0], "turbo_error_test_bs0.bmp", bs.size);
    dump(output_surface, bs.turbo[1], "turbo_error_test_bs1.bmp", bs.size);
    dump(output_surface, bs.turbo[2], "turbo_error_test_bs2.bmp", bs.size);

    dump(output_surface, bu.turbo[0], "turbo_error_test_bu0.bmp", bs.size);
    dump(output_surface, bu.turbo[1], "turbo_error_test_bu1.bmp", bs.size);
    dump(output_surface, bu.turbo[2], "turbo_error_test_bu2.bmp", bs.size);

    print_error_results(test, iber, ober, fer, num_pkts);

    delete [] in_bits;
    delete [] in;

    TurboFEC::free( bu.turbo );
    TurboFEC::free( bs.turbo );
    delete[] bu.turbo;
    delete[] bs.turbo;

    SDL_FreeSurface( input_surface );
    SDL_FreeSurface( output_surface );
    SDL_FreeSurface( testcard );
    SDL_FreeSurface( image );

    free_tdec( tdec );
    return 0;
}

const struct lte_turbo_code lte_turbo = {
        .n = 2,
        .k = 4,
        .len = LEN,
        .rgen = 013,
        .gen = 015,
};

const struct lte_test_vector tests[] = {
        {
                .name = "3GPP LTE turbo",
                .spec = "(N=2, K=4)",
                .code = &lte_turbo,
                .in_len  = LEN,
                .out_len = LEN * 3 + 4 * 3,
        },
        { /* end */ },
};

void display ( uint8_t* in, uint8_t* encoded, uint8_t* recover, size_t offset ) {
    printf("in %03zu : " BYTE_TO_BINARY_PATTERN "\n", offset, BYTE_TO_BINARY( in [offset] ) );
    auto b = offset * 8;
    printf("encoded: %d%d%d%d%d%d%d%d\n",
           encoded [ b ], encoded [ b + 1 ],
           encoded [ b + 2 ], encoded [ b + 3 ],
           encoded [ b + 4 ], encoded [ b + 5 ],
           encoded [ b + 6 ], encoded [ b + 7 ]
    );

    printf("recover: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY( recover [ offset ] ));

}

TEST_CASE("TurboFEC quantization","[TurboFEC]") {
    //uint8 to double -> uint8
    for ( auto i = 0; i < 0x100; i++) {
        auto f = Pixelable::uint8_to_double(i);
        auto j = Pixelable::double_to_uint8(f);
        if ( j != i ) SDL_Log( "Error: %f , %d, %d", f, i, j);
        REQUIRE( j == i );
    }

    //channelVector
    auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
    auto testcard = SDL_ConvertSurfaceFormat( image,
                                             SDL_PIXELFORMAT_RGBA32 , 0 );
    auto copy = Surfaceable::AllocateSurface( testcard );
    auto cv = Pixelable::AsLumaChannelVector( testcard );
    Pixelable::ApplyLumaChannelVector( copy, cv );
    auto dv = Pixelable::AsLumaChannelVector( copy );
    REQUIRE( memcmp( &cv[0], &dv[0], cv.size() ) == 0 );
    SDL_SaveBMP( copy, "turbofec_cm_test_out.bmp");
    auto image_copy = SDL_LoadBMP("turbofec_cm_test_out.bmp");
    auto recover = SDL_ConvertSurfaceFormat( image_copy,
                                             SDL_PIXELFORMAT_RGBA32 , 0 );
    auto rv = Pixelable::AsLumaChannelVector( recover );
    REQUIRE( memcmp( &rv[0], &dv[0], rv.size() ) == 0 );

    SDL_FreeSurface( copy );
    SDL_FreeSurface( testcard );
    SDL_FreeSurface( recover );
    SDL_FreeSurface( image );
    SDL_FreeSurface( image_copy );
}

TEST_CASE("TurboFEC bitdownquant, bitupquant (vectors version)","[TurboFEC]") {
    auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
    auto testcard = SDL_ConvertSurfaceFormat( image,
                                              SDL_PIXELFORMAT_RGBA32 , 0 );
    auto bitlen = DEFAULT_BITDEPTH;
    auto input_size = TurboFEC::bits( Pixelable::pixels(testcard) );
    auto ints_size = TurboFEC::downquant_size(bitlen, input_size);
    SDL_Log("original size: %lu bits", input_size );
    SDL_Log("downquant new size: %lu bits", ints_size );
    SDL_Log("upquant size: %lu bits", TurboFEC::upquant_size(bitlen, ints_size) );
    REQUIRE( TurboFEC::upquant_size(bitlen, ints_size) == input_size );
    auto in_bytes = Pixelable::AsLumaChannelVector( testcard );
    auto in_bits = TurboFEC::tobits( in_bytes );
    TurboFEC_bitvect_t encoded_v = { Pixelable_ch_t(), Pixelable_ch_t(), Pixelable_ch_t() };
    TurboFEC_bitvect_t encoded_rv = { Pixelable_ch_t(), Pixelable_ch_t(), Pixelable_ch_t() };
    TurboFEC::encode( encoded_v, in_bits );
    TurboFEC_bitvect_t encoded_6v;
    TurboFEC::bitdownquant( encoded_6v, encoded_v, bitlen );

    for ( int i=0; i < 3; i++ ) {
        SDL_Log("Encoded channel %d", i );
        TurboFEC::dump_partition(&encoded_v[i][0]);
        SDL_Log("Downquantized encoded channel %d", i );
        TurboFEC::dump_partition(&encoded_6v[i][0]);
    }

    TurboFEC::bitupquant( encoded_rv, encoded_6v, bitlen );

    for ( int i=0; i < 3; i++ )
        REQUIRE( TurboFEC::verbose_memcmp( &encoded_v[i][0], &encoded_rv[i][0], encoded_v[i].size() ) == 0);

    SDL_FreeSurface( image );
    SDL_FreeSurface( testcard );
}


TEST_CASE("TurboFEC bitdownquant, bitupquant (unsafe version)","[TurboFEC]") {
    auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
    auto testcard = SDL_ConvertSurfaceFormat( image,
                                              SDL_PIXELFORMAT_RGBA32 , 0 );
    auto bitlen = DEFAULT_BITDEPTH;
    auto input_size = TurboFEC::bits( Pixelable::pixels(testcard) );
    auto ints_size = TurboFEC::downquant_size(bitlen, input_size);
    SDL_Log("original size: %ld bits", input_size );
    SDL_Log("downquant new size: %lu bits", ints_size );
    SDL_Log("upquant size: %lu bits", TurboFEC::upquant_size(bitlen, ints_size) );
    REQUIRE( TurboFEC::upquant_size(bitlen, ints_size) == (size_t)input_size );

    auto in_bytes = Pixelable::AsLumaChannelVector( testcard );
    auto in_bits = TurboFEC::tobits( in_bytes );
    TurboFEC_bitvect_t encoded_v = TurboFEC::init_bitvect();
    TurboFEC_bitvect_t encoded_rv = TurboFEC::init_bitvect();
    TurboFEC::encode( encoded_v, in_bits );

    SDL_Log("uint6 size %zu", TurboFEC::required_size(TurboFEC::downquant_size(bitlen,  encoded_v.front().size() ) ) );
    SDL_Log("encoded_v.front().size() %zu",  encoded_v.front().size() );

    auto &uint6 = TurboFEC::AllocateC( TurboFEC::required_size(TurboFEC::downquant_size(bitlen,  encoded_v.front().size() ) ) );
    auto &buff_r = TurboFEC::AllocateC( encoded_v[0].size() );

    uint8_t* uint66[3] = { &encoded_v[0][0], &encoded_v[1][0], &encoded_v[2][0] };
    TurboFEC_bitbuff_t buff = {
            uint66,
            encoded_v.front().size()
    };

    auto new_size = TurboFEC::bitdownquant( uint6.turbo, buff.turbo, bitlen, encoded_v.front().size() );
    TurboFEC::bitupquant( buff_r.turbo, uint6.turbo, bitlen, new_size );

    TurboFEC::dump_partition( uint6.turbo[0] );
    TurboFEC::dump_partition( buff.turbo[0] );
    TurboFEC::dump_partition( buff_r.turbo[0] );

    SDL_Log( "bitupquant/dwquant read_size: %zu, new_quant_size: %lu", encoded_v.front().size() , new_size );

    for(int i =0; i < 3; i++)
        REQUIRE( TurboFEC::verbose_memcmp( buff.turbo[i], buff_r.turbo[i], encoded_v[i].size()) == 0);

    TurboFEC::free( buff_r );
    TurboFEC::free( uint6 );
    SDL_FreeSurface( image );
    SDL_FreeSurface( testcard );
}


TEST_CASE("TurboFEC tobits, frombits","[TurboFEC]") {
    auto pixels = TurboFEC::bytes( LEN );
    auto full = new uint8_t[ pixels ];
    for ( size_t i = 0; i < pixels; i++ ) full[i] = 0xFF - i;

    auto bits = new uint8_t[ LEN ];
    auto recover = new uint8_t[ pixels ];

    tobits( bits, full, LEN );
    frombits( recover, bits, LEN );

    for ( int i = 0; i <= 0xFF; i++ ) {
        //display( full , bits, recover, i );
        REQUIRE( full[i] == recover[i] );
    }

    delete[] full;
    delete[] bits;
    delete[] recover;

}

TEST_CASE("TurboFEC tobits, frombits, vectors version","[TurboFEC]") {
    auto pixels = TurboFEC::bytes( LEN );
    Pixelable_ch_t bits;
    Pixelable_ch_t full;
    Pixelable_ch_t recover;

    for ( size_t i = 0; i < pixels; i++ ) full.push_back( 0xFF - i );

    TurboFEC::tobits( bits, full );
    REQUIRE( bits.size() == full.size() * TurboFEC::bits(1) );
    TurboFEC::frombits( recover, bits );

    REQUIRE( recover.size() == full.size() );
    REQUIRE( TurboFEC::verbose_memcmp( &full[0], &recover[0], full.size() ) == 0 );
}


TEST_CASE("TurboFEC error test","[TurboFEC]") {
    printf("\n[.] BER test:\n");
    printf("[..] Testing:\n");
    srand(time(NULL));
    REQUIRE (error_test(tests, DEFAULT_NUM_PKTS,
                   DEFAULT_ITER, DEFAULT_SNR) >= 0);

}


TEST_CASE("Pixel data converter tests","[TurboFEC]") {
    SECTION("Pixelable channelmatrix surfaces") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                 SDL_PIXELFORMAT_RGBA32 , 0 );

        auto cm = Pixelable::AsLumaChannelMatrix( surface );
        auto copy = Surfaceable::AllocateSurface( surface );
        Pixelable::ApplyLumaChannelMatrix( copy, cm );
        SDL_SaveBMP( copy, "turbo_pix_channel_test.bmp");
        SDL_FreeSurface( surface );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( image );
        delete [] cm;
    }

    SECTION("Pixelable channelvector surfaces") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                 SDL_PIXELFORMAT_RGBA32 , 0 );

        auto cv = Pixelable::AsLumaChannelVector( surface );
        auto copy = Surfaceable::AllocateSurface( surface );
        Pixelable::ApplyLumaChannelVector( copy, cv );
        SDL_SaveBMP( copy, "turbo_pix_channel_vector_test.bmp");
        SDL_FreeSurface( surface );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( image );
    }

}

TEST_CASE("TurboFEC tests","[TurboFEC]") {

    //it turbo encodes mtu.input_bits from image into another
    SECTION("Frame output encode test") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                SDL_PIXELFORMAT_RGBA32, 0);
        auto lte_enc = new TurboFEC();
        auto all_input_bits = TurboFEC::bits ( Pixelable::pixels( surface ) );
        auto  mtu = TurboFEC::mtu( all_input_bits );
        auto &buff = TurboFEC::AllocateC( TurboFEC::required_size( all_input_bits ) );
        TurboFEC::encode( buff.turbo, surface );
        TurboFEC::dump_partition(&buff.turbo[0][811472] );
        TurboFEC::dump_partition(&buff.turbo[1][811472] );
        TurboFEC::dump_partition(&buff.turbo[2][811472] );
        TurboFEC::dump_partition(&buff.turbo[0][811536] );
        auto copy = Surfaceable::AllocateSurface(surface);
        Loader::blank( copy );
        auto copy_cm = Pixelable::AsLumaChannelMatrix( copy );
        TurboFEC::AsChannelMatrix( copy_cm, buff, TurboFEC::required_size( mtu.input_bits ) );
        Pixelable::ApplyLumaChannelMatrix( copy, copy_cm );

        SDL_SaveBMP(copy, "turbo_pix_channel_encoded.bmp");

        TurboFEC::free( buff );
        SDL_FreeSurface(copy);
        SDL_FreeSurface(surface);
        delete lte_enc;
        SDL_FreeSurface( image );
    }

    SECTION("Frame recovery test") {
        auto image = SDL_LoadBMP("turbo_pix_channel_encoded.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                SDL_PIXELFORMAT_RGBA32, 0);

        auto copy = Surfaceable::AllocateSurface(surface);
        auto lte_enc = new TurboFEC();
        auto all_input_bits = TurboFEC::bits ( Pixelable::pixels( surface ) );
        auto mtu = TurboFEC::mtu( all_input_bits );
        Loader::blank( copy );
        auto input_cv = Pixelable::AsLumaChannelVector( surface );
        size_t new_size = TurboFEC::bytes( TurboFEC::linear_required_size( mtu.input_bits ) );
        SDL_Log( "Original size: %zu, new size: %zu", input_cv.size(), new_size );
        input_cv.resize( new_size );
        TurboFEC_bitvect_t buff = TurboFEC::FromChannelVector( input_cv );
        SDL_Log( "Buff vector %zu bits, %zu bytes", buff.front().size(), TurboFEC::bytes( buff.front().size() ) );
        Pixelable_ch_t cv, cv_bits;
        SDL_Log( "Buff ffset %zu", buff[0].size() - 64 );
        TurboFEC::dump_partition(&buff[0][buff[0].size() - 64] );
        TurboFEC::dump_partition(&buff[1][buff[0].size() - 64] );
        TurboFEC::dump_partition(&buff[2][buff[0].size() - 64] );
        TurboFEC::decode( cv_bits, buff );
        cv = TurboFEC::frombits( cv_bits );
        TurboFEC::decode( copy, buff );
        SDL_SaveBMP( copy, "turbo_pix_channel_decoded_direct.bmp" );
        SDL_Log( "cv size, %zu", cv.size() );;
        cv.resize ( Pixelable::pixels( copy ) );
        Pixelable::ApplyLumaChannelVector( copy, cv );
        SDL_SaveBMP( copy, "turbo_pix_channel_decoded_indirect.bmp" );

        SDL_FreeSurface( surface );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( image );
        delete lte_enc;
    }


    SECTION("Encode, then block interleave") {
        size_t kernel_size = 40;
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat(image ,
                                                SDL_PIXELFORMAT_RGBA32, 0);
        auto recover = Surfaceable::AllocateSurface( surface );
        auto interleaved = Surfaceable::AllocateSurface( surface );
        auto copy = Surfaceable::AllocateSurface( surface );
        auto all_input_bits = TurboFEC::bits ( Pixelable::pixels( surface ) );
        auto mtu = TurboFEC::mtu( all_input_bits );

        TurboFEC_bitvect_t buff = TurboFEC::init_bitvect();
        TurboFEC::encode( buff, surface );
        auto enc_cv = TurboFEC::AsChannelVector( buff );
        size_t enc_cv_size = enc_cv.size();
        enc_cv.resize( Pixelable::pixels( copy ) );
        Pixelable::ApplyLumaChannelVector( copy, enc_cv );
        LibAVable::pack_all_recursive( interleaved, copy, kernel_size );
        SDL_SaveBMP( interleaved, "turbo_block_encoder_interleaved.bmp");

        Loader::blank( copy );
        LibAVable::unpack_all_recursive( copy, interleaved, kernel_size );
        auto enc_cv2 = Pixelable::AsLumaChannelVector( copy );
        size_t new_size = TurboFEC::bytes( TurboFEC::linear_required_size( mtu.input_bits ) ); // TurboFEC::bytes( TurboFEC::required_size( mtu.input_bits ) );
        REQUIRE ( new_size == enc_cv_size );
        enc_cv2.resize( new_size );
        auto bv = TurboFEC::FromChannelVector( enc_cv2 );
        TurboFEC::decode( recover, bv );
        SDL_SaveBMP( recover, "turbo_block_decoder_recover.bmp");

        SDL_FreeSurface( recover );
        SDL_FreeSurface( interleaved );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( surface );
        SDL_FreeSurface( image );
    }


    SECTION("Encodes, then requantizes, then block interleaves") {
        size_t kernel_size = 40;
        auto bitlen = DEFAULT_BITDEPTH;
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat(image ,
                                                SDL_PIXELFORMAT_RGBA32, 0);
        auto recover = Surfaceable::AllocateSurface( surface );
        auto interleaved = Surfaceable::AllocateSurface( surface );
        auto copy = Surfaceable::AllocateSurface( surface );
        auto all_input_bits = TurboFEC::bits ( Pixelable::pixels( surface ) );
        auto mtu = TurboFEC::mtu_downquant( all_input_bits, bitlen );

        auto buff = TurboFEC::encode( surface, bitlen ); //1
        auto enc_cv = TurboFEC::AsChannelVector( buff ); //2
        size_t enc_cv_size = enc_cv.size();
        enc_cv.resize( Pixelable::pixels( copy ) );
        Pixelable::ApplyLumaChannelVector( copy, enc_cv ); //3
        SDL_SaveBMP( copy, "turbo_block_encoder_quantized.bmp");
        LibAVable::pack_all_recursive( interleaved, copy, kernel_size ); //4
        SDL_SaveBMP( interleaved, "turbo_block_encoder_quantized_interleaved.bmp");

        Loader::blank( copy );
        LibAVable::unpack_all_recursive( copy, interleaved, kernel_size ); //r4
        auto enc_cv2 = Pixelable::AsLumaChannelVector( copy ); //r3
        size_t new_size = TurboFEC::bytes( TurboFEC::linear_required_size( mtu.input_bits, bitlen ) );
        REQUIRE ( new_size == enc_cv_size );
        enc_cv2.resize( new_size );
        auto bv = TurboFEC::FromChannelVector( enc_cv2 ); //r2
        TurboFEC::decode( recover, bv, bitlen ); //r1
        SDL_SaveBMP( recover, "turbo_block_decoder_quantized_recover.bmp");


        SDL_FreeSurface( recover );
        SDL_FreeSurface( interleaved );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( surface );
        SDL_FreeSurface( image );
    }

}


TEST_CASE("TurboFEC tests, frame stream","[TurboFEC]") {

    SECTION("TurboFEC as and from channelvector") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                SDL_PIXELFORMAT_RGBA32, 0);
        auto recover = Surfaceable::AllocateSurface(surface);
        auto save = Surfaceable::AllocateSurface(surface);
        Loader::blank(recover);

        auto test_cv = Pixelable::AsLumaChannelVector( surface );
        auto b = TurboFEC::FromChannelVector(test_cv);

        SDL_Log("b[0] size: %zu, input cv size: %zu", b.front().size(), test_cv.size() );
        auto rec_cv = TurboFEC::AsChannelVector(b);

        SDL_Log("rec_cv size: %zu", rec_cv.size() );
        Pixelable::ApplyLumaChannelVector(recover, rec_cv);
        Pixelable::ApplyLumaChannelVector(save, test_cv);
        SDL_SaveBMP(recover, "turbofec_channelmatrix_idempotency.bmp");
        SDL_SaveBMP(save, "turbofec_channelmatrix_idempotency2.bmp");

        REQUIRE(TurboFEC::verbose_memcmp( &rec_cv[0], &test_cv[0], rec_cv.size() ) == 0 );
        auto image_copy = SDL_LoadBMP("turbofec_channelmatrix_idempotency.bmp");
        auto copy = SDL_ConvertSurfaceFormat(image_copy,
                                             SDL_PIXELFORMAT_RGBA32, 0);

        auto copy_cv = Pixelable::AsLumaChannelVector( copy );
        REQUIRE(memcmp(&copy_cv[0], &test_cv[0], copy_cv.size() ) == 0 );

        SDL_FreeSurface( recover );
        SDL_FreeSurface( surface );
        SDL_FreeSurface( save );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( image );
        SDL_FreeSurface( image_copy );
    }

    SECTION("TurboFEC FromChannelMatrix") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                 SDL_PIXELFORMAT_RGBA32, 0);
        auto test_cv = Pixelable::AsLumaChannelVector( surface );
        auto b = TurboFEC::FromChannelVector(test_cv);
        auto &c = TurboFEC::AllocateC(b[0].size() );
        TurboFEC::FromChannelMatrix( c, &test_cv[0], test_cv.size());

        for( int i = 0; i < 3; i++ )
            REQUIRE( TurboFEC::verbose_memcmp( &b[i][0], c.turbo[i], b[i].size() ) == 0 );

        TurboFEC::free( c );

        SDL_FreeSurface( surface );
        SDL_FreeSurface( image );
    }

    SECTION("TurboFEC AsChannelMatrix") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                 SDL_PIXELFORMAT_RGBA32, 0);
        auto test_cv = Pixelable::AsLumaChannelVector( surface );
        auto test_cm = new uint8_t[test_cv.size()];
        auto b = TurboFEC::FromChannelVector(test_cv);
        auto &c = TurboFEC::AllocateC( b[0].size() );

        TurboFEC::FromChannelMatrix( c, &test_cv[0], test_cv.size());
        REQUIRE( c.size == b[0].size() );

        for( int i = 0; i < 3; i++ )
            REQUIRE( TurboFEC::verbose_memcmp( &b[i][0], c.turbo[i], b[i].size() ) == 0 );

        TurboFEC::AsChannelMatrix(test_cm, c, c.size );


        REQUIRE( TurboFEC::verbose_memcmp( &test_cv[0], test_cm, test_cv.size() ) == 0 );
        TurboFEC::free( c );
        delete[] test_cm;
        SDL_FreeSurface( surface );
        SDL_FreeSurface( image );
    }


    SECTION("Test MTU encode") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                SDL_PIXELFORMAT_RGBA32, 0);

        auto pixels = Pixelable::pixels(surface);
        auto size_bits = TurboFEC::bits(pixels);
        auto mtu = TurboFEC::mtu_downquant( size_bits, DEFAULT_BITDEPTH );
        SDL_Log("MTU size %d bits, output %d bits :: size %zu bytes, output %zu bytes", mtu.input_bits, mtu.output_bits,
                TurboFEC::bytes(mtu.input_bits), TurboFEC::bytes(mtu.output_bits));
        SDL_Log("Padding size %d bits, %zu bytes", mtu.padding, TurboFEC::bytes(mtu.padding) );

        //it first encodes a channelvector surface to in_bits
        auto surface_cv = Pixelable::AsLumaChannelVector( surface );
        surface_cv.resize( TurboFEC::bytes( mtu.input_bits ) );
        auto in_bits = TurboFEC::tobits(surface_cv );

        //it allocates 3 buffers to store the turbo encoder output
        auto &b = TurboFEC::AllocateC( TurboFEC::required_size( in_bits.size() ) );

        //it encodes in_bits to bv bitvect, and the unsafe version b
        TurboFEC_bitvect_t bv = TurboFEC::init_bitvect();
        TurboFEC::encode( bv, in_bits );
        TurboFEC::encode( b.turbo, &in_bits[0], in_bits.size() );
        auto sha256 = Loader::sha256Log(&surface_cv[0], surface_cv.size() );
        SDL_Log("Input sha256: %s", sha256.c_str() );

        for ( int i = 0; i < 3; i++ )  //it verifies both conversions
            REQUIRE( TurboFEC::verbose_memcmp( &bv[i][0], b.turbo[i], bv[i].size() ) == 0 );

        //it prepares a downconverted version
        SDL_Log( "in_bits.size() %zu", in_bits.size() );
        REQUIRE( b.size == TurboFEC::required_size( in_bits.size() ) );
        auto new_size = TurboFEC::downquant_size( DEFAULT_BITDEPTH, b.size );
        TurboFEC_bitvect_t b6_v;
        TurboFEC::bitdownquant( b6_v, bv, DEFAULT_BITDEPTH );
        SDL_Log( "Downconverted size: %zu, unconverted %zu bits, total per frame %zu", new_size, bv.front().size(), size_bits );

        //it recovers the downconverted version
        auto cv = TurboFEC::init_bitvect();
        TurboFEC::bitupquant( cv, b6_v, DEFAULT_BITDEPTH );

        for( int i =0 ; i < 3; i++ )
            REQUIRE( TurboFEC::verbose_memcmp( &bv[i][0], &cv[i][0], bv[i].size() ) == 0 );

        //then this part decodes it for sha verification, and prepares a blank recover
        //surface to print the output to
        auto recover = Surfaceable::AllocateSurface(surface);
        Loader::blank( recover );
        Pixelable_ch_t out_bits;

        //here the decoder action
        TurboFEC::decode( out_bits, cv );
        Pixelable_ch_t out_cv = TurboFEC::frombits( out_bits );
        auto sha256o = Loader::sha256Log(&out_cv[0], out_cv.size() );
        SDL_Log("Output sha256: %s", sha256o.c_str());

        REQUIRE( out_bits.size() == in_bits.size() );
        REQUIRE( out_cv.size() == surface_cv.size() );

        //it saves the result of the decoder as an image
        out_cv.resize( Pixelable::pixels( recover ) );
        Pixelable::ApplyLumaChannelVector( recover, out_cv );
        SDL_SaveBMP(recover, "turbofec_mtutest_decoded.bmp");

        for( int i = 0; i < 3; i++ ) {
            REQUIRE( TurboFEC::verbose_memcmp( &surface_cv[0], &out_cv[0], surface_cv.size() ) == 0 );
        }

        //it prepares recover for reuse, and enc_cm for encode
        Loader::blank(recover);
        Pixelable_ch_t enc_cv = TurboFEC::AsChannelVector( b6_v );
        REQUIRE( enc_cv.size() <= Pixelable::pixels( recover ) );
        SDL_Log("enc_cv.size() %zu, %zu", enc_cv.size(), enc_cv.size() * 8 );
        enc_cv.resize( Pixelable::pixels( recover ) );
        Pixelable::ApplyLumaChannelVector( recover, enc_cv );
        SDL_SaveBMP(recover, "turbofec_mtutest_encoded.bmp");

        TurboFEC::free( b );

        SDL_FreeSurface(surface);
        SDL_FreeSurface(recover);
        SDL_FreeSurface( image );
    }

    SECTION("Test MTU decode") {
        auto image_source = SDL_LoadBMP("turbofec_mtutest_encoded.bmp");

        auto in_frame = SDL_ConvertSurfaceFormat( image_source,
                                                 SDL_PIXELFORMAT_RGBA32, 0);
        auto recover = Surfaceable::AllocateSurface( in_frame );
        auto pixels = Pixelable::pixels(recover);
        auto size_bits = TurboFEC::bits(pixels);
        auto mtu = TurboFEC::mtu_downquant( size_bits, DEFAULT_BITDEPTH );
        auto in_cv = Pixelable::AsLumaChannelVector( in_frame );
        size_t new_size = TurboFEC::linear_required_size( mtu.input_bits, DEFAULT_BITDEPTH );
        SDL_Log("mtu.input_bits %d, new_size %zu", mtu.input_bits, new_size );
        in_cv.resize( TurboFEC::bytes( new_size ) );
        auto in_v = TurboFEC::FromChannelVector( in_cv );
        TurboFEC_bitvect_t bv = TurboFEC::init_bitvect();
        TurboFEC::bitupquant( bv, in_v, DEFAULT_BITDEPTH );
        Pixelable_ch_t out_bits;
        TurboFEC::decode( out_bits, bv );
        auto out_cv = TurboFEC::frombits( out_bits );
        out_cv.resize( Pixelable::pixels (recover) );
        Pixelable::ApplyLumaChannelVector( recover, out_cv );
        SDL_SaveBMP(recover, "turbofec_mturecovertest.bmp");

        SDL_FreeSurface( in_frame );
        SDL_FreeSurface( recover );
        SDL_FreeSurface( image_source );
    }

}

#endif //SDL_CRT_FILTER_TURBOTESTS_HPP
