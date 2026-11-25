#ifndef CMD_SYNC_H
#define CMD_SYNC_H

typedef struct {
  int updated;
  int unchanged;
  int errors;
} Stats;

void on_file(const char *path, void *ctx);

#endif // !CMD_SYNC_H
