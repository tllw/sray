#ifndef IMG_H_
#define IMG_H_

#include "color.h"

/** Floating point color image. Used for the framebuffer and texture maps. */
class Image {
public:
	Color *pixels;
	int xsz, ysz;

	Image();
	~Image();

	bool create(int xsz, int ysz);
	void destroy();

	bool load(const char *fname);
	bool save(const char *fname, bool alpha = true) const;

	bool set_pixels(int xsz, int ysz, const Color *pixels);
	Color *get_pixels() const;

	int get_width() const;
	int get_height() const;
};

#endif	// IMG_H_
