#ifndef OBJECT_H_
#define OBJECT_H_

#include <vmath.h>
#include "anim.h"
#include "material.h"
#include "aabb.h"
#include "cacheman.h"
#include "xmltree.h"

/** Represents a point on the surface of an object. Filled in by the
 * intersection routines with all relevant surface properties at that point.
 */
struct SurfPoint {
	double dist;
	Vector3 pos;
	Vector3 normal;
	Vector3 tangent;
	Vector2 texcoord;
};

/** Object is the base class of all renderable object types. */
class Object : public XFormNode {
protected:
	Material *mat;

	mutable DataCache<AABox, int> bbcache;

	virtual void calc_bounds(AABox *aabb, int msec) const = 0;

public:
	Object();
	virtual ~Object();

	/** load_xml expects a pointer to the <object> subtree of the scene XML
	 * tree, and attempts to initialize the Object instance with the
	 * information contained therein.
	 *
	 * Note that this function must be called through an instance of the
	 * appropriate subclass of Object, based on the type attribute of the
	 * <object> tag. You may use create_object to construct an appropriate
	 * instance by specifying the name of the object class as a string.
	 */
	virtual bool load_xml(struct xml_node *node);

	void set_material(Material *mat);
	const Material *get_material() const;
	Material *get_material();

	virtual bool in_box(const AABox *box, int msec) const = 0;
	virtual bool in_box(const AABox *box, int t0, int t1) const;

	/** retrieve the axis-aligned bounding box of the object at a particular
	 * time. The bounding box is returned through the aabb pointer. Results
	 * are cached, so if this function is called twice in a row with the same
	 * time value, the bounding box is not recalculated.
	 */
	void get_bounds(AABox *aabb, int msec) const;

	/** intersect finds the closest intersection of a ray with the object
	 * and fills in the SurfPoint structure passed through the pt pointer.
	 * \return true if an intersection is found, false otherwise.
	 */
	virtual bool intersect(const Ray &ray, SurfPoint *pt) const = 0;
};

/** creates an object of the type specified by the argument string */
Object *create_object(const char *type);

#endif	// OBJECT_H_
