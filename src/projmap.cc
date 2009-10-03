#include "projmap.h"

ProjMap::ProjMap()
{
	cell_area = 0;

	set_subdiv(64, 32);
	coverage = 0.0;
}

void ProjMap::clear()
{
	set_all_cells(false);
	coverage = 0.0;
}

bool ProjMap::empty() const
{
	for(size_t i=0; i<cells.size(); i++) {
		if(cells[i]) {
			return false;
		}
	}
	return true;
}

void ProjMap::set_subdiv(int udiv, int vdiv)
{
	this->udiv = udiv;
	this->vdiv = vdiv;

	int num_cells = udiv * vdiv;
	cells.resize(num_cells);

	delete [] cell_area;
	cell_area = new double[num_cells];

	/* precalculate the area of each cell, and the total area of all cells.
	 * useful for finding the coverage of the projection map.
	 */
	total_area = 0.0;
	for(int i=0; i<num_cells; i++) {
		int uc, vc;
		cell_coords(i, &uc, &vc);

		Vector3 v[4];
		for(int j=0; j<4; j++) {
			v[j] = cell_corner_dir(uc, vc, j);
		}

		double area = cross_product(v[1] - v[0], v[2] - v[0]).length() * 0.5;
		area += cross_product(v[2] - v[0], v[3] - v[0]).length() * 0.5;

		cell_area[i] = area;
		total_area += area;
	}
}

int ProjMap::get_subdiv(int *udiv, int *vdiv) const
{
	if(udiv) *udiv = this->udiv;
	if(vdiv) *vdiv = this->vdiv;
	return (int)cells.size();
}

#define CELL_IDX(u, v)	((v) * this->udiv + (u))

int ProjMap::cell_index(int ucell, int vcell) const
{
	return CELL_IDX(ucell, vcell);
}

void ProjMap::cell_coords(int idx, int *ucell, int *vcell) const
{
	int row = idx / udiv;
	int offs = idx - (row * udiv);

	if(ucell) *ucell = offs;
	if(vcell) *vcell = row;
}

void ProjMap::set_cell(int ucell, int vcell, bool val)
{
	int idx = CELL_IDX(ucell, vcell);

	if(val != cells[idx]) {
		double change = cell_area[idx] / total_area;
		coverage += val ? change : -change;
	}
	cells[idx] = val;
}

bool ProjMap::get_cell(int ucell, int vcell) const
{
	int idx = CELL_IDX(ucell, vcell);
	return cells[idx];
}

void ProjMap::set_all_cells(bool val)
{
	for(size_t i=0; i<cells.size(); i++) {
		cells[i] = val;
	}
	coverage = val ? 1.0 : 0.0;
}

double ProjMap::get_coverage() const
{
	return MIN(MAX(coverage, 0.0), 1.0);
}

Vector3 ProjMap::cell_corner_dir(int ucell, int vcell, int corner) const
{
	static const int offs[][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

	double du = 1.0 / udiv;
	double dv = 1.0 / vdiv;

	double u = (ucell + offs[corner][0]) * du;
	double v = (vcell + offs[corner][1]) * dv;

	double theta = u * M_PI * 2.0;	// map u -> [0, 2pi]
	double phi = v * M_PI;			// map v -> [0, pi]

	SphVector sv(theta, phi, 1.0);
	return Vector3(sv);
}

void ProjMap::cell_from_dir(const Vector3 &dir, int *ucell, int *vcell) const
{
	SphVector sv = dir;

	if(sv.theta < 0.0) {
		sv.theta = M_PI * 2.0 + sv.theta;
	}

	// map u and v back to [0, 1]
	double u = sv.theta / (M_PI * 2.0);
	double v = sv.phi / M_PI;

	int uc = (int)(u * udiv);
	int vc = (int)(v * vdiv);

	if(ucell) *ucell = uc;
	if(vcell) *vcell = vc;
}

void ProjMap::rand_cell(int *ucell, int *vcell) const
{
	if(ucell) {
		*ucell = rand() % udiv;
	}
	if(vcell) {
		*vcell = rand() % vdiv;
	}
}

Vector3 ProjMap::gen_dir(int ucell, int vcell) const
{
	double du = 1.0 / udiv;
	double dv = 1.0 / vdiv;

	double umin = ucell * du;
	double vmin = vcell * dv;

	double u = frand(du) + umin;
	double v = frand(dv) + vmin;

	double theta = u * M_PI * 2.0;	// map u -> [0, 2pi]
	double phi = v * M_PI;			// map v -> [0, pi]

	SphVector sv(theta, phi, 1.0);
	return Vector3(sv);
}

// XXX this will fall into an infinite loop if all cells are empty and towards_full == true
Vector3 ProjMap::gen_dir(bool towards_full) const
{
	Vector3 v;
	int uc, vc;

	if(!towards_full) {
		return sphrand(1.0);
	}

	do {
		v = sphrand(1.0);
		cell_from_dir(v, &uc, &vc);
	} while(!get_cell(uc, vc));

	return v;
}
