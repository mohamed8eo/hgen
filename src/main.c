#include "sync.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc < 2) { /* usage */
    return EXIT_FAILURE;
  }
  return sync_file(argv[1]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
