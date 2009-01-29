#!/bin/sh

# Ensure /bin/sh
export PATH || (echo "This isn't sh."; sh $0 $*; kill $$)

# save the precious args
ARGS="$*"
NAME_OF_THIS=`basename $0`
PATH_TO_THIS=`dirname $0`
THIS="$PATH_TO_THIS/$NAME_OF_THIS"

MGED="/Users/cyapp/brlcad-install/bin/mged"
if test ! -f "$MGED" ; then
    echo "Unable to find mged, aborting"
    exit 1  
fi

rm -f mged.g *.mged_regress mged.log

# Create mged.g as an empty db file.
$MGED -c mged.g <<EOF > mged.log 2>&1
EOF

# The difficulty with creating a self-contained test script for MGED
# is the number of commands that make sense only after OTHER commands
# have already been executed - this unavoidably complicates the creation
# and execution of testing scripts.  It also makes it more difficult to
# be sure all scripts have been tested - a side-by-side comparison of
# a complete command list with this script would not be very helpful
# unless the commands are organized in some fashion.  An alphabetical
# organization makes the most sense from a comparison standpoint as
# well as providing a "correct" place for new commands, but rules
# out a "one-shot" in-order sh script.  In order to address both
# concerns, each individual command will create its own $CMDNAME.mged_regress
# script file containing the per-command logic, and those scripts
# will be combined at the end to form the "runnable" master script.
# This means placement logic is separate from the individual command
# logic, and each command will have two entries to maintain.  The check
# performed to make sure no command is missing from the final script is
# to make sure the line count of the final mged.mged_regress file matches
# the sum of all the individual #CMDNAME.mged_regress files.  The final
# mged.mged_regress file will be organized according to functionality - 
# for example, all geometry creation commands will be grouped - in, make,
# special purpose commands like tire, etc.  Similarly, geometry editing
# commands like translate, rot, oed, etc. will be grouped.

#
#                  3 P T A R B
#

cat > 3ptarb.mged_regress << EOF
3ptarb 3ptarb_test.s 0 0 0 10 0 0 10 10 0 z 0 10 6
EOF


#
#                  A R B
#

cat > arb.mged_regress << EOF
arb arb_test1.s 30 3
arb arb_test2.s 55 20
EOF


#
#          B U I L D _ R E G I O N
#
# build_region requires a large number of target objects with the correct
# naming conventions - rather than do this manually, clone is used - this
# means clone should be tested before build_region

cat > build_region.mged_regress << EOF
make -s 10 br_epa.s epa
clone -p 20 0 0 -r 0 0 10 -n 36 br_epa.s
build_region br_epa 0 10000
build_region -a 42 br_epa 2000 10000
EOF

#
#                       C L O N E
#

cat > clone.mged_regress << EOF
make -s 10 eto_a.s eto
clone -a 10 10 10 10 eto_a.s
g clone1.c eto_a.s*
cp eto_a.s eto_b.s
clone -b 10 100 100 100 eto_b.s
g clone2.c eto_b.s*
cp eto_b.s eto_c.s
clone -i 30 -m x 10 -n 1 eto_c.s 
g clone3.c eto_c.s*
cp eto_c.s eto_d.s
clone -i 5 -p 30 30 30 -r 10 10 15 -n 20 eto_d.s
g clone4.c eto_d.s*
cp eto_d.s eto_e.s
clone -i 11 -t 1 1 1 -n 20 eto_e.s
g clone5.c eto_e.s*
cp clone5.c clone_a.c
clone -i 2 -p 5 0 10 -r 10 20 30 -n 20 clone_a.c
g comb_clone.c clone_a.c*
EOF


#
#                        C O M B 
#
# Test commands for c/comb - because comb acts on existing primitives,
# first set up primitives for comb to act on.

cat > comb.mged_regress << EOF
make -s 10 comb_sph1.s sph
make -s 20 comb_sph2.s sph
make -s 30 comb_sph3.s sph
comb comb1.c u comb_sph1.s u comb_sph2.s u comb_sph3.s
comb comb2.c u comb_sph1.s + comb_sph2.s + comb_sph3.s
comb comb3.c u comb_sph3.s - comb_sph2.s - comb_sph1.s
comb comb4.c u comb_sph2.s - comb_sph1.s + comb_sph3.s
c comb5.c u comb_sph2.s - comb_sph1.s + comb_sph3.s
comb comb6.c u comb2.c + comb1.c u comb3.c u comb4.c - comb5.c
EOF

#
#                       C P
#
# Shallow object copy - test after make and comb but before
# clone

cat > cp.mged_regress << EOF
make -s 1 cp_sph.s sph
comb cp_comb.c u cp_sph.s
cp cp_sph.s cp_copy_sph.s
cp cp_comb.c cp_copy_comb.c
EOF

#
#                  C P I
#
# cpi leaves the new shape in an edit state - for
# scripting purposes, an accept is added to leave
# editing state and allow for further operations

cat > cpi.mged_regress << EOF
in cpi-base-cyl.s rcc 1 2 3 10 10 10 3.5
cpi cpi-base-cyl.s cpi-cyl.s
accept
EOF



#
#                        G 
#
# Test commands for g - because g acts on existing primitives,
# first set up primitives and combinations to act on.

cat > g.mged_regress << EOF
make -s 10 g_sph1.s sph
make -s 20 g_sph2.s sph
make -s 30 g_sph3.s sph
comb g_comb1.c u comb_sph1.s u comb_sph2.s u comb_sph3.s
g g1.c g_sph1.s g_sph2.s g_sph3.s
g g2.c g_sph1.s g_sph2.s g_comb1.c
EOF


#
#                         I N 
#
# Test commands for in - because the behavior of this command must be
# checked on a per-primitive basis, one make command exists for each
# primitive type with working make support.

# Note:  need more sophisticated bot input example

cat > in.mged_regress << EOF
in in_arb4.s arb4 3 -3 -3 3 0 -3 3 0 0 0 0 -3
in in_arb5.s arb5 1 0 0 1 2 0 3 2 0 3 0 0 1.5 1.5 5  
in in_arb6.s arb6 2 -.5 -.5 2 0 -.5 2 0 0 2 -.5 0 2.5 -.3 -.5 2.5 -.3 0
in in_arb7.s arb7 3.25 -1.25 -0.75 3.25 -0.25 -0.75 3.25 -0.25 0.25 3.25 -1.25 -0.25 2.25 -1.25 -0.75 2.25 -0.25 -0.75 2.25 -0.25 -0.25
in in_arb8.s arb8 10 -9 -8 10 -1 -8 10 -1 0 10 -9 0 3 -9 -8 3 -1 -8 3 -1 0 3 -9 0
in in_arbn.s arbn 8 1 0 0 1000 -1 0 0 1000 0 1 0 1000 0 -1 0 1000 0 0 1 1000 0 0 -1 1000 0.57735 0.57735 0.57735 1000 -0.57735 -0.57735 -0.57735 200
in in_ars.s ars 3 3 0 0 0 0 0 100 100 0 100 100 100 100 0 0 200 
in in_bot.s bot 4 4 2 1 0 0 0 10 10 0 -10 10 0 0 10 10 0 1 2 1 2 3 3 2 0 0 3 1 
in in_ehy.s ehy 0 0 0 0 10 10 10 0 0 10 3
in in_ell.s ell 10 0 0 -12 0 0 0 -3 0 0 0 5 
in in_ell1.s ell1 3 2 8 3 -1 8 4
in in_epa.s epa 0 0 0 3 0 0 0 5 0 3 
in in_eto.s eto 0 0 0 1 1 1 10 0 2 2 1.5
in in_grip.s grip 0 0 0 3 0 0 6 
in in_half.s half 1 1 1 5
in in_hyp.s hyp 0 0 0 0 0 10 3 0 0 4 .3 
in in_part.s part 0 0 0 0 0 16 4 2
in in_pipe.s pipe 4 0 0 0 3 5 6 0 0 3 3 5 7 3 4 8 2 6 10 8 8 10 0 6 8
in in_rcc.s rcc 0 0 0 3 3 30 7
in in_rec.s rec 0 0 0 3 3 10 10 0 0 0 3 0
in in_rhc.s rhc 0 0 0 0 0 10 3 0 0 4 3
in in_rpc.s rpc 0 0 0 0 0 4 0 1 0 3 
in in_rpp.s rpp 0 30 -3 12 -1 22 
in in_sph.s sph 42 42 42 42
in in_tec.s tec 0 0 0 0 0 10 5 0 0 0 3 0 .6
in in_tgc.s tgc 0 0 0 0 0 10 5 0 0 0 8 0 2 9 
in in_tor.s tor 0 0 0 1 1 3 5 2
in in_trc.s trc 0 0 0 0 0 10 4 7
EOF

# Unsupported
# in in_nmg.s nmg  
# in in_sketch.s sketch

# Because sketch does not currently have in command support, it is necessary to
# to provide an example sketch for extrude.
cat >> in.mged_regress << EOF
put {extrude_test_sketch} sketch V {10 20 30} A {1 0 0} B {0 1 0} VL { {250 0} {500 0} {500 500} {0 500} {0 250} {250 250} {125 125} {0 125} {125 0} {200 200} } SL { { bezier D 4 P { 4 7 9 8 0 } } { line S 0 E 1 } { line S 1 E 2 } { line S 2 E 3 } { line S 3 E 4 } { carc S 6 E 5 R -1 L 0 O 0 } }
in in_extrude.s extrude 0 0 0 0 0 1000 10 0 0 0 10 0 extrude_test_sketch 1 
EOF





#
#                          M A K E
#
# Test commands for make - because the behavior of this command must be
# checked on a per-primitive basis, one make command exists for each
# primitive type with working make support.

cat > make.mged_regress << EOF
make make_arb4.s arb4
make make_arb5.s arb5
make make_arb6.s arb6
make make_arb7.s arb7
make make_arb8.s arb8
make make_arbn.s arbn
make make_ars.s ars
make make_bot.s bot
make make_ehy.s ehy
make make_ell.s ell
make make_ell1.s ell1
make make_epa.s epa
make make_eto.s eto
make make_extrude.s extrude
make make_grip.s grip
make make_half.s half
make make_nmg.s nmg
make make_part.s part
make make_pipe.s pipe
make make_rcc.s rcc
make make_rec.s rec
make make_rhc.s rhc
make make_rpc.s rpc
make make_rpp.s rpp
make make_sketch.s sketch
make make_sph.s sph
make make_tec.s tec
make make_tgc.s tgc
make make_tor.s tor
make make_trc.s trc
EOF

# Unimplemented as of 7.12.6
# make make_hyp.s hyp


# Next, test the effects of explicitly setting the origin
#cat >> make.mged_regress << EOF
#make -o 0,0,100 make_arb4_o.s arb4
#make -o 0,0,100 make_arb5_o.s arb5
#make -o 0,0,100 make_arb6_o.s arb6
#make -o 0,0,100 make_arb7_o.s arb7
#make -o 0,0,100 make_arb8_o.s arb8
#make -o 0,0,100 make_arbn_o.s arbn
#make -o 0,0,100 make_ars_o.s ars
#make -o 0,0,100 make_bot_o.s bot
#make -o 0,0,100 make_ehy_o.s ehy
#make -o 0,0,100 make_ell_o.s ell
#make -o 0,0,100 make_ell1_o.s ell1
#make -o 0,0,100 make_epa_o.s epa
#make -o 0,0,100 make_eto_o.s eto
#make -o 0,0,100 make_extrude_o.s extrude
#make -o 0,0,100 make_grip_o.s grip
#make -o 0,0,100 make_half_o.s half
#make -o 0,0,100 make_nmg_o.s nmg
#make -o 0,0,100 make_part_o.s part
#make -o 0,0,100 make_pipe_o.s pipe
#make -o 0,0,100 make_rcc_o.s rcc
#make -o 0,0,100 make_rec_o.s rec
#make -o 0,0,100 make_rhc_o.s rhc
#make -o 0,0,100 make_rpc_o.s rpc
#make -o 0,0,100 make_rpp_o.s rpp
#make -o 0,0,100 make_sketch_o.s sketch
#make -o 0,0,100 make_sph_o.s sph
#make -o 0,0,100 make_tec_o.s tec
#make -o 0,0,100 make_tgc_o.s tgc
#make -o 0,0,100 make_tor_o.s tor
#make -o 0,0,100 make_trc_o.s trc
#EOF

# And finally, test the size factor option
cat >> make.mged_regress << EOF
make -s 42 make_arb4_s.s arb4
make -s 42 make_arb5_s.s arb5
make -s 42 make_arb6_s.s arb6
make -s 42 make_arb7_s.s arb7
make -s 42 make_arb8_s.s arb8
make -s 42 make_arbn_s.s arbn
make -s 42 make_ars_s.s ars
make -s 42 make_bot_s.s bot
make -s 42 make_ehy_s.s ehy
make -s 42 make_ell_s.s ell
make -s 42 make_ell1_s.s ell1
make -s 42 make_epa_s.s epa
make -s 42 make_eto_s.s eto
make -s 42 make_extrude_s.s extrude
make -s 42 make_grip_s.s grip
make -s 42 make_half_s.s half
make -s 42 make_nmg_s.s nmg
make -s 42 make_part_s.s part
make -s 42 make_pipe_s.s pipe
make -s 42 make_rcc_s.s rcc
make -s 42 make_rec_s.s rec
make -s 42 make_rhc_s.s rhc
make -s 42 make_rpc_s.s rpc
make -s 42 make_rpp_s.s rpp
make -s 42 make_sketch_s.s sketch
make -s 42 make_sph_s.s sph
make -s 42 make_tec_s.s tec
make -s 42 make_tgc_s.s tgc
make -s 42 make_tor_s.s tor
make -s 42 make_trc_s.s trc
EOF

#
#                  M A K E _ B B
#
# In order to test this routine for each primitive,
# the primitives from the make and in command tests will
# be reused.  BE SURE THE make AND in TESTS RUN BEFORE
# RUNNING THIS TEST. It also uses comb so run after the
# comb test.

cat > make_bb.mged_regress << EOF
make_bb arb4_bb.s make_arb4.s
make_bb arb5_bb.s make_arb5.s
make_bb arb6_bb.s make_arb6.s
make_bb arb7_bb.s make_arb7.s
make_bb arb8_bb.s make_arb8.s
make_bb arbn_bb.s make_arbn.s
make_bb ars_bb.s make_ars.s
make_bb bot_bb.s make_bot.s
make_bb ehy_bb.s make_ehy.s
make_bb ell_bb.s make_ell.s
make_bb ell1_bb.s make_ell1.s
make_bb epa_bb.s make_epa.s
make_bb eto_bb.s make_eto.s
make_bb extrude_bb.s in_extrude.s
make_bb grip_bb.s make_grip.s
make_bb half_bb.s make_half.s
make_bb nmg_bb.s make_nmg.s
make_bb part_bb.s make_part.s
make_bb pipe_bb.s make_pipe.s
make_bb rcc_bb.s make_rcc.s
make_bb rec_bb.s make_rec.s
make_bb rhc_bb.s make_rhc.s
make_bb rpc_bb.s make_rpc.s
make_bb rpp_bb.s make_rpp.s
make_bb sketch_bb.s extrude_test_sketch 
make_bb sph_bb.s make_sph.s
make_bb tec_bb.s make_tec.s
make_bb tgc_bb.s make_tgc.s
make_bb tor_bb.s make_tor.s
make_bb trc_bb.s make_trc.s
comb bb_prim1.c u make_arb4.s u make_arb5.s u make_arb6.s u make_arb7.s u make_arb8.s u make_arbn.s
make_bb comb_bb1.s bb_prim1.c
comb bb_prim2.c u make_ars.s u make_bot.s u make_ehy.s u make_ell.s u make_ell1.s u make_epa.s
make_bb comb_bb2.s bb_prim2.c
comb bb_prim3.c u make_eto.s u in_extrude.s u make_grip.s u make_nmg.s u make_part.s u make_pipe.s
make_bb comb_bb3.s bb_prim3.c
comb bb_prim4.c u make_rcc.s u make_rec.s u make_rhc.s u make_rpc.s u make_rpp.s
make_bb comb_bb4.s bb_prim4.c
comb bb_prim5.c u extrude_test_sketch u make_sph.s u make_tec.s u make_tgc.s u make_tor.s u make_trc.s
make_bb comb_bb5.s bb_prim5.c 
EOF


#
#                  M I R R O R
#

cat > mirror.mged_regress << EOF

EOF




#
#                       M V
#
cat > mv.mged_regress << EOF
make -s 40 mv_sph.s sph
make -s 40 mv_comb_sph.s sph
comb mv_comb.c u mv_comb_sph.s
mv mv_sph.s moved_sph.s
mv mv_comb.c moved_comb.c
EOF


#
#                       M V A L L
#
cat > mvall.mged_regress << EOF
make -s 40 mvall_sph.s sph
make -s 40 mvall_comb_sph.s sph
comb mvall_comb_1.c u mvall_comb_sph.s
comb moved_all_comb_2.c u mvall_comb_sph.s
mvall mvall_sph.s movedall_sph.s
mvall mvall_comb1.c movedall_comb1.c
mvall mvall_comb_sph.s moved_all_comb_sph.s
EOF

#
#                  P R E F I X
#

cat > prefix.mged_regress << EOF
EOF


#
#                        R 
#
# Test commands for r command to build regions - because region building
# acts on existing primitives and combinations, first set up primitives
# and combinations for testing.

cat > r.mged_regress << EOF
make -s 10 r_sph1.s sph
make -s 20 r_sph2.s sph
make -s 30 r_sph3.s sph
comb r_comb1.c u sph1.s u sph2.s u sph3.s
comb r_comb2.c u sph1.s + sph2.s + sph3.s
r r1.r u r_sph1.s u r_sph2.s u r_sph3.s
r r2.r u r_sph1.s u r_comb1.c
r r3.r u r_comb1.c u r_comb2.c
EOF




##################################################################
#
#   Test script assembly - assemble each of the individual
#   scripts into a final mged.mged_regress that makes sense
#
##################################################################

# Create empty file to hold commands - all other MGED commands will 
# be appended to this file.
cat > mged.mged_regress << EOF
EOF

#
#      GEOMETRIC INPUT COMMANDS
#
cat in.mged_regress >> mged.mged_regress
cat make.mged_regress >> mged.mged_regress
cat 3ptarb.mged_regress >> mged.mged_regress
cat arb.mged_regress >> mged.mged_regress
cat comb.mged_regress >> mged.mged_regress
cat g.mged_regress >> mged.mged_regress
cat r.mged_regress >> mged.mged_regress
cat make_bb.mged_regress >> mged.mged_regress
cat cp.mged_regress >> mged.mged_regress
cat cpi.mged_regress >> mged.mged_regress
cat mv.mged_regress >> mged.mged_regress
cat mvall.mged_regress >> mged.mged_regress
cat clone.mged_regress >> mged.mged_regress
cat build_region.mged_regress >> mged.mged_regress

#
#      TRANSLATION/ROTATION COMMANDS
#

##################################################################
#
# Run master script to generate log and .g file for diffing
#
##################################################################

$MGED -c mged.g <<EOF >> mged.log 2>&1
`cat mged.mged_regress`
EOF
