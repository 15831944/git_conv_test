/*                     O N _ B R E P . C P P
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
/** @file ON_Brep.cpp
 *
 * File for writing out an ON_Brep structure into the STEPcode containers
 *
 */


// Make entity arrays for each of the m_V, m_S, etc arrays and create step instances of them,
// starting with the basic ones.
//
// The array indices in the ON_Brep will correspond to the step entity array locations that
// hold the step version of each entity.
//
// then, need to map the ON_Brep hierarchy to the corresponding STEP hierarchy
//
// brep -> advanced_brep_shape_representation
//         manifold_solid_brep
// faces-> closed_shell
//         advanced_face
//
// surface-> bspline_surface_with_knots
//           cartesian_point
//
// outer 3d trim loop ->  face_outer_bound    ->    SdaiFace_outer_bound -> SdaiFace_bound
//                        edge_loop           ->    SdaiEdge_loop
//                        oriented_edge       ->    SdaiOriented_edge
//                        edge_curve          ->    SdaiEdge_curve
//                        bspline_curve_with_knots
//                        vertex_point
//                        cartesian_point
//
// 2d points -> point_on_surface
// 1d points to bound curve -> point_on_curve
//
// 2d trimming curves -> pcurve using point_on_surface? almost doesn't look as if there is a good AP203 way to represent 2d trimming curves...
//
//
// Note that STEPentity is the same thing as SDAI_Application_instance... see src/clstepcore/sdai.h line 220
//

#include <sstream>
#include "STEPEntity.h"

void
ON_3dPoint_to_Cartesian_point(ON_3dPoint *inpnt, SdaiCartesian_point *step_pnt) {
	RealAggregate_ptr coord_vals = step_pnt->coordinates_();
	RealNode *xnode = new RealNode();
	xnode->value = inpnt->x;
	coord_vals->AddNode(xnode);
	RealNode *ynode = new RealNode();
	ynode->value = inpnt->y;
	coord_vals->AddNode(ynode);
	RealNode *znode = new RealNode();
	znode->value = inpnt->z;
	coord_vals->AddNode(znode);
}

void
ON_3dVector_to_Direction(ON_3dVector *invect, SdaiDirection *step_direction) {
	invect->Unitize();
	RealAggregate_ptr coord_vals = step_direction->direction_ratios_();
	RealNode *xnode = new RealNode();
	xnode->value = invect->x;
	coord_vals->AddNode(xnode);
	RealNode *ynode = new RealNode();
	ynode->value = invect->y;
	coord_vals->AddNode(ynode);
	RealNode *znode = new RealNode();
	znode->value = invect->z;
	coord_vals->AddNode(znode);
}

void
ON_NurbsCurveCV_to_EntityAggregate(ON_NurbsCurve *incrv, SdaiB_spline_curve *step_crv, Registry *registry, InstMgr *instance_list) {
	EntityAggregate *control_pnts = step_crv->control_points_list_();
	ON_3dPoint cv_pnt;
	for (int i = 0; i < incrv->CVCount(); i++) {
		SdaiCartesian_point *step_cartesian = (SdaiCartesian_point *)registry->ObjCreate("CARTESIAN_POINT");
		instance_list->Append(step_cartesian, completeSE);
		incrv->GetCV(i, cv_pnt);
		ON_3dPoint_to_Cartesian_point(&(cv_pnt), step_cartesian);
		control_pnts->AddNode(new EntityNode((SDAI_Application_instance *)step_cartesian));
	}
}

void
ON_NurbsSurfaceCV_to_GenericAggregate(ON_NurbsSurface *insrf, SdaiB_spline_surface *step_srf, Registry *registry, InstMgr *instance_list) {
	GenericAggregate *control_pnts_lists = step_srf->control_points_list_();
	ON_3dPoint cv_pnt;
	for (int i = 0; i < insrf->CVCount(0); i++) {
		std::ostringstream ss;
		ss << "(";
		for (int j = 0; j < insrf->CVCount(1); j++) {
			SdaiCartesian_point *step_cartesian = (SdaiCartesian_point *)registry->ObjCreate("CARTESIAN_POINT");
			instance_list->Append(step_cartesian, completeSE);
			insrf->GetCV(i, j, cv_pnt);
			ON_3dPoint_to_Cartesian_point(&(cv_pnt), step_cartesian);
			if (j != 0) ss << ",";
			ss << ((SDAI_Application_instance *)step_cartesian)->StepFileId();
		}
		ss << ")";
		std::string str = ss.str();
		control_pnts_lists->AddNode(new GenericAggrNode(str.c_str()));
	}
}

void
ON_NurbsCurveKnots_to_Aggregates(ON_NurbsCurve *incrv, SdaiB_spline_curve_with_knots *step_crv)
{
	IntAggregate_ptr knot_multiplicities = step_crv->knot_multiplicities_();
	RealAggregate_ptr knots = step_crv->knots_();
	int i = 0;
	while (i < incrv->KnotCount()) {
		int multiplicity_val = incrv->KnotMultiplicity(i);
		/* Add knot */
		RealNode *knot = new RealNode();
		knot->value = incrv->Knot(i);
		knots->AddNode(knot);
		/* OpenNURBS and STEP have different notions of end knot multiplicity -
		 * see http://wiki.mcneel.com/developer/onsuperfluousknot */
		if ((i == 0) || (i == (incrv->KnotCount() - incrv->KnotMultiplicity(0)))) multiplicity_val++;
		/* Set Multiplicity */
		IntNode *multiplicity = new IntNode();
		multiplicity->value = multiplicity_val;
		knot_multiplicities->AddNode(multiplicity);
		i += incrv->KnotMultiplicity(i);
	}
	step_crv->knot_spec_(Knot_type__unspecified);
}

void
ON_NurbsSurfaceKnots_to_Aggregates(ON_NurbsSurface *insrf, SdaiB_spline_surface_with_knots *step_srf)
{
	IntAggregate_ptr u_knot_multiplicities = step_srf->u_multiplicities_();
	IntAggregate_ptr v_knot_multiplicities = step_srf->v_multiplicities_();
	RealAggregate_ptr u_knots = step_srf->u_knots_();
	RealAggregate_ptr v_knots = step_srf->v_knots_();
	/* u knots */
	int i = 0;
	while (i < insrf->KnotCount(0)) {
		int multiplicity_val = insrf->KnotMultiplicity(0,i);
		/* Add knot */
		RealNode *knot = new RealNode();
		knot->value = insrf->Knot(0,i);
		u_knots->AddNode(knot);
		/* OpenNURBS and STEP have different notions of end knot multiplicity -
		 * see http://wiki.mcneel.com/developer/onsuperfluousknot */
		if ((i == 0) || (i == (insrf->KnotCount(0) - insrf->KnotMultiplicity(0,0)))) multiplicity_val++;
		/* Set Multiplicity */
		IntNode *multiplicity = new IntNode();
		multiplicity->value = multiplicity_val;
		u_knot_multiplicities->AddNode(multiplicity);
		i += insrf->KnotMultiplicity(0,i);
	}
	/* v knots */
	i = 0;
	while (i < insrf->KnotCount(1)) {
		int multiplicity_val = insrf->KnotMultiplicity(1,i);
		/* Add knot */
		RealNode *knot = new RealNode();
		knot->value = insrf->Knot(1,i);
		v_knots->AddNode(knot);
		/* OpenNURBS and STEP have different notions of end knot multiplicity -
		 * see http://wiki.mcneel.com/developer/onsuperfluousknot */
		if ((i == 0) || (i == (insrf->KnotCount(1) - insrf->KnotMultiplicity(1,0)))) multiplicity_val++;
		/* Set Multiplicity */
		IntNode *multiplicity = new IntNode();
		multiplicity->value = multiplicity_val;
		v_knot_multiplicities->AddNode(multiplicity);
		i += insrf->KnotMultiplicity(1,i);
	}
	step_srf->knot_spec_(Knot_type__unspecified);
}

#if 0
void
ON_RationalNurbsCurve_to_EntityAggregate(ON_NurbsCurve *incrv, SdaiRational_B_spline_curve *step_crv) {
}
#endif

bool ON_BRep_to_STEP(ON_Brep *brep, Registry *registry, InstMgr *instance_list)
{
	std::vector<STEPentity *> cartesian_pnts(brep->m_V.Count(), (STEPentity *)0);
	std::vector<STEPentity *> vertex_pnts(brep->m_V.Count(), (STEPentity *)0);
	std::vector<STEPentity *> three_dimensional_curves(brep->m_C3.Count(), (STEPentity *)0);
	std::vector<STEPentity *> edge_curves(brep->m_E.Count(), (STEPentity *)0);
	std::vector<STEPentity *> oriented_edges(brep->m_E.Count(), (STEPentity *)0);
	std::vector<STEPentity *> edge_loops(brep->m_L.Count(), (STEPentity *)0);
	std::vector<STEPentity *> outer_bounds(brep->m_F.Count(), (STEPentity *)0);
	std::vector<STEPentity *> surfaces(brep->m_S.Count(), (STEPentity *)0);
	std::vector<STEPentity *> faces(brep->m_F.Count(), (STEPentity *)0);
	//STEPentity *closed_shell = new STEPentity;
	//STEPentity *manifold_solid_brep = new STEPentity;
	//STEPentity *advanced_brep = new STEPentity;

        // Set up vertices and associated cartesian points
	for (int i = 0; i < brep->m_V.Count(); ++i) {
                // Cartesian points (actual 3D geometry)
		cartesian_pnts.at(i) = registry->ObjCreate("CARTESIAN_POINT");
		instance_list->Append(cartesian_pnts.at(i), completeSE);
		ON_3dPoint v_pnt = brep->m_V[i].Point();
		ON_3dPoint_to_Cartesian_point(&(v_pnt), (SdaiCartesian_point *)cartesian_pnts.at(i));
                // Vertex points (topological, references actual 3D geometry)
		vertex_pnts.at(i) = registry->ObjCreate("VERTEX_POINT");
		((SdaiVertex_point *)vertex_pnts.at(i))->vertex_geometry_((const SdaiPoint_ptr)cartesian_pnts.at(i));
		instance_list->Append(vertex_pnts.at(i), completeSE);
	}

	// 3D curves
	std::cout << "Have " << brep->m_C3.Count() << " curves\n";
	for (int i = 0; i < brep->m_C3.Count(); ++i) {
		int curve_converted = 0;
		ON_Curve* curve = brep->m_C3[i];
		/* Supported curve types */
		ON_ArcCurve *a_curve = ON_ArcCurve::Cast(curve);
		ON_LineCurve *l_curve = ON_LineCurve::Cast(curve);
		ON_NurbsCurve *n_curve = ON_NurbsCurve::Cast(curve);
		ON_PolyCurve *p_curve = ON_PolyCurve::Cast(curve);

		if (a_curve && !curve_converted) {
			std::cout << "Have ArcCurve\n";
		}
		if (l_curve && !curve_converted) {
			std::cout << "Have LineCurve\n";
			ON_Line *m_line = &(l_curve->m_line);
			/* In STEP, a line consists of a cartesian point and a 3D vector.  Since
			 * it does not appear that OpenNURBS data structures reference m_V points
			 * for these constructs, create our own */
			three_dimensional_curves.at(i) = registry->ObjCreate("LINE");
			SdaiLine *curr_line = (SdaiLine *)three_dimensional_curves.at(i);
			curr_line->pnt_((SdaiCartesian_point *)registry->ObjCreate("CARTESIAN_POINT"));
			ON_3dPoint_to_Cartesian_point(&(m_line->from), curr_line->pnt_());
			curr_line->dir_((SdaiVector *)registry->ObjCreate("VECTOR"));
			SdaiVector *curr_dir = curr_line->dir_();
			curr_dir->orientation_((SdaiDirection *)registry->ObjCreate("DIRECTION"));
			ON_3dVector on_dir = m_line->Direction();
			ON_3dVector_to_Direction(&(on_dir), curr_line->dir_()->orientation_());
			curr_line->dir_()->magnitude_(m_line->Length());
			instance_list->Append(curr_line->pnt_(), completeSE);
			instance_list->Append(curr_dir->orientation_(), completeSE);
			instance_list->Append(curr_line->dir_(), completeSE);
			instance_list->Append(three_dimensional_curves.at(i), completeSE);
			curve_converted = 1;
		}
		if (p_curve && !curve_converted) {
			std::cout << "Have PolyCurve\n";
		}
		if (n_curve && !curve_converted) {
			std::cout << "Have NurbsCurve\n";
			if (n_curve->IsRational()) {
				std::cout << "TODO - Have Rational NurbsCurve\n";
				three_dimensional_curves.at(i) = registry->ObjCreate("RATIONAL_B_SPLINE_CURVE");
			} else {
				three_dimensional_curves.at(i) = registry->ObjCreate("B_SPLINE_CURVE_WITH_KNOTS");
				SdaiB_spline_curve *curr_curve = (SdaiB_spline_curve *)three_dimensional_curves.at(i);
				curr_curve->degree_(n_curve->Degree());
				ON_NurbsCurveCV_to_EntityAggregate(n_curve, curr_curve, registry, instance_list);
				SdaiB_spline_curve_with_knots *curve_knots = (SdaiB_spline_curve_with_knots *)three_dimensional_curves.at(i);
				ON_NurbsCurveKnots_to_Aggregates(n_curve, curve_knots);
			}
			((SdaiB_spline_curve *)three_dimensional_curves.at(i))->curve_form_(B_spline_curve_form__unspecified);
			((SdaiB_spline_curve *)three_dimensional_curves.at(i))->closed_curve_(SDAI_LOGICAL(n_curve->IsClosed()));
			/* TODO:  Assume we don't have self-intersecting curves for now - need some way to test this... */
			((SdaiB_spline_curve *)three_dimensional_curves.at(i))->self_intersect_(LFalse);
			instance_list->Append(three_dimensional_curves.at(i), completeSE);
			curve_converted = 1;
		}

		/* Whatever this is, if it's not a supported type and it does have
		 * a NURBS form, use that */
		if (!curve_converted) std::cout << "Curve not converted! " << i << "\n";

	}

	// edge topology - ON_BrepEdge -> edge curves and oriented edges
	for (int i = 0; i < brep->m_E.Count(); ++i) {
		ON_BrepEdge *edge = &(brep->m_E[i]);
		edge_curves.at(i) = registry->ObjCreate("EDGE_CURVE");
		instance_list->Append(edge_curves.at(i), completeSE);
		SdaiEdge_curve *e_curve = (SdaiEdge_curve *)edge_curves.at(i);
		e_curve->edge_geometry_(((SdaiCurve *)three_dimensional_curves.at(edge->EdgeCurveIndexOf())));
		e_curve->same_sense_(BTrue);
		oriented_edges.at(i) = registry->ObjCreate("ORIENTED_EDGE");
		instance_list->Append(oriented_edges.at(i), completeSE);
		SdaiOriented_edge *oriented_edge = (SdaiOriented_edge *)oriented_edges.at(i);
		oriented_edge->edge_element_((SdaiEdge *)e_curve);
		oriented_edge->edge_start_(((SdaiVertex *)vertex_pnts.at(edge->Vertex(0)->m_vertex_index)));
		oriented_edge->edge_start_(((SdaiVertex *)vertex_pnts.at(edge->Vertex(1)->m_vertex_index)));
		/* Check whether the 3d points of the vertices correspond to the beginning and end of the curve */
		double d1 = edge->Vertex(0)->Point().DistanceTo(brep->m_C3[edge->EdgeCurveIndexOf()]->PointAtStart());
		double d1a = edge->Vertex(0)->Point().DistanceTo(brep->m_C3[edge->EdgeCurveIndexOf()]->PointAtEnd());
		double d2 = edge->Vertex(1)->Point().DistanceTo(brep->m_C3[edge->EdgeCurveIndexOf()]->PointAtStart());
		double d2a = edge->Vertex(1)->Point().DistanceTo(brep->m_C3[edge->EdgeCurveIndexOf()]->PointAtEnd());
		if ((d1 < d1a) && (d2a < d2)) {
			oriented_edge->orientation_(BTrue);
		} else {
			oriented_edge->orientation_(BFalse);
		}
	}

	// loop topology.  STEP defines loops with 3D edge curves, but OpenNURBS describes ON_BrepLoops with
	// 2d trim curves.  So for a given loop, we need to iterate over the trims, for each trim get the
	// index of its corresponding edge, and add that edge to the _edge_list for the loop.
	for (int i = 0; i < brep->m_L.Count(); ++i) {
		ON_BrepLoop *loop= &(brep->m_L[i]);
		edge_loops.at(i) = registry->ObjCreate("EDGE_LOOP");
		instance_list->Append(edge_loops.at(i), completeSE);
		// Why doesn't SdaiEdge_loop's edge_list_() function give use the edge_list from the SdaiPath??
		// Initialized to NULL and crashes - what good is it?  Have to get at the internal SdaiPath
		// directly to build something that STEPwrite will output.
		SdaiPath *e_loop_path = (SdaiPath *)edge_loops.at(i)->GetNextMiEntity();
		for (int l = 0; l < loop->TrimCount(); ++l) {
			ON_BrepEdge *edge = loop->Trim(l)->Edge();
			if (edge)
				e_loop_path->edge_list_()->AddNode(new EntityNode((SDAI_Application_instance *)(oriented_edges.at(edge->m_edge_index))));
		}
	}

	// surfaces - TODO - need to handle cylindrical, conical, toroidal, etc. types that are enumerated
	std::cout << "Have " << brep->m_S.Count() << " surfaces\n";
	for (int i = 0; i < brep->m_S.Count(); ++i) {
		int surface_converted = 0;
		ON_Surface* surface = brep->m_S[i];
		/* Supported surface types */
		ON_OffsetSurface *o_surface = ON_OffsetSurface::Cast(surface);
		ON_PlaneSurface *p_surface = ON_PlaneSurface::Cast(surface);
		ON_ClippingPlaneSurface *pc_surface = ON_ClippingPlaneSurface::Cast(surface);
		ON_NurbsSurface *n_surface = ON_NurbsSurface::Cast(surface);
		ON_RevSurface *rev_surface = ON_RevSurface::Cast(surface);
		ON_SumSurface *sum_surface = ON_SumSurface::Cast(surface);
		ON_SurfaceProxy *surface_proxy = ON_SurfaceProxy::Cast(surface);

		if (o_surface && !surface_converted) {
			std::cout << "Have OffsetSurface\n";
		}

		if (p_surface && !surface_converted) {
			std::cout << "Have PlaneSurface\n";
			ON_NurbsSurface p_nurb;
			p_surface->GetNurbForm(p_nurb);
			surfaces.at(i) = registry->ObjCreate("B_SPLINE_SURFACE_WITH_KNOTS");
			SdaiB_spline_surface *curr_surface = (SdaiB_spline_surface *)surfaces.at(i);
			curr_surface->u_degree_(p_nurb.Degree(0));
			curr_surface->v_degree_(p_nurb.Degree(1));
			ON_NurbsSurfaceCV_to_GenericAggregate(&p_nurb, curr_surface, registry, instance_list);
			SdaiB_spline_surface_with_knots *surface_knots = (SdaiB_spline_surface_with_knots *)surfaces.at(i);
			ON_NurbsSurfaceKnots_to_Aggregates(&p_nurb, surface_knots);
			curr_surface->surface_form_(B_spline_surface_form__plane_surf);
			/* Planes don't self-intersect */
			curr_surface->self_intersect_(LFalse);
			instance_list->Append(surfaces.at(i), completeSE);
			surface_converted = 1;
		}

		if (pc_surface && !surface_converted) {
			std::cout << "Have CuttingPlaneSurface\n";
		}

		if (n_surface && !surface_converted) {
			std::cout << "Have NurbsSurface\n";
			surfaces.at(i) = registry->ObjCreate("B_SPLINE_SURFACE_WITH_KNOTS");
			SdaiB_spline_surface *curr_surface = (SdaiB_spline_surface *)surfaces.at(i);
			curr_surface->u_degree_(n_surface->Degree(0));
			curr_surface->v_degree_(n_surface->Degree(1));
			ON_NurbsSurfaceCV_to_GenericAggregate(n_surface, curr_surface, registry, instance_list);
			SdaiB_spline_surface_with_knots *surface_knots = (SdaiB_spline_surface_with_knots *)surfaces.at(i);
			ON_NurbsSurfaceKnots_to_Aggregates(n_surface, surface_knots);
			curr_surface->surface_form_(B_spline_surface_form__unspecified);
			/* TODO - for now, assume the surfaces don't self-intersect - need to figure out how to test this */
			curr_surface->self_intersect_(LFalse);
			instance_list->Append(surfaces.at(i), completeSE);
			surface_converted = 1;
		}

		if (rev_surface && !surface_converted) {
			std::cout << "Have RevSurface\n";
		}

		if (sum_surface && !surface_converted) {
			std::cout << "Have SumSurface\n";
			ON_NurbsSurface sum_nurb;
			sum_surface->GetNurbForm(sum_nurb);
			surfaces.at(i) = registry->ObjCreate("B_SPLINE_SURFACE_WITH_KNOTS");
			SdaiB_spline_surface *curr_surface = (SdaiB_spline_surface *)surfaces.at(i);
			curr_surface->u_degree_(sum_nurb.Degree(0));
			curr_surface->v_degree_(sum_nurb.Degree(1));
			ON_NurbsSurfaceCV_to_GenericAggregate(&sum_nurb, curr_surface, registry, instance_list);
			SdaiB_spline_surface_with_knots *surface_knots = (SdaiB_spline_surface_with_knots *)surfaces.at(i);
			ON_NurbsSurfaceKnots_to_Aggregates(&sum_nurb, surface_knots);
			curr_surface->surface_form_(B_spline_surface_form__plane_surf);
			/* TODO - for now, assume non-self-intersecting */
			curr_surface->self_intersect_(LFalse);
			instance_list->Append(surfaces.at(i), completeSE);
			surface_converted = 1;
		}

		if (surface_proxy && !surface_converted) {
			std::cout << "Have SurfaceProxy\n";
		}

	}

	// faces
	for (int i = 0; i < brep->m_F.Count(); ++i) {
		ON_BrepFace* face = &(brep->m_F[i]);
		faces.at(i) = registry->ObjCreate("ADVANCED_FACE");
		SdaiAdvanced_face *step_face = (SdaiAdvanced_face *)faces.at(i);
		step_face->face_geometry_((SdaiSurface *)surfaces.at(face->SurfaceIndexOf()));
		// TODO - is m_bRev the same thing as same_sense?
		step_face->same_sense_((const Boolean)(face->m_bRev));
		EntityAggregate *bounds = step_face->bounds_();
		for (int j = 0; j < face->LoopCount(); ++j) {
			ON_BrepLoop *curr_loop = face->Loop(j);
			if (curr_loop == face->OuterLoop()) {
				SdaiFace_outer_bound *outer_bound = (SdaiFace_outer_bound *)registry->ObjCreate("FACE_OUTER_BOUND");
				instance_list->Append(outer_bound, completeSE);
				outer_bound->bound_((SdaiLoop *)edge_loops.at(curr_loop->m_loop_index));
				// TODO - When should this be false?
				outer_bound->orientation_(BTrue);
				bounds->AddNode(new EntityNode((SDAI_Application_instance *)outer_bound));
			} else {
				SdaiFace_bound *inner_bound = (SdaiFace_bound *)registry->ObjCreate("FACE_BOUND");
				instance_list->Append(inner_bound, completeSE);
				inner_bound->bound_((SdaiLoop *)edge_loops.at(curr_loop->m_loop_index));
				// TODO - When should this be false?
				inner_bound->orientation_(BTrue);
				bounds->AddNode(new EntityNode((SDAI_Application_instance *)inner_bound));
			}
		}
		instance_list->Append(step_face, completeSE);
	}


	return true;
}
