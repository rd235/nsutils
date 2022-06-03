/* 
 * nsutils: namespace utilities
 * Copyright (C) 2016  Renzo Davoli, University of Bologna
 * 
 * prefix: management of ns prefix.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prefix.h>
#include <time.h>

char *nsnames[] = { "cgroup", "ipc", "mnt", "net", "pid", "user", "time", "uts", (char *)0}; 
size_t nsnames_maxlen = 6;

void prefixerror_and_exit(char *progname) {
	char **ns;
	fprintf(stderr, "use a prefix to specify the type of namespace:\n   ");
	for (ns = nsnames; *ns; ns++)
		fprintf(stderr, " %s%s",*ns,progname);
	fprintf(stderr, "\n");
	exit(2);
}

#define SUFFIX ":["
int checkprefix(const char *str, const char *type) {
	if (type == NULL)
		return 0;
	ssize_t typelen=strlen(type);
	return strncmp(str, type, typelen) == 0 &&
		strncmp(str+typelen, SUFFIX, sizeof(SUFFIX)-1) == 0;
}

char *guessprefix(const char *str) {
	if (str) {
		char **ns;
		for (ns = nsnames; *ns; ns++) 
			if (checkprefix(str, *ns))
				return *ns;
	}
	return NULL;
}

char *randomprefix(void) {
	srand(time(NULL));
	return nsnames[rand() % (sizeof(nsnames)/sizeof(char *) - 1)];
}
