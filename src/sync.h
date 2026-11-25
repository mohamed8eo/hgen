#ifndef SYNC_H
#define SYNC_H

typedef enum {
  SYNC_ERROR,
  SYNC_UNCHANGED,
  SYNC_CREATED,
  SYNC_UPDATED,
  SYNC_SKIPPED /* nothing to export (e.g. main.c) */
} SyncResult;

/* verbose dumps the header to stdout; dry_run leaves both the header and the
   source untouched and prints previews. */
SyncResult sync_file(const char *c_path, int verbose, int dry_run);

#endif // !SYNC_H
