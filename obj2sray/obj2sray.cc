#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <typeinfo>
#include <henge.h>

using namespace henge;

bool write_xml(FILE *fp, scene *scn);

int main(int argc, char **argv)
{
	char path[512], *fname;

	if(argc != 2) {
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		return 1;
	}
	henge::init_no_gl();

	strcpy(path, argv[1]);
	if((fname = strrchr(path, '/'))) {
		*fname++ = 0;
		set_path(path);
	} else {
		fname = path;
		set_path(".");
	}

	scene *scn = new scene;
	if(!scn->load(fname)) {
		fprintf(stderr, "failed to open file: %s/%s: %s\n", path, fname, strerror(errno));
		return 1;
	}

	if(!write_xml(stdout, scn)) {
		return 1;
	}
	return 0;
}

static const char *objname(int idx, robject *obj)
{
	if(obj->get_name()) {
		return obj->get_name();
	}

	static char name[32];
	sprintf(name, "obj%04d", idx);
	return name;
}

bool write_xml(FILE *fp, scene *scn)
{
	robject **obj = scn->get_objects();
	int num_obj = scn->object_count();

	/*light **lt = scn->get_lights();
	int num_lt = scn->light_count();

	camera **cam = scn->get_cameras();
	int num_cam = scn->camera_count();*/

	fprintf(fp, "<scene>\n\t<!-- converted by obj2sray -->\n");

	fprintf(fp, "\n\t<!-- materials -->");

	for(int i=0; i<num_obj; i++) {
		float val;
		color col;
		texture *tex;

		material *mat = obj[i]->get_material_ptr();

		fprintf(fp, "\n\t<material name=\"%s.mat\" shader=\"phong\">\n", objname(i, obj[i]));

		col = mat->get_color(MATTR_DIFFUSE);
		fprintf(fp, "\t\t<mattr name=\"diffuse\" value=\"%.3f %.3f %.3f\"", col.x, col.y, col.z);
		if((tex = mat->get_texture())) {
			const char *texname = get_texture_name(tex);
			if(texname) {
				fprintf(fp, " map=\"%s\"", texname);
			}
		}
		fprintf(fp, "/>\n");

		col = mat->get_color(MATTR_SPECULAR);
		fprintf(fp, "\t\t<mattr name=\"specular\" value=\"%.3f %.3f %.3f\"/>\n", col.x, col.y, col.z);

		col = mat->get_color(MATTR_EMISSION);
		fprintf(fp, "\t\t<mattr name=\"emissive\" value=\"%.3f %.3f %.3f\"/>\n", col.x, col.y, col.z);

		val = mat->get(MATTR_SHININESS);
		fprintf(fp, "\t\t<mattr name=\"shininess\" value=\"%.2f\"/>\n", val);

		val = mat->get(MATTR_ROUGHNESS);
		fprintf(fp, "\t\t<mattr name=\"roughness\" value=\"%.3f\"/>\n", val);

		val = mat->get(MATTR_IOR);
		fprintf(fp, "\t\t<mattr name=\"ior\" value=\"%.3f\"/>\n", val);

		val = mat->get(MATTR_ALPHA);
		fprintf(fp, "\t\t<mattr name=\"refract\" value=\"%.3f\"/>\n", 1.0 - val);

		fprintf(fp, "\t</material>\n");
	}

	fprintf(fp, "\n\t<!-- objects -->");

	for(int i=0; i<num_obj; i++) {
		const char *name = objname(i, obj[i]);
		fprintf(fp, "\n\t<object name=\"%s\" type=\"mesh\">\n", name);
		fprintf(fp, "\t\t<matref name=\"%s.mat\"/>\n", name);

		Vector3 pos = obj[i]->get_local_position();
		fprintf(fp, "\t\t<xform pos=\"%.4f %.4f %.4f\"", pos.x, pos.y, pos.z);
		Quaternion rot = obj[i]->get_local_rotation();
		fprintf(fp, " rot=\"%.4f %.4f %.4f %.4f\"", rot.s, rot.v.x, rot.v.y, rot.v.z);
		Vector3 scale = obj[i]->get_local_scaling();
		fprintf(fp, " scale=\"%.4f %.4f %.4f\"/>\n", scale.x, scale.y, scale.z);

		fprintf(fp, "\t\t<mesh>\n");

		trimesh *mesh = obj[i]->get_mesh();
		
		Vector3 *vert = mesh->get_data_vec3(EL_VERTEX);
		int num_verts = mesh->get_count(EL_VERTEX);

		Vector3 *norm = mesh->get_data_vec3(EL_NORMAL);
		int num_norm = mesh->get_count(EL_NORMAL);
		if(norm) {
			assert(num_norm == num_verts);
		}

		Vector2 *tc = mesh->get_data_vec2(EL_TEXCOORD);
		int num_tc = mesh->get_count(EL_TEXCOORD);
		if(tc) {
			assert(num_tc == num_verts);
		}

		for(int i=0; i<num_verts; i++) {
			fprintf(fp, "\t\t\t<vertex id=\"%d\" val=\"%.4f %.4f %.4f\"/>\n", i, vert[i].x, vert[i].y, -vert[i].z);

			if(norm) {
				fprintf(fp, "\t\t\t<normal id=\"%d\" val=\"%.3f %.3f %.3f\"/>\n", i, norm[i].x, norm[i].y, -norm[i].z);
			}
			if(tc) {
				fprintf(fp, "\t\t\t<texcoord id=\"%d\" val=\"%.3f %.3f\"/>\n", i, tc[i].x, tc[i].y);
			}
		}

		unsigned int *indices = mesh->get_data_int(EL_INDEX);
		if(indices) {
			int num_faces = mesh->get_count(EL_INDEX) / 3;

			for(int i=0; i<num_faces; i++) {
				fprintf(fp, "\t\t\t<face id=\"%d\">\n", i);
				for(int j=0; j<3; j++) {
					int idx = indices[i * 3 + j];

					fprintf(fp, "\t\t\t\t<vref vertex=\"%d\"", idx);
					if(norm) {
						fprintf(fp, " normal=\"%d\"", idx);
					}
					if(tc) {
						fprintf(fp, " texcoord=\"%d\"", idx);
					}
					fprintf(fp, "/>\n");
				}
				fprintf(fp, "\t\t\t</face>\n");
			}
		} else {
			int num_faces = num_verts / 3;

			for(int i=0; i<num_faces; i++) {
				fprintf(fp, "\t\t\t<face id=\"%d\">\n", i);
				for(int j=0; j<3; j++) {
					int idx = i * 3 + j;

					fprintf(fp, "\t\t\t\t<vref vertex=\"%d\"", idx);
					if(norm) {
						fprintf(fp, " normal=\"%d\"", idx);
					}
					if(tc) {
						fprintf(fp, " texcoord=\"%d\"", idx);
					}
					fprintf(fp, "/>\n");
				}
				fprintf(fp, "\t\t\t</face>\n");
			}
		}

		fprintf(fp, "\t\t</mesh>\n");
		fprintf(fp, "\t</object>\n");
	}

	/*
	fprintf(fp, "\n\t<!-- lights -->");

	for(int i=0; i<num_lt; i++) {
		if(typeid(lt[i]) != typeid(henge::light)) {
			continue;
		}

		fprintf(fp, "\t<light type=\"point\">\n");

		Vector3 pos = lt[i]->get_local_position();
		fprintf(fp, "\t\t<xform pos=\"%f %f %f\"/>\n", pos.x, pos.y, pos.z);
		fprintf(fp, "\t</light>\n");
	}

	fprintf(fp, "\n\t<!-- cameras -->");
	*/

	fprintf(fp, "</scene>\n");
	return true;
}
