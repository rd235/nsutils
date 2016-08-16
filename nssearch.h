#ifndef NSSEARCH_H
#define NSSEARCH_H
#include <unistd.h>
#include <regex.h>

pid_t nssearch(char *type, char *name);

pid_t *nssearchre(char *type, regex_t *re);

pid_t nssearchone(char *type, char *name);

#endif
