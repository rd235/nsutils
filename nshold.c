/*
 * nsutils: namespace utilities
 * Copyright (C) 2016  Renzo Davoli, University of Bologna
 *
 * nshold: keep-alive and give a name tag to a namespace
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
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/capability.h>

#include <catargv.h>
#include <prefix.h>

#define NAME __FILE__
#define NSPATH "/proc/self/ns/"

int clear_all_caps(void) {
	cap_t caps=cap_get_proc();
	cap_clear_flag(caps, CAP_EFFECTIVE);
	cap_clear_flag(caps, CAP_INHERITABLE);
	cap_clear_flag(caps, CAP_PERMITTED);
	return cap_set_proc(caps);
}

/* read a symlink and return a strdup of the link target */
static char *readlinkdup(char *pathname) {
	char buf[PATH_MAX + 1];
	ssize_t len = readlink(pathname, buf, PATH_MAX);
	if (len < 0)
		return NULL;
	buf[len] = 0;
	return strdup(buf);
}

int dashdashargc(char **argv) {
	int i;
	for (i = 1; argv[i]; i++) {
		if (strcmp(argv[i], "--") == 0)
			return i;
	}
	return i;
}

void usage_and_exit(char *progname, char *prefix) {
	fprintf(stderr, "Usage:\n"
			"\t%s [placeholder_tag]\n"
			"or\n"
			"\t%s -- long placeholder tag\n\n", progname, progname);
	exit (1);
}

/* create a placeholder process for the namespace */
int main(int argc, char **argv, char **envp)
{
	if (guessprefix(argv[0]) == NULL) {
		char *progname = basename(argv[0]);
		int prefixlen = strlen(progname) - sizeof(NAME) + 3;
		char *prefix = NULL;
		char *nspath;
		char *nsname;
		int dashdash = dashdashargc(argv);
		char *tag = NULL;
		if (prefixlen == 0)
			prefixerror_and_exit(progname);
		else
			asprintf(&prefix, "%.*s", prefixlen, progname);

		if (dashdash < argc)
			tag = catargv(argv+(dashdash + 1));
		else if (argc == 2)
			tag = argv[1];
		else if (argc != 1)
			usage_and_exit(progname, prefix);

		clear_all_caps();

		asprintf(&nspath, "%s%.*s", NSPATH, prefixlen, basename(argv[0]));
		if ((nsname = readlinkdup(nspath)) == NULL)
			return 1;

		static char *newargv[2];
		if (tag && *tag)
			asprintf(&newargv[0],"%s %s", nsname, tag);
		else
			asprintf(&newargv[0],"%s", nsname);

		daemon(0,0);
		execve("/proc/self/exe", newargv, envp);
		return 1;

	} else {
		while (1)
			pause();
		return 0;
	}
}
