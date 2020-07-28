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
//static size_t frame_len( SDL_Surface* output_surface ) {
//    size_t packets =  Pixelable::pixels( output_surface ) / TurboFEC::bytes( LEN );
//    size_t bytes = packets * TurboFEC::bytes( LEN );
//    return bytes;
//}

// len as bytes, cp as bits
static void dump( SDL_Surface *output_surface, uint8_t *cp, size_t len_bits ) {
    Loader::blank( output_surface );
    auto out = new uint8_t[ TurboFEC::bytes(len_bits) ];
    frombits( out, cp, len_bits );
    Pixelable::ApplyLumaChannelMatrix( output_surface, out );
    delete[] out;
}

// len as bytes, cp as bits
static void dump( SDL_Surface *output_surface, uint8_t *cp, std::string name, size_t len_bits) {
    dump(output_surface, cp, len_bits);
    SDL_SaveBMP( output_surface,  name.c_str() );
}

// len as bytes, cp as bits
static void undump(uint8_t *cp, SDL_Surface *input_surface, size_t len_bits) {
    auto in = Pixelable::AsLumaChannelMatrix( input_surface );
    tobits( cp, in, len_bits );
    delete[] in;
}

static int error_test( const struct lte_test_vector *test,
                      int num_pkts, int iter, float snr )
{
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

    //channelMatrix
    auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
    auto testcard = SDL_ConvertSurfaceFormat( image,
                                             SDL_PIXELFORMAT_RGBA32 , 0 );
    auto copy = Surfaceable::AllocateSurface( testcard );
    auto cm = Pixelable::AsLumaChannelMatrix( testcard );
    Pixelable::ApplyLumaChannelMatrix( copy, cm );
    auto dm = Pixelable::AsLumaChannelMatrix( copy );
    REQUIRE( memcmp( cm, dm, Pixelable::pixels( testcard ) ) == 0 );
    SDL_SaveBMP( copy, "turbofec_cm_test_out.bmp");
    auto image_copy = SDL_LoadBMP("turbofec_cm_test_out.bmp");
    auto recover = SDL_ConvertSurfaceFormat( image_copy,
                                             SDL_PIXELFORMAT_RGBA32 , 0 );
    auto rm = Pixelable::AsLumaChannelMatrix( recover );
    REQUIRE( memcmp( rm, dm, Pixelable::pixels( testcard ) ) == 0 );

    delete [] cm;
    delete [] dm;
    delete [] rm;
    SDL_FreeSurface( copy );
    SDL_FreeSurface( testcard );
    SDL_FreeSurface( recover );
    SDL_FreeSurface( image );
    SDL_FreeSurface( image_copy );
}


TEST_CASE("TurboFEC bitdownquant, bitupquant","[TurboFEC]") {
    auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
    auto testcard = SDL_ConvertSurfaceFormat( image,
                                              SDL_PIXELFORMAT_RGBA32 , 0 );
    auto bitlen = 6;
    auto input_size = TurboFEC::conv_size( testcard );
    auto ints_size = TurboFEC::downquant_size(bitlen, input_size);
    SDL_Log("original size: %lu bits", input_size );
    SDL_Log("downquant new size: %lu bits", ints_size );
    SDL_Log("upquant size: %lu bits", TurboFEC::upquant_size(bitlen, ints_size) );
    REQUIRE( TurboFEC::upquant_size(bitlen, ints_size) == input_size );
    auto uint6 = TurboFEC::Allocate(ints_size*2);

    auto buff = TurboFEC::Allocate( testcard );
    auto buff_r = TurboFEC::Allocate( testcard );
    TurboFEC::encode( buff.turbo, testcard );
    auto new_size = TurboFEC::bitdownquant( uint6, buff.turbo, bitlen, input_size );
    TurboFEC::bitupquant( buff_r.turbo, uint6, bitlen, input_size );
    TurboFEC::dump_partition( uint6[0] );
    TurboFEC::dump_partition( buff.turbo[0] );
    TurboFEC::dump_partition( buff_r.turbo[0] );
    auto encoded_surface = Surfaceable::AllocateSurface( testcard );
    //Pixelable::ApplyLumaChannelMatrix( encoded_surface, uint6[0] );
    //SDL_SaveBMP( encoded_surface, "turbo_bdwq_bitlen.bmp" );
    //dump(encoded_surface, uint6[0], "turbo_bdwq_bitlen.bmp", input_size / 3 );
    REQUIRE( TurboFEC::verbose_memcmp( buff.turbo[0], buff_r.turbo[0], input_size ) == 0);
    REQUIRE( TurboFEC::verbose_memcmp( buff.turbo[1], buff_r.turbo[1], input_size ) == 0);
    REQUIRE( TurboFEC::verbose_memcmp( buff.turbo[2], buff_r.turbo[2], input_size ) == 0);

    SDL_Log( "bitupquant/dwquant read_size: %d, new_quant_size: %lu", input_size / 8, new_size/8 );

    TurboFEC::free(buff.turbo);
    TurboFEC::free(buff_r.turbo);
    TurboFEC::free(uint6);
    delete [] buff.turbo;
    delete [] buff_r.turbo;
    delete [] uint6;
    SDL_FreeSurface( image );
    SDL_FreeSurface( testcard );
    SDL_FreeSurface( encoded_surface );
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

}

TEST_CASE("TurboFEC tests","[TurboFEC]") {

    //tries to turbo encode an image into three images and recover them
    SECTION("API Tests") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                SDL_PIXELFORMAT_RGBA32, 0);
        auto lte_enc = new TurboFEC();
        auto buff = TurboFEC::Allocate(surface);
        TurboFEC::encode(buff.turbo, surface);
        TurboFEC::dump_partition(buff.turbo[0]);
        TurboFEC::dump_partition(buff.turbo[1]);
        TurboFEC::dump_partition(buff.turbo[2]);
        auto dst_rect = TurboFEC::conv_rect(surface);
        auto dst_copy = Surfaceable::AllocateSurface(dst_rect.w, dst_rect.h);
        auto copy = Surfaceable::AllocateSurface(surface);

        dump( dst_copy, buff.turbo[0], "turbo_pix_channel_buff0.bmp", buff.size );
        dump( dst_copy, buff.turbo[1], "turbo_pix_channel_buff1.bmp", buff.size );
        dump( dst_copy, buff.turbo[2], "turbo_pix_channel_buff2.bmp", buff.size );

        SDL_FreeSurface(dst_copy);
        TurboFEC::decode(copy, buff.turbo);
        SDL_SaveBMP(copy, "turbo_pix_channel_decoded.bmp");

        TurboFEC::free(buff.turbo);
        delete [] buff.turbo;
        SDL_FreeSurface(copy);
        SDL_FreeSurface(surface);
        delete lte_enc;
        SDL_FreeSurface( image );
    }

    SECTION("Image Recovery") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                SDL_PIXELFORMAT_RGBA32, 0);

        auto copy = Surfaceable::AllocateSurface(surface);
        auto lte_enc = new TurboFEC();
        auto buff = TurboFEC::Allocate(surface);
        auto lbuff = TurboFEC::Allocate(surface);

        //start of recovery from dumped images
        SDL_Surface* source_faces[3] =  {
                SDL_LoadBMP("turbo_pix_channel_arbuff0.bmp"),
                SDL_LoadBMP("turbo_pix_channel_buff1.bmp"),
                SDL_LoadBMP("turbo_pix_channel_buff2.bmp")
        };

        SDL_Surface* buff_faces[3] = {
                SDL_ConvertSurfaceFormat( source_faces[0], SDL_PIXELFORMAT_RGBA32, 0 ),
                SDL_ConvertSurfaceFormat( source_faces[1], SDL_PIXELFORMAT_RGBA32, 0 ),
                SDL_ConvertSurfaceFormat( source_faces[2], SDL_PIXELFORMAT_RGBA32, 0 )
        };

        undump(lbuff.turbo[0], buff_faces[0], lbuff.size );
        undump(lbuff.turbo[1], buff_faces[1], lbuff.size );
        undump(lbuff.turbo[2], buff_faces[2], lbuff.size );

        TurboFEC::dump_partition(lbuff.turbo[0]);
        TurboFEC::dump_partition(lbuff.turbo[1]);
        TurboFEC::dump_partition(lbuff.turbo[2]);

        //uint8_t* rbuff_min = std::min_element(lbuff[0], lbuff[0] + TurboFEC::conv_size_bits( buff_faces[0] ) );
        //uint8_t* rbuff_max = std::max_element(lbuff[0], lbuff[0] + TurboFEC::conv_size_bits( buff_faces[0] ) );
        //SDL_Log( "recovered buff min / max: %d/%d ", *rbuff_min, *rbuff_max );

        auto recover_rect = TurboFEC::conv_rect( surface );
        auto recover = Surfaceable::AllocateSurface( recover_rect.w, recover_rect.h );
        dump(recover, lbuff.turbo[0], "turbo_pix_channel_rbuff0.bmp", lbuff.size);
        dump(recover, lbuff.turbo[1], "turbo_pix_channel_rbuff1.bmp", lbuff.size);
        dump(recover, lbuff.turbo[2], "turbo_pix_channel_rbuff2.bmp", lbuff.size);

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
//        delete &buff;

        TurboFEC::free( lbuff.turbo );
        delete [] lbuff.turbo;
//        delete &lbuff;
        SDL_FreeSurface( buff_faces[0] );
        SDL_FreeSurface( buff_faces[1] );
        SDL_FreeSurface( buff_faces[2] );
        SDL_FreeSurface( source_faces[0] );
        SDL_FreeSurface( source_faces[1] );
        SDL_FreeSurface( source_faces[2] );
        SDL_FreeSurface( surface );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( recover );
        SDL_FreeSurface( image );
        delete lte_enc;
    }


    SECTION("Encode, then block interleave") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat(image ,
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


        dump(turbo_out[0], buff.turbo[0], buff.size);
        dump(turbo_out[1], buff.turbo[1], buff.size);
        dump(turbo_out[2], buff.turbo[2], buff.size);

        LibAVable::pack_all_recursive( interleaver_out[0],turbo_out[0], 40 );
        LibAVable::pack_all_recursive( interleaver_out[1],turbo_out[1], 40 );
        LibAVable::pack_all_recursive( interleaver_out[2],turbo_out[2], 40 );

        SDL_SaveBMP(interleaver_out[0], "turbo_block_interleave_ch0.bmp");
        SDL_SaveBMP(interleaver_out[1], "turbo_block_interleave_ch1.bmp");
        SDL_SaveBMP(interleaver_out[2], "turbo_block_interleave_ch2.bmp");

        SDL_Surface* source_faces[3] =  {
                SDL_LoadBMP("turbo_block_interleave_ach0.bmp"),
                SDL_LoadBMP("turbo_block_interleave_ch1.bmp"),
                SDL_LoadBMP("turbo_block_interleave_ch2.bmp")
        };


        SDL_Surface* buff_faces[3] = {
                SDL_ConvertSurfaceFormat( source_faces[0],
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( source_faces[1],
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( source_faces[2],
                                          SDL_PIXELFORMAT_RGBA32 , 0 )
        };

        LibAVable::unpack_all_recursive( turbo_in[0],buff_faces[0], 40 );
        LibAVable::unpack_all_recursive( turbo_in[1],buff_faces[1], 40 );
        LibAVable::unpack_all_recursive( turbo_in[2],buff_faces[2], 40 );

        //invalidread warning
        undump(buff.turbo[0], turbo_in[0], buff.size );
        undump(buff.turbo[1], turbo_in[1], buff.size );
        undump(buff.turbo[2], turbo_in[2], buff.size );

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
        SDL_FreeSurface(source_faces[0]);
        SDL_FreeSurface(source_faces[1]);
        SDL_FreeSurface(source_faces[2]);

        SDL_FreeSurface( image );
    }
/*
    SECTION("Encode, then requantize, then block interleave") {
        auto surface = SDL_ConvertSurfaceFormat(SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                                                SDL_PIXELFORMAT_RGBA32, 0);

        auto bitlen = 4;
        auto input_size = TurboFEC::conv_size( surface );
        auto ints_size = TurboFEC::downquant_size(bitlen, input_size);
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

        dump(turbo_out[0], uint6[0], buff.size);
        dump(turbo_out[1], uint6[1], buff.size);
        dump(turbo_out[2], uint6[2], buff.size);

        LibAVable::pack_miniraster( interleaver_out[0],turbo_out[0], 40 );
        LibAVable::pack_miniraster( interleaver_out[1],turbo_out[1], 40 );
        LibAVable::pack_miniraster( interleaver_out[2],turbo_out[2], 40 );

        SDL_SaveBMP(interleaver_out[0], "turbo_block_quant_interleave_ch0.bmp");
        SDL_SaveBMP(interleaver_out[1], "turbo_block_quant_interleave_ch1.bmp");
        SDL_SaveBMP(interleaver_out[2], "turbo_block_quant_interleave_ch2.bmp");

        SDL_Surface* buff_faces[3] = {
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_quant_interleave_ch0.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_quant_interleave_ch1.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 ),
                SDL_ConvertSurfaceFormat( SDL_LoadBMP("turbo_block_quant_interleave_ch2.bmp"),
                                          SDL_PIXELFORMAT_RGBA32 , 0 )
        };



        LibAVable::unpack_miniraster( turbo_in[0],buff_faces[0], 40 );
        LibAVable::unpack_miniraster( turbo_in[1],buff_faces[1], 40 );
        LibAVable::unpack_miniraster( turbo_in[2],buff_faces[2], 40 );

        undump(uint6_r[0], turbo_in[0], buff.size );
        undump(uint6_r[1], turbo_in[1], buff.size );
        undump(uint6_r[2], turbo_in[2], buff.size );

        TurboFEC::bitupquant( buff.turbo, uint6_r, bitlen, TurboFEC::conv_size( surface ) );

        auto recover = Surfaceable::AllocateSurface( surface );
        TurboFEC::decode( recover, buff.turbo );
        SDL_SaveBMP(recover, "turbo_block_requant_decoded.bmp");

        TurboFEC::free( uint6 );
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
*/
}


TEST_CASE("TurboFEC tests, frame stream","[TurboFEC]") {

    SECTION("TurboFEC as and from channelmatrix") {
        auto image = SDL_LoadBMP("resources/images/testCardRGB.bmp");
        auto surface = SDL_ConvertSurfaceFormat( image,
                                                SDL_PIXELFORMAT_RGBA32, 0);
        auto recover = Surfaceable::AllocateSurface(surface);
        auto save = Surfaceable::AllocateSurface(surface);
        Loader::blank(recover);

        auto b = TurboFEC::Allocate( surface );
        auto test_cm = Pixelable::AsLumaChannelMatrix( surface );
        auto rec_cm = Pixelable::AsLumaChannelMatrix( recover );

        TurboFEC::FromChannelMatrix( b, test_cm );
        TurboFEC::AsChannelMatrix( rec_cm, b );

        Pixelable::ApplyLumaChannelMatrix(recover, rec_cm);
        Pixelable::ApplyLumaChannelMatrix(save, test_cm);
        SDL_SaveBMP(recover, "turbofec_channelmatrix_idempotency.bmp");
        SDL_SaveBMP(save, "turbofec_channelmatrix_idempotency2.bmp");

        REQUIRE( memcmp( rec_cm, test_cm, TurboFEC::input_bytes( Pixelable::pixels( recover ) )) == 0 );
        auto image_copy = SDL_LoadBMP("turbofec_channelmatrix_idempotency.bmp");
        auto copy = SDL_ConvertSurfaceFormat(image_copy,
                                             SDL_PIXELFORMAT_RGBA32, 0);

        auto copy_cm = Pixelable::AsLumaChannelMatrix( copy );
        REQUIRE( memcmp( copy_cm, test_cm, TurboFEC::input_bytes( Pixelable::pixels( recover ) )) == 0 );

        TurboFEC::free( b.turbo );
        delete[] b.turbo;
//        delete &b;
        SDL_FreeSurface( recover );
        SDL_FreeSurface( surface );
        SDL_FreeSurface( save );
        SDL_FreeSurface( copy );
        SDL_FreeSurface( image );
        SDL_FreeSurface( image_copy );
        delete[] test_cm;
        delete[] rec_cm;
        delete[] copy_cm;
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

        //it allocates 3 buffers to store the turbo encoder output
        auto b = TurboFEC::Allocate( surface );
        //this allocates 3 b. too. For the comparision against reconstructed data
        auto c = TurboFEC::Allocate( surface );

        //it first encodes a channelmatrix surface to in_bits
        auto surface_cm = Pixelable::AsLumaChannelMatrix( surface );
        auto in_bits = new uint8_t[mtu.input_bits];
        TurboFEC::tobits( in_bits, surface_cm, mtu.input_bits );

        //then it passes through the encoder, and prints sha256 of the first 1024 bytes
        TurboFEC::encode( b.turbo, in_bits, mtu.input_bits );
        auto sha256 = Loader::sha256Log( surface_cm, TurboFEC::bytes(mtu.input_bits) -1 );
        SDL_Log("Input sha256: %s", sha256.c_str() );

        //it prepares a downconverted version
        auto new_size = TurboFEC::downquant_size( DEFAULT_BITDEPTH, mtu.output_bits );
        auto b6 = TurboFEC::AllocateC(new_size*2);
        TurboFEC::bitdownquant( b6.turbo, b.turbo, DEFAULT_BITDEPTH, mtu.output_bits );
        SDL_Log( "Downconverted size: %zu, unconverted %d bits, total per frame %zu", new_size, mtu.output_bits, size_bits );

        //it recovers the downconverted version
        TurboFEC::bitupquant( c.turbo, b6.turbo, DEFAULT_BITDEPTH, b.size );

        //then this part decodes it for sha verification, and prepares a blank recover
        //surface to print the output to
        auto out_bits = new uint8_t[b6.size];
        auto recover = Surfaceable::AllocateSurface(surface);
        Loader::blank( recover );
        auto out_cm = new uint8_t[b6.size];

        REQUIRE( TurboFEC::verbose_memcmp( b.turbo[0], c.turbo[0], mtu.input_bits ) == 0);
        REQUIRE( TurboFEC::verbose_memcmp( b.turbo[1], c.turbo[1], mtu.input_bits ) == 0);
        REQUIRE( TurboFEC::verbose_memcmp( b.turbo[2], c.turbo[2], mtu.input_bits ) == 0);

        //here the decoder action
        TurboFEC::decode(out_bits, c.turbo, mtu.input_bits );
        TurboFEC::frombits(out_cm, out_bits, mtu.input_bits );
        auto sha256o = Loader::sha256Log(out_cm, TurboFEC::bytes(mtu.input_bits) -1 );
        SDL_Log("Output sha256: %s", sha256o.c_str());

        //it blanks the non encoded part of the original channelmatrix surface for comparision
        memset(&surface_cm [ TurboFEC::bytes(mtu.input_bits) ], 0 , pixels - TurboFEC::bytes(mtu.input_bits) - 1 );
        REQUIRE( TurboFEC::verbose_memcmp( surface_cm, out_cm, TurboFEC::bytes(mtu.input_bits) - 1 ) == 0);

        //it saves the result of the decoder as an image
        Pixelable::ApplyLumaChannelMatrix( recover, out_cm );
        SDL_SaveBMP(recover, "turbofec_mtutest_decoded.bmp");

        //it prepares recover for reuse, and enc_cm for encode
        Loader::blank(recover);
        auto enc_cm = Pixelable::AsLumaChannelMatrix(recover);
        TurboFEC::AsChannelMatrix(enc_cm, b6);
        Pixelable::ApplyLumaChannelMatrix( recover, enc_cm );
        SDL_SaveBMP(recover, "turbofec_mtutest_encoded.bmp");

        TurboFEC::free(b.turbo);
        delete[] b.turbo;
        TurboFEC::free(b6.turbo);
        delete[] b6.turbo;
        TurboFEC::free(c.turbo);
        delete[] c.turbo;

        SDL_FreeSurface(surface);
        SDL_FreeSurface(recover);
        SDL_FreeSurface( image );

        delete[] surface_cm;
        delete[] in_bits;
        delete[] out_bits;
        delete[] out_cm;
        delete[] enc_cm;
    }

    SECTION("Test MTU decode") {
        SDL_Surface* images[2] = {
                SDL_LoadBMP("resources/images/testCardRGB.bmp"),
                SDL_LoadBMP("turbofec_mtutest_encoded.bmp")
        };

        auto surface = SDL_ConvertSurfaceFormat( images[0],
                                                SDL_PIXELFORMAT_RGBA32, 0);

        auto in_frame = SDL_ConvertSurfaceFormat( images[1],
                                                 SDL_PIXELFORMAT_RGBA32, 0);
        auto recover = Surfaceable::AllocateSurface( surface );
        auto pixels = Pixelable::pixels(recover);
        auto size_bits = TurboFEC::bits(pixels);
        auto mtu = TurboFEC::mtu_downquant( size_bits, DEFAULT_BITDEPTH );
        auto in_bits = new uint8_t[mtu.input_bits];
        auto inc_bits = new uint8_t[mtu.input_bits];
        auto b = TurboFEC::Allocate( surface );
        auto c = TurboFEC::Allocate( surface );
        b.size = mtu.output_bits;
        c.size = mtu.output_bits;
        auto out_cm = Pixelable::AsLumaChannelMatrix(recover);

        auto img_size = TurboFEC::bytes(b.size);
        SDL_Log("px/bytes: %lu, img_size: %lu pxby", pixels, img_size);

        //it loads the channelmatrix and then upconverts it
        auto dat_cm = Pixelable::AsLumaChannelMatrix( in_frame );
        auto new_size = TurboFEC::downquant_size( DEFAULT_BITDEPTH, mtu.output_bits );
        auto b6 = TurboFEC::AllocateC(new_size);
        TurboFEC::FromChannelMatrix( b6, dat_cm );
        TurboFEC::bitupquant( b.turbo, b6.turbo, DEFAULT_BITDEPTH, mtu.output_bits );

        //then it decodes it
        TurboFEC::decode(in_bits, b.turbo, mtu.input_bits );

        //makes a copy of the original data to compare to
        auto ref_cm = Pixelable::AsLumaChannelMatrix( surface );
        TurboFEC::tobits(inc_bits, ref_cm, mtu.input_bits );
        TurboFEC::encode(c.turbo, inc_bits, mtu.input_bits );

        bool test;
        for( auto i = 0; i < 3; i++ ) {
            SDL_Log("Verifying the turbo encoded data ... %d bits, buffer %d", mtu.input_bits, i );
            test &= TurboFEC::verbose_memcmp(c.turbo[i], b.turbo[i], mtu.input_bits);
        }

        //REQUIRE( test );

        TurboFEC::frombits(out_cm, in_bits, mtu.input_bits );
        SDL_Log("Verifying recovered data, %d bits", mtu.input_bits );
        TurboFEC::verbose_memcmp(ref_cm, out_cm, TurboFEC::bytes(mtu.input_bits) -1);
        //REQUIRE( TurboFEC::verbose_memcmp(ref_cm, out_cm, TurboFEC::bytes(mtu.input_bits) -1) == 0);
        Pixelable::ApplyLumaChannelMatrix(recover, out_cm);
        SDL_SaveBMP( recover, "turbofec_mturecovertest.bmp" );

        delete[] ref_cm;
        delete[] in_bits;
        delete[] inc_bits;
        delete[] dat_cm;
        delete[] out_cm;
        //delete[] dat_ref;
        TurboFEC::free(b.turbo);
        delete[] b.turbo;
        TurboFEC::free(c.turbo);
        delete[] c.turbo;
        TurboFEC::free(b6.turbo);
        delete[] b6.turbo;
        SDL_FreeSurface( in_frame );
        SDL_FreeSurface( recover );
        SDL_FreeSurface( surface );
        SDL_FreeSurface(images[0]);
        SDL_FreeSurface(images[1]);
        //SDL_FreeSurface( copy );

    }

}

#endif //SDL_CRT_FILTER_TURBOTESTS_HPP
