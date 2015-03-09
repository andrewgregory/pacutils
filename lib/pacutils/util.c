/*
 * Copyright 2012-2015 Andrew Gregory <andrew.gregory.8@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <string.h>

#include "util.h"

char *pu_basename(char *path)
{
  char *c;
  if(!path) { return NULL; }
  for(c = path + strlen(path); c > path && *(c - 1) != '/'; --c);
  return c;
}

char *pu_hr_size(off_t bytes, char *dest)
{
  static char *suff[] = {"B", "K", "M", "G", "T", "P", "E", NULL};
  float hrsize;
  int s = 0;
  while((bytes >= 1000000 || bytes <= -1000000) && suff[s + 1]) {
    bytes /= 1024;
    ++s;
  }
  hrsize = bytes;
  if((hrsize >= 1000 || hrsize <= -1000) && suff[s + 1]) {
    hrsize /= 1024;
    ++s;
  }
  sprintf(dest, "%.2f %s", hrsize, suff[s]);
  return dest;
}

alpm_list_t *_pu_list_append(alpm_list_t **list, void *data)
{
  alpm_list_t *l = *list, *new = malloc(sizeof(alpm_list_t));
  if(new == NULL) { return NULL; }

  new->next = NULL;
  new->data = data;

  if(l == NULL) {
    l = *list = new;
    new->prev = new;
  } else {
    l->prev->next = new;
    new->prev = l->prev;
    l->prev = new;
  }

  return new;
}

void *_pu_list_shift(alpm_list_t **list)
{
  alpm_list_t *l = *list;
  void *data;
  if(l == NULL) { return NULL; }
  data = l->data;
  if(l->next) { l->next->prev = l->prev; }
  *list = l->next;
  free(l);
  return data;
}

/* vim: set ts=2 sw=2 et: */
