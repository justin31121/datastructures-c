#ifndef HTML_PARSER_H_H
#define HTML_PARSER_H_H

#ifdef HTML_PARSER_IMPLEMENTATION
#define STRING_IMPLEMENTATION
#define TOKENIZER_IMPLEMENTATION
#endif //HTML_PARSER_IMPLEMENTATION

#include "./tokenizer.h"
#include "./string.h"

typedef struct {  
  void *(*on_node)(string name, void *arg);
  void (*on_node_attribute)(void *node, string key, string value, void *arg);
  void (*on_node_child)(void *node, void *child, void *arg);
  void (*on_node_content)(void *node, string content, void *arg);
  
  void (*on_error)(const char *message, u64 pos, void *arg);
  void *arg;
}Html_Parse_Events;

static char *html_node_singletons[] = {"meta", "input", "img", "br", "hr"};

bool html_parse(const char *cstr, u64 cstr_len, const Html_Parse_Events *events);

#ifdef HTML_PARSER_IMPLEMENTATION

#define __html_parse_expect_doctype(_type) do{		\
    if(!tokenizer_next(t, &token)) {			\
      events->on_error(err, t->last, events->arg);	\
      return false;					\
    }							\
    if(token.type != _type) {				\
      events->on_error(err, t->last, events->arg);	\
      return false;					\
    }							\
  }while(0)

static bool html_parse_expect_doctype_html(Tokenizer *t, const Html_Parse_Events *events) {
  static const char *err = "<!doctype html>";
  
  Token token;
  __html_parse_expect_doctype(TOKENTYPE_ANGLE_OPEN);
  __html_parse_expect_doctype(TOKENTYPE_DOCTYPE);
  __html_parse_expect_doctype(TOKENTYPE_WORD);
  if(!string_eq(token.content, STRING("html"))) {
    events->on_error(err, t->last, events->arg);
    return false;
  }
  __html_parse_expect_doctype(TOKENTYPE_ANGLE_CLOSE);

  return true;
}

#define __html_parse_expect_token(_type) do{					\
    if(!tokenizer_next(t, &token)) {					\
      events->on_error(tokentype_name(_type), t->last, events->arg);	\
      return false;							\
    }									\
    if(token.type != _type) {						\
      events->on_error(tokentype_name(_type), t->last, events->arg);	\
      return false;							\
    }									\
}while(0)

#define __html_parse_peek_token(_type) do{					\
    if(!tokenizer_peek(t, &token)) {					\
      events->on_error(tokentype_name(_type), t->last, events->arg);	\
      return false;							\
    }									\
}while(0)

static bool html_parse_expect_word_concatination(Tokenizer *t, const Html_Parse_Events *events, string *result) {
  assert(result);

  Token token;
  __html_parse_expect_token(TOKENTYPE_WORD);
  
  bool concat = false;
  string temp = token.content;
  __html_parse_peek_token(TOKENTYPE_WORD);
  while(token.type == TOKENTYPE_MINUS) {
    concat = true;
    tokenizer_next(t, &token);
    __html_parse_expect_token(TOKENTYPE_WORD);
    __html_parse_peek_token(TOKENTYPE_WORD);
  }
  *result =  concat
    ? string_from(temp.data, token.content.data - temp.data)
    : temp;

  return true;
}

static bool html_parse_node_open(Tokenizer *t, const Html_Parse_Events *events, bool *closed, void **root, string *root_name) {
  assert(closed);
  assert(root);
  *closed = false;

  Token token;
  __html_parse_expect_token(TOKENTYPE_ANGLE_OPEN);
  if(!html_parse_expect_word_concatination(t, events, root_name)) {
    return false;
  }
  *root = events->on_node(*root_name, events->arg);
  
  __html_parse_peek_token(TOKENTYPE_WORD);
  while(token.type == TOKENTYPE_WORD) {
    tokenizer_next(t, &token); // attribute
    string key = token.content;

    while(token.type != TOKENTYPE_EQUALS) {
      tokenizer_next(t, &token);
      if(token.content.data != key.data) {
	u64 diff = token.content.data - key.data - key.len;
	key = string_from(key.data, key.len + diff + token.content.len);
      }
      //__html_peek_token(TOKENTYPE_WORD);
    }
    key = string_from(key.data, key.len - 1);

    __html_parse_peek_token(TOKENTYPE_EQUALS);
    Tokentype quotation = TOKENTYPE_DOUBLE_QUOTATION;
    if(token.type == TOKENTYPE_DOUBLE_QUOTATION) {
      __html_parse_expect_token(TOKENTYPE_DOUBLE_QUOTATION);
    } else {
      __html_parse_expect_token(TOKENTYPE_QUOTATION);
      quotation = TOKENTYPE_QUOTATION;
    }

    __html_parse_peek_token(quotation);
    string value = token.content;
    while(token.type != quotation) {
      tokenizer_next(t, &token);
      if(token.content.data != value.data) {
	u64 diff = token.content.data - value.data - value.len;
	value = string_from(value.data, value.len + diff + token.content.len);	
      }
      __html_parse_peek_token(quotation);
    }

    __html_parse_expect_token(quotation);
    __html_parse_peek_token(TOKENTYPE_ANGLE_CLOSE);
    
    events->on_node_attribute(*root, key, value, events->arg);
  }


  __html_parse_peek_token(TOKENTYPE_ANGLE_CLOSE);
  if(token.type == TOKENTYPE_SLASH) {
    if(closed) {
      *closed = true;
    }
    tokenizer_next(t, &token);
  }
  
  __html_parse_expect_token(TOKENTYPE_ANGLE_CLOSE);
  return true;
}

static bool html_parse_expect_node_close(Tokenizer *t, const Html_Parse_Events *events, string root_name) {
  Token token;
  __html_parse_expect_token(TOKENTYPE_ANGLE_OPEN);
  __html_parse_expect_token(TOKENTYPE_SLASH);
  string result;
  if(!html_parse_expect_word_concatination(t, events, &result)) {
    return false;
  }
  if(!string_eq(result, root_name)) {
    return false;
  }
  __html_parse_expect_token(TOKENTYPE_ANGLE_CLOSE);
  return true;
}

#define __html_parse_peek_n(n, _type) do{					\
    if(!tokenizer_peek_n(t, &token, n)) {				\
      events->on_error(tokentype_name(_type), t->last, events->arg); \
      return NULL;							\
    }									\
  }while(0)

bool html_parse_peek_n_word_concatenation(Tokenizer *t, int n, string *result) {
  Token token;
  if(!tokenizer_peek_n(t, &token, n)) {
    return false;
  }
  if(token.type != TOKENTYPE_WORD) {
    return false;
  }

  u32 depth = 1;

  bool concat = false;
  string temp = token.content;
  if(!tokenizer_peek_n(t, &token, n+1)) {
    return false;
  }
  while(token.type == TOKENTYPE_MINUS) {
    concat = true;
    if(!tokenizer_peek_n(t, &token, n+1+depth)) {
      return false;
    }
    if(token.type != TOKENTYPE_WORD) {
      return false;
    }
    depth++;

    if(!tokenizer_peek_n(t, &token, n+1+depth)) {
      return false;
    }
    depth++;
  }
  
  if(result) {
    *result = concat
    ? string_from(temp.data, token.content.data - temp.data)
    : temp;
  }
  return true;
}

static void *html_parse_node(Tokenizer *t, const Html_Parse_Events *events) {
  
  bool closed;
  void *root;
  string root_name;
  
  if(!html_parse_node_open(t, events, &closed, &root, &root_name)) {
    return NULL;
  }
  if(closed) {
    return root;
  }

  bool isScript = string_eq(root_name, STRING("script"));

  u32 singletons_len = sizeof(html_node_singletons)/sizeof(html_node_singletons[0]);
  for(u32 i=0;i<singletons_len;i++) {
    string singleton = string_from_cstr(html_node_singletons[i]);
    if(string_eq(root_name, singleton)) {
      return root;
    }
  }

  Token token;
  string current = {0};

  while(true) {    
    __html_parse_peek_n(1, TOKENTYPE_ANGLE_OPEN);
    if(token.type == TOKENTYPE_ANGLE_OPEN) {
      __html_parse_peek_n(2, TOKENTYPE_SLASH);
      if(token.type == TOKENTYPE_SLASH) {
	string content;
	if(!html_parse_peek_n_word_concatenation(t, 3, &content)) {
	  events->on_error(tokentype_name(TOKENTYPE_WORD), t->last, events->arg);
	  return NULL;
	}
	if(string_eq(root_name, content)) {
	  events->on_node_content(root, current, events->arg);
	  html_parse_expect_node_close(t, events, root_name);
	  return root;
	} else {
	  if(!isScript) {
	    events->on_error(tokentype_name(TOKENTYPE_WORD), t->last, events->arg);
	    return NULL;
	  } else {
	    tokenizer_next(t, &token);
	  }
	}	
      } else {
	bool skipped = false;
	if(token.type == TOKENTYPE_EXCLAMATION) {
	  __html_parse_peek_n(3, TOKENTYPE_MINUS);
	  if(token.type == TOKENTYPE_MINUS) {
	    __html_parse_peek_n(4, TOKENTYPE_MINUS);
	    if(token.type == TOKENTYPE_MINUS) {
	      tokenizer_next(t, &token);
	      tokenizer_next(t, &token);
	      tokenizer_next(t, &token);
	      tokenizer_next(t, &token);

	      skipped = true;
	      while(true) {
		__html_parse_peek_n(1, TOKENTYPE_MINUS);
		if(token.type == TOKENTYPE_MINUS) {
		  __html_parse_peek_n(2, TOKENTYPE_MINUS);
		  if(token.type == TOKENTYPE_MINUS) {
		    __html_parse_peek_n(3, TOKENTYPE_ANGLE_CLOSE);
		    if(token.type == TOKENTYPE_ANGLE_CLOSE) {
		      tokenizer_next(t, &token);
		      tokenizer_next(t, &token);
		      tokenizer_next(t, &token);
		      break;
		    }
		  }
		}

		tokenizer_next(t, &token);
	      }
	    }
	  }
	}

	if(!skipped) {
	  if(!isScript) {
	    void *child = html_parse_node(t, events);
	    if(!child) {
	      return NULL;
	    }
	    events->on_node_child(root, child, events->arg);
	  } else {
	    tokenizer_next(t, &token);
	  }
	}	
      }
    } else {
      tokenizer_next(t, &token);

      u64 diff = token.content.data - current.data - current.len;
      current = current.len
	? string_from(current.data, current.len + diff + token.content.len)
	: token.content;
    }
  }

  panic("unreachable");
  return NULL;
}

bool html_parse(const char *cstr, u64 cstr_len, const Html_Parse_Events *events) {
  assert(events);
  assert(events->on_error);
  assert(events->on_node);
  assert(events->on_node_attribute);
  assert(events->on_node_child);
  assert(events->on_node_content);
  
  Tokenizer tokenizer = {cstr, cstr_len, 0, 0};
  if(!html_parse_expect_doctype_html(&tokenizer, events)) {
    return false;
  }
  if(!html_parse_node(&tokenizer, events)) {
    return false;
  }
  
  return true;
}

#endif //HTML_PARSER_IMPLEMENTATION

#endif //HTML_PARSER_H_H
