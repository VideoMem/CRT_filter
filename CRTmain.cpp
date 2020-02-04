//Using SDL and standard IO
#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>
#include "CRTModel.hpp"

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;

bool init() {
	//Initialization flag
	bool success = true;
    //Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		success = false;
	} else {
		//Create window
		gWindow = SDL_CreateWindow( "SDL CRT Filter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if( gWindow == NULL ) {
			printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
			success = false;
		} else {
			//Get window surface
			gScreenSurface = SDL_GetWindowSurface( gWindow );
            SDL_SetSurfaceBlendMode( gScreenSurface, SDL_BLENDMODE_NONE );

        }
	}

	return success;
}

void close() {
    //Destroy window
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

int main( int argc, char* args[] ) {
    static CRTModel* crt = new CRTModel();

    //Main loop flag
    bool quit = false;

    //Event handler
    SDL_Event e;
    float ripple = 0.03;
    float noise = 0.4;
	//Start up SDL and create window
	if( !init() ) {
		printf( "Failed to initialize!\n" );
	} else {
        while(!quit) {

            crt->update(gScreenSurface);
            //Update the surface
            SDL_UpdateWindowSurface(gWindow);

            crt->setRipple(ripple);
            crt->setNoise(noise);

            while( SDL_PollEvent( &e ) != 0 ) {
                //User requests quit
                if (e.type == SDL_QUIT) {
                    quit = true;
                    crt->setRipple(0);
                    crt->setBlend(true);
                    crt->setHRipple(true);
                    crt->setVRipple(true);
                    crt->shutdown();
                    crt->setNoise(20);
                    while (crt->getSupply() > 0.11) {
                        crt->setSupply(crt->getSupply() / 1.2);
                        crt->update(gScreenSurface);
                        SDL_UpdateWindowSurface(gWindow);
                    }
                    //crt->setHRipple(true);
                    crt->noise(false);
                    crt->setSupply(0.001);
                    crt->setRipple(1);
                    int fanout = 10;
                    while(fanout > 0) {
                        crt->shutdown();
                        crt->update(gScreenSurface);
                        SDL_UpdateWindowSurface(gWindow);
                        --fanout;
                    }
                    crt->setNoise(0);
                    fanout = 50;
                    while(fanout > 0) {
                        crt->blank(gScreenSurface);
                        crt->fade(gScreenSurface);
                        SDL_UpdateWindowSurface(gWindow);
                        --fanout;
                    }
                    break;
                } else if( e.type == SDL_KEYDOWN ) {
                    switch (e.key.keysym.sym) {
                        case SDLK_UP:
                            ripple += 0.01;
                            if (ripple > 1) ripple = 1;
                            break;
                        case SDLK_DOWN:
                            ripple -= 0.01;
                            if (ripple < 0) ripple = 0;
                            break;
                        case SDLK_LEFT:
                            noise -= 0.1;
                            if (noise < 0) noise = 0;
                            break;
                        case SDLK_RIGHT:
                            if (noise < 1.5) noise += 0.1;
                            break;
                        case SDLK_z:
                            crt->resetFrameStats();
                            break;
                        case SDLK_1:
                            crt->noise(true);
                            break;
                        case SDLK_q:
                            crt->noise(false);
                            break;
                        case SDLK_2:
                            crt->setHRipple(true);
                            break;
                        case SDLK_w:
                            crt->setHRipple(false);
                            break;
                        case SDLK_3:
                            crt->setVRipple(true);
                            break;
                        case SDLK_e:
                            crt->setVRipple(false);
                            break;
                        case SDLK_4:
                            crt->setBlend(true);
                            break;
                        case SDLK_r:
                            crt->setBlend(false);
                            break;
                        case SDLK_PAGEUP:
                            crt->channelUp();
                            break;
                        case SDLK_PAGEDOWN:
                            crt->channelDw();
                            break;
                        default:
                            break;
                    }
                }
            }


        }
	}

	//Free resources and close SDL
	close();
    delete(crt);
	return 0;
}