#include "cmd.h"

#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc < 2)
    return EXIT_FAILURE;

  return cmd_run(argc, argv);
}
