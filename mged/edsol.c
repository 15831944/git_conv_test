/*
 *			E D S O L . C
 *
 * Functions -
 *	init_sedit	set up for a Solid Edit
 *	sedit		Apply Solid Edit transformation(s)
 *	pscale		Partial scaling of a solid
 *	init_objedit	set up for object edit?
 *	f_dextrude()	extrude a drawing (nmg wire loop) to create a solid
 *	f_eqn		change face of GENARB8 to new equation
 *
 *  Authors -
 *	Keith A. Applin
 *	Bob Suckling
 *	Michael John Muuss
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

#include "conf.h"

#include <stdio.h>
#include <math.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "db.h"
#include "nmg.h"
#include "raytrace.h"
#include "nurb.h"
#include "rtgeom.h"
#include "wdb.h"

#include "./ged.h"
#include "./solid.h"
#include "./sedit.h"
#include "./dm.h"
#include "./menu.h"

extern struct rt_tol		mged_tol;	/* from ged.c */

#ifdef XMGED
extern void set_e_axis_pos();
extern void (*tran_hook)();
extern void (*rot_hook)();
extern int irot_set;
extern double irot_x;
extern double irot_y;
extern double irot_z;
extern int tran_set;
extern double tran_x;
extern double tran_y;
extern double tran_z;
extern int      update_views;
extern int      savedit;

void set_tran();
#endif

static void	arb8_edge(), ars_ed(), ell_ed(), tgc_ed(), tor_ed(), spline_ed();
static void	nmg_ed();
static void	rpc_ed(), rhc_ed(), epa_ed(), ehy_ed(), eto_ed();
static void	arb7_edge(), arb6_edge(), arb5_edge(), arb4_point();
static void	arb8_mv_face(), arb7_mv_face(), arb6_mv_face();
static void	arb5_mv_face(), arb4_mv_face(), arb8_rot_face(), arb7_rot_face();
static void 	arb6_rot_face(), arb5_rot_face(), arb4_rot_face(), arb_control();

void pscale();
void	calc_planes();

#ifdef XMGED
short int fixv;		/* used in ECMD_ARB_ROTATE_FACE,f_eqn(): fixed vertex */
#else
static short int fixv;		/* used in ECMD_ARB_ROTATE_FACE,f_eqn(): fixed vertex */
#endif

MGED_EXTERN( fastf_t nmg_loop_plane_area , ( struct loopuse *lu , plane_t pl ) );

/* data for solid editing */
int			sedraw;	/* apply solid editing changes */

struct rt_external	es_ext;
struct rt_db_internal	es_int;
struct rt_db_internal	es_int_orig;

int	es_type;		/* COMGEOM solid type */
int     es_edflag;		/* type of editing for this solid */
fastf_t	es_scale;		/* scale factor */
fastf_t	es_peqn[7][4];		/* ARBs defining plane equations */
fastf_t	es_m[3];		/* edge(line) slope */
mat_t	es_mat;			/* accumulated matrix of path */ 
mat_t 	es_invmat;		/* inverse of es_mat   KAA */

point_t	es_keypoint;		/* center of editing xforms */
char	*es_keytag;		/* string identifying the keypoint */
int	es_keyfixed;		/* keypoint specified by user? */

vect_t		es_para;	/* keyboard input param. Only when inpara set.  */
extern int	inpara;		/* es_para valid.  es_mvalid mus = 0 */
static vect_t	es_mparam;	/* mouse input param.  Only when es_mvalid set */
static int	es_mvalid;	/* es_mparam valid.  inpara must = 0 */

static int	spl_surfno;	/* What surf & ctl pt to edit on spline */
static int	spl_ui;
static int	spl_vi;

static struct edgeuse	*es_eu=(struct edgeuse *)NULL;	/* Currently selected NMG edgeuse */
static struct loopuse	*lu_copy=(struct loopuse*)NULL;	/* copy of loop to be extruded */
static plane_t		lu_pl;	/* plane equation for loop to be extruded */
static struct shell	*es_s=(struct shell *)NULL;	/* Shell where extrusion is to end up */
static point_t		lu_keypoint;	/* keypoint of lu_copy for extrusion */

/*  These values end up in es_menu, as do ARB vertex numbers */
int	es_menu;		/* item selected from menu */
#define MENU_TOR_R1		21
#define MENU_TOR_R2		22
#define MENU_TGC_ROT_H		23
#define MENU_TGC_ROT_AB 	24
#define	MENU_TGC_MV_H		25
#define MENU_TGC_MV_HH		26
#define MENU_TGC_SCALE_H	27
#define MENU_TGC_SCALE_A	28
#define MENU_TGC_SCALE_B	29
#define MENU_TGC_SCALE_C	30
#define MENU_TGC_SCALE_D	31
#define MENU_TGC_SCALE_AB	32
#define MENU_TGC_SCALE_CD	33
#define MENU_TGC_SCALE_ABCD	34
#define MENU_ARB_MV_EDGE	35
#define MENU_ARB_MV_FACE	36
#define MENU_ARB_ROT_FACE	37
#define MENU_ELL_SCALE_A	38
#define MENU_ELL_SCALE_B	39
#define MENU_ELL_SCALE_C	40
#define MENU_ELL_SCALE_ABC	41
#define MENU_RPC_B		42
#define MENU_RPC_H		43
#define MENU_RPC_R		44
#define MENU_RHC_B		45
#define MENU_RHC_H		46
#define MENU_RHC_R		47
#define MENU_RHC_C		48
#define MENU_EPA_H		49
#define MENU_EPA_R1		50
#define MENU_EPA_R2		51
#define MENU_EHY_H		52
#define MENU_EHY_R1		53
#define MENU_EHY_R2		54
#define MENU_EHY_C		55
#define MENU_ETO_R		56
#define MENU_ETO_RD		57
#define MENU_ETO_SCALE_C	58
#define MENU_ETO_ROT_C		59

extern int arb_faces[5][24];	/* from edarb.c */

struct menu_item  edge8_menu[] = {
	{ "ARB8 EDGES", (void (*)())NULL, 0 },
	{ "move edge 12", arb8_edge, 0 },
	{ "move edge 23", arb8_edge, 1 },
	{ "move edge 34", arb8_edge, 2 },
	{ "move edge 14", arb8_edge, 3 },
	{ "move edge 15", arb8_edge, 4 },
	{ "move edge 26", arb8_edge, 5 },
	{ "move edge 56", arb8_edge, 6 },
	{ "move edge 67", arb8_edge, 7 },
	{ "move edge 78", arb8_edge, 8 },
	{ "move edge 58", arb8_edge, 9 },
	{ "move edge 37", arb8_edge, 10 },
	{ "move edge 48", arb8_edge, 11 },
	{ "RETURN",       arb8_edge, 12 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  edge7_menu[] = {
	{ "ARB7 EDGES", (void (*)())NULL, 0 },
	{ "move edge 12", arb7_edge, 0 },
	{ "move edge 23", arb7_edge, 1 },
	{ "move edge 34", arb7_edge, 2 },
	{ "move edge 14", arb7_edge, 3 },
	{ "move edge 15", arb7_edge, 4 },
	{ "move edge 26", arb7_edge, 5 },
	{ "move edge 56", arb7_edge, 6 },
	{ "move edge 67", arb7_edge, 7 },
	{ "move edge 37", arb7_edge, 8 },
	{ "move edge 57", arb7_edge, 9 },
	{ "move edge 45", arb7_edge, 10 },
	{ "move point 5", arb7_edge, 11 },
	{ "RETURN",       arb7_edge, 12 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  edge6_menu[] = {
	{ "ARB6 EDGES", (void (*)())NULL, 0 },
	{ "move edge 12", arb6_edge, 0 },
	{ "move edge 23", arb6_edge, 1 },
	{ "move edge 34", arb6_edge, 2 },
	{ "move edge 14", arb6_edge, 3 },
	{ "move edge 15", arb6_edge, 4 },
	{ "move edge 25", arb6_edge, 5 },
	{ "move edge 36", arb6_edge, 6 },
	{ "move edge 46", arb6_edge, 7 },
	{ "move point 5", arb6_edge, 8 },
	{ "move point 6", arb6_edge, 9 },
	{ "RETURN",       arb6_edge, 10 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  edge5_menu[] = {
	{ "ARB5 EDGES", (void (*)())NULL, 0 },
	{ "move edge 12", arb5_edge, 0 },
	{ "move edge 23", arb5_edge, 1 },
	{ "move edge 34", arb5_edge, 2 },
	{ "move edge 14", arb5_edge, 3 },
	{ "move edge 15", arb5_edge, 4 },
	{ "move edge 25", arb5_edge, 5 },
	{ "move edge 35", arb5_edge, 6 },
	{ "move edge 45", arb5_edge, 7 },
	{ "move point 5", arb5_edge, 8 },
	{ "RETURN",       arb5_edge, 9 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  point4_menu[] = {
	{ "ARB4 POINTS", (void (*)())NULL, 0 },
	{ "move point 1", arb4_point, 0 },
	{ "move point 2", arb4_point, 1 },
	{ "move point 3", arb4_point, 2 },
	{ "move point 4", arb4_point, 4 },
	{ "RETURN",       arb4_point, 5 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  tgc_menu[] = {
	{ "TGC MENU", (void (*)())NULL, 0 },
	{ "scale H",	tgc_ed, MENU_TGC_SCALE_H },
	{ "scale A",	tgc_ed, MENU_TGC_SCALE_A },
	{ "scale B",	tgc_ed, MENU_TGC_SCALE_B },
	{ "scale c",	tgc_ed, MENU_TGC_SCALE_C },
	{ "scale d",	tgc_ed, MENU_TGC_SCALE_D },
	{ "scale A,B",	tgc_ed, MENU_TGC_SCALE_AB },
	{ "scale C,D",	tgc_ed, MENU_TGC_SCALE_CD },
	{ "scale A,B,C,D", tgc_ed, MENU_TGC_SCALE_ABCD },
	{ "rotate H",	tgc_ed, MENU_TGC_ROT_H },
	{ "rotate AxB",	tgc_ed, MENU_TGC_ROT_AB },
	{ "move end H(rt)", tgc_ed, MENU_TGC_MV_H },
	{ "move end H", tgc_ed, MENU_TGC_MV_HH },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  tor_menu[] = {
	{ "TORUS MENU", (void (*)())NULL, 0 },
	{ "scale radius 1", tor_ed, MENU_TOR_R1 },
	{ "scale radius 2", tor_ed, MENU_TOR_R2 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  eto_menu[] = {
	{ "ELL-TORUS MENU", (void (*)())NULL, 0 },
	{ "scale r", eto_ed, MENU_ETO_R },
	{ "scale D", eto_ed, MENU_ETO_RD },
	{ "scale C", eto_ed, MENU_ETO_SCALE_C },
	{ "rotate C", eto_ed, MENU_ETO_ROT_C },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  ell_menu[] = {
	{ "ELLIPSOID MENU", (void (*)())NULL, 0 },
	{ "scale A", ell_ed, MENU_ELL_SCALE_A },
	{ "scale B", ell_ed, MENU_ELL_SCALE_B },
	{ "scale C", ell_ed, MENU_ELL_SCALE_C },
	{ "scale A,B,C", ell_ed, MENU_ELL_SCALE_ABC },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  ars_menu[] = {
	{ "ARS MENU", (void (*)())NULL, 0 },
	{ "not implemented", ars_ed, 1 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  spline_menu[] = {
	{ "SPLINE MENU", (void (*)())NULL, 0 },
	{ "pick vertex", spline_ed, -1 },
	{ "move vertex", spline_ed, ECMD_VTRANS },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  nmg_menu[] = {
	{ "NMG MENU", (void (*)())NULL, 0 },
	{ "pick edge", nmg_ed, ECMD_NMG_EPICK },
	{ "move edge", nmg_ed, ECMD_NMG_EMOVE },
	{ "split edge", nmg_ed, ECMD_NMG_ESPLIT },
	{ "delete edge", nmg_ed, ECMD_NMG_EKILL },
	{ "next eu", nmg_ed, ECMD_NMG_FORW },
	{ "prev eu", nmg_ed, ECMD_NMG_BACK },
	{ "radial eu", nmg_ed, ECMD_NMG_RADIAL },
	{ "extrude loop", nmg_ed , ECMD_NMG_LEXTRU },
	{ "debug edge", nmg_ed, ECMD_NMG_EDEBUG },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv8_menu[] = {
	{ "ARB8 FACES", (void (*)())NULL, 0 },
	{ "move face 1234", arb8_mv_face, 1 },
	{ "move face 5678", arb8_mv_face, 2 },
	{ "move face 1584", arb8_mv_face, 3 },
	{ "move face 2376", arb8_mv_face, 4 },
	{ "move face 1265", arb8_mv_face, 5 },
	{ "move face 4378", arb8_mv_face, 6 },
	{ "RETURN",         arb8_mv_face, 7 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv7_menu[] = {
	{ "ARB7 FACES", (void (*)())NULL, 0 },
	{ "move face 1234", arb7_mv_face, 1 },
	{ "move face 2376", arb7_mv_face, 4 },
	{ "RETURN",         arb7_mv_face, 7 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv6_menu[] = {
	{ "ARB6 FACES", (void (*)())NULL, 0 },
	{ "move face 1234", arb6_mv_face, 1 },
	{ "move face 2365", arb6_mv_face, 2 },
	{ "move face 1564", arb6_mv_face, 3 },
	{ "move face 125" , arb6_mv_face, 4 },
	{ "move face 346" , arb6_mv_face, 5 },
	{ "RETURN",         arb6_mv_face, 6 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv5_menu[] = {
	{ "ARB5 FACES", (void (*)())NULL, 0 },
	{ "move face 1234", arb5_mv_face, 1 },
	{ "move face 125" , arb5_mv_face, 2 },
	{ "move face 235" , arb5_mv_face, 3 },
	{ "move face 345" , arb5_mv_face, 4 },
	{ "move face 145" , arb5_mv_face, 5 },
	{ "RETURN",         arb5_mv_face, 6 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item mv4_menu[] = {
	{ "ARB4 FACES", (void (*)())NULL, 0 },
	{ "move face 123" , arb4_mv_face, 1 },
	{ "move face 124" , arb4_mv_face, 2 },
	{ "move face 234" , arb4_mv_face, 3 },
	{ "move face 134" , arb4_mv_face, 4 },
	{ "RETURN",         arb4_mv_face, 5 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item rot8_menu[] = {
	{ "ARB8 FACES", (void (*)())NULL, 0 },
	{ "rotate face 1234", arb8_rot_face, 1 },
	{ "rotate face 5678", arb8_rot_face, 2 },
	{ "rotate face 1584", arb8_rot_face, 3 },
	{ "rotate face 2376", arb8_rot_face, 4 },
	{ "rotate face 1265", arb8_rot_face, 5 },
	{ "rotate face 4378", arb8_rot_face, 6 },
	{ "RETURN",         arb8_rot_face, 7 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item rot7_menu[] = {
	{ "ARB7 FACES", (void (*)())NULL, 0 },
	{ "rotate face 1234", arb7_rot_face, 1 },
	{ "rotate face 567" , arb7_rot_face, 2 },
	{ "rotate face 145" , arb7_rot_face, 3 },
	{ "rotate face 2376", arb7_rot_face, 4 },
	{ "rotate face 1265", arb7_rot_face, 5 },
	{ "rotate face 4375", arb7_rot_face, 6 },
	{ "RETURN",         arb7_rot_face, 7 },
	{ "", (void (*)())NULL, 0 }
};



struct menu_item rot6_menu[] = {
	{ "ARB6 FACES", (void (*)())NULL, 0 },
	{ "rotate face 1234", arb6_rot_face, 1 },
	{ "rotate face 2365", arb6_rot_face, 2 },
	{ "rotate face 1564", arb6_rot_face, 3 },
	{ "rotate face 125" , arb6_rot_face, 4 },
	{ "rotate face 346" , arb6_rot_face, 5 },
	{ "RETURN",         arb6_rot_face, 6 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item rot5_menu[] = {
	{ "ARB5 FACES", (void (*)())NULL, 0 },
	{ "rotate face 1234", arb5_rot_face, 1 },
	{ "rotate face 125" , arb5_rot_face, 2 },
	{ "rotate face 235" , arb5_rot_face, 3 },
	{ "rotate face 345" , arb5_rot_face, 4 },
	{ "rotate face 145" , arb5_rot_face, 5 },
	{ "RETURN",         arb5_rot_face, 6 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item rot4_menu[] = {
	{ "ARB4 FACES", (void (*)())NULL, 0 },
	{ "rotate face 123" , arb4_rot_face, 1 },
	{ "rotate face 124" , arb4_rot_face, 2 },
	{ "rotate face 234" , arb4_rot_face, 3 },
	{ "rotate face 134" , arb4_rot_face, 4 },
	{ "RETURN",         arb4_rot_face, 5 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item cntrl_menu[] = {
	{ "ARB MENU", (void (*)())NULL, 0 },
	{ "move edges", arb_control, MENU_ARB_MV_EDGE },
	{ "move faces", arb_control, MENU_ARB_MV_FACE },
	{ "rotate faces", arb_control, MENU_ARB_ROT_FACE },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  rpc_menu[] = {
	{ "RPC MENU", (void (*)())NULL, 0 },
	{ "scale B", rpc_ed, MENU_RPC_B },
	{ "scale H", rpc_ed, MENU_RPC_H },
	{ "scale r", rpc_ed, MENU_RPC_R },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  rhc_menu[] = {
	{ "RHC MENU", (void (*)())NULL, 0 },
	{ "scale B", rhc_ed, MENU_RHC_B },
	{ "scale H", rhc_ed, MENU_RHC_H },
	{ "scale r", rhc_ed, MENU_RHC_R },
	{ "scale c", rhc_ed, MENU_RHC_C },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  epa_menu[] = {
	{ "EPA MENU", (void (*)())NULL, 0 },
	{ "scale H", epa_ed, MENU_EPA_H },
	{ "scale A", epa_ed, MENU_EPA_R1 },
	{ "scale B", epa_ed, MENU_EPA_R2 },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item  ehy_menu[] = {
	{ "EHY MENU", (void (*)())NULL, 0 },
	{ "scale H", ehy_ed, MENU_EHY_H },
	{ "scale A", ehy_ed, MENU_EHY_R1 },
	{ "scale B", ehy_ed, MENU_EHY_R2 },
	{ "scale c", ehy_ed, MENU_EHY_C },
	{ "", (void (*)())NULL, 0 }
};

struct menu_item *which_menu[] = {
	point4_menu,
	edge5_menu,
	edge6_menu,
	edge7_menu,
	edge8_menu,
	mv4_menu,
	mv5_menu,
	mv6_menu,
	mv7_menu,
	mv8_menu,
	rot4_menu,
	rot5_menu,
	rot6_menu,
	rot7_menu,
	rot8_menu
};

short int arb_vertices[5][24] = {
	{ 1,2,3,0, 1,2,4,0, 2,3,4,0, 1,3,4,0, 0,0,0,0, 0,0,0,0 },	/* arb4 */
	{ 1,2,3,4, 1,2,5,0, 2,3,5,0, 3,4,5,0, 1,4,5,0, 0,0,0,0 },	/* arb5 */
	{ 1,2,3,4, 2,3,6,5, 1,5,6,4, 1,2,5,0, 3,4,6,0, 0,0,0,0 },	/* arb6 */
	{ 1,2,3,4, 5,6,7,0, 1,4,5,0, 2,3,7,6, 1,2,6,5, 4,3,7,5 },	/* arb7 */
	{ 1,2,3,4, 5,6,7,8, 1,5,8,4, 2,3,7,6, 1,2,6,5, 4,3,7,8 }	/* arb8 */
};

static void
arb8_edge( arg )
int arg;
{
	es_menu = arg;
	es_edflag = EARB;
	if(arg == 12)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb7_edge( arg )
int arg;
{
	es_menu = arg;
	es_edflag = EARB;
	if(arg == 11) {
		/* move point 5 */
		es_edflag = PTARB;
		es_menu = 4;	/* location of point */
	}
	if(arg == 12)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb6_edge( arg )
int arg;
{
	es_menu = arg;
	es_edflag = EARB;
	if(arg == 8) {
		/* move point 5   location = 4 */
		es_edflag = PTARB;
		es_menu = 4;
	}
	if(arg == 9) {
		/* move point 6   location = 6 */
		es_edflag = PTARB;
		es_menu = 6;
	}
	if(arg == 10)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb5_edge( arg )
int arg;
{
	es_menu = arg;
	es_edflag = EARB;
	if(arg == 8) {
		/* move point 5 at loaction 4 */
		es_edflag = PTARB;
		es_menu = 4;
	}
	if(arg == 9)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb4_point( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PTARB;
	if(arg == 5)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
tgc_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;
	if(arg == MENU_TGC_ROT_H )
		es_edflag = ECMD_TGC_ROT_H;
	if(arg == MENU_TGC_ROT_AB)
		es_edflag = ECMD_TGC_ROT_AB;
	if(arg == MENU_TGC_MV_H)
		es_edflag = ECMD_TGC_MV_H;
	if(arg == MENU_TGC_MV_HH)
		es_edflag = ECMD_TGC_MV_HH;

#ifdef XMGED
	set_e_axis_pos();
#endif
}


static void
tor_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
eto_ed( arg )
int arg;
{
	es_menu = arg;
	if(arg == MENU_ETO_ROT_C )
		es_edflag = ECMD_ETO_ROT_C;
	else
		es_edflag = PSCALE;

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
rpc_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
rhc_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
epa_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
ehy_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
ell_ed( arg )
int arg;
{
	es_menu = arg;
	es_edflag = PSCALE;

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb8_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 7)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb7_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 7)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}		

static void
arb6_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 6)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb5_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 6)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb4_mv_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_MOVE_FACE;
	if(arg == 5)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}

#ifdef XMGED
	set_e_axis_pos();
#endif
}

static void
arb8_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 7)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb7_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 7)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}		

static void
arb6_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 6)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb5_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 6)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb4_rot_face( arg )
int arg;
{
	es_menu = arg - 1;
	es_edflag = ECMD_ARB_SETUP_ROTFACE;
	sedraw = 1;
	if(arg == 5)  {
		es_edflag = ECMD_ARB_MAIN_MENU;
		sedraw = 1;
	}
}

static void
arb_control( arg )
int arg;
{
	es_menu = arg;
	es_edflag = ECMD_ARB_SPECIFIC_MENU;
	sedraw = 1;
}

/*ARGSUSED*/
static void
ars_ed( arg )
int arg;
{
	rt_log("NOT IMPLEMENTED YET\n");
}


/*ARGSUSED*/
static void
spline_ed( arg )
int arg;
{
	/* XXX Why wasn't this done by setting es_edflag = ECMD_SPLINE_VPICK? */
	if( arg < 0 )  {
		/* Enter picking state */
		chg_state( ST_S_EDIT, ST_S_VPICK, "Vertex Pick" );
		return;
	}
	/* For example, this will set es_edflag = ECMD_VTRANS */
	es_edflag = arg;
	sedraw = 1;

#ifdef XMGED
	set_e_axis_pos();
#endif
}
/*
 *			N M G _ E D
 *
 *  Handler for events in the NMG menu.
 *  Mostly just set appropriate state flags to prepare us for user's
 *  next event.
 */
/*ARGSUSED*/
static void
nmg_ed( arg )
int arg;
{
	switch(arg)  {
	default:
		rt_log("nmg_ed: undefined menu event?\n");
		return;
	case ECMD_NMG_EPICK:
	case ECMD_NMG_EMOVE:
	case ECMD_NMG_ESPLIT:
	case ECMD_NMG_EKILL:
		break;
	case ECMD_NMG_EDEBUG:
		if( !es_eu )  {
			rt_log("nmg_ed: no edge selected yet\n");
			return;
		}

		nmg_pr_fu_around_eu( es_eu, &mged_tol );
		{
			struct model		*m;
			struct rt_vlblock	*vbp;
			long			*tab;

			m = nmg_find_model( &es_eu->l.magic );
			NMG_CK_MODEL(m);

			if( es_eu->g.magic_p )
			{
				/* get space for list of items processed */
				tab = (long *)rt_calloc( m->maxindex+1, sizeof(long),
					"nmg_ed tab[]");
				vbp = rt_vlblock_init();

				nmg_vlblock_around_eu(vbp, es_eu, tab, 1, &mged_tol);
				cvt_vlblock_to_solids( vbp, "_EU_", 0 );	/* swipe vlist */

				rt_vlblock_free(vbp);
				rt_free( (char *)tab, "nmg_ed tab[]" );
			}
			dmaflag = 1;
		}
		if( *es_eu->up.magic_p == NMG_LOOPUSE_MAGIC )  {
			nmg_veu( &es_eu->up.lu_p->down_hd, es_eu->up.magic_p );
		}
		/* no change of state or es_edflag */
		return;
	case ECMD_NMG_FORW:
		if( !es_eu )  {
			rt_log("nmg_ed: no edge selected yet\n");
			return;
		}
		NMG_CK_EDGEUSE(es_eu);
		es_eu = RT_LIST_PNEXT_CIRC(edgeuse, es_eu);
		rt_log("edgeuse selected=x%x\n", es_eu);
		sedraw = 1;
		return;
	case ECMD_NMG_BACK:
		if( !es_eu )  {
			rt_log("nmg_ed: no edge selected yet\n");
			return;
		}
		NMG_CK_EDGEUSE(es_eu);
		es_eu = RT_LIST_PPREV_CIRC(edgeuse, es_eu);
		rt_log("edgeuse selected=x%x\n", es_eu);
		sedraw = 1;
		return;
	case ECMD_NMG_RADIAL:
		if( !es_eu )  {
			rt_log("nmg_ed: no edge selected yet\n");
			return;
		}
		NMG_CK_EDGEUSE(es_eu);
		es_eu = es_eu->eumate_p->radial_p;
		rt_log("edgeuse selected=x%x\n", es_eu);
		sedraw = 1;
		return;
	case ECMD_NMG_LEXTRU:
		{
			struct model *m,*m_tmp;
			struct nmgregion *r,*r_tmp;
			struct shell *s,*s_tmp;
			struct loopuse *lu=(struct loopuse *)NULL;
			struct loopuse *lu_tmp;
			struct edgeuse *eu;
			fastf_t area;
			int wire_loop_count=0;

			m = (struct model *)es_int.idb_ptr;
			NMG_CK_MODEL( m );

			/* look for wire loops */
			for( RT_LIST_FOR( r , nmgregion , &m->r_hd ) )
			{
				NMG_CK_REGION( r );
				for( RT_LIST_FOR( s , shell , &r->s_hd ) )
				{
					if( RT_LIST_IS_EMPTY( &s->lu_hd ) )
						continue;

					for( RT_LIST_FOR( lu_tmp , loopuse , &s->lu_hd ) )
					{
						if( !lu )
							lu = lu_tmp;
						else if( lu_tmp == lu->lumate_p )
							continue;

						wire_loop_count++;
					}
				}
			}

			if( !wire_loop_count )
			{
				rt_log( "No sketch (wire loop) to extrude\n" );
				return;
			}

			if( wire_loop_count > 1 )
			{
				rt_log( "Too many wire loops!!! Don't know which to extrude!!\n" );
				return;
			}

			if( !lu | *lu->up.magic_p != NMG_SHELL_MAGIC )
			{
				/* This should never happen */
				rt_bomb( "Cannot find wire loop!!\n" );
			}

			/* Make sure loop is not a crack */
			area = nmg_loop_plane_area( lu , lu_pl );

			if( area < 0.0 )
			{
				rt_log( "Cannot extrude loop with no area\n" );
				return;
			}

			/* Check if loop crosses itself */
			for( RT_LIST_FOR( eu , edgeuse , &lu->down_hd ) )
			{
				struct edgeuse *eu2;
				struct vertex *v1;
				vect_t edge1;

				NMG_CK_EDGEUSE( eu );

				v1 = eu->vu_p->v_p;
				NMG_CK_VERTEX( v1 );
				VSUB2( edge1, eu->eumate_p->vu_p->v_p->vg_p->coord, v1->vg_p->coord );

				for( eu2 = RT_LIST_PNEXT( edgeuse , &eu->l ) ; RT_LIST_NOT_HEAD( eu2 , &lu->down_hd ) ; eu2=RT_LIST_PNEXT( edgeuse, &eu2->l) )
				{
					struct vertex *v2;
					vect_t edge2;
					fastf_t dist[2];
					int ret_val;

					NMG_CK_EDGEUSE( eu2 );

					if( eu2 == eu )
						continue;
					if( eu2 == RT_LIST_PNEXT_CIRC( edgeuse,  &eu->l ) )
						continue;
					if( eu2 == RT_LIST_PPREV_CIRC( edgeuse, &eu->l ) )
						continue;

					v2 = eu2->vu_p->v_p;
					NMG_CK_VERTEX( v2 );
					VSUB2( edge2, eu2->eumate_p->vu_p->v_p->vg_p->coord, v2->vg_p->coord );

					if( (ret_val=rt_isect_lseg3_lseg3( dist, v1->vg_p->coord, edge1,
						v2->vg_p->coord, edge2, &mged_tol )) > (-1) )
					{
						rt_log( "Loop crosses itself, cannot extrude\n" );
						rt_log( "edge1: pt=( %g %g %g ), dir=( %g %g %g)\n",
							V3ARGS( v1->vg_p->coord ), V3ARGS( edge1 ) );
						rt_log( "edge2: pt=( %g %g %g ), dir=( %g %g %g)\n",
							V3ARGS( v2->vg_p->coord ), V3ARGS( edge2 ) );
						if( ret_val == 0 )
							rt_log( "edges are collinear and overlap\n" );
						else
						{
							point_t isect_pt;

							VJOIN1( isect_pt, v1->vg_p->coord, dist[0], edge1 );
							rt_log( "edges intersect at ( %g %g %g )\n",
								V3ARGS( isect_pt ) );
						}
						return;
					}
				}
			}

			/* Create a temporary model to store the basis loop */
			m_tmp = nmg_mm();
			r_tmp = nmg_mrsv( m_tmp );
			s_tmp = RT_LIST_FIRST( shell , &r_tmp->s_hd );
			lu_copy = nmg_dup_loop( lu , &s_tmp->l.magic , (long **)0 );
			if( !lu_copy )
			{
				rt_log( "Failed to make copy of loop\n" );
				nmg_km( m_tmp );
				return;
			}

			/* Get the first vertex in the loop as the basis for extrusion */
			eu = RT_LIST_FIRST( edgeuse, &lu->down_hd );
			VMOVE( lu_keypoint , eu->vu_p->v_p->vg_p->coord );

			s = lu->up.s_p;
			
			if( RT_LIST_NON_EMPTY( &s->fu_hd ) )
			{
				/* make a new shell to hold the extruded solid */

				r = RT_LIST_FIRST( nmgregion , &m->r_hd );
				NMG_CK_REGION( r );
				es_s = nmg_msv( r );
			}
			else
				es_s = s;

		}
		break;
	}
	/* For example, this will set es_edflag = ECMD_NMG_EPICK */
	es_edflag = arg;
	sedraw = 1;
}

/*
 *  Keypoint in model space is established in "pt".
 *  If "str" is set, then that point is used, else default
 *  for this solid is selected and set.
 *  "str" may be a constant string, in either upper or lower case,
 *  or it may be something complex like "(3,4)" for an ARS or spline
 *  to select a particular vertex or control point.
 *
 *  XXX Perhaps this should be done via solid-specific parse tables,
 *  so that solids could be pretty-printed & structprint/structparse
 *  processed as well?
 */
void
get_solid_keypoint( pt, strp, ip, mat )
point_t		pt;
char		**strp;
struct rt_db_internal	*ip;
mat_t		mat;
{
	char	*cp = *strp;
	point_t	mpt;
	static char		buf[128];

	RT_CK_DB_INTERNAL( ip );

	switch( ip->idb_type )  {
	case ID_PARTICLE:
		{
			struct rt_part_internal *part =
				(struct rt_part_internal *)ip->idb_ptr;

			RT_PART_CK_MAGIC( part );

			if( !strcmp( cp , "V" ) )
			{
				VMOVE( mpt , part->part_V );
				*strp = "V";
			}
			else if( !strcmp( cp , "H" ) )
			{
				VADD2( mpt , part->part_V , part->part_H );
				*strp = "H";	
			}
			else	/* default */
			{
				VMOVE( mpt , part->part_V );
				*strp = "V";
			}
			break;
		}
	case ID_PIPE:
		{
			struct rt_pipe_internal *pipe =
				(struct rt_pipe_internal *)ip->idb_ptr;
			struct wdb_pipeseg *pipe_seg;

			RT_PIPE_CK_MAGIC( pipe );

			pipe_seg = RT_LIST_FIRST( wdb_pipeseg , &pipe->pipe_segs_head );
			VMOVE( mpt , pipe_seg->ps_start );
			*strp = "V";
			break;
		}
	case ID_ARBN:
		{
			struct rt_arbn_internal *arbn =
				(struct rt_arbn_internal *)ip->idb_ptr;
			int i,j,k;
			int good_vert=0;

			RT_ARBN_CK_MAGIC( arbn );
			for( i=0 ; i<arbn->neqn ; i++ )
			{
				for( j=i+1 ; j<arbn->neqn ; j++ )
				{
					for( k=j+1 ; k<arbn->neqn ; k++ )
					{
						if( !rt_mkpoint_3planes( mpt , arbn->eqn[i] , arbn->eqn[j] , arbn->eqn[k] ) )
						{
							int l;

							good_vert = 1;
							for( l=0 ; l<arbn->neqn ; l++ )
							{
								if( l == i || l == j || l == k )
									continue;

								if( DIST_PT_PLANE( mpt , arbn->eqn[l] ) > mged_tol.dist )
								{
									good_vert = 0;
									break;
								}
							}

							if( good_vert )
								break;
						}
						if( good_vert )
							break;
					}
					if( good_vert )
						break;
				}
				if( good_vert )
					break;
			}

			*strp = "V";
			break;
		}
	case ID_EBM:
		{
			struct rt_ebm_internal *ebm =
				(struct rt_ebm_internal *)ip->idb_ptr;
			point_t pt;

			RT_EBM_CK_MAGIC( ebm );

			VSETALL( pt , 0.0 );
			MAT4X3PNT( mpt , ebm->mat , pt );
			*strp = "V";
			break;
		}
	case ID_HF:
		{
			struct rt_hf_internal *hf =
				(struct rt_hf_internal *)ip->idb_ptr;

			RT_HF_CK_MAGIC( hf );

			VMOVE( mpt, hf->v );
			*strp = "V";
			break;
		}
	case ID_VOL:
		{
			struct rt_vol_internal *vol =
				(struct rt_vol_internal *)ip->idb_ptr;
			point_t pt;

			RT_VOL_CK_MAGIC( vol );

			VSETALL( pt , 0.0 );
			MAT4X3PNT( mpt , vol->mat , pt );
			*strp = "V";
			break;
		}
	case ID_HALF:
		{
			struct rt_half_internal *haf =
				(struct rt_half_internal *)ip->idb_ptr;
			RT_HALF_CK_MAGIC( haf );

			VSCALE( mpt , haf->eqn , haf->eqn[H] );
			*strp = "V";
			break;
		}
	case ID_ARB8:
		{
			struct rt_arb_internal *arb =
				(struct rt_arb_internal *)ip->idb_ptr;
			RT_ARB_CK_MAGIC( arb );

			if( *cp == 'V' ) {
				int vertex_number;
				char *ptr;

				ptr = cp + 1;
				vertex_number = (*ptr) - '0';
				if( vertex_number < 1 || vertex_number > 8 )
					vertex_number = 1;
				VMOVE( mpt , arb->pt[vertex_number-1] );
				sprintf( buf, "V%d", vertex_number );
				*strp = buf;
				break;
			}

			/* Default */
			VMOVE( mpt , arb->pt[0] );
			*strp = "V1";

			break;
		}
	case ID_ELL:
	case ID_SPH:
		{
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)ip->idb_ptr;
			RT_ELL_CK_MAGIC(ell);

			if( strcmp( cp, "V" ) == 0 )  {
				VMOVE( mpt, ell->v );
				*strp = "V";
				break;
			}
			if( strcmp( cp, "A" ) == 0 )  {
				VADD2( mpt , ell->v , ell->a );
				*strp = "A";
				break;
			}
			if( strcmp( cp, "B" ) == 0 )  {
				VADD2( mpt , ell->v , ell->b );
				*strp = "B";
				break;
			}
			if( strcmp( cp, "C" ) == 0 )  {
				VADD2( mpt , ell->v , ell->c );
				*strp = "C";
				break;
			}
			/* Default */
			VMOVE( mpt, ell->v );
			*strp = "V";
			break;
		}
	case ID_TOR:
		{
			struct rt_tor_internal	*tor = 
				(struct rt_tor_internal *)ip->idb_ptr;
			RT_TOR_CK_MAGIC(tor);

			if( strcmp( cp, "V" ) == 0 )  {
				VMOVE( mpt, tor->v );
				*strp = "V";
				break;
			}
			/* Default */
			VMOVE( mpt, tor->v );
			*strp = "V";
			break;
		}
	case ID_TGC:
	case ID_REC:
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)ip->idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( strcmp( cp, "V" ) == 0 )  {
				VMOVE( mpt, tgc->v );
				*strp = "V";
				break;
			}
			if( strcmp( cp, "H" ) == 0 )  {
				VMOVE( mpt, tgc->h );
				*strp = "H";
				break;
			}
			if( strcmp( cp, "A" ) == 0 )  {
				VMOVE( mpt, tgc->a );
				*strp = "A";
				break;
			}
			if( strcmp( cp, "B" ) == 0 )  {
				VMOVE( mpt, tgc->b );
				*strp = "B";
				break;
			}
			if( strcmp( cp, "C" ) == 0 )  {
				VMOVE( mpt, tgc->c );
				*strp = "C";
				break;
			}
			if( strcmp( cp, "D" ) == 0 )  {
				VMOVE( mpt, tgc->d );
				*strp = "D";
				break;
			}
			/* Default */
			VMOVE( mpt, tgc->v );
			*strp = "V";
			break;
		}
	case ID_BSPLINE:
		{
			register struct rt_nurb_internal *sip =
				(struct rt_nurb_internal *) es_int.idb_ptr;
			register struct snurb	*surf;
			register fastf_t	*fp;

			RT_NURB_CK_MAGIC(sip);
			surf = sip->srfs[spl_surfno];
			NMG_CK_SNURB(surf);
			fp = &RT_NURB_GET_CONTROL_POINT( surf, spl_ui, spl_vi );
			VMOVE( mpt, fp );
			sprintf(buf, "Surf %d, index %d,%d",
				spl_surfno, spl_ui, spl_vi );
			*strp = buf;
			break;
		}
	case ID_GRIP:
		{
			struct rt_grip_internal *gip =
				(struct rt_grip_internal *)ip->idb_ptr;
			RT_GRIP_CK_MAGIC(gip);
			VMOVE( mpt, gip->center);
			*strp = "C";
			break;
		}
	case ID_ARS:
		{
			register struct rt_ars_internal *ars =
				(struct rt_ars_internal *)es_int.idb_ptr;
			RT_ARS_CK_MAGIC( ars );

			VMOVE( mpt , ars->curves[0] );
			*strp = "V";
			break;
		}
	case ID_RPC:
		{
			struct rt_rpc_internal *rpc =
				(struct rt_rpc_internal *)ip->idb_ptr;
			RT_RPC_CK_MAGIC( rpc );

			VMOVE( mpt , rpc->rpc_V );
			*strp = "V";
			break;
		}
	case ID_RHC:
		{
			struct rt_rhc_internal *rhc =
				(struct rt_rhc_internal *)ip->idb_ptr;
			RT_RHC_CK_MAGIC( rhc );

			VMOVE( mpt , rhc->rhc_V );
			*strp = "V";
			break;
		}
	case ID_EPA:
		{
			struct rt_epa_internal *epa =
				(struct rt_epa_internal *)ip->idb_ptr;
			RT_EPA_CK_MAGIC( epa );

			VMOVE( mpt , epa->epa_V );
			*strp = "V";
			break;
		}
	case ID_EHY:
		{
			struct rt_ehy_internal *ehy =
				(struct rt_ehy_internal *)ip->idb_ptr;
			RT_EHY_CK_MAGIC( ehy );

			VMOVE( mpt , ehy->ehy_V );
			*strp = "V";
			break;
		}
	case ID_ETO:
		{
			struct rt_eto_internal *eto =
				(struct rt_eto_internal *)ip->idb_ptr;
			RT_ETO_CK_MAGIC( eto );

			VMOVE( mpt , eto->eto_V );
			*strp = "V";
			break;
		}
	case ID_POLY:
		{
			struct rt_pg_face_internal *poly;
			struct rt_pg_internal *pg = 
				(struct rt_pg_internal *)ip->idb_ptr;
			RT_PG_CK_MAGIC( pg );

			poly = pg->poly;
			VMOVE( mpt , poly->verts );
			*strp = "V";
			break;
		}
	case ID_NMG:
		{
			struct vertex *v;
			struct vertexuse *vu;
			struct edgeuse *eu;
			struct loopuse *lu;
			struct faceuse *fu;
			struct shell *s;
			struct nmgregion *r;
			register struct model *m =
				(struct model *) es_int.idb_ptr;
			NMG_CK_MODEL(m);
			/* XXX Fall through, for now (How about first vertex?? - JRA) */

			/* set default first */
			VSETALL( mpt, 0 );
			*strp = "(origin)";

			if( RT_LIST_IS_EMPTY( &m->r_hd ) )
				break;

			r = RT_LIST_FIRST( nmgregion , &m->r_hd );
			if( !r )
				break;
			NMG_CK_REGION( r );

			if( RT_LIST_IS_EMPTY( &r->s_hd ) )
				break;

			s = RT_LIST_FIRST( shell , &r->s_hd );
			if( !s )
				break;
			NMG_CK_SHELL( s );

			if( RT_LIST_IS_EMPTY( &s->fu_hd ) )
				fu = (struct faceuse *)NULL;
			else
				fu = RT_LIST_FIRST( faceuse , &s->fu_hd );
			if( fu )
			{
				NMG_CK_FACEUSE( fu );
				lu = RT_LIST_FIRST( loopuse , &fu->lu_hd );
				NMG_CK_LOOPUSE( lu );
				if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) == NMG_EDGEUSE_MAGIC )
				{
					eu = RT_LIST_FIRST( edgeuse , &lu->down_hd );
					NMG_CK_EDGEUSE( eu );
					NMG_CK_VERTEXUSE( eu->vu_p );
					v = eu->vu_p->v_p;
				}
				else
				{
					vu = RT_LIST_FIRST( vertexuse , &lu->down_hd );
					NMG_CK_VERTEXUSE( vu );
					v = vu->v_p;
				}
				NMG_CK_VERTEX( v );
				if( !v->vg_p )
					break;
				VMOVE( mpt , v->vg_p->coord );
				*strp = "V";
				break;
			}
			if( RT_LIST_IS_EMPTY( &s->lu_hd ) )
				lu = (struct loopuse *)NULL;
			else
				lu = RT_LIST_FIRST( loopuse , &s->lu_hd );
			if( lu )
			{
				NMG_CK_LOOPUSE( lu );
				if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) == NMG_EDGEUSE_MAGIC )
				{
					eu = RT_LIST_FIRST( edgeuse , &lu->down_hd );
					NMG_CK_EDGEUSE( eu );
					NMG_CK_VERTEXUSE( eu->vu_p );
					v = eu->vu_p->v_p;
				}
				else
				{
					vu = RT_LIST_FIRST( vertexuse , &lu->down_hd );
					NMG_CK_VERTEXUSE( vu );
					v = vu->v_p;
				}
				NMG_CK_VERTEX( v );
				if( !v->vg_p )
					break;
				VMOVE( mpt , v->vg_p->coord );
				*strp = "V";
				break;
			}
			if( RT_LIST_IS_EMPTY( &s->eu_hd ) )
				eu = (struct edgeuse *)NULL;
			else
				eu = RT_LIST_FIRST( edgeuse , &s->eu_hd );
			if( eu )
			{
				NMG_CK_EDGEUSE( eu );
				NMG_CK_VERTEXUSE( eu->vu_p );
				v = eu->vu_p->v_p;
				NMG_CK_VERTEX( v );
				if( !v->vg_p )
					break;
				VMOVE( mpt , v->vg_p->coord );
				*strp = "V";
				break;
			}
			vu = s->vu_p;
			if( vu )
			{
				NMG_CK_VERTEXUSE( vu );
				v = vu->v_p;
				NMG_CK_VERTEX( v );
				if( !v->vg_p )
					break;
				VMOVE( mpt , v->vg_p->coord );
				*strp = "V";
				break;
			}
		}
	default:
		rt_log( "get_solid_keypoint: unrecognized solid type (setting keypoint to origin)\n" );
		VSETALL( mpt, 0 );
		*strp = "(origin)";
		break;
	}
	MAT4X3PNT( pt, mat, mpt );
}

/* 	CALC_PLANES()
 *		calculate the plane (face) equations for an arb
 *		in solidrec pointed at by sp
 * XXX replaced by rt_arb_calc_planes()
 */
void
calc_planes( sp, type )
struct solidrec *sp;
int type;
{
	struct solidrec temprec;
	register int i, p1, p2, p3;

	/* find the plane equations */
	/* point notation - use temprec record */
	VMOVE( &temprec.s_values[0], &sp->s_values[0] );
	for(i=3; i<=21; i+=3) {
		VADD2( &temprec.s_values[i], &sp->s_values[i], &sp->s_values[0] );
	}
	type -= 4;	/* ARB4 at location 0, ARB5 at 1, etc */
	for(i=0; i<6; i++) {
		if(arb_faces[type][i*4] == -1)
			break;	/* faces are done */
		p1 = arb_faces[type][i*4];
		p2 = arb_faces[type][i*4+1];
		p3 = arb_faces[type][i*4+2];
		if(planeqn(i, p1, p2, p3, &temprec)) {
			rt_log("No eqn for face %d%d%d%d\n",
				p1+1,p2+1,p3+1,arb_faces[type][i*4+3]+1);
			return;
		}
	}
}

/*
 *			I N I T _ S E D I T
 *
 *  First time in for this solid, set things up.
 *  If all goes well, change state to ST_S_EDIT.
 *  Solid editing is completed only via sedit_accept() / sedit_reject().
 */
void
init_sedit()
{
	register int		type;
	int			id;

	/*
	 * Check for a processed region or other illegal solid.
	 */
	if( illump->s_Eflag )  {
		rt_log(
"Unable to Solid_Edit a processed region;  select a primitive instead\n");
		return;
	}

	/* Read solid description.  Save copy of original data */
	RT_INIT_EXTERNAL(&es_ext);
	RT_INIT_DB_INTERNAL(&es_int);
	if( db_get_external( &es_ext, illump->s_path[illump->s_last], dbip ) < 0 )
		READ_ERR_return;

	id = rt_id_solid( &es_ext );
	if( rt_functab[id].ft_import( &es_int, &es_ext, rt_identity ) < 0 )  {
		rt_log("init_sedit(%s):  solid import failure\n",
			illump->s_path[illump->s_last]->d_namep );
	    	if( es_int.idb_ptr )  rt_functab[id].ft_ifree( &es_int );
		db_free_external( &es_ext );
		return;				/* FAIL */
	}
	RT_CK_DB_INTERNAL( &es_int );

	es_menu = 0;
	if( id == ID_ARB8 )
	{
		struct rt_arb_internal *arb;

		arb = (struct rt_arb_internal *)es_int.idb_ptr;
		RT_ARB_CK_MAGIC( arb );

		type = rt_arb_std_type( &es_int , &mged_tol );
		es_type = type;

		if( rt_arb_calc_planes( es_peqn , arb , es_type , &mged_tol ) )
		{
			rt_log( "Cannot calculate plane equations for ARB8\n" );
			db_free_external( &es_ext );
			rt_functab[id].ft_ifree( &es_int );
			return;
		}
	}
	else if( id == ID_BSPLINE )
	{
		register struct rt_nurb_internal *sip =
			(struct rt_nurb_internal *) es_int.idb_ptr;
		register struct snurb	*surf;
		RT_NURB_CK_MAGIC(sip);
		spl_surfno = sip->nsrf/2;
		surf = sip->srfs[spl_surfno];
		NMG_CK_SNURB(surf);
		spl_ui = surf->s_size[1]/2;
		spl_vi = surf->s_size[0]/2;
	}

	/* Save aggregate path matrix */
	pathHmat( illump, es_mat, illump->s_last-1 );

	/* get the inverse matrix */
	mat_inv( es_invmat, es_mat );

	/* Establish initial keypoint */
	es_keytag = "";
	get_solid_keypoint( es_keypoint, &es_keytag, &es_int, es_mat );

	es_eu = (struct edgeuse *)NULL;	/* Reset es_eu */
	lu_copy = (struct loopuse *)NULL;

	sedit_menu();		/* put up menu header */

	/* Finally, enter solid edit state */
	dmp->dmr_light( LIGHT_ON, BE_ACCEPT );
	dmp->dmr_light( LIGHT_ON, BE_REJECT );
	dmp->dmr_light( LIGHT_OFF, BE_S_ILLUMINATE );

	(void)chg_state( ST_S_PICK, ST_S_EDIT, "Keyboard illuminate");
	chg_l2menu(ST_S_EDIT);
	es_edflag = IDLE;
	sedraw = 1;

	button( BE_S_EDIT );	/* Drop into edit menu right away */
}

/*
 *			R E P L O T _ E D I T I N G _ S O L I D
 *
 *  All solid edit routines call this subroutine after
 *  making a change to es_int or es_mat.
 */
void
replot_editing_solid()
{
	struct rt_db_internal	*ip;

	(void)illump->s_path[illump->s_last];

	ip = &es_int;
	RT_CK_DB_INTERNAL( ip );

	(void)replot_modified_solid( illump, ip, es_mat );

}

/*
 *			T R A N S F O R M _ E D I T I N G _ S O L I D
 *
 */
void
transform_editing_solid(os, mat, is, free)
struct rt_db_internal	*os;		/* output solid */
CONST mat_t		mat;
struct rt_db_internal	*is;		/* input solid */
int			free;
{
	RT_CK_DB_INTERNAL( is );
	if( rt_functab[is->idb_type].ft_xform( os, mat, is, free ) < 0 )
		rt_bomb("transform_editing_solid");
#if 0
/*
 * this is the old way.  rt_db_xform_internal was transfered and
 * modified into rt_generic_xform() in librt/table.c
 * rt_generic_xform is normally called via rt_functab[id].ft_xform()
 */
	struct directory	*dp;

	dp = illump->s_path[illump->s_last];
	if( rt_db_xform_internal( os, mat, is, free, dp->d_namep ) < 0 )
		rt_bomb("transform_editing_solid");		/* FAIL */
#endif
}

/*
 *			S E D I T _ M E N U
 *
 *
 *  Put up menu header
 */
void
sedit_menu()  {

	menuflag = 0;		/* No menu item selected yet */

	mmenu_set( MENU_L1, MENU_NULL );
	chg_l2menu(ST_S_EDIT);
                                                                      
	switch( es_int.idb_type ) {

	case ID_ARB8:
		mmenu_set( MENU_L1, cntrl_menu );
		break;
	case ID_TGC:
		mmenu_set( MENU_L1, tgc_menu );
		break;
	case ID_TOR:
		mmenu_set( MENU_L1, tor_menu );
		break;
	case ID_ELL:
		mmenu_set( MENU_L1, ell_menu );
		break;
	case ID_ARS:
		mmenu_set( MENU_L1, ars_menu );
		break;
	case ID_BSPLINE:
		mmenu_set( MENU_L1, spline_menu );
		break;
	case ID_RPC:
		mmenu_set( MENU_L1, rpc_menu );
		break;
	case ID_RHC:
		mmenu_set( MENU_L1, rhc_menu );
		break;
	case ID_EPA:
		mmenu_set( MENU_L1, epa_menu );
		break;
	case ID_EHY:
		mmenu_set( MENU_L1, ehy_menu );
		break;
	case ID_ETO:
		mmenu_set( MENU_L1, eto_menu );
		break;
	case ID_NMG:
		mmenu_set( MENU_L1, nmg_menu );
		break;
	}
	es_edflag = IDLE;	/* Drop out of previous edit mode */
	es_menu = 0;
}

/* 			C A L C _ P N T S (  )
 * XXX replaced by rt_arb_calc_points() in facedef.c
 *
 * Takes the array es_peqn[] and intersects the planes to find the vertices
 * of a GENARB8.  The vertices are stored in the solid record 'old_srec' which
 * is of type 'type'.  If intersect fails, the points (in vector notation) of
 * 'old_srec' are used to clean up the array es_peqn[] for anyone else. The
 * vertices are put in 'old_srec' in vector notation.  This is an analog to
 * calc_planes().
 */
void
calc_pnts( old_srec, type )
struct solidrec *old_srec;
int type;
{
	struct solidrec temp_srec;
	short int i;

	/* find new points for entire solid */
	for(i=0; i<8; i++){
		/* use temp_srec until we know intersect doesn't fail */
		if( intersect(type,i*3,i,&temp_srec) ){
			rt_log("Intersection of planes fails\n");
			/* clean up array es_peqn for anyone else */
			calc_planes( old_srec, type );
			return;				/* failure */
		}
	}

	/* back to vector notation */
	VMOVE( &old_srec->s_values[0], &temp_srec.s_values[0] );
	for(i=3; i<=21; i+=3){
		VSUB2(	&old_srec->s_values[i],
			&temp_srec.s_values[i],
			&temp_srec.s_values[0]  );
	}
	return;						/* success */
}

/*
 * 			S E D I T
 * 
 * A great deal of magic takes place here, to accomplish solid editing.
 *
 *  Called from mged main loop after any event handlers:
 *		if( sedraw > 0 )  sedit();
 *  to process any residual events that the event handlers were too
 *  lazy to handle themselves.
 *
 *  A lot of processing is deferred to here, so that the "p" command
 *  can operate on an equal footing to mouse events.
 */
void
sedit()
{
	struct rt_arb_internal *arb;
	fastf_t	*eqp;
	static vect_t work;
	register int i;
	static int pnt5;		/* ECMD_ARB_SETUP_ROTFACE, special arb7 case */
	static int j;
	static float la, lb, lc, ld;	/* TGC: length of vectors */

	sedraw = 0;
#ifdef XMGED
	update_views = 1;
#endif

	switch( es_edflag ) {

	case IDLE:
		/* do nothing */
		break;

	case ECMD_ARB_MAIN_MENU:
		/* put up control (main) menu for GENARB8s */
		menuflag = 0;
		es_edflag = IDLE;
		mmenu_set( MENU_L1, cntrl_menu );
		break;

	case ECMD_ARB_SPECIFIC_MENU:
		/* put up specific arb edit menus */
		menuflag = 0;
		es_edflag = IDLE;
		switch( es_menu ){
			case MENU_ARB_MV_EDGE:  
				mmenu_set( MENU_L1, which_menu[es_type-4] );
				break;
			case MENU_ARB_MV_FACE:
				mmenu_set( MENU_L1, which_menu[es_type+1] );
				break;
			case MENU_ARB_ROT_FACE:
				mmenu_set( MENU_L1, which_menu[es_type+6] );
				break;
			default:
				rt_log("Bad menu item.\n");
				return;
		}
		break;

	case ECMD_ARB_MOVE_FACE:
		/* move face through definite point */
		if(inpara) {
			arb = (struct rt_arb_internal *)es_int.idb_ptr;
			RT_ARB_CK_MAGIC( arb );

			/* apply es_invmat to convert to real model space */
			MAT4X3PNT(work,es_invmat,es_para);
			/* change D of planar equation */
			es_peqn[es_menu][3]=VDOT(&es_peqn[es_menu][0], work);
			/* find new vertices, put in record in vector notation */
			(void)rt_arb_calc_points( arb , es_type , es_peqn , &mged_tol );
		}
		break;

	case ECMD_ARB_SETUP_ROTFACE:
		arb = (struct rt_arb_internal *)es_int.idb_ptr;
		RT_ARB_CK_MAGIC( arb );

		/* check if point 5 is in the face */
		pnt5 = 0;
		for(i=0; i<4; i++)  {
			if( arb_vertices[es_type-4][es_menu*4+i]==5 )
				pnt5=1;
		}
		
		/* special case for arb7 */
		if( es_type == ARB7  && pnt5 ){
				rt_log("\nFixed vertex is point 5.\n");
				fixv = 5;
		}
		else{
			/* find fixed vertex for ECMD_ARB_ROTATE_FACE */
			fixv=0;
			do  {
				int	type,loc,valid;
				char	line[128];
				
				type = es_type - 4;
				rt_log("\nEnter fixed vertex number( ");
				loc = es_menu*4;
				for(i=0; i<4; i++){
					if( arb_vertices[type][loc+i] )
						rt_log("%d ",
						    arb_vertices[type][loc+i]);
				}
				rt_log(") [%d]: ",arb_vertices[type][loc]);
#ifdef XMGED
				(void)mged_gets( line ); /* Null terminated */
#else
				(void)fgets( line, sizeof(line), stdin );
				line[strlen(line)-1] = '\0';		/* remove newline */
#endif
				if( feof(stdin) )  quit();
				if( line[0] == '\0' )
					fixv = arb_vertices[type][loc]; 	/* default */
				else
					fixv = atoi( line );
				
				/* check whether nimble fingers entered valid vertex */
				valid = 0;
				for(j=0; j<4; j++)  {
					if( fixv==arb_vertices[type][loc+j] )
						valid=1;
				}
				if( !valid )
					fixv=0;
			} while( fixv <= 0 || fixv > es_type );
		}
		
		pr_prompt();
		fixv--;
		es_edflag = ECMD_ARB_ROTATE_FACE;
		mat_idn( acc_rot_sol );
		dmaflag = 1;	/* draw arrow, etc */
#ifdef XMGED
		set_e_axis_pos();
#endif
		break;

	case ECMD_ARB_ROTATE_FACE:
		/* rotate a GENARB8 defining plane through a fixed vertex */

		arb = (struct rt_arb_internal *)es_int.idb_ptr;
		RT_ARB_CK_MAGIC( arb );

		if(inpara) {
			static mat_t invsolr;
			static vect_t tempvec;
			static float rota, fb;

			/*
			 * Keyboard parameters in degrees.
			 * First, cancel any existing rotations,
			 * then perform new rotation
			 */
			mat_inv( invsolr, acc_rot_sol );
			eqp = &es_peqn[es_menu][0];	/* es_menu==plane of interest */
			VMOVE( work, eqp );
			MAT4X3VEC( eqp, invsolr, work );

			if( inpara == 3 ){
				/* 3 params:  absolute X,Y,Z rotations */
				/* Build completely new rotation change */
				mat_idn( modelchanges );
				buildHrot( modelchanges,
					es_para[0] * degtorad,
					es_para[1] * degtorad,
					es_para[2] * degtorad );
				mat_copy(acc_rot_sol, modelchanges);

				/* Apply new rotation to face */
				eqp = &es_peqn[es_menu][0];
				VMOVE( work, eqp );
				MAT4X3VEC( eqp, modelchanges, work );
			}
			else if( inpara == 2 ){
				/* 2 parameters:  rot,fb were given */
				rota= es_para[0] * degtorad;
				fb  = es_para[1] * degtorad;
	
				/* calculate normal vector (length=1) from rot,fb */
				es_peqn[es_menu][0] = cos(fb) * cos(rota);
				es_peqn[es_menu][1] = cos(fb) * sin(rota);
				es_peqn[es_menu][2] = sin(fb);
			}
			else{
				rt_log("Must be < rot fb | xdeg ydeg zdeg >\n");
				return;
			}

			/* point notation of fixed vertex */
			VMOVE( tempvec, arb->pt[fixv] );

			/* set D of planar equation to anchor at fixed vertex */
			/* es_menu == plane of interest */
			es_peqn[es_menu][3]=VDOT(eqp,tempvec);	

			/*  Clear out solid rotation */
			mat_idn( modelchanges );

		}  else  {
			/* Apply incremental changes */
			static vect_t tempvec;

			eqp = &es_peqn[es_menu][0];
			VMOVE( work, eqp );
			MAT4X3VEC( eqp, incr_change, work );

			/* point notation of fixed vertex */
			VMOVE( tempvec, arb->pt[fixv] );

			/* set D of planar equation to anchor at fixed vertex */
			/* es_menu == plane of interest */
			es_peqn[es_menu][3]=VDOT(eqp,tempvec);	
		}

		(void)rt_arb_calc_points( arb , es_type , es_peqn , &mged_tol );
		mat_idn( incr_change );

		/* no need to calc_planes again */
		replot_editing_solid();

		inpara = 0;
		return;

	case SSCALE:
		/* scale the solid uniformly about it's vertex point */
		{
			mat_t	scalemat;

			es_eu = (struct edgeuse *)NULL;	/* Reset es_eu */
			if(inpara) {
				/* accumulate the scale factor */
				es_scale = es_para[0] / acc_sc_sol;
				acc_sc_sol = es_para[0];
			}

			mat_scale_about_pt( scalemat, es_keypoint, es_scale );
			transform_editing_solid(&es_int, scalemat, &es_int, 1);

			/* reset solid scale factor */
			es_scale = 1.0;
		}
		break;

	case STRANS:
		/* translate solid  */
		{
			vect_t	delta;
			mat_t	xlatemat;

			es_eu = (struct edgeuse *)NULL;	/* Reset es_eu */
			if(inpara) {
				/* Keyboard parameter.
				 * Apply inverse of es_mat to these
				 * model coordinates first, because sedit_mouse()
				 * has already applied es_mat to them.
				 * XXX this does not make sense.
				 */
				MAT4X3PNT( work, es_invmat, es_para );

				/* Need vector from current vertex/keypoint
				 * to desired new location.
				 */
				VSUB2( delta, es_para, es_keypoint );
				mat_idn( xlatemat );
				MAT_DELTAS_VEC( xlatemat, delta );
				transform_editing_solid(&es_int, xlatemat, &es_int, 1);
			}
		}
		break;
	case ECMD_VTRANS:
		/* translate a vertex */
		es_eu = (struct edgeuse *)NULL;	/* Reset es_eu */
		if( es_mvalid )  {
			/* Mouse parameter:  new position in model space */
			VMOVE( es_para, es_mparam );
			inpara = 1;
		}
		if(inpara) {
			/* Keyboard parameter:  new position in model space.
			 * XXX for now, splines only here */
			register struct rt_nurb_internal *sip =
				(struct rt_nurb_internal *) es_int.idb_ptr;
			register struct snurb	*surf;
			register fastf_t	*fp;

			RT_NURB_CK_MAGIC(sip);
			surf = sip->srfs[spl_surfno];
			NMG_CK_SNURB(surf);
			fp = &RT_NURB_GET_CONTROL_POINT( surf, spl_ui, spl_vi );
			VMOVE( fp, es_para );
		}
		break;

	case ECMD_TGC_MV_H:
		/*
		 * Move end of H of tgc, keeping plates perpendicular
		 * to H vector.
		 */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;

			RT_TGC_CK_MAGIC(tgc);
			if( inpara ) {
				/* apply es_invmat to convert to real model coordinates */
				MAT4X3PNT( work, es_invmat, es_para );
				VSUB2(tgc->h, work, tgc->v);
			}

			/* check for zero H vector */
			if( MAGNITUDE( tgc->h ) <= SQRT_SMALL_FASTF ) {
				rt_log("Zero H vector not allowed, resetting to +Z\n");
				VSET(tgc->h, 0, 0, 1 );
				break;
			}

			/* have new height vector --  redefine rest of tgc */
			la = MAGNITUDE( tgc->a );
			lb = MAGNITUDE( tgc->b );
			lc = MAGNITUDE( tgc->c );
			ld = MAGNITUDE( tgc->d );

			/* find 2 perpendicular vectors normal to H for new A,B */
			mat_vec_perp( tgc->b, tgc->h );
			VCROSS(tgc->a, tgc->b, tgc->h);
			VUNITIZE(tgc->a);
			VUNITIZE(tgc->b);

			/* Create new C,D from unit length A,B, with previous len */
			VSCALE(tgc->c, tgc->a, lc);
			VSCALE(tgc->d, tgc->b, ld);

			/* Restore original vector lengths to A,B */
			VSCALE(tgc->a, tgc->a, la);
			VSCALE(tgc->b, tgc->b, lb);
		}
		break;

	case ECMD_TGC_MV_HH:
		/* Move end of H of tgc - leave ends alone */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;

			RT_TGC_CK_MAGIC(tgc);
			if( inpara ) {
				/* apply es_invmat to convert to real model coordinates */
				MAT4X3PNT( work, es_invmat, es_para );
				VSUB2(tgc->h, work, tgc->v);
			}

			/* check for zero H vector */
			if( MAGNITUDE( tgc->h ) <= SQRT_SMALL_FASTF ) {
				rt_log("Zero H vector not allowed, resetting to +Z\n");
				VSET(tgc->h, 0, 0, 1 );
				break;
			}
		}
		break;

	case PSCALE:
		es_eu = (struct edgeuse *)NULL;	/* Reset es_eu */
		pscale();
		break;

	case PTARB:	/* move an ARB point */
	case EARB:   /* edit an ARB edge */
		if( inpara ) { 
			/* apply es_invmat to convert to real model space */
			MAT4X3PNT( work, es_invmat, es_para );
			editarb( work );
		}
		break;

	case SROT:
		/* rot solid about vertex */
		{
			mat_t	mat;

			es_eu = (struct edgeuse *)NULL;	/* Reset es_eu */
			if(inpara) {
				static mat_t invsolr;
				/*
				 * Keyboard parameters:  absolute x,y,z rotations,
				 * in degrees.  First, cancel any existing rotations,
				 * then perform new rotation
				 */
				mat_inv( invsolr, acc_rot_sol );

				/* Build completely new rotation change */
				mat_idn( modelchanges );
				buildHrot( modelchanges,
					es_para[0] * degtorad,
					es_para[1] * degtorad,
					es_para[2] * degtorad );
				/* Borrow incr_change matrix here */
				mat_mul( incr_change, modelchanges, invsolr );
				mat_copy(acc_rot_sol, modelchanges);

				/* Apply new rotation to solid */
				/*  Clear out solid rotation */
				mat_idn( modelchanges );
			}  else  {
				/* Apply incremental changes already in incr_change */
			}
			/* Apply changes to solid */
			/* xlate keypoint to origin, rotate, then put back. */
			mat_xform_about_pt( mat, incr_change, es_keypoint );
			transform_editing_solid(&es_int, mat, &es_int, 1);

			mat_idn( incr_change );
		}
		break;

	case ECMD_TGC_ROT_H:
		/* rotate height vector */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;

			RT_TGC_CK_MAGIC(tgc);
			MAT4X3VEC(work, incr_change, tgc->h);
			VMOVE(tgc->h, work);

			mat_idn( incr_change );
		}
		break;

	case ECMD_TGC_ROT_AB:
		/* rotate surfaces AxB and CxD (tgc) */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;

			RT_TGC_CK_MAGIC(tgc);

			MAT4X3VEC(work, incr_change, tgc->a);
			VMOVE(tgc->a, work);
			MAT4X3VEC(work, incr_change, tgc->b);
			VMOVE(tgc->b, work);
			MAT4X3VEC(work, incr_change, tgc->c);
			VMOVE(tgc->c, work);
			MAT4X3VEC(work, incr_change, tgc->d);
			VMOVE(tgc->d, work);

			mat_idn( incr_change );
		}
		break;

	case ECMD_ETO_ROT_C:
		/* rotate ellipse semi-major axis vector */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;

			RT_ETO_CK_MAGIC(eto);
			MAT4X3VEC(work, incr_change, eto->eto_C);
			VMOVE(eto->eto_C, work);
		}
		mat_idn( incr_change );
		break;

	case ECMD_NMG_EPICK:
		/* XXX Nothing to do here (yet), all done in mouse routine. */
		break;
	case ECMD_NMG_EMOVE:
		{
			point_t new_pt;

			if( !es_eu )
			{
				rt_log( "No edge selected!\n" );
				break;
			}
			NMG_CK_EDGEUSE( es_eu );

			if( es_mvalid )
				VMOVE( new_pt , es_mparam )
			else if( inpara == 3 )
				VMOVE( new_pt , es_para )
			else if( inpara && inpara != 3 )
			{
				rt_log( "x y z coordinates required for edge move\n" );
				break;
			}
			else if( !es_mvalid && !inpara )
				break;

			if( !nmg_find_fu_of_eu( es_eu ) && *es_eu->up.magic_p == NMG_LOOPUSE_MAGIC )
			{
				struct loopuse *lu;
				fastf_t area;
				plane_t pl;

				/* this edge is in a wire loop
				 * keep the loop planar
				 */
				lu = es_eu->up.lu_p;
				NMG_CK_LOOPUSE( lu );
				
				/* get plane equation for loop */
				area = nmg_loop_plane_area( lu , pl );
				if( area > 0.0 )
				{
					vect_t view_z_dir;
					vect_t view_dir;
					fastf_t dist;

					/* Get view direction vector */
					VSET( view_z_dir , 0 , 0 , 1 );
					MAT4X3VEC( view_dir , view2model , view_z_dir );

					/* intersect line through new_pt with plane of loop */
					if( rt_isect_line3_plane( &dist , new_pt , view_dir , pl , &mged_tol ) < 1)
					{
						/* line does not intersect plane, don't do an esplit */
						rt_log( "Edge Move: Cannot place new point in plane of loop\n" );
						break;
					}
					VJOIN1( new_pt , new_pt , dist , view_dir );
				}
			}

			if( nmg_move_edge_thru_pt( es_eu, new_pt, &mged_tol ) < 0 ) {
				VPRINT("Unable to hit", new_pt);
			}
		}
		break;

	case ECMD_NMG_EKILL:
		{
			struct model *m;
			struct edge_g_lseg *eg;

			if( !es_eu )
			{
				rt_log( "No edge selected!\n" );
				break;
			}
			NMG_CK_EDGEUSE( es_eu );

			m = nmg_find_model( &es_eu->l.magic );

			if( *es_eu->up.magic_p == NMG_LOOPUSE_MAGIC )
			{
				struct loopuse *lu;
				struct edgeuse *prev_eu,*next_eu;

				lu = es_eu->up.lu_p;
				NMG_CK_LOOPUSE( lu );

				if( *lu->up.magic_p != NMG_SHELL_MAGIC )
				{
					/* Currently can only kill wire edges or edges in wire loops */
					rt_log( "Currently, we can only kill wire edges or edges in wire loops\n" );
					es_edflag = IDLE;
					break;
				}

				prev_eu = RT_LIST_PPREV_CIRC( edgeuse , &es_eu->l );
				NMG_CK_EDGEUSE( prev_eu );

				if( prev_eu == es_eu )
				{
					/* only one edge left in the loop
					 * make it an edge to/from same vertex
					 */
					if( es_eu->vu_p->v_p == es_eu->eumate_p->vu_p->v_p )
					{
						/* refuse to delete last edge that runs
						 * to/from same vertex
						 */
						rt_log( "Cannot delete last edge running to/from same vertex\n" );
						break;
					}
					NMG_CK_EDGEUSE( es_eu->eumate_p );
					nmg_movevu( es_eu->eumate_p->vu_p , es_eu->vu_p->v_p );
					break;
				}

				next_eu = RT_LIST_PNEXT_CIRC( edgeuse , &es_eu->l );
				NMG_CK_EDGEUSE( next_eu );

				nmg_movevu( next_eu->vu_p , es_eu->vu_p->v_p );
				if( nmg_keu( es_eu ) )
				{
					/* Should never happen!!! */
					rt_bomb( "sedit(): killed edge and emptied loop!!\n" );
				}
				es_eu = prev_eu;
				nmg_rebound( m , &mged_tol );

				/* fix edge geometry for modified edge (next_eu ) */
				eg = next_eu->g.lseg_p;
				NMG_CK_EDGE_G_LSEG( eg );
				VMOVE( eg->e_pt , next_eu->vu_p->v_p->vg_p->coord );
				VSUB2( eg->e_dir, next_eu->eumate_p->vu_p->v_p->vg_p->coord, next_eu->vu_p->v_p->vg_p->coord );

				break;
			}
			else if( *es_eu->up.magic_p == NMG_SHELL_MAGIC )
			{
				/* wire edge, just kill it */
				(void)nmg_keu( es_eu );
				es_eu = (struct edgeuse *)NULL;
				nmg_rebound( m , &mged_tol );
			}
		}

	case ECMD_NMG_ESPLIT:
		{
			struct vertex *v=(struct vertex *)NULL;
			struct edge_g_lseg *eg;
			struct model *m;
			point_t new_pt;
			fastf_t area;
			plane_t pl;

			if( !es_eu )
			{
				rt_log( "No edge selected!\n" );
				break;
			}
			NMG_CK_EDGEUSE( es_eu );
			m = nmg_find_model( &es_eu->l.magic );
			NMG_CK_MODEL( m );
			if( es_mvalid )
				VMOVE( new_pt , es_mparam )
			else if( inpara == 3 )
				VMOVE( new_pt , es_para )
			else if( inpara && inpara != 3 )
			{
				rt_log( "x y z coordinates required for edge split\n" );
				break;
			}
			else if( !es_mvalid && !inpara )
				break;

			if( *es_eu->up.magic_p == NMG_LOOPUSE_MAGIC )
			{
				struct loopuse *lu;

				lu = es_eu->up.lu_p;
				NMG_CK_LOOPUSE( lu );

				/* Currently, can only split wire edges or edges in wire loops */
				if( *lu->up.magic_p != NMG_SHELL_MAGIC )
				{
					rt_log( "Currently, we can only split wire edges or edges in wire loops\n" );
					es_edflag = IDLE;
					break;
				}

				/* get plane equation for loop */
				area = nmg_loop_plane_area( lu , pl );
				if( area > 0.0 )
				{
					vect_t view_z_dir;
					vect_t view_dir;
					fastf_t dist;

					/* Get view direction vector */
					VSET( view_z_dir , 0 , 0 , 1 );
					MAT4X3VEC( view_dir , view2model , view_z_dir );

					/* intersect line through new_pt with plane of loop */
					if( rt_isect_line3_plane( &dist , new_pt , view_dir , pl , &mged_tol ) < 1)
					{
						/* line does not intersect plane, don't do an esplit */
						rt_log( "Edge Split: Cannot place new point in plane of loop\n" );
						break;
					}
					VJOIN1( new_pt , new_pt , dist , view_dir );
				}
			}
			es_eu = nmg_esplit( v , es_eu , 0 );
			nmg_vertex_gv( es_eu->vu_p->v_p , new_pt );
			nmg_rebound( m , &mged_tol );
			eg = es_eu->g.lseg_p;
			NMG_CK_EDGE_G_LSEG( eg );
			VMOVE( eg->e_pt , new_pt );
			VSUB2( eg->e_dir , es_eu->eumate_p->vu_p->v_p->vg_p->coord , new_pt );
		}
		break;
	case ECMD_NMG_LEXTRU:
		{
			fastf_t dist;
			point_t to_pt;
			vect_t extrude_vec;
			struct loopuse *new_lu;
			struct faceuse *fu;
			struct model *m;
			plane_t new_lu_pl;
			fastf_t area;

			if( es_mvalid )
				VMOVE( to_pt , es_mparam )
			else if( inpara == 3 )
				VMOVE( to_pt , es_para )
			else if( inpara && inpara != 3 )
			{
				rt_log( "x y z coordinates required for loop extrusion\n" );
				break;
			}
			else if( !es_mvalid && !inpara )
				break;

			VSUB2( extrude_vec , to_pt , lu_keypoint );

			if( rt_isect_line3_plane( &dist , to_pt , extrude_vec , lu_pl , &mged_tol ) < 1 )
			{
				rt_log( "Cannot extrude parallel to plane of loop\n" );
				return;
			}

			if( RT_LIST_NON_EMPTY( &es_s->fu_hd ) )
			{
				struct nmgregion *r;

				r = es_s->r_p;
				(void) nmg_ks( es_s );
				es_s = nmg_msv( r );
			}

			new_lu = nmg_dup_loop( lu_copy , &es_s->l.magic , (long **)0 );
			area = nmg_loop_plane_area( new_lu , new_lu_pl );
			if( area < 0.0 )
			{
				rt_log( "loop to be extruded as no area!!!\n" );
				return;
			}

			if( VDOT( extrude_vec , new_lu_pl ) > 0.0 )
			{
				plane_t tmp_pl;

				fu = nmg_mf( new_lu->lumate_p );
				NMG_CK_FACEUSE( fu );
				HREVERSE( tmp_pl , new_lu_pl );
				nmg_face_g( fu , tmp_pl );
			}
			else
			{
				fu = nmg_mf( new_lu );
				NMG_CK_FACEUSE( fu );
				nmg_face_g( fu , new_lu_pl );
			}

			(void)nmg_extrude_face( fu , extrude_vec , &mged_tol );

			nmg_fix_normals( fu->s_p , &mged_tol );

			m = nmg_find_model( &fu->l.magic );
			nmg_rebound( m , &mged_tol );
			(void)nmg_ck_geometry( m , &mged_tol );

			es_eu = (struct edgeuse *)NULL;

			replot_editing_solid();
			dmaflag = 1;
		}
		break;

	default:
		rt_log("sedit():  unknown edflag = %d.\n", es_edflag );
	}

	/* must re-calculate the face plane equations for arbs */
	if( es_int.idb_type == ID_ARB8 )
	{
		arb = (struct rt_arb_internal *)es_int.idb_ptr;
		RT_ARB_CK_MAGIC( arb );

		(void)rt_arb_calc_planes( es_peqn , arb , es_type , &mged_tol );
	}

	/* If the keypoint changed location, find about it here */
	if (! es_keyfixed)
		get_solid_keypoint( es_keypoint, &es_keytag, &es_int, es_mat );

	replot_editing_solid();

	inpara = 0;
	es_mvalid = 0;
	return;
}

/*
 *			S E D I T _ M O U S E
 *
 *  Mouse (pen) press in graphics area while doing Solid Edit.
 *  mousevec [X] and [Y] are in the range -1.0...+1.0, corresponding
 *  to viewspace.
 *
 *  In order to allow the "p" command to do the same things that
 *  a mouse event can, the preferred strategy is to store the value
 *  corresponding to what the "p" command would give in es_mparam,
 *  set es_mvalid=1, set sedraw=1, and return, allowing sedit()
 *  to actually do the work.
 */
void
sedit_mouse( mousevec )
CONST vect_t	mousevec;
{
	vect_t	pos_view;	 	/* Unrotated view space pos */
	vect_t	pos_model;		/* Rotated screen space pos */
	vect_t	tr_temp;		/* temp translation vector */
	vect_t	temp;


	if( es_edflag <= 0 )  return;
	switch( es_edflag )  {

	case SSCALE:
	case PSCALE:
		/* use mouse to get a scale factor */
		es_scale = 1.0 + 0.25 * ((fastf_t)
			(mousevec[Y] > 0 ? mousevec[Y] : -mousevec[Y]));
		if ( mousevec[Y] <= 0 )
			es_scale = 1.0 / es_scale;

		/* accumulate scale factor */
		acc_sc_sol *= es_scale;

		sedraw = 1;
		return;
	case STRANS:
		/* 
		 * Use mouse to change solid's location.
		 * Project solid's keypoint into view space,
		 * replace X,Y (but NOT Z) components, and
		 * project result back to model space.
		 * Then move keypoint there.
		 */
		{
			point_t	pt;
			vect_t	delta;
			mat_t	xlatemat;

			MAT4X3PNT( temp, es_mat, es_keypoint );
			MAT4X3PNT( pos_view, model2view, temp );
			pos_view[X] = mousevec[X];
			pos_view[Y] = mousevec[Y];
			MAT4X3PNT( temp, view2model, pos_view );
			MAT4X3PNT( pt, es_invmat, temp );

			/* Need vector from current vertex/keypoint
			 * to desired new location.
			 */
			VSUB2( delta, es_keypoint, pt );
			mat_idn( xlatemat );
			MAT_DELTAS_VEC_NEG( xlatemat, delta );
			transform_editing_solid(&es_int, xlatemat, &es_int, 1);
		}
		sedraw = 1;
		return;
	case ECMD_VTRANS:
		/* 
		 * Use mouse to change a vertex location.
		 * Project vertex (in solid keypoint) into view space,
		 * replace X,Y (but NOT Z) components, and
		 * project result back to model space.
		 * Leave desired location in es_mparam.
		 */
		MAT4X3PNT( temp, es_mat, es_keypoint );
		MAT4X3PNT( pos_view, model2view, temp );
		pos_view[X] = mousevec[X];
		pos_view[Y] = mousevec[Y];
		MAT4X3PNT( temp, view2model, pos_view );
		MAT4X3PNT( es_mparam, es_invmat, temp );
		es_mvalid = 1;	/* es_mparam is valid */
		/* Leave the rest to code in sedit() */
		sedraw = 1;
		return;
	case ECMD_TGC_MV_H:
	case ECMD_TGC_MV_HH:
		/* Use mouse to change location of point V+H */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			VADD2( temp, tgc->v, tgc->h );
			MAT4X3PNT(pos_model, es_mat, temp);
			MAT4X3PNT( pos_view, model2view, pos_model );
			pos_view[X] = mousevec[X];
			pos_view[Y] = mousevec[Y];
			/* Do NOT change pos_view[Z] ! */
			MAT4X3PNT( temp, view2model, pos_view );
			MAT4X3PNT( tr_temp, es_invmat, temp );
			VSUB2( tgc->h, tr_temp, tgc->v );
		}
		sedraw = 1;
		return;
	case PTARB:
		/* move an arb point to indicated point */
		/* point is located at es_values[es_menu*3] */
		{
			struct rt_arb_internal *arb=
				(struct rt_arb_internal *)es_int.idb_ptr;
			RT_ARB_CK_MAGIC( arb );

			VMOVE( temp , arb->pt[es_menu] );
		}
		MAT4X3PNT(pos_model, es_mat, temp);
		MAT4X3PNT(pos_view, model2view, pos_model);
		pos_view[X] = mousevec[X];
		pos_view[Y] = mousevec[Y];
		MAT4X3PNT(temp, view2model, pos_view);
		MAT4X3PNT(pos_model, es_invmat, temp);
		editarb( pos_model );
		sedraw = 1;
		return;
	case EARB:
		/* move arb edge, through indicated point */
		MAT4X3PNT( temp, view2model, mousevec );
		/* apply inverse of es_mat */
		MAT4X3PNT( pos_model, es_invmat, temp );
		editarb( pos_model );
		sedraw = 1;
		return;
	case ECMD_ARB_MOVE_FACE:
		/* move arb face, through  indicated  point */
		MAT4X3PNT( temp, view2model, mousevec );
		/* apply inverse of es_mat */
		MAT4X3PNT( pos_model, es_invmat, temp );
		/* change D of planar equation */
		es_peqn[es_menu][3]=VDOT(&es_peqn[es_menu][0], pos_model);
		/* calculate new vertices, put in record as vectors */
		{
			struct rt_arb_internal *arb=
				(struct rt_arb_internal *)es_int.idb_ptr;

			RT_ARB_CK_MAGIC( arb );
			(void)rt_arb_calc_points( arb , es_type , es_peqn , &mged_tol );
		}
		sedraw = 1;
		return;

	case ECMD_NMG_EPICK:
		/* XXX Should just leave desired location in es_mparam for sedit() */
		{
			struct model	*m = 
				(struct model *)es_int.idb_ptr;
			struct edge	*e;
			struct rt_tol	tmp_tol;
			NMG_CK_MODEL(m);

			/* Picking an edge should not depend on tolerances! */
			tmp_tol.magic = RT_TOL_MAGIC;
			tmp_tol.dist = 0.0;
			tmp_tol.dist_sq = tmp_tol.dist * tmp_tol.dist;
			tmp_tol.perp = 0.0;
			tmp_tol.para = 1 - tmp_tol.perp;

			if( (e = nmg_find_e_nearest_pt2( &m->magic, mousevec,
			    model2view, &tmp_tol )) == (struct edge *)NULL )  {
				rt_log("ECMD_NMG_EPICK: unable to find an edge\n");
				return;
			}
			es_eu = e->eu_p;
			NMG_CK_EDGEUSE(es_eu);
			rt_log("edgeuse selected=x%x\n", es_eu);
			sedraw = 1;
		}
		break;

	case ECMD_NMG_LEXTRU:
	case ECMD_NMG_EMOVE:
	case ECMD_NMG_ESPLIT:
		MAT4X3PNT( temp, view2model, mousevec );
		/* apply inverse of es_mat */
		MAT4X3PNT( es_mparam, es_invmat, temp );
		es_mvalid = 1;
		sedraw = 1;
		return;

	default:
		rt_log("mouse press undefined in this solid edit mode\n");
		break;
	}

	/* XXX I would prefer to see an explicit call to the guts of sedit()
	 * XXX here, rather than littering the place with global variables
	 * XXX for later interpretation.
	 */
}

/*
 *  Object Edit
 */
void
objedit_mouse( mousevec )
CONST vect_t	mousevec;
{
	fastf_t			scale;
	vect_t	pos_view;	 	/* Unrotated view space pos */
	vect_t	pos_model;	/* Rotated screen space pos */
	vect_t	tr_temp;		/* temp translation vector */
	vect_t	temp;

#ifdef XMGED
	update_views = 1;
#endif
	mat_idn( incr_change );
	scale = 1;
	if( movedir & SARROW )  {
		/* scaling option is in effect */
		scale = 1.0 + (fastf_t)(mousevec[Y]>0 ?
			mousevec[Y] : -mousevec[Y]);
		if ( mousevec[Y] <= 0 )
			scale = 1.0 / scale;

		/* switch depending on scaling option selected */
		switch( edobj ) {

			case BE_O_SCALE:
				/* global scaling */
				incr_change[15] = 1.0 / scale;
			break;

			case BE_O_XSCALE:
				/* local scaling ... X-axis */
				incr_change[0] = scale;
				/* accumulate the scale factor */
				acc_sc[0] *= scale;
			break;

			case BE_O_YSCALE:
				/* local scaling ... Y-axis */
				incr_change[5] = scale;
				/* accumulate the scale factor */
				acc_sc[1] *= scale;
			break;

			case BE_O_ZSCALE:
				/* local scaling ... Z-axis */
				incr_change[10] = scale;
				/* accumulate the scale factor */
				acc_sc[2] *= scale;
			break;
		}

		/* Have scaling take place with respect to keypoint,
		 * NOT the view center.
		 */
		MAT4X3PNT(temp, es_mat, es_keypoint);
		MAT4X3PNT(pos_model, modelchanges, temp);
		wrt_point(modelchanges, incr_change, modelchanges, pos_model);
	}  else if( movedir & (RARROW|UARROW) )  {
		mat_t oldchanges;	/* temporary matrix */

		/* Vector from object keypoint to cursor */
		MAT4X3PNT( temp, es_mat, es_keypoint );
		MAT4X3PNT( pos_view, model2objview, temp );
		if( movedir & RARROW )
			pos_view[X] = mousevec[X];
		if( movedir & UARROW )
			pos_view[Y] = mousevec[Y];

		MAT4X3PNT( pos_model, view2model, pos_view );/* NOT objview */
		MAT4X3PNT( tr_temp, modelchanges, temp );
		VSUB2( tr_temp, pos_model, tr_temp );
		MAT_DELTAS(incr_change,
			tr_temp[X], tr_temp[Y], tr_temp[Z]);
		mat_copy( oldchanges, modelchanges );
		mat_mul( modelchanges, incr_change, oldchanges );
	}  else  {
		rt_log("No object edit mode selected;  mouse press ignored\n");
		return;
	}
	mat_idn( incr_change );
	new_mats();
}

/*
 *			V L S _ S O L I D
 */
void
vls_solid( vp, ip, mat )
register struct rt_vls		*vp;
CONST struct rt_db_internal	*ip;
CONST mat_t			mat;
{
	struct rt_db_internal	intern;
	int			id;

	RT_VLS_CHECK(vp);
	RT_CK_DB_INTERNAL(ip);

	id = ip->idb_type;
	transform_editing_solid( &intern, mat, (struct rt_db_internal *)ip, 0 );

	if( rt_functab[id].ft_describe( vp, &intern, 1 /*verbose*/,
	    base2local ) < 0 )
		rt_log("vls_solid: describe error\n");
	rt_functab[id].ft_ifree( &intern );
}

/*
 *  			P S C A L E
 *  
 *  Partial scaling of a solid.
 */
void
pscale()
{
	static fastf_t ma,mb;

	switch( es_menu ) {

	case MENU_TGC_SCALE_H:	/* scale height vector */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->h);
			}
			VSCALE(tgc->h, tgc->h, es_scale);
		}
		break;

	case MENU_TOR_R1:
		/* scale radius 1 of TOR */
		{
			struct rt_tor_internal	*tor = 
				(struct rt_tor_internal *)es_int.idb_ptr;
			fastf_t	newrad;
			RT_TOR_CK_MAGIC(tor);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				newrad = es_para[0];
			} else {
				newrad = tor->r_a * es_scale;
			}
			if( newrad < SMALL )  newrad = 4*SMALL;
			if( tor->r_h <= newrad )
				tor->r_a = newrad;
		}
		break;

	case MENU_TOR_R2:
		/* scale radius 2 of TOR */
		{
			struct rt_tor_internal	*tor = 
				(struct rt_tor_internal *)es_int.idb_ptr;
			fastf_t	newrad;
			RT_TOR_CK_MAGIC(tor);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				newrad = es_para[0];
			} else {
				newrad = tor->r_h * es_scale;
			}
			if( newrad < SMALL )  newrad = 4*SMALL;
			if( newrad <= tor->r_a )
				tor->r_h = newrad;
		}
		break;

	case MENU_ETO_R:
		/* scale radius 1 (r) of ETO */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;
			fastf_t	ch, cv, dh, newrad;
			vect_t	Nu;

			RT_ETO_CK_MAGIC(eto);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				newrad = es_para[0];
			} else {
				newrad = eto->eto_r * es_scale;
			}
			if( newrad < SMALL )  newrad = 4*SMALL;
			VMOVE(Nu, eto->eto_N);
			VUNITIZE(Nu);
			/* get horiz and vert components of C and Rd */
			cv = VDOT( eto->eto_C, Nu );
			ch = sqrt( VDOT( eto->eto_C, eto->eto_C ) - cv * cv );
			/* angle between C and Nu */
			dh = eto->eto_rd * cv / MAGNITUDE(eto->eto_C);
			/* make sure revolved ellipse doesn't overlap itself */
			if (ch <= newrad && dh <= newrad)
				eto->eto_r = newrad;
		}
		break;

	case MENU_ETO_RD:
		/* scale Rd, ellipse semi-minor axis length, of ETO */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;
			fastf_t	dh, newrad, work;
			vect_t	Nu;

			RT_ETO_CK_MAGIC(eto);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				newrad = es_para[0];
			} else {
				newrad = eto->eto_rd * es_scale;
			}
			if( newrad < SMALL )  newrad = 4*SMALL;
			work = MAGNITUDE(eto->eto_C);
				if (newrad <= work) {
				VMOVE(Nu, eto->eto_N);
				VUNITIZE(Nu);
				dh = newrad * VDOT( eto->eto_C, Nu ) / work;
				/* make sure revolved ellipse doesn't overlap itself */
				if (dh <= eto->eto_r)
					eto->eto_rd = newrad;
			}
		}
		break;

	case MENU_ETO_SCALE_C:
		/* scale vector C */
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;
			fastf_t	ch, cv;
			vect_t	Nu, Work;

			RT_ETO_CK_MAGIC(eto);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(eto->eto_C);
			}
			if (es_scale * MAGNITUDE(eto->eto_C) >= eto->eto_rd) {
				VMOVE(Nu, eto->eto_N);
				VUNITIZE(Nu);
				VSCALE(Work, eto->eto_C, es_scale);
				/* get horiz and vert comps of C and Rd */
				cv = VDOT( Work, Nu );
				ch = sqrt( VDOT( Work, Work ) - cv * cv );
				if (ch <= eto->eto_r)
					VMOVE(eto->eto_C, Work);
			}
		}
		break;

	case MENU_RPC_B:
		/* scale vector B */
		{
			struct rt_rpc_internal	*rpc = 
				(struct rt_rpc_internal *)es_int.idb_ptr;
			RT_RPC_CK_MAGIC(rpc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(rpc->rpc_B);
			}
			VSCALE(rpc->rpc_B, rpc->rpc_B, es_scale);
		}
		break;

	case MENU_RPC_H:
		/* scale vector H */
		{
			struct rt_rpc_internal	*rpc = 
				(struct rt_rpc_internal *)es_int.idb_ptr;

			RT_RPC_CK_MAGIC(rpc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(rpc->rpc_H);
			}
			VSCALE(rpc->rpc_H, rpc->rpc_H, es_scale);
		}
		break;

	case MENU_RPC_R:
		/* scale rectangular half-width of RPC */
		{
			struct rt_rpc_internal	*rpc = 
				(struct rt_rpc_internal *)es_int.idb_ptr;

			RT_RPC_CK_MAGIC(rpc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / rpc->rpc_r;
			}
			rpc->rpc_r *= es_scale;
		}
		break;

	case MENU_RHC_B:
		/* scale vector B */
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;
			RT_RHC_CK_MAGIC(rhc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(rhc->rhc_B);
			}
			VSCALE(rhc->rhc_B, rhc->rhc_B, es_scale);
		}
		break;

	case MENU_RHC_H:
		/* scale vector H */
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;
			RT_RHC_CK_MAGIC(rhc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(rhc->rhc_H);
			}
			VSCALE(rhc->rhc_H, rhc->rhc_H, es_scale);
		}
		break;

	case MENU_RHC_R:
		/* scale rectangular half-width of RHC */
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;

			RT_RHC_CK_MAGIC(rhc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / rhc->rhc_r;
			}
			rhc->rhc_r *= es_scale;
		}
		break;

	case MENU_RHC_C:
		/* scale rectangular half-width of RHC */
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;

			RT_RHC_CK_MAGIC(rhc);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / rhc->rhc_c;
			}
			rhc->rhc_c *= es_scale;
		}
		break;

	case MENU_EPA_H:
		/* scale height vector H */
		{
			struct rt_epa_internal	*epa = 
				(struct rt_epa_internal *)es_int.idb_ptr;

			RT_EPA_CK_MAGIC(epa);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(epa->epa_H);
			}
			VSCALE(epa->epa_H, epa->epa_H, es_scale);
		}
		break;

	case MENU_EPA_R1:
		/* scale semimajor axis of EPA */
		{
			struct rt_epa_internal	*epa = 
				(struct rt_epa_internal *)es_int.idb_ptr;

			RT_EPA_CK_MAGIC(epa);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / epa->epa_r1;
			}
			if (epa->epa_r1 * es_scale >= epa->epa_r2)
				epa->epa_r1 *= es_scale;
		}
		break;

	case MENU_EPA_R2:
		/* scale semiminor axis of EPA */
		{
			struct rt_epa_internal	*epa = 
				(struct rt_epa_internal *)es_int.idb_ptr;

			RT_EPA_CK_MAGIC(epa);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / epa->epa_r2;
			}
			if (epa->epa_r2 * es_scale <= epa->epa_r1)
				epa->epa_r2 *= es_scale;
		}
		break;

	case MENU_EHY_H:
		/* scale height vector H */
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;

			RT_EHY_CK_MAGIC(ehy);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(ehy->ehy_H);
			}
			VSCALE(ehy->ehy_H, ehy->ehy_H, es_scale);
		}
		break;

	case MENU_EHY_R1:
		/* scale semimajor axis of EHY */
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;

			RT_EHY_CK_MAGIC(ehy);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / ehy->ehy_r1;
			}
			if (ehy->ehy_r1 * es_scale >= ehy->ehy_r2)
				ehy->ehy_r1 *= es_scale;
		}
		break;

	case MENU_EHY_R2:
		/* scale semiminor axis of EHY */
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;

			RT_EHY_CK_MAGIC(ehy);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / ehy->ehy_r2;
			}
			if (ehy->ehy_r2 * es_scale <= ehy->ehy_r1)
				ehy->ehy_r2 *= es_scale;
		}
		break;

	case MENU_EHY_C:
		/* scale distance between apex of EHY & asymptotic cone */
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;

			RT_EHY_CK_MAGIC(ehy);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / ehy->ehy_c;
			}
			ehy->ehy_c *= es_scale;
		}
		break;

	case MENU_TGC_SCALE_A:
		/* scale vector A */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->a);
			}
			VSCALE(tgc->a, tgc->a, es_scale);
		}
		break;

	case MENU_TGC_SCALE_B:
		/* scale vector B */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->b);
			}
			VSCALE(tgc->b, tgc->b, es_scale);
		}
		break;

	case MENU_ELL_SCALE_A:
		/* scale vector A */
		{
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_scale = es_para[0] * es_mat[15] /
					MAGNITUDE(ell->a);
			}
			VSCALE( ell->a, ell->a, es_scale );
		}
		break;

	case MENU_ELL_SCALE_B:
		/* scale vector B */
		{
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_scale = es_para[0] * es_mat[15] /
					MAGNITUDE(ell->b);
			}
			VSCALE( ell->b, ell->b, es_scale );
		}
		break;

	case MENU_ELL_SCALE_C:
		/* scale vector C */
		{
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_scale = es_para[0] * es_mat[15] /
					MAGNITUDE(ell->c);
			}
			VSCALE( ell->c, ell->c, es_scale );
		}
		break;

	case MENU_TGC_SCALE_C:
		/* TGC: scale ratio "c" */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->c);
			}
			VSCALE(tgc->c, tgc->c, es_scale);
		}
		break;

	case MENU_TGC_SCALE_D:   /* scale  d for tgc */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->d);
			}
			VSCALE(tgc->d, tgc->d, es_scale);
		}
		break;

	case MENU_TGC_SCALE_AB:
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->a);
			}
			VSCALE(tgc->a, tgc->a, es_scale);
			ma = MAGNITUDE( tgc->a );
			mb = MAGNITUDE( tgc->b );
			VSCALE(tgc->b, tgc->b, ma/mb);
		}
		break;

	case MENU_TGC_SCALE_CD:	/* scale C and D of tgc */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->c);
			}
			VSCALE(tgc->c, tgc->c, es_scale);
			ma = MAGNITUDE( tgc->c );
			mb = MAGNITUDE( tgc->d );
			VSCALE(tgc->d, tgc->d, ma/mb);
		}
		break;

	case MENU_TGC_SCALE_ABCD: 		/* scale A,B,C, and D of tgc */
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);

			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_para[0] *= es_mat[15];
				es_scale = es_para[0] / MAGNITUDE(tgc->a);
			}
			VSCALE(tgc->a, tgc->a, es_scale);
			ma = MAGNITUDE( tgc->a );
			mb = MAGNITUDE( tgc->b );
			VSCALE(tgc->b, tgc->b, ma/mb);
			mb = MAGNITUDE( tgc->c );
			VSCALE(tgc->c, tgc->c, ma/mb);
			mb = MAGNITUDE( tgc->d );
			VSCALE(tgc->d, tgc->d, ma/mb);
		}
		break;

	case MENU_ELL_SCALE_ABC:	/* set A,B, and C length the same */
		{
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);
			if( inpara ) {
				/* take es_mat[15] (path scaling) into account */
				es_scale = es_para[0] * es_mat[15] /
					MAGNITUDE(ell->a);
			}
			VSCALE( ell->a, ell->a, es_scale );
			ma = MAGNITUDE( ell->a );
			mb = MAGNITUDE( ell->b );
			VSCALE(ell->b, ell->b, ma/mb);
			mb = MAGNITUDE( ell->c );
			VSCALE(ell->c, ell->c, ma/mb);
		}
		break;

	}
}

/*
 *			I N I T _ O B J E D I T
 *
 */
void
init_objedit()
{
	int			id;
	char			*strp="";

	/* for safety sake */
	es_menu = 0;
	es_edflag = -1;
	mat_idn(es_mat);

	/*
	 * Check for a processed region 
	 */
	if( illump->s_Eflag )  {

		/* Have a processed (E'd) region - NO key solid.
		 * 	Use the 'center' as the key
		 */
		VMOVE(es_keypoint, illump->s_center);

		/* The s_center takes the es_mat into account already */
	}

	/* Not an evaluated region - just a regular path ending in a solid */
	if( db_get_external( &es_ext, illump->s_path[illump->s_last], dbip ) < 0 )  {
		rt_log("init_objedit(%s): db_get_external failure\n",
			illump->s_path[illump->s_last]->d_namep );
		button(BE_REJECT);
		return;
	}

	id = rt_id_solid( &es_ext );
	if( rt_functab[id].ft_import( &es_int, &es_ext, rt_identity ) < 0 )  {
		rt_log("init_objedit(%s):  solid import failure\n",
			illump->s_path[illump->s_last]->d_namep );
	    	if( es_int.idb_ptr )  rt_functab[id].ft_ifree( &es_int );
		db_free_external( &es_ext );
		return;				/* FAIL */
	}
	RT_CK_DB_INTERNAL( &es_int );

	if( id == ID_ARB8 )
	{
		struct rt_arb_internal *arb;

		arb = (struct rt_arb_internal *)es_int.idb_ptr;
		RT_ARB_CK_MAGIC( arb );

		es_type = rt_arb_std_type( &es_int , &mged_tol );
	}

	get_solid_keypoint( es_keypoint , &strp , &es_int , es_mat );

	/* Save aggregate path matrix */
	pathHmat( illump, es_mat, illump->s_last-1 );

	/* get the inverse matrix */
	mat_inv( es_invmat, es_mat );

#ifdef XMGED
	set_e_axis_pos();
#endif
}

void
oedit_accept()
{
	register struct solid *sp;
	/* matrices used to accept editing done from a depth
	 *	>= 2 from the top of the illuminated path
	 */
	mat_t topm;	/* accum matrix from pathpos 0 to i-2 */
	mat_t inv_topm;	/* inverse */
	mat_t deltam;	/* final "changes":  deltam = (inv_topm)(modelchanges)(topm) */
	mat_t tempm;

	switch( ipathpos )  {
	case 0:
		moveHobj( illump->s_path[ipathpos], modelchanges );
		break;
	case 1:
		moveHinstance(
			illump->s_path[ipathpos-1],
			illump->s_path[ipathpos],
			modelchanges
		);
		break;
	default:
		mat_idn( topm );
		mat_idn( inv_topm );
		mat_idn( deltam );
		mat_idn( tempm );

		pathHmat( illump, topm, ipathpos-2 );

		mat_inv( inv_topm, topm );

		mat_mul( tempm, modelchanges, topm );
		mat_mul( deltam, inv_topm, tempm );

		moveHinstance(
			illump->s_path[ipathpos-1],
			illump->s_path[ipathpos],
			deltam
		);
		break;
	}

	/*
	 *  Redraw all solids affected by this edit.
	 *  Regenerate a new control list which does not
	 *  include the solids about to be replaced,
	 *  so we can safely fiddle the displaylist.
	 */
	modelchanges[15] = 1000000000;	/* => small ratio */
	dmaflag=1;
	refresh();

	/* Now, recompute new chunks of displaylist */
	FOR_ALL_SOLIDS( sp )  {
		if( sp->s_iflag == DOWN )
			continue;
		(void)replot_original_solid( sp );
		sp->s_iflag = DOWN;
	}
	mat_idn( modelchanges );

    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
	es_int.idb_ptr = (genptr_t)NULL;
	db_free_external( &es_ext );
}

void
oedit_reject()
{
    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
	es_int.idb_ptr = (genptr_t)NULL;
	db_free_external( &es_ext );
}

/* 			F _ E Q N ( )
 * Gets the A,B,C of a  planar equation from the command line and puts the
 * result into the array es_peqn[] at the position pointed to by the variable
 * 'es_menu' which is the plane being redefined. This function is only callable
 * when in solid edit and rotating the face of a GENARB8.
 */
int
f_eqn(argc, argv)
int	argc;
char	*argv[];
{
	short int i;
	vect_t tempvec;
	struct rt_arb_internal *arb;

	if( state != ST_S_EDIT ){
		rt_log("Eqn: must be in solid edit\n");
		return CMD_BAD;
	}

	if( es_int.idb_type != ID_ARB8 )
	{
		rt_log("Eqn: type must be GENARB8\n");
		return CMD_BAD;
	}

	if( es_edflag != ECMD_ARB_ROTATE_FACE ){
		rt_log("Eqn: must be rotating a face\n");
		return CMD_BAD;
	}

	arb = (struct rt_arb_internal *)es_int.idb_ptr;
	RT_ARB_CK_MAGIC( arb );

	/* get the A,B,C from the command line */
	for(i=0; i<3; i++)
		es_peqn[es_menu][i]= atof(argv[i+1]);
	VUNITIZE( &es_peqn[es_menu][0] );

	VMOVE( tempvec , arb->pt[fixv] );
	es_peqn[es_menu][3]=VDOT( es_peqn[es_menu], tempvec );
	if( rt_arb_calc_points( arb , es_type , es_peqn , &mged_tol ) )
		return CMD_BAD;

	/* draw the new version of the solid */
	replot_editing_solid();

	/* update display information */
	dmaflag = 1;

	return CMD_OK;
}

/* Hooks from buttons.c */

void
sedit_accept()
{
	struct directory	*dp;

	if( not_state( ST_S_EDIT, "Solid edit accept" ) )  return;

	es_eu = (struct edgeuse *)NULL;	/* Reset es_eu */
	if( lu_copy )
	{
		struct model *m;

		m = nmg_find_model( &lu_copy->l.magic );
		nmg_km( m );
		lu_copy = (struct loopuse *)NULL;
	}

	/* write editing changes out to disc */
	dp = illump->s_path[illump->s_last];

	/* Scale change on export is 1.0 -- no change */
	if( rt_functab[es_int.idb_type].ft_export( &es_ext, &es_int, 1.0 ) < 0 )  {
		rt_log("sedit_accept(%s):  solid export failure\n", dp->d_namep);
	    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
		db_free_external( &es_ext );
		return;				/* FAIL */
	}
    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );

	if( db_put_external( &es_ext, dp, dbip ) < 0 )  {
		db_free_external( &es_ext );
		ERROR_RECOVERY_SUGGESTION;
		WRITE_ERR_return;
	}

	es_edflag = -1;
	menuflag = 0;
	movedir = 0;

    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
	es_int.idb_ptr = (genptr_t)NULL;
	db_free_external( &es_ext );
}

void
sedit_reject()
{
	if( not_state( ST_S_EDIT, "Solid edit reject" ) )  return;

	es_eu = (struct edgeuse *)NULL;	/* Reset es_eu */
	if( lu_copy )
	{
		struct model *m;

		m = nmg_find_model( &lu_copy->l.magic );
		nmg_km( m );
		lu_copy = (struct loopuse *)NULL;
	}

	/* Restore the original solid */
	replot_original_solid( illump );

	menuflag = 0;
	movedir = 0;
	es_edflag = -1;

    	if( es_int.idb_ptr )  rt_functab[es_int.idb_type].ft_ifree( &es_int );
	es_int.idb_ptr = (genptr_t)NULL;
	db_free_external( &es_ext );
}

/* Input parameter editing changes from keyboard */
/* Format: p dx [dy dz]		*/
int
f_param( argc, argv )
int	argc;
char	**argv;
{
	register int i;

	if( es_edflag <= 0 )  {
		rt_log("A solid editor option not selected\n");
		return CMD_BAD;
	}
	if( es_edflag == ECMD_TGC_ROT_H
		|| es_edflag == ECMD_TGC_ROT_AB
		|| es_edflag == ECMD_ETO_ROT_C ) {
		rt_log("\"p\" command not defined for this option\n");
		return CMD_BAD;
	}

	inpara = 0;
	sedraw++;
	for( i = 1; i < argc && i <= 3 ; i++ )  {
		es_para[ inpara++ ] = atof( argv[i] );
	}

	if( es_edflag == PSCALE || es_edflag == SSCALE )  {
		if(es_para[0] <= 0.0) {
			rt_log("ERROR: SCALE FACTOR <= 0\n");
			inpara = 0;
			sedraw = 0;
			return CMD_BAD;
		}
	}

	/* check if need to convert input values to the base unit */
	switch( es_edflag ) {

		case STRANS:
		case ECMD_VTRANS:
		case PSCALE:
		case EARB:
		case ECMD_ARB_MOVE_FACE:
		case ECMD_TGC_MV_H:
		case ECMD_TGC_MV_HH:
		case PTARB:
		case ECMD_NMG_ESPLIT:
		case ECMD_NMG_EMOVE:
		case ECMD_NMG_LEXTRU:
			/* must convert to base units */
			es_para[0] *= local2base;
			es_para[1] *= local2base;
			es_para[2] *= local2base;
			/* fall through */
		default:
#ifdef XMGED
			break;
	}

	if(es_edflag >= SROT && es_edflag <= ECMD_ETO_ROT_C){
	  if(!irot_set){
	    irot_x = es_para[0];
	    irot_y = es_para[1];
	    irot_z = es_para[2];
	  }

	  if(rot_hook)
	    (*rot_hook)();
	}else if(es_edflag >= STRANS && es_edflag <= PTARB){
	  if(!tran_set)
	    set_tran(es_para[0], es_para[1], es_para[2]);

	  if(tran_hook)
	    (*tran_hook)();
	}

	return CMD_OK;
#else
			return CMD_OK;
	}
#endif

	/* XXX I would prefer to see an explicit call to the guts of sedit()
	 * XXX here, rather than littering the place with global variables
	 * XXX for later interpretation.
	 */
}

/*
 *  Returns -
 *	1	solid edit claimes the rotate event
 *	0	rotate event can be used some other way.
 */
int
sedit_rotate( xangle, yangle, zangle )
double	xangle, yangle, zangle;
{
	mat_t	tempp;

	if( es_edflag != ECMD_TGC_ROT_H &&
	    es_edflag != ECMD_TGC_ROT_AB &&
	    es_edflag != SROT &&
	    es_edflag != ECMD_ETO_ROT_C &&
	    es_edflag != ECMD_ARB_ROTATE_FACE)
		return 0;

	mat_idn( incr_change );
	buildHrot( incr_change, xangle, yangle, zangle );

	/* accumulate the translations */
	mat_mul(tempp, incr_change, acc_rot_sol);
	mat_copy(acc_rot_sol, tempp);

	/* sedit() will use incr_change or acc_rot_sol ?? */
	sedit();	/* change es_int only, NOW */

	return 1;
}

/*
 *  Returns -
 *	1	object edit claimes the rotate event
 *	0	rotate event can be used some other way.
 */
int
objedit_rotate( xangle, yangle, zangle )
double	xangle, yangle, zangle;
{
	mat_t	tempp;
	vect_t	point;

	if( movedir != ROTARROW )  return 0;

	mat_idn( incr_change );
	buildHrot( incr_change, xangle, yangle, zangle );

	/* accumulate change matrix - do it wrt a point NOT view center */
	mat_mul(tempp, modelchanges, es_mat);
	MAT4X3PNT(point, tempp, es_keypoint);
	wrt_point(modelchanges, incr_change, modelchanges, point);

	new_mats();

	return 1;
}

/*
 *			L A B E L _ E D I T E D _ S O L I D
 *
 *  Put labels on the vertices of the currently edited solid.
 *  XXX This really should use import/export interface!!!  Or be part of it.
 */
label_edited_solid( pl, max_pl, xform, ip )
struct rt_point_labels	pl[];
int			max_pl;
CONST mat_t		xform;
struct rt_db_internal	*ip;
{
	register int	i;
	point_t		work;
	point_t		pos_view;
	int		npl = 0;

	RT_CK_DB_INTERNAL( ip );

	switch( ip->idb_type )  {

#define	POINT_LABEL( _pt, _char )	{ \
	VMOVE( pl[npl].pt, _pt ); \
	pl[npl].str[0] = _char; \
	pl[npl++].str[1] = '\0'; }

#define	POINT_LABEL_STR( _pt, _str )	{ \
	VMOVE( pl[npl].pt, _pt ); \
	strncpy( pl[npl++].str, _str, sizeof(pl[0].str)-1 ); }

	case ID_ARB8:
		{
			struct rt_arb_internal *arb=
				(struct rt_arb_internal *)es_int.idb_ptr;
			RT_ARB_CK_MAGIC( arb );
			switch( es_type )
			{
				case ARB8:
					for( i=0 ; i<8 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					break;
				case ARB7:
					for( i=0 ; i<7 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					break;
				case ARB6:
					for( i=0 ; i<5 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					MAT4X3PNT( pos_view, xform, arb->pt[6] );
					POINT_LABEL( pos_view, '6' );
					break;
				case ARB5:
					for( i=0 ; i<5 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					break;
				case ARB4:
					for( i=0 ; i<3 ; i++ )
					{
						MAT4X3PNT( pos_view, xform, arb->pt[i] );
						POINT_LABEL( pos_view, i+'1' );
					}
					MAT4X3PNT( pos_view, xform, arb->pt[4] );
					POINT_LABEL( pos_view, '4' );
					break;
			}
		}
		break;
	case ID_TGC:
		{
			struct rt_tgc_internal	*tgc = 
				(struct rt_tgc_internal *)es_int.idb_ptr;
			RT_TGC_CK_MAGIC(tgc);
			MAT4X3PNT( pos_view, xform, tgc->v );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, tgc->v, tgc->a );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'A' );

			VADD2( work, tgc->v, tgc->b );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VADD3( work, tgc->v, tgc->h, tgc->c );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'C' );

			VADD3( work, tgc->v, tgc->h, tgc->d );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'D' );
		}
		break;

	case ID_ELL:
		{
			point_t	work;
			point_t	pos_view;
			struct rt_ell_internal	*ell = 
				(struct rt_ell_internal *)es_int.idb_ptr;
			RT_ELL_CK_MAGIC(ell);

			MAT4X3PNT( pos_view, xform, ell->v );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, ell->v, ell->a );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'A' );

			VADD2( work, ell->v, ell->b );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VADD2( work, ell->v, ell->c );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'C' );
		}
		break;

	case ID_TOR:
		{
			struct rt_tor_internal	*tor = 
				(struct rt_tor_internal *)es_int.idb_ptr;
			fastf_t	r3, r4;
			vect_t	adir;
			RT_TOR_CK_MAGIC(tor);

			mat_vec_ortho( adir, tor->h );

			MAT4X3PNT( pos_view, xform, tor->v );
			POINT_LABEL( pos_view, 'V' );

			r3 = tor->r_a - tor->r_h;
			VJOIN1( work, tor->v, r3, adir );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'I' );

			r4 = tor->r_a + tor->r_h;
			VJOIN1( work, tor->v, r4, adir );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'O' );

			VJOIN1( work, tor->v, tor->r_a, adir );
			VADD2( work, work, tor->h );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );
		}
		break;

	case ID_RPC:
		{
			struct rt_rpc_internal	*rpc = 
				(struct rt_rpc_internal *)es_int.idb_ptr;
			vect_t	Ru;

			RT_RPC_CK_MAGIC(rpc);
			MAT4X3PNT( pos_view, xform, rpc->rpc_V );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, rpc->rpc_V, rpc->rpc_B );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VADD2( work, rpc->rpc_V, rpc->rpc_H );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );

			VCROSS( Ru, rpc->rpc_B, rpc->rpc_H );
			VUNITIZE( Ru );
			VSCALE( Ru, Ru, rpc->rpc_r );
			VADD2( work, rpc->rpc_V, Ru );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'r' );
		}
		break;

	case ID_RHC:
		{
			struct rt_rhc_internal	*rhc = 
				(struct rt_rhc_internal *)es_int.idb_ptr;
			vect_t	Ru;

			RT_RHC_CK_MAGIC(rhc);
			MAT4X3PNT( pos_view, xform, rhc->rhc_V );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, rhc->rhc_V, rhc->rhc_B );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VADD2( work, rhc->rhc_V, rhc->rhc_H );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );

			VCROSS( Ru, rhc->rhc_B, rhc->rhc_H );
			VUNITIZE( Ru );
			VSCALE( Ru, Ru, rhc->rhc_r );
			VADD2( work, rhc->rhc_V, Ru );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'r' );

			VMOVE( work, rhc->rhc_B );
			VUNITIZE( work );
			VSCALE( work, work,
				MAGNITUDE(rhc->rhc_B) + rhc->rhc_c );
			VADD2( work, work, rhc->rhc_V );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'c' );
		}
		break;

	case ID_EPA:
		{
			struct rt_epa_internal	*epa = 
				(struct rt_epa_internal *)es_int.idb_ptr;
			vect_t	A, B;

			RT_EPA_CK_MAGIC(epa);
			MAT4X3PNT( pos_view, xform, epa->epa_V );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, epa->epa_V, epa->epa_H );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );

			VSCALE( A, epa->epa_Au, epa->epa_r1 );
			VADD2( work, epa->epa_V, A );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'A' );

			VCROSS( B, epa->epa_Au, epa->epa_H );
			VUNITIZE( B );
			VSCALE( B, B, epa->epa_r2 );
			VADD2( work, epa->epa_V, B );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );
		}
		break;

	case ID_EHY:
		{
			struct rt_ehy_internal	*ehy = 
				(struct rt_ehy_internal *)es_int.idb_ptr;
			vect_t	A, B;

			RT_EHY_CK_MAGIC(ehy);
			MAT4X3PNT( pos_view, xform, ehy->ehy_V );
			POINT_LABEL( pos_view, 'V' );

			VADD2( work, ehy->ehy_V, ehy->ehy_H );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'H' );

			VSCALE( A, ehy->ehy_Au, ehy->ehy_r1 );
			VADD2( work, ehy->ehy_V, A );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'A' );

			VCROSS( B, ehy->ehy_Au, ehy->ehy_H );
			VUNITIZE( B );
			VSCALE( B, B, ehy->ehy_r2 );
			VADD2( work, ehy->ehy_V, B );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'B' );

			VMOVE( work, ehy->ehy_H );
			VUNITIZE( work );
			VSCALE( work, work,
				MAGNITUDE(ehy->ehy_H) + ehy->ehy_c );
			VADD2( work, ehy->ehy_V, work );
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'c' );
		}
		break;

	case ID_ETO:
		{
			struct rt_eto_internal	*eto = 
				(struct rt_eto_internal *)es_int.idb_ptr;
			fastf_t	ch, cv, dh, dv, cmag, phi;
			vect_t	Au, Nu;

			RT_ETO_CK_MAGIC(eto);

			MAT4X3PNT( pos_view, xform, eto->eto_V );
			POINT_LABEL( pos_view, 'V' );

			VMOVE(Nu, eto->eto_N);
			VUNITIZE(Nu);
			vec_ortho( Au, Nu );
			VUNITIZE(Au);

			cmag = MAGNITUDE(eto->eto_C);
			/* get horizontal and vertical components of C and Rd */
			cv = VDOT( eto->eto_C, Nu );
			ch = sqrt( cmag*cmag - cv*cv );
			/* angle between C and Nu */
			phi = acos( cv / cmag );
			dv = -eto->eto_rd * sin(phi);
			dh = eto->eto_rd * cos(phi);

			VJOIN2(work, eto->eto_V, eto->eto_r+ch, Au, cv, Nu);
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'C' );

			VJOIN2(work, eto->eto_V, eto->eto_r+dh, Au, dv, Nu);
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'D' );

			VJOIN1(work, eto->eto_V, eto->eto_r, Au);
			MAT4X3PNT(pos_view, xform, work);
			POINT_LABEL( pos_view, 'r' );
		}
		break;

	case ID_ARS:
		{
			register struct rt_ars_internal *ars=
				(struct rt_ars_internal *)es_int.idb_ptr;

			RT_ARS_CK_MAGIC( ars );

			MAT4X3PNT(pos_view, xform, ars->curves[0] )
		}
		POINT_LABEL( pos_view, 'V' );
		break;

	case ID_BSPLINE:
		{
			register struct rt_nurb_internal *sip =
				(struct rt_nurb_internal *) es_int.idb_ptr;
			register struct snurb	*surf;
			register fastf_t	*fp;

			RT_NURB_CK_MAGIC(sip);
			surf = sip->srfs[spl_surfno];
			NMG_CK_SNURB(surf);
			fp = &RT_NURB_GET_CONTROL_POINT( surf, spl_ui, spl_vi );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL( pos_view, 'V' );

			fp = &RT_NURB_GET_CONTROL_POINT( surf, 0, 0 );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL_STR( pos_view, " 0,0" );
			fp = &RT_NURB_GET_CONTROL_POINT( surf, 0, surf->s_size[1]-1 );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL_STR( pos_view, " 0,u" );
			fp = &RT_NURB_GET_CONTROL_POINT( surf, surf->s_size[0]-1, 0 );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL_STR( pos_view, " v,0" );
			fp = &RT_NURB_GET_CONTROL_POINT( surf, surf->s_size[0]-1, surf->s_size[1]-1 );
			MAT4X3PNT(pos_view, xform, fp);
			POINT_LABEL_STR( pos_view, " u,v" );
		}
		break;
	case ID_NMG:
		/* New way only */
		{
			register struct model *m =
				(struct model *) es_int.idb_ptr;
			NMG_CK_MODEL(m);

			if( es_eu )  {
				point_t	cent;
				NMG_CK_EDGEUSE(es_eu);
				VADD2SCALE( cent,
					es_eu->vu_p->v_p->vg_p->coord,
					es_eu->eumate_p->vu_p->v_p->vg_p->coord,
					0.5 );
				MAT4X3PNT(pos_view, xform, cent);
				POINT_LABEL_STR( pos_view, " eu" );
			}
		}
		break;
	}

	pl[npl].str[0] = '\0';	/* Mark ending */
}

/* -------------------------------- */
/*
 *			R T _ A R B _ C A L C _ P L A N E S
 *
 *	Calculate the plane (face) equations for an arb
 *	output previously went to es_peqn[i].
 *
 *  Returns -
 *	-1	Failure
 *	 0	OK
 */
int
rt_arb_calc_planes( planes, arb, type, tol )
plane_t			planes[6];
struct rt_arb_internal	*arb;
int			type;
CONST struct rt_tol	*tol;
{
	register int i, p1, p2, p3;

	RT_ARB_CK_MAGIC( arb);
	RT_CK_TOL( tol );

	type -= 4;	/* ARB4 at location 0, ARB5 at 1, etc */

	for(i=0; i<6; i++) {
		if(arb_faces[type][i*4] == -1)
			break;	/* faces are done */
		p1 = arb_faces[type][i*4];
		p2 = arb_faces[type][i*4+1];
		p3 = arb_faces[type][i*4+2];

		if( rt_mk_plane_3pts( planes[i],
		    arb->pt[p1], arb->pt[p2], arb->pt[p3], tol ) < 0 )  {
			rt_log("rt_arb_calc_planes: No eqn for face %d%d%d%d\n",
				p1+1, p2+1, p3+1,
				arb_faces[type][i*4+3]+1);
			return -1;
		}
	}
	return 0;
}

/* -------------------------------- */
void
sedit_vpick( v_pos )
point_t	v_pos;
{
	point_t	m_pos;
	int	surfno, u, v;

	MAT4X3PNT( m_pos, objview2model, v_pos );

	if( nurb_closest2d( &surfno, &u, &v,
	    (struct rt_nurb_internal *)es_int.idb_ptr,
	    m_pos, model2objview ) >= 0 )  {
		spl_surfno = surfno;
		spl_ui = u;
		spl_vi = v;
		get_solid_keypoint( es_keypoint, &es_keytag, &es_int, es_mat );
	}
	chg_state( ST_S_VPICK, ST_S_EDIT, "Vertex Pick Complete");
	dmaflag = 1;
}

#define DIST2D(P0, P1)	sqrt(	((P1)[X] - (P0)[X])*((P1)[X] - (P0)[X]) + \
				((P1)[Y] - (P0)[Y])*((P1)[Y] - (P0)[Y]) )

#define DIST3D(P0, P1)	sqrt(	((P1)[X] - (P0)[X])*((P1)[X] - (P0)[X]) + \
				((P1)[Y] - (P0)[Y])*((P1)[Y] - (P0)[Y]) + \
				((P1)[Z] - (P0)[Z])*((P1)[Z] - (P0)[Z]) )

/*
 *	C L O S E S T 3 D
 *
 *	Given a vlist pointer (vhead) to point coordinates and a reference
 *	point (ref_pt), pass back in "closest_pt" the coordinates of the
 *	point nearest the reference point in 3 space.
 *
 */
int
rt_vl_closest3d(vhead, ref_pt, closest_pt)
struct rt_list	*vhead;
point_t		ref_pt, closest_pt;
{
	fastf_t		dist, cur_dist;
	pointp_t	c_pt;
	struct rt_vlist	*cur_vp;
	
	if (vhead == RT_LIST_NULL || RT_LIST_IS_EMPTY(vhead))
		return(1);	/* fail */

	/* initialize smallest distance using 1st point in list */
	cur_vp = RT_LIST_FIRST(rt_vlist, vhead);
	dist = DIST3D(ref_pt, cur_vp->pt[0]);
	c_pt = cur_vp->pt[0];

	for (RT_LIST_FOR(cur_vp, rt_vlist, vhead)) {
		register int	i;
		register int	nused = cur_vp->nused;
		register point_t *cur_pt = cur_vp->pt;
		
		for (i = 0; i < nused; i++) {
			cur_dist = DIST3D(ref_pt, cur_pt[i]);
			if (cur_dist < dist) {
				dist = cur_dist;
				c_pt = cur_pt[i];
			}
		}
	}
	VMOVE(closest_pt, c_pt);
	return(0);	/* success */
}

/*
 *	C L O S E S T 2 D
 *
 *	Given a pointer (vhead) to vlist point coordinates, a reference
 *	point (ref_pt), and a transformation matrix (mat), pass back in
 *	"closest_pt" the original, untransformed 3 space coordinates of
 *	the point nearest the reference point after all points have been
 *	transformed into 2 space projection plane coordinates.
 */
int
rt_vl_closest2d(vhead, ref_pt, mat, closest_pt)
struct rt_list	*vhead;
point_t		ref_pt, closest_pt;
mat_t		mat;
{
	fastf_t		dist, cur_dist;
	point_t		cur_pt2d, ref_pt2d;
	pointp_t	c_pt;
	struct rt_vlist	*cur_vp;
	
	if (vhead == RT_LIST_NULL || RT_LIST_IS_EMPTY(vhead))
		return(1);	/* fail */

	/* transform reference point to 2d */
	MAT4X3PNT(ref_pt2d, mat, ref_pt);

	/* initialize smallest distance using 1st point in list */
	cur_vp = RT_LIST_FIRST(rt_vlist, vhead);
	MAT4X3PNT(cur_pt2d, mat, cur_vp->pt[0]);
	dist = DIST2D(ref_pt2d, cur_pt2d);
	c_pt = cur_vp->pt[0];

	for (RT_LIST_FOR(cur_vp, rt_vlist, vhead)) {
		register int	i;
		register int	nused = cur_vp->nused;
		register point_t *cur_pt = cur_vp->pt;
		
		for (i = 0; i < nused; i++) {
			MAT4X3PNT(cur_pt2d, mat, cur_pt[i]);
			cur_dist = DIST2D(ref_pt2d, cur_pt2d);
			if (cur_dist < dist) {
				dist = cur_dist;
				c_pt = cur_pt[i];
			}
		}
	}
	VMOVE(closest_pt, c_pt);
	return(0);	/* success */
}

/*
 *				N U R B _ C L O S E S T 3 D
 *
 *	Given a vlist pointer (vhead) to point coordinates and a reference
 *	point (ref_pt), pass back in "closest_pt" the coordinates of the
 *	point nearest the reference point in 3 space.
 *
 */
int
nurb_closest3d(surface, uval, vval, spl, ref_pt )
int				*surface;
int				*uval;
int				*vval;
CONST struct rt_nurb_internal	*spl;
CONST point_t			ref_pt;
{
	struct snurb	*srf;
	fastf_t		*mesh;
	fastf_t		d;
	fastf_t		c_dist;		/* closest dist so far */
	int		c_surfno;
	int		c_u, c_v;
	int		u, v;
	int		i;

	RT_NURB_CK_MAGIC(spl);

	c_dist = INFINITY;
	c_surfno = c_u = c_v = -1;

	for( i = 0; i < spl->nsrf; i++ )  {
		int	advance;

		srf = spl->srfs[i];
		NMG_CK_SNURB(srf);
		mesh = srf->ctl_points;
		advance = RT_NURB_EXTRACT_COORDS(srf->pt_type);

		for( v = 0; v < srf->s_size[0]; v++ )  {
			for( u = 0; u < srf->s_size[1]; u++ )  {
				/* XXX 4-tuples? */
				d = DIST3D(ref_pt, mesh);
				if (d < c_dist)  {
					c_dist = d;
					c_surfno = i;
					c_u = u;
					c_v = v;
				}
				mesh += advance;
			}
		}
	}
	if( c_surfno < 0 )  return  -1;		/* FAIL */
	*surface = c_surfno;
	*uval = c_u;
	*vval = c_v;

	return(0);				/* success */
}

/*
 *				N U R B _ C L O S E S T 2 D
 *
 *	Given a pointer (vhead) to vlist point coordinates, a reference
 *	point (ref_pt), and a transformation matrix (mat), pass back in
 *	"closest_pt" the original, untransformed 3 space coordinates of
 *	the point nearest the reference point after all points have been
 *	transformed into 2 space projection plane coordinates.
 */
int
nurb_closest2d(surface, uval, vval, spl, ref_pt, mat )
int				*surface;
int				*uval;
int				*vval;
CONST struct rt_nurb_internal	*spl;
CONST point_t			ref_pt;
CONST mat_t			mat;
{
	struct snurb	*srf;
	point_t		ref_2d;
	fastf_t		*mesh;
	fastf_t		d;
	fastf_t		c_dist;		/* closest dist so far */
	int		c_surfno;
	int		c_u, c_v;
	int		u, v;
	int		i;

	RT_NURB_CK_MAGIC(spl);

	c_dist = INFINITY;
	c_surfno = c_u = c_v = -1;

	/* transform reference point to 2d */
	MAT4X3PNT(ref_2d, mat, ref_pt);

	for( i = 0; i < spl->nsrf; i++ )  {
		int	advance;

		srf = spl->srfs[i];
		NMG_CK_SNURB(srf);
		mesh = srf->ctl_points;
		advance = RT_NURB_EXTRACT_COORDS(srf->pt_type);

		for( v = 0; v < srf->s_size[0]; v++ )  {
			for( u = 0; u < srf->s_size[1]; u++ )  {
				point_t	cur;
				/* XXX 4-tuples? */
				MAT4X3PNT( cur, mat, mesh );
				d = DIST2D(ref_2d, cur);
				if (d < c_dist)  {
					c_dist = d;
					c_surfno = i;
					c_u = u;
					c_v = v;
				}
				mesh += advance;
			}
		}
	}
	if( c_surfno < 0 )  return  -1;		/* FAIL */
	*surface = c_surfno;
	*uval = c_u;
	*vval = c_v;

	return(0);				/* success */
}


int
f_keypoint (argc, argv)
int	argc;
char	**argv;
{
	if ((state != ST_S_EDIT) && (state != ST_O_EDIT))
	{
	    state_err("keypoint assignment");
	    return CMD_BAD;
	}

	switch (--argc)
	{
	    case 0:
		rt_log("%s (%g, %g, %g)\n", es_keytag, V3ARGS(es_keypoint));
		break;
	    case 3:
		VSET(es_keypoint,
		    atof( argv[1] ) * local2base,
		    atof( argv[2] ) * local2base,
		    atof( argv[3] ) * local2base);
		es_keytag = "user-specified";
		es_keyfixed = 1;
		break;
	    case 1:
		if (strcmp(argv[1], "reset") == 0)
		{
		    es_keytag = "";
		    es_keyfixed = 0;
		    get_solid_keypoint(es_keypoint, &es_keytag,
					&es_int, es_mat);
		    break;
		}
	    default:
		rt_log("Usage: 'keypoint [<x y z> | reset]'\n");
		return CMD_BAD;
	}

	dmaflag = 1;
	return CMD_OK;
}
