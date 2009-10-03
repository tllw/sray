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
#include <string.h>
#include <errno.h>
#include <float.h>
#include "scene.h"

using namespace std;

static Object *load_object(struct xml_node *node);
static Light *load_light(struct xml_node *node);
static Camera *load_camera(struct xml_node *node);

static Scene *cur_scene;

double Scene::epsilon = ERROR_MARGIN;

void set_scene(Scene *scn)
{
	cur_scene = scn;
}

Scene *get_scene()
{
	return cur_scene;
}

Material Scene::default_mat;

Scene::Scene()
{
	cam = 0;
	env_ior = 1.0;
	env_ambient = env_color = Color(0, 0, 0, 0);

	if(!cur_scene) cur_scene = this;

	valid_octree = false;

	gather_dist = 0.001;
}

Scene::~Scene()
{
	for(size_t i=0; i<objects.size(); i++) {
		delete objects[i];
	}
	for(size_t i=0; i<lights.size(); i++) {
		delete lights[i];
	}
	for(size_t i=0; i<mat.size(); i++) {
		delete mat[i];
	}

	if(cur_scene == this) {
		cur_scene = 0;
	}
}

bool Scene::load(const char *fname)
{
	FILE *fp;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open file: %s: %s\n", fname, strerror(errno));
		return false;
	}

	bool res = load(fp);
	if(!res) {
		fprintf(stderr, "failed to load scene: %s\n", fname);
	}

	fclose(fp);
	return res;
}

bool Scene::load(FILE *fp)
{
	struct xml_node *xml, *node;

	if(!(xml = xml_read_tree_file(fp))) {
		return false;
	}

	// first load all materials to make the material library
	node = xml->chld;
	while(node) {
		if(strcmp(node->name, "material") == 0) {
			Material *mat = new Material;
			if(!(mat->load_xml(node))) {
				delete mat;
				return false;
			}
			add_material(mat);

		}
		node = node->next;
	}

	// then load everything else
	node = xml->chld;
	while(node) {
		if(strcmp(node->name, "env") == 0) {
			// environmental parameters
			struct xml_attr *attr;

			if((attr = xml_get_attr(node, "ambient"))) {
				if(attr->type == ATYPE_VEC) {
					Color env_amb(attr->vval[0], attr->vval[1], attr->vval[2], 1.0);
					set_env_ambient(env_amb);

				} else if(attr->type == ATYPE_INT || attr->type == ATYPE_FLT) {
					Color env_amb(attr->fval, attr->fval, attr->fval, 1.0);
					set_env_ambient(env_amb);
				}
			}

			if((attr = xml_get_attr(node, "bgcolor")) && attr->type == ATYPE_VEC) {
				Color bg(attr->vval[0], attr->vval[1], attr->vval[2], 1.0);
				set_env_color(bg);
			}

			if((attr = xml_get_attr(node, "ior")) && attr->type == ATYPE_FLT) {
				set_env_ior(attr->fval);
			}

		} else if(strcmp(node->name, "object") == 0) {
			Object *obj = load_object(node);
			if(!obj) {
				return false;
			}
			add_object(obj);

		} else if(strcmp(node->name, "light") == 0) {
			Light *lt = load_light(node);
			if(!lt) {
				return false;
			}
			add_light(lt);

		} else if(strcmp(node->name, "camera") == 0) {
			Camera *cam = load_camera(node);
			if(!cam) {
				return false;
			}

			const Camera *prev_cam = get_camera();
			if(prev_cam) {
				fprintf(stderr, "warning: more than one cameras specified\n");
			} else {
				set_camera(cam);
			}
		}

		node = node->next;
	}

	printf("loaded\n");
	printf("%d objects\n", (int)objects.size());
	printf("%d lights\n", (int)lights.size());
	printf("%d materials\n", (int)mat.size());

	xml_free_tree(xml);
	return true;
}


void Scene::add_object(Object *obj)
{
	objects.push_back(obj);
	valid_octree = false;
}

void Scene::add_light(Light *lt)
{
	lights.push_back(lt);
}

void Scene::add_material(Material *mat)
{
	this->mat.push_back(mat);
}

Material *Scene::get_material(const char *name)
{
	for(size_t i=0; i<mat.size(); i++) {
		if(strcmp(name, mat[i]->get_name()) == 0) {
			return mat[i];
		}
	}
	return 0;
}

void Scene::set_camera(Camera *cam)
{
	this->cam = cam;
}

const Camera *Scene::get_camera() const
{
	return cam;
}

Camera *Scene::get_camera()
{
	return cam;
}

Object * const *Scene::get_objects() const
{
	return &objects[0];
}

Light * const *Scene::get_lights() const
{
	return &lights[0];
}

Material * const *Scene::get_materials() const
{
	return &mat[0];
}

int Scene::get_object_count() const
{
	return objects.size();
}

int Scene::get_light_count() const
{
	return lights.size();
}

int Scene::get_material_count() const
{
	return mat.size();
}

void Scene::set_env_ior(double ior)
{
	env_ior = ior;
}

double Scene::get_env_ior() const
{
	return env_ior;
}

void Scene::set_env_color(const Color &col)
{
	env_color = col;
}

Color Scene::get_env_color() const
{
	return env_color;
}

void Scene::set_env_ambient(const Color &col)
{
	env_ambient = col;
}

Color Scene::get_env_ambient() const
{
	return env_ambient;
}

double Scene::get_gather_dist() const
{
	return gather_dist;
}

/*
#include <set>
static void count_tree_obj(OctNode *node, void *cls)
{
	set<void*> *objset = (set<void*>*)cls;

	list<Object*>::iterator iter = node->objects.begin();
	while(iter != node->objects.end()) {
		objset->insert(*iter++);
	}
}
*/

bool Scene::build_tree(int t0, int t1)
{
	if(t1 == INT_MIN) {
		t1 = t0;
	}

	octree.set_max_depth(opt.scnoct_max_depth);
	octree.set_max_items_per_node(opt.scnoct_max_items);

	octree.clear();

	for(size_t i=0; i<objects.size(); i++) {
		AABox start, end, box;

		objects[i]->get_bounds(&start, t0);
		objects[i]->get_bounds(&end, t1);

		box.min.x = MIN(start.min.x, end.min.x);
		box.min.y = MIN(start.min.y, end.min.y);
		box.min.z = MIN(start.min.z, end.min.z);

		box.max.x = MAX(start.max.x, end.max.x);
		box.max.y = MAX(start.max.y, end.max.y);
		box.max.z = MAX(start.max.z, end.max.z);

		octree.add(box, objects[i]);
	}

	octree.build();
	valid_octree = true;

	const AABox *root_box = &octree.get_root()->box;
	double diag_dist = (root_box->max - root_box->min).length();

	gather_dist = diag_dist * opt.gather_dist;

	return true;
}

struct LightPower {
	double intensity;
	double photon_power;
};

bool Scene::build_photon_maps(int t0, int t1)
{
	if(lights.empty() || (!opt.caust_photons && !opt.gi_photons)) {
		return false;
	}

	if(t1 == INT_MIN) {
		t1 = t0;
	}

	/* assign number of photons to each light source by dividing the total
	 * number of photons between the light sources weighted by their intensity.
	 */
	int num_lights = (int)lights.size();
	double acc_power = 0.0;
	LightPower *ltpow = new LightPower[num_lights];

	// accumulate light intensity over all lights
	for(int i=0; i<num_lights; i++) {
		Color col = lights[i]->get_color();

		// XXX is this correct?
		ltpow[i].intensity = (col.x + col.y + col.z) / 3.0;
		acc_power += ltpow[i].intensity;

		// also, since we're at it, build the projection maps
		if(!objects.empty()) {
			if(opt.verb) {
				printf("building projection maps for light %d\n", i);
			}
			lights[i]->build_projmap((const Object**)get_objects(), (int)objects.size(), t0, t1);
		}
	}

	// calculate the fraction of the total power corresponding to light
	for(int i=0; i<num_lights; i++) {
		ltpow[i].photon_power = ltpow[i].intensity / acc_power;
	}

	int cphot = build_caustics_map(t0, t1, opt.caust_photons, ltpow);
	int gphot = build_global_map(t0, t1, opt.gi_photons, ltpow);

	delete [] ltpow;

	if(opt.verb) {
		printf("caustics photons stored: %d (out of %d shot)\n", cphot, opt.caust_photons);
		printf("gi photons stored: %d (out of %d shot)\n", gphot, opt.gi_photons);
	}
	return true;
}


int Scene::build_caustics_map(int t0, int t1, int num_photons, LightPower *ltpow)
{
	caust_map.clear();

	int stored = 0;

	for(size_t i=0; i<lights.size(); i++) {
		// calculate the number of photons to cast from this source
		int nphot = ceil((double)num_photons * ltpow[i].photon_power);

		if(opt.verb) {
			printf("light %d caustics photons (%d): ", (int)i, nphot);
			fflush(stdout);
		}

		for(int j=0; j<nphot; j++) {
			// generate photon
			int msec = t0 == t1 ? t0 : (rand() % (t1 - t0) + t0);
			Photon p = lights[i]->gen_photon(msec, SAMPLE_SPEC_OBJ);

			Ray ray;
			ray.origin = p.pos;
			ray.dir = p.dir;
			ray.time = msec;
			ray.iter = opt.iter;	// XXX should I use a different limit for photons?

			if(trace_caustics_photon(ray, &p) && p.type == CAUST_PHOTON) {
				caust_map.add_photon(p.pos, p.dir, p.norm, p.col);
				stored++;

				if(opt.verb && (stored & 0xff) == 0) {
					putchar('.');
					fflush(stdout);
				}
			}
		}

		if(opt.verb) putchar('\n');

		if(nphot) caust_map.scale_photon_power(1.0 / (double)nphot);
	}

	return stored;
}

int Scene::build_global_map(int t0, int t1, int num_photons, LightPower *ltpow)
{
	gi_map.clear();

	int stored = 0;
	
	for(size_t i=0; i<lights.size(); i++) {
		// calculate the number of photons to cast from this source
		int nphot = ceil((double)num_photons * ltpow[i].photon_power);

		if(opt.verb) {
			printf("light %d gi photons (%d): ", (int)i, nphot);
			fflush(stdout);
		}

		for(int j=0; j<nphot; j++) {
			// generate photon
			int msec = t0 == t1 ? t0 : (rand() % (t1 - t0) + t0);
			Photon p = lights[i]->gen_photon(msec, SAMPLE_OBJ);

			Ray ray;
			ray.origin = p.pos;
			ray.dir = p.dir;
			ray.time = msec;
			ray.iter = opt.iter;	// XXX should I use a different limit for photons?

			// include all photons in the global photon map
			if(trace_global_photon(ray, &p)) {
				gi_map.add_photon(p.pos, p.dir, p.norm, p.col);
				stored++;

				if(opt.verb && (stored & 0xff) == 0) {
					putchar('.');
					fflush(stdout);
				}
			}
		}

		if(opt.verb) putchar('\n');

		if(nphot) gi_map.scale_photon_power(1.0 / (double)nphot);
	}

	return stored;
}

Octree<Object*> *Scene::get_octree()
{
	return &octree;
}

PhotonMap *Scene::get_caust_map()
{
	return &caust_map;
}

PhotonMap *Scene::get_gi_map()
{
	return &gi_map;
}

Color Scene::trace_ray(const Ray &ray) const
{
	SurfPoint sp;
	Object *obj;

	if((obj = cast_ray(ray, &sp))) {
		Material *mat = obj->get_material();
		if(mat) {
			return obj->get_material()->shade(ray, sp);
		} else {
			return default_mat.shade(ray, sp);
		}
	}
	return env_color;
}

bool Scene::trace_caustics_photon(const Ray &inray, Photon *phot) const
{
	SurfPoint sp;
	Object *obj;

	if(inray.iter <= 0) {
		return false;
	}

	if((obj = cast_ray(inray, &sp))) {
		const Material *mat = obj->get_material();

		Color spec = mat->get_color("specular", sp.texcoord, inray.time);
		double refl = mat->get_value("reflect", sp.texcoord, inray.time);
		double refr = mat->get_value("refract", sp.texcoord, inray.time);
		double mat_ior = mat->get_value("ior", sp.texcoord, inray.time);

		bool entering;
		Vector3 normal;
		double ray_mag = inray.dir.length();	// XXX we could skip this and use ray_magnitude

		// --- determine if the normal is pointing correctly ---
		if(dot_product(inray.dir, sp.normal) > 0.0) {
			normal = -sp.normal;
			entering = false;
		} else {
			normal = sp.normal;
			entering = true;
		}

		double ior = inray.calc_ior(entering, mat_ior);

		double ray_dot_n = dot_product(-inray.dir / ray_mag, normal);
		double sqrt_fres_0 = refr > 0.0 ? (ior - 1) / (ior + 1) : 1.0;
		double fres = fresnel(SQ(sqrt_fres_0), ray_dot_n);

		refl = fres * refl;
		refr = (1.0 - fres) * refr;

		double range = 1.0;//MAX(refl + refr, 1.0);
		double rnum = frand(range);

		if(rnum < refl) {
			// reflect photon
			Ray ray = reflect_ray(inray, normal);
			ray.iter--;
			ray.origin = sp.pos;

			phot->type = CAUST_PHOTON;
			phot->col *= spec;	// XXX is this correct?
			return trace_caustics_photon(ray, phot);

		} else if(rnum < refr) {
			Ray ray = refract_ray(inray, normal, mat_ior, entering, ray_mag);
			ray.origin = sp.pos;

			phot->type = CAUST_PHOTON;
			phot->col *= spec;	// XXX is this correct?
			return trace_caustics_photon(ray, phot);

		} else {
			// store photon
			phot->dir = inray.dir.normalized();
			phot->pos = sp.pos;
			phot->norm = normal;
			return true;
		}
	}

	// if no intersection is found, the photon is discarded
	return false;
}


bool Scene::trace_global_photon(const Ray &inray, Photon *phot) const
{
	SurfPoint sp;
	Object *obj;

	if(inray.iter <= 0) {
		return false;
	}

	Vector3 incident = inray.dir.normalized();

	if((obj = cast_ray(inray, &sp))) {
		Vector3 normal;
		bool entering;
		double ray_mag = inray.dir.length();	// XXX we could skip this and use ray_magnitude

		// --- determine if the normal is pointing correctly ---
		if(dot_product(inray.dir, sp.normal) > 0.0) {
			normal = -sp.normal;
			entering = false;
		} else {
			normal = sp.normal;
			entering = true;
		}

		const Material *mat = obj->get_material();

		Color diff = mat->get_color("diffuse", sp.texcoord, inray.time);
		double diff_avg = (diff.x + diff.y + diff.z) / 3.0;

		Color spec = mat->get_color("specular", sp.texcoord, inray.time);
		double spec_avg = (spec.x + spec.y + spec.z) / 3.0;

		double range = 1.0;
		double rnum = frand(range);

		if(rnum < diff_avg) {
			// diffusely reflect photon
			Vector3 dir = sphrand(1.0);
			// make sure we're on the correct hemisphere
			if(dot_product(dir, normal) < 0.0) {
				dir = -dir;
			}

			// find the dot product of the incident ray with the normal
			double ndotl = dot_product(-incident, normal);

			Ray ray = inray;
			ray.dir = dir * RAY_MAGNITUDE;
			ray.iter--;
			ray.origin = sp.pos;

			phot->type = GI_PHOTON;
			// diff / diff_avg is used to modulate the color without decreasing the overall energy
			phot->col *= ndotl * (diff / diff_avg);
			return trace_global_photon(ray, phot);

		} else if(rnum < diff_avg + spec_avg) {
			// specular interaction (reflection, refraction or specular BRDF)
			double refl = mat->get_value("reflect", sp.texcoord, inray.time);
			double refr = mat->get_value("refract", sp.texcoord, inray.time);
			double mat_ior = mat->get_value("ior", sp.texcoord, inray.time);

			double ior = inray.calc_ior(entering, mat_ior);

			double ray_dot_n = dot_product(-inray.dir / ray_mag, normal);
			double sqrt_fres_0 = refr > 0.0 ? (ior - 1) / (ior + 1) : 1.0;
			double fres = fresnel(SQ(sqrt_fres_0), ray_dot_n);

			refl = fres * refl;
			refr = (1.0 - fres) * refr;

			// decide on the kind of specular interaction using another random variable
			double rnum = frand(1.0);
			if(rnum < refl) {
				// reflect photon
				Ray ray = reflect_ray(inray, normal);
				ray.iter--;
				ray.origin = sp.pos;

				phot->type = CAUST_PHOTON;
				phot->col *= spec;	// XXX is this correct?
				return trace_caustics_photon(ray, phot);

			} else if(rnum < refr) {
				Ray ray = refract_ray(inray, normal, mat_ior, entering, ray_mag);
				ray.origin = sp.pos;

				phot->type = CAUST_PHOTON;
				phot->col *= spec;	// XXX is this correct?
				return trace_caustics_photon(ray, phot);

			} else {
				// reflect by sampling the specular BRDF lobe
				// TODO implement
				return false;
			}
			return false;	// can't happen

		} else {
			// store photon
			phot->dir = incident;
			phot->pos = sp.pos;
			phot->norm = normal;
			return true;
		}
	}

	// if no intersection is found, the photon is discarded
	return false;
}



Object *Scene::cast_ray(const Ray &ray, SurfPoint *sp_ret) const
{
	if(valid_octree) {
		OctItem<Object*> *item = octree.intersect(ray, sp_ret);
		return item ? item->data : 0;
	}

	// otherwise, brute-force it
	SurfPoint sp0;
	sp0.dist = DBL_MAX;
	Object *obj0 = 0;

	for(size_t i=0; i<objects.size(); i++) {
		SurfPoint sp;
		Object *obj = objects[i];

		if(obj->intersect(ray, &sp) && sp.dist > ERROR_MARGIN) {
			if(!sp_ret) return obj;

			if(sp.dist < sp0.dist) {
				sp0 = sp;
				obj0 = obj;
			}
		}
	}

	if(sp_ret) {
		*sp_ret = sp0;
	}
	return obj0;
}


static Object *load_object(struct xml_node *node)
{
	struct xml_attr *attr = xml_get_attr(node, "type");
	if(!attr) {
		fprintf(stderr, "invalid object: no type attribute.\n");
		return 0;
	}

	Object *obj = create_object(attr->str);
	if(!obj) {
		fprintf(stderr, "invalid object: unknown type: %s.\n", attr->str);
		return 0;
	}

	if(!obj->load_xml(node)) {
		delete obj;
		obj = 0;
	}
	return obj;
}

static Light *load_light(struct xml_node *node)
{
	struct xml_attr *attr = xml_get_attr(node, "type");
	if(!attr) {
		fprintf(stderr, "invalid light: no type attribute.\n");
		return 0;
	}

	Light *lt = create_light(attr->str);
	if(!lt) {
		fprintf(stderr, "invalid light: unknown type: %s.\n", attr->str);
		return 0;
	}

	if(!lt->load_xml(node)) {
		delete lt;
		lt = 0;
	}
	return lt;
}

static Camera *load_camera(struct xml_node *node)
{
	struct xml_attr *attr = xml_get_attr(node, "type");
	if(!attr) {
		fprintf(stderr, "invalid camera: no type attribute.\n");
		return 0;
	}

	Camera *cam = create_camera(attr->str);
	if(!cam) {
		fprintf(stderr, "invalid camera: unknown type: %s.\n", attr->str);
		return 0;
	}

	if(!cam->load_xml(node)) {
		delete cam;
		cam = 0;
	}
	return cam;
}

