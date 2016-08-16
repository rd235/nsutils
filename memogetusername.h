#ifndef MEMOGETPWUID_C
#define MEMOGETPWUID_C
#define _GNU_SOURCE
#include <pwd.h>

char *memogetusername(uid_t pw_uid); 

#endif // MEMOGETPWUID_C
