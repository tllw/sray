#include "render.h"
#include "tpool.h"
#include "block.h"
#include "timer.h"

static void build_accel(long t0, long t1);
static void shoot_photons(long t0, long t1);
static void render_frame(long t0, long t1);
static bool start_frame(long t0, long t1, bool calc_prior);
static void render_block(void *cls);
static void block_done(void *cls);
static float variance(Color *samples, const Color &sum, int num, float rcp_num);
static int rtaskcmp(const void *a, const void *b);
static void emit_status(char st, int arg1, int arg2, int arg3, int arg4);

static ThreadPool tpool;
static Image *framebuffer;
static Scene *scn;
static Camera *cam;


bool rend_init(Image *fb)
{
	// initialize thread pool
	if(!tpool.start(opt.threads)) {
		fprintf(stderr, "failed to create thread pool\n");
		return false;
	}

	framebuffer = fb;
	return true;
}

void render(long msec)
{
	scn = get_scene();
	cam = scn->get_camera();

	long shutter = cam->get_shutter();
	long t0 = msec - shutter / 2;
	long t1 = t0 + shutter;

	unsigned long start_timer = 0;
	if(BACKEND) {
		start_timer = get_msec();
	}

	build_accel(t0, t1);
	shoot_photons(t0, t1);
	render_frame(t0, t1);

	if(BACKEND) {
		emit_status('d', get_msec() - start_timer, 0, 0, 0);
	}
}

static void build_accel(long t0, long t1)
{
	if(VERBOSE) {
		printf("building octree\n");
	}

	scn->build_tree(t0, t1);
}


static void shoot_photons(long t0, long t1)
{
	if(!opt.caust_photons && !opt.gi_photons) {
		return;
	}

	if(VERBOSE) {
		printf("shooting photons\n");
	}

	int num_lights = scn->get_light_count();
	if(!num_lights || (!opt.caust_photons && !opt.gi_photons)) {
		return;
	}
	if(t1 == INT_MIN) {
		t1 = t0;
	}

	/* assign number of photons to each light source by dividing the total
	 * number of photons between the light sources, weighted by their intensity.
	 */
	double acc_power = 0.0;
	LightPower *ltpow = new LightPower[num_lights];

	Light **lights = (Light**)scn->get_lights();

	// accumulate light intensity over all lights
	for(int i=0; i<num_lights; i++) {
		Color col = lights[i]->get_color();

		ltpow[i].intensity = (col.x + col.y + col.z) / 3.0;
		acc_power += ltpow[i].intensity;

		// also build the projection maps
		int num_obj = scn->get_object_count();
		if(num_obj) {
			lights[i]->build_projmap((const Object**)scn->get_objects(), num_obj, t0, t1);
		}
	}

	// calculate the fraction of the total power corresponding to each light
	for(int i=0; i<num_lights; i++) {
		ltpow[i].photon_power = ltpow[i].intensity / acc_power;
	}

	int cphot = scn->build_caustics_map(t0, t1, opt.caust_photons, ltpow);
	int gphot = scn->build_global_map(t0, t1, opt.gi_photons, ltpow);

	delete [] ltpow;

	if(VERBOSE) {
		printf("caustics photons stored: %d (out of %d shot)\n", cphot, opt.caust_photons);
		printf("gi photons stored: %d (out of %d shot)\n", gphot, opt.gi_photons);
	}
	//scn->build_photon_maps(t0, t1);
}

static void render_frame(long t0, long t1)
{
	if(!QUIET) {
		printf("rendering ");
		fflush(stdout);
	}

	// start a new frame
	if(!start_frame(t0, t1, true)) {
		return;
	}

	// wait until it's done
	tpool.wait_work();

	if(!QUIET) {
		putchar('\n');
	}
}

static bool start_frame(long t0, long t1, bool calc_prior)
{
	// break the image into blocks
	int xblocks = ((opt.width << 8) / opt.blk_sz + 255) >> 8;
	int yblocks = ((opt.height << 8) / opt.blk_sz + 255) >> 8;
	int bcount = xblocks * yblocks;

	if(BACKEND) {
		emit_status('f', xblocks, yblocks, opt.blk_sz, opt.threads);
	}

	Task *tasks = new Task[bcount];
	Task *tptr = tasks;

	for(int i=0; i<xblocks; i++) {
		for(int j=0; j<yblocks; j++) {
			struct block *blk;
			
			if(!(blk = get_block(i, j, opt.blk_sz))) {
				perror("start_frame failed");
				return false;
			}
			blk->t0 = t0;
			blk->t1 = t1;
			
			if(calc_prior) {
				// calculate gaussian priority
				/*double dx = 2.0 * (double)(blk->x + blk->xsz / 2.0) / (double)opt.width - 1.0;
				double dy = 2.0 * (double)(blk->y + blk->ysz / 2.0) / (double)opt.height - 1.0;*/
				double dx = ((double)blk->x + (double)blk->xsz / 2.0) - (double)opt.width / 2.0;
				double dy = ((double)blk->y + (double)blk->ysz / 2.0) - (double)opt.height / 2.0;
				dx /= (double)opt.width * 4.0;
				dy /= (double)opt.height * 4.0;
				blk->pri = (int)(gaussian(dx * dx + dy * dy, 0.0, 0.01) * 200.0);
			}

			Task task(render_block, block_done, blk);
			*tptr++ = task;
		}
	}

	if(calc_prior) {
		qsort(tasks, bcount, sizeof *tasks, rtaskcmp);
	}
	tpool.add_work(tasks, bcount);

	delete [] tasks;
	return true;
}

static void render_block(void *cls)
{
	struct block *blk = (struct block*)cls;
	long ftime = blk->t0;

	if(BACKEND) {
		emit_status('s', blk->x, blk->y, blk->xsz, blk->ysz);
	}

	Color *subpix = (Color*)alloca(opt.max_samples * sizeof *subpix);
	double *rcp_lut = (double*)alloca((1 + opt.max_samples) * sizeof *rcp_lut);

	for(int i=1; i<=opt.max_samples; i++) {
		rcp_lut[i] = 1.0 / (double)i;
	}

	int xsz = framebuffer->get_width();
	int start_offs = blk->y * xsz + blk->x;
	float *img = framebuffer->get_pixels() + start_offs * 4;

	for(int y=0; y<blk->ysz; y++) {
		for(int x=0; x<blk->xsz; x++) {
			int i = 0;

			img[x * 4] = img[x * 4 + 1] = img[x * 4 + 2] = img[x * 4 + 3] = 0.0f;

			while(i < opt.max_samples) {
				Ray ray = cam->get_primary_ray(x + blk->x, y + blk->y, i, ftime);
				ray.iter = opt.iter;
				subpix[i] = scn->trace_ray(ray);

				Color pixel;
				pixel.x = img[x * 4] += subpix[i].x;
				pixel.y = img[x * 4 + 1] += subpix[i].y;
				pixel.z = img[x * 4 + 2] += subpix[i].z;
				pixel.w = img[x * 4 + 3] += subpix[i].w;
				i++;

				if(i >= opt.min_samples && i > 1) {
					float v = variance(subpix, pixel, i, rcp_lut[i]);
					if(v < opt.max_var) {
						break;
					}
				}
			}

			img[x] *= rcp_lut[i];
		}
		img += xsz * 4;
	}
}

static void block_done(void *cls)
{
	struct block *blk = (struct block*)cls;

	free_block(blk);

	if(!QUIET) {
		putchar('.');
		fflush(stdout);
	}
	if(BACKEND) {
		emit_status('e', blk->x, blk->y, blk->xsz, blk->ysz);
	}
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

static int rtaskcmp(const void *a, const void *b)
{
	Task *ta = (Task*)a;
	Task *tb = (Task*)b;
	return blkcmp(ta->closure, tb->closure);
}

static void emit_status(char st, int arg1, int arg2, int arg3, int arg4)
{
	static pthread_mutex_t statmut = PTHREAD_MUTEX_INITIALIZER;
	char buf[32] = {0};

	sprintf(buf, "%c %6d %6d %6d %6d\n", st, arg1, arg2, arg3, arg4);

	pthread_mutex_lock(&statmut);
	write(1, buf, sizeof buf);
	pthread_mutex_unlock(&statmut);
}
