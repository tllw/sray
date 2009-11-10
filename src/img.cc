/*
This file is part of the s-ray renderer <http://code.google.com/p/sray>.
Copyright (C) 2009 John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <imago.h>
#include "img.h"

Image::Image()
{
	pixels = 0;
	manage_pixels = false;
}

Image::~Image()
{
	destroy();
}

bool Image::create(int xsz, int ysz, float *newpix)
{
	bool should_delete = manage_pixels && pixels;

	if(!newpix) {
		try {
			newpix = new float[4 * xsz * ysz];
		}
		catch(std::bad_alloc e) {
			return false;
		}
		manage_pixels = true;
	}

	if(should_delete) {
		delete [] pixels;
	}
	this->pixels = newpix;
	this->xsz = xsz;
	this->ysz = ysz;

	return true;
}

void Image::destroy()
{
	if(manage_pixels) {
		delete [] pixels;
	}
}

bool Image::load(const char *fname)
{
	set_image_option(IMG_OPT_FLOAT, 1);
	set_image_option(IMG_OPT_ALPHA, 1);

	float *imgbuf;
	if(!(imgbuf = (float*)load_image(fname, &xsz, &ysz))) {
		return false;
	}

	float *newpix = new float[4 * xsz * ysz];
	memcpy(newpix, imgbuf, 4 * xsz * ysz * sizeof *newpix);
	free_image(imgbuf);

	delete [] pixels;
	pixels = newpix;
	return true;
}

bool Image::save(const char *fname, bool alpha) const
{
	set_image_option(IMG_OPT_ALPHA, alpha ? 1 : 0);
	set_image_option(IMG_OPT_FLOAT, 1);

	return save_image(fname, pixels, xsz, ysz, IMG_FMT_AUTO) != -1;
}

bool Image::set_pixels(int xsz, int ysz, const float *pixels)
{
	if(!create(xsz, ysz)) {
		return false;
	}
	
	memcpy(this->pixels, pixels, xsz * ysz * 4 * sizeof *pixels);
	return true;
}

float *Image::get_pixels() const
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
