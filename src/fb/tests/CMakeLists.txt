if(BRLCAD_ENABLE_TK)
    BRLCAD_ADDEXEC(fbthreadtest fbthreadtest.c "libfb;${TCL_TK_LIBRARY}" NO_INSTALL)
  SET_TARGET_PROPERTIES(fbthreadtest PROPERTIES COMPILE_DEFINITIONS "IF_TK")
  SET_TARGET_PROPERTIES(fbthreadtest PROPERTIES LINKER_LANGUAGE C)
else(BRLCAD_ENABLE_TK)
  CMAKEFILES(fbthreadtest.c)
endif(BRLCAD_ENABLE_TK)

CMAKEFILES(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

