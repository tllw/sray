#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "fb.h"
#include "wsys.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F	0x8814
#endif

#define SRAY_BIN	"sray"

struct block {
	int x, y, w, h;
	int active;
};

int start_sray(char **argv);
void cleanup(void);
void sigfunc(int s);

void read_func(int fd);

void disp(void);
void reshape(int x, int y);
void keyb(int key, int state);

int round_pow2(int x);
int have_glext(const char *name);
void add_dirty(int x, int y, int w, int h);

struct framebuffer *fb;
int sray_pid;

unsigned int tex;
int tex_xsz, tex_ysz;

int blksz, num_threads;
struct block *rblocks;

int num_blocks, num_dirty;
struct block *dirty;

int starting_frame;


int main(int argc, char **argv)
{
	unsigned int tex_int_fmt = GL_RGBA32F;

	if(start_sray(argv) == -1) {
		return 1;
	}

	wsys_init(fb->width, fb->height, 0);

	wsys_set_display_func(disp);
	wsys_set_reshape_func(reshape);
	wsys_set_key_func(keyb);
	wsys_set_read_func(0, read_func);

	wsys_set_title("vsray (GUI s-ray frontend)");

	if(!have_glext("GL_ARB_texture_float")) {
		tex_int_fmt = 4;
	}

	tex_xsz = round_pow2(fb->width);
	tex_ysz = round_pow2(fb->height);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, tex_int_fmt, tex_xsz, tex_ysz, 0, GL_RGBA, GL_FLOAT, 0);

	glDrawBuffer(GL_FRONT);
	glClear(GL_COLOR_BUFFER_BIT);

	assert(glGetError() == GL_NO_ERROR);

	wsys_main_loop();
	return 0;
}

int start_sray(char **argv)
{
	int pid, pfd[2];

	if(pipe(pfd) == -1) {
		perror("failed to setup pipe");
		return -1;
	}

	if((pid = fork()) == -1) {
		perror("failed to fork child process");
		return -1;
	} else if(pid) {
		sray_pid = pid;
		signal(SIGCHLD, sigfunc);	/* arrange to get notification for sray's demise */

		close(pfd[1]);
		dup2(pfd[0], 0);	/* arrange for sray's output to go into our stdin */

		/* set the non-blocking flag of fd 0 */
		fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
	} else {
		/* setup file descriptors */
		close(pfd[0]);
		dup2(pfd[1], 1);

		/* If no file is specified sray will try to read from stdin, blocking
		 * indefinitely in our case. By closing fd 0 we make sure that won't happen.
		 */
		close(0);

		execvp(SRAY_BIN, argv);
		perror("failed to execute sray");
		_exit(1);
	}

	/* block until sray finishes going through its init function and
	 * starts producing output
	 */
	for(;;) {
		fd_set rds;

		FD_ZERO(&rds);
		FD_SET(0, &rds);

		if(select(1, &rds, 0, 0, 0) > 0 && FD_ISSET(0, &rds)) {
			break;
		}
	}

	if(!(fb = map_framebuf(sray_pid))) {
		kill(sray_pid, SIGTERM);
		return -1;
	}
	atexit(cleanup);
	return 0;
}

void cleanup(void)
{
	unmap_framebuf(fb);
}

void read_func(int fd)
{
	int i;
	ssize_t rdsz;
	char msg[32];

	/* fd has been marked as non-blocking so we can loop until we get EAGAIN or EOF */
	while((rdsz = read(fd, msg, sizeof msg)) > 0) {
		char cmd;
		int args[4], nmatched;

		nmatched = sscanf(msg, "%c %d %d %d %d\n", &cmd, args, args + 1, args + 2, args + 3);
		if(rdsz < sizeof msg || nmatched < 5) {
			fprintf(stderr, "got malformed message: %s\n", msg);
			continue;
		}

		switch(cmd) {
		case 'f':	/* start frame */
			wsys_redisplay();
			blksz = args[2];

			if(args[3] != num_threads) {
				num_threads = args[3];

				free(rblocks);
				if((rblocks = malloc(num_threads * sizeof *rblocks))) {
					memset(rblocks, 0, num_threads * sizeof *rblocks);
				}
			}

			if(args[0] * args[1] != num_blocks) {
				num_blocks = args[0] * args[1];

				free(dirty);
				if(!(dirty = malloc(num_blocks * sizeof *dirty))) {
					perror("failed to allocate memory");
					abort();
				}
			}

			starting_frame = 1;
			break;

		case 'd':	/* frame done */
			printf("frame done in %d ms\n", args[0]);

			if(num_dirty) {
				disp();
			}
			break;

		case 's':	/* start block */
			wsys_redisplay();
			for(i=0; i<num_threads; i++) {
				if(!rblocks[i].active) {
					rblocks[i].active = 1;
					rblocks[i].x = args[0];
					rblocks[i].y = args[1];
					rblocks[i].w = args[2];
					rblocks[i].h = args[3];
					break;
				}
			}
			break;

		case 'e':	/* end block */
			wsys_redisplay();
			add_dirty(args[0], args[1], args[2], args[3]);

			for(i=0; i<num_threads; i++) {
				if(rblocks[i].active && rblocks[i].x == args[0] && rblocks[i].y == args[1]) {
					rblocks[i].active = 0;
				}
			}
			break;

		default:
			fprintf(stderr, "unexpected message: %s\n", msg);
		}
	}

	if(rdsz == -1 && errno != EAGAIN) {
		perror("failed to read from pipe");
		return;
	}

	if(rdsz == 0) {
		/* reached end of file */
		wsys_set_read_func(fd, 0);
	}
}

void sigfunc(int s)
{
	int status;

	switch(s) {
	case SIGCHLD:
		wait(&status);
		if(!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			printf("sray completed unsucessfully\n");
			exit(1);
		}
		break;

	default:
		break;
	}
}

/* --- opengl stuff --- */
#define NORM_X(x)	((float)(x) / (float)fb->width)
#define NORM_Y(y)	((float)(y) / (float)fb->height)

void disp(void)
{
	int i, j;

	if(starting_frame) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBegin(GL_QUADS);
		glColor4f(0, 0, 0, 0.75);
		glVertex2f(0, 0);
		glVertex2f(1, 0);
		glVertex2f(1, 1);
		glVertex2f(0, 1);
		glEnd();

		glDisable(GL_BLEND);
		starting_frame = 0;
	}

	for(i=0; i<num_dirty; i++) {
		float max_s, max_t;
		float x0, y0, x1, y1;
		float *pix = fb->pixels + (dirty[i].y * fb->width + dirty[i].x) * 4;

		for(j=0; j<dirty[i].h; j++) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, dirty[i].x, dirty[i].y + j, dirty[i].w,
					1, GL_RGBA, GL_FLOAT, pix);
			pix += fb->width * 4;
		}

		max_s = (float)fb->width / tex_xsz;
		max_t = (float)fb->height / tex_ysz;

		x0 = NORM_X(dirty[i].x);
		y0 = NORM_Y(dirty[i].y);
		x1 = NORM_X(dirty[i].x + dirty[i].w);
		y1 = NORM_Y(dirty[i].y + dirty[i].h);

		glEnable(GL_TEXTURE_2D);

		glBegin(GL_QUADS);
		glColor4f(1, 1, 1, 1);
		glTexCoord2f(x0 * max_s, y0 * max_t);
		glVertex2f(x0, y0);
		glTexCoord2f(x1 * max_s, y0 * max_t);
		glVertex2f(x1, y0);
		glTexCoord2f(x1 * max_s, y1 * max_t);
		glVertex2f(x1, y1);
		glTexCoord2f(x0 * max_s, y1 * max_t);
		glVertex2f(x0, y1);
		glEnd();

		glDisable(GL_TEXTURE_2D);
	}
	num_dirty = 0;

	/*for(i=0; i<num_threads; i++) {
		if(rblocks[i].active) {
			static const float scl = 0.2;

			float w = NORM_X(rblocks[i].w - 1);
			float h = NORM_Y(rblocks[i].h - 1);
			float x0 = NORM_X(rblocks[i].x);
			float y0 = NORM_Y(rblocks[i].y + 1);
			float x1 = x0 + w;
			float y1 = y0 + h;

			glBegin(GL_LINES);
			glColor3f(0, 1, 0);
			glVertex2f(x0, y0); glVertex2f(x0 + scl * w, y0);
			glVertex2f(x0, y0); glVertex2f(x0, y0 + scl * h);

			glVertex2f(x1, y0); glVertex2f(x1 - scl * w, y0);
			glVertex2f(x1, y0); glVertex2f(x1, y0 + scl * h);

			glVertex2f(x1, y1); glVertex2f(x1 - scl * w, y1);
			glVertex2f(x1, y1); glVertex2f(x1, y1 - scl * h);

			glVertex2f(x0, y1); glVertex2f(x0 + scl * w, y1);
			glVertex2f(x0, y1); glVertex2f(x0, y1 - scl * h);
			glEnd();
		}
	}*/

	glFlush();
	assert(glGetError() == GL_NO_ERROR);
}

void reshape(int x, int y)
{
	/*glViewport(0, 0, x, y);*/

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glScalef(1.0, -1.0, 1.0);
	glTranslatef(-1, -1, 0);
	glScalef(2.0, 2.0, 2.0);
}

void keyb(int key, int state)
{
	if(state) {
		switch(key) {
		case 'q':
			exit(0);

		default:
			break;
		}
	}
}

int round_pow2(int x)
{
	x--;
	x = (x >> 1) | x;
	x = (x >> 2) | x;
	x = (x >> 4) | x;
	x = (x >> 8) | x;
	x = (x >> 16) | x;
	return x + 1;
}

int have_glext(const char *name)
{
	static char *extstr;
	char *substr;

	if(!extstr) {
		char *ptr = (char*)glGetString(GL_EXTENSIONS);
		assert(ptr);

		if(!(extstr = malloc(strlen(ptr) + 2))) {
			return strstr(ptr, name) ? 1 : 0;
		}
		ptr = extstr;

		while(ptr && *ptr) {
			if((ptr = strchr(ptr, ' '))) {
				*ptr++ = 0;
			}
		}
	}

	substr = extstr;
	while(*substr) {
		if(strcmp(substr, name) == 0) {
			return 1;
		}
		substr += strlen(substr) + 1;
	}
	return 0;
}

void add_dirty(int x, int y, int w, int h)
{
	int i = num_dirty++;

	dirty[i].x = x;
	dirty[i].y = y;
	dirty[i].w = w;
	dirty[i].h = h;
}
