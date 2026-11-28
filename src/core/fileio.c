#include "fileio.h"

#include <stdio.h>
#include <stdlib.h>

char *read_file(const char *path, size_t *out_len) {
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    perror("fopen");
    return NULL;
  }

  // INFO: go to the end of the file.
  if (fseek(fp, 0, SEEK_END) != 0) {
    perror("fseek");
    fclose(fp);
    return NULL;
  }

  // INFO: get the number of bytes from the beginning.
  long size = ftell(fp);
  if (size == -1L) {
    perror("ftell");
    fclose(fp);
    return NULL;
  }

  char *buf = malloc(size + 1);
  if (buf == NULL) {
    perror("malloc");
    fclose(fp);
    return NULL;
  }

  // INFO: Return the cursor to the beginning.
  rewind(fp);

  size_t bytes_read = fread(buf, 1, size, fp);
  *out_len = bytes_read;
  buf[bytes_read] = '\0';

  fclose(fp);
  return buf;
}

int write_file(const char *path, const char *text) {
  FILE *fp = fopen(path, "w");
  if (fp == NULL) {
    perror("fopen");
    return -1;
  }

  if (fputs(text, fp) == EOF) {
    perror("fputs");
    fclose(fp);
    return -1;
  }

  if (fclose(fp) != 0) {
    perror("fclose");
    return -1;
  }

  return 0;
}
