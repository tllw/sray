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
#ifndef MESH_H_
#define MESH_H_

#include <vector>
#include "object.h"
#include "octree.h"

struct Vertex {
	Vector3 pos, norm, tang;
	Vector2 tex;
};

class Triangle {
public:
	Vertex v[3];
	Vector3 norm;

	void calc_normal();

	void calc_bounds(AABox *aabb, const Matrix4x4 &xform) const;
	bool in_box(const AABox *box, const Matrix4x4 &xform) const;

	bool intersect(const Ray &ray, SurfPoint *sp) const;
};

/** Triangle mesh class */
class Mesh : public Object {
private:
	std::vector<Triangle> faces;
	Octree<Triangle*> octree;
	bool valid_octree;

	virtual void calc_bounds(AABox *box, int msec) const;

public:
	virtual bool load_xml(struct xml_node *node);

	void add_face(const Triangle &face);

	int get_face_count() const;
	const Triangle *get_face(int n) const;

	void build_tree(void);
	Octree<Triangle*> *get_tree();
	const Octree<Triangle*> *get_tree() const;

	virtual bool in_box(const AABox *box, int msec) const;

	virtual bool intersect(const Ray &ray, SurfPoint *sp) const;
};

#endif	// MESH_H_
