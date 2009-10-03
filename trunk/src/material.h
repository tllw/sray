#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <vector>
#include "shader.h"
#include "texture.h"
#include "xmltree.h"

/** Each material attribute contains a color and an optional texture. To obtain
 * the final value of each attribute, the color is modulated by the texture.
 */
class MatAttrib {
public:
	std::string name;
	Color col;
	Texture *tex;

	MatAttrib();
	MatAttrib(const char *name, const Color &col, Texture *tex = 0);

	/** expects a pointer to a <mattr> node */
	bool load_xml(struct xml_node *node);

	/** returns the attribute value as a scalar. makes sense for things like
	 * reflectivity, shinniness, roughness, etc.
	 */
	inline float get_value(const Vector2 &tc, unsigned int time = 0) const;

	/** returns the color of the attribute. If the attribute has a texture,
	 * the color is modulated by a texel retreived using the supplied texture
	 * coordinates.
	 */
	inline Color get_color(const Vector2 &tc, unsigned int time = 0) const;

	inline bool operator <(const MatAttrib &rhs) const;
};

/** The Material class contains all properties of a surface, relevant to
 * reflectance calculations. It contains a series of named attributes (see
 * MatAttrib), and a pointer to a shading function.
 */
class Material {
private:
	static MatAttrib def_attr;
	ShaderFunc sdr;
	std::vector<MatAttrib> mattr;
	std::string name;

public:
	Material();

	/** load_xml expects a pointer to the <material> subtree of the scene XML
	 * tree, and attempts to load the material information contained there.
	 */
	bool load_xml(struct xml_node *node);

	void set_name(const char *name);
	const char *get_name() const;

	void set_shader(ShaderFunc sdr);
	ShaderFunc get_shader() const;

	/** The shade function is called by the ray tracer to calculate the color
	 * (irradiance) arriving towards the viewer along a given ray from a
	 * particular point.
	 */
	Color shade(const Ray &ray, const SurfPoint &pt) const;

	void set_attribute(const char *name, const Color &col, Texture *tex = 0);

	/** get_attribute finds the named attribute, and returns a reference to it.
	 * if the attribute does not exist, a reference to a default attribute is
	 * returned instead.
	 */
	const MatAttrib &get_attribute(const char *name) const;

	bool have_attribute(const char *name) const;

	MatAttrib *find_attribute(const char *name);
	const MatAttrib *find_attribute(const char *name) const;

	float get_value(const char *name, const Vector2 &tc, unsigned int time = 0) const;
	Color get_color(const char *name, const Vector2 &tc, unsigned int time = 0) const;
};

#include "material.inl"

#endif	// MATERIAL_H_
