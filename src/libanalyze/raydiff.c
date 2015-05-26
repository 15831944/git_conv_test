/*                       R A Y D I F F . C
 * BRL-CAD
 *
 * Copyright (c) 2015 United States Government as represented by
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
/** @file raydiff.c
 *
 * Brief description
 *
 */
#include "common.h"

#include <string.h> /* for memset */

#include "vmath.h"
#include "bu/log.h"
#include "raytrace.h"

struct raydiff_container {
    struct rt_i *rtip;
    struct resource *resp;
    int ray_dir;
    int ncpus;
    fastf_t tol;
};

int
hit(struct application *ap, struct partition *PartHeadp, struct seg *UNUSED(segs))
{
    point_t in_pt, out_pt;
    struct partition *part;
    fastf_t part_len = 0.0;
    struct raydiff_container *state = (struct raydiff_container *)ap->a_uptr;
    /*rt_pr_seg(segs);*/
    /*rt_pr_partitions(ap->a_rt_i, PartHeadp, "hits");*/

    for (part = PartHeadp->pt_forw; part != PartHeadp; part = part->pt_forw) {
	VJOIN1(in_pt, ap->a_ray.r_pt, part->pt_inhit->hit_dist, ap->a_ray.r_dir);
	VJOIN1(out_pt, ap->a_ray.r_pt, part->pt_outhit->hit_dist, ap->a_ray.r_dir);
	part_len = DIST_PT_PT(in_pt, out_pt);
	if (part_len > state->tol)
	    bu_log("HIT (%s) (len: %f): %g %g %g -> %g %g %g\n", part->pt_regionp->reg_name, part_len, V3ARGS(in_pt), V3ARGS(out_pt));
    }

    return 0;
}

int
overlap(struct application *ap,
	struct partition *pp,
	struct region *reg1,
	struct region *reg2,
	struct partition *UNUSED(hp))
{
    point_t in_pt, out_pt;
    fastf_t overlap_len = 0.0;
    struct raydiff_container *state = (struct raydiff_container *)ap->a_uptr;

    VJOIN1(in_pt, ap->a_ray.r_pt, pp->pt_inhit->hit_dist, ap->a_ray.r_dir);
    VJOIN1(out_pt, ap->a_ray.r_pt, pp->pt_outhit->hit_dist, ap->a_ray.r_dir);

    overlap_len = DIST_PT_PT(in_pt, out_pt);

    if (overlap_len > state->tol)
	bu_log("OVERLAP (%s and %s) (len: %f): %g %g %g -> %g %g %g\n", reg1->reg_name, reg2->reg_name, overlap_len, V3ARGS(in_pt), V3ARGS(out_pt));

    return 0;
}

    int
miss(struct application *ap)
{
    RT_CK_APPLICATION(ap);

    return 0;
}

void
raydiff_gen_worker(int cpu, void *ptr)
{
    struct application ap;
    struct raydiff_container *state = (struct raydiff_container *)ptr;
    point_t max;
    /*
    point_t min;
    int i, j;
    int ymin, ymax;
    int dir1, dir2, dir3;
    fastf_t d[3];
    int n[3];
    */
    RT_APPLICATION_INIT(&ap);
    ap.a_rt_i = state->rtip;
    ap.a_hit = hit;
    ap.a_miss = miss;
    ap.a_overlap = overlap;
    ap.a_onehit = 0;
    ap.a_logoverlap = rt_silent_logoverlap;
    ap.a_resource = &state->resp[cpu];
    ap.a_uptr = (void *)ptr;

    /* get min and max points of bounding box */
    /*VMOVE(min, state->rtip->mdl_min);*/
    VMOVE(max, state->rtip->mdl_max);

    /* if the delta is greater than half the max-min distance, clamp to (max-min)/2 */
    /*for (i = 0; i < 3; i++) {
	n[i] = (max[i] - min[i])/state->delta;
	if(n[i] < 2) n[i] = 2;
	d[i] = (max[i] - min[i])/n[i];
    }

    dir1 = state->ray_dir;
    dir2 = (state->ray_dir+1)%3;
    dir3 = (state->ray_dir+2)%3;

    if (state->ncpus == 1) {
	ymin = 0;
	ymax = n[dir3];
    } else {
	ymin = n[dir3]/state->ncpus * (cpu - 1) + 1;
	ymax = n[dir3]/state->ncpus * (cpu);
	if (cpu == 1) ymin = 0;
	if (cpu == state->ncpus) ymax = n[dir3];
    }
    */

    bu_log("shoot in x axis\n");
    ap.a_ray.r_dir[0] = 1;
    ap.a_ray.r_dir[1] = 0;
    ap.a_ray.r_dir[2] = 0;
    VSET(ap.a_ray.r_pt, -1001, 0, 0);
    rt_shootray(&ap);

    bu_log("shoot in z axis\n");
    ap.a_ray.r_dir[0] = 0;
    ap.a_ray.r_dir[1] = 0;
    ap.a_ray.r_dir[2] = 1;
    VSET(ap.a_ray.r_pt, 0, 0, -1001);
    rt_shootray(&ap);
}


/* 0 = no difference within tolerance, 1 = difference >= tolerance */
int
analyze_raydiff(/* TODO - decide what to return.  Probably some sort of left, common, right segment sets.  See what rtcheck does... */
	struct db_i *dbip, const char *obj1, const char *obj2, struct bn_tol *tol)
{
    int i;
    int ncpus = bu_avail_cpus();
    /*int j, dir1;
    point_t min, max;*/
    struct raydiff_container *state;

    if (!dbip || !obj1 || !obj2 || !tol) return 0;

    BU_GET(state, struct raydiff_container);

    state->rtip = rt_new_rti(dbip);
    /*state->tol = tol->dist * 0.5;*/
    state->tol = 100;

    if (rt_gettree(state->rtip, obj1) < 0) return -1;
    if (rt_gettree(state->rtip, obj2) < 0) return -1;

    rt_prep_parallel(state->rtip, 1);

    ncpus = 1;
    state->resp = (struct resource *)bu_calloc(ncpus+1, sizeof(struct resource), "resources");
    for (i = 0; i < ncpus+1; i++) {
	rt_init_resource(&(state->resp[i]), i, state->rtip);
    }

    state->ncpus = ncpus;
    bu_parallel(raydiff_gen_worker, ncpus, (void *)state);
    return 0;
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
