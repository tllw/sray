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
#ifndef COMMON_XMLTREE_H_
#define COMMON_XMLTREE_H_

#include <stdio.h>

enum {
	ATYPE_STR,
	ATYPE_INT,
	ATYPE_FLT,
	ATYPE_VEC
};

struct xml_attr {
	char *name;
	char *str;

	int type;
	int ival;
	float fval;
	float vval[4];

	struct xml_attr *next;
};

struct xml_node {
	char *name;
	char *cdata;
	struct xml_attr *attr;		/* attribute list */

	int chld_count;

	struct xml_node *up;				/* parent pointer */
	struct xml_node *chld, *chld_tail;	/* children list */
	struct xml_node *next;				/* next sibling */
};

#ifdef __cplusplus
extern "C" {
#endif

struct xml_attr *xml_create_attr(const char *name, const char *val);
void xml_free_attr(struct xml_attr *attr);
void xml_free_attr_list(struct xml_attr *alist);

struct xml_attr *xml_get_attr(struct xml_node *node, const char *attr_name);

struct xml_node *xml_create_tree(void);
void xml_free_tree(struct xml_node *x);

struct xml_node *xml_read_tree(const char *fname);
struct xml_node *xml_read_tree_file(FILE *fp);

int xml_write_tree(struct xml_node *x, const char *fname);
int xml_write_tree_file(struct xml_node *x, FILE *fp);

void xml_add_child(struct xml_node *x, struct xml_node *chld);
void xml_remove_subtree(struct xml_node *sub);

#ifdef __cplusplus
}
#endif

#endif	/* COMMON_XMLTREE_H_ */
