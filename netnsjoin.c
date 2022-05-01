/*
 * nsutils: namespace utilities
 * Copyright (C) 2016  Renzo Davoli, University of Bologna
 *
 * netnsjoin: join a network namespace.
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
#include <fcntl.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/capability.h>

#include <nssearch.h>
#include <catargv.h>

void usage_and_exit(char *progname) {
	fprintf(stderr, "Usage:\n"
			"%s pid_or_net_namespace_id [command [args]]\n"
			"%s pid or net namespace id -- [command [args]]\n\n", progname, progname);
	exit(1);
}

/* : if there is a -- argv:
	 : * change it into 0
	 : * return the index of the first argv after -- (the command to launch)
	 : otherwise:
	 : return 2 (i.e. command to launch) */
int cmdoptind(char **argv) {
	int i;
	for (i = 1; argv[i]; i++) {
		if (strcmp(argv[i], "--") == 0) {
			argv[i] = 0;
			return i + 1;
		}
	}
	return 2;
}

/* turn capabilities on and off to have "extra" powers only when needed */
int raise_effective_cap(cap_value_t cap) {
	cap_t caps=cap_get_proc();
	cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, CAP_SET);
	return cap_set_proc(caps);
}

int lower_effective_cap(cap_value_t cap) {
	cap_t caps=cap_get_proc();
	cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, CAP_CLEAR);
	return cap_set_proc(caps);
}

cap_flag_value_t get_effective_cap(cap_value_t cap) {
	cap_flag_value_t rval=0;
	cap_t caps=cap_get_proc();
	cap_get_flag(caps, cap, CAP_EFFECTIVE, &rval);
	return rval;
}

cap_flag_value_t get_inheritable_cap(cap_value_t cap) {
	cap_flag_value_t rval=0;
	cap_t caps=cap_get_proc();
	cap_get_flag(caps, cap, CAP_INHERITABLE, &rval);
	return rval;
}

int clear_inheritable_cap(void) {
	cap_t caps=cap_get_proc();
	cap_clear_flag(caps, CAP_INHERITABLE);
	return cap_set_proc(caps);
}

#define PATH1 "/proc/1/ns/net"

int main(int argc, char *argv[])
{
	pid_t pid;
	char *ns_path;
	int nsfd;
	char *nsname;
	int cmd;
	char *argvsh[]={getenv("SHELL"),NULL};
	char **cmdargv;
	char ns1[PATH_MAX+1];
	char nsid[PATH_MAX+1];

	if (argc < 2) {
		usage_and_exit(basename(argv[0]));
	}

	if (geteuid() != 0 &&
			get_effective_cap(CAP_NET_ADMIN) != CAP_SET &&
			get_inheritable_cap(CAP_NET_ADMIN) != CAP_SET) {
		fprintf(stderr,"root or cap_net_admin required: permission denied\n");
		exit(1);
	}

	/* if there is -- in the command line the name of the namespace
		 consists of several args, cat them together */
 	if ((cmd = cmdoptind(argv)) == 2)
		nsname = argv[1];
	else
		nsname = catargv(argv + 1);

	if (cmd < argc)
		cmdargv = argv + cmd;
	else {
		cmdargv = argvsh;
		if (cmdargv[0] == NULL) {
			fprintf(stderr, "Error: $SHELL env variable not set.\n");
			exit(1);
		}
	}

	if ((pid = nssearchone("net", nsname)) <= 0) {
		if (pid == 0)
			fprintf(stderr,"%s: namespace not found\n", nsname);
		else
			fprintf(stderr,"%s: too many placeholders\n", nsname);
		exit(1);
	}

	asprintf(&ns_path, "/proc/%d/ns/net",pid);

	if (readlink(ns_path, nsid, PATH_MAX+1) < 0) {
		perror(ns_path);
		exit(1);
	}

	if (geteuid() != 0) {
		if (raise_effective_cap(CAP_SYS_PTRACE)) {
			perror("raise CAP_SYS_PTRACE");
			exit(1);
		}
		if (readlink(PATH1, ns1, PATH_MAX+1) < 0) {
			perror("readlink " PATH1);
			exit(1);
		}
		if (lower_effective_cap(CAP_SYS_PTRACE)) {
			perror("lower CAP_SYS_PTRACE");
			exit(1);
		}
		if (geteuid() != 0 && strcmp(ns1,nsid)==0) {
			fprintf(stderr,"%s == " PATH1 ": permission denied\n",ns_path);
			exit(1);
		}
	}

	if (raise_effective_cap(CAP_SYS_ADMIN)) {
		perror("raise CAP_SYS_ADMIN");
		exit(1);
	}
	if ((nsfd = open(ns_path, O_RDONLY)) >= 0) {
		struct stat st;
		if (lstat(ns_path, &st) < 0) {
			perror(ns_path);
			goto close_abort;
		}
		if (geteuid() != 0 && st.st_uid != geteuid()) {
			fprintf (stderr, "cannot join netns %s: permission denied\n",ns_path);
			goto close_abort;
		}
		if (setns(nsfd, CLONE_NEWNET) != 0) {
			perror("setns");
			goto close_abort;
		}
		close(nsfd);
		if (unshare(CLONE_NEWNS) < 0) {
			perror("unshare");
			goto abort;
		}
	} else {
		fprintf (stderr, "open failed %s: %s\n", ns_path, strerror(errno));
		goto abort;
	}
	if (lower_effective_cap(CAP_SYS_ADMIN)) {
		perror("lower CAP_SYS_ADMIN");
		exit(1);
	}
	if (clear_inheritable_cap()) {
		perror("clear_inheritable_cap");
		exit(1);
	}
	fprintf(stderr, "Joining net namespace %s\n",nsid);
	execvp(cmdargv[0], cmdargv);

	perror(cmdargv[0]);
	exit(0);

close_abort:
	close(nsfd);
abort:
	exit(1);
}
