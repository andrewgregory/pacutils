/*
 * Copyright 2014-2015 Andrew Gregory <andrew.gregory.8@gmail.com>
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
 *
 * Project URL: http://github.com/andrewgregory/tap.c
 */

#ifndef TAP_C
#define TAP_C

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int _tap_tests_planned = 0;
static int _tap_tests_run = 0;
static int _tap_tests_failed = 0;
static const char *_tap_todo = NULL;

#define _tap_output stdout
#define _tap_failure_output stderr
#define _tap_todo_output stdout

#ifndef TAP_EXIT_SUCCESS
#define TAP_EXIT_SUCCESS EXIT_SUCCESS
#endif

#ifndef TAP_EXIT_FAILURE
#define TAP_EXIT_FAILURE EXIT_FAILURE
#endif

#ifndef TAP_EXIT_ASSERT
#define TAP_EXIT_ASSERT TAP_EXIT_FAILURE
#endif

void tap_plan(int test_count);
void tap_skip_all(const char *reason, ...)
    __attribute__ ((format (printf, 1, 2)));
void tap_done_testing(void);
int tap_finish(void);
void tap_todo(const char *reason);
void tap_skip(int count, const char *reason, ...)
    __attribute__ ((format (printf, 2, 3)));
void tap_bail(const char *reason, ...)
    __attribute__ ((format (printf, 1, 2)));
void tap_diag(const char *message, ...)
    __attribute__ ((format (printf, 1, 2)));
void tap_note(const char *message, ...)
    __attribute__ ((format (printf, 1, 2)));

int tap_get_testcount_planned(void);
int tap_get_testcount_run(void);
int tap_get_testcount_failed(void);
const char *tap_get_todo(void);

#define tap_assert(x) \
    if(!(x)) { tap_bail("ASSERT FAILED: '%s'", #x); exit(TAP_EXIT_ASSERT); }

#define tap_ok(...) _tap_ok(__FILE__, __LINE__, __VA_ARGS__)
#define tap_vok(success, args) _tap_vok(__FILE__, __LINE__, success, args)
#define tap_is_float(...) _tap_is_float(__FILE__, __LINE__, __VA_ARGS__)
#define tap_is_int(...) _tap_is_int(__FILE__, __LINE__, __VA_ARGS__)
#define tap_is_str(...) _tap_is_str(__FILE__, __LINE__, __VA_ARGS__)

int _tap_ok(const char *file, int line, int success, const char *name, ...)
    __attribute__ ((format (printf, 4, 5)));
int _tap_vok(const char *file, int line,
        int success, const char *name, va_list args)
    __attribute__ ((format (printf, 4, 0)));
int _tap_is_float(const char *file, int line,
        double got, double expected, double delta, const char *name, ...)
    __attribute__ ((format (printf, 6, 7)));
int _tap_is_int(const char *file, int line,
        intmax_t got, intmax_t expected, const char *name, ...)
    __attribute__ ((format (printf, 5, 6)));
int _tap_is_str(const char *file, int line,
        const char *got, const char *expected, const char *name, ...)
    __attribute__ ((format (printf, 5, 6)));

#define TAP_VPRINT_MSG(stream, msg) if(msg) { \
        va_list args; \
        va_start(args, msg); \
        fputc(' ', stream); \
        vfprintf(stream, msg, args); \
        va_end(args); \
    }


void tap_plan(int test_count)
{
    _tap_tests_planned = test_count;
    fprintf(_tap_output, "1..%d\n", test_count);
    fflush(_tap_output);
}

void tap_skip_all(const char *reason, ...)
{
    FILE *stream = _tap_output;
    fputs("1..0 # SKIP", _tap_output);
    TAP_VPRINT_MSG(stream, reason);
    fputc('\n', _tap_output);
    fflush(_tap_output);
}

void tap_done_testing(void)
{
    tap_plan(_tap_tests_run);
}

int tap_finish(void)
{
    if(_tap_tests_run != _tap_tests_planned) {
        tap_diag("Looks like you planned %d tests but ran %d.",
                _tap_tests_planned, _tap_tests_run);
    }
    return _tap_tests_run == _tap_tests_planned
        && _tap_tests_failed == 0 ? TAP_EXIT_SUCCESS : TAP_EXIT_FAILURE;
}

int tap_get_testcount_planned(void)
{
    return _tap_tests_planned;
}

int tap_get_testcount_run(void)
{
    return _tap_tests_run;
}

int tap_get_testcount_failed(void)
{
    return _tap_tests_failed;
}

const char *tap_get_todo(void)
{
    return _tap_todo;
}

void tap_todo(const char *reason)
{
    _tap_todo = reason;
}

void tap_skip(int count, const char *reason, ...)
{
    FILE *stream = _tap_output;
    while(count--) {
        fprintf(_tap_output, "ok %d # SKIP", ++_tap_tests_run);
        TAP_VPRINT_MSG(stream, reason);
        fputc('\n', _tap_output);
    }
    fflush(_tap_output);
}

void tap_bail(const char *reason, ...)
{
    FILE *stream = _tap_output;
    fputs("Bail out!", _tap_output);
    TAP_VPRINT_MSG(stream, reason);
    fputc('\n', _tap_output);
    fflush(_tap_output);
}

void tap_diag(const char *message, ...)
{
    FILE *stream = _tap_todo ? _tap_todo_output : _tap_failure_output;

    fputs("#", stream);
    TAP_VPRINT_MSG(stream, message);
    fputc('\n', stream);
    fflush(stream);

}

void tap_note(const char *message, ...)
{
    FILE *stream = _tap_output;
    fputs("#", _tap_output);
    TAP_VPRINT_MSG(stream, message);
    fputc('\n', _tap_output);
    fflush(_tap_output);
}

int _tap_vok(const char *file, int line,
        int success, const char *name, va_list args)
{
    const char *result;
    if(success) {
        result = "ok";
        if(_tap_todo) ++_tap_tests_failed;
    } else {
        result = "not ok";
        if(!_tap_todo) ++_tap_tests_failed;
    }

    fprintf(_tap_output, "%s %d", result, ++_tap_tests_run);

    if(name) {
        fputs(" - ", _tap_output);
        vfprintf(_tap_output, name, args);
    }

    if(_tap_todo) {
        fputs(" # TODO", _tap_output);
        if(*_tap_todo) {
            fputc(' ', _tap_output);
            fputs(_tap_todo, _tap_output);
        }
    }

    fputc('\n', _tap_output);
    fflush(_tap_output);

    if(!success && file) {
        /* TODO add test name if available */
        tap_diag("  Failed%s test at %s line %d.",
                _tap_todo ? " (TODO)" : "", file, line);
    }

    return success;
}

#define TAP_OK(success, name) do { \
        va_list args; \
        va_start(args, name); \
        _tap_vok(file, line, success, name, args); \
        va_end(args); \
    } while(0)

int _tap_ok(const char *file, int line,
        int success, const char *name, ...)
{
    TAP_OK(success, name);
    return success;
}

int _tap_is_float(const char *file, int line,
        double got, double expected, double delta, const char *name, ...)
{
    double diff = (expected > got ? expected - got : got - expected);
    int match = diff < delta;
    TAP_OK(match, name);
    if(!match) {
        tap_diag("         got: '%f'", got);
        tap_diag("    expected: '%f'", expected);
        tap_diag("       delta: '%f'", diff);
        tap_diag("     allowed: '%f'", delta);
    }
    return match;
}

int _tap_is_int(const char *file, int line,
        intmax_t got, intmax_t expected, const char *name, ...)
{
    int match = got == expected;
    TAP_OK(match, name);
    if(!match) {
        tap_diag("         got: '%" PRIdMAX "'", got);
        tap_diag("    expected: '%" PRIdMAX "'", expected);
    }
    return match;
}

int _tap_is_str(const char *file, int line,
        const char *got, const char *expected, const char *name, ...)
{
    int match;
    if(got && expected) {
        match = (strcmp(got, expected) == 0);
    } else {
        match = (got == expected);
    }
    TAP_OK(match, name);
    if(!match) {
        tap_diag("         got: '%s'", got);
        tap_diag("    expected: '%s'", expected);
    }
    return match;
}

#undef TAP_OK
#undef TAP_VPRINT_MSG

#endif /* TAP_C */
