#ifndef TPOOL_H_
#define TPOOL_H_

#include "block.h"

enum {
	BLK_FIFO,
	BLK_PRI_GAUSSIAN
};

#ifdef __cplusplus
extern "C" {
#endif

int init_threads(int threads);
void start_frame(int blk_sz, int calc_prior);
void stop_frame(void);
void *worker(void *cls);

void wait_frame(void);
int is_frame_done(void);
int get_work_status(int *bdone, int *bmax);

void set_block_render_func(void (*func)(struct block*));
void set_block_done_func(void (*func)(struct block*));

#ifdef __cplusplus
}
#endif

#endif	/* TPOOL_H_ */
