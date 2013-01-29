/*                 RepresentationItem.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2013 United States Government as represented by
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
/** @file step/RepresentationItem.cpp
 *
 * Routines to convert STEP "RepresentationItem" to BRL-CAD BREP
 * structures.
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"

#include "RepresentationItem.h"

#define CLASSNAME "RepresentationItem"
#define ENTITYNAME "Representation_Item"
string RepresentationItem::entityname = Factory::RegisterClass(ENTITYNAME,(FactoryMethod)RepresentationItem::Create);

RepresentationItem::RepresentationItem() {
    step = NULL;
    id = 0;
    name="";
}

RepresentationItem::RepresentationItem(STEPWrapper *sw,int step_id) {
    step = sw;
    id = step_id;
    name="";
}

RepresentationItem::~RepresentationItem() {
}

string
RepresentationItem::ClassName() {
    return entityname;
}

string
RepresentationItem::Name() {
    return name;
}


bool
RepresentationItem::Load(STEPWrapper *sw,SDAI_Application_instance *sse) {
    step=sw;
    id = sse->STEPfile_id;


    // need to do this for local attributes to makes sure we have
    // the actual entity and not a complex/supertype parent
    sse = step->getEntity(sse,ENTITYNAME);

    name = step->getStringAttribute(sse,"name");

    //std::cout << "name:" << name << std::endl;

    return true;
}

void
RepresentationItem::Print(int level) {
    TAB(level); std::cout << "RepresentationItem:" << name << std::endl;
    TAB(level); std::cout << "ID:" << STEPid() << std::endl;

    TAB(level); std::cout << "Attributes:" << std::endl;
    TAB(level+1); std::cout << "name:" << name << std::endl;
}

STEPEntity *
RepresentationItem::Create(STEPWrapper *sw, SDAI_Application_instance *sse) {
    Factory::OBJECTS::iterator i;
    if ((i = Factory::FindObject(sse->STEPfile_id)) == Factory::objects.end()) {
	RepresentationItem *object = new RepresentationItem(sw,sse->STEPfile_id);

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

bool
RepresentationItem::LoadONBrep(ON_Brep *brep)
{
    std::cerr << "Error: ::LoadONBrep(ON_Brep *brep<" << std::hex << brep << std::dec << ">) not implemented for " << entityname << std::endl;
    return false;
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
