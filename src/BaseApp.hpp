#ifndef SDL_CRT_FILTER_BASEAPP_HPP
#define SDL_CRT_FILTER_BASEAPP_HPP
#include <loaders/LazySDL2.hpp>
#include <Config.hpp>

class BaseApp {
public:
    void SDL2_Init() {
        if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
            SDL_Log( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        } else {
            //Create window
            gWindow = SDL_CreateWindow( "SDL CRT Filter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        cfg.SCREEN_WIDTH  > cfg.TARGET_WIDTH ? cfg.SCREEN_WIDTH : cfg.TARGET_WIDTH,
                                        cfg.SCREEN_HEIGHT > cfg.TARGET_HEIGHT? cfg.SCREEN_HEIGHT: cfg.TARGET_HEIGHT,
                                        SDL_WINDOW_BORDERLESS);
            if( gWindow == nullptr ) {
                SDL_Log( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
            } else {
                //Get window surface
                gScreenSurface = SDL_GetWindowSurface( gWindow );
                SDL_SetSurfaceBlendMode( gScreenSurface, SDL_BLENDMODE_NONE );
            }
        }
    }
    BaseApp(Loader& l) {
        loader = &l;
        cfg.fillImages(*loader);
        SDL2_Init();
        fallbackImage = loader->AllocateSurface(cfg.SCREEN_WIDTH, cfg.SCREEN_HEIGHT);
    }

    ~BaseApp() {
        //Destroy window
        SDL_DestroyWindow( gWindow );
        SDL_FreeSurface( fallbackImage );
        gWindow = nullptr;
        fallbackImage = nullptr;
        gScreenSurface = nullptr;
        SDL_Quit();
    }

    void Up() { loader->Up(); loader->GetSurface(fallbackImage); }
    void Dw() { loader->Dw(); loader->GetSurface(fallbackImage); }
    void Standby() {
        loader->GetSurface(fallbackImage);
        SDL_Rect rect, src;
        SDL_GetClipRect(fallbackImage  , &src  );
        SDL_GetClipRect(gScreenSurface , &rect );
        //rect = Loader::BiggestSurfaceClipRect(fallbackImage, gScreenSurface );
        SDL_BlitScaled(fallbackImage, &src, gScreenSurface, &rect);
        SDL_UpdateWindowSurface(gWindow);
    }

protected:
    Loader* loader;
    SDL_Surface* fallbackImage = nullptr;
    SDL_Window*  gWindow = nullptr;
    SDL_Surface* gScreenSurface = nullptr;
    Config cfg;

};


#endif //SDL_CRT_FILTER_BASEAPP_HPP
