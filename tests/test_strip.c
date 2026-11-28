#include "test.h"
#include "core/strip.h"

static char *strip_and_check(const char *in) {
  char *out = strip_comments_and_literals(in);
  CHECK(out != NULL);
  if (out != NULL)
    CHECK_SIZE(strlen(out), strlen(in)); /* positions are preserved */
  return out;
}

int main(void) {
  {
    char *out = strip_and_check("");
    CHECK_STR(out, "");
    free(out);
  }
  {
    char *out = strip_and_check("int x;");
    CHECK_STR(out, "int x;");
    free(out);
  }
  {
    char *out = strip_and_check("// hi\nint x;");
    CHECK_STR(out, "     \nint x;");
    free(out);
  }
  {
    char *out = strip_and_check("/* hi */int x;");
    CHECK_STR(out, "        int x;");
    free(out);
  }
  {
    char *out = strip_and_check("/* a\nb */\nint x;");
    CHECK_STR(out, "    \n    \nint x;");
    free(out);
  }
  {
    char *out = strip_and_check("char *s = \"a{b}c\";");
    CHECK_STR(out, "char *s =        ;");
    free(out);
  }
  {
    char *out = strip_and_check("char c = '\\n';");
    CHECK_STR(out, "char c =     ;");
    free(out);
  }
  {
    char *out = strip_and_check("int x; /* unterminated");
    CHECK(strstr(out, "unterminated") == NULL);
    free(out);
  }
  {
    char *out = strip_and_check("const char *s = \"// hi\";");
    CHECK(strstr(out, "//") == NULL);
    CHECK(strchr(out, '"') == NULL);
    free(out);
  }
  {
    char *out = strip_and_check("/* ZZZ */ // YYY\nchar c = '{';\n");
    CHECK(strstr(out, "ZZZ") == NULL);
    CHECK(strstr(out, "YYY") == NULL);
    CHECK(strchr(out, '{') == NULL);
    CHECK(out[strlen(out) - 1] == '\n');
    free(out);
  }

  TEST_REPORT("strip");
}
