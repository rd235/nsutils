/* 
 * nsutils: namespace utilities
 * Copyright (C) 2016  Renzo Davoli, University of Bologna
 * 
 * memogetpwuid: memoize version of getpwuid
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
#include <pwd.h>
#include <sys/types.h>

struct memopw {
	uid_t   pw_uid;        /* user ID */
	char   *pw_name;       /* username */
	struct memopw *next;
};

static struct memopw *memohead;

static char *memogetpw(struct memopw **scan, uid_t pw_uid) {
	while (*scan != NULL && (*scan)->pw_uid < pw_uid)
		scan = &((*scan)->next);	
	if (*scan != NULL  && (*scan)->pw_uid == pw_uid)
		return (*scan)->pw_name;
	else {
		struct passwd *pwd = getpwuid(pw_uid);
		struct memopw *new = malloc(sizeof(struct memopw));
		if (new) {
			new->pw_uid = pw_uid;
			if (pwd)
				new->pw_name = strdup(pwd->pw_name);
			else
				asprintf(&new->pw_name,"%d",pw_uid);
			new->next = *scan;
			*scan = new;
			return new->pw_name;
		} else
			return pwd ? pwd->pw_name : "";
	}
}

/* return the username from the uid. Memoize the results */
char *memogetusername(uid_t pw_uid) {
	return memogetpw(&memohead, pw_uid);
}

#if 0
#include <stdio.h>
int main(int argc, char *argv[]) {
	while (1) {
		uid_t uid;
		scanf("%d", &uid);
		printf("%d %s\n",uid,memogetusername(uid));
	}
}
#endif
