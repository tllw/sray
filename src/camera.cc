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
#include "camera.h"
#include "opt.h"


enum { CAM_FREE, CAM_TARGET };
static const char *type_name[] = {
	"free",
	"target",
	0
};

Camera::Camera()
{
	aspect = (double)opt.width / (double)opt.height;
	vfov = QUARTER_PI;
	max_dist = RAY_MAGNITUDE;
	shutter = opt.shutter;
}

Camera::~Camera() {}

bool Camera::load_xml(struct xml_node *node)
{
	struct xml_attr *attr;

	if(strcmp(node->name, "camera") != 0) {
		return false;
	}

	// override global shutter speed if present
	if((attr = xml_get_attr(node, "shutter"))) {
		shutter = (int)attr->fval;
	}

	if((attr = xml_get_attr(node, "fov")) && (attr->type == ATYPE_FLT || attr->type == ATYPE_INT)) {
		vfov = DEG_TO_RAD(attr->fval);
	}

	node = node->chld;
	while(node) {
		if(strcmp(node->name, "xform") == 0) {
			if(!XFormNode::load_xml(node)) {
				return false;
			}
		}
		node = node->next;
	}

	return true;
}

void Camera::set_far_plane(double dist)
{
	max_dist = dist;
}

double Camera::get_far_plane() const
{
	return max_dist;
}

void Camera::set_aspect(double aspect)
{
	this->aspect = aspect;
}

void Camera::set_vertical_fov(double fov)
{
	vfov = fov;
}

double Camera::get_vertical_fov() const
{
	return vfov;
}

void Camera::set_horizontal_fov(double fov)
{
	vfov = fov * aspect;
}

double Camera::get_horizontal_fov() const
{
	return vfov * aspect;
}

void Camera::set_shutter(int msec)
{
	shutter = msec;
}

int Camera::get_shutter() const
{
	return shutter;
}

Matrix4x4 Camera::get_matrix(unsigned int time) const
{
	return get_xform_matrix(time);
}

Ray Camera::get_primary_ray(int x, int y, int sub, unsigned int time) const
{
	double ysz = 2.0;
	double xsz = aspect * ysz;

	double px = ((double)x / (double)opt.width) * xsz - xsz / 2.0;
	double py = 1.0 - ((double)y / (double)opt.height) * ysz;

	if(sub > 0) {
		px += xsz * (frand(1.0) - 0.5) / (double)opt.width;
		py += ysz * (frand(1.0) - 0.5) / (double)opt.height;
	}

	Ray ray(Vector3(0, 0, 0), Vector3(px, py, 1.0 / tan(0.5 * vfov)));
	ray.dir *= max_dist;
	ray.time = time;

	// for motion blur, use a random time within the frame interval
	if(opt.mblur && sub > 0) {
		ray.time += rand() % shutter - shutter / 2;
	}

	return ray.transformed(get_matrix(ray.time));
}


TargetCamera::TargetCamera()
{
	set_position(Vector3(0, 5, 10));
	set_target(Vector3(0, 0, 0));
	set_roll(0.0f);
}

TargetCamera::TargetCamera(const Vector3 &pos, const Vector3 &targ)
{
	set_position(pos);
	set_target(targ);
	set_roll(0.0f);
}

TargetCamera::~TargetCamera() {}

bool TargetCamera::load_xml(struct xml_node *node)
{
	if(!Camera::load_xml(node)) {
		return false;
	}

	// find the <target> tag in <camera>'s children
	node = node->chld;
	while(node) {
		if(strcmp(node->name, "target") == 0) {
			break;
		}
		node = node->next;
	}

	if(!node) {
		fprintf(stderr, "failed to load target camera, no <target> child\n");
		return false;
	}

	// load all target <xform> tags
	node = node->chld;
	while(node) {
		if(strcmp(node->name, "xform") == 0) {
			if(!target.load_xml(node)) {
				return false;
			}
		}
		node = node->next;
	}
	return true;
}

void TargetCamera::set_target(const Vector3 &pos, unsigned int time)
{
	target.set_position(pos, time);
}

Vector3 TargetCamera::get_target(unsigned int time) const
{
	return target.get_position(time);
}

void TargetCamera::set_roll(double roll, unsigned int time)
{
	// XXX piggyback on the unused scale track of the target
	target.set_scaling(Vector3(roll, roll, roll), time);
}

double TargetCamera::get_roll(unsigned int time) const
{
	// XXX piggyback on the unused scale track of the target
	return target.get_scaling(time).x;
}

Matrix4x4 TargetCamera::get_matrix(unsigned int time) const
{
	Vector3 pos = get_position(time);
	Vector3 targ = target.get_position(time);

	Vector3 up(0, 1, 0);
	Vector3 dir = (targ - pos).normalized();
	Vector3 right = cross_product(up, dir).normalized();
	up = cross_product(dir, right);

	Quaternion q(dir, get_roll(time));
	up.transform(q);
	right.transform(q);

	return Matrix4x4(right.x, up.x, dir.x, pos.x,
			right.y, up.y, dir.y, pos.y,
			right.z, up.z, dir.z, pos.z,
			0.0f, 0.0f, 0.0f, 1.0f);
}


static int camera_type(const char *type)
{
	for(int i=0; type_name[i]; i++) {
		if(strcmp(type, type_name[i]) == 0) {
			return i;
		}
	}
	return -1;
}

Camera *create_camera(const char *type_str)
{
	int type = camera_type(type_str);

	switch(type) {
	case CAM_FREE:
		return new Camera;

	case CAM_TARGET:
		return new TargetCamera;

	default:
		break;
	}
	return 0;
}
