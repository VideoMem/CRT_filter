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
static const int ZMQ_FRAME_SIZE = Config::NKERNEL_WIDTH * Config::NKERNEL_HEIGHT;
static const int ZMQ_COMPLEX_SIZE = 2 * ZMQ_FRAME_SIZE;

typedef float* internal_complex_t;
typedef std::deque<internal_complex_t> internal_complex_stack_t;

class ZMQVideoPipe: public Loader {
public:
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
    internal_complex_t internal;
    internal_complex_t internal_store;
    static inline float translate( float &a ) { float r = (a + 1) / 2; return r > 0? r: 0.0;  }
    static inline float untranslate( float &a ) { return (a * 2) - 1;  }
    static inline uint8_t quantize( float &a, float &b );
    static inline char quantizePolar( float &a, float &b );
    static inline void unquantize( uint8_t &c, float &a, float &b );
    static void frame_to_float(SDL_Surface* surface, float arr[]);
    static void float_to_frame(float arr[], SDL_Surface* surface );
    void send( float* src, int size );
    void transferEvent();

    internal_complex_stack_t debug_frames;
    bool frameTransfer = false;
    int  byte_index = 0;
    int  copiedBytes = 0;
    void receiveFrame();
    std::string string_to_hex(const std::string& input);
    static inline int asFloatIndex(int idx)  { return idx /  sizeof(float); }

public:
    void testSendFrame( SDL_Surface* surface );
    void testReceiveFrame() { while(!frameTransfer) receiveFrame(); frameTransfer = false; }
    void testPassThru();
    void testPassThruQuant();
    void testFramePassThru();
    void testReceive() { receive( internal ); }
    ZMQVideoPipe();
    ~ZMQVideoPipe();
    bool GetSurface(SDL_Surface* surface) { SDL_BlitSurface(captured_frame, nullptr, surface, nullptr); return true; }
};

std::string ZMQVideoPipe::string_to_hex(const std::string& input) {
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input) {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}


ZMQVideoPipe::~ZMQVideoPipe() {
    delete [] internal_store;
    delete [] internal;
    SDL_FreeSurface(captured_frame);
    SDL_FreeSurface( temporary_frame );
    delete(socket);
    delete(context);
    delete(socket_rep);
    delete(context_rep);
}

ZMQVideoPipe::ZMQVideoPipe() {
    init();
    captured_frame =
            AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    temporary_frame =
            AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    internal = new float[ZMQ_COMPLEX_SIZE];
    internal_store = new float[ZMQ_COMPLEX_SIZE];

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

size_t ZMQVideoPipe::receive( float* raw_stream ) {
    zmq::message_t request;
    std::string query( { 0x00, 0x10, 0x00, 0x00 } );

    try {
        socket->send( query.c_str(), query.size(), 0 );
        socket->recv(&request);
        auto data = static_cast<float*>(request.data());
        retries = MAX_RETRIES;
        memcpy(raw_stream, data, request.size());
        return request.size();
    } catch (zmq::error_t &e) {
        SDL_Log("Cannot Receive: %s", e.what());
        init();
        if (retries > 0) {
            --retries;
            receive( internal );
        }
        return -1;
    }
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

void ZMQVideoPipe::transferEvent() {
    internal_complex_t stackable = new float[ZMQ_COMPLEX_SIZE];
    memcpy(stackable, internal_store, ZMQ_COMPLEX_SIZE);
    debug_frames.push_back(stackable);

    float_to_frame(internal_store, temporary_frame);
    SDL_BlitSurface(temporary_frame, nullptr, captured_frame, nullptr);
    frameTransfer = true;
}

void ZMQVideoPipe::receiveFrame() {
    size_t rx_size = receive( internal );
   // send( internal, rx_size );

    int current_index = byte_index;
    byte_index += rx_size;

    if ( byte_index >= ZMQ_COMPLEX_SIZE ) {
        SDL_Log("Prepared to transfer frame at %d", byte_index);
        int remainder = byte_index -  ZMQ_COMPLEX_SIZE;
        int toCopy_size = rx_size - remainder;

        memcpy(
                &internal_store[asFloatIndex(current_index)],
                internal,
                toCopy_size
                );

        transferEvent();

        memcpy( internal_store,
                &internal[asFloatIndex(toCopy_size)],
                 remainder );
        SDL_Log("Copied %d, bytes to last buffer, %d to new one from a %d bytes total",
            toCopy_size, remainder, (int) rx_size );
        assert((toCopy_size + remainder) == (int) rx_size);

        copiedBytes += toCopy_size;
        SDL_Log("Copied bytes, ZMQ_COMPLEX_SIZE, %d, %d",
                copiedBytes, ZMQ_COMPLEX_SIZE );
        assert(copiedBytes  == ZMQ_COMPLEX_SIZE);

        copiedBytes = remainder;
        byte_index = remainder;
    } else {
        memcpy(&internal_store[asFloatIndex(current_index)], internal, rx_size );
        copiedBytes += rx_size;
//        byte_index += rx_size;
    }
}

void ZMQVideoPipe::send( float *src, int size ) {
    //  ZMQ part
    zmq::message_t request;
    socket_rep->recv(&request);
    zmq::message_t reply( size );
    memcpy ( reply.data(), src, size );
    socket_rep->send (reply);

}

void ZMQVideoPipe::testSendFrame(SDL_Surface *surface) {
    auto* front_frame = new float[ ZMQ_COMPLEX_SIZE ];
    frame_to_float( surface, front_frame );
    send(front_frame, ZMQ_COMPLEX_SIZE );

    delete [] front_frame;
}


void ZMQVideoPipe::testPassThru() {
    try {
        size_t rx_size = receive( internal );
        send( internal, rx_size );
    } catch (zmq::error_t &e) {
        SDL_Log("Cannot Tx: %s", e.what());
    }
}

void ZMQVideoPipe::testPassThruQuant() {
    zmq::message_t request;
    zmq::message_t request_rep;
    std::string query( { 0x00, 0x10, 0x00, 0x00 } );
    try {
        socket->send(query.c_str(), query.size(), 0);
        socket->recv(&request);
        auto data = static_cast<float *>(request.data());
        long copy_size = request.size() / sizeof(float);
        auto *copy = new float[copy_size];
        for (int i = 0; i < copy_size ; i += 2) {
            float a, b;
            uint8_t  q = quantize(data[i], data[i+1]);
            unquantize(q, a, b);
            copy[i] = a;
            copy[i + 1] = b;
        }
        send( copy, request.size() );
        delete [] copy;
    } catch (zmq::error_t &e) {
        SDL_Log("Cannot Tx: %s", e.what());
    }
}

void ZMQVideoPipe::testFramePassThru() {
    std::deque<SDL_Surface*> received_frames;
    //stores  frames
    for (int i =0; i < 10; ++i) {
        SDL_Surface* inst = AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);
        received_frames.push_back(inst);
        testReceiveFrame();
        SDL_BlitSurface( captured_frame, nullptr, inst, nullptr );
    }

    //send packetized sequence of complex numbers
    for(auto & debug_frame : debug_frames) {
        send(debug_frame, ZMQ_COMPLEX_SIZE);
        delete [] debug_frame;
    }
    debug_frames.clear();
/*
    //send packetized sequence of images
    for(auto & received_frame : received_frames) {
        testSendFrame( received_frame );
        SDL_FreeSurface( received_frame );
    }
    received_frames.clear();
*/
}


void ZMQVideoPipe::frame_to_float(SDL_Surface *surface, float *arr) {
    int pos = 0;
    for( int  y = 0; y < Config::NKERNEL_HEIGHT; ++y )
        for( int x = 0; x < Config::NKERNEL_WIDTH; x++ ) {
            Uint32 pixel = get_pixel32( surface, x, y );
            Uint32 value[3]; comp(&pixel, &value[0], &value[1], &value[2]);

            double media = value[0] + value[1] + value[2]; media /=3;
            uint8_t quant = (uint8_t) media;

            float a, b;
            unquantize(quant, a, b);
            arr[ pos ] = a;
            arr[ pos + 1 ] = b;
            pos +=2;
        }
}

void ZMQVideoPipe::float_to_frame(float *arr, SDL_Surface *surface ) {
    Uint32 pixel = 0;
    int pos = 0;
    for( int  y = 0; y < Config::NKERNEL_HEIGHT; ++y ) {
        for( int x = 0; x < Config::NKERNEL_WIDTH; ++x ) {
            Uint32 value = quantize(arr[pos], arr[pos + 1]);
            toPixel(&pixel, &value, &value, &value);
            put_pixel32(surface, x, y, pixel);
            pos+=2;
        }
    }
}


#endif //SDL_CRT_FILTER_ZMQVIDEOPIPE_H
