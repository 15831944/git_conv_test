/*
 *			C O N C A T . C
 *
 *  Functions -
 *	f_dup()		checks for dup names before cat'ing of two files
 *	f_concat()	routine to cat another GED file onto end of current file
 *
 *  Authors -
 *	Michael John Muuss
 *	Keith A. Applin
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSconcat[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <pwd.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "externs.h"
#include "./ged.h"
#include "./sedit.h"

int			num_dups;
struct directory	**dup_dirp;

char	new_name[NAMESIZE];
char	prestr[NAMESIZE];
int	ncharadd;

/*
 *			M G E D _ D I R _ C H E C K
 *
 * Check a name against the global directory.
 */
int
mged_dir_check( input_dbip, name, laddr, len, flags )
register struct db_i	*input_dbip;
register char		*name;
long			laddr;
int			len;
int			flags;
{
	struct directory	*dupdp;
	char			local[NAMESIZE+2];

	if( input_dbip->dbi_magic != DBI_MAGIC )  rt_bomb("mged_dir_check:  bad dbip\n");

	/* Add the prefix, if any */
	if( ncharadd > 0 )  {
		(void)strncpy( local, prestr, ncharadd );
		(void)strncpy( local+ncharadd, name, NAMESIZE-ncharadd );
	} else {
		(void)strncpy( local, name, NAMESIZE );
	}
	local[NAMESIZE] = '\0';
		
	/* Look up this new name in the existing (main) database */
	if( (dupdp = db_lookup( dbip, local, LOOKUP_QUIET )) != DIR_NULL )  {
		/* Duplicate found, add it to the list */
		num_dups++;
		*dup_dirp++ = dupdp;
	}
	return 0;
}

/*
 *
 *			F _ D U P ( )
 *
 *  Check for duplicate names in preparation for cat'ing of files
 *
 *  Usage:  dup file.g [prefix]
 */
int
f_dup( argc, argv )
int	argc;
char	**argv;
{
	struct db_i		*newdbp;
	struct directory	**dirp0;

	(void)signal( SIGINT, sig2 );           /* allow interrupts */

	/* get any prefix */
	if( argc < 3 ) {
		prestr[0] = '\0';
	} else {
		(void)strcpy(prestr, argv[2]);
	}
	num_dups = 0;
	if( (ncharadd = strlen( prestr )) > 12 )  {
		ncharadd = 12;
		prestr[12] = '\0';
	}

	/* open the input file */
	if( (newdbp = db_open( argv[1], "r" )) == DBI_NULL )  {
		perror( argv[1] );
		(void)rt_log( "dup: Can't open %s\n", argv[1]);
		return CMD_BAD;
	}

	rt_log("\n*** Comparing %s with %s for duplicate names\n",
		dbip->dbi_filename,argv[1]);
	if( ncharadd ) {
		rt_log("  For comparison, all names in %s prefixed with:  %s\n",
				argv[1],prestr);
	}

	/* Get array to hold names of duplicates */
	if( (dup_dirp = dir_getspace(0)) == (struct directory **) 0) {
		rt_log( "f_dup: unable to get memory\n");
		return CMD_BAD;
	}
	dirp0 = dup_dirp;

	/* Scan new database for overlaps */
	if( db_scan( newdbp, mged_dir_check, 0 ) < 0 )  {
		(void)rt_log( "dup: db_scan failure\n");
		return CMD_BAD;
	}

	col_pr4v( dirp0, (int)(dup_dirp - dirp0));
	rt_free( (char *)dirp0, "dir_getspace array" );
	rt_log("\n -----  %d duplicate names found  -----\n",num_dups);

	db_close( newdbp );

	return CMD_OK;
}


/*
 *			M G E D _ D I R _ A D D
 *
 *  Add a solid or conbination from an auxillary database
 *  into the primary database.
 */
int
mged_dir_add( input_dbip, name, laddr, len, flags )
register struct db_i	*input_dbip;
register char		*name;
long			laddr;
int			len;
int			flags;
{
	register struct directory *input_dp;
	register struct directory *dp;
	union record		*rec;
	char			local[NAMESIZE+2+2];

	if( input_dbip->dbi_magic != DBI_MAGIC )  rt_bomb("mged_dir_add:  bad dbip\n");

	/* Add the prefix, if any */
	if( ncharadd > 0 )  {
		(void)strncpy( local, prestr, ncharadd );
		(void)strncpy( local+ncharadd, name, NAMESIZE-1-ncharadd );
	} else {
		(void)strncpy( local, name, NAMESIZE );
	}
	local[NAMESIZE-1] = '\0';
		
	/* Look up this new name in the existing (main) database */
	if( (dp = db_lookup( dbip, local, LOOKUP_QUIET )) != DIR_NULL )  {
		register int	c;
		char		loc2[NAMESIZE+2+2];

		/* This object already exists under the (prefixed) name */
		/* Protect the database against duplicate names! */
		/* Change object names, but NOT any references made by combinations. */
		(void)strncpy( loc2, local, NAMESIZE );
		/* Shift name right two characters, and further prefix */
		strncpy( local+2, loc2, NAMESIZE-2 );
		local[1] = '_';			/* distinctive separater */
		local[NAMESIZE-1] = '\0';	/* ensure null termination */

		for( c = 'A'; c <= 'Z'; c++ )  {
			local[0] = c;
			if( (dp = db_lookup( dbip, local, LOOKUP_QUIET )) == DIR_NULL )
				break;
		}
		if( c > 'Z' )  {
			rt_log("mged_dir_add: Duplicate of name '%s', ignored\n",
				local );
			return 0;
		}
		rt_log("mged_dir_add: Duplicate of '%s' given new name '%s'\nYou should have used the 'dup' command to detect this,\nand then specified a prefix for the 'concat' command.\n",
			loc2, local );
	}

	/* First, register this object in input database */
	if( (input_dp = db_diradd( input_dbip, name, laddr, len, flags)) == DIR_NULL )
		return(-1);

	/* Then, register a new object in the main database */
	if( (dp = db_diradd( dbip, local, -1L, len, flags)) == DIR_NULL )
		return(-1);
	if( db_alloc( dbip, dp, len ) < 0 )
		return(-1);

	/* Read in all the records for this object */
	if( (rec = db_getmrec( input_dbip, input_dp )) == (union record *)0 )  {
		READ_ERR;
		if( db_delete( dbip, dp ) < 0 ||
		    db_dirdelete( dbip, dp ) < 0 )  {
		    	DELETE_ERR(local);
		}
	    	/* Abort processing on first error */
		return -1;
	}

	/* Update the name, and any references */
	if( flags & DIR_SOLID )  {
		rt_log("adding solid '%s'\n", local );
		if ((ncharadd + strlen(name)) >= (unsigned)NAMESIZE)
			rt_log("WARNING: solid name \"%s%s\" truncated to \"%s\"\n",
				prestr,name, local);

		/* Depends on all kinds of solids having name in same place */
		NAMEMOVE( local, rec->s.s_name );
	} else {
		register int i;
		char	mref[NAMESIZE+2];

		rt_log("adding  comb '%s'\n", local );
		NAMEMOVE( local, rec->c.c_name );

		/* Update all the member records */
		for( i=1; i < dp->d_len; i++ )  {
			if( ncharadd ) {
				(void)strncpy( mref, prestr, ncharadd );
				(void)strncpy( mref+ncharadd,
					rec[i].M.m_instname,
					NAMESIZE-1-ncharadd );
				NAMEMOVE( mref, rec[i].M.m_instname );
			}
		}
	}

	/* Write the new record into the main database */
	if( db_put( dbip, dp, rec, 0, len ) < 0 )
		return(-1);

	rt_free( (char *)rec, "db_getmrec rec" );
	return 0;
}


/*
 *			F _ C O N C A T
 *
 *  Concatenate another GED file into the current file.
 *  Interrupts are not permitted during this function.
 *
 *  Usage:  dup file.g [prefix]
 *
 *  NOTE:  If a prefix is not given on the command line,
 *  then the users insist that they be prompted for the prefix,
 *  to prevent inadvertently sucking in a non-prefixed file.
 */
int
f_concat( argc, argv )
int	argc;
char	**argv;
{
	struct db_i		*newdbp;
	int bad = 0;

	/* get any prefix */
	if( argc < 3 )  {
		(void)rt_log(
			"concat: Enter prefix string or / for no prefix: ");
		return CMD_MORE;
	}

	if( strcmp( argv[2], "/" ) == 0 )  {
		/* No prefix desired */
		(void)strcpy(prestr, "\0");
	} else {
		(void)strcpy(prestr, argv[2]);
	}

	if( (ncharadd = strlen( prestr )) > 12 )  {
		ncharadd = 12;
		prestr[12] = '\0';
	}

	/* open the input file */
	if( (newdbp = db_open( argv[1], "r" )) == DBI_NULL )  {
		perror( argv[1] );
		(void)rt_log( "concat: Can't open %s\n", argv[1]);
		return CMD_BAD;
	}

	/* Scan new database, adding everything encountered. */
	if( db_scan( newdbp, mged_dir_add, 1 ) < 0 )  {
		(void)rt_log( "concat: db_scan failure\n");
		bad = 1;	
		/* Fall through, to close off database */
	}

	/* Free all the directory entries, and close the input database */
	db_close( newdbp );

	sync();		/* just in case... */

	return bad ? CMD_BAD : CMD_OK;
}
