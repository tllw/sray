#ifndef FB_H_
#define FB_H_

#ifdef __cplusplus
extern "C" {
#endif

void *alloc_framebuf(int xsz, int ysz);
void free_framebuf(void *fb);

#ifdef __cplusplus
}
#endif

#endif	/* FB_H_ */
