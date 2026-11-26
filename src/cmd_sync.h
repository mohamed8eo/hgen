#ifndef CMD_SYNC_H
#define CMD_SYNC_H

typedef struct {
  int created, updated, unchanged, skipped, errors;
} Stats;

/* on_file receives this through project_walk's void *ctx. */
typedef struct {
  Stats stats;
  int verbose;
  int dry_run;
} SyncCtx;

void on_file(const char *path, void *ctx);
int cmd_sync(int argc, char **argv, int verbose, int dry_run);

#endif // !CMD_SYNC_H
