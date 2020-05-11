//
// Created by sebastian on 8/5/20.
//

#ifndef SDL_CRT_FILTER_ZMQTESTS_HPP
#define SDL_CRT_FILTER_ZMQTESTS_HPP
#include <loaders/ZMQVideoPipe.hpp>


TEST_CASE( "ZMQ API", "[ZMQ][SDL2][GNURadio]") {
/*
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
  */
    SECTION( "ZMQ REQ Source" ) {
        ZMQVideoPipe zPipe;
        SDL_Surface* sample = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
        zPipe.testReceiveFrame();
        zPipe.GetSurface(sample);
        SDL_SaveBMP(sample, "encoded.bmp");
        SDL_FreeSurface(sample);
    }
/*

    SECTION( "GNURadio surface to complex, complex to surface") {
        ZMQVideoPipe zPipe;
        SDL_Surface* sample = SDL_LoadBMP("encoded.bmp");
        SDL_Surface* recover = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );

        const int copy_size = 2 * Config::NKERNEL_WIDTH * Config::NKERNEL_HEIGHT;
        auto *copy = new float[copy_size];

        zPipe.frame_to_float( sample, copy );
        zPipe.float_to_frame( copy, recover );
        SDL_SaveBMP(recover, "error.bmp");

        //REQUIRE(ZMQVideoPipe::CompareSurface(sample, recover));
        delete [] copy;
        SDL_FreeSurface(sample);
        SDL_FreeSurface(recover);
    }

*/
    SECTION( "ZMQ REP Sink" ) {
        ZMQVideoPipe zPipe;
        SDL_Surface* sample = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );

        sample = SDL_LoadBMP("loop.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_backwards.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_copied.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_blurry.bmp");
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

        sample = SDL_LoadBMP("loop_flipflop.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        /*
        sample = SDL_LoadBMP("loop_dropped.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );
*/
 /*
        sample = SDL_LoadBMP("loop_noisy.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_reversed.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );

        sample = SDL_LoadBMP("loop_blurry.bmp");
        for (int i=3; i > 0; --i)
            zPipe.testSendFrame( sample );
*/
        SDL_FreeSurface(sample);
    }

    SECTION( "ZMQ REQ/REP quantized frame pipe" ) {
        ZMQVideoPipe zPipe;
        for(int i = 1e2; i > 0; --i)
            zPipe.testFramePassThru();
    }

}

#endif //SDL_CRT_FILTER_ZMQTESTS_HPP
