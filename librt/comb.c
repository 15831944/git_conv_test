/*
 *			C O M B . C
 *
 * XXX Move to db_tree.c when complete.
 *
 *  Authors -
 *	John R. Anderson
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1996 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif


#include <stdio.h>
#include <math.h>
#include <string.h>
#include "machine.h"
#include "db.h"
#include "externs.h"
#include "vmath.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "/m/cad/librt/debug.h"

/*
 *  In-memory format for combination.
 *  (Regions and Groups are both a kind of Combination).
 *  Move to h/wdb.h
 */
struct rt_comb_internal  {
	long		magic;
	union tree	*tree;		/* Leading to tree_db_leaf leaves */
	char		region_flag;	/* !0 ==> this COMB is a REGION */
	/* Begin GIFT compatability */
	short		region_id;
	short		aircode;
	short		GIFTmater;
	short		los;
	/* End GIFT compatability */
	char		rgb_valid;	/* !0 ==> rgb[] has valid color */
	unsigned char	rgb[3];
	struct rt_vls	shader_name;
	struct rt_vls	shader_param;
	struct rt_vls	material;
	char		inherit;
};
#define RT_COMB_MAGIC	0x436f6d49	/* "ComI" */
#define RT_CK_COMB(_p)		NMG_CKMAG( _p , RT_COMB_MAGIC , "rt_comb_internal" )


/* Internal. Ripped off from db_tree.c */
struct tree_list {
	union tree *tl_tree;
	int	tl_op;
};
#define TREE_LIST_NULL	((struct tree_list *)0)


/* --- Begin John's pretty-printer --- */

char
*Recurse_tree( tree )
union tree *tree;
{
	char *left,*right;
	char *return_str;
	char op;
	int return_length;

	if( tree == NULL )
		return( (char *)NULL );
	if( tree->tr_op == OP_UNION || tree->tr_op == OP_SUBTRACT || tree->tr_op == OP_INTERSECT )
	{
		left = Recurse_tree( tree->tr_b.tb_left );
		right = Recurse_tree( tree->tr_b.tb_right );
		switch( tree->tr_op )
		{
			case OP_UNION:
				op = 'u';
				break;
			case OP_SUBTRACT:
				op = '-';
				break;
			case OP_INTERSECT:
				op = '+';
				break;
		}
		return_length = strlen( left ) + strlen( right ) + 4;
		if( op == 'u' )
			return_length += 4;
		return_str = (char *)rt_malloc( return_length , "recurse_tree: return string" );
		if( op == 'u' )
		{
			char *blankl,*blankr;

			blankl = strchr( left , ' ' );
			blankr = strchr( right , ' ' );
			if( blankl && blankr )
				sprintf( return_str , "(%s) %c (%s)" , left , op , right );
			else if( blankl && !blankr )
				sprintf( return_str , "(%s) %c %s" , left , op , right );
			else if( !blankl && blankr )
				sprintf( return_str , "%s %c (%s)" , left , op , right );
			else
				sprintf( return_str , "%s %c %s" , left , op , right );
		}
		else
			sprintf( return_str , "%s %c %s" , left , op , right );
		if( tree->tr_b.tb_left->tr_op != OP_DB_LEAF )
			rt_free( left , "Recurse_tree: left string" );
		if( tree->tr_b.tb_right->tr_op != OP_DB_LEAF )
			rt_free( right , "Recurse_tree: right string" );
		return( return_str );
	}
	else if( tree->tr_op == OP_DB_LEAF ) {
		return tree->tr_l.tl_name ;
	}
}

void
Print_tree( tree )
union tree *tree;
{
	char *str;

	str = Recurse_tree( tree );
	if( str != NULL )
	{
		printf( "%s\n" , str );
		rt_free( str , "Print_tree" );
	}
	else
		printf( "NULL Tree\n" );
}

main( argc , argv )
int argc;
char *argv[];
{
	struct db_i		*dbip;
	struct directory	*dp;
	struct rt_external	ep;
	struct rt_db_internal	ip;
	struct rt_comb_internal	*comb;
	mat_t			identity_mat;
	int			i;

	if( argc < 3 )
	{
		fprintf( stderr , "Usage:\n\t%s db_file object1 object2 ...\n" , argv[0] );
		exit( 1 );
	}

	mat_idn( identity_mat );

	if( (dbip = db_open( argv[1] , "r" ) ) == NULL )
	{
		fprintf( stderr , "Cannot open %s\n" , argv[1] );
		perror( "test" );
		exit( 1 );
	}

	/* Scan the database */
	db_scan(dbip, (int (*)())db_diradd, 1);

	for( i=2 ; i<argc ; i++ )
	{
		int j;

		printf( "%s\n" , argv[i] );

		dp = db_lookup( dbip , argv[i] , 1 );
		if( db_get_external( &ep , dp , dbip ) )
		{
			rt_log( "db_get_external failed for %s\n" , argv[i] );
			continue;
		}
		if( rt_comb_v4_import( &ip , &ep , NULL ) < 0 )  {
			rt_log("import of %s failed\n", dp->d_namep);
			continue;
		}

		RT_CK_DB_INTERNAL( &ip );

		if( ip.idb_type != ID_COMBINATION )
		{
			rt_log( "idb_type = %d\n" , ip.idb_type );
			continue;
		}

		comb = (struct rt_comb_internal *)ip.idb_ptr;
		RT_CK_COMB(comb);
		if( comb->region_flag )
		{
			rt_log( "\tRegion id = %d, aircode = %d GIFTmater = %d, los = %d\n",
				comb->region_id, comb->aircode, comb->GIFTmater, comb->los );
		}
		rt_log( "\trgb_valid = %d, color = %d/%d/%d\n" , comb->rgb_valid , V3ARGS( comb->rgb ) );
		rt_log( "\tmaterial name = %s, parameters = %s, (%s)\n" ,
				rt_vls_addr( &comb->shader_name ),
				rt_vls_addr( &comb->shader_param ),
				rt_vls_addr( &comb->material )
		);

		/* John's way */
		Print_tree( comb->tree );

		/* Standard way */
		rt_pr_tree( comb->tree, 1 );

		/* Compact way */
		{
			struct rt_vls	str;
			rt_vls_init( &str );
			rt_pr_tree_vls( &str, comb->tree );
			rt_log("%s\n", rt_vls_addr(&str) );
			rt_vls_free(&str );
		}

		/* Test the support routines */
		if( db_ck_v4gift_tree( comb->tree ) < 0 )
			rt_log("ERROR: db_ck_v4gift_tree is unhappy\n");

		/* Test the lumberjacks */
		rt_comb_ifree( &ip );
	}
}
