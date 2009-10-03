#ifndef PHOTON_MAP_H_
#define PHOTON_MAP_H_

#include <vector>
#include <vmath.h>
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
