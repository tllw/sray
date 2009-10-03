#ifndef WSYS_H_
#define WSYS_H_

#ifdef __cplusplus
extern "C" {
#endif

int wsys_init(int width, int height);
void wsys_destroy(void);
void wsys_set_title(const char *title);

void wsys_updrect(int dst_x, int dst_y, int dst_xsz, int dst_ysz,
		int src_x, int src_y, unsigned char *img, int img_width, int img_height);
void wsys_clear(void);

void wsys_set_key_func(void (*func)(int, int));
void wsys_set_idle_func(void (*func)(void));

void wsys_set_read_func(int fd, void (*func)(int));
void wsys_set_write_func(int fd, void (*func)(int));

int wsys_process_events(void);
int wsys_main_loop(void);
void wsys_exit_main_loop(void);

void wsys_redisplay(void);
void wsys_redisplay_area(int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif

#endif	/* WSYS_H_ */
