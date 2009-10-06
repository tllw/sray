#ifndef DATAPATH_H_
#define DATAPATH_H_

#ifdef __cplusplus
extern "C" {
#endif

int set_path(const char *path);
int add_path(const char *path);
const char *get_path(void);

int find_file(const char *fname, char *pathname, int pnsz);

#ifdef __cplusplus
}
#endif

#endif	/* DATAPATH_H_ */
