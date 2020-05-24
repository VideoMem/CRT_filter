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
    static void image2surface( Magick::Image& image, SDL_Surface* surface );
    static void surface2image( SDL_Surface* surface, Magick::Image& img );
    static Blob readBlob( const std::string path );
    static void saveBlob( SDL_Surface* surface, const std::string path );
    static std::string sha256Log( std::string string ) {
        Blob blob = { static_cast<const char*>(string.c_str()), string.size() };
        return sha256Log( blob );
    }

private:
    static std::string sha256Log( Blob& blob );
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

std::string MagickLoader::sha256Log(Blob &blob) {
    std::string data( static_cast<const char*>(blob.data()), 0, blob.length() );
    std::string hash_hex_str;
    picosha2::hash256_hex_string(data, hash_hex_str);
    return hash_hex_str;
}

Blob MagickLoader::readBlob( const std::string path ) {
    using namespace std;
    ifstream myfile;
    char head[5] = { 0 };
    try {
        myfile.open(path, ios::in | ios::binary);
        if (myfile.is_open()) {
            myfile.seekg(0, ios::end);
            ifstream::pos_type size = myfile.tellg();
            char *memblock = new char[size];
            myfile.seekg(0, ios::beg);
            myfile.read(memblock, size);
            SDL_Log("%ld bytes read", (long int) size);
            Blob retblob = Blob(memblock, size);
            memcpy(head, retblob.data(), 4);
            SDL_Log("SHA256 (BlobRead) : %s", sha256Log(retblob).c_str());
            myfile.close();
            delete[] memblock;
            return retblob;
        }
    } catch (Exception& e) {
        SDL_Log("Cannot open file: %s", e.what() );
    }
    //on fail
    return Blob();
}

void MagickLoader::saveBlob( SDL_Surface* surface, const std::string path ) {
    Blob blob;
    Image image = { Geometry( surface->w, surface->h ), Color() };
    surface2image( surface, image );
    image.magick( Config::magick_default_format() );
    image.write( &blob );

    using namespace std;
    ofstream myfile;
    try {
        myfile.open ( path, ios::out | ios::binary );
        if (myfile.is_open()) {
            myfile.write(static_cast<const char *>(blob.data()), blob.length() );
            myfile.close();
            SDL_Log("%ld bytes written ", blob.length() );
            SDL_Log( "SHA256 (BlobSave) : %s", sha256Log(blob).c_str() );
        } else
            SDL_Log("Cannot write: %s", path.c_str() );
    } catch (Exception &e) {
        SDL_Log("Cannot open file for writing: %s", e.what() );
    }
}

void MagickLoader::magickLoad(std::string path, SDL_Surface* surface) {
    Image image {
            Geometry( Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT ) ,
            Color()
    };

    try {
        SDL_Log("MagickLoader::Opening image %s", path.c_str());

        if (testFile(path)) {
            Blob blob = readBlob(path);
            image.fileName( ":");
            SDL_Log("MagickLoader::readBlob() OK, pinging ...");
            image.ping( blob );
            SDL_Log("MagickLoader::readBlob() OK, pinging done!");
            if(image.fileSize() > 0) {
                SDL_Log("Format detected: %s", image.magick().c_str());
                image.read(blob);
            } else
                SDL_Log( "Cannot open a truncated file" );
        } else {
             SDL_Log("MagickLoader::Cannot read %s", path.c_str() );
        }
/*
        if ( testFile( path ) ) image.read( path ); else {
            SDL_Log("MagickLoader::Cannot read %s", path.c_str() );
        }*/
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
    //blank(surface);
    SDL_Rect dstsize;
    SDL_GetClipRect( surface, &dstsize );
    size_t imgWidth  = image.columns() > (size_t) dstsize.w? dstsize.w : image.columns();
    size_t imgHeight = image.rows() > (size_t) dstsize.h   ? dstsize.h : image.rows();
    Uint32 pixel = 0;
    for ( size_t row = 0; row < imgHeight; row++ ) {
        for (size_t column = 0; column < imgWidth; column++) {
            ColorRGB px = image.pixelColor( column, row );
            Uint32 r = round(px.red()   * 0xFF);
            Uint32 g = round(px.green() * 0xFF);
            Uint32 b = round(px.blue()  * 0xFF);
            Uint32 a = round((1.0 - px.alpha()) * 0xFF );
            toPixel( &pixel, &r, &g, &b , &a );
            put_pixel32( surface, column, row, pixel );
        }
    }
}

void MagickLoader::surface2image(SDL_Surface *surface, Magick::Image &img) {
    using namespace Magick;
    SDL_Rect dstsize;
    SDL_GetClipRect(surface, &dstsize);
    size_t imgWidth = img.columns() > (size_t) dstsize.w ? dstsize.w : img.columns();
    size_t imgHeight = img.rows() > (size_t) dstsize.h ? dstsize.h : img.rows();
    Uint32 r, g, b, a;
    Uint32 pixel = 0;
    for (size_t y = 0; y < imgHeight; y++) {
        for (size_t x = 0; x < imgWidth; x++) {
            pixel = get_pixel32( surface, x, y );
            comp( &pixel, &r, &g, &b, &a );
            img.pixelColor( x, y, ColorRGB(
                    (double) r / 0xFF,
                    (double) g / 0xFF,
                    (double) b / 0xFF
                    ));

        }
    }
    //img.alphaChannel(SetAlphaChannel);
    //img.copyPixels(image, image.geometry(), Offset(0));
}


#endif //SDL_CRT_FILTER_MAGICKLOADER_HPP
