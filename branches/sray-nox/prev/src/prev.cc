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
#include <string.h>
#include <errno.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include "cam.h"
#include "scene.h"
#include "mesh.h"
#include "sphere.h"
#include "cylinder.h"
#include "opt.h"

static void disp();
static void draw_mesh(const Mesh *mesh);
static void draw_sphere(const Sphere *sph);
static void draw_cylinder(const Cylinder *cyl);
static void draw_light(const Light *lt);
static void draw_camera(const Camera *cam);

template <typename T>
static void draw_octree(Octree<T> *tree, const Color &col);

static void mult_glmatrix(const Matrix4x4 &xform);
static void reshape(int x, int y);
static void keyb(unsigned char key, int x, int y);
static void mouse(int bn, int state, int x, int y);
static void motion(int x, int y);

Scene *scn;
bool use_material_color;
bool use_photon_color;
bool show_geom = true;
bool show_mesh_octrees;
bool show_scene_octree;
bool show_mesh_normals;
bool show_camera = true;
bool show_lights = true;
bool show_projmaps;
float normal_scale_factor = 0.5;

int frame_time;
bool in_time_change;

PhotonMap *pmap = 0;
int pjmap_idx;

int main(int argc, char **argv)
{
	glutInitWindowSize(800, 600);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("scene visualizer");

	glutDisplayFunc(disp);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	parse_opt(argc, argv);
	opt.verb = 1;
	if(!opt.caust_photons) {
		opt.caust_photons = 10000;
	}
	if(!opt.gi_photons) {
		opt.gi_photons = 50000;
	}


	scn = new Scene;
	FILE *scnfile = stdin;

	if(opt.scenefile) {
		if(!(scnfile = fopen(opt.scenefile, "rb"))) {
			fprintf(stderr, "failed to open scene: %s: %s\n", opt.scenefile, strerror(errno));
			return 1;
		}
	}

	if(!scn->load(scnfile)) {
		fprintf(stderr, "failed to load scene data\n");
		return 1;
	}
	scn->build_tree();

	glutMainLoop();
	return 0;
}

static void disp()
{
	static bool first_run = true;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	set_view_matrix();

	/* draw scene objects */
	if(show_geom) {
		int num_obj = scn->get_object_count();
		Object **obj = (Object**)scn->get_objects();

		for(int i=0; i<num_obj; i++) {
			glPushMatrix();
			Matrix4x4 xform = obj[i]->get_xform_matrix(frame_time);
			mult_glmatrix(xform);

			if(use_material_color) {		
				Color col = obj[i]->get_material()->get_color("diffuse", Vector2(0, 0));
				glColor3f(col.x, col.y, col.z);
			} else {
				glColor3f(0.85, 0.85, 0.85);
			}

			if(dynamic_cast<Mesh*>(obj[i])) {
				Mesh *m = (Mesh*)obj[i];

				if(first_run) {
					m->build_tree();
				}

				draw_mesh(m);
				if(show_mesh_octrees) {
					draw_octree(m->get_tree(), Color(0, 0.5, 0));
				}
			} else if(dynamic_cast<Sphere*>(obj[i])) {
				draw_sphere((Sphere*)obj[i]);
			} else if(dynamic_cast<Cylinder*>(obj[i])) {
				draw_cylinder((Cylinder*)obj[i]);
			}

			glPopMatrix();
		}
	}

	/* draw scene octree */
	if(show_scene_octree) {
		Octree<Object*> *oct = scn->get_octree();
		draw_octree(oct, Color(0.6, 0, 0.4));
	}

	/* draw lights */
	if(show_lights) {
		int num_lt = scn->get_light_count();
		Light **lt = (Light**)scn->get_lights();

		for(int i=0; i<num_lt; i++) {
			draw_light(lt[i]);

			/* draw projection maps */
			if(show_projmaps && pmap) {
				ProjMap *pjmap = pjmap_idx ? lt[i]->get_projmap() : lt[i]->get_spec_projmap();
				Vector3 ltpos = lt[i]->get_position(frame_time);

				glPushAttrib(GL_POLYGON_BIT);

				int ncells = pjmap->get_subdiv();
				for(int i=0; i<ncells; i++) {
					int uc, vc;
					pjmap->cell_coords(i, &uc, &vc);

					if(pjmap->get_cell(uc, vc)) {
						glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						glColor3f(0.8, 0.7, 0.2);
					} else {
						glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
						glColor3f(0.3, 0.2, 0.05);
					}

					glBegin(GL_QUADS);
					for(int j=0; j<4; j++) {
						Vector3 v = ltpos + pjmap->cell_corner_dir(uc, vc, j) * normal_scale_factor;
						glVertex3f(v.x, v.y, v.z);
					}
					glEnd();
				}

				glPopAttrib();
			}
		}
	}

	/* draw camera frustum */
	Camera *cam = scn->get_camera();
	if(cam && show_camera) {
		draw_camera(cam);
	}

	/* draw photon maps */
	if(pmap) {
		Photon **phot = pmap->get_photons();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPointSize(normal_scale_factor);

		glBegin(GL_POINTS);
		for(size_t i=0; i<pmap->size(); i++) {
			if(use_photon_color) {
				glColor3f(phot[i]->col.x, phot[i]->col.y, phot[i]->col.z);
				//fprintf(stderr, "%.3f %.3f %.3f\n", phot[i]->col.x, phot[i]->col.y, phot[i]->col.z);
			} else {
				glColor3f(0, 1, 0);
			}

			glVertex3f(phot[i]->pos.x, phot[i]->pos.y, phot[i]->pos.z);
		}
		glEnd();

		glDisable(GL_BLEND);
	}

	if(in_time_change) {
		char buf[32], *ptr;

		sprintf(buf, "time: %.2fs\n", (float)frame_time / 1000.0);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glPushAttrib(GL_POLYGON_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glRasterPos3f(-0.99, -0.95, 0);

		glColor3f(1, 1, 0);

		ptr = buf;
		while(*ptr) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ptr++);
		}

		glPopAttrib();

		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	first_run = false;

	assert(glGetError() == 0);
	glutSwapBuffers();
}

static void draw_mesh(const Mesh *mesh)
{
	int num_tri = mesh->get_face_count();

	glBegin(GL_TRIANGLES);

	for(int i=0; i<num_tri; i++) {
		const Triangle *tri = mesh->get_face(i);

		for(int j=0; j<3; j++) {
			const Vertex *v = tri->v + j;
			glNormal3f(v->norm.x, v->norm.y, v->norm.z);
			glTexCoord2f(v->tex.x, v->tex.y);
			glVertex3f(v->pos.x, v->pos.y, v->pos.z);
		}
	}

	glEnd();

	if(show_mesh_normals) {
		glLineWidth(1.0);
		glColor3f(0.2, 0.3, 0.8);

		glBegin(GL_LINES);
		for(int i=0; i<num_tri; i++) {
			const Triangle *tri = mesh->get_face(i);

			for(int j=0; j<3; j++) {
				const Vertex *v = tri->v + j;
				Vector3 nend = v->pos + v->norm * normal_scale_factor;

				glVertex3f(v->pos.x, v->pos.y, v->pos.z);
				glVertex3f(nend.x, nend.y, nend.z);
			}
		}
		glEnd();
	}
}

static void draw_sphere(const Sphere *sph)
{
	glRotatef(90, 1, 0, 0);
	glutSolidSphere(sph->get_radius(), 20, 10);
}

#define CYL_SIDES	16
static void draw_cylinder(const Cylinder *cyl)
{
	float dt = TWO_PI / (float)CYL_SIDES;
	float theta = 0.0;
	float rad = cyl->get_radius();
	float top = cyl->get_start();
	float bottom = cyl->get_end();

	glBegin(GL_QUAD_STRIP);
	for(int i=0; i<=CYL_SIDES; i++) {
		float x = cos(theta) * rad;
		float y = sin(theta) * rad;

		glVertex3f(x, bottom, y);
		glVertex3f(x, top, y);
		theta += dt;
	}
	glEnd();
}

static void draw_light(const Light *lt)
{
	Matrix4x4 xform = lt->get_xform_matrix(frame_time);
	Vector3 pos = Vector3(0, 0, 0).transformed(xform);

	glColor3f(1.0, 1.0, 0.0);

	if(dynamic_cast<const SphLight*>(lt)) {
		float rad = ((const SphLight*)lt)->get_radius();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(pos.x, pos.y, pos.z);

		glutSolidSphere(rad, 12, 6);
		glPopMatrix();

	} else if(dynamic_cast<const BoxLight*>(lt)) {
		Vector3 dim = ((const BoxLight*)lt)->get_dimensions();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(pos.x, pos.y, pos.z);
		glScalef(dim.x, dim.y, dim.z);

		glutSolidCube(1.0);
		glPopMatrix();

	} else {
		glPushAttrib(GL_POINT_BIT | GL_ENABLE_BIT);

		glPointSize(5.0);
		glEnable(GL_POINT_SMOOTH);

		glEnable(GL_BLEND);

		glBegin(GL_POINTS);
		glVertex3f(pos.x, pos.y, pos.z);
		glEnd();

		glPopAttrib();
	}
}

static void draw_camera(const Camera *cam)
{
	float dist = 10.0;
	const TargetCamera *tcam;

	if((tcam = dynamic_cast<const TargetCamera*>(cam))) {
		Vector3 pos = tcam->get_position(frame_time);
		Vector3 targ = tcam->get_target(frame_time);
		dist = (targ - pos).length();
	}

	float half_width = tan(cam->get_horizontal_fov() / 2.0);
	float half_height = tan(cam->get_vertical_fov() / 2.0);

	glPushAttrib(GL_POINT_BIT | GL_ENABLE_BIT);

	glPointSize(8.0);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);

	glPushMatrix();

	Matrix4x4 xform = cam->get_matrix(frame_time);
	mult_glmatrix(xform);
	glScalef(dist, dist, dist);

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(0.5, 0.6, 0.8);
	glVertex3f(0, 0, 0);
	glVertex3f(half_width, -half_height, 1);
	glVertex3f(half_width, half_height, 1);
	glVertex3f(-half_width, half_height, 1);
	glVertex3f(-half_width, -half_height, 1);
	glVertex3f(half_width, -half_height, 1);
	glEnd();

	glBegin(GL_POINTS);
	glVertex3f(0, 0, 0);
	glEnd();

	glPopMatrix();
	glPopAttrib();
}

template <typename T>
static void draw_octnode(OctNode<T> *node, void *cls)
{
	if(!node) return;

	Color *col = (Color*)cls;

	float xsz = node->box.max[0] - node->box.min[0];
	float ysz = node->box.max[1] - node->box.min[1];
	float zsz = node->box.max[2] - node->box.min[2];

	float x = node->box.min[0] + xsz / 2.0;
	float y = node->box.min[1] + ysz / 2.0;
	float z = node->box.min[2] + zsz / 2.0;

	glPushMatrix();
	glTranslatef(x, y, z);
	glScalef(1.0, ysz / xsz, zsz / xsz);

	glLineWidth(1.5);
	glColor3f(col->x, col->y, col->z);
	glutWireCube(xsz);
	glLineWidth(1.0);

	glPopMatrix();
}

template <typename T>
static void draw_octree(Octree<T> *tree, const Color &col)
{
	tree->traverse(OCT_PREORDER, draw_octnode, (void*)&col);
}

static void mult_glmatrix(const Matrix4x4 &xform)
{
	float val[16];

	for(int i=0; i<4; i++) {
		for(int j=0; j<4; j++) {
			val[i * 4 + j] = xform[j][i];
		}
	}

	glMultMatrixf(val);
}


static void reshape(int x, int y)
{
	glViewport(0, 0, x, y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0, (float)x / (float)y, 0.5, 10000.0);
}

static void keyb(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);

	case 'g':
		show_geom = !show_geom;
		glutPostRedisplay();
		break;

	case 't':
		show_mesh_octrees = !show_mesh_octrees;
		glutPostRedisplay();
		break;

	case 'T':
		show_scene_octree = !show_scene_octree;
		glutPostRedisplay();
		break;

	case 'n':
		show_mesh_normals = !show_mesh_normals;
		glutPostRedisplay();
		break;

	case '.':
		normal_scale_factor += 0.1;
		glutPostRedisplay();
		break;

	case ',':
		normal_scale_factor -= 0.1;
		glutPostRedisplay();
		break;

	case 'c':
		show_camera = !show_camera;
		glutPostRedisplay();
		break;

	case 'l':
		show_lights = !show_lights;
		glutPostRedisplay();
		break;

	case 'C':
		print_camera();
		break;

	case 'm':
		use_material_color = !use_material_color;
		glutPostRedisplay();
		break;

	case 'p':
		printf("building photon maps\n");
		scn->build_photon_maps();
		pmap = scn->get_caust_map();
		glutPostRedisplay();
		break;

	case 'P':
		if(pmap == scn->get_caust_map()) {
			pmap = scn->get_gi_map();
			printf("showing gi map\n");
		} else {
			pmap = scn->get_caust_map();
			printf("showing caustics map\n");
		}
		glutPostRedisplay();
		break;

	case '[':
		{
			char dumpfile[64];
			sprintf(dumpfile, "%s.cdump", opt.scenefile ? opt.scenefile : "scene");
			if(scn->get_caust_map()->restore(dumpfile)) {
				glutPostRedisplay();
			}
		}
		break;

	case ']':
		{
			char dumpfile[64];
			sprintf(dumpfile, "%s.gdump", opt.scenefile ? opt.scenefile : "scene");
			if(scn->get_gi_map()->restore(dumpfile)) {
				glutPostRedisplay();
			}
		}
		break;

	case 'o':
		use_photon_color = !use_photon_color;
		glutPostRedisplay();
		break;

	case 'j':
		show_projmaps = !show_projmaps;
		glutPostRedisplay();
		break;

	case 'J':
		pjmap_idx = (pjmap_idx + 1) & 1;
		glutPostRedisplay();
		break;

	default:
		break;
	}
}

static int bnstate[16];
static bool modkeys;

static int prev_x = -1, prev_y;
static void mouse(int bn, int state, int x, int y)
{
	bnstate[bn] = state == GLUT_DOWN ? 1 : 0;
	modkeys = glutGetModifiers();

	if(state == GLUT_DOWN) {
		if(bn == 0 && (modkeys & GLUT_ACTIVE_SHIFT)) {
			in_time_change = true;
			glutPostRedisplay();
		}

		if(bn == 3) {
			cam_zoom(-10);
			glutPostRedisplay();
		} else if(bn == 4) {
			cam_zoom(10);
			glutPostRedisplay();
		} else {
			prev_x = x;
			prev_y = y;
		}
	} else {
		if(bn == 0 && in_time_change) {
			in_time_change = false;
			glutPostRedisplay();
		}
		prev_x = -1;
	}

}

static void motion(int x, int y)
{
	if(bnstate[0]) {
		if(modkeys & GLUT_ACTIVE_SHIFT) {
			frame_time += (x - prev_x) * 10;
		} else{
			cam_rotate(x - prev_x, y - prev_y);
		}
		glutPostRedisplay();
	}
	if(bnstate[1]) {
		cam_pan(x - prev_x, y - prev_y);
		glutPostRedisplay();
	}
	if(bnstate[2]) {
		cam_zoom(y - prev_y);
		glutPostRedisplay();
	}

	prev_x = x;
	prev_y = y;
}

