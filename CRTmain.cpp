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
		gWindow = SDL_CreateWindow( "SDL CRT Filter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		        SCREEN_WIDTH  > TARGET_WIDTH ? SCREEN_WIDTH : TARGET_WIDTH,
		        SCREEN_HEIGHT > TARGET_HEIGHT? SCREEN_HEIGHT: TARGET_HEIGHT,
		        SDL_WINDOW_OPENGL);
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

static void powerOff(CRTModel* crt, SDL_Surface* gScreenSurface) {
    //crt->setBlend(true);
    crt->setHRipple(true);
    crt->setVRipple(true);
    crt->noise(true);
    crt->setRipple(1);
    while (crt->getSupply() > 0.01) {
        crt->setSupply(crt->getSupply() / 1.3);
        crt->shutdown();
        crt->update(gScreenSurface);
        SDL_UpdateWindowSurface(gWindow);

    }

    //int fanout = 10;

    //while (fanout > 0) {

      //  SDL_UpdateWindowSurface(gWindow);
       // --fanout;
    //}

    crt->noise(false);

    float scale =1;
    while (scale > 0.9) {
        crt->strech(gScreenSurface, scale);
        crt->focusNoise(gScreenSurface);
        crt->dot(gScreenSurface);
        crt->fade(gScreenSurface);
        SDL_UpdateWindowSurface(gWindow);
        scale /= 1.2;
    }
    int fanout = 200;
    while(fanout > 0) {
        crt->strech(gScreenSurface, scale);
        crt->fade(gScreenSurface);
        SDL_UpdateWindowSurface(gWindow);
        --fanout;
    }
}


int main( int argc, char* args[] ) {
    static CRTModel* crt = new CRTModel();

    //Main loop flag
    bool quit = false;

    //Event handler
    SDL_Event e;
    float ripple = 0.03;
    float noise = 0.4;
    float brightness = 1;
    float contrast = 1;
    float color = 1;
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
            crt->setContrast(contrast);
            crt->setBrightness(brightness);
            crt->setColor(color);

            while( SDL_PollEvent( &e ) != 0 ) {
                //User requests quit
                if (e.type == SDL_QUIT) {
                    quit = true;
                    powerOff(crt, gScreenSurface);
                    break;
                } else if( e.type == SDL_KEYDOWN ) {
                    switch (e.key.keysym.sym) {
                        case SDLK_5:
                            contrast += 0.01;
                            if(contrast > 2) contrast = 2;
                            break;
                        case SDLK_t:
                            contrast -= 0.01;
                            if(contrast < 0) contrast = 0;
                            break;
                        case SDLK_6:
                            brightness += 0.01;
                            if(brightness > 1) brightness = 1;
                            break;
                        case SDLK_y:
                            brightness -= 0.01;
                            if(brightness < 0) brightness = 0;
                            break;
                        case SDLK_7:
                            color += 0.01;
                            if (color > 2) color = 2;
                            break;
                        case SDLK_u:
                            color -= 0.01;
                            if(color < 0) color = 0;
                            break;
                        case SDLK_UP:
                            ripple += 0.01;
                            if (ripple > 1) ripple = 1;
                            break;
                        case SDLK_DOWN:
                            ripple -= 0.01;
                            if (ripple < 0) ripple = 0;
                            break;
                        case SDLK_LEFT:
                            noise -= 0.01;
                            if (noise < 0) noise = 0;
                            break;
                        case SDLK_RIGHT:
                            if (noise < 1.5) noise += 0.01;
                            break;
                        case SDLK_z:
                            crt->resetFrameStats();
                            brightness = 1; contrast = 1; color = 1;
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
                        case SDLK_END:
                            powerOff(crt, gScreenSurface);
                            crt->noise(true);
                            crt->setBlend(true);
                            crt->setHRipple(true);
                            crt->setVRipple(true);
                            crt->setSupply(1);
                            crt->channelDw();
                            crt->channelUp();
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