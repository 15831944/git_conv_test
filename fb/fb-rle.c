/*
 *			F B - R L E . C
 *
 *  Encode a framebuffer image using the Utah Raster Toolkit RLE library
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <time.h>

#include "fb.h"
#include "svfb_global.h"

extern char	*malloc();
extern char	*getenv();

static struct sv_globals	outrle;
#define		outfp		outrle.svfb_fd
static char			comment[128];
static char			host[128];
static rle_pixel		**rows;
static long			now;
static char			*who;

static rle_map			rlemap[3*256];		/* Utah format map */
static ColorMap			cmap;			/* libfb format map */

static int	crunch;

static int	background[3];	/* default background is black */

static int	screen_width;
static int	screen_height;
static int	file_width;
static int	file_height;
static int	screen_xoff;
static int	screen_yoff;

static char	*framebuffer;

static char	usage[] = "\
Usage: fb-rle [-c -h -d] [-F framebuffer] [-C r/g/b]\n\
	[-S squarescrsize] [-W screen_width] [-N screen_height]\n\
	[-X screen_xoff] [-Y screen_yoff]\n\
	[-s squarefilesize] [-w file_width] [-n file_height]\n\
	[file.rle]\n\
\n\
If omitted, the .rle file is written to stdout\n";

extern char	*strdup();
extern void	cmap_crunch();

/*
 *			G E T _ A R G S
 */
static int
get_args( argc, argv )
register char	**argv;
{
	register int	c;
	extern int	optind;
	extern char	*optarg;

	while( (c = getopt( argc, argv, "chds:w:n:S:W:N:X:Y:C:" )) != EOF )  {
		switch( c )  {
		case 'c':
			crunch = 1;
			break;
		case 'F':
			framebuffer = optarg;
			break;
		case 'h':
			/* high-res */
			screen_height = screen_width = 1024;
			break;
		case 's':
			/* square file size */
			file_height = file_width = atoi(optarg);
			break;
		case 'S':
			screen_height = screen_width = atoi(optarg);
			break;
		case 'w':
			file_width = atoi(optarg);
			break;
		case 'W':
			screen_width = atoi(optarg);
			break;
		case 'n':
			file_height = atoi(optarg);
			break;
		case 'N':
			screen_height = atoi(optarg);
			break;
		case 'X':
			screen_xoff = atoi(optarg);
			break;
		case 'Y':
			screen_yoff = atoi(optarg);
			break;
		case 'C':
			{
				register char *cp = optarg;
				register int *conp = background;

				/* premature null => atoi gives zeros */
				for( c=0; c < 3; c++ )  {
					*conp++ = atoi(cp);
					while( *cp && *cp++ != '/' ) ;
				}
			}
			break;
		default:
		case '?':
			return	0;
		}
	}
	if( argv[optind] != NULL )  {
		if( access( argv[optind], 0 ) == 0 )  {
			(void) fprintf( stderr,
				"\"%s\" already exists.\n",
				argv[optind] );
			exit( 1 );
		}
		if( (outfp = fopen( argv[optind], "w" )) == NULL )  {
			perror(argv[optind]);
			return	0;
		}
	}
	if( argc > ++optind )
		(void) fprintf( stderr, "fb-rle: Excess arguments ignored\n" );

	if( isatty(fileno(outfp)) )
		return 0;
	return	1;
}

/*
 *			M A I N
 */
main( argc, argv )
int	argc;
char	*argv[];
{
	register FBIO	*fbp;
	register RGBpixel *scan_buf;
	register int	y;
	int		cm_save_needed;

	outfp = stdout;
	if( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	/* If screen size = default & file size is given, track file size */
	if( screen_width == 0 && file_width > 0 )
		screen_width = file_width;
	if( screen_height == 0 && screen_height > 0 )
		screen_height = file_height;

	if( (fbp = fb_open( framebuffer, screen_width, screen_height )) == FBIO_NULL )
		exit(12);

	/* Honor original screen size desires, if set, unless they shrank */
	if( screen_width == 0 || fb_getwidth(fbp) < screen_width )
		screen_width = fb_getwidth(fbp);
	if( screen_height == 0 || fb_getheight(fbp) < screen_height )
		screen_height = fb_getheight(fbp);

	/* If not specified, output file size tracks screen size */
	if( file_width == 0 )
		file_width = screen_width;
	if( file_height == 0 )
		file_height = screen_height;

	/* Clip below and to left of (0,0) */
	if( screen_xoff < 0 )  {
		file_width += screen_xoff;
		screen_xoff = 0;
	}
	if( screen_yoff < 0 )  {
		file_height += screen_yoff;
		screen_yoff = 0;
	}

	/* Clip up and to the right */
	if( screen_xoff + file_width > screen_width )
		file_width = screen_width - screen_xoff;
	if( screen_yoff + file_height > screen_height )
		file_height = screen_height - screen_yoff;

	if( file_width <= 0 || file_height <= 0 )  {
		fprintf(stderr,
			"fb-rle: Error: image rectangle entirely off screen\n");
		exit(1);
	}

	/* Read color map, see if it is linear */
	cm_save_needed = 1;
	if( fb_rmap( fbp, &cmap ) == -1 )
		cm_save_needed = 0;
	if( cm_save_needed && is_linear_cmap( &cmap ) )
		cm_save_needed = 0;
	if( crunch && (cm_save_needed == 0) )
		crunch = 0;

	/* Convert to Utah format */
	if( cm_save_needed )  for( y=0; y<255; y++ )  {
		rlemap[y+0*256] = cmap.cm_red[y];
		rlemap[y+1*256] = cmap.cm_green[y];
		rlemap[y+2*256] = cmap.cm_blue[y];
	}

	scan_buf = (RGBpixel *)malloc( sizeof(RGBpixel) * screen_width );

	/* Build RLE header */
	outrle.sv_ncolors = 3;
	SV_SET_BIT(outrle, SV_RED);
	SV_SET_BIT(outrle, SV_GREEN);
	SV_SET_BIT(outrle, SV_BLUE);
	outrle.sv_background = 2;		/* use background */
	outrle.sv_bg_color = background;
	outrle.sv_alpha = 0;			/* no alpha channel */
	if( cm_save_needed && !crunch ) {
		outrle.sv_ncmap = 3;
		outrle.sv_cmaplen = 8;		/* 1<<8 = 256 */
		outrle.sv_cmap = rlemap;
	} else {
		outrle.sv_ncmap = 0;		/* no color map */
		outrle.sv_cmaplen = 0;
		outrle.sv_cmap = (rle_map *)0;
	}
	outrle.sv_xmin = screen_xoff;
	outrle.sv_ymin = screen_yoff;
	outrle.sv_xmax = screen_xoff + file_width - 1;
	outrle.sv_ymax = screen_yoff + file_height - 1;
	outrle.sv_comments = (char **)0;

	/* Add comments to the header file, since we have one */
	if( framebuffer == (char *)0 )
		framebuffer = fbp->if_name;
	sprintf( comment, "encoded_from=%s", framebuffer );
	rle_putcom( strdup(comment), &outrle );
	now = time(0);
	sprintf( comment, "encoded_date=%24.24s", ctime(&now) );
	rle_putcom( strdup(comment), &outrle );
	if( (who = getenv("USER")) != (char *)0 ) {
		sprintf( comment, "encoded_by=%s", who);
		rle_putcom( strdup(comment), &outrle );
	}
#	ifdef BSD
	gethostname( host, sizeof(host) );
	sprintf( comment, "encoded_host=%s", host);
	rle_putcom( strdup(comment), &outrle );
#	endif

	sv_setup( RUN_DISPATCH, &outrle );
	rle_row_alloc( &outrle, &rows );

	/* Read the image a scanline at a time, and encode it */
	for( y = 0; y < file_height; y++ )  {
		if( fb_read( fbp, screen_xoff, y+screen_yoff, scan_buf,
		    file_width ) == -1 )  {
			(void) fprintf(	stderr,
				"fb-rle: read of %d pixels on line %d failed!\n",
				file_width, y+screen_yoff );
			exit(1);
		}

		if( crunch )
			cmap_crunch( scan_buf, file_width, &cmap );

		/* Grumble, convert to Utah layout */
		{
			register unsigned char	*pp = (unsigned char *)scan_buf;
			register rle_pixel	*rp = rows[0];
			register rle_pixel	*gp = rows[1];
			register rle_pixel	*bp = rows[2];
			register int		i;

			for( i=0; i<file_width; i++ )  {
				*rp++ = *pp++;
				*gp++ = *pp++;
				*bp++ = *pp++;
			}
		}
		sv_putrow( rows, file_width, &outrle );
	}
	sv_puteof( &outrle );

	fb_close( fbp );
	fclose( outfp );
	exit(0);
}
