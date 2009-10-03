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
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <alloca.h>
#include <sys/time.h>
#include "opt.h"
#include "img.h"
#include "wsys.h"
#include "scene.h"
#include "sphere.h"
#include "mesh.h"
#include "cylinder.h"
#include "tpool.h"
#include "octree.h"

using namespace std;

bool init(int xsz, int ysz);
void keyb(int key, int state);
void render(void);
void render_block(struct block *blk);
void block_done(struct block *blk);
void read_func(int fd);
void update_block(struct block *blk);

Image fb;
int pfd[2];

Scene *scn;
Camera *cam;
struct timeval tv0;

static double frame_time, frame_interval;
static int frame_idx;

bool dbg_sample_hist;
bool use_octree = true;

#ifndef NO_DBG_IMG
// debugging framebuffers
Image dbg_img_hist;
#endif

int main(int argc, char **argv)
{
	if(getenv("DBG_BRUTE_FORCE")) {
		use_octree = false;
	}

	if(parse_opt(argc, argv) == -1) {
		return 1;
	}

	if(!init(opt.width, opt.height)) {
		return 1;
	}

	gettimeofday(&tv0, 0);
	render();

	wsys_main_loop();
	stop_frame();
	wait_frame();
	wsys_destroy();

	return 0;
}

bool init(int xsz, int ysz)
{
	if(opt.interactive) {
		if(wsys_init(xsz, ysz) == -1) {
			return false;
		}
		wsys_set_title("s-ray");
	}

	if(pipe(pfd) == -1) {
		perror("failed to open self-pipe");
		return false;
	}

	if(opt.interactive) {
		wsys_set_key_func(keyb);
		wsys_set_read_func(pfd[0], read_func);
	}

	fb.create(xsz, ysz);

#ifndef NO_DBG_IMG
	dbg_img_hist.create(xsz, ysz);
#endif

	if(init_threads(opt.threads) == -1) {
		fprintf(stderr, "failed to create thread pool\n");
		return false;
	}

	frame_time = (double)opt.time_start;
	frame_interval = 1000.0 / (double)opt.fps;

	// --- create scene ---
	scn = new Scene();

	bool res;
	if(opt.scenefile) {
		res = scn->load(opt.scenefile);
	} else {
		res = scn->load(stdin);
	}
	if(!res) {
		return false;
	}

	if(!(cam = scn->get_camera())) {
		fprintf(stderr, "no camera found, adding a default camera\n");
		cam = new Camera;
		scn->set_camera(cam);
	}

	if(use_octree) {
		int t0 = (int)floor(frame_time) - cam->get_shutter() / 2;
		int t1 = t0 + cam->get_shutter();

		if(opt.verb) {
			printf("constructing octree (%d, %d)...\n", t0, t1);
		}

		scn->build_tree(t0, t1);
	}

	if(opt.caust_photons || opt.gi_photons) {
		printf("building photon maps\n");
		scn->build_photon_maps();
	}

	// restore photon dumps if available
	char dumpfile[64];
	sprintf(dumpfile, "%s.cdump", opt.scenefile ? opt.scenefile : "scene");
	PhotonMap *cmap = scn->get_caust_map();
	cmap->restore(dumpfile);
	opt.caust_photons = cmap->size();

	sprintf(dumpfile, "%s.gdump", opt.scenefile ? opt.scenefile : "scene");
	PhotonMap *gmap = scn->get_gi_map();
	gmap->restore(dumpfile);
	opt.gi_photons = gmap->size();

	return true;
}

void keyb(int key, int state)
{
	switch(key) {
	case 27:
		wsys_exit_main_loop();
		putchar('\n');
		break;

	case 's':
		printf("saving image: output.png ... ");
		fflush(stdout);
		printf("%s\n", fb.save("output.png", false) ? "done" : "failed");
		break;

#ifndef NO_DBG_IMG
	case 'h':
		printf("saving sample histogram ... ");
		fflush(stdout);
		printf("%s\n", dbg_img_hist.save("hist.png", false) ? "done" : "failed");
		break;
#endif

	case 'c':
		{
			PhotonMap *pmap = scn->get_caust_map();
			if(!pmap) {
				fprintf(stderr, "there aren't any caustics photons to dump\n");
				break;
			}
			char fname[64];
			sprintf(fname, "%s.cdump", opt.scenefile ? opt.scenefile : "scene");
			printf("dumping caustics photons ...\n");
			pmap->dump(fname);
		}
		break;

	case 'g':
		{
			PhotonMap *pmap = scn->get_gi_map();
			if(!pmap) {
				fprintf(stderr, "there aren't any global photons to dump\n");
				break;
			}
			char fname[64];
			sprintf(fname, "%s.gdump", opt.scenefile ? opt.scenefile : "scene");
			printf("dumping gi photons\n");
			pmap->dump(fname);
		}
		break;

	default:
		break;
	}
}

void render(void)
{
	for(int i=0; i<fb.xsz * fb.ysz; i++) {
		fb.pixels[i] = Color(0, 0, 0, 0);
	}

	wsys_clear();

	if(opt.verb) {
		printf("rendering frame %d (of %d) ", frame_idx + 1, opt.num_frames);
		fflush(stdout);
	}

	set_block_render_func(render_block);
	set_block_done_func(block_done);

	start_frame(opt.blk_sz, BLK_PRI_GAUSSIAN);
}

static float variance(Color *samples, const Color &sum, int num, float rcp_num)
{
	Color mean = sum * rcp_num;

	float sqdist = 0.0;
	for(int i=0; i<num; i++) {
		sqdist += (mean - samples[i]).length_sq();
	}
	return sqdist * rcp_num;
}

void render_block(struct block *blk)
{
	int ftime = (int)floor(frame_time);

	Color *subpix = (Color*)alloca(opt.max_samples * sizeof *subpix);
	float *rcp_lut = (float*)alloca((1 + opt.max_samples) * sizeof *rcp_lut);

	for(int i=1; i<=opt.max_samples; i++) {
		rcp_lut[i] = 1.0 / (float)i;
	}

	int xsz = fb.get_width();
	int start_offs = blk->y * xsz + blk->x;
	Color *img = fb.get_pixels() + start_offs;

#ifndef NO_DBG_IMG
	Color *hist = dbg_img_hist.get_pixels() + start_offs;
#endif

	for(int y=0; y<blk->ysz; y++) {
		for(int x=0; x<blk->xsz; x++) {
			int i = 0;
			
			while(i < opt.max_samples) {
				Ray ray = cam->get_primary_ray(x + blk->x, y + blk->y, i, ftime);
				ray.iter = opt.iter;
				subpix[i] = scn->trace_ray(ray);
				img[x] += subpix[i];
				i++;

				if(i >= opt.min_samples && i > 1) {
					float v = variance(subpix, img[x], i, rcp_lut[i]);
					if(v < opt.max_var) {
						break;
					}
				}
			}
			
			img[x] *= rcp_lut[i];

#ifndef NO_DBG_IMG
			hist[x] = (float)i / (float)opt.max_samples;
#endif
		}
		img += xsz;

#ifndef NO_DBG_IMG
		hist += xsz;
#endif
	}
}

void block_done(struct block *blk)
{
	putchar('.');
	fflush(stdout);
	write(pfd[1], blk, sizeof *blk);
}

void read_func(int fd)
{
	struct block blk;

	read(fd, &blk, sizeof blk);

	if(opt.interactive) {
		update_block(&blk);
	}

	int done, max, p;
	p = get_work_status(&done, &max);
	if(done == max - 1) {
		wait_frame();

		if(opt.verb) {
			printf(" done\n");
		}

		if(opt.num_frames > 1) {
			char buf[32];

			sprintf(buf, "frame%04d.png", frame_idx);
			fb.save(buf);
		} else if(!opt.interactive) {
			fb.save("output.png");
		}

		if(frame_idx == opt.num_frames - 1) {
			struct timeval tv;
			gettimeofday(&tv, 0);

			unsigned int msec = (tv.tv_sec - tv0.tv_sec) * 1000 + (tv.tv_usec - tv0.tv_usec) / 1000;
			unsigned int sec = msec / 1000;
			msec -= sec * 1000;
			unsigned int min = sec / 60;
			sec -= min * 60;
			printf("rendering time: %u min, %u sec, %u msec\n", min, sec, msec);
		} else {
			frame_time += frame_interval;
			frame_idx++;

			wsys_clear();

			render();
		}
	}
}

static inline int clamp(int x, int a, int b)
{
	return x < a ? a : (x > b ? b : x);
}

void update_block(struct block *blk)
{
	int x, y, xsz = fb.get_width();
	int scanline_skip = xsz - blk->xsz;

	Vector4 *src = fb.get_pixels() + blk->y * xsz + blk->x;
	unsigned char *block_buf = (unsigned char*)alloca(blk->xsz * blk->ysz * 4);
	unsigned char *dest = block_buf;

	for(y=0; y<blk->ysz; y++) {
		for(x=0; x<blk->xsz; x++) {
			*dest++ = clamp(src->x * 255.0, 0, 255);
			*dest++ = clamp(src->y * 255.0, 0, 255);
			*dest++ = clamp(src->z * 255.0, 0, 255);
			*dest++ = clamp(src->w * 255.0, 0, 255);
			src++;
		}
		src += scanline_skip;
	}

	wsys_updrect(blk->x, blk->y, blk->xsz, blk->ysz, 0, 0, block_buf, blk->xsz, blk->ysz);
	wsys_redisplay_area(blk->x, blk->y, blk->xsz, blk->ysz);
}
