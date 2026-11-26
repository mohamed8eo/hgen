#include "cmd_sync.h"
#include "project.h"
#include "sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void on_file(const char *path, void *ctx) {
  SyncCtx *c = ctx;
  Stats *s = &c->stats;

  switch (sync_file(path, c->verbose, c->dry_run)) {
  case SYNC_ERROR:
    s->errors++;
    break;
  case SYNC_UNCHANGED:
    s->unchanged++;
    break;
  case SYNC_CREATED:
    s->created++;
    break;
  case SYNC_UPDATED:
    s->updated++;
    break;
  case SYNC_SKIPPED:
    s->skipped++;
    break;
  }
}

int cmd_sync(int argc, char **argv, int verbose, int dry_run) {
  const char *target = argc > 2 ? argv[2] : ".";

  SyncCtx ctx = {{0}, verbose, dry_run};
  struct stat info;

  if (stat(target, &info) == -1) {
    perror(target);
    return EXIT_FAILURE;
  }

  if (S_ISDIR(info.st_mode)) {
    if (project_walk(target, on_file, &ctx))
      return EXIT_FAILURE;
  } else {
    on_file(target, &ctx);
  }

  int total = ctx.stats.created + ctx.stats.updated + ctx.stats.unchanged +
              ctx.stats.skipped + ctx.stats.errors;

  fprintf(stderr,
          "%d file%s changed, %d created(+), %d updated(~), %d unchanged, "
          "%d skipped(-), %d errors(!)\n",
          total, total == 1 ? "" : "s", ctx.stats.created, ctx.stats.updated,
          ctx.stats.unchanged, ctx.stats.skipped, ctx.stats.errors);

  return ctx.stats.errors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
