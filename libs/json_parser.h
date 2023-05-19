#ifndef JSON_PARSER_H_H
#define JSON_PARSER_H_H

#ifdef JSON_PARSER_IMPLEMENTATION
#define TOKENIZER_IMPLEMENTATION
#endif //JSON_PARSER_IMPLEMENTATION

#ifndef JSON_PARSER_DEF
#define JSON_PARSER_DEF static inline
#endif //JSON_PARSER_DEF

#include "./tokenizer.h"

typedef enum{
  JSON_PARSE_TYPE_NULL,
  JSON_PARSE_TYPE_TRUE,
  JSON_PARSE_TYPE_FALSE,
  JSON_PARSE_TYPE_NUMBER,
  JSON_PARSE_TYPE_STRING,
  JSON_PARSE_TYPE_OBJECT,
  JSON_PARSE_TYPE_ARRAY
}Json_Parse_Type;

typedef struct {
  bool (*on_elem)(Json_Parse_Type type, string content, void *arg, void **elem);

  void (*on_object_elem)(void *object, string key, void *elem, void *arg);
  void (*on_array_elem)(void *array, void *elem, void *arg);

  void (*on_error)(const char *message, u64 pos, void *arg);
    
  void *arg;
}Json_Parse_Events;

JSON_PARSER_DEF bool json_parse2(const char *cstr, size_t cstr_len, const Json_Parse_Events *events);
JSON_PARSER_DEF const char *json_parse_type_name(Json_Parse_Type type);

#ifdef JSON_PARSER_IMPLEMENTATION

JSON_PARSER_DEF const char *json_parse_type_name(Json_Parse_Type type) {
  switch(type) {
  case JSON_PARSE_TYPE_NULL:
    return "JSON_PARSE_TYPE_NULL";
  case JSON_PARSE_TYPE_TRUE:
    return "JSON_PARSE_TYPE_TRUE";
  case JSON_PARSE_TYPE_FALSE:
    return "JSON_PARSE_TYPE_FALSE";
  case JSON_PARSE_TYPE_NUMBER:
    return "JSON_PARSE_TYPE_NUMBER";
  case JSON_PARSE_TYPE_STRING:
    return "JSON_PARSE_TYPE_STRING";
  case JSON_PARSE_TYPE_OBJECT:
    return "JSON_PARSE_TYPE_OBJECT";
  case JSON_PARSE_TYPE_ARRAY:
    return "JSON_PARSE_TYPE_ARRAY";
  default:
    return "UNKNOWN";	
  }
}

JSON_PARSER_DEF bool json_parse_constant(Tokentype type, Tokenizer *t, const Json_Parse_Events *events, void **elem) {
  Token token;
  if(!tokenizer_peek(t, &token)) {
    return false;
  }
  if(token.type != type) {
    return false;
  }
  tokenizer_next(t, &token);
  if(events->on_elem != NULL) {
    Json_Parse_Type parse_type = -1;
    switch(type) {
    case TOKENTYPE_JSON_NULL:
      parse_type = JSON_PARSE_TYPE_NULL;
      break;
    case TOKENTYPE_JSON_TRUE:
      parse_type = JSON_PARSE_TYPE_TRUE;
      break;
    case TOKENTYPE_JSON_FALSE:
      parse_type = JSON_PARSE_TYPE_FALSE;
      break;
    default:
      assert(!"unreachable json_parse_constant");
    }
    if((int) parse_type == -1) assert(!"parse_type should not be negative");
    if(!events->on_elem(parse_type, token.content, events->arg, elem)) {
      return false;
    }
  }
  return true;
}

JSON_PARSER_DEF bool json_parse_number(Tokenizer *t, const Json_Parse_Events *events, void *elem) {
  Token token;
  string value = {0};
  u32 start = t->pos;

  if(!tokenizer_peek(t, &token)) {
    t->pos = start;
    return false;
  }

  bool parsed_minus = false;

  if(token.type == TOKENTYPE_MINUS) {
    tokenizer_next(t, &token);
    value = token.content;
    if(!tokenizer_peek(t, &token)) {
      t->pos = start;
      return false;
    }
    parsed_minus = true;
  }

  if(token.type == TOKENTYPE_WORD) {

    for(size_t i=0;i<token.content.len;i++) {
      if(!isdigit(token.content.data[i])) {
	t->pos = start;
	return false;
      }
    }
    tokenizer_next(t, &token);

    if(!value.len) {
      value = token.content;
    }
    else if(token.content.data != value.data) {
      size_t diff = token.content.data - value.data - value.len;
      value = string_from(value.data, value.len + diff + token.content.len);	
    }

    if(!tokenizer_peek(t, &token)) {
      if(events->on_elem != NULL) {
	if(!events->on_elem(JSON_PARSE_TYPE_NUMBER, value, events->arg, elem)) {
	  t->pos = start;
	  return false;
	}
      }
      return true;
    }
  }

  if(token.type == TOKENTYPE_DOT) {
    if(value.len == 1 && parsed_minus) {
      t->pos = start;
      return false;
    }
    tokenizer_next(t, &token);
     
    if(!value.len) {
      value = token.content;
    }
    else if(token.content.data != value.data) {
      size_t diff = token.content.data - value.data - value.len;
      value = string_from(value.data, value.len + diff + token.content.len);	
    }
    
    if(!tokenizer_peek(t, &token)) {
      if(events->on_elem != NULL) {
	if(!events->on_elem(JSON_PARSE_TYPE_NUMBER, value, events->arg, elem)) {
	  t->pos = start;
	  return false;
	}
      }
      return true;
    }
  }

  if(token.type == TOKENTYPE_WORD) {

    for(size_t i=0;i<token.content.len;i++) {
      if(!isdigit(token.content.data[i])) {
	t->pos = start;
	return false;
      }
    }
    tokenizer_next(t, &token);

    if(!value.len) {
      value = token.content;
    }
    else if(token.content.data != value.data) {
      size_t diff = token.content.data - value.data - value.len;
      value = string_from(value.data, value.len + diff + token.content.len);	
    }
  }

  if(!value.len) {
    t->pos = start;
    return false;
  }

  if(events->on_elem != NULL) {
    if(!events->on_elem(JSON_PARSE_TYPE_NUMBER, value, events->arg, elem)) {
      t->pos = start;
      return false;
    }

  }
    
  return true;
}

JSON_PARSER_DEF bool json_parse_string(Tokenizer *t, const Json_Parse_Events *events, void **elem) {
  Token token;
  u32 start = t->pos;
  if(!tokenizer_next(t, &token)) {
    t->pos = start;
    return false;
  }
  if(token.type != TOKENTYPE_DOUBLE_QUOTATION) {
    t->pos = start;
    return false;
  }

  if(!tokenizer_peek(t, &token)) {
    t->pos = start;
    return false;
  }
  string value = token.content;
  while(token.type != TOKENTYPE_DOUBLE_QUOTATION) {

    tokenizer_next(t, &token);
    if(token.content.data != value.data) {
      size_t diff = token.content.data - value.data - value.len;
      value = string_from(value.data, value.len + diff + token.content.len);	
    }

    if(token.type == TOKENTYPE_BACK_SLASH) {
      if(!tokenizer_next(t, &token)) {
	t->pos = start;
	return false;
      }
      if(token.content.data != value.data) {
	size_t diff = token.content.data - value.data - value.len;
	value = string_from(value.data, value.len + diff + token.content.len);	
      }
    }
    
    if(!tokenizer_peek(t, &token)) {
      t->pos = start;
      return false;
    }
  }
  
  if(!tokenizer_next(t, &token)) {
    t->pos = start;
    return false;
  }
  if(token.type != TOKENTYPE_DOUBLE_QUOTATION) {
    t->pos = start;
    return false;
  }

  if(events->on_elem != NULL) {
    if(!events->on_elem(JSON_PARSE_TYPE_STRING, value, events->arg, elem)) {
      t->pos = start;
      return false;
    }
  }
    
  return true;
}

JSON_PARSER_DEF bool json_parse_json(Tokenizer *t, const Json_Parse_Events *events, void **elem);

JSON_PARSER_DEF bool json_parse_object(Tokenizer *t, const Json_Parse_Events *events, void **object) {
  Token token;
  u32 start = t->pos;
  if(!tokenizer_next(t, &token)) {
    t->pos = start;
    return false;
  }
  if(token.type != TOKENTYPE_CURLY_OPEN) {
    t->pos = start;
    return false;
  }

  if(events->on_elem != NULL) {
    string empty_string = {token.content.data, 0};
    if(!events->on_elem(JSON_PARSE_TYPE_OBJECT,
			empty_string,
			events->arg, object)) {
      t->pos = start;
      return false;
    }
  }

  if(!tokenizer_peek(t, &token)) {
    t->pos = start;
    return false;
  }
    
  while(token.type != TOKENTYPE_CURLY_CLOSE) {

    string key = {0};
    {
      if(!tokenizer_next(t, &token)) {
	t->pos = start;
	return false;
      }
      if(token.type != TOKENTYPE_DOUBLE_QUOTATION) {
	t->pos = start;
	return false;
      }

      if(!tokenizer_peek(t, &token)) {
	t->pos = start;
	return false;
      }
      key = token.content;
      while(token.type != TOKENTYPE_DOUBLE_QUOTATION) {

	tokenizer_next(t, &token);
	if(token.content.data != key.data) {
	  size_t diff = token.content.data - key.data - key.len;
	  key = string_from(key.data, key.len + diff + token.content.len);	
	}

	if(token.type == TOKENTYPE_BACK_SLASH) {
	  if(!tokenizer_next(t, &token)) {
	    t->pos = start;
	    return false;
	  }
	}
    
	if(!tokenizer_peek(t, &token)) {
	  t->pos = start;
	  return false;
	}
      }
  
      if(!tokenizer_next(t, &token)) {
	t->pos = start;
	return false;
      }
      if(token.type != TOKENTYPE_DOUBLE_QUOTATION) {
	t->pos = start;
	return false;
      }

    }

    if(!tokenizer_next(t, &token)) {
      t->pos = start;
      return false;
    }
    if(token.type != TOKENTYPE_COLON) {
      t->pos = start;
      return false;
    }

    void *elem = NULL;
    if(!json_parse_json(t, events, &elem)) {
      t->pos = start;
      return false;
    }

    if(events->on_object_elem != NULL) {
      events->on_object_elem(*object, key, elem, events->arg);
    }	
       
    if(!tokenizer_peek(t, &token)) {
      t->pos = start;
      return false;
    }
    if(token.type != TOKENTYPE_COMMA) break;
    tokenizer_next(t, &token);
  }

  if(!tokenizer_next(t, &token)) {
    t->pos = start;
    return false;
  }
  if(token.type != TOKENTYPE_CURLY_CLOSE) {
    t->pos = start;
    return false;
  }
    
  return true;
}

JSON_PARSER_DEF bool json_parse_array(Tokenizer *t, const Json_Parse_Events *events, void **array) {

  Token token;
  u32 start = t->pos;
  if(!tokenizer_next(t, &token)) {
    t->pos = start;
    return false;
  }
  if(token.type != TOKENTYPE_BRACKET_OPEN) {
    t->pos = start;
    return false;
  }

  if(events->on_elem != NULL) {
    string empty_string = {0};
    if(!events->on_elem(JSON_PARSE_TYPE_ARRAY,
			empty_string,
			events->arg, array)) {
      t->pos = start;
      return false;
    }
  }

  if(!tokenizer_peek(t, &token)) {
    t->pos = start;
    return false;
  }
    
  while(token.type != TOKENTYPE_BRACKET_CLOSE) {
	
    void *elem = NULL;
    if(!json_parse_json(t, events, &elem)) {
      t->pos = start;
      return false;
    }

    if(events->on_array_elem != NULL) {
      events->on_array_elem(*array, elem, events->arg);
    }	
		
    if(!tokenizer_peek(t, &token)) {
      t->pos = start;
      return false;
    }
    if(token.type != TOKENTYPE_COMMA) break;
    tokenizer_next(t, &token);

  }
    
  if(!tokenizer_next(t, &token)) {
    t->pos = start;
    return false;
  }
  if(token.type != TOKENTYPE_BRACKET_CLOSE) {
    t->pos = start;
    return false;
  }


  return true;
}

JSON_PARSER_DEF bool json_parse_json(Tokenizer *t, const Json_Parse_Events *events, void **elem) {
  if(json_parse_constant(TOKENTYPE_JSON_NULL, t, events, elem)) { //syntax comes from tokenizer.h
    return true;
  } else if(json_parse_constant(TOKENTYPE_JSON_FALSE, t, events, elem)) {
    return true;
  } else if(json_parse_constant(TOKENTYPE_JSON_TRUE, t, events, elem)) {
    return true;
  } else if(json_parse_number(t, events, elem)) {
    return true;
  } else if(json_parse_string(t, events, elem)) {
    return true;
  } else if(json_parse_object(t, events, elem)) {
    return true;
  } else if(json_parse_array(t, events, elem)) {
    return true;
  }
  return false;
}

JSON_PARSER_DEF bool json_parse2(const char *cstr, size_t cstr_len, const Json_Parse_Events *events) {
  if(!events) {
    return false;
  }

  Tokenizer tokenizer = {cstr, (u32) cstr_len, 0, 0};
  void *elem = NULL;
  if(!json_parse_json(&tokenizer, events, &elem)) {
    return false;
  }

  if(tokenizer.pos != cstr_len) {
    return false;
  }
  
  return true;
}

#endif //JSON_PARSER_IMPLEMENTATION

#endif //JSON_PARSER_H_H
