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
#ifndef ANIM_H_
#define ANIM_H_

#include <vector>
#include <algorithm>
#include "vmath.h"
#include "cacheman.h"
#include "xmltree.h"

/** these are the available methods of interpolation
 * between keyframes.
 */
enum Interpolator {
	INTERP_STEP,	///< no interpolation
	INTERP_LINEAR,	///< linear interpolation
	INTERP_CUBIC	///< cubic spline interpolation
};

/** these are the available extrapolation methods
 * outside of the defined keyframe range.
 */
enum Extrapolator {
	EXTRAP_CLAMP,		///< use the values of the first and last keyframes
	EXTRAP_REPEAT,		///< repeat the motion outside of the range
	EXTRAP_PINGPONG		///< TODO not implemented
};

/** keyframe class */
template <typename T>
struct TrackKey {
	T val;
	int time;

	TrackKey();
	TrackKey(const T &v, int t = 0);
	TrackKey(const T &v, double t = 0.0f);
	bool operator ==(const TrackKey &k) const;
	bool operator <(const TrackKey &k) const;
};

/* Track containing a number of keys */
template <typename T>
class Track {
private:
	T def_val;
	std::vector<TrackKey<T> > keys;
	Interpolator interp;
	Extrapolator extrap;

	/** find the key nearest (less than or equal) to the specified time */
	TrackKey<T> *get_nearest_key(int time);
	/** performs binary search to find the nearest keyframe to a specific time value */
	TrackKey<T> *get_nearest_key(int start, int end, int time);
	/** given a time in milliseconds, find the previous and next keyframe */
	void get_key_interval(int time, const TrackKey<T> **start, const TrackKey<T> **end) const;
	
public:

	Track();

	/** clear the track and set default interpolator/extrapolator */
	void reset(const T &val);
	
	/** choose interpolation method for values between keyframes */
	void set_interpolator(Interpolator interp);
	Interpolator get_interpolator() const;

	/** choose extrapolation method for values outside of the keyframe range */
	void set_extrapolator(Extrapolator extrap);
	Extrapolator get_extrapolator() const;

	/** add a key to the track */
	void add_key(const TrackKey<T> &key);
	/** get a specific key. no interpolation is performed, returns zero if
	 * there isn't a key at the specified time.
	 */
	TrackKey<T> *get_key(int time);
	void delete_key(int time);

	int get_key_count() const;
	/** retreive the Nth key */
	const TrackKey<T> &get_key_at(int idx) const;
	
	/** perform interpolation/extrapolation to get the value at any given time */
	T operator()(int time) const;
	T operator()(double t) const;
};

struct XFormCache {
	Matrix4x4 xform, inv_xform;
};

/** XFormNode is the parent of any transformable object in the scene.
 * 3D surfaces, cameras, lights, everything derives from this class.
 */
class XFormNode {
private:
	char *name;
	
	Track<Vector3> ptrack, strack;
	Track<Quaternion> rtrack;

	Vector3 pivot;

	XFormNode *parent;
	std::vector<XFormNode*> children;

	mutable DataCache<XFormCache, int> xform_cache;

public:
	XFormNode();
	XFormNode(const XFormNode &node);
	virtual XFormNode &operator =(const XFormNode &node);
	virtual ~XFormNode();

	/** expects a pointer to an <xfrom> node */
	virtual bool load_xml(struct xml_node *node);

	/** choose interpolation method for values between keyframes */
	virtual void set_interpolator(Interpolator interp);
	/** choose extrapolation method for values outside of the keyframe range */
	virtual void set_extrapolator(Extrapolator extrap);

	virtual void set_name(const char *name);
	virtual const char *get_name() const;

	/** add a child to this node. a child inherits its parent's transformation */
	virtual void add_child(XFormNode *child);
	virtual void remove_child(XFormNode *child);

	/** get a pointer to an array of this node's children */
	virtual XFormNode **get_children();
	/** get the number of children */
	virtual int get_children_count() const;

	/** set the center of rotation for this object in local coordinates */
	virtual void set_pivot(const Vector3 &p);
	/** get the center of rotation for this object */
	virtual const Vector3 &get_pivot() const;

	virtual void reset_position();	///< reset position keyframe track
	virtual void reset_rotation();	///< reset rotation keyframe track
	virtual void reset_scaling();	///< reset scaling keyframe track
	virtual void reset_xform();		///< reset all keyframe tracks
	
	/** set a position keyframe */
	virtual void set_position(const Vector3 &pos, int time = 0);
	/** set a rotation keyframe, using a quaternion */
	virtual void set_rotation(const Quaternion &rot, int time = 0);
	/** set a rotation keyframe, using euler angles */
	virtual void set_rotation(const Vector3 &euler, int time = 0);
	/** set a rotation keyframe, using an axis/angle pair */
	virtual void set_rotation(double angle, const Vector3 &axis, int time = 0);
	/** set a scaling keyframe */
	virtual void set_scaling(const Vector3 &s, int time = 0);
	
	/** accumulate a translation */
	virtual void translate(const Vector3 &pos, int time = 0);
	/** concatenate a rotation, using a quaternion */
	virtual void rotate(const Quaternion &rot, int time = 0);
	/** concatenate a rotation, using euler angles */
	virtual void rotate(const Vector3 &euler, int time = 0);
	/** concatenate a rotation, using an axis/angle pair */
	virtual void rotate(double angle, const Vector3 &axis, int time = 0);
	/** modulate the scaling at some point in time */
	virtual void scale(const Vector3 &s, int time = 0);

	/** get the position at any point in time */
	virtual Vector3 get_position(int time = 0) const;
	/** get the rotation at any point in time */
	virtual Quaternion get_rotation(int time = 0) const;
	/** get the scaling at any point in time */
	virtual Vector3 get_scaling(int time = 0) const;

	/** get the local (ignoring hierarchy) position at any point in time */
	virtual Vector3 get_local_position(int time = 0) const;
	/** get the local (ignoring hierarchy) rotation at any point in time */
	virtual Quaternion get_local_rotation(int time = 0) const;
	/** get the local (ignoring hierarchy) scaling at any point in time */
	virtual Vector3 get_local_scaling(int time = 0) const;

	/** get the full transformation matrix for any given time value */
	virtual Matrix4x4 get_xform_matrix(int time = 0) const;
	/** get the full inverse transformation matrix for any given time value */
	virtual Matrix4x4 get_inv_xform_matrix(int time = 0) const;
	/** get the rotation part of the transformation matrix for any given time value */
	virtual Matrix3x3 get_rot_matrix(int time = 0) const;
};

#include "anim.inl"

#endif	// ANIM_H_
