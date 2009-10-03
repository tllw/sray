#ifndef CAM_H_
#define CAM_H_

void cam_reset(void);
void cam_pan(int dx, int dy);
void cam_rotate(int dx, int dy);
void cam_zoom(int dz);

void set_view_matrix(void);

void print_camera(void);

#endif	/* CAM_H_ */
