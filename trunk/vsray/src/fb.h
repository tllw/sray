#ifndef FB_H_
#define FB_H_

#include <semaphore.h>

#ifdef __GNUC__
#define PACKED  __attribute__((packed))
#else
#error "define PACKED for your compiler"
#endif

/* XXX this struct must be kept in sync with struct fbheader in
 * ../sray/src/fb.c
 */
struct framebuffer {
	int width, height;
	int rbits, gbits, bbits, abits;

	int progr, max_progr;

	float pixels[1];
} PACKED;

struct framebuffer *map_framebuf(int pid);
void unmap_framebuf(struct framebuffer *fb);

#endif	/* FB_H_ */
