#ifndef REGEX_H_H_
#define REGEX_H_H_

#ifdef REGEX_IMPLEMENTATION

#define ARRAY_IMPLEMENTATION
#define QUEUE_IMPLEMENTATION

#endif //REGEX_IMPLEMENTATION

#include "./queue.h"
#include "./array.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef REGEX_DEF
#define REGEX_DEF static inline
#endif //REGEX_DEF

typedef enum{
  REGEX_TYPE_NONE = 0,
  REGEX_TYPE_ANY,
  REGEX_TYPE_ELEMENT,
  REGEX_TYPE_GROUP_ELEMENT,
}Regex_Type;

typedef enum{
  REGEX_KIND_NONE = 0,
  REGEX_KIND_EXACTLY_ONE,
  REGEX_KIND_ZERO_OR_ONE,
  REGEX_KIND_ZERO_OR_MORE,
}Regex_Kind;

typedef struct Regex_State Regex_State;

typedef struct{
  Arr *states;
}Regex;

struct Regex_State{
  Regex_Type type;
  Regex_Kind kind;
  bool recursive;
  typedef union{
    char value;
    Regex states;
  };
};

typedef struct{
  bool backtrackable;
  size_t state_index;
  Arr *consumptions;
}Regex_Backtrack;

REGEX_DEF bool regex_compile(Regex *regex, const char *regex_cstr);
REGEX_DEF bool regex_match(Regex *regex, const char* cstr, size_t *off, size_t *len);

REGEX_DEF void regex_print(Regex *regex);
REGEX_DEF bool regex_init(Regex *regex);
REGEX_DEF bool regex_append(Regex *regex, Regex_State state);
REGEX_DEF void regex_free(Regex *regex);

REGEX_DEF const char *regex_type_name(Regex_Type type);
REGEX_DEF const char *regex_kind_name(Regex_Kind kind);

#ifdef REGEX_IMPLEMENTATION

REGEX_DEF const char *regex_type_name(Regex_Type type) {
  switch(type) {
  case REGEX_TYPE_NONE:
    return "NONE";
  case REGEX_TYPE_ANY:
    return "ANY";
  case REGEX_TYPE_ELEMENT:
    return "ELEMENT";
  case REGEX_TYPE_GROUP_ELEMENT:
    return "GROUP_ELEMENT";
  default:
    return NULL;
  }
}

REGEX_DEF const char *regex_kind_name(Regex_Kind kind) {
  switch(kind) {
  case REGEX_KIND_NONE:
    return "NONE";
  case REGEX_KIND_EXACTLY_ONE:
    return "EXACTLY_ONE";
  case REGEX_KIND_ZERO_OR_ONE:
    return "ZERO_OR_ONE";
  case REGEX_KIND_ZERO_OR_MORE:
    return "ZERO_OR_MORE";
  default:
    return NULL;
  }
}

REGEX_DEF void regex_print_impl(Regex *regex, int indent);

#define REGEX_PRINT_IMPL_INDENT(n) for(int _indent=0;_indent<n;_indent++) printf("\t");

REGEX_DEF void regex_state_print(Regex_State *state, int indent) {
  if(!state->recursive) {
    REGEX_PRINT_IMPL_INDENT(indent+1);
    if(state->type == REGEX_TYPE_ANY) {
      printf("{ %s, %s }\n",
	     regex_type_name(state->type),
	     regex_kind_name(state->kind));	
    } else {
      printf("{ '%c', %s, %s }\n",
	     state->value,
	     regex_type_name(state->type),
	     regex_kind_name(state->kind));	
    }
  } else {
    REGEX_PRINT_IMPL_INDENT(indent + 1);
    printf("{ %s, %s,\n",
	   regex_type_name(state->type),
	   regex_kind_name(state->kind));
    regex_print_impl(&state->states, indent+2);
    REGEX_PRINT_IMPL_INDENT(indent + 1);
    printf("}\n");
  }
}

REGEX_DEF void regex_print_impl(Regex *regex, int indent) {
  REGEX_PRINT_IMPL_INDENT(indent);
  printf("[\n");
  for(size_t i=0;i<regex->states->count;i++) {
    Regex_State *state = (Regex_State *) arr_get(regex->states, i);
    regex_state_print(state, indent);
  }
  REGEX_PRINT_IMPL_INDENT(indent);
  printf("]\n");
}

REGEX_DEF void regex_print(Regex *regex) {
  regex_print_impl(regex, 0);
}

REGEX_DEF bool regex_init(Regex *regex) {
  regex->states = arr_init(sizeof(Regex_State));
  return true;
}

REGEX_DEF bool regex_append(Regex *regex, Regex_State state) {
  arr_push(regex->states, &state);
  return true;
}

REGEX_DEF void regex_free(Regex *regex) {
  for(size_t i=0;i<regex->states->count;i++) {
    Regex_State *state = (Regex_State *) arr_get(regex->states, i);
    if(state->recursive) {
      regex_free(&state->states);
    }
  }
  arr_free(regex->states);  
}

REGEX_DEF bool regex_compile(Regex *out_regex, const char *regex_cstr) {

  size_t stack_cap = 8;
  size_t stack_len = 0;
  Regex *stack = (Regex * ) malloc(sizeof(Regex) * stack_cap);
  if(!stack) {
    return false;
  }

  if(!regex_init(&stack[stack_len++])) {
    return false;
  }

  size_t regex_cstr_len = strlen(regex_cstr);
  size_t i = 0;
  while(i < regex_cstr_len) {
    char next = regex_cstr[i];

    switch(next) {
    case '.': {
      Regex *last = &stack[stack_len-1];

      Regex_State state = {0};
      state.type = REGEX_TYPE_ANY;
      state.recursive = false;
      state.kind = REGEX_KIND_EXACTLY_ONE;

      if(!regex_append(last, state)) {
	return false;
      }
      
      i++;
      continue;
    } break;
    case '+': {
      Regex *last = &stack[stack_len-1];
      if(last->states->count == 0) {
	fprintf(stderr, "ERROR: Quantifier must follow an unquantified element or group\n");
	return false;
      }      
      Regex_State *last_element = (Regex_State *) arr_get(last->states, last->states->count - 1);
      if(last_element->kind != REGEX_KIND_EXACTLY_ONE) {
	fprintf(stderr, "ERROR: Quantifier must follow an unquantified element or group\n");
	return false;
      }

      Regex_State state = *last_element;
      state.kind = REGEX_KIND_ZERO_OR_MORE;
	
      if(!regex_append(last, state)) {
	return false;
      }

      i++;
      continue;      
    } break;
    case '*': {
      Regex *last = &stack[stack_len-1];
      if(last->states->count == 0) {
	fprintf(stderr, "ERROR: Quantifier must follow an unquantified element or group\n");
	return false;
      }
      Regex_State *last_element = (Regex_State *) arr_get(last->states, last->states->count - 1);
      if(last_element->kind != REGEX_KIND_EXACTLY_ONE) {
	fprintf(stderr, "ERROR: Quantifier must follow an unquantified element or group\n");
	return false;
      }      
      last_element->kind = REGEX_KIND_ZERO_OR_MORE;
      
      i++;
      continue;
    } break;
    case '?': {
      Regex *last = &stack[stack_len-1];
      if(last->states->count == 0) {
	fprintf(stderr, "ERROR: Quantifier must follow an unquantified element or group\n");
	return false;
      }      
      Regex_State *last_element = (Regex_State *) arr_get(last->states, last->states->count - 1);
      if(last_element->kind != REGEX_KIND_EXACTLY_ONE) {
	fprintf(stderr, "ERROR: Quantifier must follow an unquantified element or group\n");
	return false;
      }
      
      last_element->kind = REGEX_KIND_ZERO_OR_ONE;
      
      i++;
      continue;
    } break;
    case '\\': {
      if(i+1 >= regex_cstr_len) {
	fprintf(stderr, "ERROR: Bad escape character at index: %zd\n", i);
	return false;
      }
      Regex *last = &stack[stack_len-1];

      Regex_State state = {0};
      state.type = REGEX_TYPE_ELEMENT;
      state.recursive = false;
      state.value = regex_cstr[i+1];
      state.kind = REGEX_KIND_EXACTLY_ONE;

      if(!regex_append(last, state)) {
	return false;
      }

      i += 2;
      continue;
    } break;
    case '(': {

      if(stack_len == stack_cap) {
	stack_cap *= 2;
	stack = (Regex  *) realloc(stack, sizeof(Regex) * stack_cap);
	if(!stack) {
	  return false;
	}
      }

      if(!regex_init(&stack[stack_len++])) {
	return false;
      }

      i++;
      continue;
    } break;
    case ')': {
      if(stack_len <= 1) {
	fprintf(stderr, "ERROR: No group to close at index: %zd\n", i);
	return false;
      }
      Regex states = stack[stack_len-- - 1];
      Regex *last = &stack[stack_len-1];

      Regex_State state = {0};
      state.type = REGEX_TYPE_GROUP_ELEMENT;
      state.recursive = true;
      state.states = states;
      state.kind = REGEX_KIND_EXACTLY_ONE;

      if(!regex_append(last, state)) {
	return false;
      }

      i++;
      continue;
    } break;
    default: {
      Regex *last = &stack[stack_len-1];

      Regex_State state = {0};
      state.type = REGEX_TYPE_ELEMENT;
      state.recursive = false;
      state.value = next;      
      state.kind = REGEX_KIND_EXACTLY_ONE;

      if(!regex_append(last, state)) {
	return false;
      }

      i++;
      continue;
    } break;
    }
  }

  if(stack_len != 1) {
    fprintf(stderr, "ERROR: Unmatched groups in regular Expression\n");
    return false;
  }

  *out_regex = stack[0];

  free(stack);
  return true;
}

REGEX_DEF bool regex_match_impl(Regex *regex, const char *cstr, size_t cstr_len, size_t *len);

REGEX_DEF bool regex_state_matches_cstr_at(Regex_State *state,
					   const char *cstr, size_t cstr_len,
					   size_t i, size_t *consumed) {
  if(i >= cstr_len) {
    *consumed = 0;
    return false;
  }

  if(state->type == REGEX_TYPE_ANY) {
    *consumed = 1;
    return true;
  }

  if(state->type == REGEX_TYPE_ELEMENT) {
    bool matched = state->value == cstr[i];
    *consumed = matched ? 1 : 0;
    return matched;
  }

  if(state->type == REGEX_TYPE_GROUP_ELEMENT) {
    return regex_match_impl(&state->states, cstr + i, cstr_len - i, consumed);
  }
 
  fprintf(stderr, "ERROR: Unsupported type\n");
  *consumed = 0;
  return false;
}

REGEX_DEF bool regex_backtrack(Queue *queue, size_t *i, bool *continue_matching, size_t *state_current, Arr *backtrackStack) {
  queue_append_front(queue, state_current);

  bool could_backtrack = false;

  while(backtrackStack->count > 0) {
    Regex_Backtrack *backtrack = (Regex_Backtrack *) arr_pop(backtrackStack);

    if(backtrack->backtrackable) {
      if(backtrack->consumptions->count == 0) {
	queue_append_front(queue, &backtrack->state_index);
	continue;
      }

      size_t *n = (size_t *) arr_pop(backtrack->consumptions);
      *i = *i - *n;
      arr_push(backtrackStack, backtrack);
      could_backtrack = true;
      break;
    }

    queue_append_front(queue, &backtrack->state_index);
    for(size_t j=0;j<backtrack->consumptions->count;j++) {
      size_t *n = (size_t *) arr_get(backtrack->consumptions, j);
      *i = *i - *n;
    }
    
  }

  if(could_backtrack) {
    if(queue->size == 0) {
      continue_matching = false;      
    }
    *state_current = *(size_t *) queue_pop(queue);
  }
  
  return could_backtrack;
}

REGEX_DEF bool regex_match_impl(Regex *regex,
				const char *cstr, size_t cstr_len,
				size_t *len) {
  size_t i = 0;
  size_t consumed;
  size_t zero = 0;

  Arr *backtrackStack = arr_init(sizeof(Regex_Backtrack));
  Queue *queue = queue_init(regex->states->count, sizeof(size_t));
  for(size_t j=0;j<regex->states->count;j++) queue_append(queue, &j);

  size_t state_current = *(size_t *) queue_pop(queue);
  Regex_State *state;

  bool continue_matching = true;

  while(continue_matching) {
    state = (Regex_State *) arr_get(regex->states, state_current);
    //regex_state_print(state, 0);
    
    switch(state->kind) {
    case REGEX_KIND_EXACTLY_ONE: {
      bool matched = regex_state_matches_cstr_at(state, cstr, cstr_len, i, &consumed);

      if(!matched) {

	size_t index_before_backtracking = i;

	if(!regex_backtrack(queue, &i, &continue_matching, &state_current, backtrackStack)) {	  
	  for(size_t j=0;j<backtrackStack->count;j++) {
	    arr_free(((Regex_Backtrack *) arr_get(backtrackStack, j))->consumptions);
	  }
	  arr_free(backtrackStack);
	  queue_free(queue);

	  *len = index_before_backtracking;
	  return false;	  
	}

	continue;
      }      

      Regex_Backtrack backtrack = {0};
      backtrack.backtrackable = false;
      backtrack.state_index = state_current;
      backtrack.consumptions = arr_init(sizeof(size_t));
      arr_push(backtrack.consumptions, &consumed);
      
      arr_push(backtrackStack, &backtrack);

      i += consumed;
      if(queue->size == 0) {
	continue_matching = false;
	continue;
      }
      state_current = *(size_t *) queue_pop(queue);
      continue;
    } break;
    case REGEX_KIND_ZERO_OR_ONE: {
      if(i >= cstr_len) {

	Regex_Backtrack backtrack = {0};
	backtrack.backtrackable = false;
	backtrack.state_index = state_current;
	backtrack.consumptions = arr_init(sizeof(size_t));
	arr_push(backtrack.consumptions, &zero);

	arr_push(backtrackStack, &backtrack);
      
	if(queue->size == 0) {
	  continue_matching = false;
	  continue;
	}
	state_current = *(size_t *) queue_pop(queue);
	continue;
      }

      bool matched = regex_state_matches_cstr_at(state, cstr, cstr_len, i, &consumed);

      Regex_Backtrack backtrack = {0};
      backtrack.backtrackable = matched && consumed > 0;
      backtrack.state_index = state_current;
      backtrack.consumptions = arr_init(sizeof(size_t));
      arr_push(backtrack.consumptions, &consumed);

      arr_push(backtrackStack, &backtrack);
	
      i += consumed;
      if(queue->size == 0) {
	continue_matching = false;
	continue;
      }
      state_current = *(size_t *) queue_pop(queue);
      continue;
    } break;
    case REGEX_KIND_ZERO_OR_MORE: {
      Regex_Backtrack backtrack = {0};
      backtrack.backtrackable = true;
      backtrack.state_index = state_current;
      backtrack.consumptions = arr_init(sizeof(size_t));
      
      while(true) {
	if(i >= cstr_len) {

	  if(backtrack.consumptions->count == 0) {
	    arr_push(backtrack.consumptions, &zero);
	    backtrack.backtrackable = false;
	  }
	  arr_push(backtrackStack, &backtrack);
	  
	  if(queue->size == 0) {
	    continue_matching = false;
	    continue;
	  }
	  state_current = *(size_t *) queue_pop(queue);
	  break;
	}

	bool matched = regex_state_matches_cstr_at(state, cstr, cstr_len, i, &consumed);
	if(!matched || consumed == 0) {

	  if(backtrack.consumptions->count == 0) {
	    arr_push(backtrack.consumptions, &zero);
	    backtrack.backtrackable = false;
	  }
	  arr_push(backtrackStack, &backtrack);
	  
	  if(queue->size == 0) {
	    continue_matching = false;
	    continue;
	  }
	  state_current = *(size_t *) queue_pop(queue);
	  break;
	}

	arr_push(backtrack.consumptions, &consumed);
	i += consumed;
      }
      continue;
    } break;
    default: {
      fprintf(stderr, "ERROR: Unsupported kind\n");
      for(size_t j=0;j<backtrackStack->count;j++) {
	arr_free(((Regex_Backtrack *) arr_get(backtrackStack, j))->consumptions);
      }
      arr_free(backtrackStack);
      queue_free(queue);
      return false;
    } break;
    }
  }

  *len = i;
  for(size_t j=0;j<backtrackStack->count;j++) {
    arr_free(((Regex_Backtrack *) arr_get(backtrackStack, j))->consumptions);
  }
  arr_free(backtrackStack);
  queue_free(queue);
  return true;
}

REGEX_DEF bool regex_match(Regex *regex, const char* cstr, size_t *off, size_t *len) {  
  size_t cstr_len = strlen(cstr);


  for(size_t i=0;i<cstr_len;i++) {
    if(regex_match_impl(regex, cstr + i, cstr_len - i, len)) {
      *off = i;
      return true;
    }
  }
  
  return false;
}

#endif //REGEX_IMPLEMENTATION

#endif //REGEX_H_H_
