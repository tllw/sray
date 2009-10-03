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
