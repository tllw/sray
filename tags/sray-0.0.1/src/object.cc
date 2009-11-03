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
