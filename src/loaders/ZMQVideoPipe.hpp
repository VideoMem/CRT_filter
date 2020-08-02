//
// Created by sebastian on 8/5/20.
//
#ifndef SDL_CRT_FILTER_ZMQVIDEOPIPE_H
#define SDL_CRT_FILTER_ZMQVIDEOPIPE_H
#include <zmq.hpp>
#include <pmt/pmt.h>
#include <loaders/LazySDL2.hpp>
#include <loaders/fmt_tools/WaveFile.hpp>
#include <submodules/lpc/lpclient/lpc.hpp>
#include <transcoders/TurboFEC.hpp>
#define MAX_RETRIES 3

//#define PIPE_DEBUG_FRAMES

static const int ZMQ_FRAME_SIZE = Config::NKERNEL_WIDTH * Config::NKERNEL_HEIGHT;
static const size_t ZMQ_MTU_WORD_BITS = DEFAULT_BITDEPTH;
static const size_t ZMQ_MTU_INPUT_BITS = TurboFEC::mtu_downquant( TurboFEC::bits ( ZMQ_FRAME_SIZE ), ZMQ_MTU_WORD_BITS ).input_bits;
static const size_t ZMQ_MTU_INPUT_BYTES = TurboFEC::bytes( ZMQ_MTU_INPUT_BITS );
static const size_t ZMQ_MTU_LINEAR_OUT_BITS = TurboFEC::linear_required_size( ZMQ_MTU_INPUT_BITS, ZMQ_MTU_WORD_BITS );
static const size_t ZMQ_MTU_LINEAR_OUT_BYTES = TurboFEC::bytes( ZMQ_MTU_LINEAR_OUT_BITS );

static const int ZMQ_COMPLEX_SIZE = 2 * ZMQ_FRAME_SIZE;

typedef float* internal_complex_t;
typedef std::deque<internal_complex_t> internal_complex_stack_t;

class ZMQVideoPipe: public Loader {
public:
    Worker* thread_client;
    WaveIO wave;
    SDL_Surface* temporary_frame;
    SDL_Surface* captured_frame;
    void init();
    size_t receive( float raw_stream[] );
    zmq::context_t* context = nullptr;
    zmq::socket_t* socket = nullptr;
    zmq::context_t* context_rep = nullptr;
    zmq::socket_t* socket_rep = nullptr;
    int retries = MAX_RETRIES;

    internal_complex_t internal;
    internal_complex_t internal_store;
    static inline float translate( float &a ) { float r = (a + 1) / 2; return r > 0? r: 0.0;  }
    static inline float untranslate( float &a ) { return (a * 2) - 1;  }

    static inline uint8_t quantize_am( float &a, float &b );
    static inline void unquantize_am( uint8_t &c, float &a, float &b );
    static inline uint8_t quantize( float &a, float &b );
    static inline void unquantize( uint8_t &c, float &a, float &b );

    static inline uint8_t quantize_amplitude( float &a, float &b );
    static inline void unquantize_amplitude( uint8_t &c, float &a, float &b );

    static void frame_to_float(SDL_Surface* surface, float arr[]);
    static void float_to_frame(float arr[], SDL_Surface* surface );
    void send( float* src, int size );
    void transferEvent();
#ifdef PIPE_DEBUG_FRAMES
    internal_complex_stack_t debug_frames;
#endif
    bool frameTransfer = false;
    size_t  byte_index = 0;
    size_t  copiedBytes = 0;
    void receiveFrame();
    static inline int asFloatIndex(int idx)  { return idx /  sizeof(float); }
    static inline int asByteIndex(int idx)  { return idx * sizeof(float); }
    static inline double angle( float a, float b );
    SDL_Surface* reference() { return captured_frame; }
public:
    void testSendFrame( SDL_Surface* surface );
    void testReceiveFrame() { while(!frameTransfer) receiveFrame(); frameTransfer = false; }
    void pushFrame();
    void testPassThru();
    void testPassThruQuant();
    void testFramePassThru();
    void testReceive() { receive( internal ); }
    ZMQVideoPipe();
    ~ZMQVideoPipe();
    bool GetSurface(SDL_Surface* surface) { SDL_BlitSurface(captured_frame, nullptr, surface, nullptr); return true; }
};


ZMQVideoPipe::~ZMQVideoPipe() {
    delete [] internal_store;
    delete [] internal;
    SDL_FreeSurface(captured_frame);
    SDL_FreeSurface( temporary_frame );
    socket_rep->close();
    socket->close();
    delete(socket);
    delete(context);
    delete(socket_rep);
    delete(context_rep);
    delete(thread_client);
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
    socket_rep = new zmq::socket_t(*context_rep, ZMQ_REP );
    thread_client = new Worker( Config::transport::int_frame_pusher );
    thread_client->setName("Internal thread communicator");
    try {
        socket->connect ( Config::transport::grc_source );
    } catch (zmq::error_t &e) {
        SDL_Log("Cannot Connect: %s", e.what());
    }

    try {
        socket_rep->bind ( Config::transport::grc_sink );
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


double ZMQVideoPipe::angle(float real, float imaginary) {
    int quadrant = 0;
    if(real >= 0 && imaginary >= 0)
        quadrant = 1;
    if(real < 0 && imaginary >= 0)
        quadrant = 2;
    if(real < 0 && imaginary < 0)
        quadrant = 3;
    if(real >= 0 && imaginary < 0)
        quadrant = 4;

    if (real == 0) real = 1e-6;
    assert(real != 0); double ratio = imaginary / real;
    double q1angle = atan( ratio ) + M_PI_2;
    double angle = 0;
    switch (quadrant) {
        case 1:
            angle = q1angle;
            break;
        case 2:
            angle = q1angle + M_PI;
            break;
        case 3:
            angle = q1angle + M_PI;
            break;
        case 4:
            angle = q1angle;
            break;
        default:
            assert(false && "This quadrant is unexpected");
            break;
    }

    if(angle < 0 && angle > M_PI * 2){
        SDL_Log("Angle range error real, imaginary: %f, %f, %f", angle, real, imaginary );
        assert(false);
    }
    return angle;
}

uint8_t ZMQVideoPipe::quantize_am( float &real, float &imaginary ) {
    //auto shift = (real + 1) / 2;
    //return (uint8_t) round(shift * 0xFF);
    return Pixelable::double_to_uint8(real);
}

void ZMQVideoPipe::unquantize_am( uint8_t &quant, float &real, float &imaginary ) {
    //float dequant = (float) quant / 0xFF;
    //real = (dequant * 2) - 1;
    //imaginary = 0;
    real = Pixelable::uint8_to_double(quant);
}

uint8_t ZMQVideoPipe::quantize(float &real, float &imaginary) {
    double theta = angle(real, imaginary);
    double theta_normalized = theta / (2 * M_PI);
    uint8_t quant  = round(theta_normalized * MAX_WHITE_LEVEL);
    return quant;
}

void ZMQVideoPipe::unquantize(uint8_t &quant, float &real, float &imaginary) {
    double theta_normalized = (double) quant  / MAX_WHITE_LEVEL ;
    double theta = theta_normalized * 2 * M_PI;
    double angle = theta;
    double norm  = 1; //sqrt(2);
    real = norm * cos(angle);
    imaginary = norm * sin(angle);
}

uint8_t ZMQVideoPipe::quantize_amplitude(float &a, float &b) {
    uint8_t msb[2] = {
            static_cast<uint8_t>((uint8_t)round(translate(a) * MAX_WHITE_LEVEL) & 0xF0),
            static_cast<uint8_t>((uint8_t)round(translate(b) * MAX_WHITE_LEVEL) & 0xF0)
    };
    //SDL_Log(" q: %d, %d", msb[0], msb[1] );
    char assy = msb[0] | (msb[1] >> 4);
    return assy;
}


void ZMQVideoPipe::unquantize_amplitude(uint8_t &c, float &a, float &b) {
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
#ifdef PIPE_DEBUG_FRAMES
    debug_frames.push_back(stackable);
#endif
    //SDL_SaveBMP(temporary_frame, "current.bmp");
    float_to_frame(internal_store, temporary_frame);
    SDL_BlitSurface(temporary_frame, nullptr, captured_frame, nullptr);
    frameTransfer = true;
}

void ZMQVideoPipe::receiveFrame() {
    size_t rx_size = receive( internal );

    size_t current_index = byte_index;
    byte_index += rx_size;

    size_t limit =  asByteIndex(ZMQ_COMPLEX_SIZE);
    if ( byte_index >= limit ) {
//        SDL_Log("Prepared to transfer frame at %d", byte_index);
        size_t remainder = byte_index -  limit;
        size_t toCopy_size = rx_size - remainder;

        memcpy(
                &internal_store[asFloatIndex(current_index)],
                internal,
                toCopy_size
                );

        transferEvent();

        memcpy( internal_store,
                &internal[asFloatIndex(toCopy_size)],
                 remainder );
//        SDL_Log("Copied %d, bytes to last buffer, %d to new one from a %d bytes total",
//            toCopy_size, remainder, (int) rx_size );
        assert((toCopy_size + remainder) == rx_size);

        copiedBytes += toCopy_size;
//        SDL_Log("Copied bytes, ZMQ_COMPLEX_SIZE, %d, %d",
//                copiedBytes, ZMQ_COMPLEX_SIZE );
        assert(copiedBytes  == limit );

        copiedBytes = remainder;
        byte_index = remainder;
    } else {
        memcpy(&internal_store[asFloatIndex(current_index)], internal, rx_size );
        copiedBytes += rx_size;
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
    send(front_frame, asByteIndex(ZMQ_COMPLEX_SIZE) );

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
    for (int i =0; i < 2; ++i) {
        SDL_Surface* inst = AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);
        received_frames.push_back(inst);
        testReceiveFrame();
        SDL_BlitSurface( captured_frame, nullptr, inst, nullptr );
    }

#ifdef PIPE_DEBUG_FRAMES
    //send packetized sequence of complex numbers
    for(auto & debug_frame : debug_frames) {
        send(debug_frame, ZMQ_COMPLEX_SIZE);
        delete [] debug_frame;
    }
    debug_frames.clear();
#endif

    //send packetized sequence of images
    for(auto & received_frame : received_frames) {
        testSendFrame( received_frame );
        SDL_FreeSurface( received_frame );
    }
    received_frames.clear();

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
            unquantize_am(quant, a, b);
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
            Uint32 value = quantize_am(arr[pos], arr[pos + 1]);
            toPixel(&pixel, &value, &value, &value);
            put_pixel32(surface, x, y, pixel);
            pos+=2;
        }
    }
}


void ZMQVideoPipe::pushFrame() {
    testReceiveFrame();
    size_t size = captured_frame->w * captured_frame->h;
    auto stream = new char[size];
    surface_to_wave(captured_frame, (uint8_t *) stream );
    thread_client->sendTX(*stream, size);
    delete[] stream;
}

#endif //SDL_CRT_FILTER_ZMQVIDEOPIPE_H
