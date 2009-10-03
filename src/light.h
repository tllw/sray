#ifndef LIGHT_H_
#define LIGHT_H_

#include <vmath.h>
#include "anim.h"
#include "color.h"
#include "pmap.h"
#include "aabb.h"
#include "projmap.h"

enum {
	SAMPLE_UNIFORM,
	SAMPLE_OBJ,
	SAMPLE_SPEC_OBJ
};

class Object;

/** Point light source */
class Light : public XFormNode {
protected:
	Color color;
	float att_dist, att_pow;

	ProjMap projmap, spec_projmap;

	void build_proj_intern(const Object **objarr, int num_obj, int t0, int t1, int light_samples);

public:
	Light();
	virtual ~Light();

	bool load_xml(struct xml_node *node);

	virtual bool is_area_light() const;

	void set_color(const Color &col);
	Color get_color() const;

	float calc_attenuation(float dist) const;

	void set_attenuation(float dist, float apow);
	float get_attenuation_dist() const;
	float get_attenuation_power() const;

	/** return a point in the light source, for point lights it's just its position */
	virtual Vector3 get_point(unsigned int msec = 0) const;

	virtual Photon gen_photon(unsigned int msec = 0, unsigned int sampling = SAMPLE_UNIFORM) const;

	virtual void build_projmap(const Object **objarr, int num_obj, int t0, int t1 = -1);
	const ProjMap *get_projmap() const;
	ProjMap *get_projmap();
	const ProjMap *get_spec_projmap() const;
	ProjMap *get_spec_projmap();
};

/** Spherical light source */
class SphLight : public Light {
protected:
	float radius;

public:
	SphLight();
	virtual ~SphLight();

	bool load_xml(struct xml_node *node);

	virtual bool is_area_light() const;

	void set_radius(float rad);
	float get_radius() const;

	/** return a random point inside the light's spherical volume */
	virtual Vector3 get_point(unsigned int msec = 0) const;

	//virtual Photon gen_photon(unsigned int msec = 0, unsigned int sampling = SAMPLE_UNIFORM) const;

	virtual void build_projmap(const Object **objarr, int num_obj, int t0, int t1 = -1);
};

/** Box light source */
class BoxLight : public Light {
protected:
	Vector3 dim;

public:
	BoxLight();
	virtual ~BoxLight();

	bool load_xml(struct xml_node *node);

	virtual bool is_area_light() const;

	void set_dimensions(const Vector3 &dim);
	Vector3 get_dimensions() const;

	/** return a random point inside the light's cubical volume */
	virtual Vector3 get_point(unsigned int msec = 0) const;

	//virtual Photon gen_photon(unsigned int msec = 0, unsigned int sampling = SAMPLE_UNIFORM) const;
};

/** creates a light of the type specified by the type string */
Light *create_light(const char *type);

void build_projmap(ProjMap *map, const Vector3 &lpos, AABox *objbox, int nobj);

#endif	// LIGHT_H_
