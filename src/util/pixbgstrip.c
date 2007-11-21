/*                    P I X B G S T R I P . C
 * BRL-CAD
 *
 * Copyright (c) 1991-2007 United States Government as represented by
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
/** @file pixbgstrip.c
 *
 * Function -
 *	Backgound Un-Maker
 *
 *  Author -
 *	Michael John Muuss
 *
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "fb.h"


static unsigned char *scanline;		/* 1 scanline pixel buffer */
static long int	scanbytes;		/* # of bytes of scanline */

static char	*file_name;
static FILE	*infp;
static int	fileinput = 0;		/* file of pipe on input? */

static int	autosize = 0;		/* !0 to autosize input */

static long int	file_width = 512L;	/* default input width */

static int	thresh = 1;
static int	bg_x_offset = 0;

static char usage[] = "\
Usage: pixbgstrip [-a -h] [-t thresh] [-x x_off for bg pixel]\n\
	[-s squarefilesize] [-w file_width] [-n file_height]\n\
	[file.pix]\n";

int
get_args(int argc, register char **argv)
{
	register int c;

	while ( (c = bu_getopt( argc, argv, "ahs:w:n:t:x:" )) != EOF )  {
		switch( c )  {
		case 'a':
			autosize = 1;
			break;
		case 'h':
			/* high-res */
			file_width = 1024L;
			autosize = 0;
			break;
		case 's':
			/* square file size */
			file_width = atol(bu_optarg);
			autosize = 0;
			break;
		case 'w':
			file_width = atol(bu_optarg);
			autosize = 0;
			break;
		case 'n':
			autosize = 0;
			break;
		case 't':
			thresh = atoi(bu_optarg);
			break;
		case 'x':
			bg_x_offset = atoi(bu_optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( bu_optind >= argc )  {
		if( isatty(fileno(stdin)) )
			return(0);
		file_name = "-";
		infp = stdin;
	} else {
		file_name = argv[bu_optind];
		if( (infp = fopen(file_name, "r")) == NULL )  {
			perror(file_name);
			(void)fprintf( stderr,
				"pixbgstrip: cannot open \"%s\" for reading\n",
				file_name );
			bu_exit (1, "");
		}
		fileinput++;
	}

	if ( argc > ++bu_optind )
		(void)fprintf( stderr, "pixbgstrip: excess argument(s) ignored\n" );

	return(1);		/* OK */
}

int
main(int argc, char **argv)
{
	register int	r,g,b;
	register long int	i;

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		bu_exit ( 1, "" );
	}

	if( isatty(fileno(stdout)) )  {
		(void)fputs("Binary output must be redirected away from the terminal\n", stderr);
		(void)fputs(usage, stderr);
		bu_exit ( 1, "" );
	}

	/* autosize input? */
	if( fileinput && autosize ) {
		unsigned long int	w, h;
		if( fb_common_file_size(&w, &h, file_name, 3) ) {
			file_width = (long)w;
		} else {
			fprintf(stderr,"pixbgstrip: unable to autosize\n");
		}
	}

	scanbytes = file_width * sizeof(RGBpixel);
	scanline = (unsigned char *)bu_malloc(scanbytes, "scanline");

	while( !feof(infp) )  {
		if( fread( scanline, 1, scanbytes, infp ) != scanbytes )
			break;
		r = scanline[bg_x_offset*3+0];
		g = scanline[bg_x_offset*3+1];
		b = scanline[bg_x_offset*3+2];
		for( i=0; i<file_width; i++ )  {
			register int diff;

			diff = scanline[i*3+0] - r;
			if( diff <= -thresh || diff >= thresh ) continue;

			diff = scanline[i*3+1] - g;
			if( diff <= -thresh || diff >= thresh ) continue;

			diff = scanline[i*3+2] - b;
			if( diff <= -thresh || diff >= thresh ) continue;

			/* Input pixel matches background, set to black */
			scanline[i*3+0] =
				scanline[i*3+1] =
				scanline[i*3+2] = 0;
		}
		if( fwrite( scanline, 1, scanbytes, stdout) != scanbytes )  {
			perror("pixbgstrip: fwrite()");
			bu_exit (1, "");
		}
	}
	bu_free(scanline, "scanline");
	return(0);
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
