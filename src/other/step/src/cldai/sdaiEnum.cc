
//#include <Enum.h>
//#include <Enumeration.h>

#include <sdai.h>

/*
* NIST STEP Core Class Library
* clstepcore/Enumeration.cc
* April 1997
* K. C. Morris
* David Sauder

* Development of this software was funded by the United States Government,
* and is not subject to copyright.
*/

/* $Id: sdaiEnum.cc,v 1.5 1997/11/05 21:59:14 sauderd DP3.1 $  */

//static char rcsid[] ="$Id: sdaiEnum.cc,v 1.5 1997/11/05 21:59:14 sauderd DP3.1 $";

#include <sstream>

//#ifndef SCLP23(TRUE)
//#ifndef SCLP23(FALSE)
// Josh L, 3/31/95
// These constants have to be initialized outside the SDAI struct.  They
// are initialized here instead of in the header file in order to avoid
// multiple inclusions when building SCL applications.
/*
const SCLP23(BOOLEAN) SCLP23(TRUE)( BTrue );
const SCLP23(BOOLEAN) SCLP23(FALSE)( BFalse );
const SCLP23(BOOLEAN) SCLP23(UNSET)( BUnset );
const SCLP23(LOGICAL) SCLP23(UNKNOWN)( LUnknown );
*/
//#endif
//#endif


///////////////////////////////////////////////////////////////////////////////
// class Logical
///////////////////////////////////////////////////////////////////////////////

SCLP23( LOGICAL )::SCLP23_NAME( LOGICAL )( const char * val ) {
    set_value( val );
}

SCLP23( LOGICAL )::SCLP23_NAME( LOGICAL )( Logical state ) {
    set_value( state );
}

SCLP23( LOGICAL )::SCLP23_NAME( LOGICAL )( const SCLP23( LOGICAL )& source ) {
    set_value( source.asInt() );
}

SCLP23( LOGICAL )::SCLP23_NAME( LOGICAL )( int i ) {
    if( i == 0 ) {
        v =  LFalse ;
    } else {
        v =  LTrue ;
    }
}

SCLP23( LOGICAL )::~SCLP23_NAME( LOGICAL )() {
}

const char *
SCLP23( LOGICAL )::Name() const {
    return "Logical";
}

int
SCLP23( LOGICAL )::no_elements() const {
    return 3;
}

const char *
SCLP23( LOGICAL )::element_at( int n ) const {
    switch( n )  {
        case  LUnknown :
            return "U";
        case  LFalse :
            return "F";
        case  LTrue :
            return "T";
        default:
            return "UNSET";
    }
}

int
SCLP23( LOGICAL )::exists() const { // return 0 if unset otherwise return 1
    return !( v == 2 );
}

void
SCLP23( LOGICAL )::nullify() { // change the receiver to an unset status
    v = 2;
}

SCLP23( LOGICAL )::operator  Logical() const  {
    switch( v ) {
        case  LFalse :
            return  LFalse ;
        case  LTrue :
            return  LTrue ;
        case  LUnknown :
            return  LUnknown ;
        case  LUnset :
        default:
            return  LUnset ;
    }
}

SCLP23( LOGICAL ) &
SCLP23( LOGICAL )::operator= ( const SCLP23( LOGICAL )& t ) {
    set_value( t.asInt() );
    return *this;
}

SCLP23( LOGICAL ) SCLP23( LOGICAL )::operator ==( const SCLP23( LOGICAL )& t ) const {
    if( v == t.asInt() ) {
        return  LTrue ;
    }
    return  LFalse ;
}

int
SCLP23( LOGICAL )::set_value( const int i )  {
    if( i > no_elements() + 1 )  {
        v = 2;
        return v;
    }
    const char * tmp = element_at( i );
    if( tmp[0] != '\0' ) {
        return ( v = i );
    }
    // otherwise
    cerr << "(OLD Warning:) invalid enumeration value " << i
         << " for " <<  Name() << "\n";
    DebugDisplay();
    return  no_elements() + 1 ;
//    return  ENUM_NULL ;

}

int
SCLP23( LOGICAL )::set_value( const char * n )  {
    //  assigns the appropriate value based on n
    if( !n || ( !strcmp( n, "" ) ) ) {
        nullify();
        return asInt();
    }

    int i = 0;
    std::string tmp;
    while( ( i < ( no_elements() + 1 ) )  &&
            ( strcmp( ( char * )StrToUpper( n, tmp ),  element_at( i ) ) != 0 ) ) {
        ++i;
    }
    if( ( no_elements() + 1 ) == i ) { //  exhausted all the possible values
        nullify();
        return v;
    }
    v = i;
    return v;

}


Severity
SCLP23( LOGICAL )::ReadEnum( istream & in, ErrorDescriptor * err, int AssignVal,
                             int needDelims ) {
    if( AssignVal ) {
        set_null();
    }

    std::string str;
    char c;
    char messageBuf[512];
    messageBuf[0] = '\0';

    int validDelimiters = 1;

    in >> ws; // skip white space

    if( in.good() ) {
        in.get( c );
        if( c == '.' || isalpha( c ) ) {
            if( c == '.' ) {
                in.get( c ); // push past the delimiter
                // since found a valid delimiter it is now invalid until the
                //   matching ending delim is found
                validDelimiters = 0;
            }

            // look for UPPER
            if( in.good() && ( isalpha( c ) || c == '_' ) ) {
                str += c;
                in.get( c );
            }

            // look for UPPER or DIGIT
            while( in.good() && ( isalnum( c ) || c == '_' ) ) {
                str += c;
                in.get( c );
            }
            // if character is not the delimiter unread it
            if( in.good() && ( c != '.' ) ) {
                in.putback( c );
            }

            // a value was read
            if( str.length() > 0 ) {
                int i = 0;
                const char * strval = str.c_str();
                std::string tmp;
                while( ( i < ( no_elements() + 1 ) )  &&
                        ( strcmp( ( char * )StrToUpper( strval, tmp ),
                                  element_at( i ) ) != 0 ) ) {
                    ++i;
                }
                if( ( no_elements() + 1 ) == i ) {
                    //  exhausted all the possible values
                    err->GreaterSeverity( SEVERITY_WARNING );
                    err->AppendToDetailMsg( "Invalid Enumeration value.\n" );
                    err->AppendToUserMsg( "Invalid Enumeration value.\n" );
                } else {
                    if( AssignVal ) {
                        v = i;
                    }
                }

                // now also check the delimiter situation
                if( c == '.' ) { // if found ending delimiter
                    // if expecting delim (i.e. validDelimiter == 0)
                    if( !validDelimiters ) {
                        validDelimiters = 1; // everything is fine
                    } else { // found ending delimiter but no initial delimiter
                        validDelimiters = 0;
                    }
                }
                // didn't find any delimiters at all and need them.
                else if( needDelims ) {
                    validDelimiters = 0;
                }

                if( !validDelimiters ) {
                    err->GreaterSeverity( SEVERITY_WARNING );
                    if( needDelims )
                        sprintf( messageBuf,
                                 "Enumerated value has invalid period delimiters.\n" );
                    else
                        sprintf( messageBuf,
                                 "Mismatched period delimiters for enumeration.\n" );
                    err->AppendToDetailMsg( messageBuf );
                    err->AppendToUserMsg( messageBuf );
                }
                return err->severity();
            }
            // found valid or invalid delimiters with no associated value
            else if( ( c == '.' ) || !validDelimiters ) {
                err->GreaterSeverity( SEVERITY_WARNING );
                err->AppendToDetailMsg(
                    "Enumerated has valid or invalid period delimiters with no value.\n"
                );
                err->AppendToUserMsg(
                    "Enumerated has valid or invalid period delimiters with no value.\n"
                );
                return err->severity();
            } else { // no delims and no value
                err->GreaterSeverity( SEVERITY_INCOMPLETE );
            }

        } else if( ( c == ',' ) || ( c == ')' ) ) {
            in.putback( c );
            err->GreaterSeverity( SEVERITY_INCOMPLETE );
        } else {
            in.putback( c );
            err->GreaterSeverity( SEVERITY_WARNING );
            sprintf( messageBuf, "Invalid enumeration value.\n" );
            err->AppendToDetailMsg( messageBuf );
            err->AppendToUserMsg( messageBuf );
        }
    } else { // hit eof (assuming there was no error state for istream passed in)
        err->GreaterSeverity( SEVERITY_INCOMPLETE );
    }
    return err->severity();
}

///////////////////////////////////////////////////////////////////////////////
// class BOOLEAN  Jan 97
///////////////////////////////////////////////////////////////////////////////

const char *
SCLP23( BOOLEAN )::Name() const {
    return "Bool";
}

SCLP23( BOOLEAN )::SCLP23_NAME( BOOLEAN )( char * val ) {
    set_value( val );
}

SCLP23( BOOLEAN )::SCLP23_NAME( BOOLEAN )( Boolean state ) {
    set_value( state );
}

SCLP23( BOOLEAN )::SCLP23_NAME( BOOLEAN )( const SCLP23_NAME( BOOLEAN )& source ) {
    set_value( source.asInt() );
}

SCLP23( BOOLEAN )::~SCLP23_NAME( BOOLEAN )() {
}

int
SCLP23( BOOLEAN )::no_elements() const {
    return 2;
}

SCLP23( BOOLEAN )::SCLP23_NAME( BOOLEAN )( int i ) {
    if( i == 0 ) {
        v =  BFalse ;
    } else {
        v =  BTrue ;
    }
}

SCLP23( BOOLEAN )::SCLP23_NAME( BOOLEAN )( const SCLP23( LOGICAL )& val )  {
    if( val.asInt() == LUnknown ) {
        // this should set error code sdaiVT_NVLD i.e. Invalid value type.
        v = BUnset;
        return;
    }
    set_value( val );
}

SCLP23( BOOLEAN )::operator  Boolean() const  {
    switch( v ) {
        case  BFalse :
            return  BFalse ;
        case  BTrue :
            return  BTrue ;
        case  BUnset :
        default:
            return  BUnset ;
    }
}

SCLP23( BOOLEAN ) &
SCLP23( BOOLEAN )::operator= ( const SCLP23( LOGICAL )& t ) {
    set_value( t.asInt() );
    return *this;
}

SCLP23( BOOLEAN ) &
SCLP23( BOOLEAN )::operator= ( const  Boolean t ) {
    v = t;
    return *this;
}

const char *
SCLP23( BOOLEAN )::element_at( int n )  const {
    switch( n )  {
        case  BFalse :
            return "F";
        case  BTrue :
            return "T";
        default:
            return "UNSET";
    }
}

SCLP23( LOGICAL ) SCLP23( BOOLEAN )::operator ==( const SCLP23( LOGICAL )& t ) const {
    if( v == t.asInt() ) {
        return  LTrue ;
    }
    return  LFalse ;
}

///////////////////////////////////////////////////////////////////////////////

SCLP23( Enum )::SCLP23_NAME( Enum )() {
    v = 0;
}

SCLP23( Enum )::~SCLP23_NAME( Enum )() {
}

int
SCLP23( Enum )::put( int val ) {
    return set_value( val );
}

int
SCLP23( Enum )::put( const char * n ) {
    return set_value( n );
}

int
SCLP23( Enum )::exists() const { // return 0 if unset otherwise return 1
    return !( v > no_elements() );
}

void
SCLP23( Enum )::nullify() // change the receiver to an unset status
// unset is generated to be 1 greater than last element
{
    set_value( no_elements() + 1 );
}

/******************************************************************
 ** Procedure:  DebugDisplay
 ** Parameters:  ostream& out
 ** Returns:
 ** Description:  prints out some information on the enumerated
 **               item for debugging purposes
 ** Side Effects:
 ** Status:  ok 2/1/91
 ******************************************************************/
void
SCLP23( Enum )::DebugDisplay( ostream & out ) const {
    std::string tmp;
    out << "Current " << Name() << " value: " << endl
        << "  cardinal: " <<  v  << endl
        << "  string: " << asStr( tmp ) << endl
        << "  Part 21 file format: ";
    STEPwrite( out );
    out << endl;

    out << "Valid values are: " << endl;
    int i = 0;
    while( i < ( no_elements() + 1 ) ) {
        out << i << " " << element_at( i ) << endl;
        i++;
    }
    out << "\n";
}

// Read an Enumeration value
// ENUMERATION = "." UPPER { UPPER | DIGIT } "."
// *note* UPPER is defined as alpha or underscore.
// returns: Severity of the error.
// error message and error Severity is written to ErrorDescriptor *err.
// int AssignVal is:
// true => value is assigned to the SCLP23(Enum);
// true or false => value is read and appropriate error info is set and
//  returned.
// int needDelims is:
// false => absence of the period delimiters is not an error;
// true => delimiters must be valid;
// true or false => non-matching delimiters are flagged as an error

Severity
SCLP23( Enum )::ReadEnum( istream & in, ErrorDescriptor * err, int AssignVal,
                          int needDelims ) {
    if( AssignVal ) {
        set_null();
    }

    std::string str;
    char c;
    char messageBuf[512];
    messageBuf[0] = '\0';

    int validDelimiters = 1;

    in >> ws; // skip white space

    if( in.good() ) {
        in.get( c );
        if( c == '.' || isalpha( c ) ) {
            if( c == '.' ) {
                in.get( c ); // push past the delimiter
                // since found a valid delimiter it is now invalid until the
                //   matching ending delim is found
                validDelimiters = 0;
            }

            // look for UPPER
            if( in.good() && ( isalpha( c ) || c == '_' ) ) {
                str += c;
                in.get( c );
            }

            // look for UPPER or DIGIT
            while( in.good() && ( isalnum( c ) || c == '_' ) ) {
                str += c;
                in.get( c );
            }
            // if character is not the delimiter unread it
            if( in.good() && ( c != '.' ) ) {
                in.putback( c );
            }

            // a value was read
            if( str.length() > 0 ) {
                int i = 0;
                const char * strval = str.c_str();
                std::string tmp;
                while( ( i < no_elements() )  &&
                        ( strcmp( ( char * )StrToUpper( strval, tmp ),
                                  element_at( i ) ) != 0 ) ) {
                    ++i;
                }
                if( no_elements() == i ) {
                    //  exhausted all the possible values
                    err->GreaterSeverity( SEVERITY_WARNING );
                    err->AppendToDetailMsg( "Invalid Enumeration value.\n" );
                    err->AppendToUserMsg( "Invalid Enumeration value.\n" );
                } else {
                    if( AssignVal ) {
                        v = i;
                    }
                }

                // now also check the delimiter situation
                if( c == '.' ) { // if found ending delimiter
                    // if expecting delim (i.e. validDelimiter == 0)
                    if( !validDelimiters ) {
                        validDelimiters = 1; // everything is fine
                    } else { // found ending delimiter but no initial delimiter
                        validDelimiters = 0;
                    }
                }
                // didn't find any delimiters at all and need them.
                else if( needDelims ) {
                    validDelimiters = 0;
                }

                if( !validDelimiters ) {
                    err->GreaterSeverity( SEVERITY_WARNING );
                    if( needDelims )
                        sprintf( messageBuf,
                                 "Enumerated value has invalid period delimiters.\n" );
                    else
                        sprintf( messageBuf,
                                 "Mismatched period delimiters for enumeration.\n" );
                    err->AppendToDetailMsg( messageBuf );
                    err->AppendToUserMsg( messageBuf );
                }
                return err->severity();
            }
            // found valid or invalid delimiters with no associated value
            else if( ( c == '.' ) || !validDelimiters ) {
                err->GreaterSeverity( SEVERITY_WARNING );
                err->AppendToDetailMsg(
                    "Enumerated has valid or invalid period delimiters with no value.\n"
                );
                err->AppendToUserMsg(
                    "Enumerated has valid or invalid period delimiters with no value.\n"
                );
                return err->severity();
            } else { // no delims and no value
                err->GreaterSeverity( SEVERITY_INCOMPLETE );
            }

        } else if( ( c == ',' ) || ( c == ')' ) ) {
            in.putback( c );
            err->GreaterSeverity( SEVERITY_INCOMPLETE );
        } else {
            in.putback( c );
            err->GreaterSeverity( SEVERITY_WARNING );
            sprintf( messageBuf, "Invalid enumeration value.\n" );
            err->AppendToDetailMsg( messageBuf );
            err->AppendToUserMsg( messageBuf );
        }
    } else { // hit eof (assuming there was no error state for istream passed in)
        err->GreaterSeverity( SEVERITY_INCOMPLETE );
    }
    return err->severity();
}

Severity
SCLP23( Enum )::StrToVal( const char * s, ErrorDescriptor * err, int optional ) {
    istringstream in( ( char * )s ); // sz defaults to length of s

    ReadEnum( in, err, 1, 0 );
    if( ( err->severity() == SEVERITY_INCOMPLETE ) && optional ) {
        err->severity( SEVERITY_NULL );
    }

    return err->severity();
}

// reads an enumerated value in STEP file format
Severity
SCLP23( Enum )::STEPread( const char * s, ErrorDescriptor * err, int optional ) {
    istringstream in( ( char * )s );
    return STEPread( in, err, optional );
}

// reads an enumerated value in STEP file format
Severity
SCLP23( Enum )::STEPread( istream & in, ErrorDescriptor * err, int optional ) {
    ReadEnum( in, err, 1, 1 );
    if( ( err->severity() == SEVERITY_INCOMPLETE ) && optional ) {
        err->severity( SEVERITY_NULL );
    }

    return err->severity();
}


const char *
SCLP23( Enum )::asStr( std::string & s ) const  {
    if( exists() ) {
        return const_cast<char *>( ( s = element_at( v ) ).c_str() );
    } else {
        return "";
    }
}

void
SCLP23( Enum )::STEPwrite( ostream & out )  const  {
    if( is_null() ) {
        out << '$';
    } else {
        std::string tmp;
        out << "." <<  asStr( tmp ) << ".";
    }
}

const char *
SCLP23( Enum )::STEPwrite( std::string & s ) const {
    if( is_null() ) {
        s.clear();
    } else {
        std::string tmp;
        s = ".";
        s.append( asStr( tmp ) );
        s.append( "." );
    }
    return const_cast<char *>( s.c_str() );
}

/******************************************************************
 ** Procedure:  set_elements
 ** Parameters:
 ** Returns:
 ** Description:
 ** Side Effects:
 ** Status:
 ******************************************************************/
#ifdef OBSOLETE
void
SCLP23( Enum )::set_elements( const char * const e [] )  {
    elements = e;
}
#endif
Severity
SCLP23( Enum )::EnumValidLevel( istream & in, ErrorDescriptor * err,
                                int optional, char * tokenList,
                                int needDelims, int clearError ) {
    if( clearError ) {
        err->ClearErrorMsg();
    }

    in >> ws; // skip white space
    char c = ' ';
    c = in.peek();
    if( c == '$' || in.eof() ) {
        if( !optional ) {
            err->GreaterSeverity( SEVERITY_INCOMPLETE );
        }
        if( in ) {
            in >> c;
        }
        CheckRemainingInput( in, err, "enumeration", tokenList );
        return err->severity();
    } else {
        ErrorDescriptor error;

        ReadEnum( in, &error, 0, needDelims );
        CheckRemainingInput( in, &error, "enumeration", tokenList );

        Severity sev = error.severity();
        if( sev < SEVERITY_INCOMPLETE ) {
            err->AppendToDetailMsg( error.DetailMsg() );
            err->AppendToUserMsg( error.UserMsg() );
            err->GreaterSeverity( error.severity() );
        } else if( sev == SEVERITY_INCOMPLETE && !optional ) {
            err->GreaterSeverity( SEVERITY_INCOMPLETE );
        }
    }
    return err->severity();
}

Severity
SCLP23( Enum )::EnumValidLevel( const char * value, ErrorDescriptor * err,
                                int optional, char * tokenList,
                                int needDelims, int clearError ) {
    istringstream in( ( char * )value );
    return EnumValidLevel( in, err, optional, tokenList, needDelims,
                           clearError );
}

/******************************************************************
 ** Procedure:  set_value
 ** Parameters:  char * n  OR  in i  -- value to be set
 ** Returns:  value set
 ** Description:  sets the value of an enumerated attribute
 **     case is not important in the character based version
 **     if value is not acceptable, a warning is printed and
 **     processing continues
 ** Side Effects:
 ** Status:  ok 2.91
 ******************************************************************/
int
SCLP23( Enum )::set_value( const char * n )  {
    if( !n || ( !strcmp( n, "" ) ) ) {
        nullify();
        return asInt();
    }

    int i = 0;
    std::string tmp;
    while( ( i < no_elements() )  &&
            ( strcmp( ( char * )StrToUpper( n, tmp ),  element_at( i ) ) != 0 ) ) {
        ++i;
    }
    if( no_elements() == i )  {   //  exhausted all the possible values
        return v = no_elements() + 1; // defined as UNSET
    }
    v = i;
    return v;

}

//  set_value is the same function as put
int
SCLP23( Enum )::set_value( const int i )  {
    if( i > no_elements() )  {
        v = no_elements() + 1;
        return v;
    }
    const char * tmp = element_at( i );
    if( tmp[0] != '\0' ) {
        return ( v = i );
    }
    // otherwise
    cerr << "(OLD Warning:) invalid enumeration value " << i
         << " for " <<  Name() << "\n";
    DebugDisplay();
    return  no_elements() + 1 ;
}

SCLP23( Enum ) &
SCLP23( Enum )::operator= ( const int i ) {
    put( i );
    return *this;
}

SCLP23( Enum ) &
SCLP23( Enum )::operator= ( const SCLP23( Enum )& Senum ) {
    put( Senum.asInt() );
    return *this;
}

ostream & operator<< ( ostream & out, const SCLP23( Enum )& a ) {
    std::string tmp;
    out << a.asStr( tmp );
    return out;

}


#ifdef pojoldStrToValNstepRead

Severity
SCLP23( Enum )::StrToVal( const char * s, ErrorDescriptor * err, int optional ) {
    const char * sPtr = s;
    while( isspace( *sPtr ) ) {
        sPtr++;
    }
    if( *sPtr == '\0' ) {
        if( optional ) {
            err->GreaterSeverity( SEVERITY_NULL );
            return SEVERITY_NULL;
        } else {
            err->GreaterSeverity( SEVERITY_INCOMPLETE );
            return SEVERITY_INCOMPLETE;
        }
    } else if( *sPtr == '.' ) { // look for initial period delimiter
        return STEPread( sPtr, err );
    } else {
        // look for ending period delimiter (an error)
        char * periodLoc = strchr( sPtr, '.' );
        if( periodLoc ) {
            // found an ending period w/out initial period
            char * tmp = new char[strlen( sPtr ) + 1];
            strcpy( tmp, sPtr );
            tmp[periodLoc - sPtr] = '\0'; // write over ending period
            err->GreaterSeverity( SEVERITY_WARNING );
            err->AppendToDetailMsg(
                "Ending period delimiter without initial period delimiter.\n" );
            err->AppendToUserMsg(
                "Ending period delimiter without initial period delimiter.\n" );
            delete [] tmp;
            if( ValidLevel( sPtr, err, optional ) ) {
                // remaining value is valid so assign it
                put( tmp );
                return SEVERITY_WARNING;
            } else {
                err->AppendToDetailMsg( "Invalid Enumerated value.\n" );
                err->AppendToUserMsg( "Invalid Enumerated value.\n" );
                return SEVERITY_WARNING;
            }
        }
        // no surrounding delimiters
        else if( ValidLevel( sPtr, err, optional ) ) {
            // value is valid so assign it
            put( sPtr );
            return SEVERITY_NULL;
        } else {
            err->AppendToDetailMsg( "Invalid Enumerated value.\n" );
            err->AppendToUserMsg( "Invalid Enumerated value.\n" );
            return SEVERITY_WARNING;
        }
    }
}


Severity
SCLP23( Enum )::STEPread( istream & in, ErrorDescriptor * err, int optional ) {
    char enumValue [BUFSIZ];
    char c;
    char errStr[BUFSIZ];
    errStr[0] = '\0';

    err->severity( SEVERITY_NULL ); // assume ok until error happens
    in >> c;
    switch( c ) {
        case '.':
            in.getline( enumValue, BUFSIZ, '.' ); // reads but does not store the .
            /*
              // gcc 2.3.3 - It does and should read the . It doesn't store it DAS 4/27/93
                  char * pos = index(enumValue, '.');
                  if (pos) *pos = '\0';
                  //  NON-STANDARD (GNUism)  getline should not retrieve .
                  //  function gcount is unavailable
            */
            if( in.fail() ) {
                err->GreaterSeverity( SEVERITY_WARNING );
                err->AppendToUserMsg(
                    "Missing ending period delimiter for enumerated value.\n" );
                err->AppendToDetailMsg(
                    "Missing ending period delimiter for enumerated value.\n" );
            }
            if( ValidLevel( enumValue, err, optional ) == SEVERITY_NULL ) {
                set_value( enumValue );
            } else {
                err->AppendToDetailMsg( "Invalid enumerated value.\n" );
                err->GreaterSeverity( SEVERITY_WARNING );
                set_value( ENUM_NULL );
            }
            break;

        case ',':   // for next attribute or next aggregate value?
        case ')':   // for end of aggregate value?
        default:
            in.putback( c );
            set_value( ENUM_NULL );
            if( optional ) {
                err->GreaterSeverity( SEVERITY_NULL );
            } else {
                err->GreaterSeverity( SEVERITY_INCOMPLETE );
            }
            break;
    }
    return err->severity();
}

#endif

