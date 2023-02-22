#ifndef JSON_H_H
#define JSON_H_H

#ifdef JSON_IMPLEMENTATION

#define HASHTABLE_IMPLEMENTATION
#define ARRAY_IMPLEMENTATION
#define PARSE_IMPLEMENTATION

#endif

#ifdef JSON_NOTNULL

#define HT_NOTNULL
#define ARRAY_NOTNULL

#endif

#include "hashtable.h"
#include "array.h"
#include "parse.h"

//----------- JSON -------------

char *json_create_cstr(const char* cstr);
char *json_create_cstr_from_parts(const char* cstr, size_t len);
char *json_create_cstr_from_parts_escape_chars(const char* cstr, size_t len, size_t escape_chars_count);

typedef enum {
  JSON_NULL,
  JSON_BOOL,
  JSON_INT,
  JSON_FLOAT,
  JSON_DOUBLE,
  JSON_STRING,
  JSON_OBJECT,
  JSON_ARRAY
}JsonType;

bool json_type_to_cstr(char *buffer, size_t buffer_size, int type);
const char* json_type_name(int type);

typedef struct{
  JsonType type;
  union {
    int intVal;
    float floatVal;
    double doubleVal;
    bool boolVal;
    char *stringVal;
    Ht *objVal;
    Arr *arrVal;
  };
}Json;

Json json_init_object();
Json json_init_array();

//BEGIN JSON_OBJECT
static inline void json_put_null(Json obj, const char *key);
static inline void json_put_bool(Json obj, const char *key, bool value);
static inline void json_put_int(Json obj, const char *key, int value);
static inline void json_put_float(Json obj, const char *key, float value);
static inline void json_put_double(Json obj, const char *key, double value);
static inline void json_put_string(Json obj, const char *key, const char* value);
static inline void json_put_object(Json obj, const char *key, Json *value);
static inline void json_put_array(Json obj, const char *key, Json *value);

static inline void json_remove(Json obj, const char *key);

static inline Json json_get(Json obj, const char* key);
static inline bool json_has(Json obj, const char* key);

static inline bool json_get_bool(Json obj, const char *key);
static inline int json_get_int(Json obj, const char *key);
static inline float json_get_float(Json obj, const char *key);
static inline double json_get_double(Json obj, const char *key);
static inline char* json_get_string(Json obj, const char *key);
static inline Json json_get_object(Json obj, const char *key);
static inline Json json_get_array(Json obj, const char *key);
//END JSON_OBJECT

//BEGIN JSON_ARRAY
static inline void json_push_null(Json arr);
static inline void json_push_bool(Json arr, bool value);
static inline void json_push_int(Json arr, int value);
static inline void json_push_float(Json arr, float value);
static inline void json_push_double(Json arr, double value);
static inline void json_push_string(Json arr, const char* value);
static inline void json_push_object(Json arr, Json *value);
static inline void json_push_array(Json arr, Json *value);

static inline void json_pop(Json arr);

static inline Json json_opt(Json arr, int i);
static inline bool json_opt_bool(Json obj, int p);
static inline int json_opt_int(Json obj, int p);
static inline float json_opt_float(Json obj, int p);
static inline double json_opt_double(Json obj, int p);
static inline char *json_opt_string(Json obj, int p);
static inline Json json_opt_object(Json obj, int p);
static inline Json json_opt_array(Json obj, int p);
//END JSON_ARRAY

void json_fprint(FILE *f, Json json);
size_t json_write(Json json,size_t (*write_callback)(void *, size_t size, void *userdata), void *userdata);
char *json_to_cstr(Json json);

int json_size(Json json);

void json_free(Json json);
void json_free_all(Json json);

//----------- JSON_PARSE -------------

bool parseJson(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonNull(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonBool(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonInt(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonFloat(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonString(const char *buffer, size_t buffer_size, Json *json, size_t *m);

bool parsePair(const char *buffer, size_t buffer_size, char **key, Json *value, size_t *m);
bool parseJsonArray(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonObject(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJson(const char *buffer, size_t buffer_size, Json *json, size_t *m);

bool json_parse(const char* buffer, Json *json);
bool json_parse_len(const char* buffer, size_t buffer_size, Json *json);

//----------- XML_PARSE -------------------

bool parseXmlString(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseXmlInt(const char *buffer, size_t buffer_size, Json *value, size_t *m);
bool parseXmlValue(const char *buffer, size_t buffer_size, Json *value, size_t *m);
bool parseXmlPair(const char *buffer, size_t buffer_size, char **key, Json *value, size_t *m);
bool parseXmlTag(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseXmlSelfClosingTag(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseXml(const char* buffer, size_t buffer_size, Json *json, size_t *m);

bool xml_parse(const char *source, Json *json);
bool xml_parse_len(const char *source, size_t source_len, Json *json);

#ifdef JSON_IMPLEMENTATION

#ifdef JSON_ASSERT

#define ASSERT_TYPE(type, goal) do{					\
  if(type!=goal) {							\
      char buffer1[16];							\
      json_type_to_cstr(buffer1, 16, goal);				\
      char buffer2[16];							\
      json_type_to_cstr(buffer2, 16, type);				\
      fprintf(stderr, "ERROR:json.h:%s:%d:Expected type: '%s'. But got '%s'\n", __func__, __LINE__, buffer1, buffer2); \
      exit(1);								\
    }									\
  }while(0)

#else

#define ASSERT_TYPE(type, goal)

#endif

char *json_create_cstr_from_parts(const char* cstr, size_t size) {
  char *json_cstr = (char *) malloc(size + 1);
  if(!json_cstr) {
    fprintf(stderr, "ERROR: Can not allocate enough memory: json_create_string\n");
    exit(1);
  }  
  memcpy(json_cstr, cstr, size);
  json_cstr[size] = 0;
  
  return json_cstr;
}

char *json_create_cstr_from_parts_escape_chars(const char* cstr, size_t size, size_t escape_chars_count) {
  if(escape_chars_count >= size) {
    fprintf(stderr, "ERROR: Something went wrong, when parsing escape chars\n");
    exit(1);
  }  
  size_t len = size - escape_chars_count;

  char *json_cstr = (char *) malloc(size + 1 - escape_chars_count);
  if(!json_cstr) {
    fprintf(stderr, "ERROR: Can not allocate enough memory: json_create_string\n");
    exit(1);
  }  
 size_t j = 0;
  for(size_t i=0;i<size;i++) {
  	char c = cstr[i];
  	if(cstr[i] == '\\' && i<size - 1 && cstr[i+1] != 'u') {
  		char d = cstr[i+1];
  		char e = d;
  		switch(d) {
  			case '\"':
  				e = '\"';
  				break;
  			case '\\':
  				e = '\\';
  				break;
  			case '/':
  				e = '/';
  				break;
  			case 'b':
  				e = '\b';
  				break;
  			case 'f':
  				e = '\f';
  				break;
  			case 'n':
  				e = '\n';
  				break;
  			case 'r':
  				e = '\r';
  				break;
  			case 't':
  				e = '\t';
  				break;  			
  			default:
  				fprintf(stderr, "ERROR: Unknown escape code: '%c' :json_create_string\n", d);
    			exit(1);
  				break;
  		}
  		json_cstr[j++] = e;
  		i++;
  	} else {
  		json_cstr[j++] = c;	
  	}  	
  } 
  json_cstr[len] = 0;
  
  return json_cstr;	
}

char *json_create_string(const char* cstr) {
  size_t size = 0;
  while(cstr[size++]!=0);
  size--;
  return json_create_cstr_from_parts(cstr, size);
}

bool json_type_to_cstr(char *buffer, size_t buffer_size, int type) {
  char* source = NULL;
  
  switch(type) {
  case JSON_NULL:
    source = "JSON_NULL";
    break;
  case JSON_BOOL:
    source = "JSON_BOOL";
    break;
  case JSON_INT:
    source = "JSON_INT";
    break;
  case JSON_FLOAT:
    source = "JSON_FLOAT";
    break;
  case JSON_DOUBLE:
    source = "JSON_DOUBLE";
    break;
  case JSON_STRING:
    source = "JSON_STRING";
    break;
  case JSON_OBJECT:
    source = "JSON_OBJECT";
    break;
  case JSON_ARRAY:
    source = "JSON_ARRAY";
    break;
  default:
    return false;
  }

  size_t len = strlen(source);
  if(len+1>=buffer_size) return false;
  memcpy(buffer, source, len+1);
  return true;
}

const char* json_type_name(int type) {
  switch(type) {
  case JSON_NULL:
    return "JSON_NULL";
  case JSON_BOOL:
    return "JSON_BOOL";
  case JSON_INT:
    return "JSON_INT";
  case JSON_FLOAT:
    return "JSON_FLOAT";
  case JSON_DOUBLE:
    return "JSON_DOUBLE";
  case JSON_STRING:
    return "JSON_STRING";
  case JSON_OBJECT:
    return "JSON_OBJECT";
  case JSON_ARRAY:
    return "JSON_ARRAY";
  default:
    return NULL;
  }  
}

static inline void json_put_null(Json obj, const char *key) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_NULL;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

static inline void json_put_bool(Json obj, const char *key, bool value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_BOOL;
  json.boolVal = value;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

static inline void json_put_int(Json obj, const char *key, int value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_INT;
  json.intVal = value;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

static inline void json_put_float(Json obj, const char *key, float value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_FLOAT;
  json.floatVal = value;
  ht_insert(obj.objVal, key, &json, sizeof(Json));  
}

static inline void json_put_double(Json obj, const char *key, double value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_DOUBLE;
  json.doubleVal = value;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

static inline void json_put_string(Json obj, const char *key, const char* value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_STRING;
  json.stringVal = json_create_string(value);
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

static inline void json_put_object(Json obj, const char *key, Json *value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  ASSERT_TYPE(value->type, JSON_OBJECT);
  Json json;
  json.type = JSON_OBJECT;
  json.objVal = value->objVal;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

static inline void json_put_array(Json obj, const char *key, Json *value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  ASSERT_TYPE(value->type, JSON_ARRAY);
  Json json;
  json.type = JSON_ARRAY;
  json.arrVal = value->arrVal;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

static inline void json_remove(Json obj, const char *key) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  ht_remove(obj.objVal, key);
}

static inline Json json_get(Json obj, const char* key) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  void *ptr = ht_get(obj.objVal, key);
  
  if(!ptr) {
    fprintf(stderr, "ERROR:json.h:%d:Json does not have value for key: '%s'\n",
	    __LINE__, key);
    exit(1);
  }

  return *(Json *) ptr;
}

static inline bool json_has(Json obj, const char* key) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  return ht_has(obj.objVal, key);
}

static inline bool json_get_bool(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_BOOL);
  return json.boolVal;
}

static inline int json_get_int(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_INT);
  return json.intVal;
}

static inline float json_get_float(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_FLOAT);
  return json.floatVal;  
}

static inline double json_get_double(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_DOUBLE);
  return json.doubleVal;    
}

static inline char *json_get_string(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_STRING);
  return json.stringVal;
}

static inline Json json_get_object(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_OBJECT);
  return json;
}

static inline Json json_get_array(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_ARRAY);
  return json;
}

//END JSON_OBJECT

//BEGIN JSON_ARRAY

static inline void json_push_null(Json arr) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_NULL;
  arr_push(arr.arrVal, &json);
}

static inline void json_push_bool(Json arr, bool value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_BOOL;
  json.boolVal = value;
  arr_push(arr.arrVal, &json);
}

static inline void json_push_int(Json arr, int value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_INT;
  json.intVal = value;
  arr_push(arr.arrVal, &json);
}

static inline void json_push_float(Json arr, float value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_FLOAT;
  json.floatVal = value;
  arr_push(arr.arrVal, &json);
}

static inline void json_push_double(Json arr, double value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_DOUBLE;
  json.doubleVal = value;
  arr_push(arr.arrVal, &json);  
}

static inline void json_push_string(Json arr, const char* value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_STRING;
  json.stringVal = json_create_string(value);
  arr_push(arr.arrVal, &json);
}

static inline void json_push_object(Json arr, Json *value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  ASSERT_TYPE(value->type, JSON_OBJECT);
  Json json;
  json.type = JSON_OBJECT;
  json.objVal = value->objVal;
  arr_push(arr.arrVal, &json);
}

static inline void json_push_array(Json arr, Json *value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  ASSERT_TYPE(value->type, JSON_ARRAY);
  Json json;
  json.type = JSON_ARRAY;
  json.arrVal = value->arrVal;
  arr_push(arr.arrVal, &json);
}

static inline void json_pop(Json arr) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  arr_pop(arr.arrVal);
}

static inline Json json_opt(Json arr, int i) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  return *(Json *) arr_get(arr.arrVal, (size_t) i);
}

static inline bool json_opt_bool(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_BOOL);
  return json.boolVal;
}

static inline int json_opt_int(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_INT);
  return json.intVal;
}

static inline float json_opt_float(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_FLOAT);
  return json.floatVal;  
}

static inline double json_opt_double(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_DOUBLE);
  return json.doubleVal;
}

static inline char *json_opt_string(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_STRING);
  return json.stringVal;
}

static inline Json json_opt_object(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_OBJECT);
  return json;
}

static inline Json json_opt_array(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_ARRAY);
  return json;
}

//END JSON_ARRAY

void json_fprint(FILE *f, Json json) {
  switch(json.type) {
  case JSON_OBJECT:
    fputc('{', f);
    int last = -1;
    Ht_Entry *entry;
    while(ht_next(json.objVal, &last, &entry)) {
      fprintf(f, "\"%s\":", entry->key);
      json_fprint(f, *(Json *) entry->value);
      if(last != (int) json.objVal->count - 1) fprintf(f, ", ");
    }
    fputc('}', f);
    break;
  case JSON_ARRAY:
    fputc('[', f);
    for(size_t i=0;i<json.arrVal->count;i++) {
      json_fprint(f, * (Json *) arr_get(json.arrVal, i));
      if(i!=json.arrVal->count-1) fprintf(f, ",");
    }
    fputc(']', f);
    break;
  case JSON_NULL:
    fprintf(f, "null");
    break;
  case JSON_INT:
    fprintf(f, "%d", json.intVal);
    break;
  case JSON_FLOAT:
    fprintf(f, "%f", json.floatVal);
    break;
  case JSON_DOUBLE:
    fprintf(f, "%lf", json.doubleVal);
    break;
  case JSON_BOOL:
    fprintf(f, "%s", json.boolVal ? "true" : "false");
    break;
  case JSON_STRING:
    fprintf(f, "\"%s\"", json.stringVal);
    break;
  default:
    fprintf(stderr, "ERROR: Unknown JSON_TYPE: %d\n", json.type);
    exit(1);
    return;
  }
}

#define WRITE_CB(ptr, size) do{ \
  if(!(temp = write_callback(ptr, size, userdata))) return acc; \
  acc += temp;							   \
  }while(0)

size_t json_write(Json json, size_t (*write_callback)(void *,size_t,void *), void *userdata) {
  static char* chars = "{}[],\":nulltruefalse";
  static char tmp_buf[64];
  size_t acc = 0;
  size_t temp;
  switch(json.type) {
  case JSON_OBJECT:
    WRITE_CB(chars, 1);// {
    int last = -1;
    Ht_Entry *entry;
    while(ht_next(json.objVal, &last, &entry)) {      
      WRITE_CB(chars + 5, 1);// "
      WRITE_CB(entry->key, entry->key_size - 1);// %s
      WRITE_CB(chars + 5, 1);// "
      WRITE_CB(chars + 6, 1);// :

      if(!json_write(*(Json *) entry->value, write_callback, userdata)) return acc;
      if(last != (int) json.objVal->count - 1) {
	WRITE_CB(chars + 4, 1);
      }
    }
    WRITE_CB(chars + 1, 1);// }
    break;
  case JSON_ARRAY:
    WRITE_CB(chars + 2, 1);// [
    for(size_t i=0;i<json.arrVal->count;i++) {
      if(!json_write(* (Json *) arr_get(json.arrVal, i), write_callback, userdata)) return acc;
      if(i!=json.arrVal->count-1) {
	WRITE_CB(chars + 4, 1);
      }
    }
    WRITE_CB(chars + 3, 1);// ]
    break;
  case JSON_NULL:
    WRITE_CB(chars + 7, 4);// null
    break;
  case JSON_INT:
    if(!(temp = snprintf(tmp_buf, 64, "%d", json.intVal))) return acc;
    WRITE_CB(tmp_buf, temp);
    break;
  case JSON_FLOAT:
    if(!(temp = snprintf(tmp_buf, 64, "%f", json.floatVal))) return acc;
    WRITE_CB(tmp_buf, temp);
    break;
  case JSON_DOUBLE:
    if(!(temp = snprintf(tmp_buf, 64, "%lf", json.doubleVal))) return acc;
    WRITE_CB(tmp_buf, temp);
    break;
  case JSON_BOOL:
    if(json.boolVal) {
      WRITE_CB(chars + 11, 4);// true
    } else {
      WRITE_CB(chars + 15, 5);// false
    }
    break;
  case JSON_STRING:
    WRITE_CB(chars + 5, 1);// "
    // %s
    size_t size = 0;
    while(json.stringVal[size++]!=0);
    size--;
    WRITE_CB(json.stringVal, size);
    WRITE_CB(chars + 5, 1);// "
    break;
  default:
    fprintf(stderr, "ERROR: Unknown JSON_TYPE: %d\n", json.type);
    exit(1);
    return acc;
  }
  return acc;
}

char *json_to_cstr(Json json) {
  (void) json;

  char *cstr = (char *) malloc(2+1);
  if(!cstr) {
    fprintf(stderr, "ERORR: Can not allocate enough memory: json_to_cstr\n");
    exit(1);
  }

  cstr[0] = '{';
  cstr[1] = '}';
  cstr[2] = 0;
  
  return cstr;
}

Json json_init_object() {
  Json value;
  value.type = JSON_OBJECT;
  value.objVal = ht_init();
  return value;
}

Json json_init_array() {
  Json value;
  value.type = JSON_ARRAY;  
  value.arrVal = arr_init(sizeof(Json));
  return value;
}

int json_size(Json json) {
  if(json.type==JSON_OBJECT) {
    return (int) json.objVal->count;
  }
  else if(json.type==JSON_ARRAY) {
    return (int) json.arrVal->count;
  }
  else {
    return -1;
  }  
}

void json_free(Json json) {  
  if(json.type==JSON_OBJECT) {
    if(!json.objVal) return;
    ht_free(json.objVal);    
  }
  else if(json.type==JSON_ARRAY) {
    if(!json.arrVal) return;
    arr_free(json.arrVal);
  }
  else if(json.type==JSON_STRING) {
    if(json.stringVal) free(json.stringVal);
  }
  else {
    
  }
}

void json_free_all(Json json) {  
  if(json.type==JSON_OBJECT) {
    Ht_Entry *entry;
    int last = -1;

    while(ht_next(json.objVal, &last, &entry)) {
      json_free_all(*(Json *) entry->value);
    }
    
    ht_free(json.objVal);
  }
  else if(json.type==JSON_ARRAY) {
    for(size_t i=0;i<json.arrVal->count;i++) {
      json_free_all(*(Json *) arr_get(json.arrVal, i));
    }
    arr_free(json.arrVal);
  }
  else if(json.type==JSON_STRING) {
    if(json.stringVal) free(json.stringVal);
  }
  else {
    
  }
}

bool parseJsonNull(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  if((*m = parseString("null", 4, buffer, buffer_size)) == 0) {
    return false;
  }
  json->type = JSON_NULL;
  return true;
}

bool parseJsonBool(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  if((*m = parseString("true", 4, buffer, buffer_size)) != 0) {
    json->type = JSON_BOOL;
    json->boolVal = true;
    return true;
  }
  if((*m = parseString("false", 5, buffer, buffer_size)) != 0) {
    json->type = JSON_BOOL;
    json->boolVal = false;
    return true;
  }

  return false;
}

bool isDigitOrDot(char c) {
  return (c == '.') | isDigit(c);
}

bool parseJsonInt(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t neg = parseChar('-', buffer, buffer_size);
  size_t off = skipCharIf(isSpace, buffer+neg, buffer_size-neg);
  size_t n;
  if((n = parseStringIf(isDigit, buffer+neg+off, buffer_size-neg-off)) == 0) {
    return false;
  }
  if(parseStringIf(isDigitOrDot, buffer+neg+off, buffer_size-neg-off) > n) {
    return false;
  }
  json->type = JSON_INT;
  json->intVal = 0;
  for(size_t i=0;i<n;i++) {
    json->intVal = json->intVal * 10;
    if(neg)
      json->intVal -= (buffer[i+off+neg] - '0');
    else
      json->intVal += (buffer[i+off+neg] - '0');
  }
  (*m) = n + off;
  if(neg) (*m)+=1;
  return true;
}

bool parseJsonFloat(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t neg = parseChar('-', buffer, buffer_size);
  size_t off = skipCharIf(isSpace, buffer+neg, buffer_size-neg);
  size_t n;
  if((n = parseStringIf(isDigitOrDot, buffer+neg+off, buffer_size-neg-off)) == 0) {
    return false;
  }
  char *endptr;
  float f = strtof(buffer+neg+off, &endptr);
  if(f == 0.0f && *endptr != *(buffer+neg+off+n)) {
    return false;
  }
  json->type = JSON_FLOAT;
  json->floatVal = f;
  (*m) = n + off;
  if(neg) (*m)+=1;
  return true;
}

bool parseJsonString(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t i = 0;
  if((parseChar('\"', buffer, buffer_size) == 0) && (parseChar('\'', buffer, buffer_size) == 0)) {
    return false;
  }
  i++;

/*
  char *json_cstr = (char *) malloc(size + 1);
  if(!json_cstr) {
    fprintf(stderr, "ERROR: Can not allocate enough memory: json_create_string\n");
    exit(1);
  }  
  memcpy(json_cstr, cstr, size);
  json_cstr[size] = 0;
  
  return json_cstr;
  */

	// \n
    // \u1234
  size_t j = 0;
  int escape_chars_count = 0;
  while(j < buffer_size - i) {
  	char c = buffer[i+j];

  	if(c == '\\' && j < buffer_size - i - 1) {
  		char d = buffer[i+j+1];
  		if(d == '\"' || d == '\\' || d=='/' || d=='b' || d=='f' || d=='n' || d=='r' || d=='t') {
			escape_chars_count++;
			j++;
  		} else if(d == 'u') {
  			if(j >= buffer_size - i - 1 - 4) {
  				break;
  			}
  			bool loop_breaked = false;
  			for(size_t k=0;k<4;k++) {
  				char e = buffer[i+j+2+k];
  				if(!isHexDigit(e)) {
  					loop_breaked = true;
  					break;
  				}
  			}
  			if(loop_breaked) {
  				break;
  			}
  			j+=5;
  		} else {
  			break;
  		}
  	} else if(!normalChar(c)) {
  		break;
  	}  	
  	j++;
  }

  i += j;
  //i += skipCharIf(normalChar, buffer+i, buffer_size-i);

  if((parseChar('\"', buffer, buffer_size) == 0) && (parseChar('\'', buffer, buffer_size) == 0)) {
    return false;
  }
  i++;

  json->type = JSON_STRING;
  json->stringVal = escape_chars_count==0 
    ? json_create_cstr_from_parts(buffer+1, i-2) 
  	: json_create_cstr_from_parts_escape_chars(buffer+1, i-2, escape_chars_count);

  *m = i;
  
  return true;
}

bool parsePair(const char *buffer, size_t buffer_size, char **key, Json *value, size_t *m) {
  Json json;
  size_t n;  
  if(!parseJsonString(buffer, buffer_size, &json, &n)) {
    return false;
  }

  *key = json.stringVal;

  size_t i = n;
  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  if(parseChar(':', buffer + i, buffer_size - i) == 0) {
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  if(!parseJson(buffer + i, buffer_size - i, &json, &n)) {
    return false;
  }

  *value = json;

  *m = i + n;
  
  return true;
}

bool parseJsonArray(const char *buffer, size_t buffer_size,
		    Json *json, size_t *m) {
  size_t i = 0;
  if(parseChar('[', buffer + i, buffer_size - i) == 0) {
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  //PARSE ELEMENTS
  Json arr = json_init_array();
  Json element;
  size_t n;  
  while(parseJson(buffer + i, buffer_size - i, &element, &n)) {
    arr_push(arr.arrVal, &element);
    i += n;

    //sepBy
    i += skipCharIf(isSpace, buffer+i, buffer_size-i);
    if(parseChar(',', buffer + i, buffer_size - i) == 0) {
      break;
    }
    i++;
    i += skipCharIf(isSpace, buffer+i, buffer_size-i);
    
  }
  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  if(parseChar(']', buffer + i, buffer_size - i) == 0) {
    json_free_all(arr);
    return false;
  }
  i++;

  json->type = JSON_ARRAY;
  json->arrVal = arr.arrVal;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  *m = i;
  return true;
}

bool parseJsonObject(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t i = 0;
  if(parseChar('{', buffer + i, buffer_size - i) == 0) {
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  //SEP_BY
  Json obj = json_init_object();
  char *key;
  Json value;
  size_t n;
  while(parsePair(buffer + i, buffer_size - i, &key, &value, &n)) {
    //CSTR TO INSERT IN HASHTABLE
    
    ht_insert(obj.objVal, key, &value, sizeof(Json));
    i += n;

    free(key);
    
    //WHITESPACE
    i += skipCharIf(isSpace, buffer+i, buffer_size-i);

    if(parseChar(',', buffer + i, buffer_size - i) == 0) {
      break;
    }
    i++;

    //WHITESPACE
    i += skipCharIf(isSpace, buffer+i, buffer_size-i);
  }

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);
  
  if(parseChar('}', buffer + i, buffer_size - i) == 0) {
    json_free_all(obj);
    return false;
  }
  i++;

  json->type = JSON_OBJECT;
  json->objVal = obj.objVal;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  *m = i;
  return true;

}

bool parseJson(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  if(parseJsonNull(buffer, buffer_size, json, m)) {
    return true;
  }
  else if(parseJsonBool(buffer, buffer_size, json, m)) {
    return true;
  }
  else if(parseJsonInt(buffer, buffer_size, json, m)) {
    return true;
  }
  else if(parseJsonFloat(buffer, buffer_size, json, m)) {
    return true;
  }
  else if(parseJsonString(buffer, buffer_size, json, m)) {
    return true;
  }
  else if(parseJsonArray(buffer, buffer_size, json, m)) {
    return true;
  }
  else if(parseJsonObject(buffer, buffer_size, json, m)) {
    return true;
  }

  return false;
}

bool json_parse_len(const char *buffer, size_t buffer_size, Json *json) {
  size_t m;
  if(!parseJson(buffer, buffer_size, json, &m)) return false;
  if(m!=buffer_size) return false;
  return true;
}

bool parseXmlInt(const char *buffer, size_t buffer_size, Json *value, size_t *m) {
  return parseJsonInt(buffer, buffer_size, value, m);
}

bool normalCharXml(char c) {
  if(c=='>' || c=='<') return false;
  return true;
}

bool parseXmlString(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t i = 0;  
  i += skipCharIf(normalCharXml, buffer+i, buffer_size-i);
  
  if(i == 0) {
    return false;
  }

  size_t end = i - 1;
  while(isSpace(buffer[end])) {
    end--;
  }

  json->type = JSON_STRING;
  json->stringVal = json_create_cstr_from_parts(buffer, end + 1);

  *m = i;  
  return true;
}

bool parseXmlValue(const char *buffer, size_t buffer_size, Json *value, size_t *m) {
  if(parseXmlInt(buffer, buffer_size, value, m)) {
    return true;
  } else if(parseXmlString(buffer, buffer_size, value, m)) {
    return true;
  }
  return false;
}

bool normalCharNoEq(char c) {
  if(c == '=' || isSpace(c)) return false;
  return normalChar(c);
}

bool normalCharNoParent(char c) {
  if(c == '\"' || isSpace(c)) return false;
  return true;
}

bool xmlTagNameChar(char c) {
  if(isSpace(c)) return false;
  if(c == '/' || c=='=' || c=='>' || c=='<') return false;
  return normalChar(c);
}

bool parseXmlPair(const char *buffer, size_t buffer_size, char **key, Json *value, size_t *m) {
  size_t i=0;
  
  i += skipCharIf(normalCharNoEq, buffer+i, buffer_size-i);
  if(i==0) {
    return false;
  }
  size_t key_len = i;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);
  
  if((parseChar('=', buffer + i, buffer_size - i) == 0)) {
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  if((parseChar('\"', buffer + i, buffer_size - i) == 0)) {
    return false;
  }
  i++;

  size_t value_start = i;
  i += skipCharIf(normalCharNoParent, buffer+i, buffer_size-i);
  size_t value_len = i - value_start;

  if((parseChar('\"', buffer + i, buffer_size - i) == 0)) {
    return false;
  }
  i++;

  *key = json_create_cstr_from_parts(buffer, key_len);
  value->type = JSON_STRING;
  value->stringVal = json_create_cstr_from_parts(buffer + value_start, value_len);
  *m = i;

  return true;
}

size_t strlen_unicode(const char *cstr) {
	size_t i = 0;
	while(true) {
		if(cstr[i] == '\\' && cstr[i]=='u') {
			i+=5;
		} else if(cstr[i] == 0) {
			break;
		}
		i++;
	}
	return i;
}

bool json_parse(const char *buffer, Json *json) {
  return json_parse_len(buffer, strlen(buffer), json);
}

bool parseXmlTag(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t i = 0;
  //CLEAN UP BEFORE
  i += skipCharIf(isSpace, buffer+i, buffer_size-i);
  
  if((parseChar('<', buffer + i, buffer_size - i) == 0)) {
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);
  size_t start = i;

  i += skipCharIf(xmlTagNameChar, buffer+i, buffer_size-i);
  size_t len = i - start;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  Json obj = json_init_object();

  //SEP BY ATTRIBUTES
  Json value;
  size_t n;  
  char *key;
  while(parseXmlPair(buffer + i, buffer_size - i, &key, &value, &n)) {
    ht_insert(obj.objVal, key, &value, sizeof(Json));
    i += n;

    free(key);

    size_t temp = skipCharIf(isSpace, buffer + i, buffer_size - i);
    if(temp == 0) {
      break;
    }   
    i += temp;
  }

  if((parseChar('>', buffer + i , buffer_size - i) == 0)) {
    json_free_all(obj);
    return false;
  }
  i++;
  
  Json arr;
  arr.type = JSON_NULL;
  //SEP BY CHILDRENS
  while(parseXml(buffer + i, buffer_size - i, &value, &n)) {
    if(arr.type == JSON_NULL) {
      arr = json_init_array();
    }
    arr_push(arr.arrVal, &value);
    i += n;

    size_t temp = skipCharIf(isSpace, buffer + i, buffer_size - i);
    /*
    if(temp == 0) {
      break;
    }
    */
    i += temp;
  }

  //TRY TO PARSE A VALUE
  Json val;
  val.type = JSON_NULL;
  if(arr.type == JSON_NULL) {
    i+= skipCharIf(isSpace, buffer+i, buffer_size-i);
    if(parseXmlValue(buffer + i, buffer_size - i, &val, &n)) {
      i += n;
    }
  }

  i+= skipCharIf(isSpace, buffer+i, buffer_size-i);

  if((parseChar('<', buffer + i, buffer_size - i) == 0)) {
    json_free_all(obj);
    json_free_all(arr);
    return false;
  }
  i++;

  i+= skipCharIf(isSpace, buffer+i, buffer_size-i);

  if((parseChar('/', buffer + i, buffer_size - i) == 0)) {
    json_free_all(obj);
    json_free_all(arr);
    return false;
  }
  i++;
  
  i+= skipCharIf(isSpace, buffer+i, buffer_size-i);

  if(!parseString(buffer + start, len, buffer + i, buffer_size - i)) {
    json_free_all(obj);
    json_free_all(arr);
    return false;
  }
  i+=len;

  i+= skipCharIf(isSpace, buffer+i, buffer_size-i);

  if((parseChar('>', buffer + i, buffer_size - i) == 0)) {    
    json_free_all(obj);
    json_free_all(arr);
    return false;
  }
  i++;

  //CLEAN UP AFTER
  i+= skipCharIf(isSpace, buffer+i, buffer_size-i);

  Json json_string;
  json_string.type = JSON_STRING;
  json_string.stringVal = json_create_cstr_from_parts(buffer + start, len);
  
  ht_insert(obj.objVal, "XML_NAME", &json_string, sizeof(Json));
  if(arr.type != JSON_NULL) ht_insert(obj.objVal, "XML_CHILDREN", &arr, sizeof(Json));
  if(val.type != JSON_NULL) ht_insert(obj.objVal, "XML_VALUE", &val, sizeof(Json));
  
  json->type = JSON_OBJECT;
  json->objVal = obj.objVal;
  
  *m = i;

  return true;
}

bool parseXmlSelfClosingTag(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t i = 0;
  i += skipCharIf(isSpace, buffer+i, buffer_size-i);
  
  if((parseChar('<', buffer + i, buffer_size - i) == 0)) {
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);
  size_t start = i;

  i += skipCharIf(xmlTagNameChar, buffer+i, buffer_size-i);
  size_t len = i - start;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  Json obj = json_init_object();

  //SEP BY
  char *key;
  Json value;
  size_t n;
  while(parseXmlPair(buffer + i, buffer_size - i, &key, &value, &n)) {
    ht_insert(obj.objVal, key, &value, sizeof(Json));
    i += n;

    free(key);
        
    size_t temp = skipCharIf(isSpace, buffer + i, buffer_size - i);
    if(temp == 0) {
      break;
    }
    
    i += temp;
  }

  if((parseChar('/', buffer + i , buffer_size - i) == 0)) {
    json_free_all(obj);
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-i);

  if((parseChar('>', buffer + i , buffer_size - i) == 0)) {
    json_free_all(obj);
    return false;
  }
  i++;

  i+= skipCharIf(isSpace, buffer+i, buffer_size-i);

  Json json_string;
  json_string.type = JSON_STRING;
  json_string.stringVal = json_create_cstr_from_parts(buffer + start, len);
  
  ht_insert(obj.objVal, "XML_NAME", &json_string, sizeof(Json));
  
  json->type = JSON_OBJECT;
  json->objVal = obj.objVal;
  
  *m = i;

  return true;
}

bool parseXml(const char* buffer, size_t buffer_size, Json *json, size_t *m) {
  if(parseXmlSelfClosingTag(buffer, buffer_size, json, m)) {
    return true;
  } else if(parseXmlTag(buffer, buffer_size, json, m)) {
    return true;
  }
  return false;
}

bool xml_parse_len(const char *source, size_t source_len, Json *json) {
  size_t m;
  if(!parseXml(source, source_len, json, &m)) return false;
  if(m!=source_len) return false;
  return true;
}

bool xml_parse(const char *source, Json *json) {
  size_t size = strlen(source);
  return xml_parse_len(source, size, json);
}

#endif

#endif //JSON_H_H
