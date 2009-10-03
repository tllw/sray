#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include "block.h"
#include "opt.h"

#define MIN(a, b)	((a) < (b) ? (a) : (b))

static struct block *bpool;
static pthread_mutex_t bpool_mut = PTHREAD_MUTEX_INITIALIZER;

void init_block(struct block *blk, int bx, int by, int bsz)
{
	blk->x = bx * bsz;
	blk->y = by * bsz;
	blk->xsz = MIN(bsz, opt.width - blk->x);
	blk->ysz = MIN(bsz, opt.height - blk->y);
	blk->bx = bx;
	blk->by = by;
	/*blk->bptr = fb->pix + blk->y * fb->width + blk->x;*/
	blk->pri = 0;
	blk->next = 0;
}

struct block *get_block(int bx, int by, int bsz)
{
	struct block *blk;

	pthread_mutex_lock(&bpool_mut);
	if(bpool) {
		blk = bpool;
		bpool = blk->next;
	} else {
		if(!(blk = malloc(sizeof *blk))) {
			return 0;
		}
	}
	pthread_mutex_unlock(&bpool_mut);

	init_block(blk, bx, by, bsz);
	return blk;
}

void free_block(struct block *blk)
{
	pthread_mutex_lock(&bpool_mut);
	blk->next = bpool;
	bpool = blk;
	pthread_mutex_unlock(&bpool_mut);
}

void delete_bpool(void)
{
	pthread_mutex_lock(&bpool_mut);
	while(bpool) {
		struct block *b = bpool;
		bpool = bpool->next;
		free(b);
	}
	pthread_mutex_unlock(&bpool_mut);
}
