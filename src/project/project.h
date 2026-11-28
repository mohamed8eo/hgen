#ifndef PROJECT_H
#define PROJECT_H

#define HGENDIR ".hgen"
#define HGENIGNORE ".hgenignore"

typedef void (*FileCallback)(const char *path, void *ctx);
/* start: where to walk.  root: where ignore rules come from (may be NULL). */
int project_walk(const char *start, const char *root, FileCallback cb, void *ctx);
char *project_find_root(const char *start);

#endif // !PROJECT_H
