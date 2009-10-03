#ifndef BLOCK_H_
#define BLOCK_H_

/** block represents a rectangular area of the framebuffer.
 * During rendering, the framebuffer is split into blocks, which are added to
 * the work queue and are processed in turn by the rendering threads.
 */
struct block {
	/* image area rect */
	int x, y;
	int xsz, ysz;

	int bx, by;	/* block coordinates */

	/*struct pixel *bptr;*/

	int pri;	/* block priority */

	struct block *next, *tail;
	int len;
};

#ifdef __cplusplus
extern "C" {
#endif

void init_block(struct block *blk, int bx, int by, int bsz);
struct block *get_block(int bx, int by, int bsz);
void free_block(struct block *blk);
void delete_bpool(void);

#ifdef __cplusplus
}
#endif

#endif	/* BLOCK_H_ */
