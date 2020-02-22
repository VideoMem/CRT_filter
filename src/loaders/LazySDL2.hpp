#ifndef SDL_CRT_FILTER_LAZYSDL2_HPP
#define SDL_CRT_FILTER_LAZYSDL2_HPP

#include <Config.hpp>
#include <loaders/Loader.hpp>
#include <SDL2/SDL.h>


class LazyLoader: public Loader {
public:
    bool GetSurface(SDL_Surface* surface);
};

bool LazyLoader::GetSurface(SDL_Surface* surface) {
    SDL_Rect rect;
    SDL_Surface * gX;

    //Loads channel still image
    std::string imagePath = current().GetUri();
    //SDL_Log( "Loading channel [%d] -> %prngState", Pos(), imagePath.c_str() );
    gX = SDL_LoadBMP( imagePath.c_str() );

    if( gX == nullptr ) {
        SDL_Log( "Unable to load image %s! SDL Error: %s\n", imagePath.c_str(), SDL_GetError() );
        Up(); return GetSurface(surface);
    } else {
        SDL_GetClipRect( gX, &rect );
        //SDL_Log("Loaded image size: %dx%d", rect.w, rect.h);
        SDL_FillRect( surface, nullptr, 0x0);
        SDL_Rect dst;
        dst.x = 0;
        dst.y = 0;
        dst.w = Config::SCREEN_WIDTH;
        dst.h = Config::SCREEN_HEIGHT;
        //SDL_Log("Target image size: %dx%d", dst.w, dst.h);
        SDL_BlitScaled( gX, &rect, surface, &dst );
        SDL_FreeSurface( gX );
    }

    return gX != nullptr;
}

#endif //SDL_CRT_FILTER_LAZYSDL2_HPP
