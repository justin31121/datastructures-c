#ifndef TOKENIZER_H
#define TOKENIZER_H

#ifdef TOKENIZER_IMPLEMENTATION
#define STRING_IMPLEMENTATION
#define UTIL_IMPLEMENTATION
#endif //TOKENIZER_IMPLEMENTATION

#include "string.h"
#include "util.h"

typedef enum{
	     TOKENTYPE_ANGLE_OPEN,
	     TOKENTYPE_ANGLE_CLOSE,
	     TOKENTYPE_DOUBLE_QUOTATION,
	     TOKENTYPE_EXCLAMATION,
	     TOKENTYPE_EQUALS,
	     TOKENTYPE_SLASH,
	     TOKENTYPE_PAREN_OPEN,
	     TOKENTYPE_PAREN_CLOSE,
	     TOKENTYPE_CURLY_OPEN,
	     TOKENTYPE_CURLY_CLOSE,
	     TOKENTYPE_SEMICOLON,
	     TOKENTYPE_AND_SIGN,
	     TOKENTYPE_OR_SIGN,
	     TOKENTYPE_COLON,
	     TOKENTYPE_QUOTATION,
	     TOKENTYPE_BRACKET_OPEN,
	     TOKENTYPE_BRACKET_CLOSE,
	     TOKENTYPE_COMMA,
	     TOKENTYPE_DOT,
	     TOKENTYPE_UNDERSCORE,
	     TOKENTYPE_MINUS,
	     TOKENTYPE_PLUS,
	     TOKENTYPE_QUESTION,
	     TOKENTYPE_ZIRKUMFLEX,
	     TOKENTYPE_BACK_SLASH,
	     TOKENTYPE_DOLLAR,
	     TOKENTYPE_STAR,
	     TOKENTYPE_HASH,
	     TOKENTYPE_AT,
	     TOKENTYPE_PERCENT,
	     TOKENTYPE_BACK_TICK,
	     TOKENTYPE_TILDE,

	     TOKENTYPE_DOCTYPE,

	     TOKENTYPE_WORD,
}Tokentype;

const char *tokentype_name(Tokentype t);

static char symbols[] =
  { '<', '>', '\"', '!', '=', '/', '(', ')', '{', '}', ';', '&', '|', ':', '\'', '[', ']', ',', '.', '_', '-', '+', '?', '^', '\\', '$', '*', '#', '@', '%', '`', '~'};

static char *strings[] = { "!doctype" };

typedef struct{
  Tokentype type;
  string content;
}Token;

#define Token_Fmt "%s '"String_Fmt"'"
#define Token_Arg(token) tokentype_name((token).type), String_Arg((token).content)

typedef struct{
  const char *text;
  u32 len;
  u32 pos;
  u32 last;
}Tokenizer;

bool tokenizer_init(Tokenizer *t, const char *text);
bool tokenizer_next(Tokenizer *t, Token *token);
bool tokenizer_peek(Tokenizer *t, Token *token);
bool tokenizer_peek_n(Tokenizer *t, Token *token, u32 n);
void tokenizer_expect(Tokenizer *t, Token *token, Tokentype type);
void tokenizer_expect_content(Tokenizer *t, Token *token, Tokentype type, string content);

#ifdef TOKENIZER_IMPLEMENTATION

const char *tokentype_name(Tokentype t) {
  switch(t) {
  case TOKENTYPE_ANGLE_OPEN: return "ANGLE_OPEN";
  case TOKENTYPE_ANGLE_CLOSE: return "ANGLE_CLOSE";
  case TOKENTYPE_DOUBLE_QUOTATION: return "DOUBLE_QUOTATION";
  case TOKENTYPE_EXCLAMATION: return "EXCLAMATION";
  case TOKENTYPE_EQUALS: return "EQUALS";
  case TOKENTYPE_SLASH: return "SLASH";
  case TOKENTYPE_PAREN_OPEN: return "PAREN_OPEN";
  case TOKENTYPE_PAREN_CLOSE: return "PAREN_CLOSE";
  case TOKENTYPE_CURLY_OPEN: return "CURLY_OPEN";
  case TOKENTYPE_CURLY_CLOSE: return "CURLY_CLOSE";
  case TOKENTYPE_SEMICOLON: return "SEMICOLON";
  case TOKENTYPE_AND_SIGN: return "AND_SIGN";
  case TOKENTYPE_OR_SIGN: return "OR_SIGN";
  case TOKENTYPE_COLON: return "COLON";
  case TOKENTYPE_QUOTATION: return "QUOTATION";
  case TOKENTYPE_BRACKET_OPEN: return "BRACKET_OPEN";
  case TOKENTYPE_BRACKET_CLOSE: return "BRACKET_CLOSE";
  case TOKENTYPE_COMMA: return "COMMA";
  case TOKENTYPE_DOT: return "DOT";
  case TOKENTYPE_UNDERSCORE: return "UNDERSCORE";
  case TOKENTYPE_MINUS: return "MINUS";
  case TOKENTYPE_PLUS: return "PLUS";
  case TOKENTYPE_QUESTION: return "QUESTION";
  case TOKENTYPE_ZIRKUMFLEX: return "ZIRKUMFLEX";
  case TOKENTYPE_BACK_SLASH: return "BACK_SLASH";
  case TOKENTYPE_DOLLAR: return "DOLLAR";
  case TOKENTYPE_STAR: return "STAR";
  case TOKENTYPE_HASH: return "HASH";
  case TOKENTYPE_AT: return "AT";
  case TOKENTYPE_PERCENT: return "PERCENT";
  case TOKENTYPE_BACK_TICK: return "BACK_TICK";
  case TOKENTYPE_TILDE: return "TILDE";

  case TOKENTYPE_DOCTYPE: return "DOCTYPE";

  case TOKENTYPE_WORD: return "WORD";
    
  default: return "UNKNOWN TOKENTYPE";
  }
}

bool tokenizer_init(Tokenizer *t, const char *text) {
  if(!t) {
    return false;
  }
  
  t->text = text;
  t->len = strlen(text);
  t->pos = 0;
  
  return true;
}

bool tokenizer_init2(Tokenizer *t, const char *text, size_t text_len) {
  if(!t) {
    return false;
  }
  
  t->text = text;
  t->len = text_len;
  t->pos = 0;
  
  return true;
}

static void tokenizer_trim_left(Tokenizer *t) {
  while(t->pos<t->len && isspace(t->text[t->pos])) {
    t->pos++;
  }
}

bool tokenizer_next(Tokenizer *t, Token *token) {
  t->last = t->pos;
  
  if(!t || !token) {
    return false;
  }
  
  tokenizer_trim_left(t);
  if(t->pos >= t->len) {
    return false;
  }

  u32 symbols_len = sizeof(symbols)/sizeof(symbols[0]);
  u32 strings_len = sizeof(strings)/sizeof(strings[0]);

  //STRINGS
  for(u32 i=0;i<strings_len;i++) {    
    char *_string = strings[i];
    u32 _string_len = strlen(_string);
    
    if(_string_len > (t->len - t->pos)) { // TEST that
      continue;
    }

    bool ok = true;
    for(u32 j=0;j<_string_len;j++) {
      if(t->text[t->pos + j] != _string[j]) {
	ok = false;
	break;
      }
    }

    if(ok) {
      token->type = i + symbols_len;
      token->content = string_from(t->text + t->pos, _string_len);
      t->pos += _string_len;
      return true;
    }
  }
  
  //SYMBOLS
  for(u32 i=0;i<symbols_len;i++) {
    if(symbols[i] == t->text[t->pos]) {
      token->type = i;
      token->content = string_from(t->text + t->pos, 1);
      t->pos += 1;
      return true;
    }
  }

  //WORD
  u32 i=t->pos;
  for(;i<t->len;i++) {
    char c = t->text[i];
    s32 u = (s32) (u8) c;
    
    if(!isalpha(c) && !isdigit(c) && u <= 128) {
      break;
    }
  }

  if(i - t->pos >  0) {
    token->type = TOKENTYPE_WORD;
    token->content = string_from(t->text + t->pos, i - t->pos);
    t->pos = i;
    return true;
  }

  return false;
}

bool tokenizer_peek_n(Tokenizer *t, Token *token, u32 n) {
  if(!t) {
    return false;
  }
  
  u32 pos = t->pos;
  for(u32 i=0;i<n;i++) {
    if(!tokenizer_next(t, token)) {
      return false;
    }
  }

  t->pos = pos;
  return true;
}

bool tokenizer_peek(Tokenizer *t, Token *token) {
  return tokenizer_peek_n(t, token, 1);
}

void tokenizer_expect(Tokenizer *t, Token *_token, Tokentype type) {
  if(!t) {
    panic("Tokenizer is null");
  }

  Token token;
  if(!tokenizer_next(t, &token)) {
    panic("Expected Tokentype: %s. But got eof", tokentype_name(type));
  }

  if(token.type != type) {
    panic("Expected Tokentype: %s. But got "Token_Fmt" (%.*s ...)",
	  tokentype_name(type),
	  Token_Arg(token),
	  (int) 5, token.content.data);
  }

  if(_token) {
    *_token = token;
  }
}

void tokenizer_expect_content(Tokenizer *t, Token *_token, Tokentype type, string content) {
  Token token;
  tokenizer_expect(t, &token, type);  
  
  if(!string_eq(token.content, content)) {
    Token expected = {type, content};
    panic("Expected Token: "Token_Fmt". But got "Token_Fmt,
	  Token_Arg(expected),
	  Token_Arg(token));
  }

  if(_token) {
    *_token = token;
  }
}

#endif //TOKENIZER_IMPLEMENTATION

#endif //TOKENIZER_H
