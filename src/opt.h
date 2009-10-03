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
