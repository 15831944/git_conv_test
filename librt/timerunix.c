/*
 *			T I M E R U N I X . C
 *
 * Function -
 *	To provide timing information for RT.
 *	This version for any non-BSD UNIX system, including
 *	System III, Vr1, Vr2.
 *	Version 6 & 7 should also be able to use this (untested).
 *	The time() and times() sys-calls are used for all timing.
 *
 *  Author -
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
static char RCStimer[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>

#ifdef CRAY1
# include <sys/machd.h>		/* XMP only, for HZ */
#endif

#ifndef HZ
	/* It's not always in sys/param.h;  if not, guess */
#	define	HZ		60
#	define	DEFAULT_HZ	yes
#endif

#include "machine.h"
#include "rtstring.h"

/* Standard System V stuff */
extern long time();
static long time0;
static struct tms tms0;

/*
 *			R T _ P R E P _ T I M E R
 */
void
rt_prep_timer()
{
	(void)time(&time0);
	(void)times(&tms0);
}

/*
 *			R T _ G E T _ T I M E R
 *
 *  Reports on the passage of time, since rt_prep_timer() was called.
 *  Explicit return is number of CPU seconds.
 *  String return is descriptive.
 *  If "elapsed" pointer is non-null, number of elapsed seconds are returned.
 *  Times returned will never be zero.
 */
double
rt_get_timer( vp, elapsed )
struct rt_vls	*vp;
double		*elapsed;
{
	long	now;
	double	user_cpu_secs;
	double	sys_cpu_secs;
	double	elapsed_secs;
	double	percent;
	struct tms tmsnow;

	/* Real time.  1 second resolution. */
	(void)time(&now);
	elapsed_secs = now-time0;

	/* CPU time */
	(void)times(&tmsnow);
	user_cpu_secs = (tmsnow.tms_utime + tmsnow.tms_cutime) - 
		(tms0.tms_utime + tms0.tms_cutime );
	user_cpu_secs /= HZ;
	sys_cpu_secs = (tmsnow.tms_stime + tmsnow.tms_cstime) - 
		(tms0.tms_stime + tms0.tms_cstime );
	sys_cpu_secs /= HZ;
	if( user_cpu_secs < 0.00001 )  user_cpu_secs = 0.00001;
	if( elapsed_secs < 0.00001 )  elapsed_secs = user_cpu_secs;	/* It can't be any less! */

	if( elapsed )  *elapsed = elapsed_secs;

	if( vp )  {
		percent = user_cpu_secs/elapsed_secs*100.0;
		RT_VLS_CHECK(vp);
#ifdef DEFAULT_HZ
		rt_vls_printf( vp,
"%g user + %g sys in %g elapsed secs (%g%%) WARNING: HZ=60 assumed, fix librt/timerunix.c",
			user_cpu_secs, sys_cpu_secs, elapsed_secs, percent );
#else
		rt_vls_printf( vp,
			"%g user + %g sys in %g elapsed secs (%g%%)",
			user_cpu_secs, sys_cpu_secs, elapsed_secs, percent );
#endif
	}
	return( user_cpu_secs );
}

/*
 *			R T _ R E A D _ T I M E R
 * 
 *  Compatability routine
 */
double
rt_read_timer(str,len)
char *str;
{
	struct rt_vls	vls;
	double		cpu;
	int		todo;

	if( !str )  return  rt_get_timer( (struct rt_vls *)0, (double *)0 );

	rt_vls_init( &vls );
	cpu = rt_get_timer( &vls, (double *)0 );
	todo = rt_vls_strlen( &vls );
	if( todo > len )  todo = len-1;
	strncpy( str, rt_vls_addr(&vls), todo );
	str[todo] = '\0';
	return cpu;
}
