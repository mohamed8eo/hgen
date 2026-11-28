#include "../cli/cmd.h"
#include "../cli/cmd_sync.h"
#include "cmd_init.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_command(const char *arg) {
  return strcmp(arg, "init") == 0 || strcmp(arg, "sync") == 0 ||
         strcmp(arg, "auto-sync") == 0;
}

static CmdType with_command(CmdType cmd, CmdType next) {
  return (cmd & CMD_COMMANDS) ? CMD_UNKNOWN : (CmdType)(cmd | next);
}

CmdType parse_command(int argc, char **argv) {
  CmdType cmd = CMD_NONE;

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];

    if (strcmp(arg, "--verbose") == 0) {
      cmd = (CmdType)(cmd | CMD_VERBOSE);
    } else if (strcmp(arg, "--dry-run") == 0) {
      cmd = (CmdType)(cmd | CMD_DRY_RUN);
    } else if (strcmp(arg, "--help") == 0) {
      cmd = (CmdType)(cmd | CMD_HELP);
    } else if (strcmp(arg, "init") == 0) {
      cmd = with_command(cmd, CMD_INIT);
    } else if (strcmp(arg, "sync") == 0) {
      cmd = with_command(cmd, CMD_SYNC);
    } else if (strcmp(arg, "auto-sync") == 0) {
      cmd = with_command(cmd, CMD_AUTO_SYNC);
    } else if (arg[0] == '-' && arg[1] != '\0') {
      return CMD_UNKNOWN;
    }
  }

  return cmd;
}

char *command_path(int argc, char **argv) {
  char *path = NULL;

  for (int i = 1; i < argc; i++) {
    char *arg = argv[i];

    if (arg[0] == '-' && arg[1] != '\0')
      continue;
    if (is_command(arg))
      continue;
    if (path != NULL)
      return NULL;
    path = arg;
  }

  return path;
}

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

int cmd_run(int argc, char **argv) {
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
    return cmd_init(command_path(argc, argv));
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
