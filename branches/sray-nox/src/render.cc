#include "render.h"
#include "tpool.h"
#include "block.h"

static void build_accel(long t0, long t1);
static void shoot_photons(long t0, long t1);
static void render_frame(long t0, long t1);
static bool start_frame(long t0, long t1, bool calc_prior);
static void render_block(void *cls);
static void block_done(void *cls);
static float variance(Color *samples, const Color &sum, int num, float rcp_num);

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

	build_accel(t0, t1);
	shoot_photons(t0, t1);
	render_frame(t0, t1);
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

	scn->build_photon_maps(t0, t1);
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
				double dx = 2.0 * (double)(blk->x + blk->xsz / 2.0) / (double)opt.width - 1.0;
				double dy = 2.0 * (double)(blk->y + blk->ysz / 2.0) / (double)opt.height - 1.0;
				blk->pri = (int)(gaussian(dx * dx + dy * dy, 0.0, 0.3) * 500.0);
			}

			Task task(render_block, block_done, blk);
			*tptr++ = task;
		}
	}

	if(calc_prior) {
		qsort(tasks, bcount, sizeof *tasks, blkcmp);
	}
	tpool.add_work(tasks, bcount);

	delete [] tasks;
	return true;
}

static void render_block(void *cls)
{
	struct block *blk = (struct block*)cls;
	long ftime = blk->t0;

	Color *subpix = (Color*)alloca(opt.max_samples * sizeof *subpix);
	double *rcp_lut = (double*)alloca((1 + opt.max_samples) * sizeof *rcp_lut);

	for(int i=1; i<=opt.max_samples; i++) {
		rcp_lut[i] = 1.0 / (double)i;
	}

	int xsz = framebuffer->get_width();
	int start_offs = blk->y * xsz + blk->x;
	Color *img = framebuffer->get_pixels() + start_offs;

	for(int y=0; y<blk->ysz; y++) {
		for(int x=0; x<blk->xsz; x++) {
			int i = 0;

			img[x].x = img[x].y = img[x].z = img[x].w = 0.0;

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
		}
		img += xsz;
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
