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
