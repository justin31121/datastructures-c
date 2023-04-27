#ifndef HTML_PARSER_H_H
#define HTML_PARSER_H_H

#ifdef HTML_PARSER_IMPLEMENTATION
#define TOKENIZER_IMPLEMENTATION
#endif //HTML_PARSER_IMPLEMENTATION

#ifndef HTML_PARSER_DEF
#define HTML_PARSER_DEF static inline
#endif //HTML_PARSER_DEF

#include "./tokenizer.h"

typedef struct {  
  bool (*on_node)(string name, void *arg, void **node);
  void (*on_node_attribute)(void *node, string key, string value, void *arg);
  void (*on_node_child)(void *node, void *child, void *arg);
  void (*on_node_content)(void *node, string content, void *arg);
  
  void (*on_error)(const char *message, u64 pos, void *arg);
  void *arg;
}Html_Parse_Events;

HTML_PARSER_DEF bool html_parse(const char *cstr, u64 cstr_len, const Html_Parse_Events *events);

#ifdef HTML_PARSER_IMPLEMENTATION

static char *html_node_singletons[] = {"area", "base", "br", "col", "embed", "hr", "img", "input", "keygen", "link", "meta", "param", "source", "track", "wbr"};
static char *html_node_maybe_singletons[] = {"li"};

#define __html_parse_expect_doctype(_type) do{	\
    if(!tokenizer_next(t, &token)) {		\
      return false;				\
    }						\
    if(token.type != _type) {			\
      return false;				\
    }						\
  }while(0)

HTML_PARSER_DEF bool html_parse_expect_doctype_html(Tokenizer *t, const Html_Parse_Events *events) {
  static const char *err = "<!doctype html>";
  
  Token token;
  __html_parse_expect_doctype(TOKENTYPE_ANGLE_OPEN);
  if(!tokenizer_next(t, &token)) {
    return false;
  }
  if(token.type != TOKENTYPE_DOCTYPE && token.type != TOKENTYPE_DOCTYPE2) {
    return false;
  }
  __html_parse_expect_doctype(TOKENTYPE_WORD);
  if(!string_eq(token.content, STRING("html"))) {
    return false;
  }
  __html_parse_expect_doctype(TOKENTYPE_ANGLE_CLOSE);
  return true;
}

#define __html_parse_expect_token(_type) do{					\
    if(!tokenizer_next(t, &token)) {					\
      if(events->on_error) {						\
	events->on_error(tokentype_name(_type), t->last, events->arg);	\
      }									\
      return false;							\
    }									\
    if(token.type != _type) {						\
      if(events->on_error) {						\
	events->on_error(tokentype_name(_type), t->last, events->arg);	\
      }									\
      return false;							\
    }									\
}while(0)

#define __html_parse_peek_token(_type) do{					\
    if(!tokenizer_peek(t, &token)) {					\
      if(events->on_error) {						\
	events->on_error(tokentype_name(_type), t->last, events->arg);	\
      }\
      return false;							\
    }									\
}while(0)

HTML_PARSER_DEF bool html_parse_expect_word_concatination(Tokenizer *t, const Html_Parse_Events *events, string *result) {
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

HTML_PARSER_DEF bool html_parse_node_open(Tokenizer *t, const Html_Parse_Events *events, bool *closed, void **root, string *root_name) {
  assert(closed);
  *closed = false;

  Token token;
  __html_parse_expect_token(TOKENTYPE_ANGLE_OPEN);
  if(!html_parse_expect_word_concatination(t, events, root_name)) {
    return false;
  }
  if(root) {
    if(events->on_node) {
      if(!events->on_node(*root_name, events->arg, root)) {
	return false;
      }
    }
  }
  
  __html_parse_peek_token(TOKENTYPE_WORD);
  while(token.type == TOKENTYPE_WORD) {
    tokenizer_next(t, &token); // attribute
    string key = token.content;

    while(token.type != TOKENTYPE_EQUALS && token.type != TOKENTYPE_ANGLE_CLOSE) {
      tokenizer_next(t, &token);
      if(token.content.data != key.data) {
	u64 diff = token.content.data - key.data - key.len;
	key = string_from(key.data, key.len + diff + token.content.len);
      }
      //__html_parse_peek_token(TOKENTYPE_WORD);
    }
    key = string_from(key.data, key.len - 1);

    if(token.type == TOKENTYPE_ANGLE_CLOSE) {
	if(root) {
	  if(events->on_node_attribute) {
	    events->on_node_attribute(*root, key, string_from_cstr(""), events->arg);
	  }
	}
	t->pos = t->last;
	continue;
    }

    string value = {0};
    __html_parse_peek_token(TOKENTYPE_EQUALS);
    if(token.type == TOKENTYPE_DOUBLE_QUOTATION ||
       token.type == TOKENTYPE_QUOTATION) {
      
      Tokentype quotation = TOKENTYPE_DOUBLE_QUOTATION;
      if(token.type == TOKENTYPE_DOUBLE_QUOTATION) {
	__html_parse_expect_token(TOKENTYPE_DOUBLE_QUOTATION);
      } else {
	__html_parse_expect_token(TOKENTYPE_QUOTATION);
	quotation = TOKENTYPE_QUOTATION;
      }

      __html_parse_peek_token(quotation);
      value = token.content;
      while(token.type != quotation) {
	tokenizer_next(t, &token);
	if(token.content.data != value.data) {
	  u64 diff = token.content.data - value.data - value.len;
	  value = string_from(value.data, value.len + diff + token.content.len);	
	}
	__html_parse_peek_token(quotation);
      }

      __html_parse_expect_token(quotation);
    } else if(token.type != TOKENTYPE_ANGLE_CLOSE) {
      __html_parse_expect_token(token.type);
      const char *from = token.content.data + token.content.len;
      __html_parse_peek_token(TOKENTYPE_ANGLE_CLOSE);
      value = token.content;
      while(token.content.data == from &&
	    token.type != TOKENTYPE_ANGLE_CLOSE) {
	tokenizer_next(t, &token);
	from = token.content.data + token.content.len;
	if(token.content.data != value.data) {
	  u64 diff = token.content.data - value.data - value.len;
	  value = string_from(value.data, value.len + diff + token.content.len);
	}
	__html_parse_peek_token(TOKENTYPE_ANGLE_CLOSE);
      }
    }
    
    __html_parse_peek_token(TOKENTYPE_ANGLE_CLOSE);
    
    if(root) {
      if(events->on_node_attribute) {
	events->on_node_attribute(*root, key, value, events->arg);
      }
    }
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

HTML_PARSER_DEF bool html_parse_expect_node_close(Tokenizer *t, const Html_Parse_Events *events, string root_name) {
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
      if(events->on_error) {						\
	events->on_error(tokentype_name(_type), t->last, events->arg);	\
      }									\
      return false;							\
    }									\
  }while(0)

HTML_PARSER_DEF bool html_parse_peek_n_word_concatenation(Tokenizer *t, int n, string *result) {
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

HTML_PARSER_DEF bool html_parse_node(Tokenizer *t, const Html_Parse_Events *events, void **node) {
  
  bool closed;
  void *root;
  string root_name;
  
  if(!html_parse_node_open(t, events, &closed, &root, &root_name)) {
    return false;
  }
  if(closed) {
    *node = root;
    return true;
  }

  u32 old = t->pos;
  
  bool potential_maybe_singleton = false;
  u32 maybe_singletons_len = sizeof(html_node_maybe_singletons)/sizeof(html_node_maybe_singletons[0]);  
  for(u32 i=0;i<maybe_singletons_len;i++) {
      string singleton = string_from_cstr(html_node_maybe_singletons[i]);
      if(string_eq(root_name, singleton)) {
	  potential_maybe_singleton = true;
	  break;
      }
  }

  u32 singletons_len = sizeof(html_node_singletons)/sizeof(html_node_singletons[0]);
  for(u32 i=0;!potential_maybe_singleton && i<singletons_len;i++) {
      string singleton = string_from_cstr(html_node_singletons[i]);
      if(string_eq(root_name, singleton)) {
	*node = root;
	return true;
      }
  }


  bool isScript = string_eq(root_name, STRING("script"));

  Token token;
  string current = {0};

  while(true) {    
    __html_parse_peek_n(1, TOKENTYPE_ANGLE_OPEN);
    if(token.type == TOKENTYPE_ANGLE_OPEN) {
      __html_parse_peek_n(2, TOKENTYPE_SLASH);
      if(token.type == TOKENTYPE_SLASH) {
	string content;
	if(!html_parse_peek_n_word_concatenation(t, 3, &content)) {
	  if(events->on_error) {
	    events->on_error(tokentype_name(TOKENTYPE_WORD), t->last, events->arg);	    
	  }
	  return false;
	}
	if(string_eq(string_trim(root_name), string_trim(content))) {
	  if(events->on_node_content) {
	    events->on_node_content(root, current, events->arg);	    
	  }
	  html_parse_expect_node_close(t, events, root_name);
	  *node = root;
	  return true;
	} else {
	  
	  if(!isScript) {
	      if(potential_maybe_singleton) {
		  u32 current_pos = t->pos;
		  t->pos = old;
		  bool child_closed;
		  string child_name;
		  //implement a peek version
		  if(html_parse_node_open(t, events, &child_closed, NULL, &child_name)) {
		      if(string_eq(child_name, root_name)) {
			  t->pos = current_pos;
			  *node = root;
			  return true;
		      }
		  }
		  t->pos = current_pos;
	      }

	      if(string_eq(root_name, STRING("li")) && string_eq(content, STRING("ul")) ) {
		*node = root;
		return true;
	      }
	      
	      fprintf(stderr, "ERORR: Tag not closed "String_Fmt"\n", String_Arg(root_name));
	      fprintf(stderr, "ERORR: Instead closed "String_Fmt"\n", String_Arg(content));
	      if(events->on_error) {
		events->on_error(tokentype_name(TOKENTYPE_WORD), t->last, events->arg);		
	      }
	    return false;
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
	    void *child;
	    if(!html_parse_node(t, events, &child)) {
	      return false;
	    }
	    if(events->on_node_child) {
	      events->on_node_child(root, child, events->arg);	      
	    }
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
  return false;
}

HTML_PARSER_DEF bool html_parse(const char *cstr, u64 cstr_len, const Html_Parse_Events *events) {

    Tokenizer tokenizer = {cstr, cstr_len, 0, 0};
    void *node;
    if(!events) {
	Html_Parse_Events no_events = {0};      
	if(!html_parse_expect_doctype_html(&tokenizer, &no_events)) {
	    tokenizer.pos = 0;
	    tokenizer.last = 0;
	    printf("could not parse doctype\n");
	}
	void *node;
	if(!html_parse_node(&tokenizer, &no_events, &node)) {
	    return false;
	}

    } else {
	if(!html_parse_expect_doctype_html(&tokenizer, events)) {
	    tokenizer.pos = 0;
	    tokenizer.last = 0;
	    printf("could not parse doctype\n");
	}
	if(!html_parse_node(&tokenizer, events, &node)) {
	    return false;
	}      
    }
  
  
  return true;
}

#endif //HTML_PARSER_IMPLEMENTATION

#endif //HTML_PARSER_H_H

