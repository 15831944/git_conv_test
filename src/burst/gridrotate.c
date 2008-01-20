/*                    G R I D R O T A T E . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2008 United States Government as represented by
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
 *
 */
/** @file gridrotate.c
 *	Author:		Jeff Hanes
 *	Modified:	Gary S. Moss	(Added roll rotation.)
 */
#include <math.h>
#include "machine.h"
#include "./vecmath.h"

void	gridRotate();

/*
	void	gridRotate( fastf_t azim, fastf_t elev, fastf_t roll,
			register fastf_t *des_H, register fastf_t *des_V )

	Creates the unit vectors H and V which are the horizontal
	and vertical components of the grid in target coordinates.
	The vectors are found from the azimuth and elivation of the
	viewing angle according to a simplification of the rotation
	matrix from grid coordinates to target coordinates.
	To see that the vectors are, indeed, unit vectors, recall
	the trigonometric relation:

		sin( A )^2  +  cos( A )^2  =  1 .

 */
void
gridRotate( azim, elev, roll, des_H, des_V )
fastf_t	azim, elev, roll;
register fastf_t	*des_H, *des_V;
	{	fastf_t	sn_azm = sin( azim );
		fastf_t	cs_azm = cos( azim );
		fastf_t	sn_elv = sin( elev );
	des_H[0] = -sn_azm;
	des_H[1] =  cs_azm;
	des_H[2] =  0.0;
	des_V[0] = -sn_elv*cs_azm;
	des_V[1] = -sn_elv*sn_azm;
	des_V[2] =  cos( elev );

	if ( roll != 0.0 )
		{	fastf_t	tmp_V[3], tmp_H[3], prime_V[3];
			fastf_t	sn_roll = sin( roll );
			fastf_t	cs_roll = cos( roll );
		Scale2Vec( des_V, cs_roll, tmp_V );
		Scale2Vec( des_H, sn_roll, tmp_H );
		Add2Vec( tmp_V, tmp_H, prime_V );
		Scale2Vec( des_V, -sn_roll, tmp_V );
		Scale2Vec( des_H, cs_roll, tmp_H );
		Add2Vec( tmp_V, tmp_H, des_H );
		CopyVec( des_V, prime_V );
		}
	return;
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
