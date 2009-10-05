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
#include <limits.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "wsys.h"

enum { EV_RD, EV_WR };

struct evcall {
	int fd;
	void (*func[2])(int);
	struct evcall *next;
};

static void draw(int x, int y, int w, int h);
static Window create_window(Display *dpy, int scr, int xsz, int ysz);
static int handle_event(Display *dpy, XEvent *ev);
static int xerr(Display *dpy, XErrorEvent *err);

static Display *dpy;
static int scr;
static Window win;
static XVisualInfo vis;
static Atom wm_prot, wm_del;
static XShmSegmentInfo shm;
static XImage *ximg;
static GC gc;
static int xerr_caught;
static int win_width, win_height;

static int exit_loop;

static void (*keyb_func)(int, int);
static void (*idle_func)(void);

static struct evcall *evlist;


int wsys_init(int xsz, int ysz)
{
	int (*prev_xerr)(Display*, XErrorEvent*);

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server\n");
		return -1;
	}
	scr = DefaultScreen(dpy);

	if(!(win = create_window(dpy, scr, xsz, ysz))) {
		goto err;
	}
	gc = XCreateGC(dpy, win, 0, 0);

	/* try to use the MIT-SHM extension */
	if(!XShmQueryExtension(dpy)) {
		fprintf(stderr, "MIT-SHM not found\n");
		goto err;
	}
	if(!(ximg = XShmCreateImage(dpy, vis.visual, 24, ZPixmap, 0, &shm, xsz, ysz))) {
		fprintf(stderr, "failed to create shared memory XImage\n");
		goto err;
	}
	if((shm.shmid = shmget(IPC_PRIVATE, ximg->bytes_per_line * ximg->height, IPC_CREAT | 0777)) == -1) {
		perror("shmget failed");
		goto err;
	}
	shm.shmaddr = ximg->data = shmat(shm.shmid, 0, 0);
	shm.readOnly = False;

	prev_xerr = XSetErrorHandler(xerr);
	XShmAttach(dpy, &shm);
	XSync(dpy, False);
	XSetErrorHandler(prev_xerr);

	/* mark the shared mem segment for deletion after both we and X detach it */
	if(shmctl(shm.shmid, IPC_RMID, 0) == -1) {
		perror("shm remove failed");
	}

	if(xerr_caught) {
		xerr_caught = 0;
		fprintf(stderr, "XShmAttach failed\n");

		shmdt(shm.shmaddr);
		shm.shmid = -1;		/* this signifies the fallback to XPutImage */

		if(!(ximg->data = malloc(ximg->height * ximg->bytes_per_line))) {
			perror("failed to allocate image");
			goto err;
		}
	}
	return 0;

err:
	wsys_destroy();
	return -1;

}

void wsys_destroy(void)
{
	int (*prev_xerr)(Display*, XErrorEvent*);

	if(shm.shmid != -1) {
		prev_xerr = XSetErrorHandler(xerr);
		XShmDetach(dpy, &shm);
		XSync(dpy, False);
		XSetErrorHandler(prev_xerr);

		shmdt(shm.shmaddr);
	}
	if(gc) {
		XFreeGC(dpy, gc);
	}
	XCloseDisplay(dpy);
}

void wsys_set_title(const char *title)
{
	XTextProperty tp_wname;
	XStringListToTextProperty((char**)&title, 1, &tp_wname);
	XSetWMName(dpy, win, &tp_wname);
	XFree(tp_wname.value);
}

#define SPREAD(x) \
	((x) | ((uint32_t)(x) << 8) | ((uint32_t)(x) << 16) | ((uint32_t)(x) << 24))

void wsys_updrect(int dst_x, int dst_y, int dst_xsz, int dst_ysz,
		int src_x, int src_y, unsigned char *img, int img_width, int img_height)
{
	int x, y;

	uint32_t *dst = (uint32_t*)(ximg->data + ximg->bytes_per_line * dst_y
		+ dst_x * (ximg->bits_per_pixel / CHAR_BIT));
	unsigned char *src = img + src_y * img_width * 4 + src_x * 4;

	if(img) {
		for(y=0; y<dst_ysz; y++) {
			for(x=0; x<dst_xsz; x++) {
				uint32_t red = ximg->red_mask & SPREAD(src[0]);
				uint32_t green = ximg->green_mask & SPREAD(src[1]);
				uint32_t blue = ximg->blue_mask & SPREAD(src[2]);

				*dst++ = red | green | blue;
				src += 4;
			}
			dst += ximg->bytes_per_line / sizeof *dst - dst_xsz;
			src += (img_width - dst_xsz) * 4;
		}
	} else {
		memset(dst, 0x20, dst_ysz * ximg->bytes_per_line);
	}

	draw(dst_x, dst_y, dst_xsz, dst_ysz);
}

void wsys_clear(void)
{
	/*
	wsys_updrect(0, 0, ximg->width, ximg->height, 0, 0, 0, 0, 0);
	*/

	XClearWindow(dpy, win);
}

void wsys_set_key_func(void (*func)(int, int))
{
	keyb_func = func;
}

void wsys_set_idle_func(void (*func)(void))
{
	idle_func = func;
}

static struct evcall *find_event_func(int fd, struct evcall **prev_ptr)
{
	struct evcall *prev = 0, *node = evlist;

	while(node) {
		if(node->fd == fd) {
			*prev_ptr = prev;
			return node;
		}
		prev = node;
		node = node->next;
	}
	return 0;
}

/* if func is not null it sets an event callback for the specified
 * file descriptor, otherwise it removes an existing one.
 */
static int set_event_func(int fd, int rdwr, void (*func)(int))
{
	struct evcall *node, *prev;

	if((node = find_event_func(fd, &prev))) {
		node->func[rdwr] = func;

		/* if we removed both callbacks, remove the node */
		if(!node->func[rdwr] && !node->func[rdwr]) {
			if(prev) {
				prev->next = node->next;
			} else {
				evlist = node->next;
			}
			free(node);
		}
		return 0;
	}

	/* node not found, if func is non-null add it */
	if(func) {
		if(!(node = malloc(sizeof *node))) {
			return -1;
		}
		node->next = evlist;
		evlist = node;
		node->fd = fd;
		node->func[rdwr] = func;
		node->func[(rdwr + 1) % 2] = 0;
	}
	return 0;
}

void wsys_set_read_func(int fd, void (*func)(int))
{
	if(set_event_func(fd, EV_RD, func) == -1) {
		perror("wsys_set_read_func failed");
	}
}

void wsys_set_write_func(int fd, void (*func)(int))
{
	if(set_event_func(fd, EV_WR, func) == -1) {
		perror("wsys_set_write_func failed");
	}
}

int wsys_process_events(void)
{
	fd_set rds, wrs;
	int max_fd, nready;
	struct evcall *evnode;
	struct timeval tv = {0, 0};

	FD_ZERO(&rds);
	FD_ZERO(&wrs);

	max_fd = ConnectionNumber(dpy);
	FD_SET(max_fd, &rds);

	evnode = evlist;
	while(evnode) {
		if(evnode->func[EV_RD]) {
			FD_SET(evnode->fd, &rds);
		}
		if(evnode->func[EV_WR]) {
			FD_SET(evnode->fd, &wrs);
		}
		if(evnode->fd > max_fd) {
			max_fd = evnode->fd;
		}
		evnode = evnode->next;
	}

	do {
		nready = select(max_fd + 1, &rds, &wrs, 0, idle_func ? &tv : 0);
	} while(nready == -1 && errno == EINTR);

	evnode = evlist;
	while(evnode) {
		if(FD_ISSET(evnode->fd, &rds) && evnode->func[EV_RD]) {
			evnode->func[EV_RD](evnode->fd);
		}
		if(FD_ISSET(evnode->fd, &wrs) && evnode->func[EV_WR]) {
			evnode->func[EV_WR](evnode->fd);
		}
		evnode = evnode->next;
	}

	if(FD_ISSET(ConnectionNumber(dpy), &rds)) {
		while(XPending(dpy)) {
			XEvent ev;
			XNextEvent(dpy, &ev);
			if(handle_event(dpy, &ev) == -1) {
				return -1;
			}
		}
	}

	if(idle_func) {
		idle_func();
	}
	return 0;
}

int wsys_main_loop(void)
{
	int res;
	do {
		res = wsys_process_events();
	} while(res == 0 && !exit_loop);

	return res;
}

void wsys_exit_main_loop(void)
{
	exit_loop = 1;
}

void wsys_redisplay(void)
{
	wsys_redisplay_area(0, 0, win_width, win_height);
}

void wsys_redisplay_area(int x, int y, int w, int h)
{
	XEvent ev;

	ev.type = Expose;
	ev.xexpose.x = x;
	ev.xexpose.y = y;
	ev.xexpose.width = w;
	ev.xexpose.height = h;
	ev.xexpose.count = 0;

	XSendEvent(dpy, win, False, 0, &ev);
	XFlush(dpy);
}

static void draw(int x, int y, int w, int h)
{
	if(shm.shmid == -1) {
		XPutImage(dpy, win, gc, ximg, x, y, x, y, w, h);
	} else {
		XShmPutImage(dpy, win, gc, ximg, x, y, x, y, w, h, False);
	}
}

static Window create_window(Display *dpy, int scr, int xsz, int ysz)
{
	Window win, root;
	XSetWindowAttributes wattr;
	Colormap cmap;
	XClassHint chint;

	root = RootWindow(dpy, scr);

	if(!XMatchVisualInfo(dpy, scr, 24, TrueColor, &vis)) {
		fprintf(stderr, "no suitable TrueColor visual available\n");
		return 0;
	}
	cmap = XCreateColormap(dpy, root, vis.visual, AllocNone);

	wattr.background_pixel = wattr.border_pixel = BlackPixel(dpy, scr);
	wattr.colormap = cmap;

	win = XCreateWindow(dpy, root, 10, 10, xsz, ysz, 0, 24, InputOutput, vis.visual,
			CWColormap | CWBackPixel | CWBorderPixel, &wattr);
	XSelectInput(dpy, win, ExposureMask | StructureNotifyMask | KeyPressMask);

	wm_prot = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_del = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

	XSetWMProtocols(dpy, win, &wm_del, 1);

	chint.res_name = "raytracer";
	chint.res_class = "raytracer";
	XSetClassHint(dpy, win, &chint);

	XMapWindow(dpy, win);

	win_width = xsz;
	win_height = ysz;
	return win;
}

static int handle_event(Display *dpy, XEvent *ev)
{
	static int win_mapped;
	KeySym keysym;

	switch(ev->type) {
	case MapNotify:
		win_mapped = 1;
		break;

	case UnmapNotify:
		win_mapped = 0;
		break;

	case Expose:
		if(win_mapped) {
			draw(ev->xexpose.x, ev->xexpose.y, ev->xexpose.width, ev->xexpose.height);
		}
		break;

	case KeyPress:
	case KeyRelease:
		keysym = XLookupKeysym((XKeyEvent*)&ev->xkey, 0);
		if(keyb_func) {
			keyb_func(keysym & 0xff, ev->type == KeyPress);
		}
		break;

	case ClientMessage:
		if(ev->xclient.message_type == wm_prot) {
			if(ev->xclient.data.l[0] == wm_del) {
				return -1;
			}
		}
		break;

	case ConfigureNotify:
		win_width = ev->xconfigure.width;
		win_height = ev->xconfigure.height;
		break;

	default:
		break;
	}

	return 0;
}

static int xerr(Display *dpy, XErrorEvent *err)
{
	xerr_caught++;
	return 0;
}
