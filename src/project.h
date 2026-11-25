#ifndef PROJECT_H
#define PROJECT_H

typedef void (*FileCallback)(const char *path, void *ctx);
int project_walk(const char *root, FileCallback cb, void *ctx);

#endif // !PROJECT_H
