#ifndef CMD_H
#define CMD_H

/* init/sync/auto-sync are commands and can be used one at a time.
   --verbose and --dry-run are modifiers and can be combined with each other. */
typedef enum {
  CMD_NONE = 0,
  CMD_INIT = 1 << 0,
  CMD_VERBOSE = 1 << 1,
  CMD_DRY_RUN = 1 << 2,
  CMD_SYNC = 1 << 3,
  CMD_AUTO_SYNC = 1 << 4,
  CMD_HELP = 1 << 5,
  CMD_UNKNOWN = 1 << 6
} CmdType;

#define CMD_COMMANDS (CMD_INIT | CMD_SYNC | CMD_AUTO_SYNC)

/* Returns the parsed command/modifier mask. Bare arguments (paths) are
   ignored here; use command_path() to retrieve them. */
CmdType parse_command(int argc, char **argv);

/* Returns the single non-option, non-command argument, or NULL when there is
   none or more than one. */
char *command_path(int argc, char **argv);

#endif // !CMD_H
