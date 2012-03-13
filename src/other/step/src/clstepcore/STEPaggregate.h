#ifndef STEPAGGREGATE_H
#define STEPAGGREGATE_H

/*
* NIST STEP Core Class Library
* clstepcore/STEPaggregate.h
* April 1997
* K. C. Morris
* David Sauder

* Development of this software was funded by the United States Government,
* and is not subject to copyright.
*/

/* $Id: STEPaggregate.h,v 3.0.1.6 1997/11/05 21:59:27 sauderd DP3.1 $ */

#ifdef __O3DB__
#include <OpenOODB.h>
#endif

class InstMgr;
class STEPaggregate;
#ifndef _ODI_OSSG_
class LOGICAL;
class BOOLEAN;
#endif
class TypeDescriptor;

#include <errordesc.h>
#include <SingleLinkList.h>
#include <baseType.h>
#include <sdai.h>
#include <STEPundefined.h>
#include <string>

//class SCLP23(Application_instance);
//#define 	S_ENTITY_NULL	&NilSTEPentity
//extern SCLP23(Application_instance) NilSTEPentity;


#define 	AGGR_NULL	&NilSTEPaggregate
extern STEPaggregate NilSTEPaggregate;

//typedef unsigned short BOOLEAN;
class SingleLinkNode;

/******************************************************************************
 **
 *****************************************************************************/

typedef STEPaggregate * STEPaggregateH;
typedef STEPaggregate * STEPaggregate_ptr;
typedef const STEPaggregate * const_STEPaggregateH;
typedef const STEPaggregate * const_STEPaggregate_ptr;
typedef STEPaggregate_ptr STEPaggregate_var;

class STEPaggregate :  public SingleLinkList 
{
  protected:
    int _null;

  protected:

    virtual Severity ReadValue(istream &in, ErrorDescriptor *err,
			       const TypeDescriptor *elem_type,
			       InstMgr *insts, int addFileId =0,
			       int assignVal =1, int ExchangeFileFormat =1,
			       const char *currSch =0);
  public:

    int is_null() { return _null; }

    virtual Severity AggrValidLevel(const char *value, ErrorDescriptor *err,
			    const TypeDescriptor *elem_type, InstMgr *insts,
			    int optional, char *tokenList, int addFileId =0,
			    int clearError =0);

    virtual Severity AggrValidLevel(istream &in, ErrorDescriptor *err,
			    const TypeDescriptor *elem_type, InstMgr *insts,
			    int optional, char *tokenList, int addFileId =0,
			    int clearError =0);

// INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err =0,
			      const TypeDescriptor *elem_type =0,
			      InstMgr *insts =0, int addFileId = 0);
    virtual Severity STEPread (istream& in, ErrorDescriptor *err,
			       const TypeDescriptor *elem_type =0,
			       InstMgr *insts =0, int addFileId =0,
			       const char *currSch =0);
// OUTPUT
    virtual const char *asStr(std::string& s) const;
    virtual void STEPwrite (ostream& out =cout, const char * =0) const;

//    SingleLinkNode * GetHead () const
//	{ return (STEPnode *) SingleLinkList::GetHead(); }

    virtual SingleLinkNode * NewNode ();  
    void AddNode (SingleLinkNode *);
    void Empty ();
    
    STEPaggregate ();
    virtual ~STEPaggregate ();

// COPY - defined in subtypes
    virtual STEPaggregate& ShallowCopy (const STEPaggregate&);

};									      

/******************************************************************
 ** Class:  GenericAggregate
 ** Description:  This class supports LIST OF: 
 **	SELECT_TYPE, BINARY_TYPE, GENERIC_TYPE, ENUM_TYPE, UNKNOWN_TYPE type
 ******************************************************************/

class GenericAggregate  :  public STEPaggregate 
{
  public:
    virtual SingleLinkNode *NewNode();
    virtual STEPaggregate& ShallowCopy (const STEPaggregate&);

    GenericAggregate();
    virtual ~GenericAggregate();

};
typedef  GenericAggregate * GenericAggregateH;
typedef  GenericAggregate * GenericAggregate_ptr;
typedef  const GenericAggregate * const_GenericAggregateH;
typedef  const GenericAggregate * const_GenericAggregate_ptr;
typedef  GenericAggregate_ptr GenericAggregate_var;

/******************************************************************************
 **
 *****************************************************************************/

class EntityAggregate  :  public  STEPaggregate 
{
  public:
    virtual Severity ReadValue(istream &in, ErrorDescriptor *err, 
			       const TypeDescriptor *elem_type, 
			       InstMgr *insts, int addFileId =0, 
			       int assignVal =1, int ExchangeFileFormat =1,
			       const char *currSch =0);

    virtual  SingleLinkNode *NewNode();
    virtual  STEPaggregate&  ShallowCopy (const STEPaggregate&);

    EntityAggregate ();
    virtual ~EntityAggregate ();

};
typedef   EntityAggregate * EntityAggregateH;
typedef   EntityAggregate * EntityAggregate_ptr;
typedef   const EntityAggregate * const_EntityAggregateH;
typedef   const EntityAggregate * const_EntityAggregate_ptr;
typedef   EntityAggregate_ptr EntityAggregate_var;

/******************************************************************
 ** Class:  SelectAggregate
 ** Description:  this is a minimal representions for a collection of 
 **	SCLP23(Select)
 ******************************************************************/
class SelectAggregate  :  public STEPaggregate 
{
  public:
    virtual Severity ReadValue(istream &in, ErrorDescriptor *err, 
			       const TypeDescriptor *elem_type, 
			       InstMgr *insts, int addFileId =0, 
			       int assignVal =1, int ExchangeFileFormat =1,
			       const char *currSch =0);

    virtual SingleLinkNode *NewNode ();
    virtual STEPaggregate& ShallowCopy  (const STEPaggregate&);

    SelectAggregate ();
    virtual ~SelectAggregate ();

};
typedef  SelectAggregate *  SelectAggregateH;
typedef  SelectAggregate *  SelectAggregate_ptr;
typedef  const SelectAggregate *  const_SelectAggregateH;
typedef  const SelectAggregate *  const_SelectAggregate_ptr;
typedef  SelectAggregate_ptr SelectAggregate_var;

/******************************************************************
 ** Class:  StringAggregate
 ** Description:  This class supports LIST OF STRING type
 ******************************************************************/
class StringAggregate  :  public STEPaggregate 
{
  public:
    virtual SingleLinkNode *NewNode();
    virtual STEPaggregate& ShallowCopy (const STEPaggregate&);

    StringAggregate();
    virtual ~StringAggregate();

};
typedef  StringAggregate * StringAggregateH;
typedef  StringAggregate * StringAggregate_ptr;
typedef  const StringAggregate * const_StringAggregateH;
typedef  const StringAggregate * const_StringAggregate_ptr;
typedef  StringAggregate_ptr StringAggregate_var;


/******************************************************************
 ** Class:  BinaryAggregate
 ** Description:  This class supports LIST OF BINARY type
 ******************************************************************/
class BinaryAggregate  :  public STEPaggregate 
{
  public:
    virtual SingleLinkNode *NewNode();
    virtual STEPaggregate& ShallowCopy (const STEPaggregate&);

    BinaryAggregate();
    virtual ~BinaryAggregate();

};
typedef  BinaryAggregate * BinaryAggregateH;
typedef  BinaryAggregate * BinaryAggregate_ptr;
typedef  const BinaryAggregate * const_BinaryAggregateH;
typedef  const BinaryAggregate * const_BinaryAggregate_ptr;
typedef  BinaryAggregate_ptr BinaryAggregate_var;

/******************************************************************
 ** Class:  EnumAggregate
 ** Description:  this is a minimal representions for a collection of 
 **	SCLP23(Enum)
 ******************************************************************/
class EnumAggregate  :  public STEPaggregate 
{
  public:
    virtual SingleLinkNode *NewNode ();
    virtual STEPaggregate& ShallowCopy  (const STEPaggregate&);

    EnumAggregate ();
  virtual ~EnumAggregate ();

};
typedef  EnumAggregate *  EnumAggregateH;
typedef  EnumAggregate *  EnumAggregate_ptr;
typedef  const EnumAggregate *  const_EnumAggregateH;
typedef  const EnumAggregate *  const_EnumAggregate_ptr;
typedef  EnumAggregate_ptr EnumAggregate_var;

class LOGICALS  : public EnumAggregate  
{
  public:
    virtual SingleLinkNode * NewNode ();  

    LOGICALS();
    virtual ~LOGICALS();

};
typedef  LOGICALS *  LogicalsH;
typedef  LOGICALS *  LOGICALS_ptr;
typedef  const LOGICALS *  const_LogicalsH;
typedef  const LOGICALS *  const_LOGICALS_ptr;
typedef  LOGICALS_ptr LOGICALS_var;
LOGICALS * create_LOGICALS();


class BOOLEANS  : public EnumAggregate  
{
  public:
    virtual SingleLinkNode * NewNode ();  

    BOOLEANS();
    virtual ~BOOLEANS();

};

//typedef  BOOLEANS *  BooleansH;
typedef  BOOLEANS *  BOOLEANS_ptr;
typedef  const BOOLEANS *  const_BOOLEANS_ptr;
typedef  BOOLEANS_ptr BOOLEANS_var;

BOOLEANS * create_BOOLEANS();

class RealAggregate  : public STEPaggregate  {

  public:
    virtual SingleLinkNode *NewNode();
    virtual STEPaggregate& ShallowCopy  (const STEPaggregate&);

    RealAggregate();
    virtual ~RealAggregate();

};
typedef  RealAggregate *  RealAggregateH;
typedef  RealAggregate *  RealAggregate_ptr;
typedef  const RealAggregate *  const_RealAggregateH;
typedef  const RealAggregate *  const_RealAggregate_ptr;
typedef  RealAggregate_ptr RealAggregate_var;

class IntAggregate  : public STEPaggregate  {

  public:
    virtual SingleLinkNode *NewNode();
    virtual STEPaggregate& ShallowCopy  (const STEPaggregate&);

    IntAggregate();
    virtual ~IntAggregate();

};
typedef  IntAggregate *  IntAggregateH;
typedef  IntAggregate *  IntAggregate_ptr;
typedef  const IntAggregate *  const_IntAggregateH;
typedef  const IntAggregate *  const_IntAggregate_ptr;
typedef  IntAggregate_ptr IntAggregate_var;

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

class STEPnode :  public SingleLinkNode  {
  protected:
    int _null;

  public:
    int is_null() { return _null; } 
    void set_null() { _null = 1; } 

//	INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err);

//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite(std::string& s, const char * =0);
    virtual void STEPwrite (ostream& out =cout);
};
typedef  STEPnode *  STEPnodeH;
typedef  const STEPnode *  const_STEPnodeH;

/******************************************************************
 ** Class:  GenericNode
 ** Description:  This class is for the Nodes of GenericAggregates
 ******************************************************************/

class GenericAggrNode  : public STEPnode {
  public:

    SCLundefined value;

  public:
//	INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err);

//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite(std::string& s, const char * =0);
    virtual void 	STEPwrite (ostream& out =cout);

//	CONSTRUCTORS
    GenericAggrNode (const char *str);
    GenericAggrNode (GenericAggrNode& gan);
    GenericAggrNode ();
    ~GenericAggrNode ();

    virtual SingleLinkNode *	NewNode ();

};

/////////////////////////////////////////////////////////////////////////////

class EntityNode  : public STEPnode {
  public:
    SCLP23(Application_instance) * node;

// INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err, 
			      const TypeDescriptor *elem_type,
			      InstMgr *insts, int addFileId = 0);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err, 
			      const TypeDescriptor *elem_type,
			      InstMgr *insts, int addFileId = 0);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err, 
			      const TypeDescriptor *elem_type,
			      InstMgr *insts, int addFileId = 0);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err, 
			      const TypeDescriptor *elem_type,
			      InstMgr *insts, int addFileId = 0);
//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite (std::string& s, const char * =0);
    virtual void 	STEPwrite (ostream& out =cout);

//	CONSTRUCTORS
    EntityNode (SCLP23(Application_instance) * e);
    EntityNode();
    ~EntityNode();

    virtual SingleLinkNode *	NewNode ();

    // Calling these funtions is an error.
    Severity StrToVal(const char *s, ErrorDescriptor *err)
    {
	cerr << "Internal error:  " << __FILE__ <<  __LINE__
	    << "\n" << _POC_ "\n";
	return StrToVal(s, err, 0, 0, 0);
    }
    Severity StrToVal(istream &in, ErrorDescriptor *err)
    {
	cerr << "Internal error:  " << __FILE__ <<  __LINE__
	    << "\n" << _POC_ "\n";
	return StrToVal(in, err, 0, 0, 0);
    }

    Severity STEPread(const char *s, ErrorDescriptor *err)
    {
	cerr << "Internal error:  " << __FILE__ <<  __LINE__
	    << "\n" << _POC_ "\n";
	return STEPread(s, err, 0, 0, 0);
    }
    Severity STEPread(istream &in, ErrorDescriptor *err)
    {
	cerr << "Internal error:  " << __FILE__ <<  __LINE__
	    << "\n" << _POC_ "\n";
	return STEPread(in, err, 0, 0, 0);
    }

};

///////////////////////////////////////////////////////////////////////////

/******************************************************************
 ** Class:  SelectNode
 ** Description:  this is a minimal representions for node in lists of
 **	SCLP23(Select)
 ******************************************************************/

class SelectNode  : public STEPnode {
  public:

    SCLP23(Select) * node;

  public:
//	INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err, 
			      const TypeDescriptor *elem_type,
			      InstMgr *insts, int addFileId = 0);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err, 
			      const TypeDescriptor *elem_type,
			      InstMgr *insts, int addFileId = 0,
			      const char *currSch =0);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err, 
			      const TypeDescriptor *elem_type,
			      InstMgr *insts, int addFileId = 0);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err, 
			      const TypeDescriptor *elem_type,
			      InstMgr *insts, int addFileId = 0,
			      const char *currSch =0);
//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite (std::string& s, const char * =0);
    virtual void 	STEPwrite (ostream& out =cout);

//	CONSTRUCTORS
    SelectNode(SCLP23(Select) * s);
    SelectNode();
    ~SelectNode();

    virtual SingleLinkNode *	NewNode ();

    // Calling these funtions is an error.
    Severity StrToVal(const char *s, ErrorDescriptor *err)
    {
	cerr << "Internal error:  " << __FILE__ <<  __LINE__
	     << "\n" << _POC_ "\n";
	return StrToVal(s, err, 0, 0, 0);
    }
    Severity StrToVal(istream &in, ErrorDescriptor *err)
    {
	cerr << "Internal error:  " << __FILE__ <<  __LINE__
	     << "\n" << _POC_ "\n";
	return StrToVal(in, err, 0, 0, 0);
    }

    Severity STEPread(const char *s, ErrorDescriptor *err)
    {
	cerr << "Internal error:  " << __FILE__ <<  __LINE__
	     << "\n" << _POC_ "\n";
	return STEPread(s, err, 0, 0, 0);
    }
    Severity STEPread(istream &in, ErrorDescriptor *err)
    {
	cerr << "Internal error:  " << __FILE__ <<  __LINE__
	     << "\n" << _POC_ "\n";
	return STEPread(in, err, 0, 0, 0);
    }

};

///////////////////////////////////////////////////////////////////////////

/******************************************************************
 ** Class:  StringNode
 ** Description:  This class is for the Nodes of StringAggregates
 ******************************************************************/

class StringNode  : public STEPnode {
  public:

    SCLP23(String) value;

  public:
//	INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err);

//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite (std::string& s, const char * =0);
    virtual void 	STEPwrite (ostream& out =cout);

//	CONSTRUCTORS
    StringNode(StringNode& sn);
    StringNode (const char * sStr);
    StringNode();
    ~StringNode();

    virtual SingleLinkNode *	NewNode ();

};

///////////////////////////////////////////////////////////////////////////

/******************************************************************
 ** Class:  BinaryNode
 ** Description:  This class is for the Nodes of BinaryAggregates
 ******************************************************************/

class BinaryNode  : public STEPnode {
  public:

    SCLP23(Binary) value;

  public:
//	INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err);

//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite (std::string& s, const char * =0);
    virtual void 	STEPwrite (ostream& out =cout);

//	CONSTRUCTORS
    BinaryNode(BinaryNode& bn);
    BinaryNode (const char * sStr);
    BinaryNode();
    ~BinaryNode();

    virtual SingleLinkNode *	NewNode ();

};

/******************************************************************
 ** Class:  EnumNode
 ** Description:  this is a minimal representions for node in lists of
 **	SCLP23(Enum)
 ******************************************************************/

class EnumNode  : public STEPnode {
  public:

    SCLP23(Enum) * node;

  public:
//	INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err);

//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite (std::string& s, const char * =0);
    virtual void 	STEPwrite (ostream& out =cout);

//	CONSTRUCTORS
    EnumNode(SCLP23(Enum) * e);
    EnumNode();
    ~EnumNode();

    virtual SingleLinkNode *	NewNode ();

};

class RealNode  : public STEPnode {
  public:
    SCLP23(Real) value; // double

  public:
//	INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err);

//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite (std::string& s, const char * =0);
    virtual void 	STEPwrite (ostream& out =cout);

//	CONSTRUCTORS
    RealNode();
    ~RealNode();

    virtual SingleLinkNode *	NewNode ();

};

class IntNode  : public STEPnode {
  public:
    SCLP23(Integer) value; // long int

  public:
//	INPUT
    virtual Severity StrToVal(const char *s, ErrorDescriptor *err);
    virtual Severity StrToVal(istream &in, ErrorDescriptor *err);

    virtual Severity STEPread(const char *s, ErrorDescriptor *err);
    virtual Severity STEPread(istream &in, ErrorDescriptor *err);

//	OUTPUT
    virtual const char *asStr(std::string& s);
    virtual const char *STEPwrite (std::string& s, const char * =0);
    virtual void 	STEPwrite (ostream& out =cout);

//	CONSTRUCTORS
    IntNode();
    ~IntNode();

    virtual SingleLinkNode *	NewNode ();

};

/******************************************************************
 **	The following classes are currently stubs
 **							
**/
/*
class Array  :  public STEPaggregate  {	
  public:								      
    int lowerBound;							      
    int upperBound;							      
};
									      
class Bag  :  public STEPaggregate  {								      
  public:								      
    int min_ele;							      
    int max_ele;							      
};
									      
class List :  public STEPaggregate  {								      
    int min_ele;							      
    int max_ele;							      
    List *prev;								      
};
									      
class Set :								      
public STEPaggregate  {								      
    int cnt;								      
};
*/
#endif 
