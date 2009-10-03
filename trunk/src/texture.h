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
