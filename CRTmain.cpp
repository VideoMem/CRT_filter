#include <SDL2/SDL.h>
#include <CRTApp.hpp>
#include <loaders/ZMQLoader.hpp>
#include <loaders/ZMQVideoPipe.hpp>
#include <thread>

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

    double scale =1;
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

void send_frame(ZMQVideoPipe* zPipe,  ZMQLoader* zLoader) {
    while(true) {
        zPipe->pushFrame();
        //while(!zLoader->frameEventRead());
    }
}

void recv_frame( ZMQLoader* zLoader, ZMQVideoPipe* zPipe, SDL_Surface* frame ) {
    while(true) {
        zLoader->pullFrame();
    }
}

//int main( int argc, char* args[] ) {z
int main(  ) {
    SDL_Surface *frame = Loader::AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT);
    static ZMQLoader loader;
    static CRTApp crt = CRTApp(loader);
    ZMQVideoPipe zPipe;
    std::thread radio_tx(send_frame, &zPipe, &loader);
    std::thread radio_rx(recv_frame, &loader, &zPipe, frame);
    crt.Standby();

    bool quit = false;

    //Event handler
    SDL_Event e;
    double ripple = 0.03;
    double noise = 0.3;
    double brightness = 1;
    double contrast = 1;
    double color = 1;

    //Start up SDL and create window
    while(!quit) {

        crt.update(frame, &zPipe);
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
                //powerOff(crt);
                break;
            } else if( e.type == SDL_KEYDOWN ) {
                switch (e.key.keysym.sym) {
                    case SDLK_5:
                        contrast += 0.01;
                        if(contrast > 1) contrast = 1;
                        break;
                    case SDLK_t:
                        contrast -= 0.01;
                        if(contrast < 0) contrast = 0;
                        break;
                    case SDLK_y:
                        brightness += 0.01;
                        if(brightness > 1) brightness = 1;
                        break;
                    case SDLK_6:
                        brightness -= 0.01;
                        if(brightness < 0) brightness = 0;
                        break;
                    case SDLK_7:
                        color += 0.01;
                        if (color > 1.5) color = 1.5;
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
                        noise += 0.01;
                        if (noise > 1) noise = 1;
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
                    case SDLK_s:
                        crt.upSpeed();
                        break;
                    case SDLK_x:
                        crt.dwSpeed();
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
                    case SDLK_l:
                        crt.setLoop(!crt.getLoop());
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
                        crt.setBlend(false);
                        crt.setHRipple(true);
                        crt.setVRipple(true);
                        crt.setSupply(1);
                        crt.loadMedia();
                        break;
                    default:
                        break;
                }
            }
        }
    }

    SDL_FreeSurface(frame);
	//Free resources and close SDL
	return 0;
}