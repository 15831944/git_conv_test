/*
 *		G 2 A S C . C
 *  
 *  This program generates an ASCII data file which contains
 *  a GED database.
 *
 *  Usage:  g2asc < file.g > file.asc
 *  
 *  Author -
 *  	Charles M Kennedy
 *  	Michael J Muuss
 *	Susanne Muuss, J.D.
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif
 
#include <stdio.h>
#include <ctype.h>
#include "machine.h"
#include "vmath.h"
#include "externs.h"
#include "db.h"
#include "wdb.h"
#include "rtlist.h"
#include "raytrace.h"
#include "rtgeom.h"


mat_t	id_mat = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0};	/* identity matrix for pipes */

char *name();
char *strchop();
#define CH(x)	strchop(x,sizeof(x))

int	combdump();
void	idendump(), polyhead(), polydata();
void	soldump();
void	membdump(), arsadump(), arsbdump();
void	materdump(), bspldump(), bsurfdump();
void	pipe_dump(), particle_dump(), dump_pipe_segs();
void	arbn_dump();

union record	record;		/* GED database record */

main(argc, argv)
char **argv;
{
	/* Read database file */
	while( fread( (char *)&record, sizeof record, 1, stdin ) == 1  &&
	    !feof(stdin) )  {
top:
	    	if( argc > 1 )
			(void)fprintf(stderr,"0%o (%c)\n", record.u_id, record.u_id);
		/* Check record type and skip deleted records */
	    	switch( record.u_id )  {
	    	case ID_FREE:
			continue;
	    	case ID_SOLID:
			soldump();
			continue;
	    	case ID_COMB:
			if( combdump() > 0 )  goto top;
			continue;
	    	case ID_MEMB:
	    		(void)fprintf(stderr, "g2asc: stray MEMB record, skipped\n");
	    		continue;
	    	case ID_ARS_A:
			arsadump();
	    		continue;
	    	case ID_P_HEAD:
			polyhead();
	    		continue;
	    	case ID_P_DATA:
			polydata();
	    		continue;
	    	case ID_IDENT:
			idendump();
	    		continue;
	    	case ID_MATERIAL:
			materdump();
	    		continue;
	    	case DBID_PIPE:
	    		pipe_dump();
	    		continue;
	    	case DBID_PARTICLE:
	    		particle_dump();
	    		continue;
	    	case DBID_ARBN:
	    		arbn_dump();
	    		continue;
	    	case ID_BSOLID:
			bspldump();
	    		continue;
	    	case ID_BSURF:
			bsurfdump();
	    		continue;
	    	default:
			(void)fprintf(stderr,
				"g2asc: unable to convert record type '%c' (0%o), skipping\n",
				record.u_id, record.u_id);
	    		continue;
		}
	}
	exit(0);
}

/*
 *			G E T _ E X T
 *
 *  Take "ngran" granueles, and put them in memory.
 *  The first granule comes from the global extern "record",
 *  the remainder are read from stdin.
 */
void
get_ext( ep, ngran )
struct rt_external	*ep;
int			ngran;
{
	int	count;

	RT_INIT_EXTERNAL(ep);

	ep->ext_nbytes = ngran * sizeof(union record);
	ep->ext_buf = (genptr_t)rt_malloc( ep->ext_nbytes, "get_ext ext_buf" );

	/* Copy the freebie (first) record into the array of records.  */
	bcopy( (char *)&record, (char *)ep->ext_buf, sizeof(union record) );
	if( ngran <= 1 )  return;

	count = fread( ((char *)ep->ext_buf)+sizeof(union record),
		sizeof(union record), ngran-1, stdin);
	if( count != ngran-1 )  {
		fprintf(stderr,
			"g2asc: get_ext:  wanted to read %d granules, got %d\n",
			ngran-1, count);
		exit(1);
	}
}

void
idendump()	/* Print out Ident record information */
{
	(void)printf( "%c %d %.6s\n",
		record.i.i_id,			/* I */
		record.i.i_units,		/* units */
		CH(record.i.i_version)		/* version */
	);
	(void)printf( "%.72s\n",
		CH(record.i.i_title)	/* title or description */
	);

	/* Print a warning message on stderr if versions differ */
	if( strcmp( record.i.i_version, ID_VERSION ) != 0 )  {
		(void)fprintf(stderr,
			"g2asc: File is version (%s), Program is version (%s)\n",
			record.i.i_version, ID_VERSION );
	}
}

void
polyhead()	/* Print out Polyhead record information */
{
	(void)printf("%c ", record.p.p_id );		/* P */
	(void)printf("%.16s", name(record.p.p_name) );	/* unique name */
	(void)printf("\n");			/* Terminate w/ a newline */
}

void
polydata()	/* Print out Polydata record information */
{
	register int i, j;

	(void)printf("%c ", record.q.q_id );		/* Q */
	(void)printf("%d ", record.q.q_count );		/* # of vertices <= 5 */
	for( i = 0; i < 5; i++ )  {			/* [5][3] vertices */
		for( j = 0; j < 3; j++ ) {
			(void)printf("%.12e ", record.q.q_verts[i][j] );
		}
	}
	for( i = 0; i < 5; i++ )  {			/* [5][3] normals */
		for( j = 0; j < 3; j++ ) {
			(void)printf("%.12e ", record.q.q_norms[i][j] );
		}
	}
	(void)printf("\n");			/* Terminate w/ a newline */
}

void
soldump()	/* Print out Solid record information */
{
	register int i;

	(void)printf("%c ", record.s.s_id );	/* S */
	(void)printf("%d ", record.s.s_type );	/* GED primitive type */
	(void)printf("%.16s ", name(record.s.s_name) );	/* unique name */
	(void)printf("%d ", record.s.s_cgtype );/* COMGEOM solid type */
	for( i = 0; i < 24; i++ )
		(void)printf("%.12e ", record.s.s_values[i] ); /* parameters */
	(void)printf("\n");			/* Terminate w/ a newline */
}

void
pipe_dump()	/* Print out Pipe record information */
{

	int			ngranules;	/* number of granules, total */
	int			count;
	int			ret;
	char			*name;
	struct rt_pipe_internal	*pipe;		/* want a struct for the head, not a ptr. */
	struct wdb_pipeseg	head;		/* actual head, not a ptr. */
	struct rt_external	ext;
	struct rt_db_internal	intern;

	ngranules = rt_glong(record.pw.pw_count)+1;
	name = record.pw.pw_name;

	get_ext( &ext, ngranules );

	/* Hand off to librt's import() routine */
	if( (ret = rt_pipe_import( &intern, &ext, id_mat )) != 0 )  {
		fprintf(stderr, "g2asc: pipe import failure\n");
		exit(-1);
	}

	pipe = (struct rt_pipe_internal *)intern.idb_ptr;
	RT_PIPE_CK_MAGIC(pipe);

	/* send the doubly linked list off to dump_pipe_segs(), which
	 * will print all the information.
	 */

	dump_pipe_segs(name, &pipe->pipe_segs_head);

	rt_pipe_ifree( &intern );
	db_free_external( &ext );
}

void
dump_pipe_segs(name, headp)
char			*name;
struct wdb_pipeseg	*headp;
{

	struct wdb_pipeseg	*sp;

	printf("%c %.16s\n", DBID_PIPE, name);

	/* print parameters for each segment: one segment per line */

	for( RT_LIST( sp, wdb_pipeseg, &(headp->l) ) )  {
		switch(sp->ps_type)  {
		case WDB_PIPESEG_TYPE_END:
			printf("end %26.20e %26.20e %26.20e %26.20e %26.20e\n",
				sp->ps_id, sp->ps_od,
				sp->ps_start[X],
				sp->ps_start[Y],
				sp->ps_start[Z] );
			break;
		case WDB_PIPESEG_TYPE_LINEAR:
			printf("linear %26.20e %26.20e %26.20e %26.20e %26.20e\n",
				sp->ps_id, sp->ps_od,
				sp->ps_start[X],
				sp->ps_start[Y],
				sp->ps_start[Z] );
			break;
		case WDB_PIPESEG_TYPE_BEND:
			printf("bend %26.20e %26.20e %26.20e %26.20e %26.20e %26.20e %26.20e %26.20e\n",
				sp->ps_id, sp->ps_od,
				sp->ps_start[X],
				sp->ps_start[Y],
				sp->ps_start[Z],
				sp->ps_bendcenter[X],
				sp->ps_bendcenter[Y],
				sp->ps_bendcenter[Z]);
			break;
		default:
			fprintf(stderr, "g2asc: unknown pipe type %d\n",
				sp->ps_type);
			break;
		}
	}
}

/*
 * Print out Particle record information.
 * Note that particles fit into one granule only.
 */
void
particle_dump()
{
	int			ret;
	char			*type;
	struct rt_part_internal 	*part;	/* head for the structure */
	struct rt_external	ext;
	struct rt_db_internal	intern;

	get_ext( &ext, 1 );

	/* Hand off to librt's import() routine */
	if( (ret = rt_part_import( &intern, &ext, id_mat )) != 0 )  {
		fprintf(stderr, "g2asc: particle import failure\n");
		exit(-1);
	}

	part = (struct rt_part_internal *)intern.idb_ptr;
	RT_PART_CK_MAGIC(part);
	
	/* Particle type is picked up on here merely to ensure receiving
	 * valid data.  The type is not used any further.
	 */

	switch( part->part_type )  {
	case RT_PARTICLE_TYPE_SPHERE:
		type = "sphere";
		break;
	case RT_PARTICLE_TYPE_CYLINDER:
		type = "cylinder";
		break;
	case RT_PARTICLE_TYPE_CONE:
		type = "cone";
		break;
	default:
		fprintf(stderr, "g2asc: no particle type %d\n", part->part_type);
		exit(-1);
	}

	printf("%c %.16s %26.20e %26.20e %26.20e %26.20e %26.20e %26.20e %26.20e %26.20e\n",
		record.part.p_id, record.part.p_name,
		part->part_V[X],
		part->part_V[Y],
		part->part_V[Z],
		part->part_H[X],
		part->part_H[Y],
		part->part_H[Z],
		part->part_vrad, part->part_hrad);
}


/*			A R B N _ D U M P
 *
 *  Print out arbn information.
 *
 */
void
arbn_dump()
{
	int		ngranules;	/* number of granules to be read */
	int		ret;		/* return code catcher */
	int		i;		/* a counter */
	char		*name;
	struct rt_arbn_internal	*arbn;
	struct rt_external	ext;
	struct rt_db_internal	intern;

	ngranules = rt_glong(record.n.n_grans)+1;
	name = record.n.n_name;

	get_ext( &ext, ngranules );

	/* Hand off to librt's import() routine */
	if( (ret = rt_arbn_import( &intern, &ext, id_mat )) != 0 )  {
		fprintf(stderr, "g2asc: arbn import failure\n");
		exit(-1);
	}

	arbn = (struct rt_arbn_internal *)intern.idb_ptr;
	RT_ARBN_CK_MAGIC(arbn);

	fprintf(stdout, "%c %.16s %d\n", 'n', name, arbn->neqn);
	for( i = 0; i < arbn->neqn; i++ )  {
		printf("n %26.20e %20.26e %26.20e %26.20e\n",
			arbn->eqn[i][X], arbn->eqn[i][Y],
			arbn->eqn[i][Z], arbn->eqn[i][3]);
	}

	rt_arbn_ifree( &intern );
	db_free_external( &ext );
}

	
/*
 *			C O M B D U M P
 *
 *  Note that for compatability with programs such as FRED that
 *  (inappropriately) read .asc files, the member count has to be
 *  recalculated here.
 *
 *  Returns -
 *	0	converted OK
 *	1	converted OK, left next record in global "record" for reuse.
 */
int
combdump()	/* Print out Combination record information */
{
	register int i;
	register int length;	/* Keep track of number of members */
	int	m1, m2;		/* material property flags */
	struct rt_list	head;
	struct mchain {
		struct rt_list	l;
		union record	r;
	};
	struct mchain	*mp;
	struct mchain	*ret_mp = (struct mchain *)0;
	int		mcount;

	/*
	 *  Gobble up all subsequent member records, so that
	 *  an accurate count of them can be output.
	 */
	RT_LIST_INIT( &head );
	mcount = 0;
	while(1)  {
		GETSTRUCT( mp, mchain );
		if( fread( (char *)&mp->r, sizeof(mp->r), 1, stdin ) != 1
		    || feof( stdin ) )
			break;
		if( mp->r.u_id != ID_MEMB )  {
			ret_mp = mp;	/* Handle it later */
			break;
		}
		RT_LIST_INSERT( &head, &(mp->l) );
		mcount++;
	}

	/*
	 *  Output the combination
	 */
	(void)printf("%c ", record.c.c_id );		/* C */
	if( record.c.c_flags == 'R' )			/* set region flag */
		(void)printf("Y ");			/* Y if `R' */
	else
		(void)printf("N ");			/* N if ` ' */
	(void)printf("%.16s ", name(record.c.c_name) );	/* unique name */
	(void)printf("%d ", record.c.c_regionid );	/* region ID code */
	(void)printf("%d ", record.c.c_aircode );	/* air space code */
	(void)printf("%d ", mcount );       		/* DEPRECATED: # of members */
#if 1
	(void)printf("%d ", 0 );			/* DEPRECATED: COMGEOM region # */
#else
	(void)printf("%d ", record.c.c_num );           /* DEPRECATED: COMGEOM region # */
#endif
	(void)printf("%d ", record.c.c_material );	/* material code */
	(void)printf("%d ", record.c.c_los );		/* equiv. LOS est. */
	(void)printf("%d %d %d %d ",
		record.c.c_override ? 1 : 0,
		record.c.c_rgb[0],
		record.c.c_rgb[1],
		record.c.c_rgb[2] );
	m1 = m2 = 0;
	if( isascii(record.c.c_matname[0]) && isprint(record.c.c_matname[0]) )  {
		m1 = 1;
		if( record.c.c_matparm[0] )
			m2 = 1;
	}
	printf("%d %d ", m1, m2 );
	switch( record.c.c_inherit )  {
	case DB_INH_HIGHER:
		printf("%d ", DB_INH_HIGHER );
		break;
	default:
	case DB_INH_LOWER:
		printf("%d ", DB_INH_LOWER );
		break;
	}
	(void)printf("\n");			/* Terminate w/ a newline */

	if( m1 )
		(void)printf("%.32s\n", CH(record.c.c_matname) );
	if( m2 )
		(void)printf("%.60s\n", CH(record.c.c_matparm) );

	/*
	 *  Output the member records now
	 */
	while( RT_LIST_WHILE( mp, mchain, &head ) )  {
		membdump( &mp->r );
		RT_LIST_DEQUEUE( &mp->l );
		rt_free( (char *)mp, "mchain");
	}

	if( ret_mp )  {
		bcopy( (char *)&ret_mp->r, (char *)&record, sizeof(record) );
		rt_free( (char *)ret_mp, "mchain");
		return 1;
	}
	return 0;
}

/*
 *			M E M B D U M P
 *
 *  Print out Member record information.
 *  Intented to be called by combdump only.
 */
void
membdump(rp)
union record	*rp;
{
	register int i;

	(void)printf("%c ", rp->M.m_id );		/* M */
	(void)printf("%c ", rp->M.m_relation );	/* Boolean oper. */
	(void)printf("%.16s ", name(rp->M.m_instname) );	/* referred-to obj. */
	for( i = 0; i < 16; i++ )			/* homogeneous transform matrix */
		(void)printf("%.12e ", rp->M.m_mat[i] );
	(void)printf("%d ", 0 );			/* was COMGEOM solid # */
	(void)printf("\n");				/* Terminate w/ nl */
}

void
arsadump()	/* Print out ARS record information */
{
	register int i;
	register int length;	/* Keep track of number of ARS B records */

	(void)printf("%c ", record.a.a_id );	/* A */
	(void)printf("%d ", record.a.a_type );	/* primitive type */
	(void)printf("%.16s ", name(record.a.a_name) );	/* unique name */
	(void)printf("%d ", record.a.a_m );	/* # of curves */
	(void)printf("%d ", record.a.a_n );	/* # of points per curve */
	(void)printf("%d ", record.a.a_curlen );/* # of granules per curve */
	(void)printf("%d ", record.a.a_totlen );/* # of granules for ARS */
	(void)printf("%.12e ", record.a.a_xmax );	/* max x coordinate */
	(void)printf("%.12e ", record.a.a_xmin );	/* min x coordinate */
	(void)printf("%.12e ", record.a.a_ymax );	/* max y coordinate */
	(void)printf("%.12e ", record.a.a_ymin );	/* min y coordinate */
	(void)printf("%.12e ", record.a.a_zmax );	/* max z coordinate */
	(void)printf("%.12e ", record.a.a_zmin );	/* min z coordinate */
	(void)printf("\n");			/* Terminate w/ a newline */
			
	length = (int)record.a.a_totlen;	/* Get # of ARS B records */

	for( i = 0; i < length; i++ )  {
		arsbdump();
	}
}

void
arsbdump()	/* Print out ARS B record information */
{
	register int i;
	
	/* Read in a member record for processing */
	(void)fread( (char *)&record, sizeof record, 1, stdin );
	(void)printf("%c ", record.b.b_id );		/* B */
	(void)printf("%d ", record.b.b_type );		/* primitive type */
	(void)printf("%d ", record.b.b_n );		/* current curve # */
	(void)printf("%d ", record.b.b_ngranule );	/* current granule */
	for( i = 0; i < 24; i++ )  {			/* [8*3] vectors */
		(void)printf("%.12e ", record.b.b_values[i] );
	}
	(void)printf("\n");			/* Terminate w/ a newline */
}

void
materdump()	/* Print out material description record information */
{
	(void)printf( "%c %d %d %d %d %d %d\n",
		record.md.md_id,			/* m */
		record.md.md_flags,			/* UNUSED */
		record.md.md_low,	/* low end of region IDs affected */
		record.md.md_hi,	/* high end of region IDs affected */
		record.md.md_r,
		record.md.md_g,		/* color of regions: 0..255 */
		record.md.md_b );
}

void
bspldump()	/* Print out B-spline solid description record information */
{
	(void)printf( "%c %.16s %d %.12e\n",
		record.B.B_id,		/* b */
		name(record.B.B_name),	/* unique name */
		record.B.B_nsurf,	/* # of surfaces in this solid */
		record.B.B_resolution );	/* resolution of flatness */
}

void
bsurfdump()	/* Print d-spline surface description record information */
{
	register int i;
	register float *vp;
	int nbytes, count;
	float *fp;

	(void)printf( "%c %d %d %d %d %d %d %d %d %d\n",
		record.d.d_id,		/* D */
		record.d.d_order[0],	/* order of u and v directions */
		record.d.d_order[1],	/* order of u and v directions */
		record.d.d_kv_size[0],	/* knot vector size (u and v) */
		record.d.d_kv_size[1],	/* knot vector size (u and v) */
		record.d.d_ctl_size[0],	/* control mesh size (u and v) */
		record.d.d_ctl_size[1],	/* control mesh size (u and v) */
		record.d.d_geom_type,	/* geom type 3 or 4 */
		record.d.d_nknots,	/* # granules of knots */
		record.d.d_nctls );	/* # granules of ctls */
	/* 
	 * The b_surf_head record is followed by
	 * d_nknots granules of knot vectors (first u, then v),
	 * and then by d_nctls granules of control mesh information.
	 * Note that neither of these have an ID field!
	 *
	 * B-spline surface record, followed by
	 *	d_kv_size[0] floats,
	 *	d_kv_size[1] floats,
	 *	padded to d_nknots granules, followed by
	 *	ctl_size[0]*ctl_size[1]*geom_type floats,
	 *	padded to d_nctls granules.
	 *
	 * IMPORTANT NOTE: granule == sizeof(union record)
	 */

	/* Malloc and clear memory for the KNOT DATA and read it */
	nbytes = record.d.d_nknots * sizeof(union record);
	if( (vp = (float *)malloc(nbytes))  == (float *)0 )  {
		(void)fprintf(stderr, "g2asc: spline knot malloc error\n");
		exit(1);
	}
	fp = vp;
	(void)bzero( (char *)fp, nbytes );
	count = fread( (char *)fp, 1, nbytes, stdin );
	if( count != nbytes )  {
		(void)fprintf(stderr, "g2asc: spline knot read failure\n");
		exit(1);
	}
	/* Print the knot vector information */
	count = record.d.d_kv_size[0] + record.d.d_kv_size[1];
	for( i = 0; i < count; i++ )  {
		(void)printf("%.12e\n", *vp++);
	}
	/* Free the knot data memory */
	(void)free( (char *)fp );

	/* Malloc and clear memory for the CONTROL MESH data and read it */
	nbytes = record.d.d_nctls * sizeof(union record);
	if( (vp = (float *)malloc(nbytes))  == (float *)0 )  {
		(void)fprintf(stderr, "g2asc: control mesh malloc error\n");
		exit(1);
	}
	fp = vp;
	(void)bzero( (char *)fp, nbytes );
	count = fread( (char *)fp, 1, nbytes, stdin );
	if( count != nbytes )  {
		(void)fprintf(stderr, "g2asc: control mesh read failure\n");
		exit(1);
	}
	/* Print the control mesh information */
	count = record.d.d_ctl_size[0] * record.d.d_ctl_size[1] *
		record.d.d_geom_type;
	for( i = 0; i < count; i++ )  {
		(void)printf("%.12e\n", *vp++);
	}
	/* Free the control mesh memory */
	(void)free( (char *)fp );
}

/*
 *			N A M E
 *
 *  Take a database name and null-terminate it,
 *  converting unprintable characters to something printable.
 *  Here we deal with NAMESIZE long names not being null-terminated.
 */
char *name( str )
char *str;
{
	static char buf[NAMESIZE+2];
	register char *ip = str;
	register char *op = buf;
	register int warn = 0;

	while( op < &buf[NAMESIZE] )  {
		if( *ip == '\0' )  break;
		if( isascii(*ip) && isprint(*ip) && !isspace(*ip) )  {
			*op++ = *ip++;
		}  else  {
			*op++ = '@';
			ip++;
			warn = 1;
		}
	}
	*op = '\0';
	if(warn)  {
		(void)fprintf(stderr,
			"g2asc: Illegal char in object name, converted to '%s'\n",
			buf );
	}
	if( op == buf )  {
		/* Null input name */
		(void)fprintf(stderr,
			"g2asc:  NULL object name converted to -=NULL=-\n");
		return("-=NULL=-");
	}
	return(buf);
}

/*
 *			S T R C H O P
 *
 *  Take a string and a length, and null terminate,
 *  converting unprintable characters to something printable.
 */
char *strchop( str, len )
char *str;
{
	static char buf[1024];
	register char *ip = str;
	register char *op = buf;
	register int warn = 0;
	char *ep;

	if( len > sizeof(buf)-2 )  len=sizeof(buf)-2;
	ep = &buf[len-1];		/* Leave room for null */
	while( op < ep )  {
		if( *ip == '\0' )  break;
		if( isascii(*ip) && (isprint(*ip) || isspace(*ip)) )  {
			*op++ = *ip++;
		}  else  {
			*op++ = '@';
			ip++;
			warn = 1;
		}
	}
	*op = '\0';
	if(warn)  {
		(void)fprintf(stderr,
			"g2asc: Illegal char in string, converted to '%s'\n",
			buf );
	}
	if( op == buf )  {
		/* Null input name */
		(void)fprintf(stderr,
			"g2asc:  NULL string converted to -=STRING=-\n");
		return("-=STRING=-");
	}
	return(buf);
}
