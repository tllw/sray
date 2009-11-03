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

	long t0, t1;	/* frame time (interval) */

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

int blkcmp(const void *a, const void *b);

#ifdef __cplusplus
}
#endif

#endif	/* BLOCK_H_ */
