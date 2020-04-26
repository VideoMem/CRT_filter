//
// Created by sebastian on 20/2/20.
//

#ifndef SDL_CRT_FILTER_MAGICKLOADER_HPP
#define SDL_CRT_FILTER_MAGICKLOADER_HPP
#include <Magick++.h>
#include <loaders/LazySDL2.hpp>

using namespace Magick;

class MagickLoader: public Loader {
public:
    bool GetSurface(SDL_Surface* surface, SDL_PixelFormat& format) override;
    bool GetSurface(SDL_Surface* surface);
    static void image2surface( Magick::Image& img, SDL_Surface* surface );
private:
    static Blob readBlob( const std::string path );
    static void magickLoad(std::string path, SDL_Surface* surface);
    static void magick2surface(Magick::Image& image, SDL_Surface* surface);
};


void MagickLoader::magick2surface(Magick::Image &image, SDL_Surface *surface) {
    Geometry newSize = Geometry( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    newSize.aspect(true);
    image.interpolate(CatromInterpolatePixel  );
    image.resize(newSize);

    image.modifyImage();
    image2surface( image, surface );
}

Blob MagickLoader::readBlob( const std::string path ) {
    using namespace std;
    ifstream myfile;
    myfile.open ( path, ios::in | ios::binary );
    if (myfile.is_open()) {
        myfile.seekg( 0, ios::end );
        ifstream::pos_type  size = myfile.tellg();
        char* memblock = new char [size];
        myfile.read( memblock, size );
        myfile.close();
        Blob retblob = Blob( memblock, size );
        delete[] memblock;
        return retblob;
    } else
        return Blob();
}

void MagickLoader::magickLoad(std::string path, SDL_Surface* surface) {
    Image image {
            Geometry( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT ) ,
            Color()
    };

    try {
        SDL_Log("MagickLoader::Opening image %s", path.c_str() );
        if ( testFile( path ) ) image.read( readBlob( path ) ); else {
            SDL_Log("MagickLoader::Cannot read %s", path.c_str() );
        }
    }
    catch( Magick::Exception&  error_ ) {
        SDL_Log("MagickLoader::Cannot read Exception %s", error_.what());
    }
    magick2surface( image, surface );
}

bool MagickLoader::GetSurface(SDL_Surface* surface, SDL_PixelFormat& format) {
    SDL_Rect rect;
    SDL_Surface * gX;
    SDL_Surface *gC;

    //Loads channel still image
    std::string imagePath = current().GetUri();
    gX = AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    SDL_FillRect( gX, nullptr,  0x00 );

    if( gX == nullptr ) {
        SDL_Log( "Unable to allocate image %s! SDL Error: %s\n", imagePath.c_str(), SDL_GetError() );
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
        gC = SDL_ConvertSurface( gX, &format, 0 );
        SDL_FreeSurface( gX );
        SDL_BlitScaled( gC, &rect, surface, &dst );
        SDL_FreeSurface( gC );

    }

    return gX != nullptr;
}

bool MagickLoader::GetSurface(SDL_Surface *surface) {
    SDL_Rect rect;
    SDL_Surface * gX;

    //Loads channel still image
    std::string imagePath = current().GetUri();
    gX = AllocateSurface( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT );
    SDL_FillRect( gX, nullptr,  0x00 );

    if( gX == nullptr ) {
        SDL_Log( "Unable to allocate image %s! SDL Error: %s\n", imagePath.c_str(), SDL_GetError() );
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

void MagickLoader::image2surface( Magick::Image &image, SDL_Surface *surface ) {
    using namespace Magick;
    SDL_Rect dstsize;
    SDL_GetClipRect( surface, &dstsize );
    size_t imgWidth  = image.columns() > (size_t) dstsize.w? dstsize.w : image.columns();
    size_t imgHeight = image.rows() > (size_t) dstsize.h   ? dstsize.h : image.rows();
    Uint32 pixel = 0;
    for ( size_t row = 0; row < imgHeight; row++ ) {
        for (size_t column = 0; column < imgWidth; column++) {
            ColorRGB px = image.pixelColor( column, row );
            Uint32 r = px.red()   * 0xFF;
            Uint32 g = px.green() * 0xFF;
            Uint32 b = px.blue()  * 0xFF;
            Uint32 a = 0xFF - (px.alpha() * 0xFF);
            toPixel( &pixel, &r, &g, &b , &a );
            put_pixel32( surface, column, row, pixel );
        }
    }
}


#endif //SDL_CRT_FILTER_MAGICKLOADER_HPP
