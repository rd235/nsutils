#ifndef PREFIXERROR_H
#define PREFIXERROR_H

extern char *nsnames[];
extern size_t nsnames_maxlen;

void prefixerror_and_exit(char *progname);

int checkprefix(const char *str, const char *type);

char *guessprefix(const char *str);

static inline ssize_t nsalign(const char *s) {
	char *col = strchr(s,':');
	return col ? nsnames_maxlen - (col - s) : 0;
}

char *randomprefix(void);
#endif // PREFIXERROR_H
