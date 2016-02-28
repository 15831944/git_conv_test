# Tests for the various image format conversions supported
# by BRL-CAD

macro(Add_Image_Test testname testtype depends_list)
  CMAKE_PARSE_ARGUMENTS(${testname} "" "INPUT_IMAGE;CONTROL_IMAGE;TARGET_IMAGE;LOG_FILE" "" ${ARGN})
  set(TARGET_NAME ${testname})
  if (NOT "${testtype}" STREQUAL "IMPORT" AND NOT "${testtype}" STREQUAL "EXPORT")
    message(FATAL_ERROR "Valid image test types are IMPORT and EXPORT: got ${testtype}")
  endif (NOT "${testtype}" STREQUAL "IMPORT" AND NOT "${testtype}" STREQUAL "EXPORT")
  set(INPUT_IMAGE ${${testname}_INPUT_IMAGE})
  set(CONTROL_IMAGE ${${testname}_CONTROL_IMAGE})
  set(TARGET_IMAGE ${${testname}_TARGET_IMAGE})
  set(LOG_FILE ${${testname}_LOG_FILE})
  set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${testname}.done)
  if ("${testtype}" STREQUAL "IMPORT")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/regress-icv_import.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/regress-${testname}.cmake @ONLY)
  endif ("${testtype}" STREQUAL "IMPORT")
  if ("${testtype}" STREQUAL "EXPORT")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/regress-icv_export.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/regress-${testname}.cmake @ONLY)
  endif ("${testtype}" STREQUAL "EXPORT")
  add_custom_command(
    OUTPUT ${STAMP_FILE}
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/regress-${testname}.cmake
    DEPENDS pixcmp icv ${depends_list} ${INPUT_IMAGE} ${CONTROL_IMAGE}
    )

  add_test(NAME regress-icv-${testname} COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/regress-${testname}.cmake)
  add_custom_target(regress-icv-${testname} DEPENDS ${depends_list} ${STAMP_FILE})
  set_target_properties(regress-icv-${testname} PROPERTIES FOLDER "BRL-CAD Regression Tests")
  Add_Regression_Test(icv-${testname} "${depends_list}")

  add_dependencies(regress-icv regress-icv-${testname})

  DISTCLEAN(
    ${STAMP_FILE}
    ${LOG_FILE}
    ${TARGET_IMAGE}
    ${CMAKE_CURRENT_BINARY_DIR}/${testname}.done
    )
endmacro()

add_custom_target(regress-icv)
set_target_properties(regress-icv PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)
set_target_properties(regress-icv PROPERTIES FOLDER "BRL-CAD Regression Tests")


Add_Image_Test(pix_pix IMPORT ""
  INPUT_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/../../bench/ref/m35.pix"
  CONTROL_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/../../bench/ref/m35.pix"
  TARGET_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/m35_icv.pix"
  LOG_FILE "${CMAKE_CURRENT_BINARY_DIR}/pix_pix.log"
  )

Add_Image_Test(png_pix IMPORT ""
  INPUT_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/m35.png"
  CONTROL_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/../../bench/ref/m35.pix"
  TARGET_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/m35_png.pix"
  LOG_FILE "${CMAKE_CURRENT_BINARY_DIR}/png_pix.log"
  )

Add_Image_Test(pix_png EXPORT ""
  INPUT_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/../../bench/ref/m35.pix"
  CONTROL_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/m35.png"
  TARGET_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/m35_pix.png"
  LOG_FILE "${CMAKE_CURRENT_BINARY_DIR}/pix_png.log"
  )


CMAKEFILES(
  m35.png
  )

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8