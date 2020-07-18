//
// Created by sebastian on 3/6/20.
//

#ifndef SDL_CRT_FILTER_TRANSCODE_H
#define SDL_CRT_FILTER_TRANSCODE_H

#define FRONT_SAMPLERATE 9216000
#define INBUF_SIZE 4096
#define KERNING_SIZE 16
#define PACK_SIZE 16
#define TRANSPOSE_TIMES 1
#define DEPTH_MULTIPLIER 1

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

AVCodec* detect_format( std::string path ) {
    // get format from audio file
    av_register_all();
    AVFormatContext* format = avformat_alloc_context();
    if (avformat_open_input(&format, path.c_str(), NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file '%s'\n", path.c_str());
    }
    if (avformat_find_stream_info(format, NULL) < 0) {
        fprintf(stderr, "Could not retrieve stream info from file '%s'\n", path.c_str());
    }

    // Find the index of the first video stream
    int stream_index =- 1;
    for (int i=0; i<format->nb_streams; i++) {
        if (format->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            fprintf(stdout, "Found video stream at: %d\n", i );
            stream_index = i;
            break;
        }
    }
    if (stream_index == -1) {
        fprintf(stderr, "Could not retrieve audio stream from file '%s'\n", path.c_str() );
    }

    fprintf(stdout, "Detected codec id: %d\n", format->streams[stream_index]->codec->codec_id );
    return avcodec_find_decoder( format->streams[stream_index]->codec->codec_id );
}


void resend_stream(string codec_name, string file_name, ZMQVideoPipe *zPipe, CRTApp *app) {
    SDL_Surface* frame = Surfaceable::AllocateSurface(Config::NKERNEL_WIDTH, Config::NKERNEL_HEIGHT );
    auto state = LibAVable::init_state( codec_name, 1, Config::VIDEOFRAME_WIDTH, Config::VIDEOFRAME_HEIGHT );
    SDL_Surface* full_spiral = Surfaceable::AllocateSurface( frame );
    auto aux_surface = Surfaceable::AllocateSurface( frame );

    size_t depth = frame->w / KERNING_SIZE;
    auto hs = new LibAVable_hypersurface_t();
    auto hc = new LibAVable_hypersurface_t();
    auto hct = new LibAVable_hypersurface_t();
    hs->step = KERNING_SIZE;
    hs->depth = depth * DEPTH_MULTIPLIER;

    uint8_t *data;
    size_t   data_size;
    duration<double> frameTime( Waveable::conversion_size( zPipe->reference() ) /  FRONT_SAMPLERATE );

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
            auto start = high_resolution_clock::now();
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

                    if( !hct->surfaces.empty() ) {
                        LibAVable::unpack_miniraster ( aux_surface, hct->surfaces.front() , PACK_SIZE );
                        zPipe->testSendFrame( aux_surface );
                        hct->surfaces.erase(hct->surfaces.begin());
                    }

                    if( !hc->surfaces.empty() && hct->surfaces.empty() ) {
                        LibAVable::hs_iuntranspose( hc, hct, TRANSPOSE_TIMES );
                        //LibAVable::hs_untranspose( hc, hct );
                        LibAVable::hs_free( hc );
                    }

                    LibAVable::blitScaled( aux_surface, recovered_surface );
                    //LibAVable::unpack_miniraster( aux_surface, full_spiral, PACK_SIZE );
                    auto full_copy = LibAVable::hs_stack( hs, hc, aux_surface );
                    SDL_FreeSurface( full_copy );

                    auto stop = high_resolution_clock::now();
                    auto elapsed = duration_cast<milliseconds>(stop - start);
                    if(duration_cast<milliseconds>(frameTime) > elapsed) {
                        auto error = (duration_cast<milliseconds>(frameTime) - elapsed) * 0.8;
                        //SDL_Log("Elapsed %ld ms, waiting %02f ms: total %02f ms, frame %ld ms", elapsed.count(), error.count(),
                        //         (elapsed + error).count(), duration_cast<milliseconds>(frameTime).count() );
                        //std::this_thread::sleep_for(error);
                    }
                } else {
                    break;
                }
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
    SDL_FreeSurface( frame );
    SDL_FreeSurface( full_spiral );
    SDL_FreeSurface( aux_surface );

    //delete state;
    LibAVable::hs_free(hc);
    LibAVable::hs_free(hs);
    LibAVable::hs_free(hct);
    delete hs; delete hc; delete hct;

}


#endif //SDL_CRT_FILTER_TRANSCODE_H
