/* 
 * nsutils: namespace utilities
 * Copyright (C) 2016  Renzo Davoli, University of Bologna
 * 
 * nssearch: search a namespace by pid, identifier or name-tag
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
#include <ctype.h>
#include <dirent.h>
#include <regex.h>

#include <nssearch.h>
#include <prefix.h>

#define MAX_NSHOLD_CMDLINE_LEN 1024

static inline int isnumber(const char *s) {
	while (1) {
		if (!isdigit(*s++)) return 0;
		if (*s == 0) return 1;
	}
}

/* is cmd a namespaceid for type? (or for any type if type == NULL)? */
static inline int isnsid(const char *cmd, const char *type) {
	if (type == NULL)
		return guessprefix(cmd) != NULL;
	else
		return checkprefix(cmd, type);
}

/* read cmdline and store it in linebuf */
static void readcmdline(const char *pidstr, char linebuf[MAX_NSHOLD_CMDLINE_LEN]) {
	char *path;
	linebuf[0]=0;
	/* get the commandline: build the path, open the file */
	if (asprintf(&path, "/proc/%s/cmdline", pidstr) >= 0) {
		FILE *cmdline = fopen(path, "r");
		if (cmdline) {
			/* read the commandline anche check if this is a placeholder */
			fgets(linebuf, MAX_NSHOLD_CMDLINE_LEN, cmdline);
			fclose(cmdline);
		}
		free(path);
	}
}

static int ckcmdline_pid(const char *pidstr, const char *type) {
	char linebuf[MAX_NSHOLD_CMDLINE_LEN];
	readcmdline(pidstr, linebuf);
	return isnsid(linebuf, type);
}

/* check if the pid is a specific namespace placeholder */
/* name is "" when re != NULL */
static int ckcmdline(const char *pidstr, const char *name, const char *type) {
	char linebuf[MAX_NSHOLD_CMDLINE_LEN];
	readcmdline(pidstr, linebuf);
	if (isnsid(linebuf, type)) {
		char *tail = strchr(linebuf, ' ');
		if (tail) {
			/* this holder has a tag. Check if 'name' matches the namespace id or the holder tag */
			return strncmp(name, linebuf, tail-linebuf) == 0 || strcmp(tail+1, name) == 0;
		} else
			/* this holder has no tag. Check if 'name' matches the entire cmdline */
			return strcmp(name, linebuf) == 0;
	}
	return 0;
}

static int ckcmdline_re(const char *pidstr, regex_t *re, const char *type) {
	char linebuf[MAX_NSHOLD_CMDLINE_LEN];
	readcmdline(pidstr, linebuf);
	if (isnsid(linebuf, type)) {
		char *tail = strchr(linebuf, ' ');
		if (tail)
			return regexec(re, tail+1,  0, NULL, 0) == 0;
	}
	return 0;
}

/* if nssearchone: check if this process (not a placeholder)
	 is running in the desired namespace */
static int ckns(const char *pidstr, const char *name, const char *type) {
	char *path;
	int rval = 0;
	if (getpid() != atoi(pidstr) && // for some reason the nsjoin fails on the process itself
			asprintf(&path, "/proc/%s/ns/%s", pidstr, type) >= 0) {
		char nsid[PATH_MAX+1];
		int len;
		if ((len = readlink(path, nsid, PATH_MAX+1)) > 0) {
			nsid[len] = 0;
			rval = strcmp(name, nsid) == 0;
		}
		free(path);
	}
	return rval;
}

static int numberfilter(const struct dirent *de) {
	return isnumber(de->d_name);
}

static int nosort(const struct dirent **a, const struct dirent **b) {
	return -1;
}

/* search a pid of a placeholder */
/* type is the type of namespace e.g. "net", "ipc", "pid", "user" etc
	 name can be:
	 * a numeric string (a pid) e.g. "4242"
	 * a namespace id e.g. "net:[1234567890]"
	 * a tag of a namespace holder
	 */
/* return 0 if not found, -1 if too many, > 0 in case of a successful search */
pid_t nssearch(char *type, char *name) {
	pid_t rval = 0;
	if (isnumber(name) && ckcmdline_pid(name, type)) /* PID: ok if it is a placeholder */
		return atoi(name);
	else {
		struct dirent **namelist;
		int n;

		if ((n = scandir("/proc", &namelist, numberfilter, nosort)) > 0) {
			int i;
			for (i = 0; i < n; i++) {
				if (ckcmdline(namelist[i]->d_name, name, type)) {
					rval = rval > 0 ? -1 : atoi(namelist[i]->d_name);
					if (rval < 0) break; /* the first placeholder of a nsid fits */
				}
			}
			for (i = 0; i < n; i++) 
				free(namelist[n]);
			free(namelist);
		}
	}
	return rval;
}

pid_t *nssearchre(char *type, regex_t *re) {
	struct dirent **namelist;
	int n;
	char *out = NULL;
	size_t outlen = 0;
	pid_t pid;
	FILE *fout = open_memstream(&out, &outlen);
	if (fout) {
		if ((n = scandir("/proc", &namelist, numberfilter, nosort)) > 0) {
			int i;
			for (i = 0; i < n; i++) {
				if (ckcmdline_re(namelist[i]->d_name, re, type)) {
					pid = atoi(namelist[i]->d_name);
					fwrite(&pid, sizeof(pid), 1, fout);
				}
				free(namelist[n]);
			}
			free(namelist);
		}
		pid = 0;
		fwrite(&pid, sizeof(pid), 1, fout);
		fclose(fout);
		return (pid_t *) out;
	} else
		return NULL;
}

/* search a pid of a placeholder or any pid running in the desired namespace */
/* type is the type of namespace e.g. "net", "ipc", "pid", "user" etc
	 name can be:
	 * a numeric string (a pid) e.g. "4242"
	 * a namespace id e.g. "net:[1234567890]"
	 * a tag of a namespace holder
	 */
/* return 0 if not found, -1 if too many, > 0 in case of a successful search */
pid_t nssearchone(char *type, char *name) {
	if (isnumber(name))
		return atoi(name);
	else {
		pid_t rval = 0;
		struct dirent **namelist;
		int isnsid = checkprefix(name, type);
		int n;

		if ((n = scandir("/proc", &namelist, numberfilter, nosort)) > 0) {
			int i;
			for (i = 0; i < n; i++) {
				if (ckcmdline(namelist[i]->d_name, name, type)) {
					rval = rval > 0 ? -1 : atoi(namelist[i]->d_name);
					if (isnsid || rval < 0) break; /* the first placeholder of a nsid fits */
				}
			}
			if (isnsid && rval == 0) {
				for (i = 0; i < n; i++) {
					if (rval < 0 && ckns(namelist[i]->d_name, name, type)) {
						rval = atoi(namelist[i]->d_name);
						break;
					}
				}
			}
			for (i = 0; i < n; i++)
				free(namelist[n]);
			free(namelist);
		}
	  return rval;
	}
}

