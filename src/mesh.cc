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
#include <float.h>
#include <vmath/vmath.h>
#include "mesh.h"
#include "scene.h"

static inline bool check_clock(const Vector3 &v0, const Vector3 &v1,
		const Vector3 &v2, const Vector3 &normal);

static void interp_face(Vertex *interp, const Triangle &tri, const Vector3 &ispos,
		const Vector3 &normal);


void Triangle::calc_normal()
{
	Vector3 v1 = v[2].pos - v[0].pos;
	Vector3 v2 = v[1].pos - v[0].pos;

	norm = cross_product(v1, v2).normalized();
}

void Triangle::calc_bounds(AABox *aabb, const Matrix4x4 &xform) const
{
	aabb->min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
	aabb->max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for(int i=0; i<3; i++) {
		Vector3 pos = v[i].pos.transformed(xform);

		if(pos.x < aabb->min.x) aabb->min.x = pos.x;
		if(pos.y < aabb->min.y) aabb->min.y = pos.y;
		if(pos.z < aabb->min.z) aabb->min.z = pos.z;

		if(pos.x > aabb->max.x) aabb->max.x = pos.x;
		if(pos.y > aabb->max.y) aabb->max.y = pos.y;
		if(pos.z > aabb->max.z) aabb->max.z = pos.z;
	}
}

bool Triangle::in_box(const AABox *box, const Matrix4x4 &xform) const
{
	Vector3 verts[3];

	// first test if any vertex is inside the box
	for(int i=0; i<3; i++) {
		verts[i] = v[i].pos.transformed(xform);

		if(box->contains(verts[i])) {
			return true;
		}
	}

	// second test, any edges intersect the box quads
	for(int i=0; i<3; i++) {
		Ray ray;
		ray.origin = verts[i];
		ray.dir = verts[i] + verts[(i + 1) % 3];

		if(box->intersect(ray)) {
			return true;
		}
	}

	// third test, any box edges intersect the triangle
	Vector3 boxv[] = {
		Vector3(box->min.x, box->min.y, box->min.z),
		Vector3(box->max.x, box->min.y, box->min.z),
		Vector3(box->max.x, box->max.y, box->min.z),
		Vector3(box->min.x, box->max.y, box->min.z),
		Vector3(box->min.x, box->min.y, box->max.z),
		Vector3(box->max.x, box->min.y, box->max.z),
		Vector3(box->max.x, box->max.y, box->max.z),
		Vector3(box->min.x, box->max.y, box->max.z)
	};

	for(int i=0; i<4; i++) {
		Ray ray;

		ray.origin = boxv[i];
		ray.dir = boxv[(i + 1) & 3] - ray.origin;
		if(intersect(ray, 0)) {
			return true;
		}

		ray.origin = boxv[i + 4];
		ray.origin = boxv[((i + 1) & 3) + 4] - ray.origin;
		if(intersect(ray, 0)) {
			return true;
		}

		ray.origin = boxv[i];
		ray.dir = boxv[i + 4] - ray.origin;
		if(intersect(ray, 0)) {
			return true;
		}
	}

	return false;
}

bool Triangle::intersect(const Ray &ray, SurfPoint *sp) const
{
	Vector3 v1 = v[1].pos - v[0].pos;
	Vector3 v2 = v[2].pos - v[0].pos;
	Vector3 normal = cross_product(v1, v2);

	double n_dot_dir;
	if(fabs(n_dot_dir = dot_product(normal, ray.dir)) < Scene::epsilon) {
		return false;	// parallel to the plane
	}

	// vertex-to-origin
	Vector3 vo_vec = ray.origin - v[0].pos;

	// calc intersection distance
	double t = -dot_product(normal, vo_vec) / n_dot_dir;
	if(t < Scene::epsilon || t > 1.0) {
		return false;	// plane in the opposite subspace or after the end of the ray
	}

	// intersection point
	Vector3 pos = ray.origin + ray.dir * t;

	// check inside/outside
	for(int i=0; i<3; i++) {
		if(!check_clock(v[i].pos, v[(i + 1) % 3].pos, pos, normal)) {
			return false;	// outside of the triangle
		}
	}

	if(sp) {
		sp->dist = t;
		sp->pos = pos;

		Vertex v;
		interp_face(&v, *this, pos, normal);

		sp->texcoord = v.tex;

		if(v.norm.length_sq() < XSMALL_NUMBER) {
			sp->normal = normal;//.normalized();
		} else {
			sp->normal = v.norm;//.normalized();
		}

		sp->tangent = v.tang;
	}
	return true;
}

// ---- mesh ----

void Mesh::calc_bounds(AABox *box, int msec) const
{
	box->min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
	box->max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	Matrix4x4 xform = get_xform_matrix(msec);

	for(size_t i=0; i<faces.size(); i++) {
		AABox fbox;
		faces[i].calc_bounds(&fbox, xform);

		if(fbox.min.x < box->min.x) box->min.x = fbox.min.x;
		if(fbox.min.y < box->min.y) box->min.y = fbox.min.y;
		if(fbox.min.z < box->min.z) box->min.z = fbox.min.z;

		if(fbox.max.x > box->max.x) box->max.x = fbox.max.x;
		if(fbox.max.y > box->max.y) box->max.y = fbox.max.y;
		if(fbox.max.z > box->max.z) box->max.z = fbox.max.z;
	}
}


// if you're looking for Mesh::load_xml, it's implemented in meshload.cc


void Mesh::add_face(const Triangle &face)
{
	faces.push_back(face);
	valid_octree = false;
}

int Mesh::get_face_count() const
{
	return (int)faces.size();
}

const Triangle *Mesh::get_face(int n) const
{
	return &faces[n];
}

void Mesh::build_tree(void)
{
	octree.clear();

	octree.set_max_depth(opt.meshoct_max_depth);
	octree.set_max_items_per_node(opt.meshoct_max_items);

	Matrix4x4 idmat;

	for(size_t i=0; i<faces.size(); i++) {
		AABox box;
		faces[i].calc_bounds(&box, idmat);

		octree.add(box, &faces[i]);
	}

	octree.build();
	valid_octree = true;
}

Octree<Triangle*> *Mesh::get_tree()
{
	return &octree;
}

const Octree<Triangle*> *Mesh::get_tree() const
{
	return &octree;
}

bool Mesh::in_box(const AABox *box, int msec) const
{
	Matrix4x4 xform = get_xform_matrix(msec);

	for(size_t i=0; i<faces.size(); i++) {
		if(faces[i].in_box(box, xform)) {
			return true;
		}
	}
	return false;
}

bool Mesh::intersect(const Ray &wray, SurfPoint *sp) const
{
	SurfPoint sp0;
	sp0.dist = DBL_MAX;
	const Triangle *face0 = 0;

	if(!valid_octree) {
		((Mesh*)this)->build_tree();
	}

	// transform ray to local coordinates
	Matrix4x4 inv_mat = get_inv_xform_matrix(wray.time);
	Ray ray = wray.transformed(inv_mat);

	if(valid_octree) {
		OctItem<Triangle*> *item = octree.intersect(ray, &sp0);
		if(item) {
			face0 = item->data;
		}
	} else {
		static bool warned = false;
		if(!warned) {
			printf("warning: tracing rays through a mesh without an octree (slow)\n");
			warned = true;
		}

		for(size_t i=0; i<faces.size(); i++) {
			SurfPoint sp;
			if(faces[i].intersect(ray, &sp)) {
				if(sp.dist < sp0.dist) {
					sp0 = sp;
					face0 = &faces[i];
				}
			}
		}
	}

	if(!face0) {
		return false;
	}

	if(sp) {
		*sp = sp0;

		Matrix4x4 xform = get_xform_matrix(ray.time);
		Matrix4x4 inv_trans = inv_mat.transposed();

		// transform everything back into world coordinates
		sp->pos.transform(xform);
		sp->normal.transform(inv_trans);
		sp->tangent.transform(inv_trans);

		sp->normal.normalize();
		sp->tangent.normalize();
	}
	return true;
}

static inline bool check_clock(const Vector3 &v0, const Vector3 &v1,
		const Vector3 &v2, const Vector3 &normal)
{
	Vector3 test = cross_product(v1 - v0, v2 - v0);
	return dot_product(test, normal) >= 0.0;
}

#define LERP(a, b, t)	((a) + ((b) - (a)) * (t))

/* see Glassner; ch: 2; section: 3.3 (Eric Haines) */
/* TODO: readify this write-only piece of shit */
static void interp_face(Vertex *interp, const Triangle &tri, const Vector3 &ispos,
		const Vector3 &normal)
{
	Vector2 res;
	Vector3 p00, p01, p11, p10, na, nb, nc, pa, pb, pc, pd;
	double du0, du1, du2, dv0, dv1, dv2;

	double s00, s10, s01, s11, s0, s1;
	double t00, t10, t01, t11, t0, t1;
	double nx00, nx10, nx01, nx11, nx0, nx1;
	double ny00, ny10, ny01, ny11, ny0, ny1;
	double nz00, nz10, nz01, nz11, nz0, nz1;
	double gx00, gx10, gx01, gx11, gx0, gx1;
	double gy00, gy10, gy01, gy11, gy0, gy1;
	double gz00, gz10, gz01, gz11, gz0, gz1;

	p00 = tri.v[0].pos;
	p10 = tri.v[1].pos;
	p01 = p11 = tri.v[2].pos;

	s00 = tri.v[0].tex.x; t00 = tri.v[0].tex.y;
	s10 = tri.v[1].tex.x; t10 = tri.v[1].tex.y;
	s01 = s11 = tri.v[2].tex.x; t01 = t11 = tri.v[2].tex.y;

	nx00 = tri.v[0].norm.x; ny00 = tri.v[0].norm.y; nz00 = tri.v[0].norm.z;
	nx10 = tri.v[1].norm.x; ny10 = tri.v[1].norm.y; nz10 = tri.v[1].norm.z;
	nx01 = nx11 = tri.v[2].norm.x; ny01 = ny11 = tri.v[2].norm.y; nz01 = nz11 = tri.v[2].norm.z;
	
	gx00 = tri.v[0].tang.x; gy00 = tri.v[0].tang.y; gz00 = tri.v[0].tang.z;
	gx10 = tri.v[1].tang.x; gy10 = tri.v[1].tang.y; gz10 = tri.v[1].tang.z;
	gx01 = gx11 = tri.v[2].tang.x; gy01 = gy11 = tri.v[2].tang.y; gz01 = gz11 = tri.v[2].tang.z;

	pa = (p00 - p10) + (p11 - p01);
	pb = p10 - p00;
	pc = p01 - p00;
	pd = p00;

	na = cross_product(pa, normal);
	nb = cross_product(pb, normal);
	nc = cross_product(pc, normal);

	du0 = dot_product(nc, pd);
	du1 = dot_product(na, pd) + dot_product(nc, pb);
	du2 = dot_product(na, pb);

	dv0 = dot_product(nb, pd);
	dv1 = dot_product(na, pd) + dot_product(nb, pc);
	dv2 = dot_product(na, pc);

	if(fabs(du2) <= XSMALL_NUMBER) {
		double divisor = du1 - dot_product(na, ispos);
		res.x = fabs(divisor) > XSMALL_NUMBER ? ((dot_product(nc, ispos) - du0) / divisor) : 0.0;
	} else {
		Vector3 qux, quy;
		double dux, duy, ka, kb, u0, u1;
		
		qux = na / (2.0 * du2);
		quy = -nc / du2;

		dux = -du1 / (2.0 * du2);
		duy = du0 / du2;

		ka = dux + dot_product(qux, ispos);
		kb = duy + dot_product(quy, ispos);

		u0 = ka - sqrt(ka * ka - kb);
		u1 = ka + sqrt(ka * ka - kb);

		res.x = (u0 < 0.0 || u0 > 1.0) ? u1 : u0;
	}

	if(fabs(dv2) <= XSMALL_NUMBER) {
		double divisor = dv1 - dot_product(na, ispos);
		res.y = fabs(divisor) > XSMALL_NUMBER ? ((dot_product(nb, ispos) - dv0) / divisor) : 0.0;
	} else {
		Vector3 qvx, qvy;
		double dvx, dvy, ka, kb, v0, v1;

		qvx = na / (2.0 * dv2);
		qvy = -nb / dv2;

		dvx = -dv1 / (2.0 * dv2);
		dvy = dv0 / dv2;

		ka = dvx + dot_product(qvx, ispos);
		kb = dvy + dot_product(qvy, ispos);

		v0 = ka - sqrt(ka * ka - kb);
		v1 = ka + sqrt(ka * ka - kb);

		res.y = (v0 < 0.0 || v0 > 1.0) ? v1 : v0;
	}

	s0 = LERP(s00, s01, res.y);
	s1 = LERP(s10, s11, res.y);
	interp->tex.x = LERP(s0, s1, res.x);

	t0 = LERP(t00, t01, res.y);
	t1 = LERP(t10, t11, res.y);
	interp->tex.y = LERP(t0, t1, res.x);

	nx0 = LERP(nx00, nx01, res.y);
	nx1 = LERP(nx10, nx11, res.y);
	interp->norm.x = LERP(nx0, nx1, res.x);

	ny0 = LERP(ny00, ny01, res.y);
	ny1 = LERP(ny10, ny11, res.y);
	interp->norm.y = LERP(ny0, ny1, res.x);

	nz0 = LERP(nz00, nz01, res.y);
	nz1 = LERP(nz10, nz11, res.y);
	interp->norm.z = LERP(nz0, nz1, res.x);

	gx0 = LERP(gx00, gx01, res.y);
	gx1 = LERP(gx10, gx11, res.y);
	interp->tang.x = LERP(gx0, gx1, res.x);

	gy0 = LERP(gy00, gy01, res.y);
	gy1 = LERP(gy10, gy11, res.y);
	interp->tang.y = LERP(gy0, gy1, res.x);

	gz0 = LERP(gz00, gz01, res.y);
	gz1 = LERP(gz10, gz11, res.y);
	interp->tang.z = LERP(gz0, gz1, res.x);
}
