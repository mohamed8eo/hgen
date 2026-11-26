#include "cmd.h"

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

    if (arg[0] == '-' && arg[1] != '\0') continue;
    if (is_command(arg)) continue;
    if (path != NULL) return NULL;
    path = arg;
  }

  return path;
}
