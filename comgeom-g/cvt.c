/*
 *			C V T . C
 *
 *	This is the mainline for converting COM-GEOM
 * cards to a GED style database.
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Original Version -
 *	February, 1980
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"
#include "wdb.h"

extern int	getopt();
extern char	*optarg;
extern int	optind;

struct wmember	*wmp;	/* array indexed by region number */

int	version = 5;	/* v4 or v5 ? */
int	verbose = 0;	/* verbose = print a message on every item read */

char name_it[16];	/* stores argv if it exists and appends it
			to each name generated.*/

int	cur_col = 0;

FILE	*infp;
FILE	*outfp;		/* Output file descriptor */

int	sol_total, sol_work;	/* total num solids, num solids processed */
int	reg_total;


static char usage[] = "\
Usage: comgeom-g [-d debug_lvl] [-v version#] [-s name_suffix]\n\
	input_file output_file\n";

get_args( argc, argv )
register char **argv;
{
	register int	c;
	char		*file_name;

	while ( (c = getopt( argc, argv, "d:v:s:" )) != EOF )  {
		switch( c )  {
		case 'd':
			verbose = atoi(optarg);
			break;
		case 's':
			strncpy( name_it, optarg, sizeof(name_it) );
			break;
		case 'v':
			version = atoi(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( optind+2 > argc )
		return(0);		/* FAIL */

	/* Input File */
	if( optind >= argc )  {
		if( isatty(fileno(stdin)) )
			return(0);
		infp = stdin;
		optind++;
	} else {
		file_name = argv[optind++];
		if( (infp = fopen(file_name, "r")) == NULL )  {
			perror(file_name);
			return(0);
		}
	}

	/* Output File */
	if( optind >= argc )  {
		if( isatty(fileno(stdout)) )
			return(0);
		outfp = stdout;
		optind++;
	} else {
		file_name = argv[optind++];
		if( (outfp = fopen(file_name, "w")) == NULL )  {
			perror(file_name);
			return(0);
		}
	}

	if ( argc > ++optind )
		(void)fprintf( stderr, "comgeom-g: excess argument(s) ignored\n" );

	return(1);		/* OK */
}


/*
 *			M A I N
 */
main( argc, argv )
char **argv;
{
	register int i;
	register float *op;
	char	ctitle[132];
	char	units[16];

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	if( version != 4 && version != 5 )  {
		fprintf(stderr,"version %d not supported\n", version );
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	printf("Reading version %d COMGEOM file\n", version );

	if( verbose )  {
		printf("COMGEOM input file must have this format:\n");
		printf("     1.  title card\n");
		printf("     2.  control card\n");
		printf("     3.  solid table\n");
		printf("     4.  region table\n");
		printf("     5.  -1\n");
		if( version == 4 )
			printf("     6.  blank\n");
	   	printf("     7.  region ident table\n\n");
	}

	group_init();

	/*
	 *  Read title
	 */
	if( getline( ctitle, sizeof(ctitle), "title card" ) == EOF ) {
		printf("Empty input file:  no title record\n");
		exit(10);
	}

	/* First 2 chars are units */
	printf("Units: %2.2s\n", ctitle);
	printf("Title: %s\n", ctitle+2);

	/* Before converting any geometry, establish the units conversion
	 * factor which all mk_* routines will apply.
	 */
	units[0] = ctitle[0];
	units[1] = ctitle[1];
	units[2] = '\0';
	if( mk_conversion( units ) < 0 )
		printf("WARNING:  unknown units '%s', using MM\n", units);

	/* Output the MGED database header */
	mk_id( outfp, ctitle+3 );


	/*
	 *  Read control card
	 */
	sol_total = 0;

	if( getline( ctitle, sizeof(ctitle), "control card" ) == EOF ) {
		printf("No control card .... STOP\n");
		exit(10);
	}

	if( version == 5 )  {
		sscanf( ctitle, "%5d%5d", &sol_total, &reg_total );
	} else {
		sscanf( ctitle, "%20d%10d", &sol_total, &reg_total );
	}
	if(verbose) printf("Expecting %d solids, %d regions\n", sol_total, reg_total);


	/*
	 *  SOLID TABLE
	 */
	if(verbose) printf("Solid table\n");
	sol_work = 0;
	while( sol_work < sol_total ) {
		i = getsolid();
		if( i < 0 )  {
			printf("error converting solid %d\n", sol_work);
			/* Should we abort here? */
			continue;
		}
		if( i > 0 ) {
			printf("\nprocessed %d of %d solids\n\n",sol_work,sol_total);
			if( sol_work < sol_total )  {
				printf("some solids are missing, aborting\n");
				exit(1);
			}
			break;		/* done */
		}
	}

	/* REGION TABLE */

	if(verbose)printf("\nRegion table\n");

	i = sizeof(struct wmember) * (reg_total+2);
	if( (wmp = (struct wmember *)malloc(i)) == (struct wmember *)0 )  {
		printf("malloc(%d) failed\n", i );
		exit(42);
	}
	for( i=reg_total+1; i>=0; i-- )  {
		wmp[i].wm_forw = wmp[i].wm_back = &wmp[i];
	}

	cur_col = 0;
	if( getregion() < 0 )  exit(10);

	if( version == 4 )  {
		char	dummy[88];
		/* read the blank card (line) */
		getline( dummy, sizeof(dummy), "blank card" );
	}

	if(verbose) printf("\nRegion ident table\n");
	getid();

	if(verbose) printf("\nGroups\n");
	cur_col = 0;
	group_write();
	if(verbose) printf("\n");

	exit(0);
}

/*
 *			C O L _ P R
 */
col_pr( str )
char	*str;
{
	printf("%s", str);
	cur_col += strlen(str);
	while( cur_col < 78 && ((cur_col%10) > 0) )  {
		putchar(' ');
		cur_col++;
	}
	if( cur_col >= 78 )  {
		printf("\n");
		cur_col = 0;
	}
}
