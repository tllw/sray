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
