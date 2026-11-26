#include "cmd.h"
#include "cmd_sync.h"

#include <stdio.h>
#include <stdlib.h>

static void usage(FILE *stream, const char *prog) {
  fprintf(stream,
          "usage: %s [--verbose] [--dry-run] file.c\n"
          "       %s init | sync | auto-sync [target]\n",
          prog, prog);
}

/* cmd_sync reads its target from argv[2]; normalize the legacy form so a bare
   file path lands there too. */
static int run_sync(char **argv, char *target, int verbose, int dry_run) {
  char *sync_argv[] = {argv[0], "sync", target, NULL};
  return cmd_sync(3, sync_argv, verbose, dry_run);
}

int main(int argc, char **argv) {
  CmdType cmd = parse_command(argc, argv);

  if (cmd == CMD_UNKNOWN) {
    usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }
  if (cmd & CMD_HELP) {
    usage(stdout, argv[0]);
    return EXIT_SUCCESS;
  }

  int verbose = (cmd & CMD_VERBOSE) != 0;
  int dry_run = (cmd & CMD_DRY_RUN) != 0;

  switch (cmd & CMD_COMMANDS) {
  case CMD_SYNC:
  case CMD_AUTO_SYNC:
    return cmd_sync(argc, argv, verbose, dry_run);
  case CMD_INIT:
    fprintf(stderr, "%s: 'init' is not implemented yet\n", argv[0]);
    return EXIT_FAILURE;
  case CMD_NONE: {
    char *path = command_path(argc, argv);
    if (path == NULL) {
      usage(stderr, argv[0]);
      return EXIT_FAILURE;
    }
    return run_sync(argv, path, verbose, dry_run);
  }
  default:
    usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }
}
