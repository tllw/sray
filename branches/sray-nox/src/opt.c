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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "opt.h"

struct options opt;

enum {
	OPT_UNKNOWN,

	OPT_SIZE,
	OPT_SAMPLES,
	OPT_VARIANCE,
	OPT_MIN_ENERGY,
	OPT_SHADOW_SAMPLES,
	OPT_DIFFUSE_SAMPLES,
	OPT_THREADS,
	OPT_BLOCKSIZE,
	OPT_ITER,
	OPT_CAUST_PHOTONS,
	OPT_GI_PHOTONS,
	OPT_GATHER_DIST,
	OPT_PHOTON_ENERGY,
	OPT_FPS,
	OPT_TRANGE,
	OPT_MBLUR,
	OPT_SHUTTER,
	OPT_QUIET,
	OPT_VERBOSE,
	OPT_BACKEND,
	OPT_SOCT_MAX_DEPTH,
	OPT_SOCT_MAX_ITEMS,
	OPT_MOCT_MAX_DEPTH,
	OPT_MOCT_MAX_ITEMS,
	OPT_HELP
};

static struct {
	int opt;
	char sopt;
	const char *lopt;
	const char *desc;
} options[] = {
	{OPT_SIZE,			's', "size",		"output image size: wxh"},
	{OPT_SAMPLES,		'r', "rays",		"rays per pixel: rays[-maxrays]"},
	{OPT_VARIANCE,		'd', "variance",	"maximum subpixel variance"},
	{OPT_MIN_ENERGY,	'e', "minenergy",	"ray energy threshold, recursion stops if it is reached"},
	{OPT_SHADOW_SAMPLES, 0, "shadowrays",	"number of shadow rays for area lights"},
	{OPT_DIFFUSE_SAMPLES, 0, "diffuserays", "number of diffuse rays to spawn for gi"},
	{OPT_THREADS,		't', "threads",		"number of worker threads to spawn"},
	{OPT_BLOCKSIZE,		'b', "blocksz",		"rendering block dimensions"},
	{OPT_ITER,			'i', "iter",		"max recursion depth"},
	{OPT_CAUST_PHOTONS,	'c', "cphot",		"number of caustics photons to use"},
	{OPT_GI_PHOTONS,	'g', "gphot",		"number of global illumination photons to use"},
	{OPT_GATHER_DIST,	0, "gatherdisc",	"radius of the photon gathering disc (rel. scene size)"},
	{OPT_PHOTON_ENERGY, 0, "photonenergy",	"photon energy (multiplier)"},
	{OPT_FPS,			'f', "fps",			"animation frames per second (24)"},
	{OPT_TRANGE,		'a', "range",		"animation time range"},
	{OPT_MBLUR,			'm', "mblur",		"enable motion blur"},
	{OPT_SHUTTER,		'u', "shutter",		"shutter speed in milliseconds"},
	{OPT_SOCT_MAX_DEPTH, 0, "soctdepth",	"scene octree: max tree depth"},
	{OPT_SOCT_MAX_ITEMS, 0, "soctitems",	"scene octree: max items per node"},
	{OPT_MOCT_MAX_DEPTH, 0, "moctdepth",	"mesh octree: max tree depth"},
	{OPT_MOCT_MAX_ITEMS, 0, "moctitems",	"mesh octree: max items per node"},
	{OPT_QUIET,			'q', "quiet",		"run quietly (no output)"},
	{OPT_VERBOSE,		'v', "verbose",		"produce verbose output (add more for greater effect)"},
	{OPT_BACKEND,		0, "backend",		"run as a backend, emmiting only status info"},
	{OPT_HELP,			'h', "help",		"print usage information and exit"},
	{0, 0, 0, 0}
};

static void default_opt(void);
static void print_opt(void);
static int get_opt(const char *arg);

/* defined in tpool.cc */
int get_number_processors(void);

int parse_opt(int argc, char **argv)
{
	int i, j;

	default_opt();

	if(strstr(argv[0], "vsray")) {
		opt.verb = -1000;
	}

	for(i=1; i<argc; i++) {
		int opt_num = get_opt(argv[i]);

		switch(opt_num) {
		case OPT_SIZE:
			if(sscanf(argv[++i], "%dx%d", &opt.width, &opt.height) != 2) {
				fprintf(stderr, "%s must be followed by: WxH\n", argv[i - 1]);
				return -1;
			}
			break;

		case OPT_SAMPLES:
			{
				int n = sscanf(argv[++i], "%d-%d", &opt.min_samples, &opt.max_samples);
				if(n < 1) {
					fprintf(stderr, "%s must be followed by: rays[-max]\n", argv[i - 1]);
					return -1;
				} else if(n == 1) {
					opt.max_samples = opt.min_samples;
				}
			}
			break;

		case OPT_VARIANCE:
			{
				char *end;
				float maxv = strtod(argv[++i], &end);
				if(argv[i] == end) {
					fprintf(stderr, "%s must be followed by the maximum variance\n", argv[i - 1]);
					return -1;
				}
				opt.max_var = maxv;
			}
			break;

		case OPT_MIN_ENERGY:
			{
				char *end;
				float minen = strtod(argv[++i], &end);
				if(argv[i] == end) {
					fprintf(stderr, "%s must be followe by the minimum ray energy threshold\n", argv[i - 1]);
					return -1;
				}
				opt.min_energy = minen;
			}
			break;

		case OPT_SHADOW_SAMPLES:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the number of shadow rays per\n", argv[i - 1]);
				return -1;
			}
			opt.shadow_samples = atoi(argv[i]);
			break;

		case OPT_DIFFUSE_SAMPLES:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the number of diffuse rays to spawn for GI evaluation\n", argv[i - 1]);
				return -1;
			}
			opt.diffuse_samples = atoi(argv[i]);
			break;

		case OPT_THREADS:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the number of worker threads\n", argv[i - 1]);
				return -1;
			}
			opt.threads = atoi(argv[i]);
			break;

		case OPT_BLOCKSIZE:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the block size\n", argv[i - 1]);
				return -1;
			}
			opt.blk_sz = atoi(argv[i]);
			break;

		case OPT_ITER:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the maximum number of iterations\n", argv[i - 1]);
				return -1;
			}
			opt.iter = atoi(argv[i]);
			break;

		case OPT_CAUST_PHOTONS:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the number of caustics photons to use\n", argv[i - 1]);
				return -1;
			}
			opt.caust_photons = atoi(argv[i]);
			break;

		case OPT_GI_PHOTONS:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the number of global illumination photons to use\n", argv[i - 1]);
				return -1;
			}
			opt.gi_photons = atoi(argv[i]);
			break;

		case OPT_GATHER_DIST:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the radius of the photon gathering disc\n", argv[i - 1]);
				return -1;
			}
			opt.gather_dist = atof(argv[i]);
			break;

		case OPT_PHOTON_ENERGY:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the photon energy multiplier\n", argv[i - 1]);
				return -1;
			}
			opt.photon_energy = atof(argv[i]);
			break;

		case OPT_FPS:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the frames per second\n", argv[i - 1]);
				return -1;
			}
			opt.fps = atoi(argv[i]);
			break;

		case OPT_TRANGE:
			{
				int n = sscanf(argv[++i], "%d-%d", &opt.time_start, &opt.time_end);
				if(n < 1) {
					fprintf(stderr, "%s must be followed by: animation range start[-end]\n", argv[i - 1]);
					return -1;
				} else if(n == 1) {
					opt.time_end = opt.time_start;
				}
			}
			break;

		case OPT_MBLUR:
			opt.mblur = 1;
			break;

		case OPT_SHUTTER:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the shutter speed in milliseconds\n", argv[i - 1]);
				return -1;
			}
			opt.shutter = atoi(argv[i]);
			break;

		case OPT_SOCT_MAX_DEPTH:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the maximum scene octree depth\n", argv[i - 1]);
				return -1;
			}
			opt.scnoct_max_depth = atoi(argv[i]);
			break;

		case OPT_SOCT_MAX_ITEMS:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the maximum number of items in a scene octree node\n", argv[i - 1]);
				return -1;
			}
			opt.scnoct_max_items = atoi(argv[i]);
			break;

		case OPT_MOCT_MAX_DEPTH:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the maximum mesh octree depth\n", argv[i - 1]);
				return -1;
			}
			opt.meshoct_max_depth = atoi(argv[i]);
			break;

		case OPT_MOCT_MAX_ITEMS:
			if(!isdigit(argv[++i][0])) {
				fprintf(stderr, "%s must be followed by the maximum number of items in a mesh octree node\n", argv[i - 1]);
				return -1;
			}
			opt.meshoct_max_items = atoi(argv[i]);
			break;

		case OPT_QUIET:
			opt.verb--;
			break;

		case OPT_VERBOSE:
			opt.verb++;
			break;

		case OPT_BACKEND:
			opt.verb = -1000;
			break;

		case OPT_HELP:
			printf("%s options:\n", argv[0]);
			for(j=0; options[j].opt; j++) {
				fputs("  ", stdout);
				if(options[j].sopt) {
					printf("-%c, ", options[j].sopt);
				} else {
					fputs("    ", stdout);
				}
				if(options[j].lopt) {
					printf("-%-15s ", options[j].lopt);
				} else {
					fputs("                 ", stdout);
				}
				if(options[j].desc) {
					printf("%s\n", options[j].desc);
				} else {
					putchar('\n');
				}
			}
			exit(0);

		default:
			if(argv[i][0] == '-') {
				fprintf(stderr, "unknown option: %s. try -h for usage info.\n", argv[i]);
				return -1;
			}

			if(opt.scenefile) {
				fprintf(stderr, "unexpected option: %s. already got a scene file: %s\n", argv[i], opt.scenefile);
				return -1;
			}
			opt.scenefile = argv[i];
		}
	}

	if(!opt.threads) {
		if((opt.threads = get_number_processors()) <= 0) {
			opt.threads = 1;
		}
	}

	opt.num_frames = opt.fps * (opt.time_end - opt.time_start) / 1000;
	if(!opt.num_frames) {
		opt.num_frames = 1;
	}

	if(opt.verb > 0) {
		print_opt();
	}

	return 0;
}


static void default_opt(void)
{
	opt.width = 640;
	opt.height = 480;
	opt.min_samples = 1;
	opt.max_samples = 1;
	opt.max_var = 0.005;
	opt.min_energy = 0.0001;
	opt.shadow_samples = 1;
	opt.diffuse_samples = 1;
#ifndef NO_THREADS
	opt.threads = 0;
#else
	opt.threads = 1;
#endif
	opt.blk_sz = 64;
	opt.iter = 7;
	opt.caust_photons = opt.gi_photons = 0;
	opt.gather_dist = 0.001;
	opt.photon_energy = 300.0;
	opt.verb = 0;
	opt.fps = 30;
	opt.time_start = opt.time_end = 0;
	opt.mblur = 0;
	opt.shutter = 0;

	opt.meshoct_max_depth = 7;
	opt.meshoct_max_items = 14;

	opt.scnoct_max_depth = 5;
	opt.scnoct_max_items = 5;
}

static void print_opt(void)
{
	printf("render options\n--------------\n");
	printf("    image size: %dx%d\n", opt.width, opt.height);
	printf("       samples: %d-%d\n", opt.min_samples, opt.max_samples);
	printf("  max variance: %f\n", opt.max_var);
	printf("min ray energy: %f\n", opt.min_energy);
	printf("   shadow rays: %d\n", opt.shadow_samples);
	printf("  diffuse rays: %d\n", opt.diffuse_samples);
	printf("       threads: %d\n", opt.threads);
	printf("    block size: %d\n", opt.blk_sz);
	printf("    rec. depth: %d\n", opt.iter);
	printf("caust. photons: %d\n", opt.caust_photons);
	printf("    gi photons: %d\n", opt.gi_photons);
	printf("   gather disc: %f\n", opt.gather_dist);
	printf(" photon energy: %f\n", opt.photon_energy);
	printf("           fps: %d\n", opt.fps);
	printf("    frame time: %d-%d msec (%d frame(s))\n", opt.time_start, opt.time_end, opt.num_frames);
	printf("   motion blur: %s\n", opt.mblur ? "yes" : "no");
	if(opt.mblur) {
		if(!opt.shutter) {
			opt.shutter = 1000 / 30;
		}
		printf("       shutter: %d msec\n", opt.shutter);
	}
	printf(" scn octree max depth: %d\n", opt.scnoct_max_depth);
	printf(" scn octree max items: %d\n", opt.scnoct_max_items);
	printf("mesh octree max depth: %d\n", opt.meshoct_max_depth);
	printf("mesh octree max items: %d\n", opt.meshoct_max_items);
	putchar('\n');
}

static int get_opt(const char *arg)
{
	int i;

	if(arg[0] != '-') {
		return OPT_UNKNOWN;
	}

	for(i=0; options[i].opt; i++) {
		if(arg[2] == 0) {
			if(arg[1] == options[i].sopt) {
				return options[i].opt;
			}
		} else {
			if(strcmp(arg + 1, options[i].lopt) == 0) {
				return options[i].opt;
			}
		}
	}
	return OPT_UNKNOWN;
}
