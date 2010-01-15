/*
 * ------------------------------------------------------------------------
 *      PACKAGE:  [incr Tcl]
 *  DESCRIPTION:  Object-Oriented Extensions to Tcl
 *
 *  This file contains procedures that belong in the Tcl/Tk core.
 *  Hopefully, they'll migrate there soon.
 *
 * ========================================================================
 *  AUTHOR:  Arnulf Wiedemann
 *
 *     RCS:  $Id$
 * ========================================================================
 *           Copyright (c) 1993-1998  Lucent Technologies, Inc.
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include <tcl.h>
#include <tclInt.h>
#include "itclMigrate2TclCore.h"

int
Itcl_SetCallFrameResolver(
    Tcl_Interp *interp,
    Tcl_Resolve *resolvePtr)
{
    CallFrame *framePtr = ((Interp *)interp)->framePtr;
    if (framePtr != NULL) {
#ifdef ITCL_USE_MODIFIED_TCL_H
        framePtr->isProcCallFrame |= FRAME_HAS_RESOLVER;
	framePtr->resolvePtr = resolvePtr;
#endif
        return TCL_OK;
    }
    return TCL_ERROR;
}

int
_Tcl_SetNamespaceResolver(
    Tcl_Namespace *nsPtr,
    Tcl_Resolve *resolvePtr)
{
    if (nsPtr == NULL) {
        return TCL_ERROR;
    }
#ifdef ITCL_USE_MODIFIED_TCL_H
    ((Namespace *)nsPtr)->resolvePtr = resolvePtr;
#endif
    return TCL_OK;
}

Tcl_Var
Tcl_NewNamespaceVar(
    Tcl_Interp *interp,
    Tcl_Namespace *nsPtr,
    const char *varName)
{
    Var *varPtr = NULL;
    int new;

    if ((nsPtr == NULL) || (varName == NULL)) {
        return NULL;
    }

    varPtr = TclVarHashCreateVar(&((Namespace *)nsPtr)->varTable,
            varName, &new);
    TclSetVarNamespaceVar(varPtr);
    VarHashRefCount(varPtr)++;
    return (Tcl_Var)varPtr;
}

Tcl_CallFrame *
Itcl_GetUplevelCallFrame(
    Tcl_Interp *interp,
    int level)
{
    CallFrame *framePtr;
    if (level < 0) {
        return NULL;
    }
    framePtr = ((Interp *)interp)->framePtr;
    while ((framePtr != NULL) && (level-- > 0)) {
        framePtr = framePtr->callerVarPtr;
    }
    if (framePtr == NULL) {
        return NULL;
    }
    return (Tcl_CallFrame *)framePtr;
}


Tcl_CallFrame *
Itcl_ActivateCallFrame(
    Tcl_Interp *interp,
    Tcl_CallFrame *framePtr)
{
    Interp *iPtr = (Interp*)interp;
    CallFrame *oldFramePtr;

    oldFramePtr = iPtr->varFramePtr;
    iPtr->varFramePtr = (CallFrame *) framePtr;

    return (Tcl_CallFrame *) oldFramePtr;
}

Tcl_Namespace *
Itcl_GetUplevelNamespace(
    Tcl_Interp *interp,
    int level)
{
    CallFrame *framePtr;
    if (level < 0) {
        return NULL;
    }
    framePtr = ((Interp *)interp)->framePtr;
    while ((framePtr != NULL) && (level-- > 0)) {
        framePtr = framePtr->callerVarPtr;
    }
    if (framePtr == NULL) {
        return NULL;
    }
    return (Tcl_Namespace *)framePtr->nsPtr;
}

ClientData
Itcl_GetCallFrameClientData(
    Tcl_Interp *interp)
{
    CallFrame *framePtr = ((Interp *)interp)->framePtr;
    if (framePtr == NULL) {
        return NULL;
    }
    return framePtr->clientData;
}

int
Itcl_SetCallFrameNamespace(
    Tcl_Interp *interp,
    Tcl_Namespace *nsPtr)
{
    CallFrame *framePtr = ((Interp *)interp)->framePtr;
    if (framePtr == NULL) {
        return TCL_ERROR;
    }
    ((Interp *)interp)->framePtr->nsPtr = (Namespace *)nsPtr;
    return TCL_OK;
}

int
Itcl_GetCallFrameObjc(
    Tcl_Interp *interp)
{
    CallFrame *framePtr = ((Interp *)interp)->framePtr;
    if (framePtr == NULL) {
        return 0;
    }
    return ((Interp *)interp)->framePtr->objc;
}

Tcl_Obj * const *
Itcl_GetCallFrameObjv(
    Tcl_Interp *interp)
{
    CallFrame *framePtr = ((Interp *)interp)->framePtr;
    if (framePtr == NULL) {
        return NULL;
    }
    return ((Interp *)interp)->framePtr->objv;
}

int
Itcl_IsCallFrameArgument(
    Tcl_Interp *interp,
    const char *name)
{
    CallFrame *varFramePtr = ((Interp *)interp)->framePtr;
    Proc *procPtr;

    if (varFramePtr == NULL) {
        return 0;
    }
    if (!varFramePtr->isProcCallFrame) {
        return 0;
    }
    procPtr = varFramePtr->procPtr;
    /*
     *  Search through compiled locals first...
     */
    if (procPtr) {
        CompiledLocal *localPtr = procPtr->firstLocalPtr;
        int nameLen = strlen(name);

        for (;localPtr != NULL; localPtr = localPtr->nextPtr) {
            if (TclIsVarArgument(localPtr)) {
                register char *localName = localPtr->name;
                if ((name[0] == localName[0])
                        && (nameLen == localPtr->nameLength)
                        && (strcmp(name, localName) == 0)) {
                    return 1;
                }
            }
        }            
    }
    return 0;
}
