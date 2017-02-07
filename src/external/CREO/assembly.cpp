/*               C R E O - B R L - I N I T . C P P
 * BRL-CAD
 *
 * Copyright (c) 2017 United States Government as represented by
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
/** @file creo-brl-init.cpp
 *
 */

#include "common.h"
#include "creo-brl.h"

/* structure to hold info about a member of the current assembly
 * this structure is created during feature visit
 */
struct asm_member {
    ProCharName name;
    ProMatrix xform;
    ProMdlType type;
    struct asm_member *next;
};

/* structure to hold info about current assembly
 * members are added during feature visit
 */
struct asm_head {
    ProCharName name;
    ProMdl model;
    struct asm_member *members;
};



/* routine that is called by feature visit for each assembly member
 * the "app_data" is the head of the assembly info for this assembly
 */
extern "C" ProError
assembly_comp( ProFeature *feat, ProError status, ProAppData app_data )
{
    ProIdTable id_table;
    ProMdl model;
    ProAsmcomppath comp_path;
    ProMatrix xform;
    ProMdlType type;
    ProName name;
    struct asm_head *curr_assem = (struct asm_head *)app_data;
    struct asm_member *member, *prev=NULL;
    int i, j;

    status = ProAsmcompMdlNameGet( feat, &type, name );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "ProAsmcompMdlNameGet() failed\n" );
	return status;
    }
    (void)ProWstringToString( curr_part_name, name );
    if ( logger_type == LOGGER_TYPE_ALL ) {
	fprintf( logger, "Processing assembly member %s\n", curr_part_name );
    }

    /* the next two Pro/Toolkit calls are the only way I could find to get
     * the transformation matrix to apply to this member.
     * this call is creating a path from the assembly to this particular member
     * (assembly/member)
     */
    id_table[0] = feat->id;
    status = ProAsmcomppathInit( (ProSolid)curr_assem->model, id_table, 1, &comp_path );
    if ( status != PRO_TK_NO_ERROR ) {
	snprintf( astr, sizeof(astr), "Failed to get path from %s to %s (aborting)", curr_asm_name,
		curr_part_name );
	(void)ProMessageDisplay(MSGFIL, "USER_ERROR", astr );
	ProMessageClear();
	fprintf( stderr, "%s\n", astr );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return status;
    }

    /* this call accumulates the xform matrix along the path created above */
    status = ProAsmcomppathTrfGet( &comp_path, PRO_B_TRUE, xform );
    if ( status != PRO_TK_NO_ERROR ) {
	snprintf( astr, sizeof(astr), "Failed to get transformation matrix %s/%s, error = %d, id = %d",
		curr_asm_name, curr_part_name, status, feat->id );
	(void)ProMessageDisplay(MSGFIL, "USER_ERROR", astr );
	ProMessageClear();
	fprintf( stderr, "%s\n", astr );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return PRO_TK_NO_ERROR;
    }

    /* add this member to our assembly info */
    prev = NULL;
    member = curr_assem->members;
    if ( member ) {
	while ( member->next ) {
	    prev = member;
	    member = member->next;
	}
	BU_ALLOC(member->next, struct asm_member);
	prev = member;
	member = member->next;
    } else {
	BU_ALLOC(curr_assem->members, struct asm_member);
	member = curr_assem->members;
    }

    if ( !member ) {
	(void)ProMessageDisplay(MSGFIL, "USER_ERROR",
		"memory allocation for member failed" );
	ProMessageClear();
	fprintf( stderr, "memory allocation for member failed\n" );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return PRO_TK_GENERAL_ERROR;
    }
    member->next = NULL;

    /* capture its name */
    (void)ProWstringToString( member->name, name );

    /* copy xform matrix */
    for ( i=0; i<4; i++ ) {
	for ( j=0; j<4; j++ ) {
	    member->xform[i][j] = xform[i][j];
	}
    }

    /* get the model for this member */
    status = ProAsmcompMdlGet( feat, &model );
    if ( status != PRO_TK_NO_ERROR ) {
	snprintf( astr, sizeof(astr), "Failed to get model for component %s",
		curr_part_name );
	(void)ProMessageDisplay(MSGFIL, "USER_ERROR", astr );
	ProMessageClear();
	fprintf( stderr, "%s\n", astr );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return status;
    }

    /* get its type (part or assembly are the only ones that should make it here) */
    status = ProMdlTypeGet( model, &type );
    if ( status != PRO_TK_NO_ERROR ) {
	snprintf( astr, sizeof(astr), "Failed to get type for component %s",
		curr_part_name );
	(void)ProMessageDisplay(MSGFIL, "USER_ERROR", astr );
	ProMessageClear();
	fprintf( stderr, "%s\n", astr );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return status;
    }

    /* remember the type */
    member->type = type;

    /* output this member */
    switch ( type ) {
	case PRO_MDL_ASSEMBLY:
	    output_assembly( model );
	    break;
	case PRO_MDL_PART:
	    if ( output_part( model ) == 2 ) {
		/* part had no solid parts, eliminate from the assembly */
		if ( prev ) {
		    prev->next = NULL;
		} else {
		    curr_assem->members = NULL;
		}
		bu_free( (char *)member, "asm member" );
	    }
	    break;
    }

    return PRO_TK_NO_ERROR;
}

/* this routine is a filter for the feature visit routine
 * selects only "component" items (should be only parts and assemblies)
 */
extern "C" ProError
assembly_filter( ProFeature *feat, ProAppData *data )
{
    ProFeattype type;
    ProFeatStatus feat_status;
    ProError status;

    status = ProFeatureTypeGet( feat, &type );
    if ( status != PRO_TK_NO_ERROR ) {
	sprintf( astr, "In assembly_filter, cannot get feature type for feature %d",
		feat->id );
	(void)ProMessageDisplay(MSGFIL, "USER_ERROR", astr );
	ProMessageClear();
	fprintf( stderr, "%s\n", astr );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return PRO_TK_CONTINUE;
    }

    if ( type != PRO_FEAT_COMPONENT ) {
	return PRO_TK_CONTINUE;
    }

    status = ProFeatureStatusGet( feat, &feat_status );
    if ( status != PRO_TK_NO_ERROR ) {
	sprintf( astr, "In assembly_filter, cannot get feature status for feature %d",
		feat->id );
	(void)ProMessageDisplay(MSGFIL, "USER_ERROR", astr );
	ProMessageClear();
	fprintf( stderr, "%s\n", astr );
	(void)ProWindowRefresh( PRO_VALUE_UNUSED );
	return PRO_TK_CONTINUE;
    }

    if ( feat_status != PRO_FEAT_ACTIVE ) {
	return PRO_TK_CONTINUE;
    }

    return PRO_TK_NO_ERROR;
}


/* routine to free the memory associated with our assembly info */
extern "C" void
free_assem( struct asm_head *curr_assem )
{
    struct asm_member *ptr, *tmp;

    if ( logger_type == LOGGER_TYPE_ALL ) {
	fprintf( logger, "Freeing assembly info\n" );
    }

    ptr = curr_assem->members;
    while ( ptr ) {
	tmp = ptr;
	ptr = ptr->next;
	bu_free( (char *)tmp, "asm member" );
    }
}

/* routine to list assembly info (for debugging) */
extern "C" void
list_assem( struct asm_head *curr_asm )
{
    struct asm_member *ptr;

    fprintf( stderr, "Assembly %s:\n", curr_asm->name );
    ptr = curr_asm->members;
    while ( ptr ) {
	fprintf( stderr, "\t%s\n", ptr->name );
	ptr = ptr->next;
    }
}

/* routine to output an assembly as a BRL-CAD combination
 * The combination will have the Pro/E name with a ".c" suffix.
 * Cannot just use the Pro/E name, because assembly can have the same name as a part.
 */
extern "C" void
output_assembly( ProMdl model )
{
    ProName asm_name;
    ProMassProperty mass_prop;
    ProError status;
    ProBoolean is_exploded;
    struct asm_head curr_assem;
    struct asm_member *member;
    int member_count=0;
    int i, j, k;
    int ret_status=0;
    
    if ( ProMdlNameGet( model, asm_name ) != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get model name for an assembly\n" );
	return;
    }

    /* do not output this assembly more than once */
    if ( already_done_asm( asm_name ) )
	return;

    if ( logger_type == LOGGER_TYPE_ALL ) {
	fprintf( logger, "Processing assembly %s:\n", ProWstringToString( astr, asm_name ) );
    }

    /* let the user know we are doing something */

    status = ProUILabelTextSet( "creo_brl", "curr_proc", asm_name );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to update dialog label for currently processed assembly\n" );
	return;
    }
#if 0
    status = ProUIDialogActivate( "creo_brl", &ret_status );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Error in creo-brl Dialog, error = %d\n",
		status );
	fprintf( stderr, "\t dialog returned %d\n", ret_status );
    }
#endif

bu_log("got here\n");
    /* everything starts out in "curr_part_name", copy name to "curr_asm_name" */
    bu_strlcpy( curr_asm_name, curr_part_name, sizeof(curr_asm_name) );

    /* start filling in assembly info */
    bu_strlcpy( curr_assem.name, curr_part_name, sizeof(curr_assem.name) );
    curr_assem.members = NULL;
    curr_assem.model = model;

    /* make sure this assembly is not "exploded"!!!
     * some careless designers leave assemblies in exploded mode
     */
#if 0
    status = ProAssemblyIsExploded(*(ProAssembly *)model, &is_exploded );
    if ( status != PRO_TK_NO_ERROR ) {
	fprintf( stderr, "Failed to get explode status of %s\n", curr_assem.name );
	if ( logger_type == LOGGER_TYPE_ALL ) {
	    fprintf( logger, "Failed to get explode status of %s\n", curr_assem.name );
	}
    }

    if ( is_exploded ) {
	/* unexplode this assembly !!!! */
	status = ProAssemblyUnexplode(*(ProAssembly *)model );
	if ( status != PRO_TK_NO_ERROR ) {
	    fprintf( stderr, "Failed to un-explode assembly %s\n", curr_assem.name );
	    fprintf( stderr, "\tcomponents will be incorrectly positioned\n" );
	    if ( logger_type == LOGGER_TYPE_ALL ) {
		fprintf( logger, "Failed to un-explode assembly %s\n", curr_assem.name );
		fprintf( logger, "\tcomponents will be incorrectly positioned\n" );
	    }
	}
    }
#endif

    /* use feature visit to get info about assembly members.
     * also calls output functions for members (parts or assemblies)
     */
    status = ProSolidFeatVisit( ProMdlToPart(model),
	    assembly_comp,
	    (ProFeatureFilterAction)assembly_filter,
	    (ProAppData)&curr_assem );

    /* output the accumulated assembly info */
    fprintf( outfp, "put {%s.c} comb region no tree ", get_brlcad_name( curr_assem.name ) );

    /* count number of members */
    member = curr_assem.members;
    while ( member ) {
	member_count++;
	member = member->next;
    }

    if ( logger_type == LOGGER_TYPE_ALL ) {
	fprintf( logger, "Output %d members of assembly\n", member_count );
    }

    /* output the "tree" */
    for ( i=1; i<member_count; i++ ) {
	fprintf( outfp, "{u ");
    }

    member = curr_assem.members;
    i = 0;
    while ( member ) {
	/* output the member name */
	if ( member->type == PRO_MDL_ASSEMBLY ) {
	    fprintf( outfp, "{l {%s.c}", get_brlcad_name( member->name ) );
	} else {
	    fprintf( outfp, "{l {%s}", get_brlcad_name( member->name ) );
	}

	/* if there is an xform matrix, put it here */
	if ( is_non_identity( member->xform ) ) {
	    fprintf( outfp, " {" );
	    for ( j=0; j<4; j++ ) {
		for ( k=0; k<4; k++ ) {
		    if ( k == 3 && j < 3 ) {
			fprintf( outfp, " %.12e",
				member->xform[k][j] * creo_to_brl_conv );
		    } else {
			fprintf( outfp, " %.12e",
				member->xform[k][j] );
		    }
		}
	    }
	    fprintf( outfp, "}" );
	}
	if ( i ) {
	    fprintf( outfp, "}} " );
	} else {
	    fprintf( outfp, "} " );
	}
	member = member->next;
	i++;
    }
    fprintf( outfp, "\n" );

    fprintf( outfp, "attr set {%s.c} %s %s\n",
	    get_brlcad_name( curr_assem.name ),
	    CREO_NAME_ATTR,
	    curr_assem.name );

    /* calculate mass properties */
    if ( logger_type == LOGGER_TYPE_ALL ) {
	fprintf( logger, "Getting mass properties for this assembly\n" );
    }

    status = ProSolidMassPropertyGet( ProMdlToSolid( model ), NULL, &mass_prop );
    if ( status == PRO_TK_NO_ERROR ) {
	if ( mass_prop.density > 0.0 ) {
	    fprintf( outfp, "attr set {%s.c} density %g\n",
		    get_brlcad_name( curr_assem.name ),
		    mass_prop.density );
	}
	if ( mass_prop.mass > 0.0 ) {
	    fprintf( outfp, "attr set {%s.c} mass %g\n",
		    get_brlcad_name( curr_assem.name ),
		    mass_prop.mass );
	}
	if ( mass_prop.volume > 0.0 ) {
	    fprintf( outfp, "attr set {%s.c} volume %g\n",
		    get_brlcad_name( curr_assem.name ),
		    mass_prop.volume );
	}
    }

    /* add this assembly to the list of already output objects */
    add_to_done_asm( asm_name );

    /* free the memory associated with this assembly */
    free_assem( &curr_assem );
}

extern "C" void
add_to_done_asm( wchar_t *name )
{
    wchar_t *name_copy;

    if ( logger_type == LOGGER_TYPE_ALL ) {
	fprintf( logger, "Added %s to list of done assemblies\n", ProWstringToString( astr, name ) );
    }

    if (done_list_asm.find(name) == done_list_part.end()) {
	name_copy = ( wchar_t *)bu_calloc( wcslen( name ) + 1, sizeof( wchar_t ),
		"asm name for done list" );
	wcsncpy( name_copy, name, wcslen(name)+1 );
	done_list_asm.insert(name_copy);
    }
}

extern "C" int
already_done_asm( wchar_t *name )
{
    if (done_list_asm.find(name) != done_list_asm.end()) {
	return 1;
    }
    return 0;
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