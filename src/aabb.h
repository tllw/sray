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
#ifndef AABB_H_
#define AABB_H_

#include <vmath.h>
#include "xmltree.h"

struct SurfPoint;

/** Axis-aligned bounding box */
class AABox {
public:
	Vector3 min, max;

	AABox();
	AABox(const Vector3 &min, const Vector3 &max);

	/** expects a pointer to an <aabox> XML node */
	bool load_xml(struct xml_node *node);

	bool in_box(const AABox *box) const;

	bool contains(const Vector3 &pt) const;
	bool intersect(const Ray &ray, SurfPoint *pt = 0) const;
};


#endif	// AABB_H_
