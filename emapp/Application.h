//
// Created by sebastian on 1/4/20.
//

#ifndef SDL_CRT_FILTER_APPLICATION_H
#define SDL_CRT_FILTER_APPLICATION_H
#include <cstring>
#include <emscripten/html5.h>
#include <emscripten/bind.h>

namespace e = emscripten;

class Application {
public:
   // Application() { Initialize(); SayHello()}
    void Initialize();
    void SayHello();
};


#endif //SDL_CRT_FILTER_APPLICATION_H
