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
#include "texture.h"

Texture::Texture()
{
	filter = TEX_FILTER_NEAREST;
	wrap = TEX_WRAP_REPEAT;
}

bool Texture::load(const char *name)
{
	return img.load(name);
}

void Texture::set_filtering(TexFilter filter)
{
	this->filter = filter;
}

TexFilter Texture::get_filtering() const
{
	return filter;
}

void Texture::set_wrapping(TexWrap wrap)
{
	this->wrap = wrap;
}

TexWrap Texture::get_wrapping() const
{
	return wrap;
}

#define CLAMP(x, lo, hi)	((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

Color Texture::lookup(const Vector2 &tc, unsigned int time) const
{
	Vector4 tc4(tc.x, tc.y, 0.0, 1.0);
	tc4.transform(get_xform_matrix(time));

	int tx = tc4.x * img.xsz;
	int ty = tc4.y * img.ysz;

	switch(wrap) {
	case TEX_WRAP_CLAMP:
		tx = CLAMP(tx, 0, img.xsz);
		ty = CLAMP(ty, 0, img.ysz);
		break;

	case TEX_WRAP_REPEAT:
		tx %= img.xsz;
		ty %= img.ysz;
		break;
	}

	return img.pixels[ty * img.xsz + tx];
}
