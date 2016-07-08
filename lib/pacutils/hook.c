/*
 * Copyright 2012-2016 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <errno.h>
#include <string.h>
#include <glob.h>
#include <sys/utsname.h>

#include "../../ext/mini.c/mini.h"

#include "hook.h"
#include "config-defaults.h"
#include "util.h"


int _pu_hook_isspace(int c)
{
  return strchr(" \f\n\r\t\v", c) ? 1 : 0;
}

void _pu_hook_wordsplit_free(char **ws)
{
  if(ws) {
    char **c;
    for(c = ws; *c; c++) {
      free(*c);
    }
    free(ws);
  }
}

char **_pu_hook_wordsplit(char *str)
{
  char *c = str, *end;
  char **out = NULL, **outsave;
  size_t count = 0;

  if(str == NULL) {
    errno = EINVAL;
    return NULL;
  }

  while(_pu_hook_isspace(*c)) { c++; }
  while(*c) {
    size_t wordlen = 0;

    /* extend our array */
    outsave = out;
    if((out = realloc(out, (count + 1) * sizeof(char*))) == NULL) {
      out = outsave;
      goto error;
    }

    /* calculate word length and check for errors */
    for(end = c; *end && !_pu_hook_isspace(*end); end++) {
      if(*end == '\'' || *end == '"') {
        char quote = *end;
        while(*(++end) && *end != quote) {
          if(*end == '\\' && *(end + 1) == quote) {
            end++;
          }
          wordlen++;
        }
        if(*end != quote) {
          errno = EINVAL;
          goto error;
        }
      } else {
        if(*end == '\\' && (end[1] == '\'' || end[1] == '"')) {
          end++; /* skip the '\\' */
        }
        wordlen++;
      }
    }

    if(wordlen == (size_t) (end - c)) {
      /* no internal quotes or escapes, copy it the easy way */
      if((out[count++] = strndup(c, wordlen)) == NULL) { goto error; }
    } else {
      /* manually copy to remove quotes and escapes */
      char *dest = out[count++] = malloc(wordlen + 1);
      if(dest == NULL) { goto error; }
      while(c < end) {
        if(*c == '\'' || *c == '"') {
          char quote = *c;
          /* we know there must be a matching end quote,
           * no need to check for '\0' */
          for(c++; *c != quote; c++) {
            if(*c == '\\' && *(c + 1) == quote) {
              c++;
            }
            *(dest++) = *c;
          }
          c++;
        } else {
          if(*c == '\\' && (c[1] == '\'' || c[1] == '"')) {
            c++; /* skip the '\\' */
          }
          *(dest++) = *(c++);
        }
      }
      *dest = '\0';
    }

    if(*end == '\0') {
      break;
    } else {
      for(c = end + 1; _pu_hook_isspace(*c); c++);
    }
  }

  outsave = out;
  if((out = realloc(out, (count + 1) * sizeof(char*))) == NULL) {
    out = outsave;
    goto error;
  }

  out[count++] = NULL;

  return out;

error:
  /* can't use wordsplit_free here because NULL has not been appended */
  while(count) {
    free(out[--count]);
  }
  free(out);
  return NULL;
}

int pu_hook_reader_next(pu_hook_reader_t *reader)
{
  mini_t *mini = reader->_mini;
  pu_hook_t *hook = reader->hook;

#define _PU_ERR(r, s) { r->status = s; r->error = 1; return -1; }

  if(mini_next(mini) == NULL) {
    if(mini->eof) {
      reader->eof = 1;
      return -1;
    } else {
      _PU_ERR(reader, PU_HOOK_READER_STATUS_ERROR);
    }
  }

  reader->line = mini->lineno;
  reader->key = mini->key;
  reader->value = mini->value;

  if(!mini->key) {
    if(!(free(reader->section), reader->section = strdup(mini->section))) {
      _PU_ERR(reader, PU_HOOK_READER_STATUS_ERROR);
    }

    if(strcmp(reader->section, "Trigger") == 0) {
      pu_hook_trigger_t *t = calloc(sizeof(pu_hook_trigger_t), 1);
      if(t == NULL || alpm_list_append(&hook->triggers, t) == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_ERROR);
      }
    } else if(strcmp(reader->section, "Action") == 0) {
      /* do nothing */
    } else {
      _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_SECTION);
    }
  } else if(reader->section == NULL){
    _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_OPTION);
  } else if(strcmp(reader->section, "Trigger") == 0) {
    pu_hook_trigger_t *t = alpm_list_last(hook->triggers)->data;

    if(strcmp(reader->key, "Operation") == 0) {
      if(reader->value == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else if(strcmp(reader->value, "Install") == 0) {
        t->op |= PU_HOOK_TRIGGER_OPERATION_INSTALL;
      } else if(strcmp(reader->value, "Upgrade") == 0) {
        t->op |= PU_HOOK_TRIGGER_OPERATION_UPGRADE;
      } else if(strcmp(reader->value, "Remove") == 0) {
        t->op |= PU_HOOK_TRIGGER_OPERATION_REMOVE;
      } else {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      }
    } else if(strcmp(reader->key, "Type") == 0) {
      if(reader->value == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else if(strcmp(reader->value, "Package") == 0) {
        t->type = PU_HOOK_TRIGGER_TYPE_PACKAGE;
      } else if(strcmp(reader->value, "File") == 0) {
        t->type = PU_HOOK_TRIGGER_TYPE_FILE;
      } else {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      }
    } else if(strcmp(reader->key, "Target") == 0) {
      if(reader->value == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else if(!_pu_list_append_str(&t->targets, reader->value)) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_ERROR);
      }
    } else {
      _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_OPTION);
    }
  } else if(strcmp(reader->section, "Action") == 0) {
    if(strcmp(reader->key, "When") == 0) {
      if(reader->value == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else if(strcmp(reader->value, "PreTransaction") == 0) {
        hook->when = ALPM_HOOK_PRE_TRANSACTION;
      } else if(strcmp(reader->value, "PostTransaction") == 0) {
        hook->when = ALPM_HOOK_POST_TRANSACTION;
      } else {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      }
    } else if(strcmp(reader->key, "Description") == 0) {
      if(reader->value == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else if(free(hook->desc), (hook->desc = strdup(reader->value)) == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_ERROR);
      }
    } else if(strcmp(reader->key, "Depends") == 0) {
      if(reader->value == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else if(!_pu_list_append_str(&hook->depends, reader->value)) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_ERROR);
      }
    } else if(strcmp(reader->key, "AbortOnFail") == 0) {
      if(reader->value != NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else {
        hook->abort_on_fail = 1;
      }
    } else if(strcmp(reader->key, "NeedsTargets") == 0) {
      if(reader->value != NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else {
        hook->needs_targets = 1;
      }
    } else if(strcmp(reader->key, "Exec") == 0) {
      if(reader->value == NULL) {
        _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
      } else if((hook->cmd = _pu_hook_wordsplit(reader->value)) == NULL) {
        if(errno == EINVAL) {
          _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_VALUE);
        } else {
          _PU_ERR(reader, PU_HOOK_READER_STATUS_ERROR);
        }
      }
    } else {
      _PU_ERR(reader, PU_HOOK_READER_STATUS_INVALID_OPTION);
    }
  }

#undef _PU_ERR

  return 0;
}

void _pu_hook_trigger_free(pu_hook_trigger_t *trigger)
{
  if(!trigger) { return; }
  FREELIST(trigger->targets);
  free(trigger);
}

void pu_hook_free(pu_hook_t *hook)
{
  if(!hook) { return; }
  free(hook->name);
  free(hook->desc);
  _pu_hook_wordsplit_free(hook->cmd);
  alpm_list_free_inner(hook->triggers,
      (alpm_list_fn_free) _pu_hook_trigger_free);
  alpm_list_free(hook->triggers);
  free(hook);
}

pu_hook_t *pu_hook_new(void)
{
  return calloc(sizeof(pu_hook_t), 1);
}

void pu_hook_reader_free(pu_hook_reader_t *reader)
{
  if(!reader) { return; }
  free(reader->file);
  free(reader->section);
  mini_free(reader->_mini);
  free(reader);
}

pu_hook_reader_t *pu_hook_reader_finit(pu_hook_t *hook, FILE *f)
{
  pu_hook_reader_t *reader = calloc(sizeof(pu_hook_reader_t), 1);
  if(reader == NULL) { return NULL; }
  if((reader->_mini = mini_finit(f)) == NULL) {
    pu_hook_reader_free(reader); return NULL;
  }
  if((reader->file = strdup("<stream>")) == NULL) {
    pu_hook_reader_free(reader); return NULL;
  }
  reader->hook = hook;
  reader->status = PU_HOOK_READER_STATUS_OK;
  return reader;
}

pu_hook_reader_t *pu_hook_reader_init(pu_hook_t *hook, const char *file)
{
  char *bname = strrchr(file, '/');
  pu_hook_reader_t *reader = calloc(sizeof(pu_hook_reader_t), 1);
  if(reader == NULL) { return NULL; }
  if((reader->_mini = mini_init(file)) == NULL) {
    pu_hook_reader_free(reader); return NULL;
  }
  if((reader->file = strdup(file)) == NULL) {
    pu_hook_reader_free(reader); return NULL;
  }
  if((bname = strdup(bname ? bname + 1 : file)) == NULL) {
    pu_hook_reader_free(reader); return NULL;
  }
  hook->name = bname;
  reader->hook = hook;
  reader->status = PU_HOOK_READER_STATUS_OK;
  return reader;
}

/* vim: set ts=2 sw=2 et: */
