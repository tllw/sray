#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datapath.h"

#if defined(WIN32) || defined(__WIN32__)
#include <malloc.h>
#include <io.h>

#ifndef F_OK
#define F_OK	0
#endif

int setenv(const char *name, const char *value, int overwrite);
#else
#include <unistd.h>
#endif


static struct path_node {
	char *path;
	struct path_node *next;
} *plist;


static struct path_node *build_path_list(const char *path);
static void free_list(struct path_node *list);

int set_path(const char *path)
{
	if(setenv("SRAY_DATA_PATH", path, 1) == -1) {
		return -1;
	}

	free_list(plist);
	plist = 0;
	return 0;
}

int add_path(const char *path)
{
	char *val, *nval;
	
	if(!(val = getenv("SRAY_DATA_PATH"))) {
		return set_path(path);
	}

	nval = alloca(strlen(val) + strlen(path) + 2);
	sprintf(nval, "%s:%s", path, val);
	return set_path(nval);
}

const char *get_path(void)
{
	return getenv("SRAY_DATA_PATH");
}

int find_file(const char *fname, char *pathname, int pnsz)
{
	struct path_node *p = plist;

	if(!plist) {
		p = plist = build_path_list(getenv("SRAY_DATA_PATH"));
	}

	while(p) {
		snprintf(pathname, pnsz, "%s/%s", p->path, fname);

		if(access(pathname, F_OK) == 0) {
			return 0;
		}

		p = p->next;
	}

	return -1;
}

static struct path_node *build_path_list(const char *path)
{
	char *pathstr, *start, *end;
	struct path_node *p, *plist, *ptail;

	if(!path) return 0;

	plist = ptail = 0;
	
	pathstr = alloca(strlen(path) + 1);
	strcpy(pathstr, path);

	start = end = pathstr;
	while(*end) {
		while(*end && *end != ':') end++;
		*end = 0;

		if(!(p = malloc(sizeof *p)) || !(p->path = malloc(strlen(start) + 1))) {
			perror("malloc failed");
			free_list(plist);
			plist = 0;
			return 0;
		}
		strcpy(p->path, start);
		p->next = 0;

		if(ptail) {
			ptail->next = p;
			ptail = p;
		} else {
			plist = ptail = p;
		}

		start = end + 1;
	}

	return plist;
}

static void free_list(struct path_node *list)
{
	if(!list) return;

	free_list(list->next);
	free(list->path);
	free(list);
}

#if defined(WIN32) || defined(__WIN32__)
int setenv(const char *name, const char *value, int overwrite)
{
	if(!overwrite && getenv(name)) {
		return 0;
	}
	return SetEnvironmentVariable(name, value) ? 0 : -1;
}
#endif