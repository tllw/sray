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
#include <limits.h>
#include <float.h>
#include "object.h"
#include "opt.h"

template <typename T>
OctNode<T>::OctNode()
{
	num_items = 0;
	memset(child, 0, 8 * sizeof(OctNode<T>*));
}
	
template <typename T>
Octree<T>::Octree()
{
	root = 0;
	items_per_node = 5;
	max_depth = 5;
	show_build_stats = opt.verb;

	clear();
}

template <typename T>
Octree<T>::~Octree()
{
	clear();
}

template <typename T>
void Octree<T>::set_max_depth(int max_depth)
{
	this->max_depth = max_depth;
}

template <typename T>
int Octree<T>::get_max_depth() const
{
	return max_depth;
}

template <typename T>
void Octree<T>::set_max_items_per_node(int num)
{
	items_per_node = num;
}

template <typename T>
int Octree<T>::get_max_items_per_node() const
{
	return items_per_node;
}

template <typename T>
void Octree<T>::set_build_stats(bool bs)
{
	show_build_stats = bs;
}

template <typename T>
static void free_tree(OctNode<T> *node)
{
	if(!node) return;

	for(int i=0; i<8; i++) {
		free_tree(node->child[i]);
	}

	node->items.clear();
	delete node;
}

template <typename T>
void Octree<T>::clear()
{
	free_tree(root);
	items.clear();
	root = 0;
	bounds.min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
	bounds.max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

template <typename T>
void Octree<T>::add(const AABox &box, const T &data)
{
	OctItem<T> item;
	item.data = data;
	item.box = box;

	items.push_back(item);

	if(box.min.x < bounds.min.x) bounds.min.x = box.min.x;
	if(box.min.y < bounds.min.y) bounds.min.y = box.min.y;
	if(box.min.z < bounds.min.z) bounds.min.z = box.min.z;
	if(box.max.x > bounds.max.x) bounds.max.x = box.max.x;
	if(box.max.y > bounds.max.y) bounds.max.y = box.max.y;
	if(box.max.z > bounds.max.z) bounds.max.z = box.max.z;
}


template <typename T>
void Octree<T>::build()
{
	if(root) {
		delete root;
	}

	root = new OctNode<T>;
	root->box = bounds;
	root->num_items = items.size();

	for(size_t i=0; i<items.size(); i++) {
		root->items.push_back(&items[i]);
	}

	if(root->num_items > items_per_node) {
		subdivide(root, 0);
	}

	if(show_build_stats) {
		OctStats st;
		
		if(stats(&st)) {
			printf("octree h:%d in:%d out:%d items:%d (%d - %d, avg: %d)\n", st.height,
					st.num_inner, st.num_leaves, st.num_items, st.min_items, st.max_items, st.avg_items);
		} else {
			fprintf(stderr, "no root? wtf\n");
		}
	}
}


template <typename T>
void Octree<T>::subdivide(OctNode<T> *node, int lvl)
{
	if(node->num_items <= items_per_node || lvl >= max_depth) {
		return;
	}
	
	AABox box = node->box;
	AABox cbox[8];

	Vector3 cent = (box.min + box.max) * 0.5;
	cbox[0] = AABox(box.min, cent);
	cbox[1] = AABox(Vector3(cent.x, box.min.y, box.min.z), Vector3(box.max.x, cent.y, cent.z));
	cbox[2] = AABox(Vector3(cent.x, cent.y, box.min.z), Vector3(box.max.x, box.max.y, cent.z));
	cbox[3] = AABox(Vector3(box.min.x, cent.y, box.min.z), Vector3(cent.x, box.max.y, cent.z));

	cbox[4] = AABox(Vector3(box.min.x, box.min.y, cent.z), Vector3(cent.x, cent.y, box.max.z));
	cbox[5] = AABox(Vector3(cent.x, box.min.y, cent.z), Vector3(box.max.x, cent.y, box.max.z));
	cbox[6] = AABox(cent, box.max);
	cbox[7] = AABox(Vector3(box.min.x, cent.y, cent.z), Vector3(cent.x, box.max.y, box.max.z));

	for(int i=0; i<8; i++) {
		OctNode<T> *c = new OctNode<T>;
		node->child[i] = c;

		c->box = cbox[i];

		typename std::list<OctItem<T>*>::iterator iter = node->items.begin();
		while(iter != node->items.end()) {
			if((*iter)->box.in_box(&c->box)) {
				c->items.push_back(*iter);
				c->num_items++;
			}
			iter++;
		}

		subdivide(c, lvl + 1);
	}

	node->items.clear();
	node->num_items = 0;
}


template <typename T>
OctItem<T> *Octree<T>::intersect(const Ray &ray, SurfPoint *pt) const
{
	return intersect_rec(root, ray, pt);
}

template <typename T>
OctItem<T> *Octree<T>::intersect_rec(OctNode<T> *node, const Ray &ray, SurfPoint *pt) const
{
	if(!node || !node->box.intersect(ray)) {
		return 0;
	}

	SurfPoint pt0;
	pt0.dist = FLT_MAX;
	OctItem<T> *closest = 0;

	if(node->num_items) {
		typename std::list<OctItem<T>*>::iterator iter = node->items.begin();
		while(iter != node->items.end()) {
			SurfPoint pt;

			// XXX data must be a pointer to an object with an intersect function
			if(((*iter)->data->intersect(ray, &pt)) && pt.dist < pt0.dist) {
				pt0 = pt;
				closest = *iter;
			}
			iter++;
		}

	} else {

		for(int i=0; i<8; i++) {
			SurfPoint pt;
			OctItem<T> *item;

			if((item = intersect_rec(node->child[i], ray, &pt)) && pt.dist < pt0.dist) {
				pt0 = pt;
				closest = item;
			}
		}
	}

	if(closest && pt) {
		*pt = pt0;
	}
	return closest;
}

template <typename T>
static void traverse_rec(OctNode<T> *node, int ord, void (*op)(OctNode<T>*, void*), void *cls)
{
	if(!node) return;

	if(ord < 0) {
		op(node, cls);
	}
	for(int i=0; i<8; i++) {
		if(ord == i) {
			op(node, cls);
		}

		traverse_rec(node->child[i], ord, op, cls);
	}
	if(ord > 7) {
		op(node, cls);
	}
}

template <typename T>
void Octree<T>::traverse(int ord, void (*op)(OctNode<T>*, void*), void *cls)
{
	traverse_rec(root, ord, op, cls);
}

template <typename T>
static void gather_stats(const OctNode<T> *node, OctStats *st, int lvl)
{
	if(!node) return;

	st->height = MAX(st->height, lvl);

	if(node->child[0] == 0) {
		// leaf node
#ifndef NDEBUG
		for(int i=1; i<8; i++) {
			assert(node->child[i] == 0);
		}
#endif

		st->num_leaves++;
		st->num_items += node->num_items;

		if(node->num_items < st->min_items) {
			st->min_items = node->num_items;
		}
		if(node->num_items > st->max_items) {
			st->max_items = node->num_items;
		}
	} else {
		assert(node->num_items == 0);

		// inner node
		st->num_inner++;

		for(int i=0; i<8; i++) {
			assert(node->child[i] != 0);

			gather_stats(node->child[i], st, lvl + 1);
		}
	}
}

template <typename T>
OctNode<T> *Octree<T>::get_root()
{
	return root;
}

template <typename T>
const OctNode<T> *Octree<T>::get_root() const
{
	return root;
}

template <typename T>
bool Octree<T>::stats(OctStats *st) const
{
	if(!root) return false;

	memset(st, 0, sizeof *st);
	st->min_items = INT_MAX;
	st->max_items = 0;

	gather_stats(root, st, 0);

	st->avg_items = st->num_items / st->num_leaves;
	st->num_items = (int)items.size();
	return true;
}
