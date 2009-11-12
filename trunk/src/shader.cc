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
#include <algorithm>
#include <assert.h>
#include "shader.h"
#include "object.h"
#include "material.h"
#include "scene.h"

static void calc_lighting(Color *diff, Color *spec, const Scene *scn, const Light *lt,
		double shininess, const Vector3 &pt, const Vector3 &norm, const Vector3 &vdir, int tm);

ShaderFunc get_shader(const char *sdrname)
{
	if(strcmp(sdrname, "phong") == 0) {
		return shade_phong;
	}
	return shade_undef;
}

Color shade_undef(const Ray &ray, const SurfPoint &sp, const Material *mat)
{
	Scene *scn = get_scene();

	int x = (int)(sp.texcoord.x * 20.0);
	int y = (int)(sp.texcoord.y * 10.0);
	Color texcol = ((x % 2) == (y % 2)) ? Color(1.0, 0.3, 0.2, 1.0) : Color(0.2, 0.3, 1.0, 1.0);

	Color diff(0.1, 0.1, 0.1, 1.0), spec(0, 0, 0, 1.0);

	int num_lt = scn->get_light_count();
	for(int i=0; i<num_lt; i++) {
		Light *lt = scn->get_lights()[i];
		Vector3 lpos = lt->get_point(ray.time);
		Vector3 ldir = lpos - sp.pos;
		double ldist = ldir.length();

		Ray shadow_ray(sp.pos, ldir);
		shadow_ray.time = ray.time;
		ldir /= ldist;

		Vector3 vdir = -ray.dir.normalized();
		Vector3 hdir = (ldir + vdir).normalized();

		if(!scn->cast_ray(shadow_ray)) {
			double ndotl = std::max<double>(dot_product(sp.normal, ldir), 0.0);
			double ndoth = std::max<double>(dot_product(sp.normal, hdir), 0.0);
			double att = lt->calc_attenuation(ldist);

			diff += ndotl * lt->get_color() * att;
			spec += pow(ndoth, 50.0) * lt->get_color() * att;
		}
	}

	return diff * texcol + spec;
}

/* This is a hack. when spawning diffuse rays, iter will be set to INT_MAX by calling
 * the DIFFUSE_RAY macro.
 * When the shader, encounters a ray with this iter value, will use the approximate
 * radiance estimation, based on the global photon map, instead of spawning any more
 * rays including reflection, refraction, and shadow rays.
 */

#define DIFFUSE_RAY(ray)		((ray).iter = INT_MAX)
#define IS_DIFFUSE_RAY(ray)		((ray).iter == INT_MAX)

Color shade_phong(const Ray &ray, const SurfPoint &sp, const Material *mat)
{
	bool entering;
	Vector3 normal;

	Scene *scn = get_scene();

	// --- determine if the normal is pointing correctly ---
	if(dot_product(ray.dir, sp.normal) > 0.0) {
		normal = -sp.normal;
		entering = false;
	} else {
		normal = sp.normal;
		entering = true;
	}

	// calculate normalized incident ray direction vector
	double ray_mag = ray.dir.length();	// XXX we could skip this and use RAY_MAGNITUDE
	Vector3 incident = ray.dir / ray_mag;


	// first retrieve all material properties
	Color ks(0, 0, 0, 1.0), ke(0, 0, 0, 1.0);
	double refl_fact = 0.0, trans_fact = 0.0;

	Color kd = mat->get_color("diffuse", sp.texcoord, ray.time);
	if(mat->have_attribute("specular")) {
		ks = mat->get_color("specular", sp.texcoord, ray.time);
	}
	double shininess = mat->get_value("shininess", sp.texcoord, ray.time);

	/* --- approximate radiance evaluation ---
	 * if this is a diffuse ray (i.e. a ray which has been reflected diffusely before)
	 * use the approximate radiance estimation directly from the global photon map, instead
	 * of calculating direct and indirect lighting by shooting sampling rays.
	 */
	if(IS_DIFFUSE_RAY(ray)) {
		PhotonMap *gi_map = scn->get_gi_map();
		Color rad = gi_map->radiance_est(sp.pos, normal, incident, scn->get_gather_dist());
		return rad * kd;
	}
	
	// --- accurate radiance evaluation ---

	if(mat->have_attribute("reflect")) {
		refl_fact = mat->get_value("reflect", sp.texcoord, ray.time);
	}
	if(mat->have_attribute("refract")) {
		trans_fact = mat->get_value("refract", sp.texcoord, ray.time);
	}
	double mat_ior = mat->get_value("ior", sp.texcoord, ray.time);
	double ior = ray.calc_ior(entering, mat_ior);

	// if we have a normal map, grab the normal from there.
	if(mat->have_attribute("normal")) {
		normal = get_bump_normal(ray, sp, mat->get_attribute("normal"));
	}

	// Direct illumination: accumulate radiance from all light sources
	Color diff = scn->get_env_ambient() * kd;
	Color spec(0, 0, 0, 1.0);

	int num_lt = scn->get_light_count();
	for(int i=0; i<num_lt; i++) {
		Light *lt = scn->get_lights()[i];

		Color d, s;
		calc_lighting(&d, &s, scn, lt, shininess, sp.pos, normal, incident, ray.time);

		diff += d;
		spec += s;
	}

	// Global illumination: diffuse hemisphere sampling
	Color gi;
	if(opt.gi_photons) {
		for(int i=0; i<opt.diffuse_samples; i++) {
			double ndotl;

			/* TODO instead of randomly sampling the hemisphere, use the global
			 * photon map to do importance sampling.
			 */
			Vector3 dir = sphrand(1.0);
			if((ndotl = dot_product(dir, normal)) < 0.0) {
				dir = -dir;
			}

			double diff_energy = ndotl * ray.energy;

			if(diff_energy > opt.min_energy) {
				Ray diff_ray = ray;
				DIFFUSE_RAY(diff_ray);	// mark as diffuse
				diff_ray.energy = diff_energy;
				diff_ray.origin = sp.pos;
				diff_ray.dir = dir * ray_mag;

				gi += scn->trace_ray(diff_ray) * ndotl;
			}
		}
		if(opt.diffuse_samples > 1) {
			gi /= (double)opt.diffuse_samples;
		}
	}


	Color refl, refr;

	// Global illumination: calc specular reflection by shooting reflection ray(s).
	double refl_energy = refl_fact * ray.energy;
	if(refl_energy > opt.min_energy && ray.iter > 0) {
		Ray refl_ray = ray;
		refl_ray.energy = refl_energy;
		refl_ray.iter--;
		refl_ray.origin = sp.pos;
		refl_ray.dir = refl_ray.dir.reflection(normal);
		// TODO calc glossy dir by sampling the specular lobe

		refl = scn->trace_ray(refl_ray) * refl_fact;
	}

	// Global illumination: calc specular transmission by shooting refraction ray(s).
	double trans_energy = trans_fact * ray.energy;
	if(trans_energy > opt.min_energy && ray.iter > 0) {
		Ray trans_ray = ray;
		
		if(entering) {
			trans_ray.enter(mat_ior);
		} else {
			trans_ray.leave();
		}

		trans_ray.energy = trans_energy;
		trans_ray.iter--;
		trans_ray.origin = sp.pos;
		trans_ray.dir = (ray.dir / ray_mag).refraction(normal, ior) * ray_mag;

		// check TIR
		if(dot_product(trans_ray.dir, normal) > 0.0) {
			if(entering) {
				trans_ray.leave();			// we didn't actually enter
			} else {
				trans_ray.enter(mat_ior);	// we didn't actually leave
			}
		}

		refr = scn->trace_ray(trans_ray) * trans_fact;
	}

	// Global Illumination: caustics by estimating irradiance from the caustics photon map
	Color irrad;
	if(opt.caust_photons) {
		PhotonMap *caust_map = scn->get_caust_map();
		irrad = caust_map->irradiance_est(sp.pos, normal, scn->get_gather_dist());
	}

	double ray_dot_n = dot_product(-ray.dir / ray_mag, normal);
	double sqrt_fres_0 = trans_fact > 0.0 ? (ior - 1) / (ior + 1) : 1.0;
	double fres = fresnel(SQ(sqrt_fres_0), ray_dot_n);

	// interpolate between reflection and refraction based on the fresnel factor
	Color reflrefr = lerp(refr, refl, fres);

	// sum all the terms and return the total outgoing radiance.
	return (diff + gi) * kd + (spec + reflrefr) * ks + irrad;
}

/** calculate direct lighting, using the phong reflectance model */
static void calc_lighting(Color *diff, Color *spec, const Scene *scn, const Light *lt, double shininess,
		const Vector3 &pt, const Vector3 &norm, const Vector3 &vdir, int tm)
{
	int num_samples = lt->is_area_light() ? opt.shadow_samples : 1;

	*diff = *spec = Color(0.0, 0.0, 0.0);

	for(int i=0; i<num_samples; i++) {
		Vector3 lpos = lt->get_point(tm);
		Vector3 ldir = lpos - pt;
		double ldist = ldir.length();

		Ray shadow_ray(pt, ldir);
		shadow_ray.time = tm;
		ldir /= ldist;

		if(!scn->cast_ray(shadow_ray)) {
			Vector3 vref = vdir.reflection(norm);
			double ndotl = std::max<double>(dot_product(norm, ldir), 0.0);
			double rdotl = std::max<double>(dot_product(vref, ldir), 0.0);

			Color lcol = lt->get_color() * lt->calc_attenuation(ldist);

			*diff += ndotl * lcol;
			*spec += pow(rdotl, shininess) * lcol;
		}
	}

	if(num_samples > 1) {
		double scale = 1.0 / (double)num_samples;
		*diff *= scale;
		*spec *= scale;
	}
}

double fresnel(double r, double cosa)
{
	double inv_cosa = 1.0 - cosa;
	double inv_cosa_sq = SQ(inv_cosa);

	return r + (1.0 - r) * inv_cosa_sq * inv_cosa_sq * inv_cosa;
}

Vector3 get_bump_normal(const Ray &ray, const SurfPoint &sp, const MatAttrib &attr)
{
	Vector3 normal = sp.normal;
	
	if(attr.tex) {
		Color norm_col = attr.tex->lookup(sp.texcoord, ray.time);
		Vector3 n = norm_col * 2.0 - 1.0;

		n.x *= attr.col.x;
		n.y *= attr.col.x;

		n.normalize();
		Vector3 bitan = cross_product(normal, sp.tangent);

		// transform the normal to world space
		Matrix3x3 tbn(sp.tangent.x, sp.tangent.y, sp.tangent.z,
				bitan.x, bitan.y, bitan.z,
				normal.x, normal.y, normal.z);

		normal = n.transformed(tbn.transposed());
	}

	return normal;
}
