//
// Created by sebastian on 23/5/20.
//

#ifndef SDL_CRT_FILTER_MAGICKABLE_HPP
#define SDL_CRT_FILTER_MAGICKABLE_HPP
#include <transcoders/Surfaceable.hpp>
#include <transcoders/Pixelable.hpp>
#include <transcoders/Waveable.hpp>
#include <Magick++.h>
#include <vips/vips8>
#include <loaders/Loader.hpp>

using namespace vips;
using namespace Magick;

class Magickable : public Surfaceable {
public:
    static void encode(void* dst, void* src);
    static void decode(void* dst, void* src);
    static void blitScaled( SDL_Surface* dst, SDL_Surface* src);
    static Image* Allocate(SDL_Surface* reference, double scale);
    static Image* Allocate(SDL_Surface* reference) { return Allocate( reference, 1.0 ); };
    static void blitScaledMagick(SDL_Surface *dst, SDL_Surface *src);
    static void verticalize( SDL_Surface *dst, SDL_Surface *src );
    static void deverticalize( SDL_Surface *dst, SDL_Surface *src );
    static void flip_vertical( SDL_Surface *dst, SDL_Surface *src );

protected:
    static void magick2surface( Magick::Image& image, SDL_Surface* surface );
    static void image2surface( Magick::Image& image, SDL_Surface* surface );
    static void surface2image( SDL_Surface* surface, Magick::Image& img );

};

void Magickable::flip_vertical(SDL_Surface *dst, SDL_Surface *src) {
    for( int line = 0; line < src->h; line ++ ) {
        auto dstline = src->h - line - 1;
        Loader::blitLine( src, dst, line, dstline );
    }
}

void Magickable::magick2surface(Magick::Image &image, SDL_Surface *surface) {
    Geometry newSize = Geometry( surface->w, surface->h );
    newSize.aspect(true);
    image.interpolate( CatromInterpolatePixel );
    image.resize(newSize);

    image.modifyImage();
    image2surface( image, surface );
}

void Magickable::image2surface( Magick::Image &src, SDL_Surface *surface ) {
    Image image = Image(src);
    assert( image.colorSpace() == sRGBColorspace && "Source image not in sRGB Colorspace" );
    if (surface->w != (int) image.columns() || surface->h != (int) image.rows()) {
        Geometry newSize = Geometry(surface->w, surface->h);
        image.interpolate(CatromInterpolatePixel );
        image.resize(newSize);
        image.modifyImage();
    }

    Uint32 pixel = 0;
    for ( size_t row = 0; row < (size_t) surface->h; row++ ) {
        for (size_t column = 0; column < (size_t) surface->w; column++) {
            ColorRGB px = image.pixelColor( column, row );
            Uint32 r = round(px.red()   * 0xFF);
            Uint32 g = round(px.green() * 0xFF);
            Uint32 b = round(px.blue()  * 0xFF);
            Uint32 a = round((1.0 - px.alpha()) * 0xFF );
            Pixelable::fromComponents( &pixel, &r, &g, &b , &a );
            Pixelable::put32( surface, column, row, pixel );
        }
    }
}

void Magickable::surface2image(SDL_Surface *surface, Magick::Image &img) {
    using namespace Magick;
    SDL_Rect dstsize;
    SDL_GetClipRect(surface, &dstsize);
    size_t imgWidth = img.columns() > (size_t) dstsize.w ? dstsize.w : img.columns();
    size_t imgHeight = img.rows() > (size_t) dstsize.h ? dstsize.h : img.rows();
    Uint32 r, g, b, a;
    Uint32 pixel = 0;
    for (size_t y = 0; y < imgHeight; y++) {
        for (size_t x = 0; x < imgWidth; x++) {
            pixel = Pixelable::get32( surface, x, y );
            Pixelable::toComponents( &pixel, &r, &g, &b, &a );
            try {
                img.pixelColor(x, y, ColorRGB(
                        (double) r / 0xFF,
                        (double) g / 0xFF,
                        (double) b / 0xFF
                ));
            } catch ( Magick::Exception& e ) {
                SDL_Log("Cannot assign pixel at %ld, %ld : %s", x, y, e.what());
                assert(false && "Cannot assign pixel exception error");
            }

        }
    }
    img.alphaChannel(SetAlphaChannel);
}

void Magickable::encode(void *dst, void* src) {
    auto destination_surface = static_cast<SDL_Surface*>(dst);
    auto source_image = static_cast<Magick::Image*>(src);
    if( source_image->depth() != destination_surface->format->BitsPerPixel ) {
        SDL_Log("Wrong source image/destination surface depth %zu != %d", source_image->depth(), destination_surface->format->BitsPerPixel );
        assert(false && "Wrong image bit depth combination");
    }
    assert( destination_surface->format->format == SDL_PIXELFORMAT_RGBA32 && "Wrong source image/destination surface pixel format" );
    magick2surface(*source_image, destination_surface );
}

void Magickable::decode(void *dst, void* src) {
    auto source_surface = static_cast<SDL_Surface*>(src);
    auto destination_image = static_cast<Magick::Image*>(dst);
    surface2image( source_surface, *destination_image );
}

Image* Magickable::Allocate(SDL_Surface* reference, double scale) {
    auto geom = Geometry(reference->w * scale, reference->h * scale);
    auto image = new Image(geom, Color(0, 0, 0, 0) );
    image->depth( 32 );
    image->channelDepth( AllChannels, 32 );
    image->colorSpace( sRGBColorspace );
    image->alphaChannel(SetAlphaChannel );
    image->modifyImage();
    return image;
}

void Magickable::blitScaledMagick(SDL_Surface *dst, SDL_Surface *src) {

    Image* image = Allocate(src);
    decode(image, src);
    encode(dst, image);
    delete image;

}

void Magickable::blitScaled(SDL_Surface *dst, SDL_Surface *src) {
    size_t src_size = Waveable::conversion_size(src);
    size_t dst_size = Waveable::conversion_size(dst);
    if( memcmp(&src->clip_rect, &dst->clip_rect, sizeof(SDL_Rect)) == 0 ) {
        memcpy( dst->pixels , src->pixels, src_size * sizeof(Uint32) );
    } else {
        try {
            assert( dst->w > 0 && dst->h > 0 && src->w > 0 && src->h >0 );
            //auto data = new Uint32[size];
            auto source = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
            int bands = 4;
            VImage in = VImage::new_from_memory(source->pixels, src_size * bands, src->w, src->h, bands, VIPS_FORMAT_UCHAR);
            if ( src->w > dst->w ) {
                //shrink
                double scale = (double) src->w / dst->w;
                //SDL_Log("Scaling down ...");
                VImage io = in.reduce(scale, scale, VImage::option()->set("kernel", VIPS_KERNEL_NEAREST ) );
                //SDL_Log("Image info: %d, %d", io.width(), io.height() );
                //SDL_Log("Source info: %d, %d", source->w, source->h );
                //SDL_Log("Band format %d", io.format() );
                //SDL_Log("Bands %d", io.bands() );

                memcpy( dst->pixels, io.data(), dst_size * bands );
                //io.write_to_file("test_vimage.bmp");
            } else {
                //resize
                double scale = (double) dst->w / src->w;
                if(scale == 1) {
                    SDL_Log("No scale required");
                    memcpy(dst->pixels, src->pixels, dst_size * bands );
                } else {
                   // SDL_Log("Scaling up ...");
                    VImage io = in.resize(scale, VImage::option()->set("kernel", VIPS_KERNEL_NEAREST ));
                    memcpy( dst->pixels, io.data(), dst_size * bands );
                }

            }

            SDL_FreeSurface(source);

        } catch (VError &e) {
            SDL_Log("Cannot load image from memory: %s", e.what());
            assert(false && "Image loading failed");
        }
    }

}


void Magickable::verticalize( SDL_Surface *dst, SDL_Surface *src ) {
    int pos = 0;

    for (int x = 0; x < src->w; x++)
        for (int y = 0; y < src->h; y++) {
            int dx = pos % src->w;
            int dy = pos / src->w;
            Pixelable::copy32( dst, src, x, y, dx, dy );
            ++pos;
        }
}

void Magickable::deverticalize(SDL_Surface *dst, SDL_Surface *src) {
    int pos = 0;

    for ( int x = 0; x < src->w; x++ )
        for ( int y = 0; y < src->h; y++ ) {
            int dx = pos % src->w;
            int dy = pos / src->w;
            Pixelable::copy32( dst, src, dx, dy, x, y );
            ++pos;
        }
}

#endif //SDL_CRT_FILTER_MAGICKABLE_HPP
