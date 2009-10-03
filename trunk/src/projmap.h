#ifndef PROJMAP_H_
#define PROJMAP_H_

#include <vector>
#include <vmath.h>

/** ProjMap is used by lights to project the bounding spheres of scene objects
 * to a sphere around the light, to select directions for photon shooting that
 * are liable to reach objects.
 */
class ProjMap {
private:
	int udiv, vdiv;
	std::vector<bool> cells;
	double coverage;

	double *cell_area;
	double total_area;

public:
	ProjMap();

	void clear();

	// XXX this is O(n)
	bool empty() const;

	void set_subdiv(int udiv, int vdiv);
	int get_subdiv(int *udiv = 0, int *vdiv = 0) const;

	/** converts from cell coords to array index */
	int cell_index(int ucell, int vcell) const;

	/** converts from array index to cell coords */
	void cell_coords(int idx, int *ucell, int *vcell) const;

	/** set/get the value of a cell */
	void set_cell(int ucell, int vcell, bool val);
	bool get_cell(int ucell, int vcell) const;

	void set_all_cells(bool val);

	/** calculate the area of the sphere covered by hits over the total area */
	double get_coverage() const;

	/** return vector pointing towards a cell corner */
	Vector3 cell_corner_dir(int ucell, int vcell, int corner) const;

	/** find which cell corresponds to the given direction */
	void cell_from_dir(const Vector3 &dir, int *ucell, int *vcell) const;

	/** select a random cell */
	void rand_cell(int *ucell, int *vcell) const;

	/** generate a random direction within the given cell's solid angle */
	Vector3 gen_dir(int ucell, int vcell) const;

	/** generate a random direction. if towards_full is true, then it only
	 * generates directions within full cells (uses rejection sampling)
	 */
	Vector3 gen_dir(bool towards_full = true) const;
};

#endif	// PROJMAP_H_
