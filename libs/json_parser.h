#ifndef JSON_PARSER_H_H
#define JSON_PARSER_H_H

#ifdef JSON_PARSER_IMPLEMENTATION
#define TOKENIZER_IMPLEMENTATION
#endif //JSON_PARSER_IMPLEMENTATION

#ifndef JSON_PARSER_DEF
#define JSON_PARSER_DEF static inline
#endif //JSON_PARSER_DEF

#include "./tokenizer.h"

typedef struct {
  void *arg;
}Json_Parse_Events;

JSON_PARSER_DEF bool json_parse(const char *cstr, u64 cstr_len, const Json_Parse_Events *events);

#ifdef JSON_PARSER_IMPLEMENTATION

JSON_PARSER_DEF bool json_parse(const char *cstr, u64 cstr_len, const Json_Parse_Events *events) {
  if(!events) {
    return false;
  }

  Tokenizer tokenizer = {cstr, cstr_len, 0, 0};
  return true;
}

#endif //JSON_PARSER_IMPLEMENTATION

#endif //JSON_PARSER_H_H
