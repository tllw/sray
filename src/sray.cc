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

#include <stdio.h>
#include "img.h"
#include "scene.h"
#include "render.h"
#include "opt.h"
#include "tpool.h"
#include "timer.h"
#include "block.h"
#include "datapath.h"

static bool init();
static void cleanup();
static void output(int fnum);

static Scene *scn;
static Image fb;

int main(int argc, char **argv)
{
	if(parse_opt(argc, argv) == -1) {
		return 1;
	}
	double frame_interval = 1000.0 / (double)opt.fps;
	double frame_time = (double)opt.time_start;

	if(!init()) {
		return 1;
	}

	unsigned long start = get_msec();

	// render all frames
	for(int i=0; i<opt.num_frames; i++) {
		if(!QUIET) {
			printf("starting frame %d\n", i);
		}

		render(frame_time);
		output(i);

		frame_time += frame_interval;
	}

	if(!QUIET) {
		unsigned long msec, sec, min;
		
		msec = get_msec() - start;
		sec = msec / 1000;
		msec -= sec * 1000;
		min = sec / 60;
		sec -= min * 60;
		printf("rendering time: %lu min, %lu sec, %lu msec\n", min, sec, msec);
	}

	// drop dead
	cleanup();
	return 0;
}

static bool init()
{
	// create the framebuffer
	fb.create(opt.width, opt.height);

	// load scene
	scn = new Scene;
	bool res;
	if(opt.scenefile) {
		// if loading from a file, add that file's dir into the data search path
		char *lastslash, *fdir;

		fdir = (char*)alloca(strlen(opt.scenefile) + 1);
		strcpy(fdir, opt.scenefile);

		if((lastslash = strrchr(fdir, '/'))) {
			*lastslash = 0;
			add_path(fdir);
		}

		res = scn->load(opt.scenefile);
	} else {
		// otherwise add the current directory
		add_path(".");
		res = scn->load(stdin);
	}
	if(!res) {
		return false;
	}

	// if the scene file doesn't have a camera, add a default one...
	if(!scn->get_camera()) {
		fprintf(stderr, "no camera found, adding a default camera\n");
		Camera *cam = new Camera;
		scn->set_camera(cam);
	}

	if(!rend_init(&fb)) {
		return false;
	}

	return true;
}

static void cleanup()
{
	delete_bpool();
	delete scn;
}

static void output(int fnum)
{
	char fname[32];

	if(opt.num_frames > 1) {
		char buf[32];

		sprintf(buf, "frame%04d.png", fnum);
	} else {
		strcpy(fname, "output.png");
	}

	if(!fb.save(fname)) {
		fprintf(stderr, "failed to write file: %s\n", fname);
	}
}
