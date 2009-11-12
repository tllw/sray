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
#include <GL/glx.h>
#include "wsys.h"


enum { EV_RD, EV_WR };

struct evcall {
	int fd;
	void (*func[2])(int);
	struct evcall *next;
};

static struct evcall *find_event_func(int fd, struct evcall **prev_ptr);
static int remove_empty_evcalls(void);
static int set_event_func(int fd, int rdwr, void (*func)(int));

static Window create_window(Display *dpy, int scr, int xsz, int ysz);
static int handle_event(Display *dpy, XEvent *ev);
static void fill_glx_attrib(int *attr, unsigned int mask);

static Display *dpy;
static int scr;
static Window win;
static GLXContext ctx;
static Atom wm_prot, wm_del;
static GC gc;
static unsigned int inpmask;
static int win_xsz, win_ysz;
static unsigned int mode_mask;

static int exit_loop;

static void (*display_func)(void);
static void (*display_rect_func)(int, int, int, int);
static void (*reshape_func)(int, int);
static void (*keyb_func)(int, int);
static void (*idle_func)(void);

static struct evcall *evlist;


int wsys_init(int xsz, int ysz, unsigned int mmask)
{
	mode_mask = mmask;

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server\n");
		return -1;
	}
	scr = DefaultScreen(dpy);

	if(!(win = create_window(dpy, scr, xsz, ysz))) {
		goto err;
	}
	gc = XCreateGC(dpy, win, 0, 0);
	return 0;

err:
	wsys_destroy();
	return -1;

}

void wsys_destroy(void)
{
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

void wsys_set_display_func(void (*func)(void))
{
	display_func = func;

	if(!(inpmask & ExposureMask)) {
		inpmask |= ExposureMask;
		if(win) {
			XSelectInput(dpy, win, inpmask);
		}
	}
}

void wsys_set_display_rect_func(void (*func)(int, int, int, int))
{
	display_rect_func = func;

	if(!(inpmask & ExposureMask)) {
		inpmask |= ExposureMask;
		if(win) {
			XSelectInput(dpy, win, inpmask);
		}
	}
}

void wsys_set_reshape_func(void (*func)(int, int))
{
	reshape_func = func;

	if(!(inpmask & StructureNotifyMask)) {
		inpmask |= StructureNotifyMask;
		if(win) {
			XSelectInput(dpy, win, inpmask);
		}
	}
}

void wsys_set_key_func(void (*func)(int, int))
{
	keyb_func = func;

	if(!(inpmask & (KeyPressMask | KeyReleaseMask))) {
		inpmask |= KeyPressMask | KeyReleaseMask;
		if(win) {
			XSelectInput(dpy, win, inpmask);
		}
	}
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

static int remove_empty_evcalls(void)
{
	int removed = 0;
	struct evcall *prev = 0, *node = evlist;

	while(node) {
		if(!node->func[EV_RD] && !node->func[EV_WR]) {
			struct evcall *next = node->next;

			if(prev) {
				prev->next = node->next;
			} else {
				evlist = node->next;
			}
			free(node);
			removed++;

			node = next;
		} else {
			node = node->next;
		}
	}
	return removed;
}

/* if func is not null it sets an event callback for the specified
 * file descriptor, otherwise it removes an existing one.
 */
static int set_event_func(int fd, int rdwr, void (*func)(int))
{
	struct evcall *node, *prev;

	if((node = find_event_func(fd, &prev))) {
		node->func[rdwr] = func;

		/* If node->func[EV_RD] AND node->func[EV_WR] is zero as a result of
		 * this, the node is now a no-op. However, we defer deletion until
		 * remove_empty_evcalls is called, to avoid traversing invalid pointers
		 * in case this is called in a loop going through the list of evcalls
		 */
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
	int max_fd, nready, xsock;
	struct evcall *evnode;
	struct timeval tv = {0, 0};
	static int called_first_reshape;

	if(!called_first_reshape) {
		if(reshape_func) reshape_func(win_xsz, win_ysz);
		called_first_reshape = 1;
	}

	FD_ZERO(&rds);
	FD_ZERO(&wrs);

	max_fd = xsock = ConnectionNumber(dpy);
	FD_SET(xsock, &rds);

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
	remove_empty_evcalls();

	if(FD_ISSET(xsock, &rds)) {
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
	wsys_redisplay_area(0, 0, win_xsz, win_ysz);
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

void wsys_swap_buffers(void)
{
	glXSwapBuffers(dpy, win);
}

static Window create_window(Display *dpy, int scr, int xsz, int ysz)
{
	Window root;
	XSetWindowAttributes wattr;
	XClassHint chint;
	XVisualInfo *vis;
	int glx_attr[32];

	fill_glx_attrib(glx_attr, mode_mask);

	root = RootWindow(dpy, scr);

	if(!(vis = glXChooseVisual(dpy, scr, glx_attr))) {
		fprintf(stderr, "requested GLX visual is not available\n");
		return 0;
	}
	if(!(ctx = glXCreateContext(dpy, vis, 0, True))) {
		fprintf(stderr, "failed to create GLX context\n");
		XFree(vis);
		return 0;
	}

	wattr.background_pixel = wattr.border_pixel = BlackPixel(dpy, scr);
	wattr.colormap = XCreateColormap(dpy, root, vis->visual, AllocNone);

	win = XCreateWindow(dpy, root, 10, 10, xsz, ysz, 0, vis->depth, InputOutput,
			vis->visual, CWColormap | CWBackPixel | CWBorderPixel, &wattr);
	XSelectInput(dpy, win, inpmask);

	wm_prot = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_del = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

	XSetWMProtocols(dpy, win, &wm_del, 1);

	wsys_set_title("vsray");

	chint.res_name = "vsray";
	chint.res_class = "vsray";
	XSetClassHint(dpy, win, &chint);

	if(glXMakeCurrent(dpy, win, ctx) == False) {
		fprintf(stderr, "failed to make GLX context current\n");
		glXDestroyContext(dpy, ctx);
		XDestroyWindow(dpy, win);
		return 0;
	}

	XMapWindow(dpy, win);
	win_xsz = xsz;
	win_ysz = ysz;
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
			if(display_rect_func) {
				display_rect_func(ev->xexpose.x, ev->xexpose.y, ev->xexpose.width, ev->xexpose.height);
			}
			if(!ev->xexpose.count && display_func) {
				display_func();
			}
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
		win_xsz = ev->xconfigure.width;
		win_ysz = ev->xconfigure.height;
		if(reshape_func) {
			reshape_func(win_xsz, win_ysz);
		}
		break;

	default:
		break;
	}

	return 0;
}

static void fill_glx_attrib(int *attr, unsigned int mask)
{
	*attr++ = GLX_RGBA;
	*attr++ = GLX_RED_SIZE; *attr++ = 8;
	*attr++ = GLX_GREEN_SIZE; *attr++ = 8;
	*attr++ = GLX_BLUE_SIZE; *attr++ = 8;

	if(mask & WSYS_GL_DOUBLE) {
		*attr++ = GLX_DOUBLEBUFFER;
	}
	if(mask & WSYS_GL_DEPTH) {
		*attr++ = GLX_DEPTH_SIZE;
		*attr++ = 24;
	}
	if(mask & WSYS_GL_STENCIL) {
		*attr++ = GLX_STENCIL_SIZE;
		*attr++ = 8;
	}
	if(mask & WSYS_GL_STEREO) {
		*attr++ = GLX_STEREO;
	}
	
	*attr++ = None;
}
