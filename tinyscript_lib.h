#ifndef TINYSCRIPT_LIB_H
#define TINYSCRIPT_LIB_H

#include <stdbool.h>
#include "tinyscript.h"

/* User needs to define these */
void *ts_malloc(Val size);
void ts_free(void *p);

/* Call this to initialize the standard library */
int ts_define_funcs();

/* List type */
typedef struct ts_list {
  Val size;
  uint8_t * data;
  Val capacity;
} ts_list;

ts_list * ts_list_new(Val capacity);
ts_list * ts_list_dup(ts_list * list);
void ts_list_free(ts_list * list);

/* Get element from list end or index, returns -1 when no element available */
Val ts_list_pop(ts_list * list);
Val ts_list_get(ts_list * list, Val idx);

/* Add element or elements to list end or set index. Return success status */
/* The push methods are otherwise identical, but the underscore ones are
convenience methods to push more in a single line */
bool ts_list_push(ts_list * list, Val val);
bool ts_list_push_(ts_list * list, Val val1, Val val2);
bool ts_list_push__(ts_list * list, Val val1, Val val2, Val val3);
bool ts_list_set(ts_list * list, Val idx, Val val);
Val ts_list_size(ts_list * list);

/* Utility functions */
char * ts_list_to_string(const ts_list * list);
ts_list * ts_string_to_list(const char * str);
ts_list * ts_bytes_to_list(const char * str, int num_bytes);

#endif /* TINYSCRIPT_LIB_H */
