/* 
 * nsutils: namespace utilities
 * Copyright (C) 2016  Renzo Davoli, University of Bologna
 * 
 * nslist: list the processes in a namespace
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
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>
#include <getopt.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <catargv.h>
#include <memogetusername.h>
#include <prefix.h>
#include <printflen.h>

#define NAME __FILE__

/* filter for scandir to pick only the /proc/[pid] directories */
static inline int isnumber(const char *s) {
	while (1) {
		if (!isdigit(*s++)) return 0; /* an empty string is *not* a number */
		if (*s == 0) return 1;
	}
}

static int numberfilter(const struct dirent *de) {
	return isnumber(de->d_name);
}

/* scandir qsort comaprison: do not sort */
static int nosort(const struct dirent **a, const struct dirent **b) {
	return -1;
}

struct nsproc {
	int isplaceholder;
	pid_t pid;
	uid_t uid;
	char *cmdline;
	char *comm;
	struct nsproc *next;
};

struct nslist {
	char *nsid;
	struct nsproc *proclist;
	struct nslist *next;
};

#define BUFSIZE 1024
/* get the comm of a process */
static char *dupcomm(pid_t pid) {
	char *out = NULL;
	size_t len = 0;
	FILE *fin, *fout;
	char *path;
	asprintf(&path, "/proc/%d/comm", pid);
	if (path) {
		if ((fin = fopen(path, "r")) != NULL) {
			if ((fout = open_memstream(&out, &len)) != NULL) {
				char buf[BUFSIZE];
				size_t n;
				while ((n = fread(buf, 1, BUFSIZE, fin)) > 0) {
					fwrite(buf, 1, n, fout);
				}
				fclose(fout);
				if (out[len - 1] == '\n') out[len - 1] = 0;
			}
			fclose(fin);
		}
		free(path);
	}
	return out;
}

/* get the command line of a process.
	 read the process status to get the name of kernel threads if cmdline is null */
static char *dupcmdline(pid_t pid, char *comm) {
	char *out = NULL;
	size_t len = 0;
	FILE *fin, *fout;
	char *path;
	asprintf(&path, "/proc/%d/cmdline", pid);
	if (path) {
		if ((fin = fopen(path, "r")) != NULL) {
			if ((fout = open_memstream(&out, &len)) != NULL) {
				char buf[BUFSIZE];
				size_t n;
				while ((n = fread(buf, 1, BUFSIZE, fin)) > 0) {
					int i;
					for (i = 0; i < n; i++)
						if (buf[i] == 0) buf[i] = ' ';
					fwrite(buf, 1, n, fout);
				}
				fclose(fout);
				if (out[len - 1] == ' ') out[len - 1] = 0;
			}
			fclose(fin);
		}
		free(path);
	}
	if (*out == 0) {
		free(out);
		asprintf(&out, "[%s]", comm);
	}
	return out;
}

struct scanfilter {
	uid_t uid;
	uint32_t flags;
};
#define SCANFILTER_INIT {.uid = -1, .flags=0}

/* add a process in a namespace entry */
void nsproc_addproc (struct nsproc **scan, char *nsid, pid_t pid, uid_t uid, struct scanfilter *sf) {
	char *comm = dupcomm(pid);
	char *cmdline = dupcmdline(pid, comm);
	int isplaceholder = cmdline != NULL && strncmp(cmdline, nsid, strlen(nsid)) == 0;
	while (*scan != NULL && ((*scan)->isplaceholder > isplaceholder ||
				((*scan)->isplaceholder == isplaceholder && (*scan)->pid < pid)))
		scan = &((*scan)->next);
	struct nsproc *new = malloc(sizeof(struct nsproc));
	if (new) {
		new->isplaceholder = isplaceholder;
		new->pid = pid;
		new->uid = uid;
		new->cmdline = cmdline;
		new->comm = comm;
		new->next = *scan;
		*scan = new;
	}
}

/* add a process in a the data structure, create a new namespace if*/
void nslist_addproc(struct nslist **scan, char *nsid, pid_t pid, uid_t uid, struct scanfilter *sf) {
	int cmp;
	while (*scan != NULL && (cmp = strcmp((*scan)->nsid, nsid)) < 0)
		scan = &((*scan)->next);
	if (cmp == 0) 
		nsproc_addproc(&((*scan)->proclist), nsid, pid, uid, sf);
	else {
		struct nslist *new = malloc(sizeof(struct nslist));
		new->nsid = strdup(nsid);
		new->proclist = NULL;
		nsproc_addproc(&(new->proclist), nsid, pid, uid, sf);
		new->next = *scan;
		*scan = new;
	}
}

/* scan /proc and create the data structure of namspaces and processes */
struct nslist *scanproc(char *prefix, struct scanfilter *sf) {
	struct dirent **namelist;
	int n;
	struct nslist *nslist=NULL;
	if ((n = scandir("/proc", &namelist, numberfilter, nosort)) > 0) {
		int i;
		for (i = 0; i < n; i++) {
			char path[PATH_MAX];
			char nsid[PATH_MAX];
			struct stat st;
			int len;
			char *typelist[] = {prefix, (char *) 0};
			char **typescan;
			for (typescan = (prefix == NULL) ? nsnames : typelist; *typescan; typescan++) {
				char *type = *typescan; 
				snprintf(path,PATH_MAX,"/proc/%s/ns/%s",namelist[i]->d_name,type);
				if (lstat(path, &st) == 0 &&
						(sf->uid == -1 || sf->uid == st.st_uid)) {
					len = readlink(path, nsid, PATH_MAX);
					if (len > 0) {
						nsid[len] = 0;
						nslist_addproc(&nslist, nsid, atoi(namelist[i]->d_name), st.st_uid, sf);
					}
				}
			}
			free(namelist[i]);
		}
		free(namelist);
	}
	return nslist;
}

/* seach for a namespace id in the data structure */
struct nslist *nstag(struct nslist *head, char *tag) {
	for (; head; head = head->next) {
		struct nsproc *proc;
		if (strcmp(tag, head->nsid) == 0) 
			return head;
		for (proc = head->proclist; proc; proc = proc->next)
			if (proc->isplaceholder) {
				if (strcmp(tag, proc->cmdline + (strlen(head->nsid) + 1)) == 0)
					return head;
			} else
				break;
	}
	return NULL;
}

struct nslist **nstagre(struct nslist *head, regex_t *re) {
	char *out = NULL;
	size_t outlen = 0;
	FILE *fout = open_memstream(&out, &outlen);
	if (fout) {
		for (; head; head = head->next) {
			struct nsproc *proc;
			for (proc = head->proclist; proc; proc = proc->next) {
				if (proc->isplaceholder) {
					if (regexec(re, proc->cmdline + (strlen(head->nsid) + 1), (size_t) 0, NULL, 0) == 0) {
						fwrite(&head, sizeof(struct nslist *), 1, fout);
						break;
					}
				}
			}
		}
		fwrite(&head, sizeof(struct nslist *), 1, fout);
		fclose(fout);
		return (struct nslist **) out;
	}
	return NULL;
}

#define PRINTFLAG_SHOWUSER         0x1
#define PRINTFLAG_PLACEHOLDER_ONLY 0x2
#define PRINTFLAG_SHOWCOMM         0x4
#define PRINTFLAG_PIDONLY          0x8
#define PRINTFLAG_COUNT           0x10
#define PRINTFLAG_NSID_HEADER     0x20
#define PRINTFLAG_TABULAR         0x40
#define PRINTFLAG_NUMERIC_NSID    0x80
#define PRINTFLAG_TAG            0x100
#define PRINTFLAG_TAGONLY        0x200
#define PRINTFLAG_SHORT1        0x1000
#define PRINTFLAG_SHORT2        0x2000
//#define PRINTFLAG_SHORT3        0x4000
//#define PRINTFLAG_SHORT4        0x8000

/* returns s[n] if it is the last char in s, ' ' if the s is shorter, '+' if it is longer. */
char strplus(char *s, int n) {
	size_t last = strlen(s) - 1;
	if (last < n)
		return ' ';
	else if (last == n)
		return s[n];
	else
		return '+';
}

/* nsid or nsid + tag */
char *nsidx(struct nslist *ns, uint32_t flags) {
	if (flags & PRINTFLAG_TAG) { 
		if (ns->proclist && ns->proclist->isplaceholder) {
			if (flags & PRINTFLAG_TAGONLY && !(flags & PRINTFLAG_NUMERIC_NSID)) {
				char *tag = strchr(ns->proclist->cmdline, ' ');
				if (tag)
					return (tag+1);
			}
			return ns->proclist->cmdline;
		}
	}
	return ns->nsid;
}

/* print nsid (numeric if requested) */
void print_nsid(const char *nsid, uint32_t flags, int aligned) {
	if (flags & PRINTFLAG_NUMERIC_NSID) {
		char *begin = strchr(nsid, '[');
		char *end = strchr(nsid, ']');
		if (begin && end) {
			unsigned long long nsidnum = strtoull(begin+1, NULL, 10);
			printflen("%*llu%s",end-begin-1,nsidnum,aligned ? " " : "");
		}
	} else {
	if (aligned) 
		printflen("%*s%s ",nsalign(nsid),"",nsid);
	else
		printflen("%s",nsid);
	}
}

/* print header lines */
void print_header(char *nsid, uint32_t flags){
	if (flags & PRINTFLAG_NSID_HEADER) {
		print_nsid(nsid, flags, 0);
		printflen("\n");
	}
	if (!(flags & PRINTFLAG_SHORT1)) {
		ssize_t nsidlen;
		if (!(flags & PRINTFLAG_NSID_HEADER)) {
			if (flags & PRINTFLAG_NUMERIC_NSID) {
				char *begin = strchr(nsid, '[');
				char *end = strchr(nsid, ']');
				nsidlen = end-begin-1;
			} else
				nsidlen = nsalign(nsid) + strlen(nsid);
			printflen("%*s ",nsidlen,"NAMESPACE");
		}
		printflen("%8s ", "PID");
		if (flags & PRINTFLAG_SHOWUSER) 
			printflen("%-9s ", "USER");
		if (flags & PRINTFLAG_SHOWCOMM)
			printflen("COMMAND\n");
		else
			printflen("CMDLINE\n");
	}
}

/* nslist output for one process */
void print_oneproc(struct nsproc *proc, char *nsid, uint32_t flags) {
	static int count;
	if (proc == NULL) {
		if (count > 0) printflen("\n");
		count = 0;
		return;
	}
	if (count++ == 0)
		print_header(nsid, flags);
	if (flags & PRINTFLAG_SHORT1) {
		if (!(flags & PRINTFLAG_SHORT2))
			printflen("%8d ", proc->pid);
		print_nsid(nsid, flags, 0);
		printflen("\n");
	} else {
		if (!(flags & PRINTFLAG_NSID_HEADER))
			print_nsid(nsid, flags, 1);
		if (!(flags & PRINTFLAG_PLACEHOLDER_ONLY) || proc->isplaceholder) {
			printflen("%8d ", proc->pid);
			if (flags & PRINTFLAG_SHOWUSER) {
				char *user = memogetusername(proc->uid);
				printflen("%-8.8s%c ",user,strplus(user,8));
			}
			if (flags & PRINTFLAG_SHOWCOMM)
			printflen("%s\n",proc->comm);
			else
				printflen("%s\n",proc->cmdline);
		}
	}
}

/* scan the processes of the namespace pointed by 'head' and print_oneproc for each one */ 
int print_ns_proc(struct nslist *head, regex_t *re, pid_t pid, uint32_t flags) {
	int count = 0;
	struct nsproc *proc;
	// list all + PRINTFLAG_SHORT2, print only one line per namespace
	if (flags & PRINTFLAG_SHORT2 && pid < 0 && head->proclist)
		pid = head->proclist->pid;
	if (pid < 0 && !(flags & PRINTFLAG_TABULAR)) 
		flags |= PRINTFLAG_NSID_HEADER;
	else if (!(flags & PRINTFLAG_SHORT1))
		flags &= ~PRINTFLAG_TAG;
	if (flags & PRINTFLAG_NSID_HEADER) 
		print_oneproc(NULL, NULL, flags); //set line count to zero
	for (proc = head->proclist; proc; proc = proc->next) {
		if (pid == -1 || pid == proc->pid) {
			if (regexec(re, proc->comm, (size_t) 0, NULL, 0) == 0) {
				if (!(flags & PRINTFLAG_PLACEHOLDER_ONLY) || proc->isplaceholder) {
					if (!(flags & PRINTFLAG_COUNT)) {
						if (flags & PRINTFLAG_PIDONLY) {
							printf("%d\n",proc->pid);
						} else
							print_oneproc(proc, nsidx(head, flags), flags);
					}
					count++;
				}
			}
		}
	}
	return count;
}

/* scan all the namespace and print what is requested */
int print_ns(struct nslist *head, regex_t *re, char *args, uint32_t flags) {
	pid_t pid = isnumber(args) ? atoi(args) : -1;
	struct nslist *thisns = NULL;
	int count = 0;
	if (pid == -1 && *args) {
		thisns = nstag(head, args);
		if (thisns == NULL) {
			fprintf(stderr,"%s: namespace not found\n",args);
			return 0;
		}
	}
	for (; head; head = head->next) {
		if (thisns == NULL || thisns == head) 
			count += print_ns_proc(head, re, pid, flags);
	}
	return count;
}

void usage_and_exit(char *progname, char *prefix) {
	if (prefix == NULL) 
		prefix = randomprefix();
	fprintf(stderr, "Usage: %s [options] [pid_nsid_tag] [pid_nsid_tag] ... [-- long ns tag]\n"
			"  pid_nsid_tag can be:\n"
			"    * a pid\n" 
			"    * a namespace (ns from now on) id (e.g. %s:[1234567890])\n"
			"    * the tag of a ns holder defined by a nshold(1) command (e.g. %snshold)\n"
			"    or a regular expression of a tag (if -r)\n"
			"\n", progname, prefix, prefix);
	fprintf(stderr, "OPTIONS:\n"
			"  -h | --help          Display this information\n"
			"  -r | --regex         arguments are regular expressions of ns tags\n"
			"  -u | --listuser      show process owners' usernames\n"
			"  -U username |\n"
			"      --user username  only match processes belonging to this username (or uid).\n"
			"  -H | --placeholder   show only placeholder processes\n"
			"  -C | --listcomm      show the command instead of the command line\n"
			"  -p | --pid           return just the list of pids of matching processes\n"
			"  -c | --count         return just the number of matching processes\n"
			"  -T | --tabular       give a tabular output\n"
		  "                      	(without titles and separations between namespaces)\n"
			"  -n | --numeric       list the ns numeric id\n"
			"                       (1234567890 instead of %s[1234567890])\n"
			"  -s                   short form: list only pid and ns id (for scripts)\n"
			"  -ss | -s -s          very short form: list ns id only\n"
			"  --short n            --short n has the same meaning of -s repeated n times\n"
			"  -t | -tag            show a ns tag together with the ns id (when defined)\n"
			"  -tt | -t -t |\n"
			"       --tagonly       show a ns tag instead of the ns id (when defined)\n"
			"  -R regex |\n"
			"       --recomm regex  show only those processes whose command match regex\n"
			"                       (regex is a regular expression)\n"
			"\n", prefix);
	exit (1);
}

static char *short_options = "hU:uHCR:rpctsnT";
static struct option long_options[] = {
	{"help",        no_argument,       0,  'h' },
	{"user",        required_argument, 0,  'U' },
	{"listuser",    no_argument,       0,  'u' },
	{"placeholder", no_argument,       0,  'H' },
	{"listcomm",    no_argument,       0,  'C' },
	{"recomm",      required_argument, 0,  'R' },
	{"regex",       no_argument,       0,  'r' },
	{"pid",         no_argument      , 0,  'p' },
	{"count",       no_argument      , 0,  'c' },
	{"tabular",     no_argument      , 0,  'T' },
	{"numeric",     no_argument      , 0,  'n' },
	{"tag",         no_argument      , 0,  't' },
	{"short",       required_argument, 0,  0x100 + 's' },
	{"tagonly",     no_argument      , 0,  0x100 + 't' },
	{0,         0,                 0,  0 }
};

static regex_t *getrefilter(const char *regex, int cflags) {
	regex_t *preg = malloc(sizeof(regex_t));
	if (preg) {
		int err = regcomp(preg, regex, cflags);
		if (err != 0) {
			size_t errsize = regerror(err, preg, NULL, 0);
			char errbuf[errsize];
			regerror(err, preg, errbuf, errsize);
			fprintf(stderr, "regex error: %s\n", errbuf);
			free(preg);
			preg = NULL;
		}
	}
	return preg;
}

int dashdashargc(char **argv) {
	int i;
	for (i = 1; argv[i]; i++) {
		if (strcmp(argv[i], "--") == 0)
			return i;
	}
	return i;
}

int main(int argc, char *argv[]) {
	char *progname = basename(argv[0]);
	int prefixlen = strlen(progname) - sizeof(NAME) + 3;
	char *prefix = NULL;
	struct nslist *nslist;
	static struct scanfilter scanfilter = SCANFILTER_INIT;
	static uint32_t printflags;
	regex_t *refilter = NULL;
	int refilter_flags = REG_NOSUB;
	int dashdash = dashdashargc(argv);
	char *finalarg = NULL;
	int regex_mode = 0;
	int count = 0;

	if (dashdash < argc) {
		argc = dashdash + 1;
		argv[dashdash] = finalarg = catargv(argv+argc);
		argv[argc] = 0;
	}

	if (prefixlen != 0)
		asprintf(&prefix, "%.*s", prefixlen, progname);

	while (1) {
		int c, option_index = 0;
		c = getopt_long(argc, argv, short_options, long_options, &option_index);
		if (c < 0)
			break;
		switch (c) {
			case 'u':
				printflags |= PRINTFLAG_SHOWUSER;
				break;
			case 'U': 
				if (isnumber(optarg))
					scanfilter.uid = atoi(optarg);
				else {
					struct passwd *pwd = getpwnam(optarg);
					if (pwd == NULL) {
						fprintf(stderr,"User %s not found\n",optarg);
						usage_and_exit(progname, prefix);
					}
					scanfilter.uid = pwd->pw_uid;
				}
				break;
			case 'H':
				printflags |= PRINTFLAG_PLACEHOLDER_ONLY;
				break;
			case 'C':
				printflags |= PRINTFLAG_SHOWCOMM;
				break;
			case 'R':
				if ((refilter = getrefilter(optarg, refilter_flags)) == NULL)
					exit(1);
				break;
			case 'T':
				printflags |= PRINTFLAG_TABULAR;
				break;
			case 'p':
				printflags |= PRINTFLAG_PIDONLY;
				printflags |= PRINTFLAG_TABULAR;
				break;
			case 'c':
				printflags |= PRINTFLAG_COUNT;
				printflags |= PRINTFLAG_TABULAR;
				break;
			case 'n':
				printflags |= PRINTFLAG_NUMERIC_NSID;
				break;
			case 0x100 + 't': 
				printflags |= PRINTFLAG_TAG;
			case 't':
				if (printflags & PRINTFLAG_TAG) printflags |= PRINTFLAG_TAGONLY;
				printflags |= PRINTFLAG_TAG;
				break;
			case 's':
				//if (printflags & PRINTFLAG_SHORT3) printflags |= PRINTFLAG_SHORT4;
				//if (printflags & PRINTFLAG_SHORT2) printflags |= PRINTFLAG_SHORT3;
				if (printflags & PRINTFLAG_SHORT1) printflags |= PRINTFLAG_SHORT2;
				printflags |= PRINTFLAG_SHORT1;
				printflags |= PRINTFLAG_TABULAR;
				break;
			case 0x100 + 's': {
													int n = atoi(optarg);
													switch (n) {
														//case 4: printflags |= PRINTFLAG_SHORT4;
														//case 3: printflags |= PRINTFLAG_SHORT3;
														case 2: printflags |= PRINTFLAG_SHORT2;
														case 1: printflags |= PRINTFLAG_SHORT1;
																		printflags |= PRINTFLAG_TABULAR;
																		break;
														default: 
																		fprintf(stderr,"\nshort level must be 1, 2, 3 or 4\n\n");
																		usage_and_exit(progname, prefix);
													}
												}
												break;
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

	nslist = scanproc(prefix, &scanfilter);

	if (refilter == NULL)
		refilter = getrefilter(".*", 0);

	if (argc == 0 && !finalarg)
		count += print_ns(nslist, refilter, "", printflags);
	else {
		for (; *argv; argv++) {
			if (regex_mode && *argv != finalarg) {
				regex_t *regex = getrefilter(*argv, refilter_flags);
				if (regex) {
					struct nslist **nstags = nstagre(nslist, regex);
					if (nstags) {
						struct nslist **scan;
						for (scan=nstags; *scan; scan++) 
							count += print_ns_proc(*scan, refilter, -1, printflags);
					}
					regfree(regex);
					free(regex);
				}
			} else 
				count += print_ns(nslist, refilter, *argv, printflags);
		}
	}
	if (printflags & PRINTFLAG_COUNT)
		printf("%d\n", count);
	return 0;
}
