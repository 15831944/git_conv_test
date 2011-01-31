# Pretty-printing macro that generates a box around a string and prints the
# resulting message.
MACRO(BOX_PRINT input_string border_string)
	STRING(LENGTH ${input_string} MESSAGE_LENGTH)
	STRING(LENGTH ${border_string} SEPARATOR_STRING_LENGTH)
	WHILE(${MESSAGE_LENGTH} GREATER ${SEPARATOR_STRING_LENGTH})
		SET(SEPARATOR_STRING "${SEPARATOR_STRING}${border_string}")
		STRING(LENGTH ${SEPARATOR_STRING} SEPARATOR_STRING_LENGTH)
	ENDWHILE(${MESSAGE_LENGTH} GREATER ${SEPARATOR_STRING_LENGTH})
	MESSAGE("${SEPARATOR_STRING}")
	MESSAGE("${input_string}")
	MESSAGE("${SEPARATOR_STRING}")
ENDMACRO()

# Windows builds need a DLL variable defined per-library, and BRL-CAD
# uses a fairly standard convention - try and automate the addition of
# the definition.
MACRO(DLL_DEFINE libname)
	IF(MSVC)
		STRING(REGEX REPLACE "lib" "" LOWERCORE "${libname}")
		STRING(TOUPPER ${LOWERCORE} UPPER_CORE)
		add_definitions("-D${UPPER_CORE}_EXPORT_DLL")
	ENDIF(MSVC)
ENDMACRO()

# Core routines for adding executables and libraries to the build and
# install lists of CMake
MACRO(BRLCAD_ADDEXEC execname srcs libs)
  STRING(REGEX REPLACE " " ";" srcslist "${srcs}")
  STRING(REGEX REPLACE " " ";" libslist1 "${libs}")
  STRING(REGEX REPLACE "-framework;" "-framework " libslist "${libslist1}")
  add_executable(${execname} ${srcslist})
  target_link_libraries(${execname} ${libslist})
  INSTALL(TARGETS ${execname} DESTINATION ${BRLCAD_INSTALL_BIN_DIR})
  # Enable extra compiler flags if local executables and/or global options dictate
  SET(LOCAL_COMPILE_FLAGS "")
  FOREACH(extraarg ${ARGN})
	  IF(${extraarg} MATCHES "STRICT" AND BRLCAD-ENABLE_STRICT)
		  SET(LOCAL_COMPILE_FLAGS "${LOCAL_COMPILE_FLAGS} ${STRICT_FLAGS}")
	  ENDIF(${extraarg} MATCHES "STRICT" AND BRLCAD-ENABLE_STRICT)
  ENDFOREACH(extraarg ${ARGN})
  IF(LOCAL_COMPILE_FLAGS)
	  SET_TARGET_PROPERTIES(${execname} PROPERTIES COMPILE_FLAGS ${LOCAL_COMPILE_FLAGS})
  ENDIF(LOCAL_COMPILE_FLAGS)
ENDMACRO(BRLCAD_ADDEXEC execname srcs libs)

# Library macro handles both shared and static libs, so one "BRLCAD_ADDLIB"
# statement will cover both automatically
MACRO(BRLCAD_ADDLIB libname srcs libs)
  STRING(REGEX REPLACE " " ";" srcslist "${srcs}")
  STRING(REGEX REPLACE " " ";" libslist1 "${libs}")
  STRING(REGEX REPLACE "-framework;" "-framework " libslist "${libslist1}")
  DLL_DEFINE(${libname})
  IF(BUILD_SHARED_LIBS)
	  add_library(${libname} SHARED ${srcslist})
	  if(NOT ${libs} MATCHES "NONE")
		  target_link_libraries(${libname} ${libslist})
	  endif(NOT ${libs} MATCHES "NONE")
	  INSTALL(TARGETS ${libname} DESTINATION ${BRLCAD_INSTALL_LIB_DIR})
  ENDIF(BUILD_SHARED_LIBS)
  IF(BUILD_STATIC_LIBS AND NOT MSVC)
	  add_library(${libname}-static STATIC ${srcslist})
	  if(NOT ${libs} MATCHES "NONE")
		  target_link_libraries(${libname}-static ${libslist})
	  endif(NOT ${libs} MATCHES "NONE")
	  IF(NOT WIN32)
		  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES OUTPUT_NAME "${libname}")
	  ENDIF(NOT WIN32)
	  IF(WIN32)
		  # We need the lib prefix on win32, so add it even if our add_library
		  # wrapper function has removed it due to the target name - see
		  # http://www.cmake.org/Wiki/CMake_FAQ#How_do_I_make_my_shared_and_static_libraries_have_the_same_root_name.2C_but_different_suffixes.3F
		  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES PREFIX "lib")
	  ENDIF(WIN32)
	  INSTALL(TARGETS ${libname}-static DESTINATION ${BRLCAD_INSTALL_LIB_DIR})
  ENDIF(BUILD_STATIC_LIBS AND NOT MSVC)

  # Enable extra compiler flags if local libraries and/or global options dictate
  SET(LOCAL_COMPILE_FLAGS "")
  FOREACH(extraarg ${ARGN})
	  IF(${extraarg} MATCHES "STRICT" AND BRLCAD-ENABLE_STRICT)
		  SET(LOCAL_COMPILE_FLAGS "${LOCAL_COMPILE_FLAGS} ${STRICT_FLAGS}")
	  ENDIF(${extraarg} MATCHES "STRICT" AND BRLCAD-ENABLE_STRICT)
  ENDFOREACH(extraarg ${ARGN})
  IF(LOCAL_COMPILE_FLAGS)
	  IF(BUILD_SHARED_LIBS)
		  SET_TARGET_PROPERTIES(${libname} PROPERTIES COMPILE_FLAGS ${LOCAL_COMPILE_FLAGS})
	  ENDIF(BUILD_SHARED_LIBS)
	  IF(BUILD_STATIC_LIBS AND NOT MSVC)
		  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES COMPILE_FLAGS ${LOCAL_COMPILE_FLAGS})
	  ENDIF(BUILD_STATIC_LIBS AND NOT MSVC)
  ENDIF(LOCAL_COMPILE_FLAGS)
ENDMACRO(BRLCAD_ADDLIB libname srcs libs)

MACRO(BRLCAD_ADDDATA datalist targetdir)
	FILE(COPY ${${datalist}} DESTINATION ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir})
	STRING(REGEX REPLACE "/" "_" targetprefix ${targetdir})
	SET(${targetprefix}_dependslist "")
	FOREACH(filename ${${datalist}})
		IF(BRLCAD-ENABLE_DATA_TARGETS)
			STRING(REGEX REPLACE "/" "_" filestring ${filename})
			ADD_CUSTOM_COMMAND(
				OUTPUT ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename}
				COMMAND ${CMAKE_COMMAND} -E copy	${CMAKE_CURRENT_SOURCE_DIR}/${filename} ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename}
				DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${filename}
				)
			ADD_CUSTOM_TARGET(${targetprefix}_${filestring}_cp ALL DEPENDS ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename})
			SET(${targetprefix}_dependslist ${${targetprefix}_dependslist}	${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename})
		ENDIF(BRLCAD-ENABLE_DATA_TARGETS)
		INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${filename} DESTINATION ${${CMAKE_PROJECT_NAME}_INSTALL_DATA_DIR}/${targetdir})
	ENDFOREACH(filename ${${datalist}})
ENDMACRO(BRLCAD_ADDDATA datalist targetdir)

MACRO(BRLCAD_ADDFILE filename targetdir)
	FILE(COPY ${filename} DESTINATION ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir})
	STRING(REGEX REPLACE "/" "_" targetprefix ${targetdir})
	IF(BRLCAD-ENABLE_DATA_TARGETS)
		STRING(REGEX REPLACE "/" "_" filestring ${filename})
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename}
			COMMAND ${CMAKE_COMMAND} -E copy	${CMAKE_CURRENT_SOURCE_DIR}/${filename} ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${filename}
			)
		ADD_CUSTOM_TARGET(${targetprefix}_${filestring}_cp ALL DEPENDS ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename})
	ENDIF(BRLCAD-ENABLE_DATA_TARGETS)
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${filename} DESTINATION ${${CMAKE_PROJECT_NAME}_INSTALL_DATA_DIR}/${targetdir})
ENDMACRO(BRLCAD_ADDFILE datalist targetdir)


