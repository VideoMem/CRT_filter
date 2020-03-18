//
// Created by sebastian on 16/3/20.
//

#ifndef SDL_CRT_FILTER_MAGICKOSD_HPP
#define SDL_CRT_FILTER_MAGICKOSD_HPP
#include <generators/Generator.hpp>
#include <loaders/MagickLoader.hpp>

using namespace Magick;

class MagickOSD: public Generator {
public:
    MagickOSD();
    void text( double x, double y, double size, const std::string& txt );
    void getSurface( SDL_Surface* surface );
    void test();

protected:
    static void initBuffer( Magick::Image& buff );
    void copy( Magick::Image& source, Magick::Image& target );
    void blankMe();
    Magick::Image img;
    Magick::Image blank;
    const std::string font = "resources/fonts/Vintage2513ROM.ttf";
};


void MagickOSD::initBuffer( Magick::Image& buff ) {
    Geometry newSize = Geometry( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    buff.size( newSize );
    buff.magick( "RGBA" );
    buff.strokeColor("transparent" );
    buff.fillColor("white" );
    buff.modifyImage();
}

MagickOSD::MagickOSD() {
    initBuffer( img );
    initBuffer( blank );
}

void MagickOSD::text( double x, double y, double size, const std::string &txt ) {
    blankMe();
    img.font( font );
    img.fontPointsize( size );
    img.draw(DrawableText{ x, y, txt } );
}

void MagickOSD::copy( Magick::Image &source, Magick::Image &target ) {
    Blob blob;
    source.magick("BMP");
    target.magick("BMP");
    source.write( &blob );
    target.read( blob );
}

void MagickOSD::blankMe() {
    copy ( blank, img );
}

void MagickOSD::test() {
    double size = Config::SCREEN_HEIGHT / 10;
    double ypos = Config::SCREEN_HEIGHT / 2;
    std::string txt = "THIS IS A TEST";
    double xpos = 10;
    text( xpos, ypos, size, txt );
}

void MagickOSD::getSurface( SDL_Surface *surface ) {
    MagickLoader::image2surface( img, surface );
}

#endif //SDL_CRT_FILTER_MAGICKOSD_HPP
