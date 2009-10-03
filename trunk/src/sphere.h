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
#ifndef SPHERE_H_
#define SPHERE_H_

#include "object.h"

/** quadratic sphere object */
class Sphere : public Object {
private:
	double rad;

	virtual void calc_bounds(AABox *aabb, int msec) const;

public:
	Sphere(double rad = 1.0);

	virtual bool load_xml(struct xml_node *node);

	void set_radius(double rad);
	double get_radius() const;

	virtual bool in_box(const AABox *box, int msec) const;

	virtual bool intersect(const Ray &ray, SurfPoint *pt) const;
};

#endif	// SPHERE_H_
