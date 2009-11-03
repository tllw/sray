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
#ifndef CYLINDER_H_
#define CYLINDER_H_

#include "object.h"

/** Perfect finite cylinder object, without caps */
class Cylinder : public Object {
private:
	double radius;
	double start, end;

	virtual void calc_bounds(AABox *aabb, int msec) const;

public:
	Cylinder(double rad = 1.0, double start = -1.0, double end = 1.0);
	virtual ~Cylinder();

	virtual bool load_xml(struct xml_node *node);

	void set_radius(double rad);
	double get_radius() const;

	void set_start(double s);
	double get_start() const;

	void set_end(double e);
	double get_end() const;

	virtual bool in_box(const AABox *box, int msec) const;

	virtual bool intersect(const Ray &ray, SurfPoint *sp) const;
};

#endif	// CYLINDER_H_
