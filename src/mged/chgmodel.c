/*                      C H G M O D E L . C
 * BRL-CAD
 *
 * Copyright (c) 1985-2008 United States Government as represented by
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
/** @file chgmodel.c
 *
 * This module contains functions which change particulars of the
 * model, generally on a single solid or combination.
 * Changes to the tree structure of the model are done in chgtree.c
 *
 */

#include "common.h"

#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "bio.h"

#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "nmg.h"
#include "nurb.h"
#include "rtgeom.h"
#include "ged.h"
#include "wdb.h"

#include "./mged.h"
#include "./mged_solid.h"
#include "./mged_dm.h"
#include "./sedit.h"
#include "./cmd.h"


extern struct bn_tol mged_tol;

void set_tran();
void	aexists(char *name);

static char	tmpfil[MAXPATHLEN];

int		newedge;		/* new edge for arb editing */

/* Add/modify item and air codes of a region */
/* Format: item region item <air>	*/
int
f_itemair(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    Tcl_DString ds;
    int ret;
    
    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    ret = ged_item(wdbp, argc, argv);

    /* Convert to Tcl codes */
    if (ret == GED_OK)
	ret = TCL_OK;
    else
	ret = TCL_ERROR;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, bu_vls_addr(&wdbp->wdb_result_str), -1);
    Tcl_DStringResult(interp, &ds);

    return ret;
}

/* Modify material information */
/* Usage:  mater region_name shader r g b inherit */
int
f_mater(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    register struct directory *dp;
    int r=0, g=0, b=0;
    int skip_args = 0;
    char inherit;
    struct rt_db_internal	intern;
    struct rt_comb_internal	*comb;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 2 || 8 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help mater");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ( (dp = db_lookup( dbip,  argv[1], LOOKUP_NOISY )) == DIR_NULL )
	return TCL_ERROR;
    if ( (dp->d_flags & DIR_COMB) == 0 )  {
	Tcl_AppendResult(interp, dp->d_namep, ": not a combination\n", (char *)NULL);
	return TCL_ERROR;
    }

    if ( rt_db_get_internal( &intern, dp, dbip, (fastf_t *)NULL, &rt_uniresource ) < 0 )  {
	TCL_READ_ERR_return;
    }
    comb = (struct rt_comb_internal *)intern.idb_ptr;
    RT_CK_COMB(comb);

    if ( argc >= 3 )  {
	if ( strncmp( argv[2], "del", 3 ) != 0 )  {
	    /* Material */
	    bu_vls_trunc( &comb->shader, 0 );
	    if ( bu_shader_to_tcl_list( argv[2], &comb->shader ))
	    {
		Tcl_AppendResult(interp, "Problem with shader string: ", argv[2], (char *)NULL );
		return TCL_ERROR;
	    }
	} else {
	    bu_vls_free( &comb->shader );
	}
    } else {
	/* Shader */
	struct bu_vls tmp_vls;

	bu_vls_init( &tmp_vls );
	if ( bu_vls_strlen( &comb->shader ) )
	{
	    if ( bu_shader_to_key_eq( bu_vls_addr(&comb->shader), &tmp_vls ) )
	    {
		Tcl_AppendResult(interp, "Problem with on disk shader string: ", bu_vls_addr(&comb->shader), (char *)NULL );
		bu_vls_free( &tmp_vls );
		return TCL_ERROR;
	    }
	}
	curr_cmd_list->cl_quote_string = 1;
	Tcl_AppendResult(interp, "Shader = ", bu_vls_addr(&tmp_vls),
			 "\n", MORE_ARGS_STR,
			 "Shader?  ('del' to delete, CR to skip) ", (char *)NULL);

	if ( bu_vls_strlen( &comb->shader ) == 0 )
	    bu_vls_printf(&curr_cmd_list->cl_more_default, "del");
	else
	    bu_vls_printf(&curr_cmd_list->cl_more_default, "\"%S\"", &tmp_vls );

	bu_vls_free( &tmp_vls );

	goto fail;
    }

    if (argc >= 4) {
	if ( strncmp(argv[3], "del", 3) == 0 ) {
	    /* leave color as is */
	    comb->rgb_valid = 0;
	    skip_args = 2;
	} else if (argc < 6) {
	    /* prompt for color */
	    goto color_prompt;
	} else {
	    /* change color */
	    sscanf(argv[3], "%d", &r);
	    sscanf(argv[4], "%d", &g);
	    sscanf(argv[5], "%d", &b);
	    comb->rgb[0] = r;
	    comb->rgb[1] = g;
	    comb->rgb[2] = b;
	    comb->rgb_valid = 1;
	}
    } else {
	/* Color */
    color_prompt:
	if ( comb->rgb_valid ) {
	    struct bu_vls tmp_vls;

	    bu_vls_init(&tmp_vls);
	    bu_vls_printf(&tmp_vls, "Color = %d %d %d\n",
			  comb->rgb[0], comb->rgb[1], comb->rgb[2] );
	    Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
	    bu_vls_free(&tmp_vls);

	    bu_vls_printf(&curr_cmd_list->cl_more_default, "%d %d %d",
			  comb->rgb[0],
			  comb->rgb[1],
			  comb->rgb[2] );
	} else {
	    Tcl_AppendResult(interp, "Color = (No color specified)\n", (char *)NULL);
	    bu_vls_printf(&curr_cmd_list->cl_more_default, "del");
	}

	Tcl_AppendResult(interp, MORE_ARGS_STR,
			 "Color R G B (0..255)? ('del' to delete, CR to skip) ", (char *)NULL);
	goto fail;
    }

    if (argc >= 7 - skip_args) {
	inherit = *argv[6 - skip_args];
    } else {
	/* Inherit */
	switch ( comb->inherit )  {
	    case 0:
		Tcl_AppendResult(interp, "Inherit = 0:  lower nodes (towards leaves) override\n",
				 (char *)NULL);
		break;
	    default:
		Tcl_AppendResult(interp, "Inherit = 1:  higher nodes (towards root) override\n",
				 (char *)NULL);
		break;
	}

	Tcl_AppendResult(interp, MORE_ARGS_STR,
			 "Inheritance (0|1)? (CR to skip) ", (char *)NULL);
	switch ( comb->inherit ) {
	    default:
		bu_vls_printf(&curr_cmd_list->cl_more_default, "1");
		break;
	    case 0:
		bu_vls_printf(&curr_cmd_list->cl_more_default, "0");
		break;
	}

	goto fail;
    }

    switch ( inherit )  {
	case '1':
	    comb->inherit = 1;
	    break;
	case '0':
	    comb->inherit = 0;
	    break;
	case '\0':
	case '\n':
	    break;
	default:
	    Tcl_AppendResult(interp, "Unknown response ignored\n", (char *)NULL);
	    break;
    }

    if ( rt_db_put_internal( dp, dbip, &intern, &rt_uniresource ) < 0 )  {
	TCL_WRITE_ERR_return;
    }
    return TCL_OK;
 fail:
    rt_db_free_internal( &intern, &rt_uniresource );
    return TCL_ERROR;
}

int
f_edmater(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    FILE *fp;
    int i;
    int status;

    char **av;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 2) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help edmater");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    fp = bu_temp_file(tmpfil, MAXPATHLEN);
    if (!fp)
	return TCL_ERROR;

    av = (char **)bu_malloc(sizeof(char *)*(argc + 2), "f_edmater: av");
    av[0] = "wmater";
    av[1] = tmpfil;
    for(i = 2; i < argc + 1; ++i)
	av[i] = argv[i-1];

    av[i] = NULL;

    if (f_wmater(clientData, interp, argc + 1, av) == TCL_ERROR) {
	(void)unlink(tmpfil);
	bu_free((genptr_t)av, "f_edmater: av");
	return TCL_ERROR;
    }

    (void)fclose(fp);

    if (editit(tmpfil)) {
	av[0] = "rmater";
	av[2] = NULL;
	status = f_rmater(clientData, interp, 2, av);
    } else {
	status = TCL_ERROR;
    }

    (void)unlink(tmpfil);
    bu_free((genptr_t)av, "f_edmater: av");

    return status;
}


int
f_wmater(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
    Tcl_DString ds;
    int ret;
    
    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    ret = ged_wmater(wdbp, argc, argv);

    /* Convert to Tcl codes */
    if (ret == GED_OK)
	ret = TCL_OK;
    else
	ret = TCL_ERROR;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, bu_vls_addr(&wdbp->wdb_result_str), -1);
    Tcl_DStringResult(interp, &ds);

    return ret;
}


int
f_rmater(
    ClientData clientData,
    Tcl_Interp *interp,
    int     argc,
    char    *argv[])
{
    Tcl_DString ds;
    int ret;
    
    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    ret = ged_rmater(wdbp, argc, argv);

    /* Convert to Tcl codes */
    if (ret == GED_OK)
	ret = TCL_OK;
    else
	ret = TCL_ERROR;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, bu_vls_addr(&wdbp->wdb_result_str), -1);
    Tcl_DStringResult(interp, &ds);

    return ret;
}


/*
 *			F _ C O M B _ C O L O R
 *
 *  Simple command-line way to set object color
 *  Usage: ocolor combination R G B
 */
int
f_comb_color(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    int				i;
    int				val;
    register struct directory	*dp;
    struct rt_db_internal	intern;
    struct rt_comb_internal	*comb;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 5 || 5 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help comb_color");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ((dp = db_lookup(dbip,  argv[1], LOOKUP_NOISY)) == DIR_NULL)
	return TCL_ERROR;
    if ( (dp->d_flags & DIR_COMB) == 0 )  {
	Tcl_AppendResult(interp, dp->d_namep, ": not a combination\n", (char *)NULL);
	return TCL_ERROR;
    }

    if ( rt_db_get_internal( &intern, dp, dbip, (fastf_t *)NULL, &rt_uniresource ) < 0 )  {
	TCL_READ_ERR_return;
    }
    comb = (struct rt_comb_internal *)intern.idb_ptr;
    RT_CK_COMB(comb);

    for (i = 0; i < 3; ++i)  {
	if (((val = atoi(argv[i + 2])) < 0) || (val > 255))
	{
	    Tcl_AppendResult(interp, "RGB value out of range: ", argv[i + 2],
			     "\n", (char *)NULL);
	    rt_db_free_internal( &intern, &rt_uniresource );
	    return TCL_ERROR;
	}
	else
	    comb->rgb[i] = val;
    }

    comb->rgb_valid = 1;
    if ( rt_db_put_internal( dp, dbip, &intern, &rt_uniresource ) < 0 )  {
	TCL_WRITE_ERR_return;
    }
    return TCL_OK;
}

/*
 *			F _ S H A D E R
 *
 *  Simpler, command-line version of 'mater' command.
 *  Usage: shader combination shader_material [shader_argument(s)]
 */
int
f_shader(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    register struct directory *dp;
    struct rt_db_internal	intern;
    struct rt_comb_internal	*comb;

    CHECK_DBI_NULL;

    if (argc < 2) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help shader");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ( (dp = db_lookup( dbip,  argv[1], LOOKUP_NOISY )) == DIR_NULL )
	return TCL_ERROR;
    if ( (dp->d_flags & DIR_COMB) == 0 )  {
	Tcl_AppendResult(interp, dp->d_namep, ": not a combination\n", (char *)NULL);
	return TCL_ERROR;
    }

    if ( rt_db_get_internal( &intern, dp, dbip, (fastf_t *)NULL, &rt_uniresource ) < 0 )  {
	TCL_READ_ERR_return;
    }
    comb = (struct rt_comb_internal *)intern.idb_ptr;
    RT_CK_COMB(comb);

    if (argc == 2)  {
	/* Return the current shader string */
	Tcl_AppendResult( interp, bu_vls_addr(&comb->shader), (char *)NULL);
	rt_db_free_internal( &intern, &rt_uniresource );
    } else {
	CHECK_READ_ONLY;

	/* Replace with new shader string from command line */
	bu_vls_free( &comb->shader );

	/* Bunch up the rest of the args, space separated */
	bu_vls_from_argv( &comb->shader, argc-2, (const char **)argv+2 );

	if ( rt_db_put_internal( dp, dbip, &intern, &rt_uniresource ) < 0 )  {
	    TCL_WRITE_ERR_return;
	}
	/* Internal representation has been freed by rt_db_put_internal */
    }
    return TCL_OK;
}


/* 
 * Usage:
 *     mirror [-d dir] [-o origin] [-p scalar_pt] [-x] [-y] [-z] old new
 */
int
f_mirror(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    Tcl_DString ds;
    int ret;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    ret = ged_mirror(wdbp, argc, argv);

    /* Convert to Tcl codes */
    if (ret == GED_OK)
	ret = TCL_OK;
    else
	ret = TCL_ERROR;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, bu_vls_addr(&wdbp->wdb_result_str), -1);
    Tcl_DStringResult(interp, &ds);

    if (wdbp->wdb_result_flags == 0 && ret == TCL_OK) {
	char *av[3];

	av[0] = "draw";
	av[1] = argv[argc-1];
	av[2] = NULL;
	cmd_draw(clientData, interp, 2, av);
    }

    return ret;
}

/* Modify Combination record information */
/* Format: edcomb combname Regionflag regionid air los GIFTmater */
int
f_edcomb(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    register struct directory *dp;
    int regionid, air, mat, los;
    struct rt_db_internal	intern;
    struct rt_comb_internal	*comb;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 6 || 7 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help edcomb");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ( (dp = db_lookup( dbip,  argv[1], LOOKUP_NOISY )) == DIR_NULL )
	return TCL_ERROR;
    if ( (dp->d_flags & DIR_COMB) == 0 )  {
	Tcl_AppendResult(interp, dp->d_namep, ": not a combination\n", (char *)NULL);
	return TCL_ERROR;
    }

    regionid = atoi( argv[3] );
    air = atoi( argv[4] );
    los = atoi( argv[5] );
    mat = atoi( argv[6] );

    if ( rt_db_get_internal( &intern, dp, dbip, (fastf_t *)NULL, &rt_uniresource ) < 0 )  {
	TCL_READ_ERR_return;
    }
    comb = (struct rt_comb_internal *)intern.idb_ptr;
    RT_CK_COMB(comb);

    if ( argv[2][0] == 'R' )
	comb->region_flag = 1;
    else
	comb->region_flag = 0;
    comb->region_id = regionid;
    comb->aircode = air;
    comb->los = los;
    comb->GIFTmater = mat;
    if ( rt_db_put_internal( dp, dbip, &intern, &rt_uniresource ) < 0 )  {
	TCL_WRITE_ERR_return;
    }
    return TCL_OK;
}

/* tell him it already exists */
void
aexists(char *name)
{
    Tcl_AppendResult(interp, name, ":  already exists\n", (char *)NULL);
}

/*
 *  			F _ M A K E
 *
 *  Create a new solid of a given type
 *  (Generic, or explicit)
 */
int
f_make(ClientData	clientData,
       Tcl_Interp	*interp,
       int		argc,
       char		**argv)
{
    register struct directory *dp;
    int i;
    struct rt_db_internal	internal;
    struct rt_arb_internal	*arb_ip;
    struct rt_ars_internal	*ars_ip;
    struct rt_tgc_internal	*tgc_ip;
    struct rt_ell_internal	*ell_ip;
    struct rt_tor_internal	*tor_ip;
    struct rt_grip_internal	*grp_ip;
    struct rt_half_internal *half_ip;
    struct rt_rpc_internal *rpc_ip;
    struct rt_rhc_internal *rhc_ip;
    struct rt_epa_internal *epa_ip;
    struct rt_ehy_internal *ehy_ip;
    struct rt_eto_internal *eto_ip;
    struct rt_part_internal *part_ip;
    struct rt_pipe_internal *pipe_ip;
    struct rt_sketch_internal *sketch_ip;
    struct rt_extrude_internal *extrude_ip;
    struct rt_bot_internal *bot_ip;
    struct rt_arbn_internal *arbn_ip;
    struct rt_superell_internal	*superell_ip;
    struct rt_metaball_internal	*metaball_ip;

    if (argc == 2) {
	struct bu_vls vls;

	if (argv[1][0] == '-' && argv[1][1] == 't') {
	    Tcl_AppendElement(interp, "arb8");
	    Tcl_AppendElement(interp, "arb7");
	    Tcl_AppendElement(interp, "arb6");
	    Tcl_AppendElement(interp, "arb5");
	    Tcl_AppendElement(interp, "arb4");
	    Tcl_AppendElement(interp, "arbn");
	    Tcl_AppendElement(interp, "ars");
	    Tcl_AppendElement(interp, "bot");
	    Tcl_AppendElement(interp, "ehy");
	    Tcl_AppendElement(interp, "ell");
	    Tcl_AppendElement(interp, "ell1");
	    Tcl_AppendElement(interp, "epa");
	    Tcl_AppendElement(interp, "eto");
	    Tcl_AppendElement(interp, "extrude");
	    Tcl_AppendElement(interp, "grip");
	    Tcl_AppendElement(interp, "half");
	    Tcl_AppendElement(interp, "nmg");
	    Tcl_AppendElement(interp, "part");
	    Tcl_AppendElement(interp, "pipe");
	    Tcl_AppendElement(interp, "rcc");
	    Tcl_AppendElement(interp, "rec");
	    Tcl_AppendElement(interp, "rhc");
	    Tcl_AppendElement(interp, "rpc");
	    Tcl_AppendElement(interp, "rpp");
	    Tcl_AppendElement(interp, "sketch");
	    Tcl_AppendElement(interp, "sph");
	    Tcl_AppendElement(interp, "tec");
	    Tcl_AppendElement(interp, "tgc");
	    Tcl_AppendElement(interp, "tor");
	    Tcl_AppendElement(interp, "trc");
	    Tcl_AppendElement(interp, "superell");
	    Tcl_AppendElement(interp, "metaball");

	    return TCL_OK;
	}

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help make");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc != 3) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help make");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ( db_lookup( dbip,  argv[1], LOOKUP_QUIET ) != DIR_NULL )  {
	aexists( argv[1] );
	return TCL_ERROR;
    }

    RT_INIT_DB_INTERNAL( &internal );

    /* make name <arb8 | arb7 | arb6 | arb5 | arb4 | ellg | ell | superell
     * sph | tor | tgc | rec | trc | rcc | grp | half | nmg | bot | sketch | extrude> */
    if (strcmp(argv[2], "arb8") == 0 ||
	strcmp(argv[2],  "rpp") == 0)  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ARB8;
	internal.idb_meth = &rt_functab[ID_ARB8];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_arb_internal), "rt_arb_internal" );
	arb_ip = (struct rt_arb_internal *)internal.idb_ptr;
	arb_ip->magic = RT_ARB_INTERNAL_MAGIC;
	VSET( arb_ip->pt[0] ,
	      -view_state->vs_vop->vo_center[MDX] +view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDY] -view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDZ] -view_state->vs_vop->vo_scale );
	for ( i=1; i<8; i++ )			VMOVE( arb_ip->pt[i], arb_ip->pt[0] );
	arb_ip->pt[1][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Z] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[3][Z] += view_state->vs_vop->vo_scale*2.0;
	for ( i=4; i<8; i++ )
	    arb_ip->pt[i][X] -= view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[5][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[6][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[6][Z] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[7][Z] += view_state->vs_vop->vo_scale*2.0;
    } else if ( strcmp( argv[2], "arb7" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ARB8;
	internal.idb_meth = &rt_functab[ID_ARB8];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_arb_internal), "rt_arb_internal" );
	arb_ip = (struct rt_arb_internal *)internal.idb_ptr;
	arb_ip->magic = RT_ARB_INTERNAL_MAGIC;
	VSET( arb_ip->pt[0] ,
	      -view_state->vs_vop->vo_center[MDX] +view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDY] -view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDZ] -(0.5*view_state->vs_vop->vo_scale) );
	for ( i=1; i<8; i++ )
	    VMOVE( arb_ip->pt[i], arb_ip->pt[0] );
	arb_ip->pt[1][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Z] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[3][Z] += view_state->vs_vop->vo_scale;
	for ( i=4; i<8; i++ )
	    arb_ip->pt[i][X] -= view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[5][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[6][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[6][Z] += view_state->vs_vop->vo_scale;
    } else if ( strcmp( argv[2], "arb6" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ARB8;
	internal.idb_meth = &rt_functab[ID_ARB8];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_arb_internal), "rt_arb_internal" );
	arb_ip = (struct rt_arb_internal *)internal.idb_ptr;
	arb_ip->magic = RT_ARB_INTERNAL_MAGIC;
	VSET( arb_ip->pt[0],
	      -view_state->vs_vop->vo_center[MDX] +view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDY] -view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDZ] -view_state->vs_vop->vo_scale );
	for ( i=1; i<8; i++ )
	    VMOVE( arb_ip->pt[i], arb_ip->pt[0] );
	arb_ip->pt[1][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Z] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[3][Z] += view_state->vs_vop->vo_scale*2.0;
	for ( i=4; i<8; i++ )
	    arb_ip->pt[i][X] -= view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[4][Y] += view_state->vs_vop->vo_scale;
	arb_ip->pt[5][Y] += view_state->vs_vop->vo_scale;
	arb_ip->pt[6][Y] += view_state->vs_vop->vo_scale;
	arb_ip->pt[6][Z] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[7][Y] += view_state->vs_vop->vo_scale;
	arb_ip->pt[7][Z] += view_state->vs_vop->vo_scale*2.0;
    } else if ( strcmp( argv[2], "arb5" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ARB8;
	internal.idb_meth = &rt_functab[ID_ARB8];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_arb_internal), "rt_arb_internal" );
	arb_ip = (struct rt_arb_internal *)internal.idb_ptr;
	arb_ip->magic = RT_ARB_INTERNAL_MAGIC;
	VSET( arb_ip->pt[0] ,
	      -view_state->vs_vop->vo_center[MDX] +view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDY] -view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDZ] -view_state->vs_vop->vo_scale );
	for ( i=1; i<8; i++ )
	    VMOVE( arb_ip->pt[i], arb_ip->pt[0] );
	arb_ip->pt[1][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Z] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[3][Z] += view_state->vs_vop->vo_scale*2.0;
	for ( i=4; i<8; i++ )
	{
	    arb_ip->pt[i][X] -= view_state->vs_vop->vo_scale*2.0;
	    arb_ip->pt[i][Y] += view_state->vs_vop->vo_scale;
	    arb_ip->pt[i][Z] += view_state->vs_vop->vo_scale;
	}
    } else if ( strcmp( argv[2], "arb4" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ARB8;
	internal.idb_meth = &rt_functab[ID_ARB8];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_arb_internal), "rt_arb_internal" );
	arb_ip = (struct rt_arb_internal *)internal.idb_ptr;
	arb_ip->magic = RT_ARB_INTERNAL_MAGIC;
	VSET( arb_ip->pt[0] ,
	      -view_state->vs_vop->vo_center[MDX] +view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDY] -view_state->vs_vop->vo_scale,
	      -view_state->vs_vop->vo_center[MDZ] -view_state->vs_vop->vo_scale );
	for ( i=1; i<8; i++ )
	    VMOVE( arb_ip->pt[i], arb_ip->pt[0] );
	arb_ip->pt[1][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[2][Z] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[3][Y] += view_state->vs_vop->vo_scale*2.0;
	arb_ip->pt[3][Z] += view_state->vs_vop->vo_scale*2.0;
	for ( i=4; i<8; i++ )
	{
	    arb_ip->pt[i][X] -= view_state->vs_vop->vo_scale*2.0;
	    arb_ip->pt[i][Y] += view_state->vs_vop->vo_scale*2.0;
	}
    } else if ( strcmp( argv[2], "arbn") == 0 ) {
	point_t view_center;

	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ARBN;
	internal.idb_meth = &rt_functab[ID_ARBN];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof( struct rt_arbn_internal), "rt_arbn_internal" );
	arbn_ip = (struct rt_arbn_internal *)internal.idb_ptr;
	arbn_ip->magic = RT_ARBN_INTERNAL_MAGIC;
	arbn_ip->neqn = 8;
	arbn_ip->eqn = (plane_t *)bu_calloc( arbn_ip->neqn,
					     sizeof( plane_t ), "arbn plane eqns" );
	VSET( arbn_ip->eqn[0], 1, 0, 0 );
	arbn_ip->eqn[0][3] = view_state->vs_vop->vo_scale;
	VSET( arbn_ip->eqn[1], -1, 0, 0 );
	arbn_ip->eqn[1][3] = view_state->vs_vop->vo_scale;
	VSET( arbn_ip->eqn[2], 0, 1, 0 );
	arbn_ip->eqn[2][3] = view_state->vs_vop->vo_scale;
	VSET( arbn_ip->eqn[3], 0, -1, 0 );
	arbn_ip->eqn[3][3] = view_state->vs_vop->vo_scale;
	VSET( arbn_ip->eqn[4], 0, 0, 1 );
	arbn_ip->eqn[4][3] = view_state->vs_vop->vo_scale;
	VSET( arbn_ip->eqn[5], 0, 0, -1 );
	arbn_ip->eqn[5][3] = view_state->vs_vop->vo_scale;
	VSET( arbn_ip->eqn[6], 0.57735, 0.57735, 0.57735 );
	arbn_ip->eqn[6][3] = view_state->vs_vop->vo_scale;
	VSET( arbn_ip->eqn[7], -0.57735, -0.57735, -0.57735 );
	arbn_ip->eqn[7][3] = view_state->vs_vop->vo_scale;
	VSET( view_center,
	      -view_state->vs_vop->vo_center[MDX],
	      -view_state->vs_vop->vo_center[MDY],
	      -view_state->vs_vop->vo_center[MDZ] );
	for ( i=0; i<arbn_ip->neqn; i++ ) {
	    arbn_ip->eqn[i][3] +=
		VDOT( view_center, arbn_ip->eqn[i] );
	}
    } else if ( strcmp( argv[2], "ars" ) == 0 )  {
	int curve;
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ARS;
	internal.idb_meth = &rt_functab[ID_ARS];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_ars_internal), "rt_ars_internal" );
	ars_ip = (struct rt_ars_internal *)internal.idb_ptr;
	ars_ip->magic = RT_ARS_INTERNAL_MAGIC;
	ars_ip->ncurves = 3;
	ars_ip->pts_per_curve = 3;
	ars_ip->curves = (fastf_t **)bu_malloc((ars_ip->ncurves+1) * sizeof(fastf_t **), "ars curve ptrs" );

	for ( curve=0; curve < ars_ip->ncurves; curve++ ) {
	    ars_ip->curves[curve] = (fastf_t *)bu_calloc(
		(ars_ip->pts_per_curve + 1) * 3,
		sizeof(fastf_t), "ARS points");

	    if (curve == 0) {
		VSET( &(ars_ip->curves[0][0]),
		      -view_state->vs_vop->vo_center[MDX],
		      -view_state->vs_vop->vo_center[MDY],
		      -view_state->vs_vop->vo_center[MDZ] );
		VMOVE(&(ars_ip->curves[curve][3]), &(ars_ip->curves[curve][0]));
		VMOVE(&(ars_ip->curves[curve][6]), &(ars_ip->curves[curve][0]));
	    } else if (curve == (ars_ip->ncurves - 1) ) {
		VSET( &(ars_ip->curves[curve][0]),
		      -view_state->vs_vop->vo_center[MDX],
		      -view_state->vs_vop->vo_center[MDY],
		      -view_state->vs_vop->vo_center[MDZ]+curve*(0.25*view_state->vs_vop->vo_scale));
		VMOVE(&(ars_ip->curves[curve][3]), &(ars_ip->curves[curve][0]));
		VMOVE(&(ars_ip->curves[curve][6]), &(ars_ip->curves[curve][0]));

	    } else {
		fastf_t x, y, z;
		x = -view_state->vs_vop->vo_center[MDX]+curve*(0.25*view_state->vs_vop->vo_scale);
		y = -view_state->vs_vop->vo_center[MDY]+curve*(0.25*view_state->vs_vop->vo_scale);
		z = -view_state->vs_vop->vo_center[MDZ]+curve*(0.25*view_state->vs_vop->vo_scale);

		VSET(&ars_ip->curves[curve][0],
		     -view_state->vs_vop->vo_center[MDX],
		     -view_state->vs_vop->vo_center[MDY],
		     z);
		VSET(&ars_ip->curves[curve][3],
		     x,
		     -view_state->vs_vop->vo_center[MDY],
		     z);
		VSET(&ars_ip->curves[curve][6],
		     x, y, z);
	    }
	}

    } else if ( strcmp( argv[2], "sph" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ELL;
	internal.idb_meth = &rt_functab[ID_ELL];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_ell_internal), "rt_ell_internal" );
	ell_ip = (struct rt_ell_internal *)internal.idb_ptr;
	ell_ip->magic = RT_ELL_INTERNAL_MAGIC;
	VSET( ell_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ] );
	VSET( ell_ip->a, (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );	/* A */
	VSET( ell_ip->b, 0.0, (0.5*view_state->vs_vop->vo_scale), 0.0 );	/* B */
	VSET( ell_ip->c, 0.0, 0.0, (0.5*view_state->vs_vop->vo_scale) );	/* C */
    } else if (( strcmp( argv[2], "grp" ) == 0 ) ||
	       ( strcmp( argv[2], "grip") == 0 )) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_GRIP;
	internal.idb_meth = &rt_functab[ID_GRIP];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_grip_internal), "rt_grp_internal" );
	grp_ip = (struct rt_grip_internal *) internal.idb_ptr;
	grp_ip->magic = RT_GRIP_INTERNAL_MAGIC;
	VSET( grp_ip->center, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY],
	      -view_state->vs_vop->vo_center[MDZ]);
	VSET( grp_ip->normal, 1.0, 0.0, 0.0);
	grp_ip->mag = view_state->vs_vop->vo_scale*0.75;
    } else if ( strcmp( argv[2], "ell1" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ELL;
	internal.idb_meth = &rt_functab[ID_ELL];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_ell_internal), "rt_ell_internal" );
	ell_ip = (struct rt_ell_internal *)internal.idb_ptr;
	ell_ip->magic = RT_ELL_INTERNAL_MAGIC;
	VSET( ell_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ] );
	VSET( ell_ip->a, (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );	/* A */
	VSET( ell_ip->b, 0.0, (0.25*view_state->vs_vop->vo_scale), 0.0 );	/* B */
	VSET( ell_ip->c, 0.0, 0.0, (0.25*view_state->vs_vop->vo_scale) );	/* C */
    } else if ( strcmp( argv[2], "ell" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ELL;
	internal.idb_meth = &rt_functab[ID_ELL];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_ell_internal), "rt_ell_internal" );
	ell_ip = (struct rt_ell_internal *)internal.idb_ptr;
	ell_ip->magic = RT_ELL_INTERNAL_MAGIC;
	VSET( ell_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ] );
	VSET( ell_ip->a, view_state->vs_vop->vo_scale, 0.0, 0.0 );		/* A */
	VSET( ell_ip->b, 0.0, (0.5*view_state->vs_vop->vo_scale), 0.0 );	/* B */
	VSET( ell_ip->c, 0.0, 0.0, (0.25*view_state->vs_vop->vo_scale) );	/* C */
    } else if ( strcmp( argv[2], "tor" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_TOR;
	internal.idb_meth = &rt_functab[ID_TOR];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_tor_internal), "rt_tor_internal" );
	tor_ip = (struct rt_tor_internal *)internal.idb_ptr;
	tor_ip->magic = RT_TOR_INTERNAL_MAGIC;
	VSET( tor_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ] );
	VSET( tor_ip->h, 1.0, 0.0, 0.0 );	/* unit normal */
	tor_ip->r_h = 0.5*view_state->vs_vop->vo_scale;
	tor_ip->r_a = view_state->vs_vop->vo_scale;
	tor_ip->r_b = view_state->vs_vop->vo_scale;
	VSET( tor_ip->a, 0.0, view_state->vs_vop->vo_scale, 0.0 );
	VSET( tor_ip->b, 0.0, 0.0, view_state->vs_vop->vo_scale );
    } else if ( strcmp( argv[2], "tgc" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_TGC;
	internal.idb_meth = &rt_functab[ID_TGC];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_tgc_internal), "rt_tgc_internal" );
	tgc_ip = (struct rt_tgc_internal *)internal.idb_ptr;
	tgc_ip->magic = RT_TGC_INTERNAL_MAGIC;
	VSET( tgc_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	VSET( tgc_ip->h,  0.0, 0.0, (view_state->vs_vop->vo_scale*2) );
	VSET( tgc_ip->a,  (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->b,  0.0, (0.25*view_state->vs_vop->vo_scale), 0.0 );
	VSET( tgc_ip->c,  (0.25*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->d,  0.0, (0.5*view_state->vs_vop->vo_scale), 0.0 );
    } else if ( strcmp( argv[2], "tec" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_TGC;
	internal.idb_meth = &rt_functab[ID_TGC];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_tgc_internal), "rt_tgc_internal" );
	tgc_ip = (struct rt_tgc_internal *)internal.idb_ptr;
	tgc_ip->magic = RT_TGC_INTERNAL_MAGIC;
	VSET( tgc_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	VSET( tgc_ip->h,  0.0, 0.0, (view_state->vs_vop->vo_scale*2) );
	VSET( tgc_ip->a,  (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->b,  0.0, (0.25*view_state->vs_vop->vo_scale), 0.0 );
	VSET( tgc_ip->c,  (0.25*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->d,  0.0, (0.125*view_state->vs_vop->vo_scale), 0.0 );
    } else if ( strcmp( argv[2], "rec" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_TGC;
	internal.idb_meth = &rt_functab[ID_TGC];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_tgc_internal), "rt_tgc_internal" );
	tgc_ip = (struct rt_tgc_internal *)internal.idb_ptr;
	tgc_ip->magic = RT_TGC_INTERNAL_MAGIC;
	VSET( tgc_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	VSET( tgc_ip->h,  0.0, 0.0, (view_state->vs_vop->vo_scale*2) );
	VSET( tgc_ip->a,  (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->b,  0.0, (0.25*view_state->vs_vop->vo_scale), 0.0 );
	VSET( tgc_ip->c,  (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->d,  0.0, (0.25*view_state->vs_vop->vo_scale), 0.0 );
    } else if ( strcmp( argv[2], "trc" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_TGC;
	internal.idb_meth = &rt_functab[ID_TGC];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_tgc_internal), "rt_tgc_internal" );
	tgc_ip = (struct rt_tgc_internal *)internal.idb_ptr;
	tgc_ip->magic = RT_TGC_INTERNAL_MAGIC;
	VSET( tgc_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	VSET( tgc_ip->h,  0.0, 0.0, (view_state->vs_vop->vo_scale*2) );
	VSET( tgc_ip->a,  (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->b,  0.0, (0.5*view_state->vs_vop->vo_scale), 0.0 );
	VSET( tgc_ip->c,  (0.25*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->d,  0.0, (0.25*view_state->vs_vop->vo_scale), 0.0 );
    } else if ( strcmp( argv[2], "rcc" ) == 0 )  {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_TGC;
	internal.idb_meth = &rt_functab[ID_TGC];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_tgc_internal), "rt_tgc_internal" );
	tgc_ip = (struct rt_tgc_internal *)internal.idb_ptr;
	tgc_ip->magic = RT_TGC_INTERNAL_MAGIC;
	VSET( tgc_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	VSET( tgc_ip->h,  0.0, 0.0, (view_state->vs_vop->vo_scale*2) );
	VSET( tgc_ip->a,  (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->b,  0.0, (0.5*view_state->vs_vop->vo_scale), 0.0 );
	VSET( tgc_ip->c,  (0.5*view_state->vs_vop->vo_scale), 0.0, 0.0 );
	VSET( tgc_ip->d,  0.0, (0.5*view_state->vs_vop->vo_scale), 0.0 );
    } else if ( strcmp( argv[2], "half" ) == 0 ) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_HALF;
	internal.idb_meth = &rt_functab[ID_HALF];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_half_internal), "rt_half_internal" );
	half_ip = (struct rt_half_internal *)internal.idb_ptr;
	half_ip->magic = RT_HALF_INTERNAL_MAGIC;
	VSET( half_ip->eqn, 0.0, 0.0, 1.0 );
	half_ip->eqn[3] = (-view_state->vs_vop->vo_center[MDZ]);
    } else if ( strcmp( argv[2], "rpc" ) == 0 ) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_RPC;
	internal.idb_meth = &rt_functab[ID_RPC];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_rpc_internal), "rt_rpc_internal" );
	rpc_ip = (struct rt_rpc_internal *)internal.idb_ptr;
	rpc_ip->rpc_magic = RT_RPC_INTERNAL_MAGIC;
	VSET( rpc_ip->rpc_V, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale*0.5 );
	VSET( rpc_ip->rpc_H, 0.0, 0.0, view_state->vs_vop->vo_scale );
	VSET( rpc_ip->rpc_B, 0.0, (view_state->vs_vop->vo_scale*0.5), 0.0 );
	rpc_ip->rpc_r = view_state->vs_vop->vo_scale*0.25;
    } else if ( strcmp( argv[2], "rhc" ) == 0 ) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_RHC;
	internal.idb_meth = &rt_functab[ID_RHC];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_rhc_internal), "rt_rhc_internal" );
	rhc_ip = (struct rt_rhc_internal *)internal.idb_ptr;
	rhc_ip->rhc_magic = RT_RHC_INTERNAL_MAGIC;
	VSET( rhc_ip->rhc_V, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale*0.5 );
	VSET( rhc_ip->rhc_H, 0.0, 0.0, view_state->vs_vop->vo_scale );
	VSET( rhc_ip->rhc_B, 0.0, (view_state->vs_vop->vo_scale*0.5), 0.0 );
	rhc_ip->rhc_r = view_state->vs_vop->vo_scale*0.25;
	rhc_ip->rhc_c = view_state->vs_vop->vo_scale*0.10;
    } else if ( strcmp( argv[2], "epa" ) == 0 ) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_EPA;
	internal.idb_meth = &rt_functab[ID_EPA];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_epa_internal), "rt_epa_internal" );
	epa_ip = (struct rt_epa_internal *)internal.idb_ptr;
	epa_ip->epa_magic = RT_EPA_INTERNAL_MAGIC;
	VSET( epa_ip->epa_V, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale*0.5 );
	VSET( epa_ip->epa_H, 0.0, 0.0, view_state->vs_vop->vo_scale );
	VSET( epa_ip->epa_Au, 0.0, 1.0, 0.0 );
	epa_ip->epa_r1 = view_state->vs_vop->vo_scale*0.5;
	epa_ip->epa_r2 = view_state->vs_vop->vo_scale*0.25;
    } else if ( strcmp( argv[2], "ehy" ) == 0 ) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_EHY;
	internal.idb_meth = &rt_functab[ID_EHY];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_ehy_internal), "rt_ehy_internal" );
	ehy_ip = (struct rt_ehy_internal *)internal.idb_ptr;
	ehy_ip->ehy_magic = RT_EHY_INTERNAL_MAGIC;
	VSET( ehy_ip->ehy_V, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale*0.5 );
	VSET( ehy_ip->ehy_H, 0.0, 0.0, view_state->vs_vop->vo_scale );
	VSET( ehy_ip->ehy_Au, 0.0, 1.0, 0.0 );
	ehy_ip->ehy_r1 = view_state->vs_vop->vo_scale*0.5;
	ehy_ip->ehy_r2 = view_state->vs_vop->vo_scale*0.25;
	ehy_ip->ehy_c = ehy_ip->ehy_r2;
    } else if ( strcmp( argv[2], "eto" ) == 0 ) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_ETO;
	internal.idb_meth = &rt_functab[ID_ETO];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_eto_internal), "rt_eto_internal" );
	eto_ip = (struct rt_eto_internal *)internal.idb_ptr;
	eto_ip->eto_magic = RT_ETO_INTERNAL_MAGIC;
	VSET( eto_ip->eto_V, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ] );
	VSET( eto_ip->eto_N, 0.0, 0.0, 1.0 );
	VSET( eto_ip->eto_C, view_state->vs_vop->vo_scale*0.1, 0.0, view_state->vs_vop->vo_scale*0.1 );
	eto_ip->eto_r = view_state->vs_vop->vo_scale*0.5;
	eto_ip->eto_rd = view_state->vs_vop->vo_scale*0.05;
    } else if ( strcmp( argv[2], "part" ) == 0 ) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_PARTICLE;
	internal.idb_meth = &rt_functab[ID_PARTICLE];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_part_internal), "rt_part_internal" );
	part_ip = (struct rt_part_internal *)internal.idb_ptr;
	part_ip->part_magic = RT_PART_INTERNAL_MAGIC;
	VSET( part_ip->part_V, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale*0.5 );
	VSET( part_ip->part_H, 0.0, 0.0, view_state->vs_vop->vo_scale );
	part_ip->part_vrad = view_state->vs_vop->vo_scale*0.5;
	part_ip->part_hrad = view_state->vs_vop->vo_scale*0.25;
	part_ip->part_type = RT_PARTICLE_TYPE_CONE;
    } else if ( strcmp( argv[2], "nmg" ) == 0 ) {
	struct model *m;
	struct nmgregion *r;
	struct shell *s;

	m = nmg_mm();
	r = nmg_mrsv( m );
	s = BU_LIST_FIRST( shell, &r->s_hd );
	nmg_vertex_g( s->vu_p->v_p, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]);
	(void)nmg_meonvu( s->vu_p );
	(void)nmg_ml( s );
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_NMG;
	internal.idb_meth = &rt_functab[ID_NMG];
	internal.idb_ptr = (genptr_t)m;
    } else if ( strcmp( argv[2], "pipe" ) == 0 ) {
	struct wdb_pipept *ps;

	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_PIPE;
	internal.idb_meth = &rt_functab[ID_PIPE];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_pipe_internal), "rt_pipe_internal" );
	pipe_ip = (struct rt_pipe_internal *)internal.idb_ptr;
	pipe_ip->pipe_magic = RT_PIPE_INTERNAL_MAGIC;
	BU_LIST_INIT( &pipe_ip->pipe_segs_head );
	BU_GETSTRUCT( ps, wdb_pipept );
	ps->l.magic = WDB_PIPESEG_MAGIC;
	VSET( ps->pp_coord, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	ps->pp_od = 0.5*view_state->vs_vop->vo_scale;
	ps->pp_id = 0.5*ps->pp_od;
	ps->pp_bendradius = ps->pp_od;
	BU_LIST_INSERT( &pipe_ip->pipe_segs_head, &ps->l );
	BU_GETSTRUCT( ps, wdb_pipept );
	ps->l.magic = WDB_PIPESEG_MAGIC;
	VSET( ps->pp_coord, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]+view_state->vs_vop->vo_scale );
	ps->pp_od = 0.5*view_state->vs_vop->vo_scale;
	ps->pp_id = 0.5*ps->pp_od;
	ps->pp_bendradius = ps->pp_od;
	BU_LIST_INSERT( &pipe_ip->pipe_segs_head, &ps->l );
    } else if ( strcmp( argv[2], "bot" ) == 0 ) {
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_BOT;
	internal.idb_meth = &rt_functab[ID_BOT];
	BU_GETSTRUCT( bot_ip, rt_bot_internal );
	internal.idb_ptr = (genptr_t)bot_ip;
	bot_ip = (struct rt_bot_internal *)internal.idb_ptr;
	bot_ip->magic = RT_BOT_INTERNAL_MAGIC;
	bot_ip->mode = RT_BOT_SOLID;
	bot_ip->orientation = RT_BOT_UNORIENTED;
	bot_ip->num_vertices = 4;
	bot_ip->num_faces = 4;
	bot_ip->faces = (int *)bu_calloc( bot_ip->num_faces * 3, sizeof( int ), "BOT faces" );
	bot_ip->vertices = (fastf_t *)bu_calloc( bot_ip->num_vertices * 3, sizeof( fastf_t ), "BOT vertices" );
	bot_ip->thickness = (fastf_t *)NULL;
	bot_ip->face_mode = (struct bu_bitv *)NULL;
	VSET( &bot_ip->vertices[0],  -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]+view_state->vs_vop->vo_scale );
	VSET( &bot_ip->vertices[3], -view_state->vs_vop->vo_center[MDX]-0.5*view_state->vs_vop->vo_scale, -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	VSET( &bot_ip->vertices[6], -view_state->vs_vop->vo_center[MDX]-0.5*view_state->vs_vop->vo_scale, -view_state->vs_vop->vo_center[MDY]-0.5*view_state->vs_vop->vo_scale, -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	VSET( &bot_ip->vertices[9], -view_state->vs_vop->vo_center[MDX]+0.5*view_state->vs_vop->vo_scale, -view_state->vs_vop->vo_center[MDY]-0.5*view_state->vs_vop->vo_scale, -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale );
	VSET( &bot_ip->faces[0], 0, 1, 3 );
	VSET( &bot_ip->faces[3], 0, 1, 2 );
	VSET( &bot_ip->faces[6], 0, 2, 3 );
	VSET( &bot_ip->faces[9], 1, 2, 3 );
    } else if ( strcmp( argv[2], "extrude" ) == 0 ) {
	char *av[3];

	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_EXTRUDE;
	internal.idb_meth = &rt_functab[ID_EXTRUDE];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof( struct rt_extrude_internal), "rt_extrude_internal" );
	extrude_ip = (struct rt_extrude_internal *)internal.idb_ptr;
	extrude_ip->magic = RT_EXTRUDE_INTERNAL_MAGIC;
	VSET( extrude_ip->V, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale*0.5 );
	VSET( extrude_ip->h, 0.0, 0.0, view_state->vs_vop->vo_scale/3.0 );
	VSET( extrude_ip->u_vec, 1.0, 0.0, 0.0 );
	VSET( extrude_ip->v_vec, 0.0, 1.0, 0.0 );
	extrude_ip->keypoint = 0;
	av[0] = "make_name";
	av[1] = "skt_";
	Tcl_ResetResult( interp );
	cmd_make_name( (ClientData)NULL, interp, 2, av );
	extrude_ip->sketch_name = bu_strdup( Tcl_GetStringResult(interp) );
	Tcl_ResetResult( interp );
	extrude_ip->skt = (struct rt_sketch_internal *)NULL;
	av[0] = "make";
	av[1] = extrude_ip->sketch_name;
	av[2] = "sketch";
	f_make( clientData, interp, 3, av );
    } else if ( strcmp( argv[2], "sketch" ) == 0 ) {
#if 0
	/* used by the now defunct default sketch object */
	struct carc_seg *csg;
	struct line_seg *lsg;
#endif

	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_SKETCH;
	internal.idb_meth = &rt_functab[ID_SKETCH];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_sketch_internal), "rt_sketch_internal" );
	sketch_ip = (struct rt_sketch_internal *)internal.idb_ptr;
	sketch_ip->magic = RT_SKETCH_INTERNAL_MAGIC;
	VSET( sketch_ip->u_vec, 1.0, 0.0, 0.0 );
	VSET( sketch_ip->v_vec, 0.0, 1.0, 0.0 );
	VSET( sketch_ip->V, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ]-view_state->vs_vop->vo_scale*0.5 );
#if 0
	/* XXX this creates a "default" sketch object -- this
	   code should probably be made optional somewhere
	   else now? */
	sketch_ip->vert_count = 7;
	sketch_ip->verts = (point2d_t *)bu_calloc( sketch_ip->vert_count, sizeof( point2d_t ), "sketch_ip->verts" );
	sketch_ip->verts[0][0] = 0.25*view_state->vs_vop->vo_scale;
	sketch_ip->verts[0][1] = 0.0;
	sketch_ip->verts[1][0] = 0.5*view_state->vs_vop->vo_scale;
	sketch_ip->verts[1][1] = 0.0;
	sketch_ip->verts[2][0] = 0.5*view_state->vs_vop->vo_scale;
	sketch_ip->verts[2][1] = 0.5*view_state->vs_vop->vo_scale;
	sketch_ip->verts[3][0] = 0.0;
	sketch_ip->verts[3][1] = 0.5*view_state->vs_vop->vo_scale;
	sketch_ip->verts[4][0] = 0.0;
	sketch_ip->verts[4][1] = 0.25*view_state->vs_vop->vo_scale;
	sketch_ip->verts[5][0] = 0.25*view_state->vs_vop->vo_scale;
	sketch_ip->verts[5][1] = 0.25*view_state->vs_vop->vo_scale;
	sketch_ip->verts[6][0] = 0.125*view_state->vs_vop->vo_scale;
	sketch_ip->verts[6][1] = 0.125*view_state->vs_vop->vo_scale;
	sketch_ip->skt_curve.seg_count = 6;
	sketch_ip->skt_curve.reverse = (int *)bu_calloc( sketch_ip->skt_curve.seg_count, sizeof( int ), "sketch_ip->skt_curve.reverse" );
	sketch_ip->skt_curve.segments = (genptr_t *)bu_calloc( sketch_ip->skt_curve.seg_count, sizeof( genptr_t ), "sketch_ip->skt_curve.segments" );

	csg = (struct carc_seg *)bu_calloc( 1, sizeof( struct carc_seg ), "segments" );
	sketch_ip->skt_curve.segments[0] = (genptr_t)csg;
	csg->magic = CURVE_CARC_MAGIC;
	csg->start = 4;
	csg->end = 0;
	csg->radius = 0.25*view_state->vs_vop->vo_scale;
	csg->center_is_left = 1;
	csg->orientation = 0;

	lsg = (struct line_seg *)bu_calloc( 1, sizeof( struct line_seg ), "segments" );
	sketch_ip->skt_curve.segments[1] = (genptr_t)lsg;
	lsg->magic = CURVE_LSEG_MAGIC;
	lsg->start = 0;
	lsg->end = 1;

	lsg = (struct line_seg *)bu_calloc( 1, sizeof( struct line_seg ), "segments" );
	sketch_ip->skt_curve.segments[2] = (genptr_t)lsg;
	lsg->magic = CURVE_LSEG_MAGIC;
	lsg->start = 1;
	lsg->end = 2;

	lsg = (struct line_seg *)bu_calloc( 1, sizeof( struct line_seg ), "segments" );
	sketch_ip->skt_curve.segments[3] = (genptr_t)lsg;
	lsg->magic = CURVE_LSEG_MAGIC;
	lsg->start = 2;
	lsg->end = 3;

	lsg = (struct line_seg *)bu_calloc( 1, sizeof( struct line_seg ), "segments" );
	sketch_ip->skt_curve.segments[4] = (genptr_t)lsg;
	lsg->magic = CURVE_LSEG_MAGIC;
	lsg->start = 3;
	lsg->end = 4;

	csg = (struct carc_seg *)bu_calloc( 1, sizeof( struct carc_seg ), "segments" );
	sketch_ip->skt_curve.segments[5] = (genptr_t)csg;
	csg->magic = CURVE_CARC_MAGIC;
	csg->start = 6;
	csg->end = 5;
	csg->radius = -1.0;
	csg->center_is_left = 1;
	csg->orientation = 0;
#else
	sketch_ip->vert_count = 0;
	sketch_ip->verts = (point2d_t *)NULL;
	sketch_ip->skt_curve.seg_count = 0;
	sketch_ip->skt_curve.reverse = (int *)NULL;
	sketch_ip->skt_curve.segments = (genptr_t *)NULL;
#endif
    } else if ( strcmp( argv[2], "superell" ) == 0 )  {


	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_SUPERELL;
	internal.idb_meth = &rt_functab[ID_SUPERELL];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_superell_internal), "rt_superell_internal" );
	superell_ip = (struct rt_superell_internal *)internal.idb_ptr;
	superell_ip->magic = RT_SUPERELL_INTERNAL_MAGIC;
	VSET( superell_ip->v, -view_state->vs_vop->vo_center[MDX], -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ] );
	VSET( superell_ip->a, view_state->vs_vop->vo_scale, 0.0, 0.0 );		/* A */
	VSET( superell_ip->b, 0.0, (0.5*view_state->vs_vop->vo_scale), 0.0 );	/* B */
	VSET( superell_ip->c, 0.0, 0.0, (0.25*view_state->vs_vop->vo_scale) );	/* C */
	superell_ip->n = 1.0;
	superell_ip->e = 1.0;
	fprintf(stdout, "superell being made with %f and %f\n", superell_ip->n, superell_ip->e);

    } else if (strcmp(argv[2], "hf") == 0) {
	Tcl_AppendResult(interp, "make: the height field is deprecated and not supported by this command.\nUse the dsp primitive.\n", (char *)NULL);
	return TCL_ERROR;
    } else if (strcmp(argv[2], "pg") == 0 ||
	       strcmp(argv[2], "poly") == 0) {
	Tcl_AppendResult(interp, "make: the polysolid is deprecated and not supported by this command.\nUse the bot primitive.\n", (char *)NULL);
	return TCL_ERROR;
    } else if (strcmp(argv[2], "cline") == 0 ||
	       strcmp(argv[2], "dsp") == 0 ||
	       strcmp(argv[2], "ebm") == 0 ||
	       strcmp(argv[2], "nurb") == 0 ||
	       strcmp(argv[2], "spline") == 0 ||
	       strcmp(argv[2], "submodel") == 0 ||
	       strcmp(argv[2], "vol") == 0) {
	Tcl_AppendResult(interp, "make: the ", argv[2], " primitive is not supported by this command.\n", (char *)NULL);
	return TCL_ERROR;
    } else if (strcmp(argv[2], "metaball") == 0) {
	struct wdb_metaballpt *mbpt;
	internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	internal.idb_type = ID_METABALL;
	internal.idb_meth = &rt_functab[ID_METABALL];
	internal.idb_ptr = (genptr_t)bu_malloc( sizeof(struct rt_metaball_internal), "rt_metaball_internal" );
	metaball_ip = (struct rt_metaball_internal *)internal.idb_ptr;
	metaball_ip->magic = RT_METABALL_INTERNAL_MAGIC;
	metaball_ip->threshold = 1.0;
	metaball_ip->method = 1;
	BU_LIST_INIT( &metaball_ip->metaball_ctrl_head );

	mbpt = (struct wdb_metaballpt *)malloc(sizeof(struct wdb_metaballpt));
	mbpt->fldstr = 1.0;
	mbpt->sweat = 1.0;
	VSET(mbpt->coord, -view_state->vs_vop->vo_center[MDX] - 1.0, -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ] );
	BU_LIST_INSERT(&metaball_ip->metaball_ctrl_head, &mbpt->l);

	mbpt = (struct wdb_metaballpt *)malloc(sizeof(struct wdb_metaballpt));
	mbpt->fldstr = 1.0;
	mbpt->sweat = 1.0;
	VSET(mbpt->coord, -view_state->vs_vop->vo_center[MDX] + 1.0, -view_state->vs_vop->vo_center[MDY], -view_state->vs_vop->vo_center[MDZ] );
	BU_LIST_INSERT(&metaball_ip->metaball_ctrl_head, &mbpt->l);

	fprintf(stdout, "metaball being made with %f threshold and two points using the %s rendering method\n", 
		metaball_ip->threshold, rt_metaball_lookup_type_name(metaball_ip->method));
    } else {
	Tcl_AppendResult(interp, "make:  ", argv[2], " is not a known primitive\n",
			 "\tchoices are: arb8, arb7, arb6, arb5, arb4, arbn, ars, bot,\n",
			 "\t\tehy, ell, ell1, epa, eto, extrude, grip, half,\n",
			 "\t\tmetaball, nmg, part, pipe, rcc, rec, rhc, rpc,\n",
			 "\t\tsketch, sph, superell, tec, tgc, tor, trc\n",
			 (char *)NULL);
	return TCL_ERROR;
    }

    /* no interuprts */
    (void)signal( SIGINT, SIG_IGN );

    if ( (dp = db_diradd( dbip, argv[1], -1L, 0, DIR_SOLID, (genptr_t)&internal.idb_type)) == DIR_NULL )  {
	TCL_ALLOC_ERR_return;
    }
    if ( rt_db_put_internal( dp, dbip, &internal, &rt_uniresource ) < 0 )  {
	TCL_WRITE_ERR_return;
    }

    {
	char *av[3];

	av[0] = "e";
	av[1] = argv[1]; /* depends on name being in argv[1] */
	av[2] = NULL;

	/* draw the "made" solid */
	return cmd_draw( clientData, interp, 2, av );
    }
}

int
mged_rot_obj(Tcl_Interp *interp, int iflag, fastf_t *argvect)
{
    point_t model_pt;
    point_t point;
    point_t s_point;
    mat_t temp;
    vect_t v_work;

    update_views = 1;

    if (movedir != ROTARROW) {
	/* NOT in object rotate mode - put it in obj rot */
	movedir = ROTARROW;
    }

    /* find point for rotation to take place wrt */
#if 0
    MAT4X3PNT(model_pt, es_mat, es_keypoint);
#else
    VMOVE(model_pt, es_keypoint);
#endif
    MAT4X3PNT(point, modelchanges, model_pt);

    /* Find absolute translation vector to go from "model_pt" to
     * 	"point" without any of the rotations in "modelchanges"
     */
    VSCALE(s_point, point, modelchanges[15]);
    VSUB2(v_work, s_point, model_pt);

    /* REDO "modelchanges" such that:
     *	1. NO rotations (identity)
     *	2. trans == v_work
     *	3. same scale factor
     */
    MAT_IDN(temp);
    MAT_DELTAS_VEC(temp, v_work);
    temp[15] = modelchanges[15];
    MAT_COPY(modelchanges, temp);

    /* build new rotation matrix */
    MAT_IDN(temp);
    bn_mat_angles(temp, argvect[0], argvect[1], argvect[2]);

    if (iflag) {
	/* apply accumulated rotations */
	bn_mat_mul2(acc_rot_sol, temp);
    }

    /*XXX*/ MAT_COPY(acc_rot_sol, temp); /* used to rotate solid/object axis */

    /* Record the new rotation matrix into the revised
     *	modelchanges matrix wrt "point"
     */
    wrt_point(modelchanges, temp, modelchanges, point);

#ifdef DO_NEW_EDIT_MATS
    new_edit_mats();
#else
    new_mats();
#endif

    return TCL_OK;
}


/* allow precise changes to object rotation */
int
f_rot_obj(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    int iflag = 0;
    vect_t argvect;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 4 || 5 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help %s", argv[0]);
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ( not_state( ST_O_EDIT, "Object Rotation" ) )
	return TCL_ERROR;

    /* Check for -i option */
    if (argv[1][0] == '-' && argv[1][1] == 'i') {
	iflag = 1;  /* treat arguments as incremental values */
	++argv;
	--argc;
    }

    if (argc != 4)
	return TCL_ERROR;

    argvect[0] = atof(argv[1]);
    argvect[1] = atof(argv[2]);
    argvect[2] = atof(argv[3]);

    return mged_rot_obj(interp, iflag, argvect);
}

/* allow precise changes to object scaling, both local & global */
int
f_sc_obj(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    mat_t incr;
    vect_t point, temp;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 2 || 2 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help oscale");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ( not_state( ST_O_EDIT, "Object Scaling" ) )
	return TCL_ERROR;

    if ( atof(argv[1]) <= 0.0 ) {
	Tcl_AppendResult(interp, "ERROR: scale factor <=  0\n", (char *)NULL);
	return TCL_ERROR;
    }

    update_views = 1;

#if 0
    if (movedir != SARROW) {
	/* Put in global object scale mode */
	if ( edobj == 0 )
	    edobj = BE_O_SCALE;	/* default is global scaling */
	movedir = SARROW;
    }
#endif

    MAT_IDN(incr);

    /* switch depending on type of scaling to do */
    switch ( edobj ) {
	default:
	case BE_O_SCALE:
	    /* global scaling */
	    incr[15] = 1.0 / (atof(argv[1]) * modelchanges[15]);
	    break;
	case BE_O_XSCALE:
	    /* local scaling ... X-axis */
	    incr[0] = atof(argv[1]) / acc_sc[0];
	    acc_sc[0] = atof(argv[1]);
	    break;
	case BE_O_YSCALE:
	    /* local scaling ... Y-axis */
	    incr[5] = atof(argv[1]) / acc_sc[1];
	    acc_sc[1] = atof(argv[1]);
	    break;
	case BE_O_ZSCALE:
	    /* local scaling ... Z-axis */
	    incr[10] = atof(argv[1]) / acc_sc[2];
	    acc_sc[2] = atof(argv[1]);
	    break;
    }

    /* find point the scaling is to take place wrt */
#if 0
    MAT4X3PNT(temp, es_mat, es_keypoint);
#else
    VMOVE(temp, es_keypoint);
#endif
    MAT4X3PNT(point, modelchanges, temp);

    wrt_point(modelchanges, incr, modelchanges, point);
#ifdef DO_NEW_EDIT_MATS
    new_edit_mats();
#else
    new_mats();
#endif

    return TCL_OK;
}

/*
 *			F _ T R _ O B J
 *
 *  Bound to command "translate"
 *
 *  Allow precise changes to object translation
 */
int
f_tr_obj(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    register int i;
    mat_t incr, old;
    vect_t model_sol_pt, model_incr, ed_sol_pt, new_vertex;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 4 || 4 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help translate");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ( state == ST_S_EDIT )  {
	/* In solid edit mode,
	 * perform the equivalent of "press sxy" and "p xyz"
	 */
	if ( be_s_trans(clientData, interp, argc, argv) == TCL_ERROR )
	    return TCL_ERROR;
	return f_param(clientData, interp, argc, argv);
    }

    if ( not_state( ST_O_EDIT, "Object Translation") )
	return TCL_ERROR;

    /* Remainder of code concerns object edit case */

    update_views = 1;

    MAT_IDN(incr);
    MAT_IDN(old);

    if ( (movedir & (RARROW|UARROW)) == 0 ) {
	/* put in object trans mode */
	movedir = UARROW | RARROW;
    }

    for (i=0; i<3; i++) {
	new_vertex[i] = atof(argv[i+1]) * local2base;
    }
#if 0
    MAT4X3PNT(model_sol_pt, es_mat, es_keypoint);
#else
    VMOVE(model_sol_pt, es_keypoint);
#endif
    MAT4X3PNT(ed_sol_pt, modelchanges, model_sol_pt);
    VSUB2(model_incr, new_vertex, ed_sol_pt);
    MAT_DELTAS_VEC(incr, model_incr);
    MAT_COPY(old, modelchanges);
    bn_mat_mul(modelchanges, incr, old);
#ifdef DO_NEW_EDIT_MATS
    new_edit_mats();
#else
    new_mats();
#endif

    return TCL_OK;
}

/* Change the default region ident codes: item air los mat
 */
int
f_regdef(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    if (argc == 1) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "ident %d air %d los %d material %d",
		      item_default, air_default, los_default, mat_default);
	Tcl_AppendResult(interp, bu_vls_addr(&vls), (char *)NULL);
	bu_vls_free(&vls);

	return TCL_OK;
    }

    if (argc < 2 || 5 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help regdef");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);

	return TCL_ERROR;
    }

    view_state->vs_flag = 1;
    item_default = atoi(argv[1]);
    wdbp->wdb_item_default = item_default;

    if (argc == 2)
	return TCL_OK;

    air_default = atoi(argv[2]);
    wdbp->wdb_air_default = air_default;
    if (air_default) {
	item_default = 0;
	wdbp->wdb_item_default = 0;
    }

    if (argc == 3)
	return TCL_OK;

    los_default = atoi(argv[3]);
    wdbp->wdb_los_default = los_default;

    if (argc == 4)
	return TCL_OK;

    mat_default = atoi(argv[4]);
    wdbp->wdb_mat_default = mat_default;

    return TCL_OK;
}

static int frac_stat;
void
mged_add_nmg_part(char *newname, struct model *m)
{
    struct rt_db_internal	new_intern;
    struct directory *new_dp;
    struct nmgregion *r;

    if (dbip == DBI_NULL)
	return;

    if ( db_lookup( dbip,  newname, LOOKUP_QUIET ) != DIR_NULL )  {
	aexists( newname );
	/* Free memory here */
	nmg_km(m);
	frac_stat = 1;
	return;
    }

    if ( (new_dp=db_diradd( dbip, newname, -1, 0, DIR_SOLID, (genptr_t)&new_intern.idb_type)) == DIR_NULL )  {
	TCL_ALLOC_ERR;
	return;
    }

    /* make sure the geometry/bounding boxes are up to date */
    for (BU_LIST_FOR(r, nmgregion, &m->r_hd))
	nmg_region_a(r, &mged_tol);


    /* Export NMG as a new solid */
    RT_INIT_DB_INTERNAL(&new_intern);
    new_intern.idb_major_type = DB5_MAJORTYPE_BRLCAD;
    new_intern.idb_type = ID_NMG;
    new_intern.idb_meth = &rt_functab[ID_NMG];
    new_intern.idb_ptr = (genptr_t)m;

    if ( rt_db_put_internal( new_dp, dbip, &new_intern, &rt_uniresource ) < 0 )  {
	/* Free memory */
	nmg_km(m);
	Tcl_AppendResult(interp, "rt_db_put_internal() failure\n", (char *)NULL);
	frac_stat = 1;
	return;
    }
    /* Internal representation has been freed by rt_db_put_internal */
    new_intern.idb_ptr = (genptr_t)NULL;
    frac_stat = 0;
}
/*
 *			F _ F R A C T U R E
 *
 * Usage: fracture nmgsolid [prefix]
 *
 *	given an NMG solid, break it up into several NMG solids, each
 *	containing a single shell with a single sub-element.
 */
int
f_fracture(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    register int i;
    struct directory *old_dp;
    struct rt_db_internal	old_intern;
    struct model	*m, *new_model;
    char		newname[32];
    char		prefix[32];
    int	maxdigits;
    struct nmgregion *r, *new_r;
    struct shell *s, *new_s;
    struct faceuse *fu;
    struct vertex *v_new, *v;
    unsigned long tw, tf, tp;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 2 || 3 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help fracture");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    Tcl_AppendResult(interp, "fracture:", (char *)NULL);
    for (i=0; i < argc; i++)
	Tcl_AppendResult(interp, " ", argv[i], (char *)NULL);
    Tcl_AppendResult(interp, "\n", (char *)NULL);

    if ( (old_dp = db_lookup( dbip,  argv[1], LOOKUP_NOISY )) == DIR_NULL )
	return TCL_ERROR;

    if ( rt_db_get_internal( &old_intern, old_dp, dbip, bn_mat_identity, &rt_uniresource ) < 0 )  {
	Tcl_AppendResult(interp, "rt_db_get_internal() error\n", (char *)NULL);
	return TCL_ERROR;
    }

    if ( old_intern.idb_type != ID_NMG )
    {
	Tcl_AppendResult(interp, argv[1], " is not an NMG solid!!\n", (char *)NULL );
	rt_db_free_internal( &old_intern, &rt_uniresource );
	return TCL_ERROR;
    }

    m = (struct model *)old_intern.idb_ptr;
    NMG_CK_MODEL(m);

    /* how many characters of the solid names do we reserve for digits? */
    nmg_count_shell_kids(m, &tf, &tw, &tp);

    maxdigits = (int)(log10((double)(tf+tw+tp)) + 1.0);

    {
	struct bu_vls tmp_vls;

	bu_vls_init(&tmp_vls);
	bu_vls_printf(&tmp_vls, "%ld = %d digits\n", (long)(tf+tw+tp), maxdigits);
	Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
	bu_vls_free(&tmp_vls);
    }

    /*	for (maxdigits=1, i=tf+tw+tp; i > 0; i /= 10)
     *	maxdigits++;
     */

    /* get the prefix for the solids to be created. */
    memset(prefix, 0, sizeof(prefix));
    bu_strlcpy(prefix, argv[argc-1], sizeof(prefix));
    bu_strlcat(prefix, "_", sizeof(prefix));

    /* Bust it up here */

    i = 1;
    for (BU_LIST_FOR(r, nmgregion, &m->r_hd)) {
	NMG_CK_REGION(r);
	for (BU_LIST_FOR(s, shell, &r->s_hd)) {
	    NMG_CK_SHELL(s);
	    if (s->vu_p) {
		NMG_CK_VERTEXUSE(s->vu_p);
		NMG_CK_VERTEX(s->vu_p->v_p);
		v = s->vu_p->v_p;

/*	nmg_start_dup(m); */
		new_model = nmg_mm();
		new_r = nmg_mrsv(new_model);
		new_s = BU_LIST_FIRST(shell, &r->s_hd);
		v_new = new_s->vu_p->v_p;
		if (v->vg_p) {
		    nmg_vertex_gv(v_new, v->vg_p->coord);
		}
/*	nmg_end_dup(); */

		snprintf(newname, 32, "%s%0*d", prefix, maxdigits, i++);

		mged_add_nmg_part(newname, new_model);
		if (frac_stat) return CMD_BAD;
		continue;
	    }
	    for (BU_LIST_FOR(fu, faceuse, &s->fu_hd)) {
		if (fu->orientation != OT_SAME)
		    continue;

		NMG_CK_FACEUSE(fu);

		new_model = nmg_mm();
		NMG_CK_MODEL(new_model);
		new_r = nmg_mrsv(new_model);
		NMG_CK_REGION(new_r);
		new_s = BU_LIST_FIRST(shell, &new_r->s_hd);
		NMG_CK_SHELL(new_s);
/*	nmg_start_dup(m); */
		NMG_CK_SHELL(new_s);
		nmg_dup_face(fu, new_s);
/*	nmg_end_dup(); */

		snprintf(newname, 32, "%s%0*d", prefix, maxdigits, i++);
		mged_add_nmg_part(newname, new_model);
		if (frac_stat) return CMD_BAD;
	    }
#if 0
	    while (BU_LIST_NON_EMPTY(&s->lu_hd)) {
		lu = BU_LIST_FIRST(loopuse, &s->lu_hd);
		new_model = nmg_mm();
		r = nmg_mrsv(new_model);
		new_s = BU_LIST_FIRST(shell, &r->s_hd);

		nmg_dup_loop(lu, new_s);
		nmg_klu(lu);

		snprintf(newname, 32, "%s%0*d", prefix, maxdigits, i++);
		mged_add_nmg_part(newname, new_model);
		if (frac_stat) return CMD_BAD;
	    }
	    while (BU_LIST_NON_EMPTY(&s->eu_hd)) {
		eu = BU_LIST_FIRST(edgeuse, &s->eu_hd);
		new_model = nmg_mm();
		r = nmg_mrsv(new_model);
		new_s = BU_LIST_FIRST(shell, &r->s_hd);

		nmg_dup_edge(eu, new_s);
		nmg_keu(eu);

		snprintf(newname, 32, "%s%0*d", prefix, maxdigits, i++);

		mged_add_nmg_part(newname, new_model);
		if (frac_stat) return TCL_ERROR;
	    }
#endif
	}
    }
    return TCL_OK;

}
/*
 *			F _ Q O R O T
 *
 * Usage: qorot x y z dx dy dz theta
 *
 *	rotate an object through a specified angle
 *	about a specified ray.
 */
int
f_qorot(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    mat_t temp;
    vect_t s_point, point, v_work, model_pt;
    vect_t	specified_pt, direc;

    CHECK_DBI_NULL;
    CHECK_READ_ONLY;

    if (argc < 8 || 8 < argc) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "help qorot");
	Tcl_Eval(interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
	return TCL_ERROR;
    }

    if ( not_state( ST_O_EDIT, "Object Rotation" ) )
	return TCL_ERROR;

    if (movedir != ROTARROW) {
	/* NOT in object rotate mode - put it in obj rot */
	movedir = ROTARROW;
    }
    VSET(specified_pt, atof(argv[1]), atof(argv[2]), atof(argv[3]));
    VSCALE(specified_pt, specified_pt, dbip->dbi_local2base);
    VSET(direc, atof(argv[4]), atof(argv[5]), atof(argv[6]));

    /* find point for rotation to take place wrt */
    MAT4X3PNT(model_pt, es_mat, specified_pt);
    MAT4X3PNT(point, modelchanges, model_pt);

    /* Find absolute translation vector to go from "model_pt" to
     * 	"point" without any of the rotations in "modelchanges"
     */
    VSCALE(s_point, point, modelchanges[15]);
    VSUB2(v_work, s_point, model_pt);

    /* REDO "modelchanges" such that:
     *	1. NO rotations (identity)
     *	2. trans == v_work
     *	3. same scale factor
     */
    MAT_IDN(temp);
    MAT_DELTAS_VEC(temp, v_work);
    temp[15] = modelchanges[15];
    MAT_COPY(modelchanges, temp);

    /* build new rotation matrix */
    MAT_IDN(temp);
    bn_mat_angles(temp, 0.0, 0.0, atof(argv[7]));

    /* Record the new rotation matrix into the revised
     *	modelchanges matrix wrt "point"
     */
    wrt_point_direc(modelchanges, temp, modelchanges, point, direc);

#ifdef DO_NEW_EDIT_MATS
    new_edit_mats();
#else
    new_mats();
#endif

    return TCL_OK;
}

void
set_localunit_TclVar(void)
{
    struct bu_vls vls;
    struct bu_vls units_vls;
    const char	*str;

    if (dbip == DBI_NULL)
	return;

    bu_vls_init(&vls);
    bu_vls_init(&units_vls);

    str = bu_units_string(dbip->dbi_local2base);
    if (str)
	bu_vls_strcpy(&units_vls, str);
    else
	bu_vls_printf(&units_vls, "%gmm", dbip->dbi_local2base);

    bu_vls_strcpy(&vls, "localunit");
    Tcl_SetVar(interp, bu_vls_addr(&vls), bu_vls_addr(&units_vls), TCL_GLOBAL_ONLY);

    bu_vls_free(&vls);
    bu_vls_free(&units_vls);
}


int
f_binary(ClientData	clientData,
	 Tcl_Interp	*interp,
	 int		argc,
	 char 		**argv)
{
    CHECK_DBI_NULL;

    return wdb_binary_cmd(wdbp, interp, argc, argv);
}

int cmd_bot_smooth( ClientData	clientData,
		    Tcl_Interp	*interp,
		    int		argc,
		    char 		**argv)
{
    CHECK_DBI_NULL;

    return wdb_bot_smooth_cmd( wdbp, interp, argc, argv );
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
