# Include directories needed by librt users
set(RT_INCLUDE_DIRS
  ${BRLCAD_BINARY_DIR}/include
  ${BRLCAD_SOURCE_DIR}/include
  ${BU_INCLUDE_DIRS}
  ${BN_INCLUDE_DIRS}
  ${BG_INCLUDE_DIRS}
  ${NMG_INCLUDE_DIRS}
  ${BREP_INCLUDE_DIRS}
  ${P2T_INCLUDE_DIR}
  ${OPENNURBS_INCLUDE_DIRS}
  )

if(BRLCAD_ENABLE_OPENCL)
  add_definitions(-DUSE_OPENCL)
  set(RT_INCLUDE_DIRS ${RT_INCLUDE_DIRS} ${OPENCL_INCLUDE_DIRS})
endif (BRLCAD_ENABLE_OPENCL)

# locally used but not needed by users of the library
set(RT_LOCAL_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${REGEX_INCLUDE_DIR}
  ${VDS_INCLUDE_DIR}
  ${GDIAM_INCLUDE_DIR}
  ${LZ4_INCLUDE_DIR}
  )

BRLCAD_LIB_INCLUDE_DIRS(rt RT_INCLUDE_DIRS RT_LOCAL_INCLUDE_DIRS)

set(LIBRT_SOURCES
  attributes.c
  bbox.c
  binunif/binunif.c
  binunif/db5_bin.c
  bool.c
  bundle.c
  cache.c
  cmd.c
  comb/comb.c
  comb/comb_brep.cpp
  comb/comb_mirror.c
  comb/db_comb.c
  constraint.c
  cut.c
  cut_hlbvh.c
  db5_alloc.c
  db5_attr.c
  db5_attr_registry.cpp
  db5_io.c
  db5_size.cpp
  db5_scan.c
  db5_types.c
  db_alloc.c
  db_anim.c
  db_corrupt.c
  db_diff.c
  db_flags.c
  db_flip.c
  db_fullpath.c
  db_inmem.c
  db_io.c
  db_lookup.c
  db_match.c
  db_open.c
  db_scan.c
  db_tree.c
  db_walk.c
  dir.c
  dspline.c
  fortray.c
  globals.c
  htbl.c
  ls.c
  mater.c
  memalloc.c
  mkbundle.c
  op.c
  pr.c
  prep.c
  primitives/annot/annot.c
  primitives/arb8/arb8.c
  primitives/arb8/arb8_brep.cpp
  primitives/arb8/arb8_mirror.c
  primitives/arb8/arb_edit.c
  primitives/arbn/arbn.c
  primitives/arbn/arbn_brep.cpp
  primitives/arbn/arbn_mirror.c
  primitives/ars/ars.c
  primitives/ars/ars_brep.cpp
  primitives/ars/ars_mirror.c
  primitives/bot/bot.c
  primitives/bot/bot_brep.cpp
  primitives/bot/bot_edge.c
  primitives/bot/bot_mirror.c
  primitives/bot/bot_oriented_bbox.cpp
  primitives/bot/btg.c
  primitives/bot/btgf.c
  primitives/bot/decimate.c
  primitives/brep/brep.cpp
  primitives/brep/brep_cdt.cpp
  primitives/brep/brep_debug.cpp
  primitives/bspline/bspline.cpp
  primitives/bspline/bspline_brep.cpp
  primitives/bspline/bspline_mirror.c
  primitives/cline/cline.c
  primitives/cline/cline_brep.cpp
  primitives/datum/datum.c
  primitives/dsp/dsp.c
  primitives/dsp/dsp_brep.cpp
  primitives/dsp/dsp_mirror.c
  primitives/ebm/ebm.c
  primitives/ebm/ebm_brep.cpp
  primitives/ebm/ebm_mirror.c
  primitives/ehy/ehy.c
  primitives/ehy/ehy_brep.cpp
  primitives/ell/ell.c
  primitives/ell/ell_brep.cpp
  primitives/ell/ell_mirror.c
  primitives/epa/epa.c
  primitives/epa/epa_brep.cpp
  primitives/epa/epa_mirror.c
  primitives/eto/eto.c
  primitives/eto/eto_brep.cpp
  primitives/eto/eto_mirror.c
  primitives/extrude/extrude.c
  primitives/extrude/extrude_brep.cpp
  primitives/generic.c
  primitives/grip/grip.c
  primitives/grip/grip_brep.cpp
  primitives/grip/grip_mirror.c
  primitives/half/half.c
  primitives/half/half_brep.cpp
  primitives/half/half_mirror.c
  primitives/hf/hf.c
  primitives/hf/hf_brep.cpp
  primitives/hrt/hrt.c
  primitives/hrt/hrt_mirror.c
  primitives/hyp/hyp.c
  primitives/hyp/hyp_brep.cpp
  primitives/hyp/hyp_mirror.c
  primitives/joint/joint.c
  primitives/joint/joint_brep.cpp
  primitives/joint/joint_mirror.c
  primitives/metaball/metaball.c
  primitives/metaball/metaball_tri.c
  primitives/mirror.c
  primitives/nmg/nmg.c
  primitives/nmg/nmg_brep.cpp
  primitives/nmg/nmg_mirror.c
  primitives/nmg/nmg_tri_mc.c
  primitives/obj_adjust.c
  primitives/obj_curve.c
  primitives/obj_describe.c
  primitives/obj_export.c
  primitives/obj_free.c
  primitives/obj_get.c
  primitives/obj_ifree.c
  primitives/obj_import.c
  primitives/obj_make.c
  primitives/obj_mirror.c
  primitives/obj_norm.c
  primitives/obj_params.c
  primitives/obj_plot.c
  primitives/obj_prep.c
  primitives/obj_print.c
  primitives/obj_shot.c
  primitives/obj_tess.c
  primitives/obj_tnurb.c
  primitives/obj_uv.c
  primitives/obj_vshot.c
  primitives/obj_xform.c
  primitives/part/part.c
  primitives/part/part_brep.cpp
  primitives/part/part_mirror.c
  primitives/pipe/pipe.c
  primitives/pipe/pipe_brep.cpp
  primitives/pipe/pipe_mirror.c
  primitives/pnts/pnts.c
  primitives/pnts/pnts_brep.cpp
  primitives/poly/poly.c
  primitives/poly/poly_brep.cpp
  primitives/poly/poly_mirror.c
  primitives/primitive_util.c
  primitives/rec/rec.c
  primitives/revolve/revolve.c
  primitives/revolve/revolve_brep.cpp
  primitives/rhc/rhc.c
  primitives/rhc/rhc_brep.cpp
  primitives/rhc/rhc_mirror.c
  primitives/rpc/rpc.c
  primitives/rpc/rpc_brep.cpp
  primitives/rpc/rpc_mirror.c
  primitives/script/script.c
  primitives/sketch/sketch.c
  primitives/sketch/sketch_brep.cpp
  primitives/sketch/sketch_tess.cpp
  primitives/sph/sph.c
  primitives/sph/sph_brep.cpp
  primitives/submodel/submodel.c
  primitives/superell/superell.c
  primitives/superell/superell_brep.cpp
  primitives/superell/superell_mirror.c
  primitives/table.cpp
  primitives/tgc/tgc.c
  primitives/tgc/tgc_brep.cpp
  primitives/tgc/tgc_mirror.c
  primitives/tor/tor.c
  primitives/tor/tor_brep.cpp
  primitives/tor/tor_mirror.c
  primitives/vol/vol.c
  primitives/vol/vol_brep.cpp
  primitives/vol/vol_mirror.c
  primitives/xxx/xxx.c
  reduce.c
  reduce_db.cpp
  regionfix.c
  roots.c
  search.c
  shoot.c
  tol.c
  transform.c
  tree.c
  uv.c
  vers.c
  vlist.c
  vshoot.c
  wdb.c
  )
if(BRLCAD_ENABLE_GCT)
  set(LIBRT_SOURCES
    ${LIBRT_SOURCES}
    primitives/bot/gct_decimation/auxiliary/mmbinsort.c
    primitives/bot/gct_decimation/auxiliary/mmbitmap.c
    primitives/bot/gct_decimation/auxiliary/mm.c
    primitives/bot/gct_decimation/auxiliary/mmhash.c
    primitives/bot/gct_decimation/meshoptimization.c
    primitives/bot/gct_decimation/meshoptimizer.c
    primitives/bot/gct_decimation/meshdecimation.c
    primitives/bot/gct_decimation/meshdecimationSSE2.c
    primitives/bot/gct_decimation/meshdecimationSSE3.c
    primitives/bot/gct_decimation/meshdecimationSSE4p1.c
    )
endif(BRLCAD_ENABLE_GCT)

set(librt_ignored_files
  cache.h
  columnparse.c
  librt_private.h
  prcomb.c
  primitives/bot/bot_edge.h
  primitives/bot/bot_wireframe.cpp
  primitives/bot/btg.h
  primitives/bot/g_bot_include.c
  primitives/bot/gct_decimation/auxiliary/cc.h
  primitives/bot/gct_decimation/auxiliary/mm.h
  primitives/bot/gct_decimation/auxiliary/mmatomic.h
  primitives/bot/gct_decimation/auxiliary/mmbinsort.h
  primitives/bot/gct_decimation/auxiliary/mmbitmap.h
  primitives/bot/gct_decimation/auxiliary/mmhash.h
  primitives/bot/gct_decimation/auxiliary/mmthread.h
  primitives/bot/gct_decimation/meshdecimation.h
  primitives/bot/gct_decimation/meshoptimization.h
  primitives/bot/gct_decimation/meshoptimizer.h
  primitives/bot/tie.c
  primitives/bot/tie_kdtree.c
  primitives/bot/tieprivate.h
  primitives/brep/brep_debug.h
  primitives/brep/brep_local.h
  primitives/datum/datum.h
  primitives/dsp/dsp.h
  primitives/fixpt.h
  primitives/metaball/metaball.h
  primitives/revolve/revolve.h
  primitives/sph/benchmark.sh
  primitives/xxx/xxx.h

  primitives/bool.cl
  primitives/common.cl
  primitives/rt.cl
  primitives/solver.cl

  primitives/arb8/arb8_shot.cl
  primitives/bot/bot_shot.cl
  primitives/ebm/ebm_shot.cl
  primitives/ehy/ehy_shot.cl
  primitives/ell/ell_shot.cl
  primitives/epa/epa_shot.cl
  primitives/eto/eto_shot.cl
  primitives/hrt/hrt_shot.cl
  primitives/part/part_shot.cl
  primitives/rec/rec_shot.cl
  primitives/rhc/rhc_shot.cl
  primitives/rpc/rpc_shot.cl
  primitives/sph/sph_shot.cl
  primitives/tgc/tgc_shot.cl
  primitives/tor/tor_shot.cl

  search.h
  subd_test_bot.asc
  test_bot2nurbs.cpp
  test_brepreport.cpp
  test_dbio.c
  test_nurbsfit.cpp
  test_root3-subd.cpp
  timer-nt.c
  timerunix.c
  uvpoints.cpp
  )
if(NOT BRLCAD_ENABLE_GCT)
  set(librt_ignored_files
    ${librt_ignored_files}
    primitives/bot/gct_decimation/auxiliary/mmbinsort.c
    primitives/bot/gct_decimation/auxiliary/mmbitmap.c
    primitives/bot/gct_decimation/auxiliary/mm.c
    primitives/bot/gct_decimation/auxiliary/mmhash.c
    primitives/bot/gct_decimation/meshoptimization.c
    primitives/bot/gct_decimation/meshoptimizer.c
    primitives/bot/gct_decimation/meshdecimation.c
    primitives/bot/gct_decimation/meshdecimationSSE2.c
    primitives/bot/gct_decimation/meshdecimationSSE3.c
    primitives/bot/gct_decimation/meshdecimationSSE4p1.c
    )
endif(NOT BRLCAD_ENABLE_GCT)
CMAKEFILES(${librt_ignored_files})

set(CL_FILES
  primitives/bool.cl
  primitives/common.cl
  primitives/rt.cl
  primitives/solver.cl

  primitives/arb8/arb8_shot.cl
  primitives/bot/bot_shot.cl
  primitives/ebm/ebm_shot.cl
  primitives/ehy/ehy_shot.cl
  primitives/ell/ell_shot.cl
  primitives/epa/epa_shot.cl
  primitives/eto/eto_shot.cl
  primitives/hrt/hrt_shot.cl
  primitives/part/part_shot.cl
  primitives/rec/rec_shot.cl
  primitives/rhc/rhc_shot.cl
  primitives/rpc/rpc_shot.cl
  primitives/sph/sph_shot.cl
  primitives/tgc/tgc_shot.cl
  primitives/tor/tor_shot.cl
  )

BRLCAD_ADDDATA(nurb_example.c sample_applications)
BRLCAD_ADDDATA(raydebug.tcl sample_applications)
BRLCAD_ADDDATA(CL_FILES opencl)

if(MSVC)
  set(LIBRT_SOURCES ${LIBRT_SOURCES} timer-nt.c)
else(MSVC)
  set(LIBRT_SOURCES ${LIBRT_SOURCES} timer42.c)
endif(MSVC)

if(BRLCAD_ENABLE_OPENCL)
  set(OPENCL_LIBS ${OPENCL_LIBRARIES})
endif(BRLCAD_ENABLE_OPENCL)

if(BRLCAD_ENABLE_SPR)
  set(SPR_LIB libSPR)
endif(BRLCAD_ENABLE_SPR)

BRLCAD_ADDLIB(librt "${LIBRT_SOURCES}" "${OPENCL_LIBS};${GDIAM_LIBRARY};${VDS_LIBRARY};libbrep;libnmg;libbg;libbn;libbu;${OPENNURBS_LIBRARIES};${P2T_LIBRARY};${SPR_LIB};${REGEX_LIBRARY};${LZ4_LIBRARY};${WINSOCK_LIB};${RPCRT_LIB};${STDCXX_LIBRARIES}")

set_target_properties(librt PROPERTIES VERSION 20.0.1 SOVERSION 20)
if(CPP_DLL_DEFINES)
  set_property(TARGET librt APPEND PROPERTY COMPILE_DEFINITIONS "TIE_DLL_EXPORTS")
  set_property(TARGET librt APPEND PROPERTY COMPILE_DEFINITIONS "DB5_DLL_EXPORTS")
  set_property(TARGET librt APPEND PROPERTY COMPILE_DEFINITIONS "VDS_DLL_IMPORTS")
  set_property(TARGET librt APPEND PROPERTY COMPILE_DEFINITIONS "GDIAM_DLL_IMPORTS")
  set_property(TARGET librt APPEND PROPERTY COMPILE_DEFINITIONS "LZ4_DLL_IMPORT=1")
endif(CPP_DLL_DEFINES)

# The following is build code for test applications used during
# development
#
#include_directories(
#  ${CMAKE_SOURCE_DIR}/src/other/Eigen
#  ${CMAKE_CURRENT_SOURCE_DIR}/../libbrep
#  )
#
#BRLCAD_ADDEXEC(test_bot2nurbs test_bot2nurbs.cpp "librt;libwdb;libbrep;libbu" NO_STRICT NO_INSTALL)
#BRLCAD_ADDEXEC(test_brepreport test_brepreport.cpp "librt;libwdb;libbrep;libbu" NO_STRICT NO_INSTALL)
#BRLCAD_ADDEXEC(test_nurbsfit test_nurbsfit.cpp "librt;libwdb;libbrep;libbu" NO_STRICT NO_INSTALL)
#BRLCAD_ADDEXEC(test_root3-subd test_root3-subd.cpp "librt;libwdb;libbrep;libbu" NO_STRICT NO_INSTALL)
#BRLCAD_ADDEXEC(test_surfacetree test_surfacetree.cpp "librt;libwdb;libbrep;libbu" NO_STRICT NO_INSTALL)

add_subdirectory(tests)

CMAKEFILES(CMakeLists.txt)
CMAKEFILES(db_fp.cpp)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
