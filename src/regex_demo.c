#include <stdio.h>

#define REGEX_IMPLEMENTATION
#include "../libs/regex.h"

#include "../libs/util.h"

int main() {

  Regex regex;
  if(!regex_compile(&regex, "a[:print:]*c")) {
    panic("regex_compile");
  }
 
  regex_print(&regex);
  printf("==============================================================\n");  

  const char *cstr = "aBB00120_39192aBc";
  size_t off, len;
  if(regex_match(&regex, cstr, &off, &len)) {
    printf("GOOD: regex matched: '%s' with '%.*s'\n", cstr, (int) len, cstr + off);
  } else {
    printf("BAD: regex did not matched: '%s'\n", cstr);
  }
  
  regex_free(&regex);

  return 0;
}

