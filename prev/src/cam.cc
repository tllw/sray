#if defined(__APPLE__) && defined(__MACH__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <vmath.h>
#include "cam.h"

static float cam_theta = 0, cam_phi = 25, cam_dist = 5;
static Vector3 cam_pos(0, 0.25, 0);

static float cam_speed = 0.005;
static float rot_speed = 0.5;
static float zoom_speed = 0.1;

void cam_reset(void)
{
	cam_theta = 0;
	cam_phi = 25;
	cam_dist = 5;
	cam_pos = Vector3(0, 0.25, 0);
}

void cam_pan(int dx, int dy)
{
	float dxf = dx * cam_speed * cam_dist;
	float dyf = dy * cam_speed * cam_dist;
	float angle = -DEG_TO_RAD(cam_theta);

	cam_pos.x += cos(angle) * dxf + sin(angle) * dyf;
	cam_pos.z += -sin(angle) * dxf + cos(angle) * dyf;
}

void cam_rotate(int dx, int dy)
{
	cam_theta += dx * rot_speed;
	cam_phi += dy * rot_speed;

	if(cam_phi <= -90) cam_phi = -89;
	if(cam_phi >= 90) cam_phi = 89;
}

void cam_zoom(int dz)
{
	cam_dist += dz * zoom_speed;
	if(cam_dist < 0.001) {
		cam_dist = 0.001;
	}
}

void set_view_matrix(void)
{
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);
	glTranslatef(-cam_pos.x, -cam_pos.y, -cam_pos.z);
}

void print_camera(void)
{
	float theta = DEG_TO_RAD(cam_theta - 90);
	float phi = DEG_TO_RAD(cam_phi - 90);

	Vector3 dir;
	dir.x = cos(theta) * sin(phi) * cam_dist;
	dir.y = cos(phi) * cam_dist;
	dir.z = sin(theta) * sin(phi) * cam_dist;

	Vector3 targ = cam_pos;
	Vector3 pos = targ + dir;

	printf("<camera type=\"target\" fov=\"50\">\n");
	printf("\t<xform pos=\"%.3f %.3f %.3f\"/>\n", pos.x, pos.y, pos.z);
	printf("\t<target>\n");
	printf("\t\t<xform pos=\"%.3f %.3f %.3f\"/>\n", targ.x, targ.y, targ.z);
	printf("\t</target>\n");
	printf("</camera>\n");
}
