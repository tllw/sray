#include <stdlib.h>
#include <imago.h>
#include "img.h"

Image::Image()
{
	pixels = 0;
}

Image::~Image()
{
	destroy();
}

bool Image::create(int xsz, int ysz)
{
	Color *newpix;

	if(!(newpix = (Color*)malloc(xsz * ysz * sizeof *newpix))) {
		return false;
	}
	free(this->pixels);
	this->pixels = newpix;
	this->xsz = xsz;
	this->ysz = ysz;

	return true;
}

void Image::destroy()
{
	free(pixels);
}

bool Image::load(const char *fname)
{
	set_image_option(IMG_OPT_FLOAT, 1);
	set_image_option(IMG_OPT_ALPHA, 1);

	Color *newpix;
	if(!(newpix = (Color*)load_image(fname, &xsz, &ysz))) {
		return false;
	}

	free(pixels);
	pixels = newpix;
	return true;
}

bool Image::save(const char *fname, bool alpha) const
{
	set_image_option(IMG_OPT_ALPHA, alpha ? 1 : 0);
	set_image_option(IMG_OPT_FLOAT, 1);

	if(save_image(fname, pixels, xsz, ysz, IMG_FMT_AUTO) == -1) {
		return false;
	}
	return true;
}

bool Image::set_pixels(int xsz, int ysz, const Color *pixels)
{
	if(!create(xsz, ysz)) {
		return false;
	}
	
	memcpy(this->pixels, pixels, xsz * ysz * sizeof *pixels);
	return true;
}

Color *Image::get_pixels() const
{
	return pixels;
}

int Image::get_width() const
{
	return xsz;
}

int Image::get_height() const
{
	return ysz;
}
