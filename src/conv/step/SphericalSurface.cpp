/*                 SphericalSurface.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2011 United States Government as represented by
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
/** @file SphericalSurface.cpp
 *
 * Routines to interface to STEP "SphericalSurface".
 *
 */
#include "STEPWrapper.h"
#include "Factory.h"

#include "Axis2Placement3D.h"

#include "SphericalSurface.h"

#define CLASSNAME "SphericalSurface"
#define ENTITYNAME "Spherical_Surface"
string SphericalSurface::entityname = Factory::RegisterClass(ENTITYNAME,(FactoryMethod)SphericalSurface::Create);

SphericalSurface::SphericalSurface() {
	step = NULL;
	id = 0;
}

SphericalSurface::SphericalSurface(STEPWrapper *sw,int step_id) {
	step=sw;
	id = step_id;
}

SphericalSurface::~SphericalSurface() {
}

const double *
SphericalSurface::GetOrigin() {
	return position->GetOrigin();
}

const double *
SphericalSurface::GetNormal() {
	return position->GetAxis(2);
}

const double *
SphericalSurface::GetXAxis() {
	return position->GetXAxis();
}

const double *
SphericalSurface::GetYAxis() {
	return position->GetYAxis();
}


bool
SphericalSurface::Load(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
	step=sw;
	id = sse->STEPfile_id;

	if ( !ElementarySurface::Load(step,sse) ) {
		std::cout << CLASSNAME << ":Error loading base class ::Surface." << std::endl;
		return false;
	}

	// need to do this for local attributes to makes sure we have
	// the actual entity and not a complex/supertype parent
	sse = step->getEntity(sse,ENTITYNAME);

	radius = step->getRealAttribute(sse,"radius");

	return true;
}

void
SphericalSurface::Print(int level) {
	TAB(level); std::cout << CLASSNAME << ":" << name << "(";
	std::cout << "ID:" << STEPid() << ")" << std::endl;

	TAB(level+1); std::cout << "radius: " << radius << std::endl;

	ElementarySurface::Print(level+1);
}

STEPEntity *
SphericalSurface::Create(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
	Factory::OBJECTS::iterator i;
	if ((i = Factory::FindObject(sse->STEPfile_id)) == Factory::objects.end()) {
		SphericalSurface *object = new SphericalSurface(sw,sse->STEPfile_id);

		Factory::AddObject(object);

		if (!object->Load(sw, sse)) {
			std::cerr << CLASSNAME << ":Error loading class in ::Create() method." << std::endl;
			delete object;
			return NULL;
		}
		return static_cast<STEPEntity *>(object);
	} else {
		return (*i).second;
	}
}
// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
