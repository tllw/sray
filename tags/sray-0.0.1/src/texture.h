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
#ifndef TEXTURE_H_
#define TEXTURE_H_

#include "img.h"
#include "anim.h"

enum TexFilter {
	TEX_FILTER_NEAREST,
	TEX_FILTER_LINEAR
};

enum TexWrap {
	TEX_WRAP_CLAMP,
	TEX_WRAP_REPEAT
};

/** Textures derive from XFormNode to provide the ability to transform and
 * animate texture coordinates.
 */
class Texture : public XFormNode {
private:
	Image img;
	TexFilter filter;
	TexWrap wrap;

	Color get_texel(int tx, int ty) const;

public:
	Texture();

	bool load(const char *name);

	void set_filtering(TexFilter filter);
	TexFilter get_filtering() const;

	void set_wrapping(TexWrap wrap);
	TexWrap get_wrapping() const;

	/** This function takes a texture coordinate set and returns a Color,
	 * taking into account the wrapping and filtering properties.
	 */
	Color lookup(const Vector2 &tc, unsigned int time = 0) const;
};

#endif	// TEXTURE_H_
