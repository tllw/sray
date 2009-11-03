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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <lib3ds/file.h>
#include <lib3ds/camera.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/mesh.h>
#include <lib3ds/material.h>
#include <lib3ds/matrix.h>
#include <lib3ds/vector.h>
#include <lib3ds/light.h>

enum {
	MESHES 		= 1,
	LIGHTS 		= 2,
	CAMERAS 	= 4,
	MATERIALS 	= 8,
	ENV			= 16
};

int write_scene(Lib3dsFile *file, FILE *out);
int write_env(Lib3dsFile *file, FILE *out);
int write_matlib(Lib3dsFile *file, FILE *out);
int write_objects(Lib3dsFile *file, FILE *out);
int write_camera(Lib3dsFile *file, FILE *out);
int write_lights(Lib3dsFile *file, FILE *out);

char *fname;
unsigned int expsel;

int main(int argc, char **argv)
{
	int i;
	Lib3dsFile *file;
	FILE *out = stdout;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-' && argv[i][2] == 0) {
			switch(argv[i][1]) {
			case 'o':
				if(!(out = fopen(argv[++i], "w"))) {
					fprintf(stderr, "failed to open output file %s: %s\n", argv[i], strerror(errno));
					return 1;
				}
				break;

			case 'm':
				expsel |= MESHES;
				break;

			case 'l':
				expsel |= LIGHTS;
				break;

			case 'c':
				expsel |= CAMERAS;
				break;

			case 'M':
				expsel |= MATERIALS;
				break;

			case 'e':
				expsel |= ENV;
				break;

			default:
				fprintf(stderr, "unrecognized option: %s\n", argv[i]);
				return 1;
			}
		} else {
			if(fname) {
				fprintf(stderr, "unexpected argument: %s\n", argv[i]);
				return 1;
			}
			fname = argv[i];
		}
	}

	if(!fname) {
		fprintf(stderr, "missing 3ds filename\n");
		return 1;
	}

	if(!expsel) {
		expsel = 0xffffffff;
	}

	if(!(file = lib3ds_file_load(fname))) {
		fprintf(stderr, "failed to load input file: %s\n", fname);
		return 1;
	}
	lib3ds_file_eval(file, 0);

	write_scene(file, out);

	lib3ds_file_free(file);
	if(out != stdout) {
		fclose(out);
	}
	return 0;
}

int write_scene(Lib3dsFile *file, FILE *out)
{
	fprintf(out, "<!-- converted from 3ds with 3ds2sray by John Tsiombikas (nuclear@member.fsf.org) -->\n");
	fprintf(out, "<scene name=\"%s\">\n", fname);

	if(expsel & ENV) {
		write_env(file, out);
	}
	if(expsel & MATERIALS) {
		write_matlib(file, out);
	}
	if(expsel & CAMERAS) {
		write_camera(file, out);
	}
	if(expsel & LIGHTS) {
		write_lights(file, out);
	}
	if(expsel & MESHES) {
		write_objects(file, out);
	}

	fprintf(out, "</scene>\n");
	return 0;
}

int write_env(Lib3dsFile *file, FILE *out)
{
	return 0;	/* TODO */
}


#define MATTR3(attr, r, g, b, tex)	\
	do { \
		fprintf(out, "\t\t<mattr name=\"%s\" value=\"%.3f %.3f %.3f\"", attr, r, g, b); \
		if((tex) && *(tex)) \
			fprintf(out, " map=\"%s\"/>\n", tex); \
		else \
			fprintf(out, "/>\n"); \
	} while(0)

#define MATTR1(attr, val, tex)	\
	do { \
		fprintf(out, "\t\t<mattr name=\"%s\" value=\"%.3f\"", attr, val); \
		if((tex) && *(tex)) \
			fprintf(out, " map=\"%s\"/>\n", tex); \
		else \
			fprintf(out, "/>\n"); \
	} while(0)


int write_matlib(Lib3dsFile *file, FILE *out)
{
	Lib3dsMaterial *m = file->materials;

	fprintf(out, "\n\t<!-- materials -->\n");

	while(m) {
		float sstr = m->shin_strength;
		char *nomap = "";

		fprintf(out, "\t<material name=\"%s\" shader=\"phong\">\n", m->name);
		MATTR3("ambient", m->ambient[0], m->ambient[1], m->ambient[2], nomap);
		MATTR3("diffuse", m->diffuse[0], m->diffuse[1], m->diffuse[2], m->texture1_map.name);
		MATTR3("specular", m->specular[0] * sstr, m->specular[1] * sstr, m->specular[2] * sstr,
				m->specular_map.name);
		MATTR1("shininess", m->shininess * 100.0, m->shininess_map.name);
		MATTR1("refract", m->transparency, m->opacity_map.name);
		fprintf(out, "\t</material>\n");

		m = m->next;
	}

	return 0;
}

int write_objects(Lib3dsFile *file, FILE *out)
{
	int i, j;
	Lib3dsNode *node;
	Lib3dsMesh *m = file->meshes;

	fprintf(out, "\n\t<!-- objects -->\n");

	while(m) {
		Lib3dsVector *norm;

		if(!m->faces || !m->points) { /* skip non-meshes */
			m = m->next;
			continue;
		}

		if((norm = malloc(3 * sizeof *norm * m->faces))) {
			lib3ds_mesh_calculate_normals(m, norm);
		} else {
			perror("failed to allocate vertex normal array\n");
		}

		if(!(node = lib3ds_file_node_by_name(file, m->name, LIB3DS_OBJECT_NODE))) {
			fprintf(stderr, "warning, mesh: %s does not have a node!\n", m->name);
		}

		fprintf(out, "\t<object name=\"%s\" type=\"mesh\">\n", m->name);
		
		/* material reference */
		if(m->faceL[0].material && *m->faceL[0].material) {
			fprintf(out, "\t\t<matref name=\"%s\"/>\n", m->faceL[0].material);
		}

		/* transformation (TODO animation) */
		/*fprintf(out, "\t\t<xform pos=\"%.3f %.3f %.3f\"", node->data.object.pos[0],
				node->data.object.pos[2], node->data.object.pos[1]);
		fprintf(out, " rot=\"%.3f %.3f %.3f %.3f\"", node->data.object.rot[3],
				node->data.object.rot[0], node->data.object.rot[2], node->data.object.rot[1]);
		fprintf(out, " scale=\"%.3f %.3f %.3f\"", node->data.object.scl[0],
				node->data.object.scl[2], node->data.object.scl[1]);
		fprintf(out, " pivot=\"%.3f %.3f %.3f\"/>\n", node->data.object.pivot[0],
				node->data.object.pivot[2], node->data.object.pivot[1]);*/

		/* mesh */
		fprintf(out, "\t\t<mesh>\n");
		for(i=0; i<m->points; i++) {
			fprintf(out, "\t\t\t<vertex id=\"%d\" val=\"%.3f %.3f %.3f\"/>\n", i,
					m->pointL[i].pos[0], m->pointL[i].pos[2], m->pointL[i].pos[1]);
			if(m->texels) {
				fprintf(out, "\t\t\t<texcoord id=\"%d\" val=\"%.3f %.3f\"/>\n", i,
						m->texelL[i][0], m->texelL[i][1]);
			}
		}

		if(norm) {
			for(i=0; i<m->faces; i++) {
				for(j=0; j<3; j++) {
					int nidx = i * 3 + j;
					fprintf(out, "\t\t\t<normal id=\"%d\" val=\"%.3f %.3f %.3f\"/>\n", nidx,
							norm[nidx][0], norm[nidx][2], norm[nidx][1]);
				}
			}
		}

		for(i=0; i<m->faces; i++) {
			fprintf(out, "\t\t\t<face id=\"%d\">\n", i);
			
			for(j=0; j<3; j++) {
				int idx = m->faceL[i].points[j];
				int nidx = i * 3 + j;
				fprintf(out, "\t\t\t\t<vref vertex=\"%d\" normal=\"%d\" texcoord=\"%d\"/>\n", idx, nidx, idx);
			}
			fprintf(out, "\t\t\t</face>\n");
		}
		fprintf(out, "\t\t</mesh>\n");

		fprintf(out, "\t</object>\n");
		m = m->next;
	}

	return 0;
}

/* XXX what to do with multiple cameras? currently ignoring all but the first */
int write_camera(Lib3dsFile *file, FILE *out)
{
	Lib3dsCamera *c = file->cameras;
	if(!c) return -1;

	fprintf(out, "\n\t<!-- camera -->\n");
	
	fprintf(out, "\t<camera type=\"target\" fov=\"%.3f\">\n", c->fov / 1.3333333);
	fprintf(out, "\t\t<xform pos=\"%.3f %.3f %.3f\"/>\n", c->position[0], c->position[2], c->position[1]);
	fprintf(out, "\t\t<target>\n");
	fprintf(out, "\t\t\t<xform pos=\"%.3f %.3f %.3f\"/>\n", c->target[0], c->target[2], c->target[1]);
	fprintf(out, "\t\t</target>\n");
	fprintf(out, "\t</camera>\n");

	return 0;
}

int write_lights(Lib3dsFile *file, FILE *out)
{
	Lib3dsLight *lt = file->lights;

	fprintf(out, "\n\t<!-- lights -->\n");

	while(lt) {
		if(!lt->spot_light) {
			fprintf(out, "\t<light type=\"point\" color=\"%.3f %.3f %.3f\">\n", lt->color[0] * lt->multiplier,
					lt->color[1] * lt->multiplier, lt->color[2] * lt->multiplier);
			fprintf(out, "\t\t<xform pos=\"%.3f %.3f %.3f\"/>\n", lt->position[0],
					lt->position[2], lt->position[1]);
			fprintf(out, "\t</light>\n");
		}
		lt = lt->next;
	}

	return 0;
}
