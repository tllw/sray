#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "fb.h"

#define FBSHM	"/srayfb.%d"

static char shmname[64];

struct framebuffer *map_framebuf(int pid)
{
	int fd;
	struct framebuffer *fb;
	int fbsz = sizeof *fb;

	sprintf(shmname, FBSHM, pid);

	if((fd = shm_open(shmname, O_RDONLY, 0)) == -1) {
		fprintf(stderr, "failed to open shared framebuffer\n");
		return 0;
	}
	ftruncate(fd, fbsz);

	/* first map just the header to find out the framebuffer size */
	if((fb = mmap(0, fbsz, PROT_READ, MAP_SHARED, fd, 0)) == (void*)-1) {
		fprintf(stderr, "failed to map the shared memory framebuffer header\n");
		close(fd);
		shm_unlink(shmname);
		return 0;
	}
	
	printf("allocating %dx%d framebuffer\n", fb->width, fb->height);
	fbsz = fb->width * fb->height * 4 * sizeof(float) + sizeof *fb;
	munmap(fb, sizeof *fb);

	/* now that we know the actual size of the framebuffer, map it in full */
	if((fb = mmap(0, fbsz, PROT_READ, MAP_SHARED, fd, 0)) == (void*)-1) {
		fprintf(stderr, "failed to map the shared memory framebuffer\n");
		close(fd);
		shm_unlink(shmname);
		return 0;
	}
	close(fd);

	return fb;
}


void unmap_framebuf(struct framebuffer *fb)
{
	if(fb) {
		munmap(fb, fb->width * fb->height * 4 * sizeof(float) + sizeof *fb);
	}
	if(shmname[0]) {
		shm_unlink(shmname);
	}
}
