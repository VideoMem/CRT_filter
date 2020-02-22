//
// Created by sebastian on 20/2/20.
//

#ifndef SDL_CRT_FILTER_MAGICKLOADER_HPP
#define SDL_CRT_FILTER_MAGICKLOADER_HPP
#include <Magick++.h>
#include <loaders/LazySDL2.hpp>

class MagickLoader: public Loader {
public:
    bool GetSurface(SDL_Surface* surface) override;

private:
    static void magickLoad(std::string path, SDL_Surface* surface);
};

void MagickLoader::magickLoad(std::string path, SDL_Surface* surface) {
    using namespace Magick;
    Image image;
    try {
        image.read( path );
        //SDL_Log("Magick++ loaded image %ldx%ld", image.columns(), image.rows() );
        Geometry newSize = Geometry(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
        newSize.aspect(true);
        image.interpolate(BicubicInterpolatePixel );
        image.resize(newSize);
        int imgWidth = image.columns();
        int imgHeight = image.rows();
        //SDL_Log("Magick++ resized to screen %dx%d", imgWidth, imgHeight);
        image.modifyImage();
        Uint32 pixel = 0;
        for ( int row = 0; row < imgHeight; row++ ) {
            for (int column = 0; column < imgWidth; column++) {
                ColorRGB px = image.pixelColor(column, row);
                Uint32 r = px.red()   * 0xFF;
                Uint32 g = px.green() * 0xFF;
                Uint32 b = px.blue()  * 0xFF;
                toPixel(&pixel, &r, &g, &b);
                put_pixel32( surface, column, row, pixel );
            }
        }
    }
    catch( Exception &error_ ) {
        SDL_Log("Caught exception: %prngState", error_.what());
    }

}

bool MagickLoader::GetSurface(SDL_Surface* surface) {
    SDL_Rect rect;
    SDL_Surface * gX;

    //Loads channel still image
    std::string imagePath = current().GetUri();
    gX = AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    SDL_FillRect( gX, nullptr,  0x00 );

    if( gX == nullptr ) {
        SDL_Log( "Unable to allocate image %s! SDL Error: %prngState\n", imagePath.c_str(), SDL_GetError() );
    } else {
        magickLoad(imagePath, gX);
        SDL_GetClipRect( gX, &rect );
        //SDL_Log("Loaded image size: %dx%d", rect.w, rect.h);
        SDL_FillRect( surface, nullptr, 0x0);
        SDL_Rect dst;
        dst.x = 0;
        dst.y = 0;
        dst.w = Config::SCREEN_WIDTH;
        dst.h = Config::SCREEN_HEIGHT;
        //SDL_Log("Target image size: %dx%d", dst.w, dst.h);
        SDL_BlitScaled( gX, &rect, surface, &dst );
        SDL_FreeSurface( gX );
    }

    return gX != nullptr;
}

#endif //SDL_CRT_FILTER_MAGICKLOADER_HPP
