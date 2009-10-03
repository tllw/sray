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
#ifndef REND_OPT_H_
#define REND_OPT_H_

#define RAY_MAGNITUDE	10000.0

extern struct options {
	char *scenefile;
	int width, height;
	int min_samples, max_samples;
	float max_var;
	float min_energy;
	int shadow_samples;
	int diffuse_samples;
	int threads;
	int blk_sz;
	int iter;
	int verb;
	int fps;
	int mblur;
	int shutter;
	int time_start, time_end;
	int caust_photons, gi_photons;
	float gather_dist;
	float photon_energy;

	int scnoct_max_depth, scnoct_max_items;
	int meshoct_max_depth, meshoct_max_items;

	int num_frames;
	int interactive;	/* connect to X server? */
} opt;

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

int parse_opt(int argc, char **argv);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* REND_OPT_H_ */
