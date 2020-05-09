//
// Created by sebastian on 8/5/20.
//

#ifndef SDL_CRT_FILTER_ZMQTESTS_HPP
#define SDL_CRT_FILTER_ZMQTESTS_HPP
#include <loaders/ZMQVideoPipe.hpp>

TEST_CASE( "ZMQ API", "[ZMQ][SDL2][GNURadio]") {

    SECTION( "ZMQ REQ Source" ) {
        Config cfg;
        ZMQVideoPipe zPipe;
        SDL_Surface* sample = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
        //cfg.initResources( &zPipe );
        zPipe.testReceiveFrame();
        zPipe.GetSurface(sample);
        SDL_SaveBMP(sample, "encoded.bmp");
        SDL_FreeSurface(sample);
    }

    SECTION( "ZMQ REP Sink" ) {
        ZMQVideoPipe zPipe;
        SDL_Surface* sample = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
        sample = SDL_LoadBMP("encoded.bmp");
        zPipe.testSendFrame( sample );
        SDL_FreeSurface(sample);
    }
}

#endif //SDL_CRT_FILTER_ZMQTESTS_HPP
