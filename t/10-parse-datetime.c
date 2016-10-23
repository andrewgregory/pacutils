#include <time.h>

#include "pacutils/util.h"

#include "tap.h"

#define IS(str, yr, mo, dy, h, m, s) \
	_tap_ok(__FILE__, __LINE__, pu_parse_datetime(str, &t) == &t, "%s - parse", str); \
	_tap_is_int(__FILE__, __LINE__, t.tm_year + 1900, yr, "%s - year", str); \
	_tap_is_int(__FILE__, __LINE__, t.tm_mon + 1, mo, "%s - month", str); \
	_tap_is_int(__FILE__, __LINE__, t.tm_mday, dy, "%s - day", str); \
	_tap_is_int(__FILE__, __LINE__, t.tm_hour, h, "%s - hour", str); \
	_tap_is_int(__FILE__, __LINE__, t.tm_min, m, "%s - minute", str); \
	_tap_is_int(__FILE__, __LINE__, t.tm_sec, s, "%s - second", str); \
	_tap_is_int(__FILE__, __LINE__, t.tm_isdst, -1, "%s - dst", str); \

int main(void) {
	struct tm t;

	tap_plan(80);

	IS("2016-12-31T23:59:59.99-04:00", 2016, 12, 31, 23, 59, 59);
	IS("2016-12-31T23:59:59-04:00", 2016, 12, 31, 23, 59, 59);
	IS("2016-12-31T23:59:59Z", 2016, 12, 31, 23, 59, 59);
	IS("2016-12-31T23:59:59", 2016, 12, 31, 23, 59, 59);
	IS("2016-12-31 23:59:59", 2016, 12, 31, 23, 59, 59);
	IS("2015-11-30 22:58", 2015, 11, 30, 22, 58, 0);
	IS("2014-10-29 21", 2014, 10, 29, 21, 0, 0);
	IS("2013-09-28", 2013, 9, 28, 0, 0, 0);
	IS("2012-09", 2012, 9, 1, 0, 0, 0);
	IS("2011", 2011, 1, 1, 0, 0, 0);

	return tap_finish();
}
