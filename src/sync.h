#ifndef SYNC_H
#define SYNC_H

/* 0 on success, -1 on error. verbose dumps the header to stdout; dry_run
   leaves both the header and the source untouched and prints previews. */
int sync_file(const char *c_path, int verbose, int dry_run);

#endif // !SYNC_H
