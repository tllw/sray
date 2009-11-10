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
#ifndef IMG_H_
#define IMG_H_

#include "color.h"

/** Floating point color image. Used for the framebuffer and texture maps. */
class Image {
public:
	float *pixels;
	int xsz, ysz;
	bool manage_pixels;

	Image();
	~Image();

	bool create(int xsz, int ysz, float *newpix = 0);
	void destroy();

	bool load(const char *fname);
	bool save(const char *fname, bool alpha = true) const;

	bool set_pixels(int xsz, int ysz, const float *pixels);
	float *get_pixels() const;

	int get_width() const;
	int get_height() const;
};

#endif	// IMG_H_
