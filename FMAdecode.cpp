#include <SDL2/SDL.h>
#include <CRTApp.hpp>
#include <loaders/ZMQLoader.hpp>
#include <loaders/ZMQVideoPipe.hpp>
#include <transcoders/libAVable.hpp>
#include <thread>

#define FRONT_SAMPLERATE 480e3

#define INBUF_SIZE 4096
void resend_stream(string codec_name, string file_name, ZMQVideoPipe *zPipe, CRTApp *app) {
    SDL_Surface* frame = Surfaceable::AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    SDL_Surface* copy  = Surfaceable::AllocateSurface( frame );
    SDL_Surface* interlaced = Surfaceable::AllocateSurface( Config::VIDEOFRAME_WIDTH, Config::VIDEOFRAME_HEIGHT );
    SDL_Surface* deinterlaced  = Surfaceable::AllocateSurface( interlaced );
    auto state = LibAVable::init_state( codec_name, 1, Config::VIDEOFRAME_WIDTH, Config::VIDEOFRAME_HEIGHT );
    uint8_t *data;
    size_t   data_size;

    FILE* f = fopen( file_name.c_str() , "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", file_name.c_str() );
        exit(1);
    }

    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    auto recovered_surface = Surfaceable::AllocateSurface( state->frame->width, state->frame->height );
    bool quit = false;
    SDL_Event e;

    while (!feof(f) && !quit) {
        fflush(stdout);

        while( SDL_PollEvent( &e ) != 0 ) {
            //User requests quit
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            }
        }

        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            int ret = av_parser_parse2(state->parser, state->c, &state->pkt->data, &state->pkt->size,
                                       data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;

            if ( state->pkt->size ) {
                if (LibAVable::readfile(state, f) >= 0) {
                    LibAVable::encode( recovered_surface, state->frame );
                    app->pushCode( recovered_surface );
                    //LibAVable::pack_deinterlace( deinterlaced, recovered_surface );
                    Magickable::blitScaled( frame, recovered_surface );
                    //double error = Pixelable::surface_diff( frame, copy );
                   // SDL_Log("Framediff: %02lf", error );
                   // if( abs(error) * 1000 > 0.27 )
                    zPipe->testSendFrame( frame );
                } else { break; }
            }
        }
    }
    
    //flush
    state->pkt = nullptr;
    LibAVable::readfile( state, f );

    av_parser_close(state->parser);
    avcodec_free_context(&state->c);
    av_packet_free(&state->pkt);
    SDL_FreeSurface( recovered_surface );
    SDL_FreeSurface( interlaced );
    SDL_FreeSurface( deinterlaced );
    SDL_FreeSurface( frame );
    SDL_FreeSurface( copy );
    //delete state;

}

int main(  ) {
    if (VIPS_INIT ("namefile"))
        vips_error_exit (nullptr);

    static ZMQLoader zLoader;
    static ZMQVideoPipe zPipe;
    static CRTApp crt = CRTApp(zLoader);
    crt.Standby();
    crt.update();
    resend_stream( "h264" , "outstream0.mp4", &zPipe, &crt );
	return 0;
}