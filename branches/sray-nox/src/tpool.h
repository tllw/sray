/*
This file is part of the s-ray renderer <http://code.google.com/p/sray>.
Copyright (C) 2009 John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
