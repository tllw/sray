#include <algorithm>
#include "material.h"
#include "xmltree.h"

using namespace std;

MatAttrib::MatAttrib()
{
	tex = 0;
}

MatAttrib::MatAttrib(const char *name, const Color &col, Texture *tex)
{
	this->name = name;
	this->col = col;
	this->tex = tex;
}

bool MatAttrib::load_xml(struct xml_node *node)
{
	struct xml_attr *attr;

	if(strcmp(node->name, "mattr") != 0) {
		return false;
	}

	if(!(attr = xml_get_attr(node, "name"))) {
		fprintf(stderr, "invalid material attribute: name missing\n");
		return false;
	}
	name = attr->str;

	if(!(attr = xml_get_attr(node, "value"))) {
		fprintf(stderr, "invalid material attribute: %s: value missing\n", name.c_str());
		return false;
	}
	if(attr->type == ATYPE_VEC) {
		col = Color(attr->vval[0], attr->vval[1], attr->vval[2], 1.0);
	} else if(attr->type == ATYPE_FLT || attr->type == ATYPE_INT) {
		col.x = col.y = col.z = attr->fval;
		col.w = 1.0;
	} else {
		fprintf(stderr, "invalid material attribute: %s: invalid value: %s\n", name.c_str(), attr->str);
		return false;
	}

	if((attr = xml_get_attr(node, "map"))) {
		Texture *tex = new Texture;
		if(!tex->load(attr->str)) {
			fprintf(stderr, "failed to load texture: %s\n", attr->str);
			delete tex;
			tex = 0;
		}
		this->tex = tex;
	}
	return true;
}


MatAttrib Material::def_attr("default attrib", Color(1.0, 1.0, 1.0, 1.0), 0);

Material::Material()
{
	sdr = shade_undef;

	// add reasonable default values to some attributes used by all (most) shaders
	set_attribute("diffuse", Color(1.0, 1.0, 1.0), 0);
	set_attribute("specular", 0.0, 0);
	set_attribute("reflect", 0.0, 0);
	set_attribute("refract", 0.0, 0);
	set_attribute("ior", 1.5, 0);	// glass
}

bool Material::load_xml(struct xml_node *node)
{
	struct xml_attr *attr;

	if(strcmp(node->name, "material") != 0) {
		return false;
	}

	if(!(attr = xml_get_attr(node, "name"))) {
		fprintf(stderr, "invalid material: name attribute not found\n");
		return false;
	}
	set_name(attr->str);

	if((attr = xml_get_attr(node, "shader"))) {
		if((sdr = ::get_shader(attr->str)) == shade_undef) {
			fprintf(stderr, "undefined shader %s in material %s\n", attr->str, get_name());
		}
	}

	node = node->chld;
	while(node) {
		if(strcmp(node->name, "mattr") == 0) {
			MatAttrib mattr;
			if(mattr.load_xml(node)) {
				set_attribute(mattr.name.c_str(), mattr.col, mattr.tex);
			}
		}
		node = node->next;
	}

	std::sort(mattr.begin(), mattr.end());
	return true;
}

void Material::set_name(const char *name)
{
	this->name = name;
}

const char *Material::get_name() const
{
	return name.c_str();
}

void Material::set_shader(ShaderFunc sdr)
{
	this->sdr = sdr ? sdr : shade_undef;
}

ShaderFunc Material::get_shader() const
{
	return sdr;
}

Color Material::shade(const Ray &ray, const SurfPoint &pt) const
{
	return sdr(ray, pt, this);
}

void Material::set_attribute(const char *name, const Color &col, Texture *tex)
{
	MatAttrib *attr = find_attribute(name);
	if(attr) {
		attr->col = col;
		attr->tex = tex;
	} else {
		MatAttrib new_attr(name, col, tex);
		mattr.push_back(new_attr);
		sort(mattr.begin(), mattr.end());
	}
}

const MatAttrib &Material::get_attribute(const char *name) const
{
	const MatAttrib *attr = find_attribute(name);
	return attr ? *attr : def_attr;
}

bool Material::have_attribute(const char *name) const
{
	return find_attribute(name) != 0;
}

MatAttrib *Material::find_attribute(const char *name)
{
	MatAttrib cmpattr(name, 0);

	// this performs binary search
	vector<MatAttrib>::iterator iter;
	iter = lower_bound(mattr.begin(), mattr.end(), cmpattr);

	if(iter != mattr.end() && iter->name == string(name)) {
		return &*iter;
	}
	return 0;
}

const MatAttrib *Material::find_attribute(const char *name) const
{
	MatAttrib cmpattr(name, 0);

	// this performs binary search
	vector<MatAttrib>::const_iterator iter;
	iter = lower_bound(mattr.begin(), mattr.end(), cmpattr);

	if(iter != mattr.end() && iter->name == string(name)) {
		return &*iter;
	}
	return 0;
}

float Material::get_value(const char *name, const Vector2 &tc, unsigned int time) const
{
	return get_attribute(name).get_value(tc, time);
}

Color Material::get_color(const char *name, const Vector2 &tc, unsigned int time) const
{
	return get_attribute(name).get_color(tc, time);
}
