/*
 * Copyright 2014-2016 Andrew Gregory <andrew.gregory.8@gmail.com>
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

#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include <pacutils.h>

#include "config-defaults.h"

#include "../ext/mini.c/mini.c"

const char *myname = "pacini", *myver = BUILDVER;

mini_t *ini = NULL;
alpm_list_t *directives = NULL;
char sep = '\n', *section_name = NULL, *input_file = NULL;
int section_list = 0, verbose = 0;

void cleanup(void)
{
	free(section_name);
	alpm_list_free(directives);
}

void usage(int ret)
{
	FILE *stream = (ret ? stderr : stdout);
#define hputs(x) fputs(x"\n", stream)
	hputs("pacini - query pacman-style configuration file");
	hputs("usage:  pacini [options] <file> [<directive>...]");
	hputs("        pacini (--section-list|--help|--version)");
	hputs("options:");
	hputs("  --section=<name>  query options for a specific section");
	hputs("  --verbose         always show directive names");
	hputs("  --section-list    list configured sections");
	hputs("  --help            display this help information");
	hputs("  --version         display version information");
#undef hputs
	cleanup();
	exit(ret);
}

void parse_opts(int argc, char **argv)
{
	int c;

	char *short_opts = "";
	struct option long_opts[] = {
		{ "section"      , required_argument , NULL , 's' },
		{ "section-list" , no_argument       , NULL , 'l' },
		{ "verbose"      , no_argument       , NULL , 'v' },
		{ "help"         , no_argument       , NULL , 'h' },
		{ "version"      , no_argument       , NULL , 'V' },
		{ 0, 0, 0, 0 },
	};

	while((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch(c) {
			case 'l':
				section_list = 1;
				break;
			case 's':
				free(section_name);
				section_name = strdup(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				usage(0);
				break;
			case 'V':
				pu_print_version(myname, myver);
				cleanup();
				exit(0);
				break;
			case '?':
				usage(1);
				break;
		}
	}
}

alpm_list_t *find_str(alpm_list_t *haystack, const char *needle)
{
	for(; haystack; haystack = alpm_list_next(haystack)) {
		if(strcasecmp(haystack->data, needle) == 0) {
			return haystack;
		}
	}
	return NULL;
}

void print_section(mini_t *ini)
{
	if(directives) { return; }
	printf("[%s]", ini->section);
	fputc(sep, stdout);
}

void print_option(mini_t *ini)
{
	if(directives && !find_str(directives, ini->key)) { return; }
	if(verbose || !ini->value) { fputs(ini->key, stdout); }
	if(ini->value) {
		if(verbose) fputs(" = ", stdout);
		fputs(ini->value, stdout);
	}
	fputc(sep, stdout);
}

void list_sections(void)
{
	while(mini_next(ini)) {
		if(!ini->key) {
			fputs(ini->section, stdout);
			fputc(sep, stdout);
		}
	}
}

void show_section(void)
{
	int in_section = (section_name[0] == '\0');
	while(mini_next(ini)) {
		if(!ini->key) {
			const char *section = ini->section ? ini->section : "";
			in_section = (strcmp(section, section_name) == 0);
		} else if(in_section) {
			print_option(ini);
		}
	}
}

void show_directives(void)
{
	while(mini_next(ini)) {
		if(!ini->key) { print_section(ini); }
		else          { print_option(ini); }
	}
}

int main(int argc, char **argv)
{
	int ret = 0;

	parse_opts(argc, argv);

	input_file = argv[optind++];

	if(input_file == NULL || strcmp(input_file, "-") == 0) {
		ini = mini_finit(stdin);
		input_file = "<stdin>";
	} else {
		ini = mini_init(input_file);
	}

	if(!ini) {
		ret = 1;
		goto cleanup;
	}

	for(; optind < argc; optind++) {
		directives = alpm_list_add(directives, argv[optind]);
	}

	if(alpm_list_count(directives) != 1) {
		verbose = 1;
	}

	if(section_list) {
		list_sections();
	} else if(section_name) {
		show_section();
	} else {
		show_directives();
	}
	if(!ini->eof) {
		fprintf(stderr, "error reading '%s' (%s)\n", argv[1], strerror(errno));
		return 1;
	}

cleanup:
	cleanup();

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
