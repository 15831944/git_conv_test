# - Find lex executable and provides a macro to generate custom build rules
#
# The module defines the following variables:
#  LEX_FOUND - true is lex executable is found
#  LEX_EXECUTABLE - the path to the lex executable
#  LEX_LIBRARIES - The lex libraries
#
# If lex is found on the system, the module provides the macro:
#  LEX_TARGET(Name FlexInput FlexOutput [COMPILE_FLAGS <string>])
# which creates a custom command  to generate the <FlexOutput> file from
# the <FlexInput> file.  If  COMPILE_FLAGS option is specified, the next
# parameter is added to the lex  command line. Name is an alias used to
# get  details of  this custom  command.  Indeed the  macro defines  the
# following variables:
#  LEX_${Name}_DEFINED - true is the macro ran successfully
#  LEX_${Name}_OUTPUTS - the source file generated by the custom rule, an
#  alias for FlexOutput
#  LEX_${Name}_INPUT - the lex source file, an alias for ${FlexInput}
#
# Flex scanners oftenly use tokens  defined by Yacc: the code generated
# by Flex  depends of the header  generated by Yacc.   This module also
# defines a macro:
#  ADD_LEX_YACC_DEPENDENCY(FlexTarget YaccTarget)
# which  adds the  required dependency  between a  scanner and  a parser
# where  <FlexTarget>  and <YaccTarget>  are  the  first parameters  of
# respectively LEX_TARGET and YACC_TARGET macros.
#
#  ====================================================================
#  Example:
#
#   find_package(YACC)
#   find_package(LEX)
#
#   YACC_TARGET(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
#   LEX_TARGET(MyScanner lexer.l  ${CMAKE_CURRENT_BIANRY_DIR}/lexer.cpp)
#   ADD_LEX_YACC_DEPENDENCY(MyScanner MyParser)
#
#   include_directories(${CMAKE_CURRENT_BINARY_DIR})
#   add_executable(Foo
#      Foo.cc
#      ${YACC_MyParser_OUTPUTS}
#      ${LEX_MyScanner_OUTPUTS}
#   )
#  ====================================================================
#
#=============================================================================
# Copyright 2010 United States Government as represented by
#                the U.S. Army Research Laboratory.
# Copyright 2009 Kitware, Inc.
# Copyright 2006 Tristan Carel
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# 
# * The names of the authors may not be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

FIND_PROGRAM(LEX_EXECUTABLE flex DOC "path to the lex executable")
IF(NOT LEX_EXECUTABLE)
   FIND_PROGRAM(LEX_EXECUTABLE lex DOC "path to the lex executable")
ENDIF(NOT LEX_EXECUTABLE)
MARK_AS_ADVANCED(LEX_EXECUTABLE)

FIND_LIBRARY(FL_LIBRARY NAMES fl
  DOC "path to the fl library")
MARK_AS_ADVANCED(FL_LIBRARY)
SET(LEX_LIBRARIES ${FL_LIBRARY})

IF(LEX_EXECUTABLE)

  #============================================================
  # LEX_TARGET (public macro)
  #============================================================
  #
  MACRO(LEX_TARGET Name Input Output)
    SET(LEX_TARGET_usage "LEX_TARGET(<Name> <Input> <Output> [COMPILE_FLAGS <string>]")
    IF(${ARGC} GREATER 3)
      IF(${ARGC} EQUAL 5)
        IF("${ARGV3}" STREQUAL "COMPILE_FLAGS")
          SET(LEX_EXECUTABLE_opts  "${ARGV4}")
          SEPARATE_ARGUMENTS(LEX_EXECUTABLE_opts)
        ELSE()
          MESSAGE(SEND_ERROR ${LEX_TARGET_usage})
        ENDIF()
      ELSE()
        MESSAGE(SEND_ERROR ${LEX_TARGET_usage})
      ENDIF()
    ENDIF()

    ADD_CUSTOM_COMMAND(OUTPUT ${Output}
      COMMAND ${LEX_EXECUTABLE}
      ARGS ${LEX_EXECUTABLE_opts} -o${Output} ${Input}
      DEPENDS ${Input}
      COMMENT "[LEX][${Name}] Building scanner with ${LEX_EXECUTABLE}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    SET(LEX_${Name}_DEFINED TRUE)
    SET(LEX_${Name}_OUTPUTS ${Output})
    SET(LEX_${Name}_INPUT ${Input})
    SET(LEX_${Name}_COMPILE_FLAGS ${LEX_EXECUTABLE_opts})
  ENDMACRO(LEX_TARGET)
  #============================================================


  #============================================================
  # ADD_LEX_YACC_DEPENDENCY (public macro)
  #============================================================
  #
  MACRO(ADD_LEX_YACC_DEPENDENCY FlexTarget YaccTarget)

    IF(NOT LEX_${FlexTarget}_OUTPUTS)
      MESSAGE(SEND_ERROR "Flex target `${FlexTarget}' does not exists.")
    ENDIF()

    IF(NOT YACC_${YaccTarget}_OUTPUT_HEADER)
      MESSAGE(SEND_ERROR "Yacc target `${YaccTarget}' does not exists.")
    ENDIF()

    SET_SOURCE_FILES_PROPERTIES(${LEX_${FlexTarget}_OUTPUTS}
      PROPERTIES OBJECT_DEPENDS ${YACC_${YaccTarget}_OUTPUT_HEADER})
  ENDMACRO(ADD_LEX_YACC_DEPENDENCY)
  #============================================================

ENDIF(LEX_EXECUTABLE)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LEX DEFAULT_MSG LEX_EXECUTABLE)

# FindLEX.cmake ends here
