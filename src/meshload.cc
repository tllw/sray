#include "mesh.h"

struct VertexElement {
	Vector3 v;
	int id;
};

struct FaceRef {
	int vert_idx[3];
	int norm_idx[3];
	int tex_idx[3];
};

enum {
	EL_VERTEX,
	EL_NORMAL,
	EL_TEXCOORD,

	NUM_VERTEX_ELEM
};

static const char *velem_name[] = {
	"vertex",
	"normal",
	"texcoord",
	0
};

static bool operator <(const VertexElement &a, const VertexElement &b);
static int vertex_element(const char *name);
static void read_vertex_element(VertexElement *elem, struct xml_node *node, int cur_idx);
static void read_face(FaceRef *face, struct xml_node *node);


bool Mesh::load_xml(struct xml_node *node)
{
	if(!Object::load_xml(node)) {
		return false;
	}

	// search the children for <mesh>
	node = node->chld;
	while(node) {
		if(strcmp(node->name, "mesh") == 0) {
			break;
		}
		node = node->next;
	}
	if(!node) {
		fprintf(stderr, "<mesh> not found in mesh object %s\n", get_name());
		return false;
	}

	bool ordered[NUM_VERTEX_ELEM] = {true, true, true};
	int cur_idx[NUM_VERTEX_ELEM] = {0};

	std::vector<VertexElement> velem[NUM_VERTEX_ELEM];
	std::vector<FaceRef> faceref;

	node = node->chld;
	while(node) {
		int elem_idx = vertex_element(node->name);

		switch(elem_idx) {
		case EL_VERTEX:
		case EL_NORMAL:
		case EL_TEXCOORD:
			{
				VertexElement elem;
				read_vertex_element(&elem, node, cur_idx[elem_idx]);

				if(elem.id != cur_idx[elem_idx]++) {
					ordered[elem_idx] = false;
				}
				velem[elem_idx].push_back(elem);
			}
			break;

		default:
			if(strcmp(node->name, "face") == 0) {
				FaceRef f;
				read_face(&f, node);

				faceref.push_back(f);
			}
			break;
		}

		node = node->next;
	}

	for(int i=0; i<NUM_VERTEX_ELEM; i++) {
		if(!ordered[i]) {
			std::sort(velem[i].begin(), velem[i].end());
		}
		if(!velem[i].empty() && velem[i].back().id != (int)velem[i].size() - 1) {
			fprintf(stderr, "error loading mesh %s: missing vertex elements\n", get_name());
			return false;
		}
	}

	for(size_t i=0; i<faceref.size(); i++) {
		Triangle tri;

		for(int j=0; j<3; j++) {
			Vertex v;
			v.pos = velem[EL_VERTEX][faceref[i].vert_idx[j]].v;

			if(velem[EL_NORMAL].empty()) {
				v.norm = Vector3(0, 0, 0);
			} else {
				v.norm = velem[EL_NORMAL][faceref[i].norm_idx[j]].v;
			}

			if(velem[EL_TEXCOORD].empty()) {
				v.tex = Vector2(0, 0);
			} else {
				v.tex = velem[EL_TEXCOORD][faceref[i].tex_idx[j]].v;

				// fix negative texture coordinates so that we don't have to check during lookups
				while(v.tex.x < 0.0) v.tex.x += 1.0;
				while(v.tex.y < 0.0) v.tex.y += 1.0;
			}

			tri.v[j] = v;
		}

		tri.calc_normal();
		add_face(tri);
	}

	build_tree();
	return true;
}


static bool operator <(const VertexElement &a, const VertexElement &b)
{
	return a.id < b.id;
}

static int vertex_element(const char *name)
{
	for(int i=0; velem_name[i]; i++) {
		if(strcmp(velem_name[i], name) == 0) {
			return i;
		}
	}
	return -1;
}


static void read_vertex_element(VertexElement *elem, struct xml_node *node, int cur_idx)
{
	struct xml_attr *attr;

	// retreive the element ID number
	if((attr = xml_get_attr(node, "id")) && attr->type == ATYPE_INT) {
		elem->id = attr->ival;
	} else {
		fprintf(stderr, "invalid, or no id in %s element\n", node->name);
		elem->id = cur_idx;
	}

	// retreive the value
	if((attr = xml_get_attr(node, "val")) && attr->type == ATYPE_VEC) {
		elem->v = Vector3(attr->vval[0], attr->vval[1], attr->vval[2]);
	} else {
		fprintf(stderr, "invalid, or no value in %s element\n", node->name);
	}
}

static void read_face(FaceRef *face, struct xml_node *node)
{
	static bool warn_non_triangle;
	int i = 0;
	struct xml_attr *attr;

	node = node->chld;
	while(node && i < 3) {
		if(strcmp(node->name, "vref") != 0) {
			fprintf(stderr, "unknown tag %s in <face>\n", node->name);
			node = node->next;
			continue;
		}

		if((attr = xml_get_attr(node, "vertex")) && attr->type == ATYPE_INT) {
			face->vert_idx[i] = attr->ival;
		} else {
			face->vert_idx[i] = 0;
		}

		if((attr = xml_get_attr(node, "normal")) && attr->type == ATYPE_INT) {
			face->norm_idx[i] = attr->ival;
		} else {
			face->norm_idx[i] = 0;
		}

		if((attr = xml_get_attr(node, "texcoord")) && attr->type == ATYPE_INT) {
			face->tex_idx[i] = attr->ival;
		} else {
			face->tex_idx[i] = 0;
		}

		i++;
		node = node->next;
	}

	if(node && !warn_non_triangle) {
		fprintf(stderr, "warning: only triangle meshes are supported at the moment\n");
		warn_non_triangle = true;
	}
}
