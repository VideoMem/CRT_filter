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
    float ripple = 0.1;
    float noise = 0.2;
	//Start up SDL and create window
	if( !init() ) {
		printf( "Failed to initialize!\n" );
	} else {
        while(!quit) {
            while( SDL_PollEvent( &e ) != 0 ) {
                //User requests quit
                if (e.type == SDL_QUIT) {
                    quit = true;
                } else if( e.type == SDL_KEYDOWN ) {
                    switch (e.key.keysym.sym) {
                        case SDLK_UP:
                            if (ripple < 10.0) ripple += 0.1;
                            break;
                        case SDLK_DOWN:
                            if (ripple > 0) ripple -= 0.1;
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
                        default:
                            break;
                    }
                }
            }

            crt->update(gScreenSurface);
            //Update the surface
            SDL_UpdateWindowSurface(gWindow);

            crt->setRipple(ripple);
            crt->setNoise(noise);

        }
	}

	//Free resources and close SDL
	close();
    delete(crt);
	return 0;
}