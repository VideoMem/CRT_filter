//
// Created by sebastian on 8/5/20.
//

#ifndef SDL_CRT_FILTER_ZMQTESTS_HPP
#define SDL_CRT_FILTER_ZMQTESTS_HPP
#include <loaders/ZMQVideoPipe.hpp>
#include <loaders/ZMQLoader.hpp>
#include <thread>

double quadrant_test(ZMQVideoPipe& zPipe, float a, float b) {
    float c,d;
    uint8_t q;
    q = zPipe.quantize(a, b);
    zPipe.unquantize(q ,c, d);
    double error = abs( zPipe.angle(a, b) - zPipe.angle(c, d) );
    SDL_Log("real input %f, imaginary input %f\n quantized value -> q %d\n-> real result %f, imaginary result %f\n angle input: %f, angle transform: %f error %f", a ,b ,q , c, d, zPipe.angle(a,b), zPipe.angle(c,d), error);
//    assert( (a > 0 && c < 0) || (a < 0 && c > 0) );
//    assert( (b > 0 && d < 0) || (b < 0 && d > 0) );
    //assert(error <  M_PI/2 );
    return zPipe.angle(c, d);
}

bool quantize_am_test() {
    float one = 1;
    float uq = 0;
    for( float x = 1; x >=-1 ; x-=0.01 ) {
        uint8_t quant = ZMQVideoPipe::quantize_am(x, one);
        ZMQVideoPipe::unquantize_am(quant, uq, one );
        if( abs(uq - x ) > 0.01 ) {
            printf( "[%d] %f != %f\n", quant, x, uq);
            return false;
        }
    }
    return true;
}

TEST_CASE( "ZMQ API", "[ZMQ][SDL2][GNURadio]") {

    SECTION( "Wav File handler" ) {
        size_t size = 1024;
        char* data = new char[size];
        memset(data, 0, size);
        ZMQVideoPipe zPipe;
        zPipe.wave.setFM();
        zPipe.wave.write("test.wav", data, size);
        SDL_Log("head:\n%s", zPipe.wave.getInfo().c_str());
    }

    SECTION( "VideoPipe AM quantization" ) {
        REQUIRE(quantize_am_test());
    }

    SECTION( "VideoPipe FM phase quantization" ) {
        ZMQVideoPipe zPipe;

        //quadrants
        quadrant_test(zPipe, 1,1);
        quadrant_test(zPipe, 1,-1);
        quadrant_test(zPipe, -1,1);
        quadrant_test(zPipe, -1,-1);

        //zeroes
        quadrant_test(zPipe, 0.01,1);
        quadrant_test(zPipe, 0.01,-1);
        quadrant_test(zPipe, -0.01,1);
        quadrant_test(zPipe, -0.01,-1);
        quadrant_test(zPipe,-1,0.01);
        quadrant_test(zPipe, 1,0.01);
        quadrant_test(zPipe,-1,-0.01);
        quadrant_test(zPipe, 1,-0.01);
/*
        for(float x = -1; x < 1; x+=0.1)
            for(float y = -1; y < 1; y+=0.1)
                quadrant_test(zPipe, x,y);
 */
    }

    SECTION( "ZMQ REQ/REP complex pipe" ) {
        ZMQVideoPipe zPipe;
        for(int i = 1e2; i > 0; --i)
            zPipe.testPassThru();
    }

    SECTION( "ZMQ REQ/REP quantized pipe" ) {
        ZMQVideoPipe zPipe;
        for(int i = 1e2; i > 0; --i)
            zPipe.testPassThruQuant();
    }

    SECTION( "ZMQ REQ Source" ) {
        ZMQVideoPipe zPipe;
        SDL_Surface* sample = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
        zPipe.testReceiveFrame();
        zPipe.GetSurface(sample);
        SDL_SaveBMP(sample, "encoded.bmp");
        SDL_FreeSurface(sample);
    }

    SECTION( "GNURadio surface to complex, complex to surface") {
        ZMQVideoPipe zPipe;
        SDL_Surface* sample = SDL_LoadBMP("encoded.bmp");
        SDL_Surface* recover = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );

        const int copy_size = 2 * Config::NKERNEL_WIDTH * Config::NKERNEL_HEIGHT;
        auto *copy = new float[copy_size];

        zPipe.frame_to_float( sample, copy );
        zPipe.float_to_frame( copy, recover );
        SDL_SaveBMP(recover, "error.bmp");
        auto* wave = new uint8_t[Config::NKERNEL_WIDTH * Config::NKERNEL_HEIGHT];
        auto* hoo = new uint8_t[Config::NKERNEL_WIDTH * Config::NKERNEL_HEIGHT];
        size_t siz = zPipe.surface_to_wave( sample, wave );
        zPipe.wave.setFM();
        zPipe.wave.write("encoded.wav", wave, siz );
        SDL_Log("->>> encoded:\n%s", zPipe.wave.getInfo().c_str());
        zPipe.wave.read("encoded_mess.wav", hoo );
        zPipe.wave_to_surface( hoo, sample );
        SDL_SaveBMP(recover, "encoded_mess.bmp");
        delete [] wave;
        //REQUIRE(ZMQVideoPipe::CompareSurface(sample, recover));
        delete [] copy;
        SDL_FreeSurface(sample);
        SDL_FreeSurface(recover);
    }

    /*
    SECTION( "ZMQ REP Sink" ) {
        ZMQVideoPipe zPipe;
        SDL_Surface* sample = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );

        sample = SDL_LoadBMP("phone_test.bmp");
        for (int i=6; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_blurry.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_copied.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_modulate.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_flop.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_negate.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_flip.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_flipflop.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_noisy.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_webp.bmp");
        assert(sample != nullptr);
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );


        SDL_FreeSurface(sample);
    }
    */

    SECTION( "ZMQ REQ/REP quantized frame pipe" ) {
        ZMQVideoPipe zPipe;
        for(int i = 10; i > 0; --i)
            zPipe.testFramePassThru();
    }

}

volatile bool quit_signal = false;

void send_frame(ZMQVideoPipe* zPipe) {
    while(!quit_signal)
        zPipe->pushFrame();
}

TEST_CASE( "ZMQ Loader", "[ZMQ][SDL2][App]") {
    SECTION( "ZMQ Loader inproc data flow" ) {
        Config cfg;
        ZMQLoader zLoader;
        cfg.initResources(zLoader);
        BaseApp app(zLoader);
        ZMQVideoPipe zPipe;
        std::thread radio_tx(send_frame, &zPipe);
        SDL_Surface *frame = Loader::AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);

        for( int count = 20; count > 0 ; --count ) {
            zLoader.pullFrame();
            zLoader.GetSurface(frame);
            app.publish(frame);
            zPipe.testSendFrame(frame);
        }

        quit_signal = true;
        SDL_FreeSurface(frame);
        radio_tx.join();
    }
}

#endif //SDL_CRT_FILTER_ZMQTESTS_HPP
