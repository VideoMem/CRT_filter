/*This source code copyrighted by Lazy Foo' Productions (2004-2020)
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

void setPixel();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;
	
//The surface contained by the window
SDL_Surface* gScreenSurface = NULL;

//The image we will load and show on the screen
SDL_Surface* gHelloWorld = NULL;

SDL_Surface* gBuffer = NULL;
SDL_Surface* gBlank = NULL;
SDL_Surface* gBack = NULL;

bool createBuffer() {
    Uint32 rmask, gmask, bmask, amask;

    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    gBuffer = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                   rmask, gmask, bmask, amask);
    gBlank = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                   rmask, gmask, bmask, amask);
    gBack  = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                  rmask, gmask, bmask, amask);

    if (gBuffer == NULL || gBlank == NULL || gBack == NULL) {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

Uint32 get_pixel32( SDL_Surface *surface, int x, int y )
{
    //Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;

    //Get the requested pixel
    return pixels[ ( y * surface->w ) + x ];
}

void put_pixel32( SDL_Surface *surface, int x, int y, Uint32 pixel )
{
    //Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;

    //Set the pixel
    pixels[ ( y * surface->w ) + x ] = pixel;
}

void testInvert( SDL_Surface *surface ) {
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            put_pixel32(surface, x, y,
                        (0x00FFFFFF - (get_pixel32(surface, x, y) & 0x00FFFFFF)) | 0xFF000000);
        }
}

void testDesaturate( SDL_Surface *surface, SDL_Surface *dest ) {
    SDL_FillRect(dest, NULL, 0x000000);
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            int pixel = get_pixel32(surface, x, y);
            int R = (pixel & 0xFF0000) >> 16;
            int G = (pixel & 0xFF00) >> 8;
            int B = pixel & 0xFF;
            int luma = round((R + G + B) /3);
            int npx = ((luma << 16) + (luma << 8) + luma) | 0xFF000000;
            put_pixel32(dest, x, y,
                        npx);
        }
}

void testNoise( SDL_Surface *surface, SDL_Surface *dest  ) {
    SDL_FillRect(dest, NULL, 0x000000);
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            int noise = rand() & 0x30;
            int snow  = (noise << 16) + (noise << 8) + noise;
            int pixel = get_pixel32(surface, x, y) & 0x00FFFFFF;
            int biasR = (pixel & 0xFF0000) > (snow & 0xFF0000)? (pixel & 0xFF0000) - (snow & 0xFF0000): (snow & 0xFF0000) - (pixel & 0xFF0000);
            int biasG = (pixel & 0xFF00) > (snow & 0xFF00)? (pixel & 0xFF00) - (snow & 0xFF00): (snow & 0xFF00) - (pixel & 0xFF00);
            int biasB = (pixel & 0xFF) > (snow & 0xFF)? (pixel & 0xFF) - (snow & 0xFF): (snow & 0xFF) - (pixel & 0xFF);

            int pxno  = (biasR + biasG + biasB) | 0xFF000000;
            put_pixel32(dest, x, y,
                        pxno );
        }
}

void testHRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    SDL_FillRect(dest, NULL, 0x000000);
    for(int y=0; y< SCREEN_HEIGHT; ++y) {
        int noiseSlip = round((rand() & 0xFF) / 0x50);
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int pixel = get_pixel32(surface, x, y);
            float screenpos = ((float) y + warp) / SCREEN_HEIGHT;
            int slip = round((sin((M_PI * 4) * screenpos)) * 6) + noiseSlip;
            int newx = x + slip;
            if (newx > 0) put_pixel32(dest, newx, y, pixel);
        }
    }
}

void testVRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    SDL_FillRect(dest, NULL, 0x000000);
    int noiseSlip = round((rand() & 0xFF) / 0xF0);
    for(int y=0; y< SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int pixel = get_pixel32(surface, x, y);
            float screenpos = ((float) y + warp) / SCREEN_HEIGHT;
            int slip = round((cos((M_PI * 2) * screenpos)) * 10) + noiseSlip;
            int newy = y + slip;
            if (newy > 0 && newy < SCREEN_HEIGHT) put_pixel32(dest, x, newy, pixel);
        }
    }
}

void testBlend( SDL_Surface *surface, SDL_Surface *last, SDL_Surface *dest) {
    SDL_FillRect(dest, NULL, 0x000000);

    for(int y=0; y< SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int persist  = get_pixel32(last, x, y);
            int pxno = (persist & 0x000FFFFFF) | 0xF0000000;
            put_pixel32(last, x, y, pxno);
        }
    }
    //SDL_SetSurfaceBlendMode(dest, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(last, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(last, NULL, dest, NULL);
    SDL_BlitSurface(surface, NULL, dest, NULL);
}

void testGhost( SDL_Surface *surface, SDL_Surface *dest, int delay) {
    SDL_FillRect(dest, NULL, 0x000000);

    for(int y=0; y< SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            int pixel  = get_pixel32(surface, x, y);
            int pixeld = get_pixel32(surface, x + delay, y);
            put_pixel32(dest, x + delay, y, ((pixel & 0xFFFFFF) + (pixeld & 0xFFFFFF)) | 0xFF000000);
        }
    }
}


bool init()
{
	//Initialization flag
	bool success = true;
    srand(time(0));
    //Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		success = false;
	}
	else
	{
		//Create window
		gWindow = SDL_CreateWindow( "SDL CRT Filter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if( gWindow == NULL )
		{
			printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
			success = false;
		}
		else
		{
			//Get window surface
			gScreenSurface = SDL_GetWindowSurface( gWindow );
		}
	}

	return success;
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;

	//Load splash image
	gHelloWorld = SDL_LoadBMP( "testCardRGB.bmp" );
	if( gHelloWorld == NULL )
	{
		printf( "Unable to load image %s! SDL Error: %s\n", "standby640.bmp", SDL_GetError() );
		success = false;
	}
	return success;
}

void close()
{

    SDL_FreeSurface( gBuffer );
    SDL_FreeSurface( gBlank );
    SDL_FreeSurface( gBack );
	//Deallocate surface
	SDL_FreeSurface( gHelloWorld );
	gHelloWorld = NULL;

	//Destroy window
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

int main( int argc, char* args[] )
{

    //Main loop flag
    bool quit = false;

    //Event handler
    SDL_Event e;

	//Start up SDL and create window
	if( !init() )
	{
		printf( "Failed to initialize!\n" );
	}
	else
	{
		//Load media
		if( !loadMedia() )
		{
			printf( "Failed to load media!\n" );
		}
		else
		{
		    int warp = 0;
            loadMedia();
            createBuffer();
            SDL_BlitSurface(gHelloWorld, NULL, gBack, NULL);
            while(!quit) {
                while( SDL_PollEvent( &e ) != 0 ) {
                    //User requests quit
                    if (e.type == SDL_QUIT) {
                        quit = true;
                    }
                }
                //if((rand() & 1)) testInvert(gHelloWorld);


                SDL_FillRect(gScreenSurface, NULL, 0x000000);
                SDL_BlitSurface(gHelloWorld, NULL, gBlank, NULL);
                //if((rand() & 1)) testDesaturate(gBuffer, gBlank); else SDL_BlitSurface(gBuffer, NULL, gBlank, NULL);
                testNoise(gBlank, gBuffer);
                testHRipple(gBuffer, gBlank, warp);
                testVRipple(gBlank, gBuffer, warp);
                testBlend(gBuffer, gBack, gBlank);

                //Apply the image
                SDL_BlitSurface(gBlank, NULL, gScreenSurface, NULL);
                SDL_BlitSurface(gScreenSurface, NULL, gBack, NULL);

                //Update the surface
                SDL_UpdateWindowSurface(gWindow);
                SDL_Delay(25);
                ++warp;
            }
			//Wait two seconds
			//SDL_Delay( 2000 );
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}