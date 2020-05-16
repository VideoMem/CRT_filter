//
// Created by sebastian on 14/5/20.
//

#ifndef SDL_CRT_FILTER_ZMQLOADER_HPP
#define SDL_CRT_FILTER_ZMQLOADER_HPP
#include <loaders/MagickLoader.hpp>
#include <submodules/lpc/lpclient/lpc.hpp>
#include <loaders/fmt_tools/WaveFile.hpp>
#include <picosha2.h>

class ZMQLoader: public MagickLoader {
public:
    WaveIO wave;
    SDL_Surface* current_frame = nullptr;
    bool readLock = false;
    zmq::context_t* context = nullptr;
    zmq::socket_t* socket = nullptr;

public:
    void pullFrame();
    ZMQLoader();
    ~ZMQLoader();
    //bool GetSurface(SDL_Surface* surface) { if(!readLock) surface = SDL_LoadBMP("encoded.bmp"); return true; }

    bool GetSurface(SDL_Surface* surface) {
        while(!readLock);
        SDL_BlitSurface(current_frame, nullptr, surface, nullptr);
        return true;
    }

};

ZMQLoader::ZMQLoader() {
    current_frame = AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);
    assert(current_frame != nullptr && "Cannot allocate surface");
    context = new zmq::context_t(1);
    socket = new zmq::socket_t(*context, ZMQ_REP);
    socket->bind ("tcp://0.0.0.0:5133" );
}

ZMQLoader::~ZMQLoader() {
    SDL_FreeSurface(current_frame);
    socket->close();
    delete socket;
    delete context;
}

void ZMQLoader::pullFrame()  {
    readLock = true;
    zmq::message_t request;
    socket->recv(&request);

    uint8_t* data = new uint8_t[request.size()];
    uint8_t* copy = new uint8_t[request.size()];
    memcpy((void *) data, request.data(), request.size());
    SDL_Log("request() size: %ld", request.size() );

    SDL_Log("received sha256: %s", Loader::sha256Log(data, request.size()).c_str());
    blank(current_frame);
    wave_to_surface(data, current_frame, 1 );
    SDL_SaveBMP( current_frame, "pull_frame.bmp" );
    surface_to_wave( current_frame, copy );
    SDL_Log("copied sha256: %s", Loader::sha256Log(copy, request.size()).c_str());
    //    assert(memcmp(data, copy, request.size()) == 0 && "Non idempotent wave_to_frame(frame_to_wave) conversion allowed");
    //SDL_Log("Recived data: %s", request.data());
    auto control_message = std::string("200");
    zmq::message_t reply( control_message.size() );
    memcpy ( reply.data(), control_message.c_str(), control_message.size() );
    socket->send(reply);

    readLock = false;
    delete [] copy;
    delete [] data;
}


#endif //SDL_CRT_FILTER_ZMQLOADER_HPP
