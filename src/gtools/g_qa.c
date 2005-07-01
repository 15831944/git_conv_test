/*
 *
 *	Designed to be a framework for 3d sampling of the geometry volume.
 *	Options
 *	h	help
 */
#include <stdlib.h>

#include "common.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>			/* home of INT_MAX aka MAXINT */
#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "raytrace.h"
#include "plot3.h"

#define SEM_WORK RT_SEM_LAST

/* declarations to support use of getopt() system call */
char *options = "hP:g:df:t:";
extern char *optarg;
extern int optind, opterr, getopt();

/* variables set by command line flags */
char *progname = "(noname)";
int npsw = 1;
double grid_size = 50.0; /* 50.0mm grid default */
int debug;
#define dlog if (debug) bu_log
char *densityFileName;
double tol = 0.5;


struct resource	resource[MAX_PSW];	/* memory resources for multi-cpu processing */

struct state {
    int u_axis; /* these three are in the range 0..2 inclusive and indicate which axis (X,Y,or Z) */
    int v_axis; /* is being used for the U, V, or invariant vector direction */
    int i_axis;
    int v;	/* this indicates how many "grid_size" steps in the v direction have been taken */
    vect_t lenDensity;
    vect_t volume;
    struct rt_i *rtip;
};


long steps[3]; /* # of "grid_size" steps to take along each axis (X,Y,Z) to cover the face of the bounding rpp? */

vect_t span; /* How much space does the geometry span in each of X,Y,Z directions */


struct density_entry {
    long	magic;
    double	density;
    char	*name;
} *densities;
int num_densities;
#define DENSITY_MAGIC 0xaf0127

/*
 *	U S A G E --- tell user how to invoke this program, then exit
 */
void
usage(s)
     char *s;
{
    if (s) (void)fputs(s, stderr);

    (void) fprintf(stderr, "Usage: %s [-P #processors] [-g initial_gridsize] [-t vol_tolerance] [-f density_table_file] [-d(ebug)] geom.g obj [obj...]\n",
		   progname);
    exit(1);
}

/*
 *	P A R S E _ A R G S --- Parse through command line flags
 */
int
parse_args(ac, av)
     int ac;
     char *av[];
{
    int  c;
    char *strrchr();
    char unitstr[64];
    double conv;

    if (  ! (progname=strrchr(*av, '/'))  )
	progname = *av;
    else
	++progname;

    /* Turn off getopt's error messages */
    opterr = 0;

    /* get all the option flags from the command line */
    while ((c=getopt(ac,av,options)) != EOF)
	switch (c) {
	case 't'	: tol = strtod(optarg, (char **)NULL); break;
	case 'f'	: densityFileName = optarg; break;
	case 'd'	: debug = !debug; break;
	case 'P'	:
	    if ((c=atoi(optarg)) > 0) npsw = c;
	    break;	
	case 'g'	:
	    sscanf(optarg, "%lg%60s", &grid_size, unitstr); 
	    conv = bu_units_conversion(unitstr);
	    if (conv != 0.0) grid_size *= conv;
	    break;
	case '?'	:
	case 'h'	:
	default		: usage("Bad or help flag specified\n"); break;
	}

    return(optind);
}

void
multioverlap(struct application *ap, struct partition *pp, struct bu_ptbl *regiontable, struct partition *InputHdp)
{
    int n_regions;

    RT_CK_AP(ap);
    RT_CK_PARTITION(pp);
    BU_CK_PTBL(regiontable);
    RT_CK_PT_HD(InputHdp);
    struct region **reg;

    /* just blow em away for now */
    for( BU_PTBL_FOR( reg, (struct region **), regiontable ) )  {
	if (*reg == REGION_NULL) continue;
	
	*reg = REGION_NULL;
    }
}



void
logoverlap(struct application *ap, const struct partition *pp, const struct bu_ptbl *regiontable, const struct partition *InputHdp)
{
    RT_CK_AP(ap);
    RT_CK_PT(pp);
    BU_CK_PTBL(regiontable);
    return;
}


/*
 *  rt_shootray() was told to call this on a hit.  He gives up the
 *  application structure which describes the state of the world
 *  (see raytrace.h), and a circular linked list of partitions,
 *  each one describing one in and out segment of one region.
 */
hit(register struct application *ap, struct partition *PartHeadp, struct seg *segs)
{
    /* see raytrace.h for all of these guys */
    register struct partition *pp;
    register struct hit *hitp;
    register struct soltab *stp;
    struct curvature cur;
    point_t		pt;
    vect_t		inormal;
    vect_t		onormal;


    /* examine each partition until we get back to the head */
    for( pp=PartHeadp->pt_forw; pp != PartHeadp; pp = pp->pt_forw )  {
	double dist;
	struct density_entry *de;

	/* inhit info */

	dist = pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist;

	/* try to factor in the density of this object */
	if (pp->pt_regionp->reg_gmater >= num_densities) {
	    bu_log("density index %d on region %s is outside of range of table [1..%d]\nSet GIFTmater on region or add entry to density table\n",
		   pp->pt_regionp->reg_gmater,
		   pp->pt_regionp->reg_name,
		   num_densities);
	    bu_bomb("");
	} else {
	    de = &densities[pp->pt_regionp->reg_gmater];
	    if (de->magic == DENSITY_MAGIC) {
		ap->a_color[0] += de->density * dist;
		ap->a_color[1] += dist;
	    } else {
		bu_log("density index %d from region %s is not set.\nAdd entry to density table\n",
		       pp->pt_regionp->reg_gmater, pp->pt_regionp->reg_name);
		bu_bomb("");
	    }
	}
    }

    /*
     * This value is returned by rt_shootray
     * a hit usually returns 1, miss 0.
     */
    return(1);
}

/*
 * rt_shootray() was told to call this on a miss.
 */
miss(register struct application *ap)
{
#if 0
    bu_log("missed\n");
#endif
    return(0);
}


void
plane_worker (int cpu, genptr_t ptr)
{
    struct application ap;
    int u, v;
    double v_coord;
    double z_coord;
    struct state *state = (struct state *)ptr;


    RT_APPLICATION_INIT(&ap);
    ap.a_rt_i = (struct rt_i *)state->rtip;	/* application uses this instance */
    ap.a_hit = hit;			/* where to go on a hit */
    ap.a_miss = miss;		/* where to go on a miss */
    ap.a_logoverlap = logoverlap;
    ap.a_multioverlap = multioverlap;
    ap.a_resource = &resource[cpu];
    ap.a_color[0] = 0.0; /* really the cumulative length*density */
    ap.a_color[1] = 0.0; /* really the cumulative length*density */

    ap.a_ray.r_dir[state->u_axis] = ap.a_ray.r_dir[state->v_axis] = 0.0;
    ap.a_ray.r_dir[state->i_axis] = 1.0;

    while (1) {
	/* get a row to work on */
	bu_semaphore_acquire(SEM_WORK);
	if (state->v >= steps[state->v_axis]) {
	    state->lenDensity[state->i_axis] += ap.a_color[0]; /* add our length*density value */
	    state->volume[state->i_axis] += ap.a_color[1]; /* add our length*density value */
	    bu_semaphore_release(SEM_WORK);
	    return;
	}
	v = state->v++;
	bu_semaphore_release(SEM_WORK);

	v_coord = grid_size * 0.5 + v * grid_size; 

	state->lenDensity[state->i_axis] = 0.0;
	state->volume[state->i_axis] = 0.0;


	if (state->lenDensity[state->i_axis] == 0.0 || (v&1) == 0) {
	    /* shoot all the rays in this row */
	    for (u=0 ; u < steps[state->u_axis]; u++) {
		ap.a_ray.r_pt[state->u_axis] = ap.a_rt_i->mdl_min[state->u_axis] + u * grid_size + 0.5 * grid_size;
		ap.a_ray.r_pt[state->v_axis] = ap.a_rt_i->mdl_min[state->v_axis] + v * grid_size + 0.5 * grid_size;
		ap.a_ray.r_pt[state->i_axis] = ap.a_rt_i->mdl_min[state->i_axis];

		dlog("%g %g %g\n", V3ARGS(ap.a_ray.r_pt));
		(void)rt_shootray( &ap );
	    }
	} else {
	    /* shoot only the rays we need to on this row */
	    for (u=1 ; u < steps[state->u_axis]; u+=2) {
		ap.a_ray.r_pt[state->u_axis] = ap.a_rt_i->mdl_min[state->u_axis] + u * grid_size + 0.5 * grid_size;
		ap.a_ray.r_pt[state->v_axis] = ap.a_rt_i->mdl_min[state->v_axis] + v * grid_size + 0.5 * grid_size;
		ap.a_ray.r_pt[state->i_axis] = ap.a_rt_i->mdl_min[state->i_axis];

		dlog("%g %g %g\n", V3ARGS(ap.a_ray.r_pt));
		(void)rt_shootray( &ap );
	    }
	}
    }
}
void
clear_region_values()
{
}

void
report_results(struct state *state)
{
    state->volume[state->i_axis] *= grid_size*grid_size;

    bu_log("volume %lg  Weight %lg\n", state->volume[state->i_axis], state->lenDensity[state->i_axis]*grid_size*grid_size);
}

void
parse_densities_buffer(char *buf, unsigned long len)
{
    char *p, *q, *last;
    long idx;
    double density;
    char name[128];

    buf[len] = '\0';
    last = &buf[len];

    p = buf;

    densities = bu_malloc(sizeof(struct density_entry)*128, "density entries");
    num_densities = 128;

    while (*p) {
	idx = strtol(p, &q, 10);
	if (q == (char *)NULL) {
	    bu_bomb("could not convert idx\n");
	}

	if (idx < 0) {
	    bu_log("bad density index (< 0) %d\n", idx);
	    bu_bomb("");
	}

	density = strtod(q, &p);
	if (q == p) {
	    bu_bomb("could not convert density\n");
	}
	if (density < 0.0) {
	    bu_log("bad density (< 0) %g\n", density);
	    bu_bomb("");
	}
	
	while (isblank(*p)) p++;

	if (q = strchr(p, '\n')) {
	    *q++ = '\0';
	} else {
	    q = last;
	}

	while (idx >= num_densities) {
	    densities = bu_realloc(densities, sizeof(struct density_entry)*num_densities*2, "density entries");
	    num_densities *= 2;
	}

	densities[idx].magic = DENSITY_MAGIC;
	densities[idx].density = density;
	densities[idx].name = bu_strdup(p);

	p = q;
    } while (p && p < last);

#if 0
    for (idx=0 ; idx < num_densities ; idx++) {
	if (densities[idx].magic == DENSITY_MAGIC) {
	    bu_log("%4d %6g %s\n", 
		   idx,
		   densities[idx].density,
		   densities[idx].name
		   );
	}
    }
#endif
}
/* 
 * Returns
 *	 0 on success
 *	!0 on failure
 */
int
get_densities_from_file(char *name)
{
    FILE *fp;
    struct stat sb;
    char *buf;

    if ((fp=fopen(name, "r")) == (FILE *)NULL) {
	perror(name);
	return 1;
    }

    if (fstat(fileno(fp), &sb)) { 
	perror(name);
	return 1;
    }

    buf = bu_malloc(sb.st_size, "density buffer");
    fread(buf, sb.st_size, 1, fp);
    parse_densities_buffer(buf, (unsigned long)sb.st_size);
    bu_free(buf, "density buffer");
    fclose(fp);
    return 0;
}
/* 
 * Returns
 *	 0 on success
 *	!0 on failure
 */
int
get_densities_from_database(struct rt_i *rtip)
{
    struct directory *dp;
    struct rt_db_internal intern;
    struct rt_binunif_internal *bu;

    dp = db_lookup(rtip->rti_dbip, "_DENSITIES", LOOKUP_QUIET);
    if (dp == (struct directory *)NULL) {
	bu_log("No density table object in database\n");
	return -1;
    }

    if (rt_db_get_internal(&intern, dp, rtip->rti_dbip, NULL, &rt_uniresource) < 0) {
	bu_log("could not import %s\n", dp->d_namep);
	return 1;
    }

    if (intern.idb_major_type&DB5_MAJORTYPE_BINARY_MASK == 0) {
	return 1;
    }


    bu = (struct rt_binunif_internal *)intern.idb_ptr;

    RT_CHECK_BINUNIF(bu);

    parse_densities_buffer(bu->u.int8, bu->count);
    return 0;
}
/*
 */
int
get_densities( struct rt_i * rtip)
{
    struct directory *dp;
}


/*
 *	M A I N
 *
 *	Call parse_args to handle command line arguments first, then
 *	process input.
 */
int
main(ac,av)
     int ac;
     char *av[];
{
    int arg_count;
    FILE *inp;
    int status;
    struct rt_i *rtip;
    int u_axis, v_axis, i_axis;
    long u, v;
    char idbuf[132];
    vect_t origin;
    vect_t dir;
    int i;
    struct directory *dp;
    struct state state;
    double lim, val;


    npsw = bu_avail_cpus();

    arg_count = parse_args(ac, av);
	
    if ((ac-arg_count) < 2) {
	usage("oops\n");
    }

    bu_semaphore_init(RT_SEM_LAST+2);

    rt_init_resource( &rt_uniresource, npsw, NULL );
    bn_rand_init( rt_uniresource.re_randptr, 0 );
	

    /*
     *  Load database.
     *  rt_dirbuild() returns an "instance" pointer which describes
     *  the database to be ray traced.  It also gives back the
     *  title string in the header (ID) record.
     */
    if( (rtip=rt_dirbuild(av[arg_count], idbuf, sizeof(idbuf))) == RTI_NULL ) {
	fprintf(stderr,"rtexample: rt_dirbuild failure\n");
	exit(2);
    }

    if ( densityFileName) {
	bu_log("density from file\n");
	if (get_densities_from_file(densityFileName)) {
	    return -1;
	}
    } else {
	bu_log("density from db\n");
	if (get_densities_from_database(rtip)) {
	    return -1;
	}
    }

    /* Walk trees.
     * Here we identify any object trees in the database that the user
     * wants included in the ray trace.
     */
    for ( arg_count++ ; arg_count < ac ; arg_count++ )  {
	if( rt_gettree(rtip, av[arg_count]) < 0 )
	    fprintf(stderr,"rt_gettree(%s) FAILED\n", av[arg_count]);
    }

    /*
     *  Initialize all the per-CPU memory resources.
     *  The number of processors can change at runtime, init them all.
     */
    for( i=0; i < npsw; i++ )  {
	rt_init_resource( &resource[i], i, rtip );
	bn_rand_init( resource[i].re_randptr, i );
    }

    /*
     * This gets the database ready for ray tracing.
     * (it precomputes some values, sets up space partitioning, etc.)
     */
    rt_prep_parallel(rtip,npsw);


    /* we now have to subdivide space
     *
     */
    bu_log("bounds %g %g %g  %g %g %g\n", V3ARGS(rtip->mdl_min), V3ARGS(rtip->mdl_max));

    VSUB2(span, rtip->mdl_max, rtip->mdl_min);
    VSETALL(state.lenDensity, 0.0);
    VSETALL(state.volume, 0.0);
    state.rtip = rtip;

    lim = tol+1.0;
    bu_log("tol: %g\n", tol);

    while (lim > tol) {

	VSCALE(steps, span, 1.0/grid_size);

	bu_log("grid size %g mm\n", grid_size);

	state.u_axis = 0;
	state.v_axis = 1;
	state.i_axis = 2;
	state.v = 0;
	
	bu_log("\txy %d x %d samples ", steps[X], steps[Y]);
	clear_region_values();
	bu_parallel(plane_worker, npsw, (genptr_t)&state); /* xy plane */
	report_results(&state);

	state.u_axis = 0;
	state.v_axis = 2;
	state.i_axis = 1;
	state.v = 0;

	bu_log("\txz %d x %d samples ", steps[X], steps[Z]);
	clear_region_values();
	bu_parallel(plane_worker, npsw, (genptr_t)&state); /* xz plane */
	report_results(&state);

	state.u_axis = 1;
	state.v_axis = 2;
	state.i_axis = 0;
	state.v = 0;

	bu_log("\tyz %d x %d samples ", steps[Y], steps[Z]);
	clear_region_values();
	bu_parallel(plane_worker, npsw, (genptr_t)&state); /* yz plane */
	report_results(&state);

	lim = fabs(state.volume[0] - state.volume[1]);
	val = fabs(state.volume[0] - state.volume[2]);
	if (lim > val) lim = val;

	val = fabs(state.volume[1] - state.volume[2]);
	if (lim < val) lim = val;

	bu_log("lim %g\n", lim);
	grid_size *= 0.5;

    }

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
