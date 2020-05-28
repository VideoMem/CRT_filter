//
// Created by sebastian on 22/5/20.
//

#ifndef SDL_CRT_FILTER_TRANSCODEABLE_HPP
#define SDL_CRT_FILTER_TRANSCODEABLE_HPP

class Transcodeable {
public:
    static void encode( void*, void* ) { assert (false && "Function not implemented"); };
    static void decode( void*, void* ) { assert (false && "Function not implemented");};
    static size_t encode( void* , void* , size_t ) { assert (false && "Function not implemented"); return  -1; };
    static size_t decode( void* , void* , size_t ) { assert (false && "Function not implemented"); return  -1; };
    static size_t encoded_size( size_t , size_t ) { assert (false && "Function not implemented"); return  -1; };
    static size_t decoded_size( size_t , size_t ) { assert (false && "Function not implemented"); return  -1; };
};

#endif //SDL_CRT_FILTER_TRANSCODEABLE_HPP
