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
