/*                         B E V . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file bev.c
 *
 * The bev command.
 *
 */

#include "common.h"
#include "bio.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "rtgeom.h"
#include "ged_private.h"

static union tree *ged_facetize_region_end(register struct db_tree_state *tsp, struct db_full_path *pathp, union tree *curtree, genptr_t client_data);

static union tree *ged_facetize_tree;
static struct model *ged_nmg_model;

int
ged_bev(struct ged *gedp, int argc, const char *argv[])
{
    int			i;
    register int	c;
    int			ncpu;
    int			triangulate;
    char			*cmdname;
    char			*newname;
    struct rt_db_internal	intern;
    struct directory	*dp;
    union tree		*tmp_tree;
    char		op;
    int			failed;
    static const char *usage = "[P|t] new_obj obj1 op obj2 op obj3 ...";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);
    gedp->ged_result = GED_RESULT_NULL;
    gedp->ged_result_flags = 0;

    /* must be wanting help */
    if (argc == 1) {
	gedp->ged_result_flags |= GED_RESULT_FLAGS_HELP_BIT;
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_OK;
    }

    if (argc < 3) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    cmdname = (char *)argv[0];

    /* Establish tolerances */
    gedp->ged_wdbp->wdb_initial_tree_state.ts_ttol = &gedp->ged_wdbp->wdb_ttol;
    gedp->ged_wdbp->wdb_initial_tree_state.ts_tol = &gedp->ged_wdbp->wdb_tol;

    gedp->ged_wdbp->wdb_ttol.magic = RT_TESS_TOL_MAGIC;

    /* Initial values for options, must be reset each time */
    ncpu = 1;
    triangulate = 0;

    /* Parse options. */
    bu_optind = 1;		/* re-init bu_getopt() */
    while ( (c=bu_getopt(argc, (char * const *)argv, "tP:")) != EOF )  {
	switch (c)  {
	    case 'P':
#if 0
		/* not yet supported */
		ncpu = atoi(bu_optarg);
#endif
		break;
	    case 't':
		triangulate = 1;
		break;
	    default:
	    {
		bu_vls_printf(&gedp->ged_result_str, "%s: option '%c' unknown\n", cmdname, c);
	    }

	    break;
	}
    }
    argc -= bu_optind;
    argv += bu_optind;

    newname = (char *)argv[0];
    argv++;
    argc--;

    if (argc < 1) {
	bu_vls_printf(&gedp->ged_result_str, "%s: Nothing to evaluate!!!\n", cmdname);
	return BRLCAD_ERROR;
    }

    if ( db_lookup( gedp->ged_wdbp->dbip, newname, LOOKUP_QUIET ) != DIR_NULL )  {
	bu_vls_printf(&gedp->ged_result_str, "%s: solid '%s' already exists, aborting\n", cmdname, newname);
	return BRLCAD_ERROR;
    }

    bu_vls_printf(&gedp->ged_result_str,
		  "%s:  tessellating primitives with tolerances a=%g, r=%g, n=%g\n",
		  argv[0],
		  gedp->ged_wdbp->wdb_ttol.abs,
		  gedp->ged_wdbp->wdb_ttol.rel,
		  gedp->ged_wdbp->wdb_ttol.norm);

    ged_facetize_tree = (union tree *)0;
    ged_nmg_model = nmg_mm();
    gedp->ged_wdbp->wdb_initial_tree_state.ts_m = &ged_nmg_model;

    op = ' ';
    tmp_tree = (union tree *)NULL;

    while ( argc )
    {
	i = db_walk_tree( gedp->ged_wdbp->dbip, 1, (const char **)argv,
			  ncpu,
			  &gedp->ged_wdbp->wdb_initial_tree_state,
			  0,			/* take all regions */
			  ged_facetize_region_end,
			  nmg_booltree_leaf_tess,
			  (genptr_t)gedp );

	if ( i < 0 )  {
	    bu_vls_printf(&gedp->ged_result_str, "%s: error in db_walk_tree()\n", cmdname);
	    /* Destroy NMG */
	    nmg_km( ged_nmg_model );
	    return BRLCAD_ERROR;
	}
	argc--;
	argv++;

	if ( tmp_tree && op != ' ' )
	{
	    union tree *new_tree;

	    BU_GETUNION( new_tree, tree );

	    new_tree->magic = RT_TREE_MAGIC;
	    new_tree->tr_b.tb_regionp = REGION_NULL;
	    new_tree->tr_b.tb_left = tmp_tree;
	    new_tree->tr_b.tb_right = ged_facetize_tree;

	    switch ( op )
	    {
		case 'u':
		case 'U':
		    new_tree->tr_op = OP_UNION;
		    break;
		case '-':
		    new_tree->tr_op = OP_SUBTRACT;
		    break;
		case '+':
		    new_tree->tr_op = OP_INTERSECT;
		    break;
		default:
		{
		    bu_vls_printf(&gedp->ged_result_str, "%s: Unrecognized operator: (%c)\nAborting\n",
				  argv[0], op);
		    db_free_tree( ged_facetize_tree, &rt_uniresource );
		    nmg_km( ged_nmg_model );
		    return BRLCAD_ERROR;
		}
	    }

	    tmp_tree = new_tree;
	    ged_facetize_tree = (union tree *)NULL;
	}
	else if ( !tmp_tree && op == ' ' )
	{
	    /* just starting out */
	    tmp_tree = ged_facetize_tree;
	    ged_facetize_tree = (union tree *)NULL;
	}

	if ( argc )
	{
	    op = *argv[0];
	    argc--;
	    argv++;
	}
	else
	    op = ' ';

    }

    if ( tmp_tree )
    {
	/* Now, evaluate the boolean tree into ONE region */
	bu_vls_printf(&gedp->ged_result_str, "%s: evaluating boolean expressions\n", cmdname);

	if ( BU_SETJUMP )
	{
	    BU_UNSETJUMP;

	    bu_vls_printf(&gedp->ged_result_str, "%s: WARNING: Boolean evaluation failed!!!\n", cmdname);
	    if ( tmp_tree )
		db_free_tree( tmp_tree, &rt_uniresource );
	    tmp_tree = (union tree *)NULL;
	    nmg_km( ged_nmg_model );
	    ged_nmg_model = (struct model *)NULL;
	    return BRLCAD_ERROR;
	}

	failed = nmg_boolean( tmp_tree, ged_nmg_model, &gedp->ged_wdbp->wdb_tol, &rt_uniresource );
	BU_UNSETJUMP;
    }
    else
	failed = 1;

    if ( failed )  {
	bu_vls_printf(&gedp->ged_result_str, "%s: no resulting region, aborting\n", cmdname);
	if ( tmp_tree )
	    db_free_tree( tmp_tree, &rt_uniresource );
	tmp_tree = (union tree *)NULL;
	nmg_km( ged_nmg_model );
	ged_nmg_model = (struct model *)NULL;
	return BRLCAD_ERROR;
    }
    /* New region remains part of this nmg "model" */
    NMG_CK_REGION( tmp_tree->tr_d.td_r );
    bu_vls_printf(&gedp->ged_result_str, "%s: facetize %s\n", cmdname, tmp_tree->tr_d.td_name);

    nmg_vmodel( ged_nmg_model );

    /* Triangulate model, if requested */
    if ( triangulate )
    {
	bu_vls_printf(&gedp->ged_result_str, "%s: triangulating resulting object\n", cmdname);
	if ( BU_SETJUMP )
	{
	    BU_UNSETJUMP;
	    bu_vls_printf(&gedp->ged_result_str, "%s: WARNING: Triangulation failed!!!\n", cmdname);
	    if ( tmp_tree )
		db_free_tree( tmp_tree, &rt_uniresource );
	    tmp_tree = (union tree *)NULL;
	    nmg_km( ged_nmg_model );
	    ged_nmg_model = (struct model *)NULL;
	    return BRLCAD_ERROR;
	}
	nmg_triangulate_model( ged_nmg_model, &gedp->ged_wdbp->wdb_tol );
	BU_UNSETJUMP;
    }

    bu_vls_printf(&gedp->ged_result_str, "%s: converting NMG to database format\n", cmdname);

    /* Export NMG as a new solid */
    RT_INIT_DB_INTERNAL(&intern);
    intern.idb_major_type = DB5_MAJORTYPE_BRLCAD;
    intern.idb_type = ID_NMG;
    intern.idb_meth = &rt_functab[ID_NMG];
    intern.idb_ptr = (genptr_t)ged_nmg_model;
    ged_nmg_model = (struct model *)NULL;

    if ( (dp=db_diradd( gedp->ged_wdbp->dbip, newname, -1L, 0, DIR_SOLID, (genptr_t)&intern.idb_type)) == DIR_NULL ) {
	bu_vls_printf(&gedp->ged_result_str, "%s: Cannot add %s to directory\n", cmdname, newname);
	return BRLCAD_ERROR;
    }

    if ( rt_db_put_internal( dp, gedp->ged_wdbp->dbip, &intern, &rt_uniresource ) < 0 )
    {
	rt_db_free_internal( &intern, &rt_uniresource );
	bu_vls_printf(&gedp->ged_result_str, "%s: Database write error, aborting\n", cmdname);
	return BRLCAD_ERROR;
    }

    tmp_tree->tr_d.td_r = (struct nmgregion *)NULL;

    /* Free boolean tree, and the regions in it. */
    db_free_tree( tmp_tree, &rt_uniresource );

    return BRLCAD_OK;
}

/*
 *			M G E D _ F A C E T I Z E _ R E G I O N _ E N D
 *
 *  This routine must be prepared to run in parallel.
 */
static union tree *
ged_facetize_region_end(register struct db_tree_state *tsp, struct db_full_path *pathp, union tree *curtree, genptr_t client_data)
{
    struct bu_list vhead;
    struct ged *gedp = (struct ged *)client_data;

    BU_LIST_INIT( &vhead );

    if (RT_G_DEBUG&DEBUG_TREEWALK)  {
	char	*sofar = db_path_to_string(pathp);

	bu_vls_printf(&gedp->ged_result_str, "ged_facetize_region_end() path='%s'\n", sofar);
	bu_free((genptr_t)sofar, "path string");
    }

    if ( curtree->tr_op == OP_NOP )  return  curtree;

    bu_semaphore_acquire( RT_SEM_MODEL );
    if ( ged_facetize_tree )  {
	union tree	*tr;
	tr = (union tree *)bu_calloc(1, sizeof(union tree), "union tree");
	tr->magic = RT_TREE_MAGIC;
	tr->tr_op = OP_UNION;
	tr->tr_b.tb_regionp = REGION_NULL;
	tr->tr_b.tb_left = ged_facetize_tree;
	tr->tr_b.tb_right = curtree;
	ged_facetize_tree = tr;
    } else {
	ged_facetize_tree = curtree;
    }
    bu_semaphore_release( RT_SEM_MODEL );

    /* Tree has been saved, and will be freed later */
    return( TREE_NULL );
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
