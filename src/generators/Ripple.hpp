//
// Created by sebastian on 27/2/20.
//

#ifndef SDL_CRT_FILTER_RIPPLE_HPP
#define SDL_CRT_FILTER_RIPPLE_HPP

#include <generators/Generator.hpp>
#include <Config.hpp>
#include <math.h>

class Ripple: public Generator {

public:
    static float get( int& sync, float& ripple, float& lastR ) {
        int line = sync % Config::SCREEN_HEIGHT;
        float screenpos = (float) line / Config::SCREEN_HEIGHT;
        float vt = abs(sin(M_PI *  screenpos));
        float ret = vt > lastR? vt: lastR;
        float correct = 1 - (10e-4 * ripple);
        lastR = ret * correct;
        return ret ;
    }

    float get(int& sync, float& ripple) {
        return get( sync, ripple, hold_state );
    }

protected:
    float hold_state = 0;
};

#endif //SDL_CRT_FILTER_RIPPLE_HPP
