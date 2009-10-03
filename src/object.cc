#include <limits.h>
#include "object.h"
#include "scene.h"
#include "octree.h"

#include "sphere.h"
#include "cylinder.h"
#include "mesh.h"


enum { OBJ_SPHERE, OBJ_CYLINDER, OBJ_MESH };
static const char *obj_type_name[] = { "sphere", "cylinder", "mesh", 0 };

static int get_obj_type(const char *str);


Object::Object()
{
	bbcache.set_invalid_key(INT_MIN);
	mat = 0;
}

Object::~Object() {}


bool Object::load_xml(struct xml_node *node)
{
	struct xml_attr *attr;
	if((attr = xml_get_attr(node, "name"))) {
		set_name(attr->str);
	}

	node = node->chld;
	while(node) {
		if(strcmp(node->name, "matref") == 0) {
			if(!(attr = xml_get_attr(node, "name"))) {
				fprintf(stderr, "invalid material reference in object %s: no name attribute\n",
						get_name());
				return false;
			}

			Scene *scn = get_scene();
			Material *mat = scn->get_material(attr->str);
			if(!mat) {
				fprintf(stderr, "material %s referenced by object %s does not exist\n",
						attr->str, get_name());
				return false;
			}
			this->mat = mat;

		} else if(strcmp(node->name, "xform") == 0) {
			if(!XFormNode::load_xml(node)) {
				return false;
			}
		}
		node = node->next;
	}

	return true;
}

void Object::set_material(Material *mat)
{
	this->mat = mat;
}

const Material *Object::get_material() const
{
	return mat;
}

Material *Object::get_material()
{
	return mat;
}

#define ITER	3
bool Object::in_box(const AABox *box, int t0, int t1) const
{
	int dt = (t1 - t0) / (ITER - 1);
	for(int i=0; i<ITER; i++) {
		if(in_box(box, t0 + i * dt)) {
			return true;
		}
	}
	return false;
}

void Object::get_bounds(AABox *aabb, int msec) const
{
	if(bbcache.is_valid(msec)) {
		*aabb = bbcache.get_data();
	} else {
		calc_bounds(aabb, msec);
		bbcache.set_data(*aabb, msec);
	}
}

Object *create_object(const char *typestr)
{
	int type = get_obj_type(typestr);

	switch(type) {
	case OBJ_SPHERE:
		return new Sphere;

	case OBJ_CYLINDER:
		return new Cylinder;

	case OBJ_MESH:
		return new Mesh;

	default:
		fprintf(stderr, "unknown object type: %s\n", typestr);
		break;
	}
	return 0;
}

static int get_obj_type(const char *str)
{
	for(int i=0; obj_type_name[i]; i++) {
		if(strcmp(str, obj_type_name[i]) == 0) {
			return i;
		}
	}
	return -1;
}
