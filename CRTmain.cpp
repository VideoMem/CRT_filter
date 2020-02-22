#include <SDL2/SDL.h>
#include <CRTApp.hpp>
#include <loaders/MagickLoader.hpp>

static void powerOff(CRTApp& crt) {
    //crt.setBlend(true);
    crt.setHRipple(true);
    crt.setVRipple(true);
    crt.noise(true);
    crt.setRipple(1);
    while (crt.getSupply() > 0.01) {
        crt.setSupply(crt.getSupply() / 1.3);
        crt.shutdown();
        crt.update();
    }

    crt.noise(false);

    float scale =1;
    while (scale > 0.9) {
        crt.strech( scale );
        crt.focusNoise();
        crt.dot();
        crt.fade();
        scale /= 1.2;
        crt.redraw();
    }
    Uint32 fanout = crt.wTime() + 4347;
    while(crt.wTime() < fanout) {
        crt.strech(scale);
        crt.fade();
        crt.redraw();
    }
}

//int main( int argc, char* args[] ) {
int main(  ) {
    static MagickLoader loader;
    static CRTApp crt = CRTApp(loader);
    crt.Standby();

    bool quit = false;

    //Event handler
    SDL_Event e;
    float ripple = 0.03;
    float noise = 0.4;
    float brightness = 1;
    float contrast = 1;
    float color = 1;

    //Start up SDL and create window
    while(!quit) {

        crt.update();
        //Update the surface

        crt.setRipple(ripple);
        crt.setNoise(noise);
        crt.setContrast(contrast);
        crt.setBrightness(brightness);
        crt.setColor(color);

        while( SDL_PollEvent( &e ) != 0 ) {
            //User requests quit
            if (e.type == SDL_QUIT) {
                quit = true;
                powerOff(crt);
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
                        crt.resetFrameStats();
                        brightness = 1; contrast = 1; color = 1;
                        break;
                    case SDLK_1:
                        crt.noise(true);
                        break;
                    case SDLK_q:
                        crt.noise(false);
                        break;
                    case SDLK_2:
                        crt.setHRipple(true);
                        break;
                    case SDLK_w:
                        crt.setHRipple(false);
                        break;
                    case SDLK_3:
                        crt.setVRipple(true);
                        break;
                    case SDLK_e:
                        crt.setVRipple(false);
                        break;
                    case SDLK_4:
                        crt.setBlend(true);
                        break;
                    case SDLK_r:
                        crt.setBlend(false);
                        break;
                    case SDLK_PAGEUP:
                        crt.Up();
                        crt.loadMedia();
                        break;
                    case SDLK_PAGEDOWN:
                        crt.Dw();
                        crt.loadMedia();
                        break;
                    case SDLK_END:
                        powerOff(crt);
                        crt.noise(true);
                        crt.setBlend(true);
                        crt.setHRipple(true);
                        crt.setVRipple(true);
                        crt.setSupply(1);
                        crt.Dw();
                        crt.Up();
                        crt.loadMedia();
                        break;
                    default:
                        break;
                }
            }
        }
    }

	//Free resources and close SDL
	return 0;
}