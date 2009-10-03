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
