#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>

#if defined(__unix__) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#else	/* assume win32 */
#include <windows.h>
#endif

#include "fb.h"

#define FBSHM	"/srayfb.%d"


#ifdef __GNUC__
#define PACKED	__attribute__((packed))
#else
#define PACKED
#endif

/* XXX this struct must be kept in sync with struct framebuffer in
 * vsray/src/fb.h
 */

#pragma pack(push, 1)
struct fbheader {
	int width, height;
	int rbits, gbits, bbits, abits;

	int progr, max_progr;

	float pixels[1];
} PACKED;
#pragma pack(pop)


static void *map_framebuffer(size_t fbsz);
static void unmap_framebuffer(void *fb, size_t fbsz);

static int fb_is_shm;

void *alloc_framebuf(int xsz, int ysz)
{
	struct fbheader *hdr;
	size_t sz = xsz * ysz * 4 * sizeof(float) + sizeof *hdr;

	if(!(hdr = map_framebuffer(sz))) {
		return malloc(sz);
	}

	hdr->width = xsz;
	hdr->height = ysz;
	hdr->rbits = hdr->gbits = hdr->bbits = hdr->abits = sizeof(float) * CHAR_BIT;
	hdr->progr = hdr->max_progr = 0;
	fb_is_shm = 1;

	return hdr->pixels;
}

void free_framebuf(void *fb)
{
	struct fbheader *hdr = (void*)((char*)fb - offsetof(struct fbheader, pixels));
	size_t sz = hdr->width * hdr->height * 4 * sizeof(float) + sizeof *hdr;

	if(fb_is_shm) {
		unmap_framebuffer(hdr, sz);
	} else {
		free(hdr);
	}
}

#if defined(unix) || defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
static void *map_framebuffer(size_t fbsz)
{
	void *shmptr;
	int fd;
	char shmname[64];
	sprintf(shmname, FBSHM, (int)getpid());

	if((fd = shm_open(shmname, O_RDWR | O_CREAT, 0664)) == -1) {
		fprintf(stderr, "failed to create shared memory area for the framebuffer\n");
		return 0;
	}
	ftruncate(fd, fbsz);

	if((shmptr = mmap(0, fbsz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == (void*)-1) {
		fprintf(stderr, "failed to map shared memory framebuffer\n");
		close(fd);
		shm_unlink(shmname);
		return 0;
	}
	close(fd);
	return shmptr;
}

static void unmap_framebuffer(void *fb, size_t fbsz)
{
	char shmname[64];

	if(fb) {
		munmap(fb, fbsz);
	}

	sprintf(shmname, FBSHM, (int)getpid());
	shm_unlink(shmname);
}
#endif

#if defined(WIN32) || defined(__WIN32__)
static void *map_framebuffer(size_t fbsz)
{
	HANDLE fd;
	char shmname[64];
	void *shmptr;
	sprintf(shmname, FBSHM, GetCurrentProcessId());

	if(!(fd = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, fbsz, shmname))) {
		fprintf(stderr, "failed to create shared memory area for the framebuffer\n");
		return 0;
	}
	if(!(shmptr = MapViewOfFile(fd, 0, 0, 0, fbsz))) {
		fprintf(stderr, "failed to map the shared memory framebuffer\n");
	}
	CloseHandle(fd);
	return shmptr;
}

static void unmap_framebuffer(void *fb, size_t fbsz)
{
	if(fb) {
		UnmapViewOfFile(fb);
	}
}
#endif