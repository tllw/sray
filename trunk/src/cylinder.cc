#include "cylinder.h"

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

Cylinder::Cylinder(double rad, double start, double end)
{
	radius = rad;
	this->start = start;
	this->end = end;
}

Cylinder::~Cylinder() {}

bool Cylinder::load_xml(struct xml_node *node)
{
	if(!Object::load_xml(node)) {
		return false;
	}

	bool res = false;
	node = node->chld;
	while(node) {
		if(strcmp(node->name, "cylinder") == 0) {
			struct xml_attr *attr;
			
			if((attr = xml_get_attr(node, "rad"))) {
				radius = attr->fval;
			}
			if((attr = xml_get_attr(node, "start"))) {
				start = attr->fval;
			}
			if((attr = xml_get_attr(node, "end"))) {
				end = attr->fval;
			}
			res = true;
		}
		node = node->next;
	}

	return res;
}

/* calculates a bounding box around 2 spheres centered on the cylinder
 * end points, with radii equal to the cylinder's radius.
 */
void Cylinder::calc_bounds(AABox *aabb, int msec) const
{
	Vector3 s1(0, start, 0), s2(0, end, 0);
	Matrix4x4 xform = get_xform_matrix(msec);
	Vector3 radvec(radius, radius, radius);

	s1.transform(xform);
	s2.transform(xform);

	Vector3 s1min = s1 - radvec;
	Vector3 s1max = s1 + radvec;

	Vector3 s2min = s2 - radvec;
	Vector3 s2max = s2 + radvec;

	aabb->min.x = MIN(s1min.x, s2min.x);
	aabb->min.y = MIN(s1min.y, s2min.y);
	aabb->min.z = MIN(s1min.z, s2min.z);
	
	aabb->max.x = MAX(s1max.x, s2max.x);
	aabb->max.y = MAX(s1max.y, s2max.y);
	aabb->max.z = MAX(s1max.z, s2max.z);
}

void Cylinder::set_radius(double rad)
{
	radius = rad;
}

double Cylinder::get_radius() const
{
	return radius;
}

void Cylinder::set_start(double s)
{
	start = s;
}

double Cylinder::get_start() const
{
	return start;
}

void Cylinder::set_end(double e)
{
	end = e;
}

double Cylinder::get_end() const
{
	return end;
}

bool Cylinder::in_box(const AABox *box, int msec) const
{
	AABox cylbb;
	get_bounds(&cylbb, msec);
	return cylbb.in_box(box);
}

bool Cylinder::intersect(const Ray &wray, SurfPoint *sp) const
{
	// transform ray to local coordinates
	Matrix4x4 inv_mat = get_inv_xform_matrix(wray.time);
	Ray ray = wray.transformed(inv_mat);

	double a = SQ(ray.dir.x) + SQ(ray.dir.z);
	double b = 2.0 * ray.dir.x * ray.origin.x + 2.0 * ray.dir.z * ray.origin.z;
	double c = SQ(ray.origin.x) + SQ(ray.origin.z) - SQ(radius);
	double d = SQ(b) - 4.0 * a * c;

	if(d < 0.0) {
		return false;
	}

	double sqrt_d = sqrt(d);
	double t1 = (-b + sqrt_d) / (2.0 * a);
	double t2 = (-b - sqrt_d) / (2.0 * a);

	if((t1 < ERROR_MARGIN && t2 < ERROR_MARGIN) || (t1 > 1.0 && t2 > 1.0)) {
		return false;
	}

	Vector3 pos1 = ray.origin + ray.dir * t1;
	Vector3 pos2 = ray.origin + ray.dir * t2;

	bool valid[2] = {true, true};
	if(t1 < ERROR_MARGIN || pos1.y < start || pos1.y >= end) {
		valid[0] = false;
		t1 = t2;
	}
	if(t2 < ERROR_MARGIN || pos2.y < start || pos2.y >= end) {
		valid[1] = false;
		t2 = t1;
	}

	if(!valid[0] && !valid[1]) {
		return false;
	}

	if(sp) {
		Matrix4x4 xform = get_xform_matrix(ray.time);
		Matrix4x4 inv_trans = inv_mat.transposed();

		sp->dist = t1 < t2 ? t1 : t2;
		
		sp->pos = ray.origin + ray.dir * sp->dist;
		sp->normal = Vector3(1.0, 0.0, 1.0) * sp->pos / radius;
		sp->tangent = cross_product(Vector3(0, 1, 0), sp->normal);

		SphVector sphv(sp->pos);
		sphv.theta += M_PI;
		sp->texcoord = Vector2(sphv.theta / TWO_PI, sp->pos.y);

		// transform everything back into world coordinates
		sp->pos.transform(xform);
		sp->normal.transform(inv_trans);
		sp->tangent.transform(inv_trans);
	}
	return true;
}
