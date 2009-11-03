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
#ifndef SCENE_H_
#define SCENE_H_

#include <vector>
#include <limits.h>
#include "object.h"
#include "light.h"
#include "camera.h"
#include "material.h"
#include "octree.h"
#include "pmap.h"

class Scene;

void set_scene(Scene *scn);
Scene *get_scene();

/** Scene database */
class Scene {
private:
	std::vector<Object*> objects;
	std::vector<Light*> lights;
	Camera *cam;

	static Material default_mat;
	std::vector<Material*> mat;

	double env_ior;
	Color env_color;
	Color env_ambient;

	bool valid_octree;
	Octree<Object*> octree;

	PhotonMap caust_map, gi_map;
	double gather_dist;

	struct LightPower {
		double intensity, photon_power;
	};

	int build_caustics_map(int t0, int t1, int num_photons, LightPower *ltpow);
	int build_global_map(int t0, int t1, int num_photons, LightPower *ltpow);

	bool trace_caustics_photon(const Ray &ray, Photon *phot) const;
	bool trace_global_photon(const Ray &ray, Photon *phot) const;

public:
	static double epsilon;
	static double ray_magnitude;

	Scene();
	~Scene();

	bool load(const char *fname);
	bool load(FILE *fp);

	void add_object(Object *obj);
	void add_light(Light *lt);

	void add_material(Material *mat);
	Material *get_material(const char *name);

	void set_camera(Camera *cam);
	const Camera *get_camera() const;
	Camera *get_camera();

	Object * const *get_objects() const;
	Light * const *get_lights() const;
	Material * const *get_materials() const;

	int get_object_count() const;
	int get_light_count() const;
	int get_material_count() const;

	void set_env_ior(double ior);
	double get_env_ior() const;

	void set_env_color(const Color &col);
	Color get_env_color() const;

	void set_env_ambient(const Color &col);
	Color get_env_ambient() const;

	double get_gather_dist() const;

	/** build_tree creates the octree which is used to accelerate intersection
	 * tests.
	 *
	 * Arguments t0 and t1 specify the time interval in which we're interested
	 * in. The tree is supposed to be rebuild for each frame, and this interval
	 * is supposed to be as long as the shutter speed, to allow for motion blur
	 * by distributing rays throughout the time period when the shutter is open.
	 * 
	 * If motion blur is not used (shutter speed is 0) just pass a single
	 * argument equal to the frame time.
	 */
	bool build_tree(int t0 = 0, int t1 = INT_MIN);

	bool build_photon_maps(int t0 = 0, int t1 = INT_MIN);

	Octree<Object*> *get_octree();
	PhotonMap *get_caust_map();
	PhotonMap *get_gi_map();

	Color trace_ray(const Ray &ray) const;

	Object *cast_ray(const Ray &ray, SurfPoint *sp = 0) const;
};

#endif	// SCENE_H_
