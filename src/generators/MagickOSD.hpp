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
    void text( double x, double y, const std::string& txt );
    void centerXtxt( double y, const std::string& txt );
    void getSurface( SDL_Surface* surface );
    void test();
    double getTextWidth( Magick::Image& img, const std::string& txt );
    double getTextHeight( Magick::Image& img, const std::string& txt );
    double getFontSize() { return fontsize; }
    void   setFontSize( double f ) {  fontsize = f; }

protected:
    static void initBuffer( Magick::Image& buff );
    void copy( Magick::Image& source, Magick::Image& target );
    void blankMe();
    Magick::Image img;
    Magick::Image blank;
    const std::string font = "resources/fonts/Vintage2513ROM.ttf";
    double fontsize = 0;
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


void MagickOSD::text( double x, double y, const std::string &txt ) {
    //blankMe();
    img.font( font );
    img.fontPointsize( fontsize );
    img.draw(DrawableText{ x, y, txt } );
}

void MagickOSD::centerXtxt( double y, const std::string &txt ) {
    //blankMe();
    img.font( font );
    img.fontPointsize( fontsize );
    double x = ( Config::SCREEN_WIDTH -  getTextWidth( img, txt ) ) / 2;
    y += getTextHeight( img, txt ) / 2 ;
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
    double ypos = Config::SCREEN_HEIGHT * 3 / 4;
    std::string txt = "THIS IS A TEST";
    double xpos = 10;
    setFontSize( size );
    blankMe();
    text( xpos, ypos, txt );
    blankMe();
    centerXtxt( ypos, txt );
}

void MagickOSD::getSurface( SDL_Surface *surface ) {
    MagickLoader::image2surface( img, surface );
}

double MagickOSD::getTextWidth(Magick::Image &img, const std::string &txt) {
    TypeMetric metric;
    img.font( font );
    img.fontTypeMetrics( txt, &metric );
    return metric.textWidth();
}

double MagickOSD::getTextHeight(Magick::Image &img, const std::string &txt) {
    TypeMetric metric;
    img.font( font );
    img.fontTypeMetrics( txt, &metric );
    return metric.textHeight();
}

#endif //SDL_CRT_FILTER_MAGICKOSD_HPP
