#include <stdio.h>

#define HTML_PARSER_IMPLEMENTATION
#include "../libs/html_parser.h"

#define HTTP_IMPLEMENTATION
#include "../libs/http.h"

#define NODE_CAP (1024)
#define CHILDREN_CAP 1024
#define ATTRIBUTES_CAP 1024

typedef struct{
  string key;
  string value;
}Attribute;

struct Node {
  string name;
  string content;

  Attribute attributes[ATTRIBUTES_CAP];
  u32 attributes_count;
  
  struct Node *children[CHILDREN_CAP];
  u32 children_count;
};

typedef struct Node Node;

Node node_pool[NODE_CAP] = {0};
u32 node_count = 0;

void node_print(Node *node, u32 depth) {
  static const char *indent = "-";
  assert(node);

  if(node->children_count) {
    for(u32 i=0;i<depth;i++) fputs(indent, stdout);
    printf("<"String_Fmt, String_Arg(node->name));
    for(u32 i=0;i<node->attributes_count;i++) {
      printf(" "String_Fmt"=\""String_Fmt"\"",
	     String_Arg(node->attributes[i].key),
	     String_Arg(node->attributes[i].value));      
    }
    printf(">\n");
    for(u32 i=0;i<node->children_count;i++) {
      node_print(node->children[i], depth + 1);
    }

    if(node->content.len) {
      for(u32 i=0;i<depth;i++) fputs(indent, stdout);
      printf(String_Fmt"\n", String_Arg(node->content));
    }
    
    for(u32 i=0;i<depth;i++) fputs(indent, stdout);
    printf("</"String_Fmt">\n", String_Arg(node->name));
  } else {
    for(u32 i=0;i<depth;i++) fputs(indent, stdout);
    printf("<"String_Fmt, String_Arg(node->name));
    for(u32 i=0;i<node->attributes_count;i++) {
      printf(" "String_Fmt"=\""String_Fmt"\"",
	     String_Arg(node->attributes[i].key),
	     String_Arg(node->attributes[i].value));      
    }
    printf(">"String_Fmt"</"String_Fmt">\n",
	   String_Arg(node->content),
	   String_Arg(node->name));  
  }
}

Node *node_query(Node *node,
		 const char *_name,
		 const char *_key,
		 const char *_value) {
  assert(node);

  string name = string_from_cstr(_name);
  string key = string_from_cstr(_key);
  string value = string_from_cstr(_value);
  
  for(u32 i=0;i<node->children_count;i++) {
    Node *child = node->children[i];
    if(string_eq(child->name, name)) {
      for(u32 j=0;j<child->attributes_count;j++) {
	Attribute *attribute = &child->attributes[j];
	if(string_eq(attribute->key, key)
	   && string_eq(attribute->value, value)) {
	  return child;
	}
      }
    }
  }
  return NULL;
}

Node *node_alloc(string name) {
  assert(node_count < NODE_CAP);
  Node *node = &node_pool[node_count++];
  node->name = name;
  node->content = (string) {0};
  node->children_count = 0;
  node->attributes_count = 0;
  return node;
}

void node_append(Node *node, Node *child) {
  assert(node);
  assert(node->children_count < CHILDREN_CAP);
  node->children[node->children_count++] = child;
}

void node_add(Node *node, string key, string value) {
  assert(node);
  assert(node->attributes_count < ATTRIBUTES_CAP);
  node->attributes[node->attributes_count++] = (Attribute) {key, value};
}

///////////////////////////////////////////////////////////////////////////////////////

void google_search(const char *keyword, u32 offset, String_Buffer *sb) {
  char buffer[2][1024];
  size_t buffer_size;
  if(!http_encodeURI(keyword, strlen(keyword),
		     buffer[0], 1023,
		     &buffer_size)) {
    panic("http_encodeURI");
  }
  buffer[0][buffer_size] = '\0';
	
  if(snprintf(buffer[1], 1024, "https://www.google.com/search?q=%s&ucbcb=1&start=%u",
	      buffer[0], offset) >= 1023) {
    panic("buffer overflow");
  }
    
  HttpHeader header;
  if(!http_get(buffer[1], string_buffer_callback, sb, &header, NULL)) {
    panic("http_get");
  }

  int response_code = http_header_response_code(&header);
  if(response_code != 200) {
    panic("Not ok: %d\n", response_code);
  }
}

u32 calc_line(const char *text, u32 text_len, u64 pos) {
  assert(text_len > pos);
  
  u32 line = 1;
  for(u64 i=0;i<pos;i++) {
    if(text[i]=='\n') {
      line++;
    }
  }
  
  return line;
}

u32 calc_char(const char *text, u32 text_len, u64 pos) {
  assert(text_len > pos);
  
  u32 c = 0;
  for(u64 i=0;i<pos;i++) {
    c++;
    if(text[i]=='\n') {
      c = 0;      
    }
  }
  
  return c;
}

void on_error(const char *message, u64 pos, void *arg) {
  String_Buffer *sb = (String_Buffer *) arg;
  printf("pretty.txt:%d:%d:ERROR: Expected '%s'\n",
	 calc_line(sb->data, sb->len, pos),
	 calc_char(sb->data, sb->len, pos),
	 message);
}

void *on_node(string name, void *arg) {
  (void) arg;
  Node *node = node_alloc(name);
  return node;
}

void on_node_attribute(void *node, string key, string value, void *arg) {
  (void) arg;
  node_add((Node *) node, key, value);
}

void on_node_child(void *node, void *child, void *arg) {
  (void) arg;
  node_append((Node *) node, (Node *) child);
}

void on_node_content(void *node, string content, void *arg) {
  (void) arg;
  Node *_node = (Node *) node;
  _node->content = content;
}

bool div_has_result(Node *node, string *title, string *url, string *desc) {
  assert(node);
  
  string div_string = STRING("div");
  string a_string = STRING("a");
  /* string h3_string = STRING("h3"); */

  if(!string_eq(node->name, div_string)) {
    return false;
  }

  if(!node->children_count) {
    return false;
  }
  Node *div = node->children[0];

  if(!div->children_count) {
    return false;
  }

  //DESC
  {
    if(div->children_count < 2) {
      return false;
    }
    Node *desc_div =  div->children[1];

    if(!desc_div->children_count) {
      return false;
    }
    desc_div = desc_div->children[0];

    if(!desc_div->children_count) {
      return false;
    }
    desc_div = desc_div->children[0];
    if(!desc_div->children_count) {
      return false;
    }
    desc_div = desc_div->children[0];
    
    if(!desc_div->children_count) {
      return false;
    }
    desc_div = desc_div->children[0];

    if(!desc_div->children_count) {
      return false;
    }
    desc_div = desc_div->children[0];

    if(desc) {
      *desc = desc_div->content;
    }
  }
  
  div = div->children[0];

  if(!div->children_count) {
    return false;
  }
  Node *a = div->children[0];

  string raw = {0};
  if(url) {
    raw = a->attributes[0].value;
  }
  
  if(!string_eq(a->name, a_string)) {
    return false;
  }

  if(!a->children_count) {
    return false;
  }
  div = a->children[0];

  if(!div->children_count) {
    return false;
  }
  div = div->children[0];

  if(!div->children_count) {
    return false;
  }
  Node *h3 = div->children[0];

  if(!h3->children_count) {
    return false;
  }
  div = h3->children[0];
  
  if(title) {
    *title = div->content;
  }

  if(url) {
    string _url = string_chop_by_delim(&raw, '&');
    string_chop_left(&_url, 7);

#define GOOGLE_URL_BUFFER_CAP 1024
    static char buffer[GOOGLE_URL_BUFFER_CAP];
    size_t buffer_size;
    if(!http_decodeURI(_url.data, _url.len, buffer, GOOGLE_URL_BUFFER_CAP, &buffer_size)) {
      warn("http_decodeURI");
      *url = _url;
    } else {
      *url = string_from(buffer, buffer_size);      
    }
  }

  return true;
}

int main(s32 argc, char **argv) {
  
  // Parsing command line arguments
  ////////////////////////////////////////////////////////////

  String_Buffer temp = {0};

  if(argc < 2) {
    fprintf(stderr, "ERROR: Please provice atleast one argument\n");
    fprintf(stderr, "USAGE: ./google <keyword> ... \n");
    exit(1);
  }

  for(s32 i=1;i<argc;i++) {    
    string_buffer_append(&temp, argv[i], strlen(argv[i]));
    if(i != argc - 1) {
      string_buffer_append(&temp, " ", 1);
    }
  }
  string_buffer_append(&temp, "\0", 1);

  // Requsting and Parsing html
  ////////////////////////////////////////////////////////////

  String_Buffer sb = {0};
  google_search(temp.data, 0, &sb);
  
  Html_Parse_Events events = {0};  
  events.on_node = on_node;
  events.on_node_attribute = on_node_attribute;
  events.on_node_child = on_node_child;
  events.on_node_content = on_node_content;
  events.on_error = on_error;
  events.arg = &sb;
  
  if(!html_parse(sb.data, sb.len , &events)) {
    panic("Failed to parse googles-response");
  }

  // Dislaying results
  //////////////////////////////////////////////////////////

  Node *body = node_pool[0].children[1];
  Node *main = node_query(body, "div", "id", "main");

  if(!main) {
    panic("Can not find main div");
  }
  
  string title, url, desc;  
  for(u32 i=0;i<main->children_count;i++) {
    Node *div = main->children[i];
    if(!div_has_result(div, &title, &url, &desc)) {
      continue;
    }

    printf("==========================================\n");
    printf(String_Fmt"\n\n", String_Arg(url));
    printf(String_Fmt"\n\n", String_Arg(title));
    printf(String_Fmt"\n", String_Arg(desc));
  }

  //string_buffer_free(&sb);
  //string_buffer_free(&temp);
  return 0;
}
