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
