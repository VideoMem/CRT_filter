//
// Created by sebastian on 1/4/20.
//

#ifndef SDL_CRT_FILTER_APPLICATION_H
#define SDL_CRT_FILTER_APPLICATION_H

#include <emscripten/html5.h>
#include <emscripten/bind.h>

namespace e = emscripten;

class Application {
public:
    void Initialize();
    void SayHello();
};

EMSCRIPTEN_BINDINGS(EMTest) {
        e::class_<Application>("Application")
                .constructor()
                .function("Initialize", &Application::Initialize)
                .function("SayHello", &Application::SayHello);
}

#endif //SDL_CRT_FILTER_APPLICATION_H
