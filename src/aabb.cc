#include <float.h>
#include "aabb.h"
#include "object.h"

AABox::AABox()
{
	min = max = Vector3(0, 0, 0);
}

AABox::AABox(const Vector3 &min, const Vector3 &max)
{
	this->min = min;
	this->max = max;
}

bool AABox::load_xml(struct xml_node *node)
{
	struct xml_attr *attr;

	if(strcmp(node->name, "aabox") != 0) {
		return false;
	}

	if(!(attr = xml_get_attr(node, "min")) || attr->type != ATYPE_VEC) {
		return false;
	}
	min = Vector3(attr->vval[0], attr->vval[1], attr->vval[2]);

	if(!(attr = xml_get_attr(node, "max")) || attr->type != ATYPE_VEC) {
		return false;
	}
	max = Vector3(attr->vval[0], attr->vval[1], attr->vval[2]);
	return true;
}

bool AABox::in_box(const AABox *box) const
{
	if(min.x > box->max.x || box->min.x > max.x) {
		return false;
	}
	if(min.y > box->max.y || box->min.y > max.y) {
		return false;
	}
	if(min.z > box->max.z || box->min.z > max.z) {
		return false;
	}
	return true;
}

bool AABox::contains(const Vector3 &pt) const
{
	if(pt.x >= min.x && pt.y >= min.y && pt.z >= min.z &&
			pt.x < max.x && pt.y < max.y && pt.z < max.z) {
		return true;
	}
	return false;
}

/* ray-aabb intersection test based on:
 * "An Efficient and Robust Ray-Box Intersection Algorithm",
 * Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
 * Journal of graphics tools, 10(1):49-54, 2005
 */
bool AABox::intersect(const Ray &ray, SurfPoint *pt) const
{
	Vector3 bbox[2] = {min, max};
	static const float t0 = 0.0;
	static const float t1 = 1.0;

	int xsign = (int)(ray.dir.x < 0.0);
	float invdirx = 1.0 / ray.dir.x;
	float tmin = (bbox[xsign].x - ray.origin.x) * invdirx;
	float tmax = (bbox[1 - xsign].x - ray.origin.x) * invdirx;

	int ysign = (int)(ray.dir.y < 0.0);
	float invdiry = 1.0 / ray.dir.y;
	float tymin = (bbox[ysign][1] - ray.origin.y) * invdiry;
	float tymax = (bbox[1 - ysign][1] - ray.origin.y) * invdiry;

	if((tmin > tymax) || (tymin > tmax)) {
		return false;
	}

	if(tymin > tmin) tmin = tymin;
	if(tymax < tmax) tmax = tymax;

	int zsign = (int)(ray.dir.z < 0.0);
	float invdirz = 1.0 / ray.dir.z;
	float tzmin = (bbox[zsign][2] - ray.origin.z) * invdirz;
	float tzmax = (bbox[1 - zsign][2] - ray.origin.z) * invdirz;

	if((tmin > tzmax) || (tzmin > tmax)) {
		return false;
	}

	if(tzmin > tmin) tmin = tzmin;
	if(tzmax < tmax) tmax = tzmax;

	bool res = (tmin < t1) && (tmax > t0);

	if(res && pt) {
		printf("warning do you really need to calculate this?\n");
		pt->dist = tmin;
		pt->pos = ray.origin + ray.dir * pt->dist;
		// TODO normal (if it's any use)
	}
	return res;
}
