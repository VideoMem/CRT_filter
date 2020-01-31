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

void testNoise( SDL_Surface *surface ) {
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            int noise = rand() & 0x50;
            int pixel = get_pixel32(surface, x, y) & 0xFF;
            int bias  = pixel > noise? pixel - noise: noise - pixel; //gain
            int snow  = (bias << 16) + (bias << 8) + bias;
            int pxno  = snow | 0xFF000000;
            put_pixel32(surface, x, y,
                        pxno);
        }
}

void testHRipple( SDL_Surface *surface, SDL_Surface *dest, int warp ) {
    for(int x=0; x< SCREEN_WIDTH; ++x)
        for(int y=0; y< SCREEN_HEIGHT; ++y) {
            int pixel = get_pixel32(surface, x, y);
            float screenpos = ((float)y + warp )/ SCREEN_HEIGHT;
            int slip = round((sin((M_PI* 4) * screenpos)) * 10);
            int newx = x + slip;
            if (newx > 0 && newx < SCREEN_WIDTH) put_pixel32(dest, newx, y, pixel);
            if (newx > SCREEN_WIDTH) put_pixel32(dest, newx, y, 0);

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
            //testInvert(gHelloWorld);
            while(!quit) {
                while( SDL_PollEvent( &e ) != 0 ) {
                    //User requests quit
                    if (e.type == SDL_QUIT) {
                        quit = true;
                    }
                }
                //SDL_RenderClear( gWindow );
                SDL_BlitSurface(gHelloWorld, NULL, gBuffer, NULL);

                testNoise(gBuffer);
                SDL_BlitSurface(gBuffer, NULL, gBlank, NULL);

                testHRipple(gBlank, gBuffer, warp);

                //Apply the image
                SDL_BlitSurface(gBuffer, NULL, gScreenSurface, NULL);

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