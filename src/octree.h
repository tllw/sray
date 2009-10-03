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
#ifndef OCTREE_H_
#define OCTREE_H_

#include <list>
#include <vector>
#include "aabb.h"

template <typename T>
struct OctItem {
	// XXX data must be a pointer to an object with an intersect function
	T data;
	AABox box;
};

template <typename T>
class OctNode {
public:
	AABox box;
	std::list<OctItem<T>*> items;
	int num_items;
	OctNode<T> *child[8];

	OctNode();
};

struct OctStats {
	int height;
	int num_inner, num_leaves;
	int num_items;
	int min_items, max_items, avg_items;
};

enum {
	OCT_PREORDER = 0,
	OCT_POSTORDER = 8
};

/** Abstract Octree data structure */
template <typename T>
class Octree {
private:
	int items_per_node, max_depth;
	OctNode<T> *root;

	std::vector<OctItem<T> > items;
	AABox bounds;

	bool show_build_stats;

	void subdivide(OctNode<T> *node, int lvl);
	OctItem<T> *intersect_rec(OctNode<T> *node, const Ray &ray, SurfPoint *pt) const;

public:
	Octree();
	~Octree();

	/** set the maximum tree depth. no tree will grow deeper than this limit */
	void set_max_depth(int max_depth);
	int get_max_depth() const;

	/** set the maximum number of items per node. if this limit is surpassed
	 * the node will be subdivided unless the maximum tree depth prohibits it.
	 */
	void set_max_items_per_node(int num);
	int get_max_items_per_node() const;

	void set_build_stats(bool bs);

	void clear();

	void add(const AABox &box, const T &data);
	void build();

	/** find the nearest intersection between a given ray and the items in
	 * the tree.
	 * If an intersection is found, a pointer to the item is returned, and
	 * the sp pointer is filled with details about the point of intersection.
	 * Otherwise, null is returned if no intersection occurs.
	 */
	OctItem<T> *intersect(const Ray &ray, SurfPoint *pt = 0) const;

	/** traverse the tree in the order specified (OCT_PREORDER or OCT_POSTORDER),
	 * and call the supplied function for each node.
	 */
	void traverse(int ord, void (*op)(OctNode<T>*, void*), void *cls);

	OctNode<T> *get_root();
	const OctNode<T> *get_root() const;

	bool stats(OctStats *st) const;
};

#include "octree.inl"

#endif	// OCTREE_H_
