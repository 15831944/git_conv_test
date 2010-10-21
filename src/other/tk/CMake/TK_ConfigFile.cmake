# Automate putting variables from tests into a config.h.in file

INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckTypeSize)

MACRO(TK_FUNCTION_EXISTS function var)
  CHECK_FUNCTION_EXISTS(${function} ${var})
  if(CONFIG_H_FILE)
     FILE(APPEND ${CONFIG_H_FILE} "#cmakedefine ${var} 1\n")
  endif(CONFIG_H_FILE)
ENDMACRO(TK_FUNCTION_EXISTS)

MACRO(TK_INCLUDE_FILE filename var)
  CHECK_INCLUDE_FILE(${filename} ${var})
  if(CONFIG_H_FILE)
     FILE(APPEND ${CONFIG_H_FILE} "#cmakedefine ${var} 1\n")
  endif(CONFIG_H_FILE)
ENDMACRO(TK_INCLUDE_FILE)

MACRO(TK_TYPE_SIZE typename var)
  CHECK_TYPE_SIZE(${typename} ${var})
  if(CONFIG_H_FILE)
     FILE(APPEND ${CONFIG_H_FILE} "#cmakedefine HAVE_${var} 1\n")
  endif(CONFIG_H_FILE)
ENDMACRO(TK_TYPE_SIZE)

