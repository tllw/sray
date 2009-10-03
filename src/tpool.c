#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <vmath.h>
#include "tpool.h"
#include "block.h"
#include "opt.h"

static struct block *merge_sort(struct block *head);
static struct block *merge(struct block *h1, struct block *h2);

static struct block *blist, *btail;
static int work_left, block_count;
static pthread_cond_t work_pending_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t work_pending_mut = PTHREAD_MUTEX_INITIALIZER;

static int frame_done = 1;
static pthread_cond_t done_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t done_mut = PTHREAD_MUTEX_INITIALIZER;

static void (*blk_render_func)(struct block*);
static void (*blk_done_func)(struct block*);


int init_threads(int threads)
{
	int i;

	for(i=0; i<threads; i++) {
		pthread_t pt;
		if(pthread_create(&pt, 0, worker, 0) != 0) {
			perror("failed to create thread");
			return -1;
		}
	}
	return 0;
}

/* start rendering a frame... break up in blocks and append to the work queue */
void start_frame(int blk_sz, int calc_prior)
{
	int i, j, xblocks, yblocks, bcount;
	struct block *blk, *bset = 0, *bset_tail = 0;

	if(!blk_render_func) {
		fprintf(stderr, "start_frame: nothing to be done, no block render function specified!\n");
		return;
	}

	xblocks = ((opt.width << 8) / blk_sz + 255) >> 8;
	yblocks = ((opt.height << 8) / blk_sz + 255) >> 8;
	bcount = xblocks * yblocks;

	for(i=0; i<xblocks; i++) {
		for(j=0; j<yblocks; j++) {
			double dx, dy;
			
			if(!(blk = get_block(i, j, blk_sz))) {
				perror("start_frame failed");
				return;
			}
			if(calc_prior != BLK_FIFO) {
				dx = 2.0 * (double)(blk->x + blk->xsz / 2.0) / (double)opt.width - 1.0;
				dy = 2.0 * (double)(blk->y + blk->ysz / 2.0) / (double)opt.height - 1.0;
				blk->pri = (int)(gaussian(dx * dx + dy * dy, 0.0, 0.3) * 100.0);
			}

			blk->next = bset;
			bset = blk;

			if(i == 0 && j == 0) {
				bset_tail = blk;
			}
		}
	}
	if(calc_prior != BLK_FIFO) {
		bset->tail = bset_tail;
		bset->len = bcount;
		bset = merge_sort(bset);
	}
	
	frame_done = 0;

	pthread_mutex_lock(&work_pending_mut);
	block_count = bcount;

	if(btail) {
		btail->next = bset;
		btail = bset;
	} else {
		blist = btail = bset;
	}
	while(btail->next) {
		btail = btail->next;
	}
	work_left += block_count;
	/* wake up all workers: there's work to do, you lazy bastards! */
	pthread_cond_broadcast(&work_pending_cond);
	pthread_mutex_unlock(&work_pending_mut);
}

/* stop the current rendering by removing all work items from the queue */
void stop_frame(void)
{
	pthread_mutex_lock(&work_pending_mut);
	while(blist) {
		struct block *blk = blist;
		blist = blist->next;
		free_block(blk);
		work_left--;
	}
	btail = 0;
	pthread_mutex_unlock(&work_pending_mut);
}

/* finish rendering a block */
void finish_block(struct block *blk)
{
	if(!--work_left) {
		/*pthread_mutex_lock(&done_mut);*/
		frame_done = 1;
		pthread_cond_signal(&done_cond);
		/*pthread_mutex_unlock(&done_mut);*/
	}

	if(blk_done_func) {
		blk_done_func(blk);
	}
	free_block(blk);
}

/* each worker thread runs this */
void *worker(void *cls)
{
	for(;;) {
		pthread_mutex_lock(&work_pending_mut);
		if(blist) {
			struct block *blk;

			blk = blist;
			blist = blist->next;
			if(!blist) {
				btail = 0;
			}
			pthread_mutex_unlock(&work_pending_mut);

			blk_render_func(blk);
			finish_block(blk);	/* this will send an appropriate expose event */
			
		} else {
			while(!blist) {
				pthread_cond_wait(&work_pending_cond, &work_pending_mut);
			}
			pthread_mutex_unlock(&work_pending_mut);	/* XXX maybe I can avoid this unlock somehow */
		}
	}

	return 0;
}

void wait_frame(void)
{
	pthread_mutex_lock(&done_mut);
	while(!frame_done) {
		pthread_cond_wait(&done_cond, &done_mut);
	}
	pthread_mutex_unlock(&done_mut);
}

int is_frame_done(void)
{
	int res;
	/*pthread_mutex_lock(&done_mut);*/
	res = frame_done;
	/*pthread_mutex_unlock(&done_mut);*/
	return res;
}

int get_work_status(int *bdone, int *bmax)
{
	int bcount, done;

	pthread_mutex_lock(&work_pending_mut);
	bcount = block_count;
	done = bcount - work_left;
	pthread_mutex_unlock(&work_pending_mut);

	if(bdone) *bdone = done;
	if(bmax) *bmax = bcount;

	return 100 * done / (bcount - 1);
}

void set_block_render_func(void (*func)(struct block*))
{
	blk_render_func = func;
}

void set_block_done_func(void (*func)(struct block*))
{
	blk_done_func = func;
}


static struct block *merge_sort(struct block *head)
{
	int i, mid;
	struct block *left, *right, *tmp;

	if(!head || !head->next) {
		return head;
	}

	mid = head->len / 2;
	tmp = head;

	for(i=0; i<mid-1; i++) {
		tmp = tmp->next;
	}
	
	right = tmp->next;
	right->tail = head->tail;
	right->len = head->len - (i + 1);

	tmp->next = 0;
	left = head;
	left->tail = tmp;
	left->len = (i + 1);

	left = merge_sort(left);
	right = merge_sort(right);
	return merge(left, right);
}

static struct block *merge(struct block *h1, struct block *h2)
{
	struct block *tmp, *res = 0;

	if(!h1) return h2;
	if(!h2) return h1;

	while(h1 && h2) {
		if(h1->pri > h2->pri) {
			tmp = h1;
			h1 = h1->next;
			if(h1) {
				h1->tail = tmp->tail;
				h1->len = tmp->len - 1;
			}
		} else {
			tmp = h2;
			h2 = h2->next;
			if(h2) {
				h2->tail = tmp->tail;
				h2->len = tmp->len - 1;
			}
		}
		tmp->next = 0;

		if(res) {
			res->tail->next = tmp;
			res->tail = tmp;
			res->len++;
		} else {
			res = tmp;
			res->tail = tmp;
			res->len = 1;
		}
	}

	tmp = 0;
	if(h1) tmp = h1;
	if(h2) tmp = h2;

	if(tmp) {
		res->tail->next = tmp;
		res->tail = tmp;
		res->len += tmp->len;
	}
	return res;
}
