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
#ifndef SHADER_H_
#define SHADER_H_

#include <string>
#include <vmath.h>
#include "color.h"

struct SurfPoint;
class Material;
class MatAttrib;

typedef Color (*ShaderFunc)(const Ray&, const SurfPoint&, const Material*);

ShaderFunc get_shader(const char *sdrname);

Color shade_undef(const Ray &ray, const SurfPoint &sp, const Material *mat);
Color shade_phong(const Ray &ray, const SurfPoint &sp, const Material *mat);

double fresnel(double r, double cosa);

Vector3 get_bump_normal(const Ray &ray, const SurfPoint &sp, const MatAttrib &attr);

#endif	// SHADER_H_
