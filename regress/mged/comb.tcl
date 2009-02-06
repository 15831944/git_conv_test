puts "*** Testing 'comb' command ***"

if {![info exists make_primitives_list]} {  
   source regression_resources.tcl
}

in_sph comb 1
in_sph comb 2
in_sph comb 3
comb comb1.c u comb_sph1.s u comb_sph2.s u comb_sph3.s
comb comb2.c u comb_sph1.s + comb_sph2.s + comb_sph3.s
comb comb3.c u comb_sph3.s - comb_sph2.s - comb_sph1.s
comb comb4.c u comb_sph2.s - comb_sph1.s + comb_sph3.s
comb comb5.c u comb_sph2.s - comb_sph1.s + comb_sph3.s
comb comb6.c u comb2.c + comb1.c u comb3.c u comb4.c - comb5.c

puts "*** 'comb' testing completed ***\n"
