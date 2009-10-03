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
#include <algorithm>
#include <limits.h>
#include <pthread.h>
#include "anim.h"

using namespace std;

XFormNode::XFormNode()
{
	parent = 0;
	name = 0;

	xform_cache.set_invalid_key(INT_MIN);

	reset_xform();
}

XFormNode::XFormNode(const XFormNode &node)
{
	*this = node;
}

XFormNode &XFormNode::operator =(const XFormNode &node)
{
	if(this == &node) {
		return *this;
	}

	if(node.name) {
		name = new char[strlen(node.name) + 1];
		strcpy(name, node.name);
	} else {
		name = 0;
	}
	parent = node.parent;
	children = node.children;

	ptrack = node.ptrack;
	strack = node.strack;
	rtrack = node.rtrack;
	pivot = node.pivot;

	return *this;
}

XFormNode::~XFormNode()
{
	delete [] name;
}

bool XFormNode::load_xml(struct xml_node *node)
{
	struct xml_attr *attr;
	int msec = 0;

	if((attr = xml_get_attr(node, "time")) && attr->type == ATYPE_INT) {
		msec = attr->ival;
	}

	if((attr = xml_get_attr(node, "pos")) && attr->type == ATYPE_VEC) {
		Vector3 pos(attr->vval[0], attr->vval[1], attr->vval[2]);
		set_position(pos, msec);
	}
	if((attr = xml_get_attr(node, "rot")) && attr->type == ATYPE_VEC) {
		Quaternion quat(attr->vval[0], attr->vval[1], attr->vval[2], attr->vval[3]);
		set_rotation(quat, msec);
	}
	if((attr = xml_get_attr(node, "axisrot")) && attr->type == ATYPE_VEC) {
		double angle = DEG_TO_RAD(attr->vval[0]);
		Vector3 axis(attr->vval[1], attr->vval[2], attr->vval[3]);
		set_rotation(Quaternion(axis, angle), msec);
	}
	if((attr = xml_get_attr(node, "scale")) && attr->type == ATYPE_VEC) {
		Vector3 scale(attr->vval[0], attr->vval[1], attr->vval[2]);
		set_scaling(scale, msec);
	}
	if((attr = xml_get_attr(node, "pivot")) && attr->type == ATYPE_VEC) {
		Vector3 pivot(attr->vval[0], attr->vval[1], attr->vval[2]);
		set_pivot(pivot);
	}

	return true;
}

void XFormNode::set_interpolator(Interpolator interp)
{
	ptrack.set_interpolator(interp);
	rtrack.set_interpolator(interp);
	strack.set_interpolator(interp);
	xform_cache.invalidate();
}

void XFormNode::set_extrapolator(Extrapolator extrap)
{
	ptrack.set_extrapolator(extrap);
	rtrack.set_extrapolator(extrap);
	strack.set_extrapolator(extrap);
	xform_cache.invalidate();
}

void XFormNode::set_name(const char *name)
{
	delete [] this->name;
	this->name = 0;

	if(name) {
		this->name = new char[strlen(name) + 1];
		strcpy(this->name, name);
	}
}

const char *XFormNode::get_name() const
{
	return name;
}

void XFormNode::add_child(XFormNode *child)
{
	if(find(children.begin(), children.end(), child) == children.end()) {
		child->parent = this;
		child->xform_cache.invalidate();
		children.push_back(child);
	}
}

void XFormNode::remove_child(XFormNode *child)
{
	vector<XFormNode*>::iterator iter;
	iter = find(children.begin(), children.end(), child);
	if(iter != children.end()) {
		(*iter)->xform_cache.invalidate();
		children.erase(iter);
	}
}

XFormNode **XFormNode::get_children()
{
	return &children[0];
}

int XFormNode::get_children_count() const
{
	return (int)children.size();
}

void XFormNode::set_pivot(const Vector3 &p)
{
	pivot = p;
	xform_cache.invalidate();
}

const Vector3 &XFormNode::get_pivot() const
{
	return pivot;
}

void XFormNode::reset_position()
{
	ptrack.reset(Vector3(0, 0, 0));
	xform_cache.invalidate();
}

void XFormNode::reset_rotation()
{
	rtrack.reset(Quaternion());
	xform_cache.invalidate();
}

void XFormNode::reset_scaling()
{
	strack.reset(Vector3(1, 1, 1));
	xform_cache.invalidate();
}

void XFormNode::reset_xform()
{
	reset_position();
	reset_rotation();
	reset_scaling();
}
	
void XFormNode::set_position(const Vector3 &pos, int time)
{
	TrackKey<Vector3> *key = ptrack.get_key(time);
	if(key) {
		key->val = pos;
	} else {
		ptrack.add_key(TrackKey<Vector3>(pos, time));
	}

	if(xform_cache.is_valid(time)) {
		xform_cache.invalidate();
	}
}

static void set_rotation_quat(Track<Quaternion> *track, const Quaternion &rot, int time)
{
	TrackKey<Quaternion> *key = track->get_key(time);
	if(key) {
		key->val = rot;
	} else {
		track->add_key(TrackKey<Quaternion>(rot, time));
	}
}

void XFormNode::set_rotation(const Quaternion &rot, int time)
{
	set_rotation_quat(&rtrack, rot, time);

	if(xform_cache.is_valid(time)) {
		xform_cache.invalidate();
	}
}

void XFormNode::set_rotation(const Vector3 &euler, int time)
{
	Quaternion xrot, yrot, zrot;
	xrot.set_rotation(Vector3(1, 0, 0), euler.x);
	yrot.set_rotation(Vector3(0, 1, 0), euler.y);
	zrot.set_rotation(Vector3(0, 0, 1), euler.z);
	set_rotation_quat(&rtrack, xrot * yrot * zrot, time);

	if(xform_cache.is_valid(time)) {
		xform_cache.invalidate();
	}
}

void XFormNode::set_rotation(double angle, const Vector3 &axis, int time)
{
	set_rotation_quat(&rtrack, Quaternion(axis, angle), time);

	if(xform_cache.is_valid(time)) {
		xform_cache.invalidate();
	}
}

void XFormNode::set_scaling(const Vector3 &s, int time)
{
	TrackKey<Vector3> *key = strack.get_key(time);
	if(key) {
		key->val = s;
	} else {
		strack.add_key(TrackKey<Vector3>(s, time));
	}

	if(xform_cache.is_valid(time)) {
		xform_cache.invalidate();
	}
}
	
void XFormNode::translate(const Vector3 &pos, int time)
{
	TrackKey<Vector3> *key = ptrack.get_key(time);
	if(key) {
		key->val += pos;
	} else {
		ptrack.add_key(TrackKey<Vector3>(pos, time));
	}

	if(xform_cache.is_valid(time)) {
		xform_cache.invalidate();
	}
}

void XFormNode::rotate(const Quaternion &rot, int time)
{
	TrackKey<Quaternion> *key = rtrack.get_key(time);
	if(key) {
		key->val = rot * key->val;
	} else {
		rtrack.add_key(TrackKey<Quaternion>(rot, time));
	}

	if(xform_cache.is_valid(time)) {
		xform_cache.invalidate();
	}
}

void XFormNode::rotate(const Vector3 &euler, int time)
{
	Quaternion xrot, yrot, zrot;
	xrot.set_rotation(Vector3(1, 0, 0), euler.x);
	yrot.set_rotation(Vector3(0, 1, 0), euler.y);
	zrot.set_rotation(Vector3(0, 0, 1), euler.z);
	rotate(xrot * yrot * zrot, time);
}

void XFormNode::rotate(double angle, const Vector3 &axis, int time)
{
	rotate(Quaternion(axis, angle), time);
}

void XFormNode::scale(const Vector3 &s, int time)
{
	TrackKey<Vector3> *key = strack.get_key(time);
	if(key) {
		key->val *= s;
	} else {
		strack.add_key(TrackKey<Vector3>(s, time));
	}

	if(xform_cache.is_valid(time)) {
		xform_cache.invalidate();
	}
}

Vector3 XFormNode::get_position(int time) const
{
	Vector3 pos = ptrack(time);
	if(parent) {
		Vector3 parpos = parent->get_position(time);
		Quaternion parrot = get_rotation(time);

		pos -= parpos;
		pos.transform(parrot.conjugate());
		pos += parpos + parpos.transformed(parrot.conjugate());
		pos *= parent->get_scaling(time);
	}
	return pos;
}

Vector3 XFormNode::get_local_position(int time) const
{
	return ptrack(time);
}

Quaternion XFormNode::get_local_rotation(int time) const
{
	return rtrack(time);
}

Vector3 XFormNode::get_local_scaling(int time) const
{
	return strack(time);
}

Quaternion XFormNode::get_rotation(int time) const
{
	if(parent) {
		return parent->get_rotation(time) * rtrack(time);
	}
	return rtrack(time);
}

Vector3 XFormNode::get_scaling(int time) const
{
	if(parent) {
		return strack(time) * parent->get_scaling(time);
	}
	return strack(time);
}

Matrix4x4 XFormNode::get_xform_matrix(int time) const
{
	if(xform_cache.is_valid(time)) {
		return xform_cache.get_data().xform;
	}
	
	XFormCache xfc;
	Matrix4x4 trans_mat, rot_mat, scale_mat, pivot_mat, neg_pivot_mat;
	
	Vector3 pos = ptrack(time);
	Quaternion rot = rtrack(time);
	Vector3 scale = strack(time);

	pivot_mat.set_translation(pivot);
	neg_pivot_mat.set_translation(-pivot);

	trans_mat.set_translation(pos);
	rot_mat = (Matrix4x4)rot.get_rotation_matrix();
	scale_mat.set_scaling(scale);

	xfc.xform = pivot_mat * trans_mat * rot_mat * scale_mat * neg_pivot_mat;
	if(parent) {
		xfc.xform = parent->get_xform_matrix(time) * xfc.xform;
	}

	xfc.inv_xform = xfc.xform.inverse();
	xform_cache.set_data(xfc, time);
	return xfc.xform;
}

Matrix4x4 XFormNode::get_inv_xform_matrix(int time) const
{
	if(!xform_cache.is_valid(time)) {
		get_xform_matrix(time);	// calculate invxform
	}
	return xform_cache.get_data().inv_xform;
}

Matrix3x3 XFormNode::get_rot_matrix(int time) const
{
	Quaternion rot = get_rotation(time);
	return rot.get_rotation_matrix();
}
