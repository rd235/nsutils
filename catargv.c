/* 
 * nsutils: namespace utilities
 * Copyright (C) 2016  Renzo Davoli, University of Bologna
 * 
 * catargv: create a malloc-ed copy of all the argv arguments as a string
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

/* join all the args in a single string */
char *catargv(char *argv[]) {
	char *out = NULL;
	size_t len = 0;
	FILE *fout = open_memstream(&out, &len);
	if (fout) {
		while (*argv) {
			fprintf(fout, "%s",*argv++);
			if (*argv) fprintf(fout," ");
		}
		fclose(fout);
	}
	return out;
}
