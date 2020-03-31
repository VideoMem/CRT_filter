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
    static double get( int& sync, double& ripple, double& lastR ) {
        int line = sync % Config::SCREEN_HEIGHT;
        double screenpos = (double) line / Config::SCREEN_HEIGHT;
        double vt = abs(sin(M_PI *  screenpos));
        double ret = vt > lastR? vt: lastR;
        double correct = 1 - (10e-4 * ripple);
        lastR = ret * correct;
        return ret ;
    }

    double get(int& sync, double& ripple) {
        return get( sync, ripple, hold_state );
    }

protected:
    double hold_state = 0;
};

#endif //SDL_CRT_FILTER_RIPPLE_HPP
