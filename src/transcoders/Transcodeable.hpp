//
// Created by sebastian on 22/5/20.
//

#ifndef SDL_CRT_FILTER_TRANSCODEABLE_HPP
#define SDL_CRT_FILTER_TRANSCODEABLE_HPP

class Transcodeable {
public:
    static void encode( void* dst, void* src ) { assert (false && "Function not implemented"); };
    static void decode( void* dst, void* src ) { assert (false && "Function not implemented");};
    static size_t encode( void* dst, void* src, size_t in_size ) { assert (false && "Function not implemented"); return  -1; };
    static size_t decode( void* dst, void* src, size_t in_size ) { assert (false && "Function not implemented"); return  -1; };
    static size_t encoded_size( size_t in_size, size_t out_size ) { assert (false && "Function not implemented"); return  -1; };
    static size_t decoded_size( size_t out_size, size_t in_size ) { assert (false && "Function not implemented"); return  -1; };
};

#endif //SDL_CRT_FILTER_TRANSCODEABLE_HPP
