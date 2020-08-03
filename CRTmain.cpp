#include <SDL2/SDL.h>
#include <CRTApp.hpp>
#include <loaders/ZMQLoader.hpp>
#include <loaders/ZMQVideoPipe.hpp>
#include <transcoders/libAVable.hpp>
#include <thread>
#include "transcode.h"

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

void send_frame( bool* quit, ZMQVideoPipe* zPipe ) {
    SDL_Log("Frame send thread initialized");
    //duration<double> frameTime( Waveable::conversion_size( zPipe->reference() ) /  FRONT_SAMPLERATE );
    //SDL_Log("Frontend Samplerate: %lf, frame time %02lf ms", FRONT_SAMPLERATE, frameTime.count() );
    while(!*quit) {
        auto start = high_resolution_clock::now();
        zPipe->pushFrame();
        auto stop = high_resolution_clock::now();
        auto elapsed = duration_cast<milliseconds>(stop - start);
        //if(duration_cast<milliseconds>(frameTime) > elapsed) {
        //    auto error = duration_cast<milliseconds>(frameTime - elapsed) * 0.8;
        //    std::this_thread::sleep_for(error);
        //}
    }
    SDL_Log("Frame send thread done!");
}

void recv_frame( bool* quit, ZMQLoader* zLoader, ZMQVideoPipe* zPipe, CRTApp* app ) {
    SDL_Surface* frame = Surfaceable::AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    size_t depth = frame->w / KERNING_SIZE;
    auto hs = new LibAVable_hypersurface_t();
    auto hc = new LibAVable_hypersurface_t();
    auto hct = new LibAVable_hypersurface_t();
    hs->step = KERNING_SIZE;
    hs->depth = depth * DEPTH_MULTIPLIER;

    SDL_Surface* frame_spiral = Surfaceable::AllocateSurface( frame );

    SDL_Surface* full = Surfaceable::AllocateSurface( Config::VIDEOFRAME_WIDTH, Config::VIDEOFRAME_HEIGHT );
    auto aux_surface = Surfaceable::AllocateSurface( frame );
    duration<double> frameTime( Waveable::conversion_size( zPipe->reference() ) /  FRONT_SAMPLERATE );

    std::string filename = "outstream.mp4";
    std::string codec_name = "libx264";
    auto state = LibAVable::init_state( full, codec_name ); //ceil((double) FRONT_SAMPLERATE / (frame->w * frame->h)) );
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };


    FILE* f = fopen( filename.c_str() , "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename.c_str() );
        exit(1);
    }

    SDL_Log("Frame receive thread initialized.");
    state->frame->pts = 0;
    //int frame_diff = floor((double) 25 / ((double) FRONT_SAMPLERATE / (frame->w * frame->h) ));
    auto encoded_frame = LibAVable::AllocateFrame( state->frame->width, state->frame->height, state->frame->format );
    while(!*quit) {
        auto start = high_resolution_clock::now();
        zLoader->pullFrame();
        app->update();
        app->getCode(frame);

        app->getFrame(full);


        if( !hct->surfaces.empty() ) {
            LibAVable::blitScaled( full, hct->surfaces.front() );
            LibAVable::decode( state->frame, full );
            LibAVable::writefile(state, f);
            hct->surfaces.erase(hct->surfaces.begin());
        }

        if( !hc->surfaces.empty() && hct->surfaces.empty() ) {
            //for (auto &img: hc->surfaces ) {
            //    LibAVable::pack_miniraster ( full_copy, img, PACK_SIZE );
            //    Loader::SurfacePixelsCopy( full_copy, img );
            //}
            LibAVable::hs_itranspose( hc, hct, TRANSPOSE_TIMES );
            //LibAVable::hs_transpose( hc, hct );

            LibAVable::hs_free( hc );
        }
        //LibAVable::pack_all ( aux_surface, frame, PACK_SIZE );
        auto full_copy = LibAVable::hs_stack( hs, hc, frame );
        SDL_FreeSurface( full_copy );
        /*
        int ret = avcodec_receive_frame( state->c , encoded_frame );
        if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
        }
         */
        //LibAVable::encode( full, encoded_frame );
        //Magickable::blitScaled( frame, full );
        zPipe->testSendFrame(frame);
        state->frame->pts++;
        //}

        auto stop = high_resolution_clock::now();
        auto elapsed = duration_cast<milliseconds>(stop - start);
        if(duration_cast<milliseconds>(frameTime) > elapsed) {
             auto error = (duration_cast<milliseconds>(frameTime) - elapsed) * 0.8;
             //SDL_Log("Elapsed %ld ms, waiting %02f ms: total %02f ms, frame %ld ms", elapsed.count(), error.count(),
             //         (elapsed + error).count(), duration_cast<milliseconds>(frameTime).count() );
             //std::this_thread::sleep_for(error);
        }
    }

    //flush
    state->frame->pts++;
    LibAVable::writefile( state, f );
    /* add sequence end code to have a real MPEG file */
    if (state->codec->id == AV_CODEC_ID_MPEG1VIDEO || state->codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    LibAVable::hs_free(hct);
    LibAVable::hs_free(hc);
    LibAVable::hs_free(hs);
    delete hct;
    delete hc;
    delete hs;
    avcodec_free_context(&state->c);
    av_frame_free(&state->frame);
    av_packet_free(&state->pkt);
    SDL_FreeSurface(full);
    SDL_FreeSurface(frame_spiral);
    SDL_FreeSurface(frame);
    SDL_FreeSurface(aux_surface);
    SDL_Log("Frame receive thread done!");
    SDL_Log("Resending video frames");
    resend_stream("h264", filename, zPipe, app);
    SDL_Log("End of video");
}

int main(  ) {
    if (VIPS_INIT ("namefile"))
        vips_error_exit (nullptr);

    SDL_Surface* frame = Loader::AllocateSurface( Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    static ZMQLoader zLoader;
    static ZMQVideoPipe zPipe;
    static bool quit = false;
    static CRTApp crt = CRTApp(zLoader);
    std::thread radio_tx(send_frame, &quit, &zPipe);
    std::thread radio_rx(recv_frame, &quit, &zLoader, &zPipe, &crt);
    crt.Standby();

    //Event handler
    SDL_Event e;
    double ripple = 0.01;
    double noise = 0.01;
    double brightness = 1;
    double contrast = 1;
    double color = 1;

    //Start up SDL and create window
    while(!quit) {

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
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
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
	//Free resources and close SDL
	SDL_FreeSurface(frame);
	radio_rx.join();
    radio_tx.join();
	return 0;
}