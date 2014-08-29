/*                          D M - T K . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2014 United States Government as represented by
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
/** @file mged/dm-tk.c
 *
 * Routines specific to MGED's use of LIBDM's Tk display manager.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_TK
#  include "tk.h"
#endif

#include "bu.h"
#include "vmath.h"
#include "mater.h"
#include "raytrace.h"
#include "dm/dm_xvars.h"
#include "dm/dm-tk.h"
#include "../libdm/dm_private.h"
#include "fbio.h"

#include "./mged.h"
#include "./sedit.h"
#include "./mged_dm.h"

extern int _tk_open_existing();	/* XXX TJM will be defined in libfb/if_tk.c */

int
tk_dm_init(struct dm_list *o_dm_list,
	   int argc,
	   const char *argv[])
{
    struct bu_vls vls = BU_VLS_INIT_ZERO;

    dm_var_init(o_dm_list);

    /* register application provided routines */
    cmd_hook = dm_commands;

    Tk_DeleteGenericHandler(doEvent, (ClientData)NULL);
    dmp = dm_open(INTERP, DM_TYPE_TK, argc-1, argv);
    if (dmp == DM_NULL)
	return TCL_ERROR;

    /* keep display manager in sync */
    dm_set_perspective(dmp, mged_variables->mv_perspective_mode);

    eventHandler = tk_doevent;
    Tk_CreateGenericHandler(doEvent, (ClientData)NULL);
    (void)dm_configure_win(dmp, 0);

    bu_vls_printf(&vls, "mged_bind_dm %s", bu_vls_addr(dm_get_pathname(dmp)));
    Tcl_Eval(INTERP, bu_vls_addr(&vls));
    bu_vls_free(&vls);

    return TCL_OK;
}

/*
  This routine is being called from doEvent() to handle Expose events.
*/
static int
tk_doevent(ClientData UNUSED(clientData), XEvent *eventPtr)
{
    if (eventPtr->type == Expose && eventPtr->xexpose.count == 0) {
	dirty = 1;

	/* no further processing of this event */
	return TCL_RETURN;
    }

    /* allow further processing of this event */
    return TCL_OK;
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
