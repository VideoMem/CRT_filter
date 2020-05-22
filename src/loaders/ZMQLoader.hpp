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
    SDL_Surface* cache_frame = nullptr;
    SDL_Surface* last_cache = nullptr;
    volatile bool readLock;
    volatile bool writeLock;
    volatile bool ready;
    zmq::context_t* context = nullptr;
    zmq::socket_t* socket = nullptr;
    volatile unsigned long rxFrameId;
    volatile unsigned long rxFrameCurrentId;

public:
    void pullFrame();
    ZMQLoader();
    ~ZMQLoader();
    //bool GetSurface(SDL_Surface* surface) { if(!readLock) surface = SDL_LoadBMP("encoded.bmp"); return true; }

    void pushCache(SDL_Surface* frame) { SurfacePixelsCopy( frame, cache_frame ); ready = true; }

    void GetRAWSurface(SDL_Surface* surface) {
        //while(readLock);
        writeLock = true;
        SurfacePixelsCopy(current_frame, surface);
        writeLock = false;
    }

    bool GetSurface(SDL_Surface* surface) {
        while(readLock);
        writeLock = true;
        blitFill(current_frame, surface);
        writeLock = false;
        return true;
    }
    bool frameEventRead() { return rxFrameId != rxFrameCurrentId; }
    void frameEventReset()  { rxFrameCurrentId = rxFrameId; }
    bool frameEvent() { if(frameEventRead()) { frameEventReset(); return true; } return false; }

    SDL_Surface *cacheFrame();
};

ZMQLoader::ZMQLoader() {
    readLock = false;
    writeLock = false;
    ready = false;
    rxFrameId = 0;
    rxFrameCurrentId = 0;
    current_frame = AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);
    cache_frame = AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);
    last_cache = AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);
    assert(current_frame != nullptr && cache_frame != nullptr && last_cache != nullptr && "Cannot allocate surface");
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
    while(writeLock);

    readLock = true;
    zmq::message_t request;
    socket->recv(&request);

    uint8_t* data = new uint8_t[request.size()];
    uint8_t* copy = new uint8_t[request.size()];
    memcpy((void *) data, request.data(), request.size());

   // SDL_Log("request() size: %ld", request.size() );
    //SDL_Log("received sha256: %s", Loader::sha256Log(data, request.size()).c_str());
    blank(current_frame);
    wave_to_surface(data, current_frame );
    surface_to_wave( current_frame, copy );
    //SDL_Log("copied sha256: %s", Loader::sha256Log(copy, request.size()).c_str());
//    assert(memcmp(data, copy, request.size()) == 0 && "Idempotent wave_to_frame(frame_to_wave) conversion allowed");
    ++rxFrameId;

    auto control_message = std::string("200");
    zmq::message_t reply( control_message.size() );
    memcpy ( reply.data(), control_message.c_str(), control_message.size() );
    socket->send(reply);
    readLock = false;
    delete [] copy;
    delete [] data;
}

SDL_Surface *ZMQLoader::cacheFrame() {
    while(!ready); //if new push
    //while(CompareSurface(last_cache, cache_frame)); //if new frame
    ready = false;
    SurfacePixelsCopy(cache_frame, last_cache);
    return cache_frame;
}

#endif //SDL_CRT_FILTER_ZMQLOADER_HPP
