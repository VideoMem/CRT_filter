#include <SDL2/SDL.h>
#include <CRTApp.hpp>
#include <loaders/ZMQLoader.hpp>
#include <loaders/ZMQVideoPipe.hpp>
#include <transcoders/libAVable.hpp>
#include <thread>
#include "transcode.h"

int main(int argc, char *argv[]) {
    string filename("outstream.mp4");
    if ( argc > 1 ) {
        filename = argv[1];
    }

    if (VIPS_INIT ("namefile"))
        vips_error_exit (nullptr);

    static ZMQLoader zLoader;
    static ZMQVideoPipe zPipe;
    static CRTApp crt = CRTApp(zLoader);
    crt.Standby();
    crt.update();
    auto codec = detect_format( filename );
    resend_stream( codec->name , filename, &zPipe, &crt );
	return 0;
}