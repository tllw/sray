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
#ifndef PHOTON_MAP_H_
#define PHOTON_MAP_H_

#include <vector>
#include <vmath/vmath.h>
#include "kdtree.h"
#include "color.h"

enum PhotonType {
	DIRECT_PHOTON,
	CAUST_PHOTON,
	GI_PHOTON
};

struct Photon {
	PhotonType type;
	Vector3 pos, dir, norm;
	Color col;
};

class PhotonMap {
private:
	kdtree *kd;

	std::vector<Photon*> photons;
	int prev_scale;

public:
	PhotonMap();
	~PhotonMap();

	void clear();

	bool empty() const;
	size_t size() const;

	bool add_photon(const Vector3 &pos, const Vector3 &dir, const Vector3 &norm, const Color &col);
	void scale_photon_power(double scale);

	Photon **get_photons();

	Color irradiance_est(const Vector3 &pos, const Vector3 &norm, double max_dist, int max_photons = -1) const;
	Color radiance_est(const Vector3 &pos, const Vector3 &norm, const Vector3 &dir, double max_dist, int max_photons = -1) const;

	bool dump(const char *fname) const;
	bool restore(const char *fname);
};

#endif	// PHOTON_MAP_H_
