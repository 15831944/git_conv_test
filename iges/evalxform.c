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
#include "./iges_struct.h"
#include "./iges_extern.h"


/*	This routine evaluates the transformation matrix list
	for each transformation matrix entity */

struct list /* simple linked list for remembering a matrix sequence */
{
	int index; /* index to "dir" array for a xform matrix */
	struct list *prev; /* link to previous xform matrix */
};

Evalxform()
{

	int i,j,xform;
	struct list *ptr,*ptr1;
	mat_t rot;

	ptr = NULL;
	ptr1 = NULL;

	for( i=0 ; i<totentities ; i++ ) /* loop through all entities */
	{
		/* skip non-transformation entities */
		if( dir[i]->type != 124 && dir[i]->type != 700 )
			continue;

		if( dir[i]->trans && !dir[i]->referenced )
		{
			/* Make a linked list of the xform matrices
				in reverse order */
			xform = i;
			while( xform )
			{
				if( ptr == NULL )
					ptr = (struct list *)malloc( sizeof( struct list ) );
				else
				{
					ptr1 = ptr;
					ptr = (struct list *)malloc( sizeof( struct list ) );
				}
				ptr->prev = ptr1;
				ptr->index = xform;
				xform = dir[xform]->trans;
			}

			for( j=0 ; j<16 ; j++ )
				rot[j] = (*identity)[j];

			while( ptr != NULL )
			{
				if( !dir[ptr->index]->referenced )
				{
					Matmult( rot , *dir[ptr->index]->rot , rot );
					for( j=0 ; j<16 ; j++ )
						(*dir[ptr->index]->rot)[j] = rot[j];
					dir[ptr->index]->referenced++;
				}
				else
				{
					for( j=0 ; j<16 ; j++ )
					rot[j] = (*dir[ptr->index]->rot)[j];
				}
				ptr = ptr->prev;
			}
		}
	}

	/* set matrices for all other entities */
	for( i=0 ; i<totentities ; i++ )
	{
		/* skip xform entities */
		if( dir[i]->type == 124 || dir[i]->type == 700 )
			continue;

		if( dir[i]->trans )
			dir[i]->rot = dir[dir[i]->trans]->rot;
		else
			dir[i]->rot = identity;
	}
}
