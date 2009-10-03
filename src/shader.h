#ifndef SHADER_H_
#define SHADER_H_

#include <string>
#include <vmath.h>
#include "color.h"

struct SurfPoint;
class Material;

typedef Color (*ShaderFunc)(const Ray&, const SurfPoint&, const Material*);

ShaderFunc get_shader(const char *sdrname);

Color shade_undef(const Ray &ray, const SurfPoint &sp, const Material *mat);
Color shade_phong(const Ray &ray, const SurfPoint &sp, const Material *mat);

double fresnel(double r, double cosa);

#endif	// SHADER_H_
