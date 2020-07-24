/* Tinyscript Library
 *
 * Copyright 2020 Daniel Landau
 *
 * +--------------------------------------------------------------------
 * ¦  TERMS OF USE: MIT License
 * +--------------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * +--------------------------------------------------------------------
 */

#include "tinyscript.h"
#include "tinyscript_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

ts_list * ts_list_new(Val capacity) {
  ts_list * list = ts_malloc(sizeof(ts_list));
  list->data = ts_malloc(capacity);
  list->size = 0;
  list->capacity = capacity;
  return list;
}

ts_list * ts_list_dup(ts_list * old) {
  ts_list * list = ts_malloc(sizeof(ts_list));
  list->data = ts_malloc(old->capacity);
  list->size = old->size;
  list->capacity = old->capacity;
  for (Val i = 0; i < old->size; ++i)
    list->data[i] = old->data[i];
  return list;
}

void ts_list_free(ts_list * list) {
  ts_free(list->data);
  ts_free(list);
}

Val ts_list_pop(ts_list * list) {
  if (list->size == 0)
    return -1;
  list->size--;
  return list->data[list->size];
}

bool ts_list_push(ts_list * list, Val val) {
  if (list->size == list->capacity)
    return false;
  list->data[list->size] = (uint8_t)val;
  list->size++;
  return true;
}

bool ts_list_push_(ts_list * list, Val val1, Val val2) {
  bool success;
  success = ts_list_push(list, val1);
  if (success) success = ts_list_push(list, val2);
  return success;
}
bool ts_list_push__(ts_list * list, Val val1, Val val2, Val val3) {
  bool success;
  success = ts_list_push(list, val1);
  if (success) success = ts_list_push(list, val2);
  if (success) success = ts_list_push(list, val3);
  return success;
}

bool ts_list_set(ts_list * list, Val idx, Val val) {
  if (idx < list->capacity) {
    // initialize to 0 everything that is between previous list size and new list end
    if (list->size <= idx) {
      for (Val i = list->size; i < idx; ++i) {
        list->data[i] = 0;
      }
      list->size = idx + 1;
    }
    list->data[idx] = (uint8_t)val;
    return true;
  }
  return false;
}

Val ts_list_get(ts_list * list, Val idx) {
  if (idx < list->size) {
    return list->data[idx];
  }
  return -1;
}

Val ts_list_size(ts_list * list) {
  return list->size;
}

void ts_list_truncate(ts_list * list, Val new_size) {
  if (new_size < list->size && new_size >= 0) list->size = new_size;
}

ts_list * ts_list_expand(ts_list * list, Val new_capacity) {
  if (list->capacity >= new_capacity) {
    return list;
  }
  ts_list * new_list = ts_list_new(new_capacity);
  for (int i = 0; i < list->size; ++i) {
    new_list->data[i] = list->data[i];
  }
  new_list->size = list->size;
  ts_list_free(list);
  return new_list;
}

ts_list * ts_list_cat(ts_list * list_a, ts_list * list_b) {
  Val new_size = list_a->size + list_b->size;
  ts_list *new_list = ts_list_new(new_size);
  for (int i = 0; i < list_a->size; ++i)
    new_list->data[i] = list_a->data[i];
  for (int i = 0; i < list_b->size; ++i)
    new_list->data[list_a->size + i] = list_b->data[i];

  new_list->size = new_size;

  return new_list;
}


ts_list * ts_string_to_list(const char * str) {
  Val len = strlen(str);
  ts_list * list = ts_list_new(len + 1);
  for (Val i = 0; i < len; ++i)
    ts_list_push(list, str[i]);
  ts_list_push(list, '\0');
  return list;
}

ts_list * ts_bytes_to_list(const char * str, int num_bytes) {
  ts_list * list = ts_list_new(num_bytes);
  for (Val i = 0; i < num_bytes; ++i)
    ts_list_push(list, str[i]);
  return list;
}

char * ts_list_to_string(const ts_list *list) {
  char *str = ts_malloc(list->size + 1);
  for (Val i = 0; i < list->size; ++i)
    str[i] = list->data[i];
  str[list->size] = '\0';
  return str;
}

Val ts_not(Val value) {
  return !value;
}

Val ts_bool(Val value) {
  return !!value;
}

int ts_define_funcs() {
  int err = 0;
  err |= TinyScript_Define("not", CFUNC(1), (Val)ts_not);
  err |= TinyScript_Define("bool", CFUNC(1), (Val)ts_bool);

  err |= TinyScript_Define("list_new", CFUNC(1), (Val)ts_list_new);
  err |= TinyScript_Define("list_dup", CFUNC(1), (Val)ts_list_dup);
  err |= TinyScript_Define("list_free", CFUNC(1), (Val)ts_list_free);
  err |= TinyScript_Define("list_pop", CFUNC(1), (Val)ts_list_pop);
  err |= TinyScript_Define("list_get", CFUNC(2), (Val)ts_list_get);
  err |= TinyScript_Define("list_push", CFUNC(2), (Val)ts_list_push);
  err |= TinyScript_Define("list_push_", CFUNC(3), (Val)ts_list_push_);
  err |= TinyScript_Define("list_push__", CFUNC(4), (Val)ts_list_push__);
  err |= TinyScript_Define("list_set", CFUNC(3), (Val)ts_list_set);
  err |= TinyScript_Define("list_size", CFUNC(1), (Val)ts_list_size);
  err |= TinyScript_Define("list_truncate", CFUNC(2), (Val)ts_list_truncate);
  err |= TinyScript_Define("list_expand", CFUNC(2), (Val)ts_list_expand);
  err |= TinyScript_Define("list_cat", CFUNC(2), (Val)ts_list_cat);

  err |= TinyScript_Define("free", CFUNC(1), (Val)ts_free);
  return err;
}
