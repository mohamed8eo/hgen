#include "test.h"
#include "fileio.h"

static const char *input_path = "build/test_fileio_input.txt";

static void write_input(const char *text) {
  FILE *fp = fopen(input_path, "w");
  CHECK(fp != NULL);
  if (fp == NULL)
    return;
  fputs(text, fp);
  fclose(fp);
}

int main(void) {
  write_input("hello\nworld\n");
  size_t len = 0;
  char *buf = read_file(input_path, &len);
  CHECK(buf != NULL);
  if (buf != NULL) {
    CHECK_SIZE(len, 12);
    CHECK_STR(buf, "hello\nworld\n");
    free(buf);
  }

  write_input("");
  len = 99;
  buf = read_file(input_path, &len);
  CHECK(buf != NULL);
  if (buf != NULL) {
    CHECK_SIZE(len, 0);
    CHECK_STR(buf, "");
    free(buf);
  }

  len = 99;
  CHECK(read_file("build/definitely_missing_file.txt", &len) == NULL);

  remove(input_path);
  TEST_REPORT("fileio");
}
