puts "*** Testing 'mirror' command ***"

if {![info exists make_primitives_list]} {  
   source regression_resources.tcl
}

in_eto mirror 1
in mirror_eto2.s eto -10 0 0 -1 -1 -1 20 0 3 3 2.5
comb mirror1.c u mirror_eto1.s u mirror_eto2.s
mirror -d {1 1 1} -p 100 mirror1.c mirror2.c
mirror -d {1 1 4} -o {3 2 2} -p 30 mirror1.c mirror3.c
mirror -x mirror1.c mirror4.c
mirror -y mirror1.c mirror5.c
mirror -z mirror1.c mirror6.c

puts "*** 'mirror' testing completed ***\n"
