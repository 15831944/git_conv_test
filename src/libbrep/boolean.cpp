/*                  B O O L E A N . C P P
 * BRL-CAD
 *
 * Copyright (c) 2013 United States Government as represented by
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
/** @file boolean.cpp
 *
 * Evaluate NURBS booleans (union, intersection and difference).
 *
 */

#include "common.h"

#include <assert.h>
#include <vector>
#include <stack>
#include <algorithm>
#include "bio.h"

#include "vmath.h"
#include "bu.h"

#include "brep.h"
#include "raytrace.h"

#define DEBUG_BREP_BOOLEAN 1
#define USE_CONNECTIVITY_GRAPH 1
#define INTERSECTION_TOL 0.001


struct IntersectPoint {
    ON_3dPoint m_pt;	// 3D intersection point
    int m_seg;		// which curve of the loop
    double m_t;		// param on the loop curve
    int m_type;		// which intersection curve
    int m_rank;		// rank on the chain
    double m_t_for_rank;// param on the SSI curve
    enum {
	UNSET,
	IN,
	OUT
    } m_in_out;		// dir is going inside/outside
    int m_pos;		// between curve[m_pos] and curve[m_pos+1]
			// after the outerloop is split
};


struct TrimmedFace {
    ON_SimpleArray<ON_Curve*> outerloop;
    std::vector<ON_SimpleArray<ON_Curve*> > innerloop;
    const ON_BrepFace *face;
#if USE_CONNECTIVITY_GRAPH
    // Connectivity graph support
    ON_SimpleArray<TrimmedFace*> neighbors;
    // which parts of its parent's outerloop are used, decribed in multiple
    // pairs of IntersectPoints (multiple intervals)
    ON_ClassArray<std::pair<IntersectPoint, IntersectPoint> > parts;
#endif
    TrimmedFace *Duplicate() const
    {
	TrimmedFace *out = new TrimmedFace();
	out->face = face;
	out->outerloop = outerloop;
	out->innerloop = innerloop;
#if USE_CONNECTIVITY_GRAPH
	out->parts = parts;
	// Don't copy the neighbors
#endif
	return out;
    }
};


HIDDEN int
compare_t(const IntersectPoint* a, const IntersectPoint* b)
{
    if (a->m_seg != b->m_seg)
	return a->m_seg - b->m_seg;
    return a->m_t > b->m_t ? 1 : (a->m_t < b->m_t ? -1 : 0);
}


HIDDEN int
compare_for_rank(IntersectPoint* const *a, IntersectPoint* const *b)
{
    return (*a)->m_t_for_rank > (*b)->m_t_for_rank ? 1 : ((*a)->m_t_for_rank < (*b)->m_t_for_rank ? -1 : 0);
}


HIDDEN void
AppendToPolyCurve(ON_Curve* curve, ON_PolyCurve& polycurve)
{
    // use this function rather than ON_PolyCurve::Append() to avoid
    // getting nested polycurves, which makes ON_Brep::IsValid() to fail.

    ON_PolyCurve* nested = ON_PolyCurve::Cast(curve);
    if (nested != NULL) {
	// The input curve is a polycurve
	const ON_CurveArray& segments = nested->SegmentCurves();
	for (int i = 0; i < segments.Count(); i++)
	    AppendToPolyCurve(segments[i], polycurve);
    } else
	polycurve.Append(curve);
}


HIDDEN bool
IsLoopValid(const ON_SimpleArray<ON_Curve*>& loop, double tolerance, ON_PolyCurve* polycurve = NULL)
{
    bool delete_curve = false;
    bool ret = true;

    if (loop.Count() == 0) {
	bu_log("The input loop is empty.\n");
	ret = false;
    }

    // First, use a ON_PolyCurve to represent the loop.
    if (ret) {
	if (polycurve == NULL) {
	    polycurve = new ON_PolyCurve;
	    if (polycurve)
		delete_curve = true;
	    else
		ret = false;
	}
    }

    // Check the loop is continuous and closed or not.
    if (ret) {
	AppendToPolyCurve(loop[0]->Duplicate(), *polycurve);
	for (int i = 1 ; i < loop.Count(); i++) {
	    if (loop[i] && loop[i - 1] && loop[i]->PointAtStart().DistanceTo(loop[i-1]->PointAtEnd()) < ON_ZERO_TOLERANCE)
		AppendToPolyCurve(loop[i]->Duplicate(), *polycurve);
	    else {
		bu_log("The input loop is not continuous.\n");
		ret = false;
	    }
	}
    }
    if (ret && polycurve->PointAtStart().DistanceTo(polycurve->PointAtEnd()) >= ON_ZERO_TOLERANCE) {
	bu_log("The input loop is not closed.\n");
	ret = false;
    }

    if (ret) {
	// Check whether the loop is degenerated.
	ON_BoundingBox bbox = polycurve->BoundingBox();
	ret = !ON_NearZero(bbox.Diagonal().Length(), tolerance)
	      && !polycurve->IsLinear(tolerance);
    }

    if (delete_curve)
	delete polycurve;

    return ret;
}


HIDDEN int
IsPointInsideLoop(const ON_2dPoint& pt, const ON_SimpleArray<ON_Curve*>& loop)
{
    // returns:
    //   -1: the input is not a valid loop
    //   0:  the point is not inside the loop or on boundary
    //   1:  the point is inside the loop or on boundary
    // note:
    //   If you want to know whether this point is on boundary, please call
    //   IsPointOnLoop().

    ON_PolyCurve polycurve;
    if (!IsLoopValid(loop, ON_ZERO_TOLERANCE, &polycurve))
	return -1;

    ON_BoundingBox bbox = polycurve.BoundingBox();
    if (!bbox.IsPointIn(pt))
	return 0;

    // The input point is inside the loop's bounding box.
    // out must be outside the closed region (and the bbox).
    ON_2dPoint out = pt + ON_2dVector(bbox.Diagonal());
    ON_LineCurve linecurve(pt, out);
    ON_SimpleArray<ON_X_EVENT> x_event;
    ON_Intersect(&linecurve, &polycurve, x_event, INTERSECTION_TOL);
    int count = x_event.Count();
    for (int i = 0; i < x_event.Count(); i++) {
	// Find tangent intersections.
	// What should we do if it's ccx_overlap?
	if (polycurve.TangentAt(x_event[i].m_a[0]).IsParallelTo(linecurve.m_line.Direction()))
	    count++;
    }

    return count % 2 ? 1 : 0;
}


HIDDEN int
IsPointOnLoop(const ON_2dPoint& pt, const ON_SimpleArray<ON_Curve*>& loop)
{
    // returns:
    //   -1: the input is not a valid loop
    //   0:  the point is not on the boundary of the loop
    //   1:  the point is on the boundary of the loop

    ON_PolyCurve polycurve;
    if (!IsLoopValid(loop, ON_ZERO_TOLERANCE, &polycurve))
	return -1;

    ON_ClassArray<ON_PX_EVENT> px_event;
    return ON_Intersect(ON_3dPoint(pt), polycurve, px_event, INTERSECTION_TOL) ? 1 : 0;
}


HIDDEN int
get_subcurve_inside_faces(const ON_Brep* brep1, const ON_Brep* brep2, int fi1, int fi2, ON_SSX_EVENT* Event)
{
    // The ON_SSX_EVENT from SSI is the intersection of two whole surfaces.
    // We need to get the part that lies inside both trimmed patches.
    // (brep1's face[fi1] and brep2's face[fi2])
    // returns 0 for success, -1 for error.

    if (Event == NULL)
	return -1;

    if (Event->m_curve3d == NULL || Event->m_curveA == NULL || Event->m_curveB == NULL)
	return -1;

    // 1. Get the outerloops.
    ON_SimpleArray<ON_Curve*> outerloop1, outerloop2;
    if (fi1 < 0 || fi1 >= brep1->m_F.Count()) {
	bu_log("get_subcurve_inside_faces(): invalid fi1 (%d).\n", fi1);
	return -1;
    }
    if (fi2 < 0 || fi2 >= brep2->m_F.Count()) {
	bu_log("get_subcurve_inside_faces(): invalid fi2 (%d).\n", fi2);
	return -1;
    }
    const ON_BrepLoop& loop1 = brep1->m_L[brep1->m_F[fi1].m_li[0]];
    const ON_BrepLoop& loop2 = brep2->m_L[brep2->m_F[fi2].m_li[0]];

    for (int i = 0; i < loop1.TrimCount(); i++) {
	outerloop1.Append(brep1->m_C2[brep1->m_T[loop1.m_ti[i]].m_c2i]);
    }
    for (int i = 0; i < loop2.TrimCount(); i++) {
	outerloop2.Append(brep2->m_C2[brep2->m_T[loop2.m_ti[i]].m_c2i]);
    }

    // 2.1 Intersect the curves in Event with outerloop1, and get the parts
    // inside. (Represented with param intervals on the curve's domain [0, 1])
    ON_SimpleArray<double> divT;
    ON_SimpleArray<ON_Interval> intervals;
    ON_SimpleArray<ON_X_EVENT> x_event1, x_event2;
    for (int i = 0; i < outerloop1.Count(); i++)
	ON_Intersect(Event->m_curveA, outerloop1[i], x_event1, INTERSECTION_TOL);
    for (int i = 0; i < x_event1.Count(); i++) {
	divT.Append(x_event1[i].m_a[0]);
	if (x_event1[i].m_type == ON_X_EVENT::ccx_overlap)
	    divT.Append(x_event1[i].m_a[1]);
    }
    divT.QuickSort(ON_CompareIncreasing);
    if (divT.Count() != 0) {
	if (!ON_NearZero(divT[0] - Event->m_curveA->Domain().Min()))
	    divT.Insert(0, Event->m_curveA->Domain().Min());
	if (!ON_NearZero(*divT.Last() - Event->m_curveA->Domain().Max()))
	    divT.Append(Event->m_curveA->Domain().Max());
	for (int i = 0; i < divT.Count() - 1; i++) {
	    ON_Interval interval(divT[i], divT[i + 1]);
	    if (ON_NearZero(interval.Length()))
		continue;
	    ON_2dPoint pt = Event->m_curveA->PointAt(interval.Mid());
	    if (!IsPointOnLoop(pt, outerloop1) && IsPointInsideLoop(pt, outerloop1))
		intervals.Append(interval);
	}
    }

    // 2.2 Intersect the curves in Event with outerloop2, and get the parts
    // inside. (Represented with param intervals on the curve's domain [0, 1])
    divT.Empty();
    for (int i = 0; i < outerloop2.Count(); i++)
	ON_Intersect(Event->m_curveB, outerloop2[i], x_event2, INTERSECTION_TOL);
    for (int i = 0; i < x_event2.Count(); i++) {
	divT.Append(x_event2[i].m_a[0]);
	if (x_event2[i].m_type == ON_X_EVENT::ccx_overlap)
	    divT.Append(x_event2[i].m_a[1]);
    }
    divT.QuickSort(ON_CompareIncreasing);
    if (divT.Count() != 0) {
	if (!ON_NearZero(divT[0] - Event->m_curveB->Domain().Min()))
	    divT.Insert(0, Event->m_curveB->Domain().Min());
	if (!ON_NearZero(*divT.Last() - Event->m_curveB->Domain().Max()))
	    divT.Append(Event->m_curveB->Domain().Max());
	for (int i = 0; i < divT.Count() - 1; i++) {
	    ON_Interval interval(divT[i], divT[i + 1]);
	    if (ON_NearZero(interval.Length()))
		continue;
	    ON_2dPoint pt = Event->m_curveB->PointAt(interval.Mid());
	    if (!IsPointOnLoop(pt, outerloop2) && IsPointInsideLoop(pt, outerloop2)) {
		// According to openNURBS's definition, the domain of m_curve3d,
		// m_curveA, m_curveB in an ON_SSX_EVENT should be the same.
		// (See ON_SSX_EVENT::IsValid().
		// So we don't need to pull the interval back to A
		intervals.Append(interval);
	    }
	}
    }

    // 3. Merge the intervals and get the final result.
    ON_Interval merged_interval;
    for (int i = 0; i < intervals.Count(); i++) {
	merged_interval.Union(intervals[i]);
    }

    if (!merged_interval.IsValid())
	return -1;

    if (DEBUG_BREP_BOOLEAN)
	bu_log("merge_interval: [%g, %g]\n", merged_interval.Min(), merged_interval.Max());

    // 4. Replace with the sub-curves.
    Event->m_curve3d = sub_curve(Event->m_curve3d, merged_interval.Min(), merged_interval.Max());
    Event->m_curveA = sub_curve(Event->m_curveA, merged_interval.Min(), merged_interval.Max());
    Event->m_curveB = sub_curve(Event->m_curveB, merged_interval.Min(), merged_interval.Max());

    if (Event->m_curve3d == NULL || Event->m_curveA == NULL || Event->m_curveB == NULL)
	return -1;
    return 0;
}


HIDDEN void
link_curves(const ON_SimpleArray<ON_Curve*>& in, ON_SimpleArray<ON_Curve*>& out)
{
    // There might be two reasons why we need to link these curves.
    // 1) They are from intersections with two different surfaces.
    // 2) They are not continuous in the other surface's UV domain.

    ON_SimpleArray<ON_Curve*> tmp;
    for (int i = 0; i < in.Count(); i++) {
	tmp.Append(in[i]->Duplicate());
    }

    // As usual, we use a greedy approach.
    for (int i = 0; i < tmp.Count(); i++) {
	for (int j = 0; j < tmp.Count(); j++) {
	    if (tmp[i] == NULL || tmp[i]->IsClosed())
		break;

	    if (tmp[j] == NULL || tmp[j]->IsClosed() || j == i)
		continue;

	    ON_Curve *c1 = NULL, *c2 = NULL;
	    double dis;
	    // Link curves that share an end point.
	    if ((dis = tmp[i]->PointAtStart().DistanceTo(tmp[j]->PointAtEnd())) < INTERSECTION_TOL) {
		// end -- start -- end -- start
		c1 = tmp[j];
		c2 = tmp[i];
	    } else if ((dis = tmp[i]->PointAtEnd().DistanceTo(tmp[j]->PointAtStart())) < INTERSECTION_TOL) {
		// start -- end -- start -- end
		c1 = tmp[i];
		c2 = tmp[j];
	    } else if ((dis = tmp[i]->PointAtStart().DistanceTo(tmp[j]->PointAtStart())) < INTERSECTION_TOL) {
		// end -- start -- start -- end
		if (tmp[i]->Reverse()) {
		    c1 = tmp[i];
		    c2 = tmp[j];
		}
	    } else if ((dis = tmp[i]->PointAtEnd().DistanceTo(tmp[j]->PointAtEnd())) < INTERSECTION_TOL) {
		// start -- end -- end -- start
		if (tmp[j]->Reverse()) {
		    c1 = tmp[i];
		    c2 = tmp[j];
		}
	    } else
		continue;

	    if (c1 != NULL && c2 != NULL) {
		ON_PolyCurve* polycurve = new ON_PolyCurve;
		AppendToPolyCurve(c1, *polycurve);
		if (dis > ON_ZERO_TOLERANCE)
		    AppendToPolyCurve(new ON_LineCurve(c1->PointAtEnd(), c2->PointAtStart()), *polycurve);
		AppendToPolyCurve(c2, *polycurve);
		tmp[i] = polycurve;
		tmp[j] = NULL;
	    }

	    // Check whether tmp[i] is closed within a tolerance
	    if (tmp[i]->PointAtStart().DistanceTo(tmp[i]->PointAtEnd()) < INTERSECTION_TOL && !tmp[i]->IsClosed()) {
		// make IsClosed() true
		c1 = tmp[i];
		c2 = new ON_LineCurve(tmp[i]->PointAtEnd(), tmp[i]->PointAtStart());
		ON_PolyCurve* polycurve = new ON_PolyCurve;
		AppendToPolyCurve(c1, *polycurve);
		AppendToPolyCurve(c2, *polycurve);
		tmp[i] = polycurve;
	    }
	}
    }

    // Append the remaining curves to out.
    for (int i = 0; i < tmp.Count(); i++)
	if (tmp[i] != NULL)
	    out.Append(tmp[i]);

    if (DEBUG_BREP_BOOLEAN)
	bu_log("link_curves(): %d curves remaining.\n", out.Count());
}


HIDDEN int
split_trimmed_face(ON_SimpleArray<TrimmedFace*> &out, const TrimmedFace *in, const ON_SimpleArray<ON_Curve*> &curves)
{
    /* We followed the algorithms described in:
     * S. Krishnan, A. Narkhede, and D. Manocha. BOOLE: A System to Compute
     * Boolean Combinations of Sculptured Solids. Technical Report TR95-008,
     * Department of Computer Science, University of North Carolina, 1995.
     * Appendix B: Partitioning a Simple Polygon using Non-Intersecting
     * Chains.
     */

    if (curves.Count() == 0) {
	// No curve, no splitting
	out.Append(in->Duplicate());
	return 0;
    }

    ON_SimpleArray<IntersectPoint> intersect;
    ON_SimpleArray<bool> have_intersect(curves.Count());
    for (int i = 0; i < curves.Count(); i++)
	have_intersect[i] = false;

    for (int i = 0; i < in->outerloop.Count(); i++) {
	for (int j = 0; j < curves.Count(); j++) {
	    ON_SimpleArray<ON_X_EVENT> x_event;
	    ON_Intersect(in->outerloop[i], curves[j], x_event, INTERSECTION_TOL);
	    for (int k = 0; k < x_event.Count(); k++) {
		IntersectPoint tmp_pt;
		tmp_pt.m_pt = x_event[k].m_A[0];
		tmp_pt.m_seg = i;
		tmp_pt.m_t = x_event[k].m_a[0];
		tmp_pt.m_type = j;
		tmp_pt.m_t_for_rank = x_event[k].m_b[0];
		intersect.Append(tmp_pt);
		if (x_event[k].m_type == ON_X_EVENT::ccx_overlap) {
		    tmp_pt.m_pt = x_event[k].m_A[1];
		    // tmp_pt.m_seg = i;
		    tmp_pt.m_t = x_event[k].m_a[1];
		    // tmp_pt.m_type = j;
		    tmp_pt.m_t_for_rank = x_event[k].m_b[1];
		    intersect.Append(tmp_pt);
		}
	    }
	    if (x_event.Count())
		have_intersect[j] = true;
	}
    }

    // deal with the situations where there is no intersection
    std::vector<ON_SimpleArray<ON_Curve*> > innerloops;
    for (int i = 0; i < curves.Count(); i++) {
	if (!have_intersect[i]) {
	    // The start point cannot be on the boundary of the loop, because
	    // there is no intersections between curves[i] and the loop.
	    if (IsPointInsideLoop(curves[i]->PointAtStart(), in->outerloop)) {
		if (curves[i]->IsClosed()) {
		    ON_SimpleArray<ON_Curve*> iloop;
		    iloop.Append(curves[i]);
		    innerloops.push_back(iloop);
		    TrimmedFace *newface = new TrimmedFace();
		    newface->face = in->face;
		    newface->outerloop.Append(curves[i]);
#if USE_CONNECTIVITY_GRAPH
		    // It doesn't share its parent's outerloop
		    newface->parts.Empty();
#endif
		    out.Append(newface);
		}
	    }
	}
    }

    // rank these intersection points
    // during the time using pts_on_curves(), must make sure that
    // the capacity of intersect[] never change.
    ON_SimpleArray<ON_SimpleArray<IntersectPoint*>*> pts_on_curves(curves.Count());
    for (int i = 0; i < curves.Count(); i++)
	pts_on_curves[i] = new ON_SimpleArray<IntersectPoint*>();
    for (int i = 0; i < intersect.Count(); i++) {
	pts_on_curves[intersect[i].m_type]->Append(&(intersect[i]));
    }
    for (int i = 0; i < curves.Count(); i++) {
	pts_on_curves[i]->QuickSort(compare_for_rank);
	for (int j = 0; j < pts_on_curves[i]->Count(); j++)
	    (*pts_on_curves[i])[j]->m_rank = j;
    }
    // Determine whether it's going inside or outside.
    for (int i = 0; i < curves.Count(); i++) {
	for (int j = 0;  j < pts_on_curves[i]->Count(); j++) {
	    IntersectPoint* ipt = (*pts_on_curves[i])[j];
	    if (pts_on_curves[i]->Count() < 2)
		ipt->m_in_out = IntersectPoint::UNSET;
	    else {
		ON_3dPoint left = j == 0 ? curves[i]->PointAtStart() :
		    curves[i]->PointAt((ipt->m_t_for_rank+(*pts_on_curves[i])[j-1]->m_t_for_rank)*0.5);
		ON_3dPoint right = j == pts_on_curves[i]->Count() - 1 ? curves[i]->PointAtEnd() :
		    curves[i]->PointAt((ipt->m_t_for_rank+(*pts_on_curves[i])[j+1]->m_t_for_rank)*0.5);
		// If the point is on the boundary, we treat it with the same
		// way as it's outside.
		// For example, the left side is inside, and the right's on
		// boundary, that point should be IntersectPoint::OUT, the
		// same as the right's outside the loop.
		// Other cases are similar.
		int left_in = IsPointInsideLoop(left, in->outerloop) && !IsPointOnLoop(left, in->outerloop);
		int right_in = IsPointInsideLoop(right, in->outerloop) && !IsPointOnLoop(right, in->outerloop);
		if (left_in < 0 || right_in < 0) {
		    // not a loop
		    ipt->m_in_out = IntersectPoint::UNSET;
		    continue;
		}
		if (j == 0 && ON_NearZero(ipt->m_t_for_rank - curves[i]->Domain().Min())) {
		    ipt->m_in_out = right_in ? IntersectPoint::IN : IntersectPoint::OUT;
		} else if (j == pts_on_curves[i]->Count() - 1 && ON_NearZero(ipt->m_t_for_rank - curves[i]->Domain().Max())) {
		    ipt->m_in_out = left_in ? IntersectPoint::OUT : IntersectPoint::IN;
		} else {
		    if (left_in && right_in) {
			// tangent point, both sides in, duplicate that point
			intersect.Append(*ipt);
			intersect.Last()->m_in_out = IntersectPoint::IN;
			intersect.Last()->m_rank = ipt->m_rank+1;
			for (int k = j + 1; k < pts_on_curves[i]->Count(); k++)
			    (*pts_on_curves[i])[k]->m_rank++;
			ipt->m_in_out = IntersectPoint::OUT;
		    } else if (!left_in && !right_in) {
			// tangent point, both sides out, useless
			ipt->m_in_out = IntersectPoint::UNSET;
		    } else if (left_in && !right_in) {
			// transversal point, going outside
			ipt->m_in_out = IntersectPoint::OUT;
		    } else {
			// transversal point, going inside
			ipt->m_in_out = IntersectPoint::IN;
		    }
		}
	    }
	    if (DEBUG_BREP_BOOLEAN)
		bu_log("[%d][%d] in_out = %d\n", i, j, ipt->m_in_out);
	}
    }
    for (int i = 0; i < curves.Count(); i++) {
	delete pts_on_curves[i];
    }

    intersect.QuickSort(compare_t);

    // Split the outer loop.
    ON_SimpleArray<ON_Curve*> outerloop;    // segments of the outerloop
#if USE_CONNECTIVITY_GRAPH
    // the start point and end point of an outerloop segment
    // outerloop_start_end[] and outerloop[] should have the same size, and
    // outerloop_start_end[i] should be corresponding to outerloop[i] exactly.
    ON_ClassArray<std::pair<IntersectPoint, IntersectPoint> > outerloop_start_end;
#endif
    int isect_iter = 0;
    for (int i = 0; i < in->outerloop.Count(); i++) {
	ON_Curve *curve_on_loop = in->outerloop[i]->Duplicate();
	if (curve_on_loop == NULL) {
	    bu_log("ON_Curve::Duplicate() failed.\n");
	    continue;
	}
	for (; isect_iter < intersect.Count() && intersect[isect_iter].m_seg == i; isect_iter++) {
	    const IntersectPoint& isect_pt = intersect[isect_iter];
	    ON_Curve* left = NULL;
	    bool split_called = false;
	    if (curve_on_loop) {
		if (ON_NearZero(isect_pt.m_t - curve_on_loop->Domain().Max())) {
		    // Don't call Split(), which may fail when the point is
		    // at the ends.
		    left = curve_on_loop;
		    curve_on_loop = NULL;
		} else if (!ON_NearZero(isect_pt.m_t - curve_on_loop->Domain().Min())) {
		    curve_on_loop->Split(isect_pt.m_t, left, curve_on_loop);
		    split_called = true;
		}
	    }
	    if (left != NULL) {
		outerloop.Append(left);
#if USE_CONNECTIVITY_GRAPH
		IntersectPoint start;
		if (outerloop_start_end.Count() == 0 || outerloop_start_end.Last()->second.m_seg != i) {
		    // It should use the start point of in->outerloop[i]
		    start.m_seg = i;
		    start.m_t = in->outerloop[i]->Domain().Min();
		} else {
		    // Continuous to the last one
		    start = outerloop_start_end.Last()->second;
		}
		outerloop_start_end.Append(std::make_pair(start, isect_pt));
#endif
	    } else if (split_called) {
		bu_log("Split failed.\n");
		if (curve_on_loop) {
		    bu_log("Domain: [%lf, %lf]\n", curve_on_loop->Domain().Min(), curve_on_loop->Domain().Max());
		    bu_log("m_t: %lf\n", isect_pt.m_t);
		}
	    }
	    intersect[isect_iter].m_pos = outerloop.Count() - 1;
	}
	if (curve_on_loop) {
	    outerloop.Append(curve_on_loop);
#if USE_CONNECTIVITY_GRAPH
	    IntersectPoint start, end;
	    if (outerloop_start_end.Count() == 0 || outerloop_start_end.Last()->second.m_seg != i) {
		// It should use the start point of in->outerloop[i]
		start.m_seg = i;
		start.m_t = in->outerloop[i]->Domain().Min();
	    } else {
		// Continuous to the last one
		start = outerloop_start_end.Last()->second;
	    }
	    end.m_seg = i;
	    end.m_t = in->outerloop[i]->Domain().Max();
	    outerloop_start_end.Append(std::make_pair(start, end));
#endif
	}
    }

    // Append the first element at the last to handle some special cases.
    if (intersect.Count()) {
	intersect.Append(intersect[0]);
	intersect.Last()->m_seg += in->outerloop.Count();
	for (int i = 0; i <= intersect[0].m_pos; i++) {
	    ON_Curve* dup = outerloop[i]->Duplicate();
	    if (dup != NULL) {
		outerloop.Append(dup);
	    }
	    else {
		bu_log("ON_Curve::Duplicate() failed.\n");
		outerloop.Append(outerloop[i]);
	    }
#if USE_CONNECTIVITY_GRAPH
	    outerloop_start_end.Append(outerloop_start_end[i]);
#endif
	}
	intersect.Last()->m_pos = outerloop.Count() - 1;
    }

    std::stack<int> s;

    for (int i = 0; i < intersect.Count(); i++) {
#if USE_CONNECTIVITY_GRAPH
	// Check the validity of outerloop_start_end
	if (outerloop_start_end.Count() != outerloop.Count()) {
	    bu_log("split_trimmed_face() Error [i = %d]: outerloop_start_end.Count() != outerloop.Count()\n", i);
	    return -1;
	}
#endif

	// Ignore UNSET IntersectPoints.
	if (intersect[i].m_in_out == IntersectPoint::UNSET)
	    continue;

	if (s.empty()) {
	    s.push(i);
	    continue;
	}

	const IntersectPoint& p = intersect[s.top()];
	const IntersectPoint& q = intersect[i];

	if (compare_t(&p, &q) > 0 || q.m_pos < p.m_pos) {
	    bu_log("stack error or sort failure.\n");
	    bu_log("s.top() = %d, i = %d\n", s.top(), i);
	    bu_log("p->m_pos = %d, q->m_pos = %d\n", p.m_pos, q.m_pos);
	    bu_log("p->m_seg = %d, q->m_seg = %d\n", p.m_seg, q.m_seg);
	    bu_log("p->m_t = %g, q->m_t = %g\n", p.m_t, q.m_t);
	    continue;
	}
	if (q.m_type != p.m_type) {
	    s.push(i);
	    continue;
	} else if (q.m_rank - p.m_rank == 1 && q.m_in_out == IntersectPoint::OUT && p.m_in_out == IntersectPoint::IN) {
	    s.pop();
	} else if (p.m_rank - q.m_rank == 1 && p.m_in_out == IntersectPoint::OUT && q.m_in_out == IntersectPoint::IN) {
	    s.pop();
	} else {
	    s.push(i);
	    continue;
	}

	// need to form a new loop
	ON_SimpleArray<ON_Curve*> newloop;
#if USE_CONNECTIVITY_GRAPH
	ON_ClassArray<std::pair<IntersectPoint, IntersectPoint> > newloop_start_end;
#endif
	int curve_count = q.m_pos - p.m_pos;
	for (int j = p.m_pos + 1; j <= q.m_pos; j++) {
	    newloop.Append(outerloop[j]);
#if USE_CONNECTIVITY_GRAPH
	    newloop_start_end.Append(outerloop_start_end[j]);
#endif
	}

	if (p.m_type != q.m_type) {
	    bu_log("Error: p->type != q->type\n");
	    continue;
	}

	// The curves on the outer loop is from p to q, so the curves on the
	// SSI curve should be from q to p (to form a loop)
	double t1 = p.m_t_for_rank, t2 = q.m_t_for_rank;
	bool need_reverse = true;
	if (t1 > t2) {
	    std::swap(t1, t2);
	    need_reverse = false;
	}
	ON_Curve* seg_on_SSI = sub_curve(curves[p.m_type], t1, t2);
	if (seg_on_SSI == NULL) {
	    bu_log("sub_curve() failed.\n");
	    continue;
	}
	if (need_reverse) {
	    if (!seg_on_SSI->Reverse()) {
		bu_log("Reverse failed.\n");
		continue;
	    }
	}
	newloop.Append(seg_on_SSI);

	ON_Curve* rev_seg_on_SSI = seg_on_SSI->Duplicate();
	if (!rev_seg_on_SSI || !rev_seg_on_SSI->Reverse()) {
	    bu_log("Reverse failed.\n");
	} else {
	    // Update the outerloop
	    outerloop[p.m_pos + 1] = rev_seg_on_SSI;
	    int k = p.m_pos + 2;
	    for (int j = q.m_pos + 1; j < outerloop.Count(); j++)
		outerloop[k++] = outerloop[j];
	    while (k < outerloop.Count()) {
		outerloop[outerloop.Count()-1] = NULL;
		outerloop.Remove();
	    }
#if USE_CONNECTIVITY_GRAPH
	    // Update outerloop_start_end
	    IntersectPoint invalid_point;
	    invalid_point.m_seg = -1;
	    outerloop_start_end[p.m_pos + 1] = std::make_pair(invalid_point, invalid_point);
	    k = p.m_pos + 2;
	    for (int j = q.m_pos + 1; j < outerloop_start_end.Count(); j++)
		outerloop_start_end[k++] = outerloop_start_end[j];
	    while (k < outerloop_start_end.Count()) {
		outerloop_start_end.Remove();
	    }
#endif
	    // Update m_pos
	    for (int j = i + 1; j < intersect.Count(); j++)
		intersect[j].m_pos -= curve_count - 1;
	}

	// Append a trimmed face with newloop as its outerloop
	// Don't add a face if the outerloop is not valid (e.g. degenerated).
	if (IsLoopValid(newloop, ON_ZERO_TOLERANCE)) {
	    TrimmedFace *newface = new TrimmedFace();
	    newface->face = in->face;
	    newface->outerloop.Append(newloop.Count(), newloop.Array());
#if USE_CONNECTIVITY_GRAPH
	    for (int i = 0; i < newloop_start_end.Count(); i++)
		if (newloop_start_end[i].first.m_seg != -1)
		    newface->parts.Append(newloop_start_end[i]);
#endif
	    out.Append(newface);
	}
    }

    // Remove the duplicated segments before the first intersection point.
    if (intersect.Count()) {
	for (int i = 0; i <= intersect[0].m_pos; i++) {
	    outerloop.Remove(0);
#if USE_CONNECTIVITY_GRAPH
	    outerloop_start_end.Remove(0);
#endif
	}
    }

    if (out.Count() == 0) {
	out.Append(in->Duplicate());
    } else {
	// The remaining part after splitting some parts out.
	if (IsLoopValid(outerloop, ON_ZERO_TOLERANCE)) {
	    TrimmedFace *newface = new TrimmedFace();
	    newface->face = in->face;
	    newface->outerloop = outerloop;
	    newface->innerloop = in->innerloop;
	    newface->innerloop.insert(newface->innerloop.end(), innerloops.begin(), innerloops.end());
#if USE_CONNECTIVITY_GRAPH
	    /* if (intersect.Count()) {
		// Eliminate the parts of outerloop used by other sub-faces
		for (int i = 0; i < out.Count(); i++) {
		    if (out[i]->parts.Count() == 0)
			continue;
		    IntersectPoint& start = intersect[0];
		    for (int j = 0; j < out[i]->parts.Count(); j++) {
			newface->parts.Append(std::make_pair(start, out[i]->parts[j].first));
			start = out[i]->parts[j].second;
		    }
		}
	    } else {
		newface->parts = in->parts;
	    } */
	    for (int i = 0; i < outerloop_start_end.Count(); i++)
		if (outerloop_start_end[i].first.m_seg != -1)
		    newface->parts.Append(outerloop_start_end[i]);
#endif
	    out.Append(newface);
	}
    }

    bu_log("Split to %d faces.\n", out.Count());
    if (DEBUG_BREP_BOOLEAN) {
	for (int i = 0; i < out.Count(); i++) {
	    bu_log("Trimmed Face %d:\n", i);
	    bu_log("outerloop:\n");
	    ON_wString wstr;
	    ON_TextLog textlog(wstr);
	    textlog.PushIndent();
	    for (int j = 0; j < out[i]->outerloop.Count(); j++) {
		textlog.Print("Curve %d\n", j);
		out[i]->outerloop[j]->Dump(textlog);
	    }
	    bu_log(ON_String(wstr).Array());

	    for (unsigned int j = 0; j < out[i]->innerloop.size(); j++) {
		bu_log("innerloop %d:\n", j);
		ON_wString wstr2;
		ON_TextLog textlog2(wstr2);
		textlog2.PushIndent();
		for (int k = 0; k < out[i]->innerloop[j].Count(); k++) {
		    textlog2.Print("Curve %d\n", k);
		    out[i]->innerloop[j][k]->Dump(textlog2);
		}
		bu_log(ON_String(wstr2).Array());
	    }
	}
    }

    return 0;
}


HIDDEN void
add_elements(ON_Brep *brep, ON_BrepFace &face, const ON_SimpleArray<ON_Curve*> &loop, ON_BrepLoop::TYPE loop_type)
{
    if (!loop.Count())
	return;

    ON_BrepLoop &breploop = brep->NewLoop(loop_type, face);
    int start_count = brep->m_V.Count();
    for (int k = 0; k < loop.Count(); k++) {
	ON_Curve* c3d = NULL;
	// First, try the ON_Surface::Pushup() method.
	// If Pushup() does not succeed, use sampling method.
	c3d = face.SurfaceOf()->Pushup(*(loop[k]), INTERSECTION_TOL);
	if (c3d) {
	    brep->AddEdgeCurve(c3d);
	} else {
	    // TODO: Sometimes we need a singular trim.
	    ON_3dPointArray ptarray(101);
	    for (int l = 0; l <= 100; l++) {
		ON_3dPoint pt2d;
		pt2d = loop[k]->PointAt(loop[k]->Domain().ParameterAt(l/100.0));
		ptarray.Append(face.SurfaceOf()->PointAt(pt2d.x, pt2d.y));
	    }
	    c3d = new ON_PolylineCurve(ptarray);
	    brep->AddEdgeCurve(c3d);
	}

	int ti = brep->AddTrimCurve(loop[k]);
	ON_2dPoint start = loop[k]->PointAtStart(), end = loop[k]->PointAtEnd();
	int start_idx, end_idx;
	if (k > 0)
	    start_idx = brep->m_V.Count() - 1;
	else {
	    start_idx = brep->m_V.Count();
	    brep->NewVertex(face.SurfaceOf()->PointAt(start.x, start.y), 0.0);
	}
	if (c3d->IsClosed())
	    end_idx = start_idx;
	else {
	    end_idx = brep->m_V.Count();
	    brep->NewVertex(face.SurfaceOf()->PointAt(end.x, end.y), 0.0);
	}
	if (k == loop.Count() - 1) {
	    if (!c3d->IsClosed()) {
		brep->m_V.Remove();
		end_idx = start_count;
	    } else if (k) {
		brep->m_V.Remove();
		start_idx = end_idx = start_count;
		if (brep->m_E.Last() && brep->m_T.Last()) {
		    brep->m_E.Last()->m_vi[1] = start_count;
		    brep->m_T.Last()->m_vi[1] = start_count;
		    brep->m_V[start_count].m_ei.Append(brep->m_E.Count() - 1);
		}
	    }
	}

	ON_BrepEdge &edge = brep->NewEdge(brep->m_V[start_idx], brep->m_V[end_idx],
	    brep->m_C3.Count() - 1, (const ON_Interval *)0, MAX_FASTF);
	ON_BrepTrim &trim = brep->NewTrim(edge, 0, breploop, ti);
	trim.m_tolerance[0] = trim.m_tolerance[1] = MAX_FASTF;

	// TODO: Deal with split seam trims to pass ON_Brep::IsValid()
    }
    //if (brep->m_V.Count() < brep->m_V.Capacity())
	//brep->m_V[brep->m_V.Count()].m_ei.Empty();
}


#if USE_CONNECTIVITY_GRAPH
HIDDEN int
build_connectivity_graph(const ON_Brep* brep, ON_SimpleArray<TrimmedFace*>& trimmedfaces, int start_idx = -1, int end_idx = -1)
{
    if (start_idx == -1)
	start_idx = 0;

    if (end_idx == -1)
	end_idx = trimmedfaces.Count() - 1;

    int facecount = brep->m_F.Count();
    if (end_idx - start_idx + 1 != facecount) {
	bu_log("build_connectivity_graph() Error: length of [start_idx, end_idx] not equal to the face count.\n");
	return -1;
    }

    // Index of the edges of a face
    ON_ClassArray<ON_SimpleArray<int> > edge_index(facecount);

    for (int i = 0; i < facecount; i++) {
	ON_SimpleArray<int> ei;
	const ON_BrepFace& face = brep->m_F[i];
	for (int j = 0; j < face.m_li.Count(); j++) {
	    const ON_BrepLoop& loop = brep->m_L[face.m_li[j]];
	    for (int k = 0; k < loop.m_ti.Count(); k++) {
		const ON_BrepTrim& trim = brep->m_T[loop.m_ti[k]];
		if (trim.m_ei != -1) {
		    // The trim is not singular. It's corresponding to an edge
		    ei.Append(trim.m_ei);
		}
	    }
	}
	edge_index.Append(ei);
    }

    if (edge_index.Count() != facecount) {
	bu_log("build_connectivity_graph() Error: edge_index.Count() != brep->m_F.Count()\n");
	return -1;
    }

    // Find the faces that share an edge
    for (int i = 0; i < facecount; i++) {
	for (int j = i + 1; j < facecount; j++) {
	    bool shared = false;
	    // Using O(n^2) searching. Because the problem size is usually very
	    // small, this is an ideal choice.
	    for (int k = 0; k < edge_index[i].Count(); k++) {
		if (edge_index[j].Search(edge_index[i][k]) != -1) {
		    shared = true;
		    break;
		}
	    }
	    if (shared) {
		// They share an edge, so they're neighbors.
		// Search the array to avoid duplication
		if (trimmedfaces[start_idx+i]->neighbors.Search(trimmedfaces[start_idx+j]) == -1) {
		    trimmedfaces[start_idx+i]->neighbors.Append(trimmedfaces[start_idx+j]);
		}
		if (trimmedfaces[start_idx+j]->neighbors.Search(trimmedfaces[start_idx+i]) == -1) {
		    trimmedfaces[start_idx+j]->neighbors.Append(trimmedfaces[start_idx+i]);
		}
	    }
	}
    }

    return 0;
}
#endif	// #if USE_CONNECTIVITY_GRAPH


int
ON_Boolean(ON_Brep* brepO, const ON_Brep* brepA, const ON_Brep* brepB, int UNUSED(operation))
{
    int facecount1 = brepA->m_F.Count();
    int facecount2 = brepB->m_F.Count();
    ON_SimpleArray<ON_Curve*> *curvesarray = new ON_SimpleArray<ON_Curve*> [facecount1 + facecount2];

    // calculate intersection curves
    for (int i = 0; i < facecount1; i++) {
	for (int j = 0; j < facecount2; j++) {
	    // Possible enhancement: Some faces may share the same surface.
	    // We can store the result of SSI to avoid re-computation.
	    ON_ClassArray<ON_SSX_EVENT> events;
	    if (ON_Intersect(brepA->m_S[brepA->m_F[i].m_si],
			     brepB->m_S[brepB->m_F[j].m_si],
			     events,
			     INTERSECTION_TOL) <= 0)
		continue;
	    ON_SimpleArray<ON_Curve*> curve_uv, curve_st;
	    for (int k = 0; k < events.Count(); k++) {
		if (events[k].m_type == ON_SSX_EVENT::ssx_tangent
		    || events[k].m_type == ON_SSX_EVENT::ssx_transverse) {
		    if (get_subcurve_inside_faces(brepA, brepB, i, j, &events[k]) < 0)
			continue;
		    curve_uv.Append(events[k].m_curveA);
		    curve_st.Append(events[k].m_curveB);
		    // Set m_curveA and m_curveB to NULL, in case that they are
		    // deleted by ~ON_SSX_EVENT().
		    events[k].m_curveA = events[k].m_curveB = NULL;
		}
	    }
	    curvesarray[i].Append(curve_uv.Count(), curve_uv.Array());
	    curvesarray[facecount1 + j].Append(curve_st.Count(), curve_st.Array());
	}
    }

    ON_SimpleArray<TrimmedFace*> original_faces;
    for (int i = 0; i < facecount1 + facecount2; i++) {
	const ON_BrepFace &face = i < facecount1 ? brepA->m_F[i] : brepB->m_F[i - facecount1];
	const ON_Brep *brep = i < facecount1 ? brepA : brepB;
	const ON_SimpleArray<int> &loopindex = face.m_li;

	TrimmedFace *first = new TrimmedFace();
	first->face = &face;

	for (int j = 0; j < loopindex.Count(); j++) {
	    const ON_BrepLoop &loop = brep->m_L[loopindex[j]];
	    const ON_SimpleArray<int> &trimindex = loop.m_ti;
	    ON_SimpleArray<ON_Curve*> iloop;
	    for (int k = 0; k < trimindex.Count(); k++) {
		ON_Curve *curve2d = brep->m_C2[brep->m_T[trimindex[k]].m_c2i];
		if (j == 0) {
		    first->outerloop.Append(curve2d->Duplicate());
		} else {
		    iloop.Append(curve2d->Duplicate());
		}
	    }
	    if (j != 0)
		first->innerloop.push_back(iloop);
	}
#if USE_CONNECTIVITY_GRAPH
	if (first->outerloop.Count()) {
	    IntersectPoint start, end;
	    start.m_seg = 0;
	    start.m_t = first->outerloop[0]->Domain().Min();
	    end.m_seg = first->outerloop.Count() - 1;
	    end.m_t = (*first->outerloop.Last())->Domain().Max();
	    first->parts.Append(std::make_pair(start, end));
	}
#endif
	original_faces.Append(first);
    }

    if (original_faces.Count() != facecount1 + facecount2) {
	bu_log("ON_Boolean() Error: TrimmedFace generation failed.\n");
	return -1;
    }

#if USE_CONNECTIVITY_GRAPH
    build_connectivity_graph(brepA, original_faces, 0, facecount1 - 1);
    build_connectivity_graph(brepB, original_faces, facecount1, original_faces.Count() - 1);

    if (DEBUG_BREP_BOOLEAN) {
	bu_log("The connectivity graph for the first brep structure.");
	for (int i = 0; i < facecount1; i++) {
	    bu_log("\nFace[%d]'s neighbors:", i);
	    for (int j = 0; j < original_faces[i]->neighbors.Count(); j++) {
		bu_log(" %d", original_faces.Search(original_faces[i]->neighbors[j]));
	    }
	}
	bu_log("\nThe connectivity graph for the second brep structure.");
	for (int i = 0; i < facecount2; i++) {
	    bu_log("\nFace[%d]'s neighbors:", i);
	    for (int j = 0; j < original_faces[facecount1 + i]->neighbors.Count(); j++) {
		bu_log(" %d", original_faces.Search(original_faces[facecount1 + i]->neighbors[j]) - facecount1);
	    }
	}
	bu_log("\n");
    }
#endif	// #if USE_CONNECTIVITY_GRAPH

    // split the surfaces with the intersection curves
    ON_ClassArray<ON_SimpleArray<TrimmedFace*> > trimmedfaces;
    for (int i = 0; i < original_faces.Count(); i++) {
	TrimmedFace* first = original_faces[i];
	ON_SimpleArray<ON_Curve*> linked_curves;
	link_curves(curvesarray[i], linked_curves);
	ON_SimpleArray<TrimmedFace*> splitted;
	split_trimmed_face(splitted, first, linked_curves);
	trimmedfaces.Append(splitted);
    }

    if (trimmedfaces.Count() != original_faces.Count()) {
	bu_log("ON_Boolean() Error: trimmedfaces.Count() != original_faces.Count()\n");
	delete [] curvesarray;
	return -1;
    }

#if USE_CONNECTIVITY_GRAPH
    // Update the connectivity graph after surface partitioning
    for (int i = 0; i < original_faces.Count(); i++) {
	for (int j = 0; j < trimmedfaces[i].Count(); j++) {
	    TrimmedFace* t_face = trimmedfaces[i][j];
	    if (t_face->parts.Count() == 0)
		continue;
	    for (int k = 0; k < original_faces[i]->neighbors.Count(); k++) {
		int neighbor_index = original_faces.Search(original_faces[i]->neighbors[k]);
		if (neighbor_index == -1)
		    continue;
		for (int l = 0; l < trimmedfaces[neighbor_index].Count(); l++) {
		    // Search all of its parent's neighbors' children, and check
		    // whether they share a common part of the original outerloop.
		    // If their "parts" intersect (overlap) in some way, they are
		    // neighbors again. (Neighboring parents' children become new
		    // neighbors: parent[i]'s child[j] with parent[i]'s neighbor[k]'s
		    // child[l])
		    TrimmedFace* another_face = trimmedfaces[neighbor_index][l];
		    if (another_face->parts.Count() == 0)
			continue;
		    // Find an intersection between all their "parts".
		    for (int i1 = 0; i1 < t_face->parts.Count(); i1++) {
			for (int i2 = 0; i2 < another_face->parts.Count(); i2++) {
			    const IntersectPoint& start1 = t_face->parts[i1].first;
			    const IntersectPoint& start2 = another_face->parts[i2].first;
			    const IntersectPoint& end1 = t_face->parts[i1].second;
			    const IntersectPoint& end2 = another_face->parts[i2].second;
			    const IntersectPoint& start_max = compare_t(&start1, &start2) < 0 ? start2 : start1;
			    const IntersectPoint& end_min = compare_t(&end1, &end2) < 0 ? end1 : end2;
			    if (compare_t(&start_max, &end_min) < 0) {
				// The intervals intersect
				if (t_face->neighbors.Search(another_face) == -1)
				    t_face->neighbors.Append(another_face);
				if (another_face->neighbors.Search(t_face) == -1)
				    another_face->neighbors.Append(t_face);
				break;
			    }
			}
		    }
		}
	    }
	}
    }

    if (DEBUG_BREP_BOOLEAN) {
	bu_log("The new connectivity graph for the first brep structure.");
	for (int i = 0; i < facecount1; i++) {
	    for (int j = 0; j < trimmedfaces[i].Count(); j++) {
		bu_log("\nFace[%p]'s neighbors:", trimmedfaces[i][j]);
		for (int k = 0; k < trimmedfaces[i][j]->neighbors.Count(); k++)
		    bu_log(" %p", trimmedfaces[i][j]->neighbors[k]);
	    }
	}
	bu_log("\nThe new connectivity graph for the second brep structure.");
	for (int i = 0; i < facecount2; i++) {
	    for (int j = 0; j < trimmedfaces[facecount1 + i].Count(); j++) {
		bu_log("\nFace[%p]'s neighbors:", trimmedfaces[facecount1 + i][j]);
		for (int k = 0; k < trimmedfaces[facecount1 + i][j]->neighbors.Count(); k++)
		    bu_log(" %p", trimmedfaces[facecount1 + i][j]->neighbors[k]);
	    }
	}
	bu_log("\n");
    }
#endif	// #if USE_CONNECTIVITY_GRAPH

    for (int i = 0; i < trimmedfaces.Count(); i++) {
	const ON_SimpleArray<TrimmedFace*>& splitted = trimmedfaces[i];
	const ON_Surface* surf = splitted.Count() ? splitted[0]->face->SurfaceOf() : NULL;
	/* TODO: Perform inside-outside test to decide whether the trimmed face
	 * should be used in the final b-rep structure or not.
	 * Different operations should be dealt with accordingly.
	 * Another solution is to use connectivity graphs which represents the
	 * topological structure of the b-rep. This can reduce time-consuming
	 * inside-outside tests.
	 * Here we just use all of these trimmed faces.
	 */
	for (int j = 0; j < splitted.Count(); j++) {
	    // Add the surfaces, faces, loops, trims, vertices, edges, etc.
	    // to the brep structure.
	    ON_Surface *new_surf = surf->Duplicate();
	    int surfindex = brepO->AddSurface(new_surf);
	    ON_BrepFace& new_face = brepO->NewFace(surfindex);

	    add_elements(brepO, new_face, splitted[j]->outerloop, ON_BrepLoop::outer);
	    // ON_BrepLoop &loop = brepO->m_L[brepO->m_L.Count() - 1];
	    for (unsigned int k = 0; k < splitted[j]->innerloop.size(); k++)
		add_elements(brepO, new_face, splitted[j]->innerloop[k], ON_BrepLoop::inner);

	    brepO->SetTrimIsoFlags(new_face);
	    brepO->FlipFace(new_face);
	}
    }

    // Check IsValid() and output the message.
    ON_wString ws;
    ON_TextLog log(ws);
    brepO->IsValid(&log);
    bu_log(ON_String(ws).Array());

    // WIP
    delete [] curvesarray;
    return 0;
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
