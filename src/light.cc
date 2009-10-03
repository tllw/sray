#include "light.h"
#include "octree.h"	// for AABox
#include "object.h"
#include "opt.h"

enum { POINT_LIGHT, SPHERE_LIGHT, BOX_LIGHT };
static const char *type_name[] = {
	"point",
	"sphere",
	"box",
	0
};

Light::Light()
{
	color = Color(1.0, 1.0, 1.0, 1.0);
	att_dist = att_pow = 0.0;
}

Light::~Light() {}

bool Light::load_xml(struct xml_node *node)
{
	if(strcmp(node->name, "light") != 0) {
		return false;
	}

	struct xml_attr *attr;
	if((attr = xml_get_attr(node, "color"))) {
		if(attr->type == ATYPE_VEC) {
			color = Color(attr->vval[0], attr->vval[1], attr->vval[2], 1.0);
		} else if(attr->type == ATYPE_INT || attr->type == ATYPE_FLT) {
			color = Color(attr->fval, attr->fval, attr->fval, 1.0);
		}
	}

	node = node->chld;
	while(node) {
		if(strcmp(node->name, "xform") == 0) {
			if(!XFormNode::load_xml(node)) {
				return false;
			}
		}
		node = node->next;
	}

	return true;
}

bool Light::is_area_light() const
{
	return false;
}

void Light::set_color(const Color &col)
{
	color = col;
}

Color Light::get_color() const
{
	return color;
}

float Light::calc_attenuation(float dist) const
{
	if(att_dist == 0.0 && att_pow == 0.0) {
		return 1.0;
	}
	return 2.0 / (1.0 + pow(dist / att_dist, att_pow));
}

void Light::set_attenuation(float dist, float apow)
{
	att_dist = dist;
	att_pow = apow;
}

float Light::get_attenuation_dist() const
{
	return att_dist;
}

float Light::get_attenuation_power() const
{
	return att_pow;
}

Vector3 Light::get_point(unsigned int msec) const
{
	return get_position(msec);
}

Photon Light::gen_photon(unsigned int msec, unsigned int sampling) const
{
	Photon p;

	p.pos = get_point(msec);
	p.col = color * opt.photon_energy;
	p.type = DIRECT_PHOTON;

	const ProjMap *proj;

	switch(sampling) {
	case SAMPLE_OBJ:
		proj = &projmap;
		p.col *= projmap.get_coverage();
		break;
	case SAMPLE_SPEC_OBJ:
		proj = &spec_projmap;
		p.col *= spec_projmap.get_coverage();
		break;
	default:
		proj = 0;
	}

	// avoid infinite loops in gen_dir
	if(proj && proj->get_coverage() == 0.0) {
		proj = 0;
		p.col = color * opt.photon_energy;
	}

	if(proj) {
		p.dir = proj->gen_dir(true);
	} else {
		p.dir = sphrand(1.0);
	}
	p.dir *= RAY_MAGNITUDE;

	return p;
}


void Light::build_proj_intern(const Object **objarr, int num_obj, int t0, int t1, int light_samples)
{
	int tm[3] = {t0, t1, -1};
	if(t0 == t1) {
		tm[1] = -1;
	}

	projmap.clear();
	spec_projmap.clear();

	AABox *objbox, *caust_objbox;

	objbox = new AABox[num_obj];
	caust_objbox = new AABox[num_obj];

	int tidx = 0;
	while(tm[tidx] != -1) {
		int num_caust = 0;

		for(int i=0; i<num_obj; i++) {
			const Object *obj = objarr[i];

			// find out if it's an object capable of producing caustics
			const Material *mat = obj->get_material();
			MatAttrib attr_refl = mat->get_attribute("reflect");
			MatAttrib attr_refr = mat->get_attribute("refract");
			bool spec_obj = attr_refl.col.x > 0.0 || attr_refr.col.x > 0.0;

			obj->get_bounds(objbox + i, tm[tidx]);
			if(spec_obj) {
				caust_objbox[num_caust++] = objbox[i];
			}
		}

		for(int i=0; i<light_samples; i++) {
			Vector3 lpos = get_point(tm[tidx]);
			::build_projmap(&projmap, lpos, objbox, num_obj);
			::build_projmap(&spec_projmap, lpos, caust_objbox, num_caust);
		}

		tidx++;
	}

	delete [] objbox;
	delete [] caust_objbox;

	if(opt.verb) {
		printf("   coverage: proj(%f) spec_proj(%f)\n", projmap.get_coverage(), spec_projmap.get_coverage());
	}
}

void Light::build_projmap(const Object **objarr, int num_obj, int t0, int t1)
{
	build_proj_intern(objarr, num_obj, t0, t1, 1);	// 1 sample
}

const ProjMap *Light::get_projmap() const
{
	return &projmap;
}

ProjMap *Light::get_projmap()
{
	return &projmap;
}

const ProjMap *Light::get_spec_projmap() const
{
	return &spec_projmap;
}

ProjMap *Light::get_spec_projmap()
{
	return &spec_projmap;
}

// ----- spherical area light ------

SphLight::SphLight()
{
	radius = 1.0;
}

SphLight::~SphLight() {}

bool SphLight::load_xml(struct xml_node *node)
{
	if(!Light::load_xml(node)) {
		return false;
	}

	node = node->chld;
	while(node) {
		if(strcmp(node->name, "sphere") == 0) {
			struct xml_attr *attr;
			if(!(attr = xml_get_attr(node, "rad"))) {
				return false;
			}
			radius = attr->fval;
		}
		node = node->next;
	}

	return true;
}

bool SphLight::is_area_light() const
{
	return true;
}

void SphLight::set_radius(float rad)
{
	radius = rad;
}

float SphLight::get_radius() const
{
	return radius;
}

Vector3 SphLight::get_point(unsigned int msec) const
{
	return get_position(msec) + sphrand(radius);
}


/*Photon SphLight::gen_photon(unsigned int msec, unsigned int sampling) const
{
	Photon p;

	Vector3 lpos = get_position(msec);
	Vector3 norm = sphrand(1.0);

	p.pos = lpos + norm * radius;
	p.dir = sphrand(RAY_MAGNITUDE);

	if(dot_product(p.dir, norm) < 0.0) {
		p.dir = -p.dir;
	}

	p.col = color;
	p.type = DIRECT_PHOTON;
	return p;
}*/

void SphLight::build_projmap(const Object **objarr, int num_obj, int t0, int t1)
{
	build_proj_intern(objarr, num_obj, t0, t1, 8);	// 8 samples ?
}


BoxLight::BoxLight()
{
	dim = Vector3(1.0, 1.0, 1.0);
}

BoxLight::~BoxLight() {}

bool BoxLight::load_xml(struct xml_node *node)
{
	if(!Light::load_xml(node)) {
		return false;
	}

	node = node->chld;
	while(node) {
		if(strcmp(node->name, "aabox") == 0) {
			AABox box;
			if(!box.load_xml(node)) {
				return false;
			}
			dim = box.max - box.min;
		}
		node = node->next;
	}

	return true;
}

bool BoxLight::is_area_light() const
{
	return true;
}

void BoxLight::set_dimensions(const Vector3 &dim)
{
	this->dim = dim;
}

Vector3 BoxLight::get_dimensions() const
{
	return dim;
}

Vector3 BoxLight::get_point(unsigned int msec) const
{
	Vector3 v;
	v.x = frand(dim.x) - 0.5 * dim.x;
	v.y = frand(dim.y) - 0.5 * dim.y;
	v.z = frand(dim.z) - 0.5 * dim.z;

	return v.transformed(get_xform_matrix());
}

// TODO implement this
/*Photon BoxLight::gen_photon(unsigned int msec, unsigned int sampling) const
{
}
*/

static int light_type(const char *name)
{
	for(int i=0; type_name[i]; i++) {
		if(strcmp(name, type_name[i]) == 0) {
			return i;
		}
	}
	return -1;
}

Light *create_light(const char *type_str)
{
	int type = light_type(type_str);

	switch(type) {
	case POINT_LIGHT:
		return new Light;

	case SPHERE_LIGHT:
		return new SphLight;

	case BOX_LIGHT:
		return new BoxLight;

	default:
		break;
	}
	return 0;
}

void build_projmap(ProjMap *map, const Vector3 &lpos, AABox *objbox, int nobj)
{
	for(int i=0; i<nobj; i++) {
		// if the light is contained inside the bounding box mark all cells as full
		if(objbox[i].contains(lpos)) {
			map->set_all_cells(true);
			return;
		}

		Vector3 bsph_center = (objbox[i].min + objbox[i].max) / 2.0;
		double bsph_rad_sq = (objbox[i].min - bsph_center).length_sq();

		// check which cell extends to the center of the sphere
		Vector3 v = bsph_center - lpos;
		double objdist = v.length();

		int uc, vc;
		map->cell_from_dir(v, &uc, &vc);
		map->set_cell(uc, vc, true);

		/* find out if the vectors extended from the corners of each cell
		 * intersect the sphere or not.
		 */
		int num_cells = map->get_subdiv();
		for(int j=0; j<num_cells; j++) {
			map->cell_coords(j, &uc, &vc);

			for(int k=0; k<4; k++) {
				Vector3 corner_dir = map->cell_corner_dir(uc, vc, k);
				corner_dir *= objdist;	// extend to the distance of the object's .. "orbit"

				// check the distance from the center of the bounding sphere
				double dist_sq = ((lpos + corner_dir) - bsph_center).length_sq();
				if(dist_sq <= bsph_rad_sq) {
					map->set_cell(uc, vc, true);
					break;
				}
			}
		}
	}
}
