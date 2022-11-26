#ifndef JSON_H_H
#define JSON_H_H

#ifdef JSON_IMPLEMENTATION

#define HASHTABLE_IMPLEMENTATION
#define ARRAY_IMPLEMENTATION
#define PARSE_IMPLEMENTATION

#endif

#include "hashtable.h"
#include "array.h"
#include "parse.h"

//----------- JSON -------------

#define JSON_STRING_BUFFER_CAP 64

typedef struct{
  char data[JSON_STRING_BUFFER_CAP];
  size_t size;
}JsonString;

JsonString json_create_string(const char* cstr);
JsonString json_create_string_from_parts(const char* cstr, size_t len);

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

typedef struct{
  JsonType type;
  union {
    int intVal;
    float floatVal;
    double doubleVal;
    bool boolVal;
    JsonString stringVal;
    Ht *objVal;
    Arr *arrVal;
  };
}Json;

Json json_init_object();
Json json_init_array();

//BEGIN JSON_OBJECT
void json_put_null(Json obj, const char *key);
void json_put_bool(Json obj, const char *key, bool value);
void json_put_int(Json obj, const char *key, int value);
void json_put_float(Json obj, const char *key, float value);
void json_put_double(Json obj, const char *key, double value);
void json_put_string(Json obj, const char *key, const char* value);
void json_put_object(Json obj, const char *key, Json *value);
void json_put_array(Json obj, const char *key, Json *value);

void json_remove(Json obj, const char *key);

Json json_get(Json obj, const char* key);
bool json_has(Json obj, const char* key);

bool json_get_bool(Json obj, const char *key);
int json_get_int(Json obj, const char *key);
float json_get_float(Json obj, const char *key);
double json_get_double(Json obj, const char *key);
JsonString json_get_string(Json obj, const char *key);
Json json_get_object(Json obj, const char *key);
Json json_get_array(Json obj, const char *key);
//END JSON_OBJECT

//BEGIN JSON_ARRAY
void json_push_null(Json arr);
void json_push_bool(Json arr, bool value);
void json_push_int(Json arr, int value);
void json_push_float(Json arr, float value);
void json_push_double(Json arr, double value);
void json_push_string(Json arr, const char* value);
void json_push_arrect(Json arr, Json *value);
void json_push_array(Json arr, Json *value);

void json_pop(Json arr);

Json json_opt(Json arr, int i);
bool json_opt_bool(Json obj, int p);
int json_opt_int(Json obj, int p);
float json_opt_float(Json obj, int p);
double json_opt_double(Json obj, int p);
JsonString json_opt_string(Json obj, int p);
Json json_opt_object(Json obj, int p);
Json json_opt_array(Json obj, int p);
//END JSON_ARRAY

void json_fprint(FILE *f, Json json);
char *json_to_cstr(Json json);

int json_size(Json json);

void json_free(Json json);
void json_free_all(Json json);

//----------- JSON_PARSE -------------

bool parseJson(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonNull(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonBool(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonInt(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonString(const char *buffer, size_t buffer_size, Json *json, size_t *m);

bool parsePair(const char *buffer, size_t buffer_size, JsonString *key, Json *value, size_t *m);
bool parseJsonArray(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJsonObject(const char *buffer, size_t buffer_size, Json *json, size_t *m);
bool parseJson(const char *buffer, size_t buffer_size, Json *json, size_t *m);

bool json_parse(const char* buffer, Json *json);
bool json_parse_len(const char* buffer, size_t buffer_size, Json *json);

#ifdef JSON_IMPLEMENTATION

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

JsonString json_create_string(const char* cstr) {
  JsonString string;
  size_t size = 0;  
  while(cstr[size++]!=0);
  if(size>=JSON_STRING_BUFFER_CAP) {
    fprintf(stderr, "Json_String-Buffer-Overflow: %lld>%d", size, JSON_STRING_BUFFER_CAP);
    exit(1);
  }
  memcpy(string.data, cstr, size);
  string.data[size] = 0;
  string.size = size;
  return string;
}

JsonString json_create_string_from_parts(const char* cstr, size_t len) {
  if(len>=JSON_STRING_BUFFER_CAP) {
    fprintf(stderr, "Json_String-Buffer-Overflow: %lld>%d", len, JSON_STRING_BUFFER_CAP);
    exit(1);
  }

  JsonString string;
  memcpy(string.data, cstr, len);
  string.data[len] = 0;
  string.size = len;
  return string;
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

void json_put_null(Json obj, const char *key) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_NULL;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

void json_put_bool(Json obj, const char *key, bool value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_BOOL;
  json.boolVal = value;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

void json_put_int(Json obj, const char *key, int value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_INT;
  json.intVal = value;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

void json_put_float(Json obj, const char *key, float value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_FLOAT;
  json.floatVal = value;
  ht_insert(obj.objVal, key, &json, sizeof(Json));  
}

void json_put_double(Json obj, const char *key, double value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_DOUBLE;
  json.doubleVal = value;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

void json_put_string(Json obj, const char *key, const char* value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  Json json;
  json.type = JSON_STRING;
  json.stringVal = json_create_string(value);
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

void json_put_object(Json obj, const char *key, Json *value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  ASSERT_TYPE(value->type, JSON_OBJECT);
  Json json;
  json.type = JSON_OBJECT;
  json.objVal = value->objVal;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

void json_put_array(Json obj, const char *key, Json *value) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  ASSERT_TYPE(value->type, JSON_ARRAY);
  Json json;
  json.type = JSON_ARRAY;
  json.arrVal = value->arrVal;
  ht_insert(obj.objVal, key, &json, sizeof(Json));
}

void json_remove(Json obj, const char *key) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  ht_remove(obj.objVal, key);
}

Json json_get(Json obj, const char* key) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  void *ptr = ht_get(obj.objVal, key);
  
  if(!ptr) {
    fprintf(stderr, "ERROR:json.h:%d:Json does not have value for key: '%s'\n",
	    __LINE__, key);
    exit(1);
  }

  return *(Json *) ptr;
}

bool json_has(Json obj, const char* key) {
  ASSERT_TYPE(obj.type, JSON_OBJECT);
  return ht_has(obj.objVal, key);
}

bool json_get_bool(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_BOOL);
  return json.boolVal;
}

int json_get_int(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_INT);
  return json.intVal;
}

float json_get_float(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_FLOAT);
  return json.floatVal;  
}

double json_get_double(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_DOUBLE);
  return json.doubleVal;    
}

JsonString json_get_string(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_STRING);
  return json.stringVal;
}

Json json_get_object(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_OBJECT);
  return json;
}

Json json_get_array(Json obj, const char *key) {
  Json json = json_get(obj, key);
  ASSERT_TYPE(json.type, JSON_ARRAY);
  return json;
}

//END JSON_OBJECT

//BEGIN JSON_ARRAY

void json_push_null(Json arr) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_NULL;
  arr_push(arr.arrVal, &json);
}

void json_push_bool(Json arr, bool value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_BOOL;
  json.boolVal = value;
  arr_push(arr.arrVal, &json);
}

void json_push_int(Json arr, int value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_INT;
  json.intVal = value;
  arr_push(arr.arrVal, &json);
}

void json_push_float(Json arr, float value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_FLOAT;
  json.floatVal = value;
  arr_push(arr.arrVal, &json);
}

void json_push_double(Json arr, double value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_DOUBLE;
  json.doubleVal = value;
  arr_push(arr.arrVal, &json);  
}

void json_push_string(Json arr, const char* value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  Json json;
  json.type = JSON_STRING;
  json.stringVal = json_create_string(value);
  arr_push(arr.arrVal, &json);
}

void json_push_object(Json arr, Json *value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  ASSERT_TYPE(value->type, JSON_OBJECT);
  Json json;
  json.type = JSON_OBJECT;
  json.objVal = value->objVal;
  arr_push(arr.arrVal, &json);
}

void json_push_array(Json arr, Json *value) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  ASSERT_TYPE(value->type, JSON_ARRAY);
  Json json;
  json.type = JSON_ARRAY;
  json.arrVal = value->arrVal;
  arr_push(arr.arrVal, &json);
}

void json_pop(Json arr) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  arr_pop(arr.arrVal);
}

Json json_opt(Json arr, int i) {
  ASSERT_TYPE(arr.type, JSON_ARRAY);
  return *(Json *) arr_get(arr.arrVal, (size_t) i);
}

bool json_opt_bool(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_BOOL);
  return json.boolVal;
}

int json_opt_int(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_INT);
  return json.intVal;
}

float json_opt_float(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_FLOAT);
  return json.floatVal;  
}

double json_opt_double(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_DOUBLE);
  return json.doubleVal;
}

JsonString json_opt_string(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_STRING);
  return json.stringVal;
}

Json json_opt_object(Json obj, int p) {
  Json json = json_opt(obj, p);
  ASSERT_TYPE(json.type, JSON_OBJECT);
  return json;
}

Json json_opt_array(Json obj, int p) {
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
      fprintf(f, "\"%s\": ", entry->key);
      json_fprint(f, *(Json *) entry->value);
      if(last != (int) (json.objVal)->count - 1) fprintf(f, ", ");
    }
    fputc('}', f);
    break;
  case JSON_ARRAY:
    fputc('[', f);
    for(size_t i=0;i<json.arrVal->count;i++) {
      json_fprint(f, * (Json *) arr_get(json.arrVal, i));
      if(i!=json.arrVal->count-1) fprintf(f, ", ");
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
    fprintf(f, "\"%.*s\"", (int) json.stringVal.size, json.stringVal.data);
    break;
  default:
    fprintf(stderr, "ERROR: Unknown JSON_TYPE: %d\n", json.type);
    exit(1);
    return;
  }
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
  Arr *array = arr_init(sizeof(Json));
  value.arrVal = array;
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
    if(json.objVal) ht_free(json.objVal);    
  }
  else if(json.type==JSON_ARRAY) {
    if(json.arrVal) arr_free(json.arrVal);
  }
  else {
    
  }
}

void json_free_all(Json json) {  
  if(json.type==JSON_OBJECT) {
    if(!json.objVal) return;
    Ht_Entry *entry;
    int last = -1;

    while(ht_next(json.objVal, &last, &entry)) {
      json_free_all(*(Json *) entry->value);
    }
    
    ht_free(json.objVal);
  }
  else if(json.type==JSON_ARRAY) {
    if(!json.arrVal) return;
    for(size_t i=0;i<json.arrVal->count;i++) {
      json_free_all(*(Json *) arr_get(json.arrVal, i));
    }
    arr_free(json.arrVal);
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

bool parseJsonInt(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t neg = parseChar('-', buffer, buffer_size);
  size_t off = skipCharIf(isSpace, buffer+neg, buffer_size-neg);
  if((*m = parseStringIf(isDigit, buffer+neg+off, buffer_size-neg-off)) == 0) {
    return false;
  }
  json->type = JSON_INT;
  json->intVal = 0;
  for(size_t i=0;i<*m;i++) {
    json->intVal = json->intVal * 10;
    if(neg)
      json->intVal -= (buffer[i+off+neg] - '0');
    else
      json->intVal += (buffer[i+off+neg] - '0');
  }
  if(neg) (*m)+=off+1;
  return true;
}

bool parseJsonString(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t i = 0;
  if(parseChar('\"', buffer, buffer_size) == 0) {
    return false;
  }
  i++;

  i += skipCharIf(normalChar, buffer+i, buffer_size-1);

  if(parseChar('\"', buffer + i, buffer_size - i) == 0) {
    return false;
  }
  i++;

  json->type = JSON_STRING;
  json->stringVal = json_create_string_from_parts(buffer+1, i-2);

  *m = i;
  
  return true;
}

bool parsePair(const char *buffer, size_t buffer_size, JsonString *key, Json *value, size_t *m) {
  Json json;
  size_t n;
  if(!parseJsonString(buffer, buffer_size, &json, &n)) {
    return false;
  }
  
  memcpy(key, &json.stringVal, sizeof(JsonString));

  size_t i = n;
  i += skipCharIf(isSpace, buffer+i, buffer_size-1);

  if(parseChar(':', buffer + i, buffer_size - i) == 0) {
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-1);

  if(!parseJson(buffer + i, buffer_size - i, &json, &n)) {
    return false;
  }

  memcpy(value, &json, sizeof(Json));

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

  i += skipCharIf(isSpace, buffer+i, buffer_size-1);

  //PARSE ELEMENTS
  Json arr = json_init_array();
  Json element;
  size_t n;  
  while(parseJson(buffer + i, buffer_size - i, &element, &n)) {
    arr_push(arr.arrVal, &element);
    i += n;

    //sepBy
    i += skipCharIf(isSpace, buffer+i, buffer_size-1);
    if(parseChar(',', buffer + i, buffer_size - i) == 0) {
      break;
    }
    i++;
    i += skipCharIf(isSpace, buffer+i, buffer_size-1);
    
  }
  i += skipCharIf(isSpace, buffer+i, buffer_size-1);

  if(parseChar(']', buffer + i, buffer_size - i) == 0) {
    json_free(arr);
    return false;
  }
  i++;

  json->type = JSON_ARRAY;
  json->arrVal = arr.arrVal;

  i += skipCharIf(isSpace, buffer+i, buffer_size-1);

  *m = i;
  return true;
}

bool parseJsonObject(const char *buffer, size_t buffer_size, Json *json, size_t *m) {
  size_t i = 0;
  if(parseChar('{', buffer + i, buffer_size - i) == 0) {
    return false;
  }
  i++;

  i += skipCharIf(isSpace, buffer+i, buffer_size-1);

  //SEP_BY
  Json obj = json_init_object();
  JsonString key;
  Json value;
  size_t n;
  while(parsePair(buffer + i, buffer_size - i, &key, &value, &n)) {
    //CSTR TO INSERT IN HASHTABLE
    char cstr[key.size+1];
    memcpy(cstr, key.data, key.size);
    cstr[key.size] = 0;
    
    ht_insert(obj.objVal, cstr, &value, sizeof(Json));
    i += n;
    
    //WHITESPACE
    i += skipCharIf(isSpace, buffer+i, buffer_size-1);

    if(parseChar(',', buffer + i, buffer_size - i) == 0) {
      break;
    }
    i++;

    //WHITESPACE
    i += skipCharIf(isSpace, buffer+i, buffer_size-1);
  }

  i += skipCharIf(isSpace, buffer+i, buffer_size-1);
  
  if(parseChar('}', buffer + i, buffer_size - i) == 0) {
    return false;
  }
  i++;

  json->type = JSON_OBJECT;
  json->objVal = obj.objVal;

  i += skipCharIf(isSpace, buffer+i, buffer_size-1);

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

bool json_parse(const char *buffer, Json *json) {
  return json_parse_len(buffer, strlen(buffer), json);
}

#endif

#endif //JSON_H_H
