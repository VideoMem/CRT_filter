//
// Created by sebastian on 8/5/20.
//

#ifndef SDL_CRT_FILTER_ZMQVIDEOPIPE_H
#define SDL_CRT_FILTER_ZMQVIDEOPIPE_H
#include <zmq.hpp>
#include <pmt/pmt.h>
#include <loaders/LazySDL2.hpp>
#define MAX_RETRIES 3
#define MAX_WHITE_LEVEL 200

class ZMQVideoPipe: public Loader {
protected:
    SDL_Surface* temporary_frame;
    SDL_Surface* captured_frame;
    void init();
    size_t receive( float raw_stream[] );
    zmq::context_t* context = nullptr;
    zmq::socket_t* socket = nullptr;
    zmq::context_t* context_rep = nullptr;
    zmq::socket_t* socket_rep = nullptr;
    int retries = MAX_RETRIES;
    float nullresponse[4] = { 0x00 };
    //float internal[4] = { 0 };
    float internal[ Config::SCREEN_WIDTH * Config::SCREEN_HEIGHT ] = { 0 };
    static inline float translate( float &a ) { float r = (a + 1) / 2; return r > 0? r: 0.0;  }
    static inline float untranslate( float &a ) { return (a * 2) - 1;  }
    static inline uint8_t quantize( float &a, float &b );
    static inline char quantizePolar( float &a, float &b );
    static inline void unquantize( uint8_t &c, float &a, float &b );
    bool frameTransfer = false;
    long pxIndex = 0;
    void receiveFrame();
    std::string string_to_hex(const std::string& input);

public:
    void testSendFrame( SDL_Surface* surface );
    void testReceiveFrame() { while(!frameTransfer) receiveFrame(); frameTransfer = false; }
    void testPassThru();
    void testReceive() { receive( internal ); }
    ZMQVideoPipe();
    ~ZMQVideoPipe();
    bool GetSurface(SDL_Surface* surface) { SDL_BlitSurface(captured_frame, nullptr, surface, nullptr); return true; }
};

std::string ZMQVideoPipe::string_to_hex(const std::string& input) {
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input)
    {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}


ZMQVideoPipe::ZMQVideoPipe() {
    init();
    captured_frame =
            AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    temporary_frame =
            AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );

    if(captured_frame == nullptr || temporary_frame == nullptr )
        SDL_Log("Cannot allocate internal frames");
    else {
        blank(captured_frame);
        blank(temporary_frame);
    }
}

void ZMQVideoPipe::init() {
    //  Prepare our context and socket
    if(socket != nullptr) { delete(socket); socket = nullptr; }
    if(context != nullptr) { delete(context); context = nullptr; }

    context = new zmq::context_t(1);
    socket = new zmq::socket_t( *context, ZMQ_REQ );
    context_rep = new zmq::context_t(1);
    socket_rep = new zmq::socket_t(*context_rep, ZMQ_REP);

    try {
        socket->connect ("tcp://localhost:5656");
    } catch (zmq::error_t &e) {
        SDL_Log("Cannot Connect: %s", e.what());
    }

    try {
        socket_rep->bind ("tcp://0.0.0.0:5555");
    } catch (zmq::error_t &e) {
        SDL_Log("Cannot Bind: %s", e.what());
    }

}

size_t ZMQVideoPipe::receive( float raw_stream[] ) {
    zmq::message_t request;
    std::string query( { 0x00, 0x10, 0x00, 0x00 } );

    try {
        socket->send( query.c_str(), query.size(), 0 );
        socket->recv(&request);
        //SDL_Log("ReplySize %d", request.size() );
        auto data = static_cast<float*>(request.data());
/*
        SDL_Log("Received [0] : [%f, %f]",
                data[0],
                data[1]
        );

        SDL_Log("Received ( min, max ) : [%f, %f]",
               *std::min_element(data, data + sizeof(data)),
               *std::max_element(data, data + sizeof(data))
        );

        uint8_t quantized = quantize(data[0], data[1]);
        float a, b;
        unquantize(quantized, a , b);
        SDL_Log("Quantized [0] : [%d]",
                quantized
        ); */

        retries = MAX_RETRIES;
        memcpy(raw_stream, data, sizeof(data));
        return sizeof(data);
    } catch (zmq::error_t &e) {
        SDL_Log("Cannot Receive: %s", e.what());
        init();
        if (retries > 0) {
            --retries;
            receive( internal );
        }
        memcpy(raw_stream, nullresponse, sizeof(nullresponse));
        return sizeof(nullresponse);
    }
}

ZMQVideoPipe::~ZMQVideoPipe() {
    SDL_FreeSurface(captured_frame);
    SDL_FreeSurface( temporary_frame );
    delete(socket);
    delete(context);
    delete(socket_rep);
    delete(context_rep);
}

char ZMQVideoPipe::quantizePolar(float &a, float &b) {
    double intensity = sqrt( pow( a, 2) + pow ( b, 2 ) ) / sqrt(2);
    double angle = atan(a / b ) /  M_PI ;
    SDL_Log(" q: %f, %f", intensity, angle );
    char msb = round(intensity * MAX_WHITE_LEVEL );
    char lsb = round(angle * MAX_WHITE_LEVEL ) / 16;
    char assy = (msb & 0xF0) | (lsb & 0x0F);
    return assy;
}

uint8_t ZMQVideoPipe::quantize(float &a, float &b) {
    uint8_t msb[2] = {
            static_cast<uint8_t>((uint8_t)round(translate(a) * MAX_WHITE_LEVEL) & 0xF0),
            static_cast<uint8_t>((uint8_t)round(translate(b) * MAX_WHITE_LEVEL) & 0xF0)
    };
    //SDL_Log(" q: %d, %d", msb[0], msb[1] );
    char assy = msb[0] | (msb[1] >> 4);
    return assy;
}


void ZMQVideoPipe::unquantize(uint8_t &c, float &a, float &b) {
    uint8_t msb[2] = {
            static_cast<uint8_t>( c & 0xF0),
            static_cast<uint8_t>( c << 4 )
    };
    float p = (float) msb[0] / MAX_WHITE_LEVEL;
    float q = (float) msb[1] / MAX_WHITE_LEVEL;
    a = untranslate(p);
    b = untranslate(q);
    //SDL_Log("uq: %d, %d", msb[0], msb[1] );
    //SDL_Log("UnQuantized: %f, %f", a, b );
}

void ZMQVideoPipe::receiveFrame() {
    size_t rx_size = receive( internal );
    int offset = 0;
    for(size_t i =0; i < rx_size; i+=2 ) {
        offset = i / 2;
        Uint32 value = quantize(internal[i], internal[i + 1]);
        Uint32 pixel;
        toPixel(&pixel, &value, &value, &value);
        long newIndex = pxIndex + offset;
        int x = static_cast<int>(newIndex) % Config::NKERNEL_WIDTH;
        int y = static_cast<int>(newIndex) / Config::NKERNEL_WIDTH;
        if( y < Config::NKERNEL_HEIGHT ) {
            put_pixel32(temporary_frame, x, y, pixel );
        } else {
            SDL_BlitSurface(temporary_frame, nullptr, captured_frame, nullptr);
            blank(temporary_frame);
            //SurfacePixelsCopy(temporary_frame, captured_frame );
            pxIndex = 0;
            frameTransfer = true;
        }
    }
    pxIndex += offset;
}

void ZMQVideoPipe::testSendFrame(SDL_Surface *surface) {
    const int bsize = 2 * Config::NKERNEL_WIDTH * Config::NKERNEL_HEIGHT;
    float front_frame[ bsize ];
    int pos  = 0;
    for(int  y = 0; y < Config::NKERNEL_HEIGHT; ++y )
        for( int x = 0; x < Config::NKERNEL_WIDTH; x++ ) {
            Uint32 pixel = get_pixel32( surface, x, y );
            float a, b;
            Uint32 value[3]; comp(&pixel, &value[0], &value[1], &value[2]); double media = value[0] + value[1] + value[2]; media /=3;
            uint8_t quant = (uint8_t) media;
            unquantize(quant, a, b);
            front_frame[ pos ] = a;
            front_frame[ pos + 1 ] = b;
            pos +=2;
        }

    //  ZMQ part
    zmq::message_t request;
    socket_rep->recv(&request);
    zmq::message_t reply( sizeof(front_frame) );
    memcpy (reply.data (), front_frame, sizeof(front_frame)  );

    socket_rep->send (reply);

}

void ZMQVideoPipe::testPassThru() {
    zmq::message_t request;
    zmq::message_t request_rep;
    std::string query( { 0x00, 0x10, 0x00, 0x00 } );

    try {
        socket_rep->recv(&request_rep);
        socket->send( request_rep.data(), request_rep.size(), 0 );
        socket->recv(&request);
        socket_rep->send (request );
    } catch (zmq::error_t &e) {
        SDL_Log("Cannot Tx: %s", e.what());
    }
}


#endif //SDL_CRT_FILTER_ZMQVIDEOPIPE_H
