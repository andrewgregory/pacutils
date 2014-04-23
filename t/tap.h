#include <math.h>

int _tap_tests_run = 0;
int _tap_tests_failed = 0;

const char *_tap_todo = NULL;

void tap_todo(const char *reason)
{
    _tap_todo = reason;
}

void tap_ok(int success, const char *description)
{
	const char *result;
	if(success) {
		result = "ok";
        if(_tap_todo) ++_tap_tests_failed;
	} else {
		result = "not ok";
        if(!_tap_todo) ++_tap_tests_failed;
	}

	if(description) {
		printf("%s %d - %s", result, ++_tap_tests_run, description);
	} else {
        printf("%s %d", result, ++_tap_tests_run);
    }

    if(_tap_todo) {
        fputs(" # TODO", stdout);
        if(*_tap_todo) {
            fputc(' ', stdout);
            fputs(_tap_todo, stdout);
        }
    }

    fputc('\n', stdout);
}

void tap_bail(const char *format, ...)
{
	va_list args;
    fputs("Bail out! ", stdout);
	va_start(args, format);
    vprintf(format, args);
	va_end(args);
    fputc('\n', stdout);
}

void tap_diag(const char *format, ...)
{
	va_list args;
    fputs("    # ", stdout);
	va_start(args, format);
    vprintf(format, args);
	va_end(args);
    fputc('\n', stdout);
}

void tap_is_float(float got, float expected, float delta,
        const char *description)
{
    float diff = fabs(expected - got);
    int match = diff < delta;
	tap_ok(match, description);
	if(!match) {
		tap_diag("expected '%f'", expected);
		tap_diag("got      '%f'", got);
        tap_diag("delta    '%f'", diff);
        tap_diag("allowed  '%f'", delta);
	}
}

void tap_is_int(int got, int expected, const char *description)
{
	int match = got == expected;
	tap_ok(match, description);
	if(!match) {
		tap_diag("expected '%d'", expected);
		tap_diag("got      '%d'", got);
	}
}

void tap_is_str(const char *got, const char *expected, const char *description)
{
	int match;
    if(got && expected) {
        match = (strcmp(got, expected) == 0);
    } else {
        match = (got == expected);
    }
	tap_ok(match, description);
	if(!match) {
		tap_diag("expected '%s'", expected);
		tap_diag("got      '%s'", got);
	}
}

void tap_plan(int test_count)
{
	printf("1..%d\n", test_count);
}

void tap_done_testing(void)
{
	tap_plan(_tap_tests_run);
}

int tap_tests_run(void)
{
    return _tap_tests_run;
}

int tap_tests_failed(void)
{
    return _tap_tests_failed;
}
