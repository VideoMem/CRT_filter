//
// Created by sebastian on 16/3/20.
//

#ifndef SDL_CRT_FILTER_MAGICKOSD_HPP
#define SDL_CRT_FILTER_MAGICKOSD_HPP
#include <generators/Generator.hpp>
#include <loaders/MagickLoader.hpp>
#include <SDL2/SDL.h>

using namespace Magick;

class MagickOSD: public Generator {
public:
    struct TextSize { const double p =  3.0 / 50; const double H1 = 3 * p ;  const double H2 = 2 * p; };
    MagickOSD();
    void text( double x, double y, const std::string& txt );
    void shadowText( size_t x, double y, const std::string& txt );
    void centerXtxt( double y, const std::string& txt );
    void getSurface( SDL_Surface* surface );
    void test();
    double getTextWidth( Magick::Image& img, const std::string& txt );
    double getTextHeight( Magick::Image& img, const std::string& txt );
    double getFontSize() { return fontsize; }
    void   setFontSize( double f ) { fontsize = f; }
    static SDL_Rect textSafeArea( Magick::Image& img );
    void clear() { blankMe(); }

protected:
    static void initBuffer( Magick::Image& buff );
    void copy( Magick::Image& source, Magick::Image& target );
    void blankMe();
    Magick::Image img {
        Geometry( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT ) ,
        Color()// Color( 0, 0, 0, TransparentAlphaChannel )
    };
    Magick::Image blank {
        Geometry( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT ) ,
        Color()//Color( 0, 0, 0, TransparentAlphaChannel )
    };
    const std::string font = "resources/fonts/Vintage2513ROM.ttf";
    double fontsize = 0;
};


void MagickOSD::initBuffer( Magick::Image& buff ) {
    buff.strokeColor("transparent" );
    buff.fillColor("white" );
    buff.modifyImage();
}


MagickOSD::MagickOSD() {
    initBuffer( img );
    initBuffer( blank );

    TextSize txs;
    setFontSize( Config::SCREEN_HEIGHT * txs.p );
    img.fontPointsize( fontsize );
    blank.fontPointsize( fontsize );
    img.font(font);
    blank.font(font);

}


void MagickOSD::text( double x, double y, const std::string &txt ) {
    try {
        const Drawable line = DrawableText{ x, y, txt };
        img.draw( line );
    } catch (const std::exception& e) {
        SDL_Log("Error loading font: %s", e.what());
    }
}


void MagickOSD::centerXtxt( double y, const std::string &txt ) {
    double w = Config::SCREEN_WIDTH;
    double t = getTextWidth( img, txt );
    double x = ( w - t ) / 2;
    y += getTextHeight( img, txt ) / 2.0;
    shadowText( x, y , txt );
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

    double ypos = Config::SCREEN_HEIGHT / 2;
    std::string txt = "[ THIS IS A TEST ]";
    SDL_Log("MagickOSD() test: %s", txt.c_str() );
    double xpos = 10;
    blankMe();
    SDL_Log("MagickOSD() test: blank" );
    text( xpos, ypos, txt );
    SDL_Log("MagickOSD() test: text" );
    blankMe();
    centerXtxt( ypos, txt );
    SDL_Log("MagickOSD() test: centerXtxt" );
}

void MagickOSD::getSurface( SDL_Surface *surface ) {
    //img.modifyImage();
    MagickLoader::image2surface( img, surface );
}

double MagickOSD::getTextWidth(Magick::Image &img, const std::string &txt) {
    TypeMetric metric;
    //img.font( font );
    img.fontTypeMetrics( txt, &metric );
    return metric.textWidth();
}

double MagickOSD::getTextHeight(Magick::Image &img, const std::string &txt) {
    TypeMetric metric;
    //img.font( font );
    img.fontTypeMetrics( txt, &metric );
    return metric.textHeight();
}

SDL_Rect MagickOSD::textSafeArea(Magick::Image& frame) {
    SDL_Rect area;
    area.w = round(frame.size().width() * 41.6 / 51.95 );
    area.x = round(( frame.size().width() - area.w ) / 2 );
    area.h = round( frame.size().height() * 230.0 / 287 );
    area.y = round( ( frame.size().height() - area.h ) / 2 );
    return area;
}

void MagickOSD::shadowText(size_t x, double y, const std::string &txt) {
    img.fillColor( ColorRGB( 0,0.1, 0.1) );
    //img.strokeColor( "gray" );
    double xoffset = x + fontsize / 20;
    double yoffset = y + fontsize / 20;
    text( xoffset, yoffset, txt );
    img.fillColor( ColorRGB( 1,1,1 ) );
    //img.strokeColor( "white" );
    text( x, y, txt );
}

#endif //SDL_CRT_FILTER_MAGICKOSD_HPP
