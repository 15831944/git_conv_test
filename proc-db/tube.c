/*
 *			T U B E . C
 *
 *  Program to generate a gun-tube as a procedural spline.
 *  The tube's core lies on the X axis.
 *
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright 
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "db.h"
#include "vmath.h"
#include "wdb.h"

#include "../libspl/b_spline.h"
#include "../rt/mathtab.h"

extern struct b_spline *spl_new();

mat_t	identity;
double	degtorad = 0.0174532925199433;
double	inches2mm = 25.4;

#define N_CIRCLE_KNOTS	12
fastf_t	circle_knots[N_CIRCLE_KNOTS] = {
	0,	0,	0,
	1,	1,
	2,	2,
	3,	3,
	4,	4,	4
};

#define IRT2	0.70710678	/* 1/sqrt(2) */
#define NCOLS	9
/* When scaling, multiply only XYZ, not W */
fastf_t poly[NCOLS*4] = {
	0,	1,	0,	1,
	0,	IRT2,	IRT2,	IRT2,
	0,	0,	1,	1,
	0,	-IRT2,	IRT2,	IRT2,
	0,	-1,	0,	1,
	0,	-IRT2,	-IRT2,	IRT2,
	0,	0,	-1,	1,
	0,	IRT2,	-IRT2,	IRT2,
	0,	1,	0,	1
};

point_t	sample[1024];
int	nsamples;

double	length;
double	spacing;

main(argc, argv)
char	**argv;
{
	register int i;
	int	frame;
	char	name[128];
	char	gname[128];
	double	iradius, oradius;
	vect_t	normal;
	struct wmember head, ghead;
	matp_t	matp;
	mat_t	xlate, prod;
	mat_t	rot1, rot2, rot3;
	fastf_t	pos;
	vect_t	from, to;
	vect_t	offset;

	head.wm_forw = head.wm_back = &head;
	ghead.wm_forw = ghead.wm_back = &ghead;

	mk_id( stdout, "Spline Tube" );

	VSET( normal, 0, -1, 0 );
	mk_half( stdout, "cut", 0, normal );

#ifdef never
	/* Numbers for a 105-mm M68 gun */
	oradius = 5 * inches2mm / 2;		/* 5" outer diameter */
	iradius = 4.134 * inches2mm / 2;	/* 5" inner (land) diameter */
#else
	/* Numbers invented to match 125-mm KE (Erline) round */
	iradius = 125.0/2;
	oradius = iradius + (5-4.134) * inches2mm / 2;		/* 5" outer diameter */
#endif
	fprintf(stderr,"inner radius=%gmm, outer radius=%gmm\n", iradius, oradius);

	length = 187 * inches2mm;
#ifdef never
	spacing = 100;			/* mm per sample */
	nsamples = ceil(length/spacing);
	fprintf(stderr,"length=%gmm, spacing=%gmm\n", length, spacing);
#endif

	for( frame=0;; frame++ )  {
#ifdef never
		/* Generate some dummy sample data */
		if( frame < 16 ) break;
		for( i=0; i<nsamples; i++ )  {
			sample[i][X] = i * spacing;
			sample[i][Y] = 0;
			sample[i][Z] = 4 * oradius * sin(
				((double)i*i)/nsamples * 2 * 3.14159265358979323 +
				frame * 3.141592 * 2 / 8 );
		}
#endif
		if( read_frame() < 0 )  break;

#define build_spline build_cyl
		sprintf( name, "tube%do", frame);
		build_spline( name, nsamples, oradius );
		(void)mk_addmember( name, &head );

		sprintf( name, "tube%di", frame);
		build_spline( name, nsamples, iradius );
		mk_addmember( name, &head )->wm_op = SUBTRACT;

		mk_addmember( "cut", &head )->wm_op = SUBTRACT;

		sprintf( name, "tube%d", frame);
		mk_lcomb( stdout, name, 1,
			"plastic", "",
			0, (char *)0, &head );

		/*  Place the tube region and the ammo together.
		 *  The origin of the ammo is expected to be the center
		 *  of the rearmost plate.
		 */
		mk_addmember( name, &ghead );
		matp = mk_addmember( "ke", &ghead )->wm_mat;
		pos = ((double)frame)/64 *	/* XXX hack: nframes */
			(sample[nsamples-1][X] - sample[0][X]); /* length */

		VSET( from, 0, -1, 0 );
		VSET( to, 1, 0, 0 );		/* to X axis */
		mat_fromto( rot1, from, to );

		VSET( from, 1, 0, 0 );
		xfinddir( to, pos, offset );
		mat_fromto( rot2, from, to );

		mat_idn( xlate );
		MAT_DELTAS( xlate, offset[X], offset[Y], offset[Z] );
		mat_mul( rot3, rot2, rot1 );
		mat_mul( matp, xlate, rot3 );

		(void)mk_addmember( "light.r", &ghead );
		sprintf( gname, "g%d", frame);
		mk_lcomb( stdout, gname, 0,
			(char *)0, "",
			0, (char *)0, &ghead );

		fprintf( stderr, "%d, ", frame );  fflush(stderr);
	}
}
#undef build_spline

build_spline( name, npts, radius )
char	*name;
int	npts;
double	radius;
{
	struct b_spline *bp;
	register int i;
	int	nv;
	int	cur_kv;
	fastf_t	*meshp;
	register int col;
	int	row;
	vect_t	point;

	/*
	 *  This spline will look like a cylinder.
	 *  In the mesh, the circular cross section will be presented
	 *  across the first row by filling in the 9 (NCOLS) columns.
	 *
	 *  The U direction is across the first row,
	 *  and has NCOLS+order[U] positions, 12 in this instance.
	 *  The V direction is down the first column,
	 *  and has NROWS+order[V] positions.
	 */
	bp = spl_new( 3,	4,		/* u,v order */
		N_CIRCLE_KNOTS,	npts+6,		/* u,v knot vector size */
		npts+2,		NCOLS,		/* nrows, ncols */
		P4 );

	/*  Build the U knots */
	for( i=0; i<N_CIRCLE_KNOTS; i++ )
		bp->u_kv->knots[i] = circle_knots[i];

	/* Build the V knots */
	cur_kv = 0;		/* current knot value */
	nv = 0;			/* current knot subscript */
	for( i=0; i<4; i++ )
		bp->v_kv->knots[nv++] = cur_kv;
	cur_kv++;
	for( i=4; i<(npts+4-2); i++ )
		bp->v_kv->knots[nv++] = cur_kv++;
	for( i=0; i<4; i++ )
		bp->v_kv->knots[nv++] = cur_kv;

	/*
	 *  The control mesh is stored in row-major order,
	 *  which works out well for us, as a row is one
	 *  circular slice through the tube.  So we just
	 *  have to write down the slices, one after another.
	 *  The first and last "slice" are the center points that
	 *  create the end caps.
	 */
	meshp = bp->ctl_mesh->mesh;

	/* Row 0 */
	for( col=0; col<9; col++ )  {
		*meshp++ = sample[0][X];
		*meshp++ = sample[0][Y];
		*meshp++ = sample[0][Z];
		*meshp++ = 1;
	}

	/* Rows 1..npts */
	for( i=0; i<npts; i++ )  {
		/* row = i; */
		VMOVE( point, sample[i] );
		for( col=0; col<9; col++ )  {
			register fastf_t h;

			h = poly[col*4+H];
			*meshp++ = poly[col*4+X]*radius + point[X]*h;
			*meshp++ = poly[col*4+Y]*radius + point[Y]*h;
			*meshp++ = poly[col*4+Z]*radius + point[Z]*h;
			*meshp++ = h;
		}
	}

	/* Row npts+1 */
	for( col=0; col<9; col++ )  {
		*meshp++ = sample[npts-1][X];
		*meshp++ = sample[npts-1][Y];
		*meshp++ = sample[npts-1][Z];
		*meshp++ = 1;
	}
		
	mk_bsolid( stdout, name, 1, 0.1 );
	mk_bsurf( stdout, bp );
	spl_sfree( bp );
}

/* Returns -1 if done, 0 if something to draw */
read_frame()
{
	char	buf[256];
	int	i;

	if( feof(stdin) )
		return(-1);

	for( nsamples=0;;nsamples++)  {
		if( fgets( buf, sizeof(buf), stdin ) == NULL )  break;
		if( buf[0] == '\0' )  {
			/* Blank line, marks break in implicit connection */
			fprintf(stderr,"implicit break unimplemented\n");
			continue;
		}
		if( buf[0] == '=' )  {
			/* End of frame */
			break;
		}
		i = sscanf( buf, "%f %f %f",
			&sample[nsamples][X],
			&sample[nsamples][Y],
			&sample[nsamples][Z] );
		if( i != 3 )  {
			fprintf("input line didn't have 3 numbers: %s\n", buf);
			break;
		}
		/* Phil's numbers are in meters, not mm */
		sample[nsamples][X] *= 1000;
		sample[nsamples][Y] *= 1000;
		sample[nsamples][Z] *= 1000;
	}
	if( nsamples <= 0 )
		return(-1);
	return(0);
}

build_cyl( cname, npts, radius )
char	*cname;
int	npts;
double	radius;
{
	register int i;
	vect_t	v, h, a, b;
	char	name[32];
	struct wmember head;

	head.wm_forw = head.wm_back = &head;

	for( i=0; i<npts-1; i++ )  {
		VMOVE( v, sample[i] );
		VSUB2( h, sample[i+1], v );
		VSET( a, 0, radius, 0 );
		VSET( b, 0, 0, radius );

		sprintf( name, "%s%d", cname, i );
		mk_tgc( stdout, name, v, h, a, b, a, b );
		(void)mk_addmember( name, &head );
	}
	mk_lcomb( stdout, cname, 0,
		(char *)0, "",
		0, (char *)0,
		&head );
}

/*
 * Find which section a given X value is in, and indicate what
 * direction the tube is headed in then.
 */
xfinddir( dir, x, loc)
vect_t	dir;
double	x;
point_t	loc;
{
	register int i;
	fastf_t	ratio;

	for( i=0; i<nsamples-1; i++ )  {
		if( x < sample[i][X] )
			break;
		if( x >= sample[i+1][X] )
			continue;
		VSUB2( dir, sample[i+1], sample[i] );
		ratio = (x-sample[i][X]) / (sample[i+1][X]-sample[i][X]);
		VJOIN1( loc, sample[i], ratio, dir );

		VUNITIZE( dir );
		return;
	}
	fprintf(stderr, "xfinddir: couldn't find x=%g\n", x);
	VSET( dir, 1, 0, 0 );
}
