/*                    P L O T 3 - H P G L . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2013 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file util/plot3-hpgl.c
 *
 * Description -
 * Convert a unix-plot file to hpgl codes.
 *
 * Author -
 * Mark Huston Bowden
 *
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bu.h"


#define ASPECT (9.8/7.1)
#define geti(x) { (x) = getchar(); (x) |= (short)(getchar()<<8); }
#define getb(x)	((x) = getchar())


int
main(int argc, char **argv)
{
    char colors[8][3];
    int numcolors = 0;
    int c, i, x, y, x_1, x_2, y_1, y_2, r, g, b;

    if (argc != 1) {
	bu_exit(1, "Usage: %s < infile > outfile\n", argv[0]);
    }

    getb(c);
    do {
	switch (c) {
	    case 'p':		/* point */
		geti(x);
		geti(y);
		printf("PU;\n");
		printf("PA %d %d;\n", x, y);
		printf("PD;\n");
		break;
	    case 'l':		/* line */
		geti(x_1);
		geti(y_1);
		geti(x_2);
		geti(y_2);
		printf("PU;\n");
		printf("PA %d %d;\n", x_1, y_1);
		printf("PD;\n");
		printf("PA %d %d;\n", x_2, y_2);
		break;
	    case 'f':		/* line style */
		while (getchar() != '\n');
		/* set line style ignored */
		break;
	    case 'm':		/* move */
		geti(x);
		geti(y);
		printf("PU;\n");
		printf("PA %d %d;\n", x, y);
		break;
	    case 'n':		/* draw */
		geti(x);
		geti(y);
		printf("PD;\n");
		printf("PA %d %d;\n", x, y);
		break;
	    case 't':		/* text */
		while (getchar() != '\n');
		/* draw text ignored */
		break;
	    case 's':		/* space */
		geti(x_1);
		geti(y_1);
		geti(x_2);
		geti(y_2);
		x_1 *= ASPECT;
		x_2 *= ASPECT;
		printf("SC %d %d %d %d;\n", x_1, x_2, y_1, y_2);
		printf("SP 1;\n");
		printf("PU;\n");
		printf("PA %d %d;\n", x_1, y_1);
		printf("PD;\n");
		printf("PA %d %d;\n", x_1, y_2);
		printf("PA %d %d;\n", x_2, y_2);
		printf("PA %d %d;\n", x_2, y_1);
		printf("PA %d %d;\n", x_1, y_1);
		break;
	    case 'C':		/* color */
		r = getchar();
		g = getchar();
		b = getchar();
		for (i = 0; i < numcolors; i++)
		    if (r == colors[i][0]
			&&  g == colors[i][1]
			&&  b == colors[i][2])
			break;
		if (i < numcolors)
		    i++;
		else
		    if (numcolors < 8) {
			colors[numcolors][0] = r;
			colors[numcolors][1] = g;
			colors[numcolors][2] = b;
			numcolors++;
			i++;
		    } else
			i = 8;
		printf("SP %d;\n", i);
		break;
	    default:
		bu_log("unable to process cmd x%x\n", c);
		break;
	}
	getb(c);
    } while (!feof(stdin));

    return 0;
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
