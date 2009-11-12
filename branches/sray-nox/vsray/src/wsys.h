#ifndef WSYS_H_
#define WSYS_H_

enum {
	WSYS_GL_DEPTH		= 1,
	WSYS_GL_STENCIL		= 2,
	WSYS_GL_STEREO		= 4,
	WSYS_GL_FULLSCREEN	= 8,
	WSYS_GL_DOUBLE		= 16
};

#ifdef __cplusplus
extern "C" {
#endif

int wsys_init(int width, int height, unsigned int mmask);
void wsys_destroy(void);
void wsys_set_title(const char *title);

void wsys_set_display_func(void (*func)(void));
void wsys_set_display_rect_func(void (*func)(int, int, int, int));
void wsys_set_reshape_func(void (*func)(int, int));
void wsys_set_key_func(void (*func)(int, int));
void wsys_set_idle_func(void (*func)(void));

void wsys_set_read_func(int fd, void (*func)(int));
void wsys_set_write_func(int fd, void (*func)(int));

int wsys_process_events(void);
int wsys_main_loop(void);
void wsys_exit_main_loop(void);

void wsys_redisplay(void);
void wsys_redisplay_area(int x, int y, int w, int h);

void wsys_swap_buffers(void);

#ifdef __cplusplus
}
#endif

#endif	/* WSYS_H_ */
