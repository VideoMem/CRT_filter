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
static int uint8_to_err(int8_t *dst, uint8_t *src, int n, float snr)
{
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

//len as bytes
static size_t frame_len( SDL_Surface* output_surface ) {
    size_t packets =  Pixelable::pixels( output_surface ) / TurboFEC::bytes( LEN );
    size_t bytes = packets * TurboFEC::bytes( LEN );
    return bytes;
}

// len as bytes, cp as bits
static void dump ( uint8_t* cp, SDL_Surface* output_surface ) {
    Loader::blank( output_surface );
    auto bytes = frame_len( output_surface );
    auto out = new uint8_t[ bytes ];
    frombits( out, cp, TurboFEC::bits( bytes ) );
    Pixelable::ApplyLumaChannelMatrix( output_surface, out );
    delete[] out;
}



// len as bytes, cp as bits
static void dump ( uint8_t* cp, SDL_Surface* output_surface, std::string name ) {
    dump ( cp, output_surface );
    SDL_SaveBMP( output_surface,  name.c_str() );
}

// len as bytes, cp as bits
static void undump ( uint8_t* cp, SDL_Surface* input_surface ) {
    auto bytes = frame_len( input_surface );
    auto in = Pixelable::AsLumaChannelMatrix( input_surface );
    tobits( cp, in, TurboFEC::bits( bytes ) );
}


static void dump_partition( uint8_t* in, size_t start=0, size_t end=64) {
    printf( "Dump partition [%03zu~%03zu]: ", start, end );
    for ( auto i = start; i < end; i ++ ) {
        printf( "%u",  in[i] );
    }
    printf( "\r\n" );
}

static int error_test( const struct lte_test_vector *test,
                      int num_pkts, int iter, float snr )
{
    // dump buffers as surfaces
    auto testcard = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
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

    tobits(in_bits, in, test->in_len );

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


    dump( bu.turbo[0], output_surface, "turbo_error_test_out.bmp" );

    dump( bs.turbo[0], output_surface, "turbo_error_test_bs0.bmp" );
    dump( bs.turbo[1], output_surface, "turbo_error_test_bs1.bmp" );
    dump( bs.turbo[2], output_surface, "turbo_error_test_bs2.bmp" );

    dump( bu.turbo[0], output_surface, "turbo_error_test_bu0.bmp" );
    dump( bu.turbo[1], output_surface, "turbo_error_test_bu1.bmp" );
    dump( bu.turbo[2], output_surface, "turbo_error_test_bu2.bmp" );

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

    //channelMatrix
    auto testcard = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                             SDL_PIXELFORMAT_RGBA32 , 0 );
    auto copy = Surfaceable::AllocateSurface( testcard );
    auto cm = Pixelable::AsLumaChannelMatrix( testcard );
    Pixelable::ApplyLumaChannelMatrix( copy, cm );
    auto dm = Pixelable::AsLumaChannelMatrix( copy );
    REQUIRE( memcmp( cm, dm, Pixelable::pixels( testcard ) ) == 0 );
    SDL_SaveBMP( copy, "turbofec_cm_test_out.bmp");
    auto recover = SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbofec_cm_test_out.bmp"),
                                             SDL_PIXELFORMAT_RGBA32 , 0 );
    auto rm = Pixelable::AsLumaChannelMatrix( recover );
    REQUIRE( memcmp( rm, dm, Pixelable::pixels( testcard ) ) == 0 );

    delete [] cm;
    delete [] dm;
    delete [] rm;
    SDL_FreeSurface( copy );
    SDL_FreeSurface( testcard );
    SDL_FreeSurface( recover );
}


TEST_CASE("TurboFEC bitdownquant, bitupquant","[TurboFEC]") {
    auto testcard = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                              SDL_PIXELFORMAT_RGBA32 , 0 );
    auto bitlen = 6;
    auto input_size = TurboFEC::conv_size( testcard );
    auto ints_size = TurboFEC::bits( TurboFEC::int_count(bitlen, TurboFEC::bits(input_size) ) );
    auto uint6 = TurboFEC::Allocate(ints_size);

    auto buff = TurboFEC::Allocate( testcard );
    auto buff_r = TurboFEC::Allocate( testcard );
    TurboFEC::encode( buff.turbo, testcard );
    auto new_size = TurboFEC::bitdownquant( uint6, buff.turbo, bitlen, input_size );
    TurboFEC::bitupquant( buff_r.turbo, uint6, bitlen, new_size );
    dump_partition( uint6[0] );
    dump_partition( buff.turbo[0] );
    dump_partition( buff_r.turbo[0] );
    auto repack_bitlen = Surfaceable::AllocateSurface( testcard );
    dump( uint6[0], repack_bitlen, "turbo_bdwq_bitlen.bmp" );
    REQUIRE( memcmp( buff.turbo[0], buff_r.turbo[0], buff.size ) == 0);
    REQUIRE( memcmp( buff.turbo[1], buff_r.turbo[1], buff.size ) == 0);
    REQUIRE( memcmp( buff.turbo[2], buff_r.turbo[2], buff.size ) == 0);

    SDL_Log( "bitupquant/dwquant read_size: %d, new_quant_size: %lu", input_size / 8, new_size/8 );

    TurboFEC::free(buff.turbo);
    TurboFEC::free(buff_r.turbo);
    TurboFEC::free(uint6);
    delete [] buff.turbo;
    delete [] buff_r.turbo;
    delete [] uint6;
    SDL_FreeSurface( testcard );
    SDL_FreeSurface( repack_bitlen );
}


TEST_CASE("TurboFEC tobits, frombits","[TurboFEC]") {
    auto pixels = TurboFEC::bytes( LEN );
    auto full = new uint8_t[pixels];
    for ( int i = 0; i < pixels; i++ ) full[i] = 0xFF - i;

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


TEST_CASE("TurboFEC error test","[TurboFEC]") {
    printf("\n[.] BER test:\n");
    printf("[..] Testing:\n");
    srand(time(NULL));
    REQUIRE (error_test(tests, DEFAULT_NUM_PKTS,
                   DEFAULT_ITER, DEFAULT_SNR) >= 0);

}


TEST_CASE("Pixel data converter tests","[TurboFEC]") {
    SECTION("Pixelable channel surfaces") {
        auto surface = SDL_ConvertSurfaceFormat( SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                 SDL_PIXELFORMAT_RGBA32 , 0 );

        auto cm = Pixelable::AsLumaChannelMatrix( surface );
        auto copy = Surfaceable::AllocateSurface( surface );
        Pixelable::ApplyLumaChannelMatrix( copy, cm );
        SDL_SaveBMP( copy, "turbo_pix_channel_test.bmp");

        SDL_FreeSurface( surface );
        SDL_FreeSurface( copy );
        delete [] cm;
    }

}

TEST_CASE("TurboFEC tests","[TurboFEC]") {

    //tries to turbo encode an image into three images and recover them
    SECTION("API Tests") {
        auto surface = SDL_ConvertSurfaceFormat(SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                SDL_PIXELFORMAT_RGBA32, 0);
        auto lte_enc = new TurboFEC();
        auto buff = TurboFEC::Allocate(surface);
        TurboFEC::encode(buff.turbo, surface);
        dump_partition(buff.turbo[0]);
        dump_partition(buff.turbo[1]);
        dump_partition(buff.turbo[2]);
        auto dst_rect = TurboFEC::conv_rect(surface);
        auto dst_copy = Surfaceable::AllocateSurface(dst_rect.w, dst_rect.h);
        auto copy = Surfaceable::AllocateSurface(surface);

        dump(buff.turbo[0], dst_copy, "turbo_pix_channel_buff0.bmp");
        dump(buff.turbo[1], dst_copy, "turbo_pix_channel_buff1.bmp");
        dump(buff.turbo[2], dst_copy, "turbo_pix_channel_buff2.bmp");

        SDL_FreeSurface(dst_copy);
        TurboFEC::decode(copy, buff.turbo);
        SDL_SaveBMP(copy, "turbo_pix_channel_decoded.bmp");

        TurboFEC::free(buff.turbo);
        delete [] buff.turbo;
        SDL_FreeSurface(copy);
        SDL_FreeSurface(surface);
        delete lte_enc;
    }

    SECTION("Image Recovery") {
        auto surface = SDL_ConvertSurfaceFormat(SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                SDL_PIXELFORMAT_RGBA32, 0);

        auto copy = Surfaceable::AllocateSurface(surface);
        auto lte_enc = new TurboFEC();
        auto buff = TurboFEC::Allocate(surface);
        auto lbuff = TurboFEC::Allocate(surface);

        //start of recovery from dumped images
        SDL_Surface* buff_faces[3] = {
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_pix_channel_arbuff0.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_pix_channel_buff1.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_pix_channel_buff2.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 )
        };

        undump( lbuff.turbo[0], buff_faces[0] );
        undump( lbuff.turbo[1], buff_faces[1] );
        undump( lbuff.turbo[2], buff_faces[2] );

        dump_partition(lbuff.turbo[0]);
        dump_partition(lbuff.turbo[1]);
        dump_partition(lbuff.turbo[2]);

        //uint8_t* rbuff_min = std::min_element(lbuff[0], lbuff[0] + TurboFEC::conv_size_bits( buff_faces[0] ) );
        //uint8_t* rbuff_max = std::max_element(lbuff[0], lbuff[0] + TurboFEC::conv_size_bits( buff_faces[0] ) );
        //SDL_Log( "recovered buff min / max: %d/%d ", *rbuff_min, *rbuff_max );

        auto recover_rect = TurboFEC::conv_rect( surface );
        auto recover = Surfaceable::AllocateSurface( recover_rect.w, recover_rect.h );
        dump( lbuff.turbo[0], recover, "turbo_pix_channel_rbuff0.bmp" );
        dump( lbuff.turbo[1], recover, "turbo_pix_channel_rbuff1.bmp" );
        dump( lbuff.turbo[2], recover, "turbo_pix_channel_rbuff2.bmp" );

        //TurboFEC::encode( buff, surface );
        //uint8_t* buff_min = std::min_element(buff[0], buff[0] + TurboFEC::conv_size_bits( buff_faces[0] ) );
        //uint8_t* buff_max = std::max_element(buff[0], buff[0] + TurboFEC::conv_size_bits( buff_faces[0] ) );
        //SDL_Log( "buff min / max: %d/%d ", *buff_min, *buff_max );

        //REQUIRE ( memcmp ( buff[0], lbuff[0], TurboFEC::conv_size_bits( buff_faces[0] ) ) == 0 );
        //REQUIRE ( memcmp ( buff[1], lbuff[1], TurboFEC::conv_size_bits( buff_faces[1] ) ) == 0 );
        //REQUIRE ( memcmp ( buff[2], lbuff[2], TurboFEC::conv_size_bits( buff_faces[2] ) ) == 0 );

        TurboFEC::decode( copy, lbuff.turbo );
        SDL_SaveBMP( copy, "turbo_pix_channel_decoded_from_images.bmp");

        TurboFEC::free( buff.turbo );
        delete [] buff.turbo;

        //TurboFEC::free( bit_buff );
        TurboFEC::free( lbuff.turbo );
        //delete [] bit_buff;
        SDL_FreeSurface( buff_faces[0] );
        SDL_FreeSurface( buff_faces[1] );
        SDL_FreeSurface( buff_faces[2] );
        SDL_FreeSurface( surface );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( recover );
        delete lte_enc;
    }


    SECTION("Encode, then block interleave") {
        auto surface = SDL_ConvertSurfaceFormat(SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                SDL_PIXELFORMAT_RGBA32, 0);
        auto lte_enc = new TurboFEC();
        auto buff = TurboFEC::Allocate(surface);
        TurboFEC::encode(buff.turbo, surface);

        SDL_Surface *turbo_out[3] = {
            Surfaceable::AllocateSurface( surface ),
            Surfaceable::AllocateSurface( surface ),
            Surfaceable::AllocateSurface( surface )
        };
        SDL_Surface *interleaver_out[3] = {
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface )
        };

        SDL_Surface *turbo_in[3] = {
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface )
        };


        dump( buff.turbo[0], turbo_out[0] );
        dump( buff.turbo[1], turbo_out[1] );
        dump( buff.turbo[2], turbo_out[2] );

        LibAVable::pack_all_recursive( interleaver_out[0],turbo_out[0], 40 );
        LibAVable::pack_all_recursive( interleaver_out[1],turbo_out[1], 40 );
        LibAVable::pack_all_recursive( interleaver_out[2],turbo_out[2], 40 );

        SDL_SaveBMP(interleaver_out[0], "turbo_block_interleave_ch0.bmp");
        SDL_SaveBMP(interleaver_out[1], "turbo_block_interleave_ch1.bmp");
        SDL_SaveBMP(interleaver_out[2], "turbo_block_interleave_ch2.bmp");

        SDL_Surface* buff_faces[3] = {
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_interleave_ach0.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_interleave_ch1.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_interleave_ch2.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 )
        };

        LibAVable::unpack_all_recursive( turbo_in[0],buff_faces[0], 40 );
        LibAVable::unpack_all_recursive( turbo_in[1],buff_faces[1], 40 );
        LibAVable::unpack_all_recursive( turbo_in[2],buff_faces[2], 40 );

        undump(buff.turbo[0], turbo_in[0] );
        undump(buff.turbo[1], turbo_in[1] );
        undump(buff.turbo[2], turbo_in[2] );

        auto recover = Surfaceable::AllocateSurface( surface );
        TurboFEC::decode( recover, buff.turbo );
        SDL_SaveBMP(recover, "turbo_block_decoded.bmp");

        TurboFEC::free( buff.turbo );
        delete [] buff.turbo;
        delete lte_enc;
        SDL_FreeSurface( surface );
        SDL_FreeSurface( recover );
        SDL_FreeSurface(buff_faces[0]);
        SDL_FreeSurface(buff_faces[1]);
        SDL_FreeSurface(buff_faces[2]);
        SDL_FreeSurface(turbo_out[0]);
        SDL_FreeSurface(turbo_out[1]);
        SDL_FreeSurface(turbo_out[2]);
        SDL_FreeSurface(interleaver_out[0]);
        SDL_FreeSurface(interleaver_out[1]);
        SDL_FreeSurface(interleaver_out[2]);
        SDL_FreeSurface(turbo_in[0]);
        SDL_FreeSurface(turbo_in[1]);
        SDL_FreeSurface(turbo_in[2]);

    }

    SECTION("Encode, then requantize, then block interleave") {
        auto surface = SDL_ConvertSurfaceFormat(SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                SDL_PIXELFORMAT_RGBA32, 0);

        auto bitlen = 4;
        auto input_size = TurboFEC::conv_size( surface );
        auto ints_size = TurboFEC::bits( TurboFEC::int_count(bitlen, TurboFEC::bits(input_size) ) );
        auto uint6 = TurboFEC::Allocate(ints_size);
        auto uint6_r = TurboFEC::Allocate(ints_size);
        auto buff = TurboFEC::Allocate(surface);

        TurboFEC::encode(buff.turbo, surface);

        SDL_Surface *turbo_out[3] = {
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface )
        };

        SDL_Surface *interleaver_out[3] = {
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface )
        };

        SDL_Surface *turbo_in[3] = {
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface ),
                Surfaceable::AllocateSurface( surface )
        };

        auto new_size = TurboFEC::bitdownquant( uint6, buff.turbo, bitlen, input_size );

        dump( uint6[0], turbo_out[0] );
        dump( uint6[1], turbo_out[1] );
        dump( uint6[2], turbo_out[2] );

        LibAVable::pack_all_recursive( interleaver_out[0],turbo_out[0], 40 );
        LibAVable::pack_all_recursive( interleaver_out[1],turbo_out[1], 40 );
        LibAVable::pack_all_recursive( interleaver_out[2],turbo_out[2], 40 );

        SDL_SaveBMP(interleaver_out[0], "turbo_block_quant_interleave_ch0.bmp");
        SDL_SaveBMP(interleaver_out[1], "turbo_block_quant_interleave_ch1.bmp");
        SDL_SaveBMP(interleaver_out[2], "turbo_block_quant_interleave_ch2.bmp");

        SDL_Surface* buff_faces[3] = {
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_quant_interleave_ach0.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_quant_interleave_ch1.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_quant_interleave_ch2.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 )
        };



        LibAVable::unpack_all_recursive( turbo_in[0],buff_faces[0], 40 );
        LibAVable::unpack_all_recursive( turbo_in[1],buff_faces[1], 40 );
        LibAVable::unpack_all_recursive( turbo_in[2],buff_faces[2], 40 );

        undump(uint6_r[0], turbo_in[0] );
        undump(uint6_r[1], turbo_in[1] );
        undump(uint6_r[2], turbo_in[2] );

        TurboFEC::bitupquant( buff.turbo, uint6_r, bitlen, TurboFEC::conv_size( surface ) );

        auto recover = Surfaceable::AllocateSurface( surface );
        TurboFEC::decode( recover, buff.turbo );
        SDL_SaveBMP(recover, "turbo_block_requant_decoded.bmp");

        //TurboFEC::free( uint6 );
        TurboFEC::free( buff.turbo );
        delete [] buff.turbo;
        delete [] uint6;
        SDL_FreeSurface( surface );
        SDL_FreeSurface( recover );
        SDL_FreeSurface(buff_faces[0]);
        SDL_FreeSurface(buff_faces[1]);
        SDL_FreeSurface(buff_faces[2]);
        SDL_FreeSurface(turbo_out[0]);
        SDL_FreeSurface(turbo_out[1]);
        SDL_FreeSurface(turbo_out[2]);
        SDL_FreeSurface(interleaver_out[0]);
        SDL_FreeSurface(interleaver_out[1]);
        SDL_FreeSurface(interleaver_out[2]);
        SDL_FreeSurface(turbo_in[0]);
        SDL_FreeSurface(turbo_in[1]);
        SDL_FreeSurface(turbo_in[2]);

    }
    
}


TEST_CASE("TurboFEC tests, frame stream","[TurboFEC]") {

    SECTION("Test MTU") {
        auto surface = SDL_ConvertSurfaceFormat(SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                SDL_PIXELFORMAT_RGBA32, 0);

        auto size = TurboFEC::bits( Pixelable::pixels( surface ) );
        auto mtu = TurboFEC::mtu( size );
        SDL_Log( "MTU size %lu bits, padding %lu bits :: size %zu bytes, padding %zu bytes", mtu, size - mtu, TurboFEC::bytes(mtu), TurboFEC::bytes( size - mtu ) );
        auto b = TurboFEC::AllocateC( mtu );

        SDL_FreeSurface( surface );
    }

}

#endif //SDL_CRT_FILTER_TURBOTESTS_HPP
