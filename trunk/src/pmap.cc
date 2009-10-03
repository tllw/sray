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
#include "pmap.h"
#include "opt.h"

// TODO implement photon pool for the allocator?
static Photon *alloc_photon()
{
	try {
		return new Photon;
	}
	catch(...) {}
	return 0;
}

static void free_photon(Photon *p)
{
	delete p;
}

PhotonMap::PhotonMap()
{
	if(!(kd = kd_create(3))) {
		throw std::bad_alloc();
	}
	kd_data_destructor(kd, (void (*)(void*))free_photon);
	
	prev_scale = 0;
}

PhotonMap::~PhotonMap()
{
	kd_free(kd);
}

void PhotonMap::clear()
{
	kd_clear(kd);
	photons.clear();

	prev_scale = 0;
}

bool PhotonMap::empty() const
{
	return photons.empty();
}

size_t PhotonMap::size() const
{
	return photons.size();
}

bool PhotonMap::add_photon(const Vector3 &pos, const Vector3 &dir, const Vector3 &norm, const Color &col)
{
	Photon *phot = alloc_photon();
	if(!phot) {
		return false;
	}
	
	phot->pos = pos;
	phot->dir = dir;
	phot->col = col;
	phot->norm = norm;

	photons.push_back(phot);

	return kd_insert3f(kd, pos.x, pos.y, pos.z, phot) != -1;
}

void PhotonMap::scale_photon_power(double scale)
{
	printf("DBG scale_photon_power(%f)\n", scale);
	int num_photons = (int)photons.size();
	for(int i=prev_scale; i<num_photons; i++) {
		photons[i]->col.x *= scale;
		photons[i]->col.y *= scale;
		photons[i]->col.z *= scale;
	}
	prev_scale = num_photons;
}

Photon **PhotonMap::get_photons()
{
	return photons.empty() ? 0 : &photons[0];
}

// estimates irradiance at any given point
// TODO: max_photons is unused for now
Color PhotonMap::irradiance_est(const Vector3 &pos, const Vector3 &norm, double max_dist, int max_photons) const
{
	kdres *res;
	Color irrad(0, 0, 0);

	if(!(res = kd_nearest_range3f(kd, pos.x, pos.y, pos.z, max_dist))) {
		return irrad;
	}

	if(kd_res_size(res) < 8) {
		kd_res_free(res);
		return irrad;
	}

	// sum irradiance
	while(!kd_res_end(res)) {
		Photon *phot = (Photon*)kd_res_item(res, 0);

		// add if it came from the front of the surface
		if(dot_product(phot->dir, norm) < 0.0) {
			irrad += phot->col;
		}

		kd_res_next(res);
	}
	kd_res_free(res);

	// density estimate
	irrad *= 1.0 / (M_PI * SQ(max_dist));
	return irrad;
}

// estimates radiance exiting towards direction dir from a given point
// TODO: max_photons is unused for now
Color PhotonMap::radiance_est(const Vector3 &pos, const Vector3 &norm, const Vector3 &dir, double max_dist, int max_photons) const
{
	kdres *res;
	Color flux(0, 0, 0);
	static const double max_cos_angle = cos(M_PI / 2.5);

	if(!(res = kd_nearest_range3f(kd, pos.x, pos.y, pos.z, max_dist))) {
		return flux;
	}

	if(kd_res_size(res) <= 0) {
		kd_res_free(res);
		return flux;
	}

	// sum radiant flux
	while(!kd_res_end(res)) {
		Photon *phot = (Photon*)kd_res_item(res, 0);

		double ndotl = dot_product(-phot->dir, norm);

		/* if the photon came from the front of the surface, and it's
		 * stored on a surface with a normal similar to the one we hit,
		 * use it.
		 */
		if(ndotl > 0.0 && dot_product(phot->norm, norm) > max_cos_angle) {
			flux += phot->col * ndotl;
		}

		kd_res_next(res);
	}
	kd_res_free(res);

	flux *= 1.0 / (M_PI * SQ(max_dist));
	return flux;
}

bool PhotonMap::dump(const char *fname) const
{
	FILE *fp;

	if(!(fp = fopen(fname, "w"))) {
		fprintf(stderr, "failed to dump photon map: %s: %s\n", fname, strerror(errno));
		return false;
	}

	fprintf(fp, "energy %f\n", opt.photon_energy);

	for(size_t i=0; i<photons.size(); i++) {
		const Photon *p = photons[i];
		fprintf(fp, "<%f %f %f> ", p->pos.x, p->pos.y, p->pos.z);
		fprintf(fp, "<%f %f %f> ", p->dir.x, p->dir.y, p->dir.z);
		fprintf(fp, "<%f %f %f> ", p->norm.x, p->norm.y, p->norm.z);
		fprintf(fp, "<%f %f %f>\n", p->col.x, p->col.y, p->col.z);
	}

	if(opt.verb) {
		printf("dumped %d photons at %s\n", (int)photons.size(), fname);
	}

	fclose(fp);
	return true;
}

bool PhotonMap::restore(const char *fname)
{
	FILE *fp;
	float energy, prev_energy;

	if(!(fp = fopen(fname, "r"))) {
		return false;
	}

	if(fscanf(fp, "energy %f\n", &prev_energy) < 1) {
		fprintf(stderr, "failed to restore photon dump: %s\n", fname);
		fclose(fp);
		return false;
	}
	energy = opt.photon_energy / prev_energy;

	int count = 0;
	for(;;) {
		Vector3 pos, dir, col, norm;
		
		int res = fscanf(fp, "<%f %f %f> <%f %f %f> <%f %f %f> <%f %f %f>\n",
				&pos.x, &pos.y, &pos.z, &dir.x, &dir.y, &dir.z,
				&norm.x, &norm.y, &norm.z, &col.x, &col.y, &col.z);
		if(res == EOF) break;
		if(res < 12) {
			fprintf(stderr, "failed to restore photon dump: %s\n", fname);
			fclose(fp);
			return false;
		}

		add_photon(pos, dir, norm, col * energy);
		count++;
	}

	if(count && opt.verb) {
		printf("restored %d photons from dump file: %s\n", count, fname);
	}
	fclose(fp);
	return true;
}
