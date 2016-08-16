/* 
 * nsutils: namespace utilities
 * Copyright (C) 2016  Renzo Davoli, University of Bologna
 * 
 * nsrelease: release a "keepalive" process
 *
 * Cado is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>. 
 *  
 */
    
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <getopt.h>
#include <regex.h>
#include <sys/types.h>

#include <nssearch.h>
#include <catargv.h>
#include <prefix.h>

#define NAME __FILE__

static char *short_options = "hr";
static struct option long_options[] = {
	{"help",        no_argument,       0,  'h' },
	{"regex",       no_argument,       0,  'r' },
	{0,         0,                 0,  0 }
};

void usage_and_exit(char *progname, char *prefix) {
	if (prefix == NULL) 
		prefix = randomprefix();
	fprintf(stderr, "Usage:\n"
			"  %s [-r | --regex] placeholder [placeholder] [-- long placeholder tag]\n"
			"  placeholder can be dentified by (if not -r):\n"
			"    * a pid\n"
			"    * a namespace id (e.g. %s:[1234567890])\n"
			"    * the tag of a namespace holder defined by some nshold(1) command (e.g. %snshold)\n"
			"    or a regular expression of a tag (if -r)\n"
			"\n", progname, prefix, prefix);
	exit (1);
}

int dashdashargc(char **argv) {
	int i;
	for (i = 1; argv[i]; i++) {
		if (strcmp(argv[i], "--") == 0)
			return i;
	}
	return i;
}

/* kill placeholder processes */
int main(int argc, char *argv[])
{
	char *progname = basename(argv[0]);
	int prefixlen = strlen(progname) - sizeof(NAME) + 3;
	char *prefix = NULL;
	int regex_mode = 0;
	int dashdash = dashdashargc(argv);
	char *finalarg = NULL;
	int count = 0;
	int reflags = 0;

	if (dashdash < argc) {
		argc = dashdash + 1;
		argv[dashdash] = finalarg = catargv(argv+argc);
		argv[argc] = 0;
	}

	if (prefixlen != 0)
		asprintf(&prefix, "%*.*s", prefixlen, prefixlen, progname);

	while (1) {
		int c, option_index = 0;
		c = getopt_long(argc, argv, short_options, long_options, &option_index);
		if (c < 0)
			break;
		switch (c) {
			case 'r':
				regex_mode = 1;
				break;
			case 'h':
			default:
				usage_and_exit(progname, prefix);
				break;
		}
	}

	argc -= optind;
	argv += optind;

	for (; *argv; argv++) {
		if (regex_mode && *argv != finalarg) {
			regex_t re;
			int err = regcomp(&re, *argv, reflags);
			if (err != 0) {
				size_t errsize = regerror(err, &re, NULL, 0);
				char errbuf[errsize];
				regerror(err, &re, errbuf, errsize);
				fprintf(stderr, "%s: regex error: %s\n", *argv, errbuf);
			} else {
				pid_t *pids = nssearchre(prefix, &re);
				if (*pids) {
					pid_t *scan;
					for (scan = pids; *scan; scan ++) {
						kill(*scan, SIGTERM);
						count++;
					}
				} else
					fprintf(stderr,"no namespace placeholder matches \"%s\"\n", *argv);
				free(pids);
				regfree(&re);
			}
		} else {
			pid_t pid;
			if ((pid = nssearch(prefix ? prefix : guessprefix(*argv), *argv)) <= 0) {
				if (pid == 0)
					fprintf(stderr,"%s: namespace placeholder not found\n", *argv);
				else
					fprintf(stderr,"%s: too many namespace placeholders match\n", *argv);
			} else {
				kill(pid, SIGTERM);
				count ++;
			}
		}
	}
	return count == 0 ? 1 : 0;
}
