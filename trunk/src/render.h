#ifndef RENDER_H_
#define RENDER_H_

#include "scene.h"

bool rend_init(Image *fb);
void render(long msec);

#endif	// RENDER_H_
