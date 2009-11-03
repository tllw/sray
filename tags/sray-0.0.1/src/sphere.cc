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
#include "sphere.h"
#include "octree.h"
#include "scene.h"

Sphere::Sphere(double rad)
{
	this->rad = rad;
}

void Sphere::calc_bounds(AABox *aabb, int msec) const
{
	Vector3 pos = Vector3(0, 0, 0).transformed(get_xform_matrix(msec));

	Vector3 radvec(rad, rad, rad);
	aabb->min = pos - radvec;
	aabb->max = pos + radvec;
}

bool Sphere::load_xml(struct xml_node *node)
{
	if(!Object::load_xml(node)) {
		return false;
	}

	bool res = false;
	node = node->chld;
	while(node) {
		if(strcmp(node->name, "sphere") == 0) {
			struct xml_attr *attr = xml_get_attr(node, "rad");
			if(attr) {
				rad = attr->fval;
			}
			res = true;
		}
		node = node->next;
	}

	return res;
}

void Sphere::set_radius(double rad)
{
	this->rad = rad;
}

double Sphere::get_radius() const
{
	return rad;
}

bool Sphere::in_box(const AABox *box, int msec) const
{
	Vector3 cent = Vector3(0, 0, 0).transformed(get_xform_matrix(msec));
	double d, dmin = 0.0;

	for(int i=0; i<3; i++) {
		if(cent[i] > box->max[i]) {
			d = cent[i] - box->max[i];
			dmin += SQ(d);
		} else if(cent[i] < box->min[i]) {
			d = cent[i] - box->min[i];
			dmin += SQ(d);
		}
	}
	return dmin <= SQ(rad);
}

bool Sphere::intersect(const Ray &wray, SurfPoint *pt) const
{
	// transform ray to local coordinates
	Matrix4x4 inv_mat = get_inv_xform_matrix(wray.time);
	Ray ray = wray.transformed(inv_mat);

	double a = dot_product(ray.dir, ray.dir);
	double b = 2.0 * ray.dir.x * ray.origin.x + 2.0 * ray.dir.y * ray.origin.y +
		2.0 * ray.dir.z * ray.origin.z;
	double c = dot_product(ray.origin, ray.origin) - SQ(rad);
	double d = SQ(b) - 4.0 * a * c;

	if(d < 0.0) {
		return false;
	}

	double sqrt_d = sqrt(d);
	double t1 = (-b + sqrt_d) / (2.0 * a);
	double t2 = (-b - sqrt_d) / (2.0 * a);

	if((t1 < Scene::epsilon && t2 < Scene::epsilon) || (t1 > 1.0 && t2 > 1.0)) {
		return false;
	}

	if(pt) {
		if(t1 < Scene::epsilon) t1 = t2;
		if(t2 < Scene::epsilon) t2 = t1;

		Matrix4x4 xform = get_xform_matrix(ray.time);
		Matrix4x4 inv_trans = inv_mat.transposed();

		pt->dist = t1 < t2 ? t1 : t2;
		
		pt->pos = ray.origin + ray.dir * pt->dist;
		pt->normal = pt->pos / rad;
		pt->tangent = cross_product(Vector3(0, 1, 0), pt->normal);

		SphVector sphv(pt->pos);
		sphv.theta += M_PI;
		pt->texcoord = Vector2(sphv.theta / TWO_PI, sphv.phi / M_PI);

		// transform everything back into world coordinates
		pt->pos.transform(xform);
		pt->normal.transform(inv_trans);
		pt->tangent.transform(inv_trans);
	}
	return true;
}
