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
#ifndef CAMERA_H_
#define CAMERA_H_

#include <vmath.h>
#include "anim.h"
#include "xmltree.h"

/** The camera class is responsible for generating primary rays */
class Camera : public XFormNode {
private:
	float vfov;
	float aspect;
	float max_dist;
	int shutter;

public:
	Camera();
	virtual ~Camera();

	virtual bool load_xml(struct xml_node *node);

	virtual void set_far_plane(float dist);
	virtual float get_far_plane() const;

	virtual void set_aspect(float aspect);

	/** \param fov vertical field of view in radians */
	virtual void set_vertical_fov(float fov);
	virtual float get_vertical_fov() const;
	
	/** \param fov horizontal field of view in radians */
	virtual void set_horizontal_fov(float fov);
	virtual float get_horizontal_fov() const;

	virtual void set_shutter(int msec);
	virtual int get_shutter() const;

	/** get_matrix calculates and caches the matrix required to bring
	 * primary rays to world space.
	 */
	virtual Matrix4x4 get_matrix(unsigned int time = 0) const;

	/** calculate primary ray in world space for pixel (x,y) subpixel sub
	 * and the specified time. if motion blur is enabled, a random offset
	 * of +/- half a frame is added to the time value.
	 */
	virtual Ray get_primary_ray(int x, int y, int sub, unsigned int time) const;
};

/** TargetCamera can be set up using a position and a target vector,
 * with an optional roll angle.
 */
class TargetCamera : public Camera {
protected:
	XFormNode target;

public:
	TargetCamera();
	TargetCamera(const Vector3 &pos, const Vector3 &targ);
	virtual ~TargetCamera();

	virtual bool load_xml(struct xml_node *node);

	void set_target(const Vector3 &pos, unsigned int time = 0);
	Vector3 get_target(unsigned int time = 0) const;

	void set_roll(float roll, unsigned int time = 0);
	float get_roll(unsigned int time = 0) const;

	/* get_matrix calculates and caches the matrix required to bring
	 * primary rays to world space.
	 */
	virtual Matrix4x4 get_matrix(unsigned int time = 0) const;
};

Camera *create_camera(const char *type_str);

#endif	// CAMERA_H_
