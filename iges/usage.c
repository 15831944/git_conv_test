/*
 *  Authors -
 *	John R. Anderson
 *	Susanne L. Muuss
 *	Earl P. Weaver
 *
 *  Source -
 *	VLD/ASB Building 1065
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */

#include <stdio.h>
#include "machine.h"
#include "vmath.h"

usage()
{
	fprintf( stderr , "Usage:  iges-g [-n] file.iges file.g\n" );
	fprintf( stderr , "\t-n - Convert all rational B-spline surfaces to a single spline solid\n" );
	exit( 1 );
}
