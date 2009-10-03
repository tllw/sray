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
