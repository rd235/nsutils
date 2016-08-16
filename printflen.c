#define _GNU_SOURCE         
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <printflen.h>


/* get the current width of the terminal line.
	 returns -1 is the output is not a terminal.
	 it memoizes the result */

static int getlinewidth(void) {
	static int linewidth;
	if (__builtin_expect((linewidth == 0), 0)) {
		if (isatty(STDOUT_FILENO)) {
			struct winsize w;
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

			linewidth = w.ws_col;
		} else
			linewidth = -1;
	}
	return linewidth;
}

/* printf cutting the line to the terminal width (only if the STDOUT is a terminal */
int printflen(const char *format, ...) {
	va_list ap;
	int linewidth = getlinewidth();
	int rval = -1;
	va_start(ap, format);
	if (linewidth < 0)
		rval = vprintf(format, ap);
	else {
		static char *linebuf;
		static int pos = 0;
		char *printfout;
		int i;
		if (__builtin_expect(linebuf == NULL, 0))
			linebuf = malloc(linewidth+1);
		if (__builtin_expect(linebuf == NULL, 0))
			goto error;
		rval = vasprintf(&printfout, format, ap);
		if (__builtin_expect(rval < 0, 0))
			goto error;
		for (i = 0; i < rval; i++) {
			if (printfout[i] == '\n') {
				linebuf[pos]=0;
				printf("%s\n",linebuf);
				pos=0;
			} else {
				if (pos < linewidth)
					linebuf[pos++] = printfout[i];
				else
					linebuf[pos - 1] = '+';
			}
		}
		free(printfout);
	}
error:
	va_end(ap);
	return rval;
}

