#
#			P L O T . T C L
#
#	Widget for producing Unix Plot files of MGED's current view.
#
#	Author - Robert G. Parker
#

check_externs "_mged_opendb _mged_pl"

proc init_plotTool { id } {
    global mged_gui
    global pl_control

    set top .$id.do_plot

    if [winfo exists $top] {
	raise $top
	return
    }

    if ![info exists pl_control($id,file_or_filter)] {
	set pl_control($id,file_or_filter) file
    }

    if ![info exists pl_control($id,file)] {
	regsub \.g$ [_mged_opendb] .plot default_file
	set pl_control($id,file) $default_file
    }

    if ![info exists pl_control($id,filter)] {
	set pl_control($id,filter) ""
    }

    if ![info exists pl_control($id,zclip)] {
	set pl_control($id,zclip) 1
    }

    if ![info exists pl_control($id,2d)] {
	set pl_control($id,2d) 0
    }

    if ![info exists pl_control($id,float)] {
	set pl_control($id,float) 0
    }

    toplevel $top -screen $mged_gui($id,screen)

    frame $top.gridF
    frame $top.gridF2
    frame $top.gridF3

    if {$pl_control($id,file_or_filter) == "file"} {
	set file_state normal
	set filter_state disabled
    } else {
	set filter_state normal
	set file_state disabled
    }

    entry $top.fileE -width 12 -textvar pl_control($id,file)\
	    -state $file_state
    radiobutton $top.fileRB -text "File Name" -anchor w\
	    -value file -variable pl_control($id,file_or_filter)\
	    -command "pl_set_file_state $id"

    entry $top.filterE -width 12 -textvar pl_control($id,filter)\
	    -state $filter_state
    radiobutton $top.filterRB -text "Filter" -anchor w\
	    -value filter -variable pl_control($id,file_or_filter)\
	    -command "pl_set_filter_state $id"

    checkbutton $top.zclip -relief raised -text "Z Clipping"\
	    -variable pl_control($id,zclip)
    checkbutton $top.twoDCB -relief raised -text "2D"\
	    -variable pl_control($id,2d)
    checkbutton $top.floatCB -relief raised -text "Float"\
	    -variable pl_control($id,float)

    button $top.createB -relief raised -text "Create"\
	    -command "do_plot $id"
    button $top.dismissB -relief raised -text "Dismiss"\
	    -command "catch { destroy $top }"

    grid $top.fileE $top.fileRB -sticky "ew" -in $top.gridF -pady 4
    grid $top.filterE $top.filterRB -sticky "ew" -in $top.gridF -pady 4
    grid columnconfigure $top.gridF 0 -weight 1

    grid $top.zclip x $top.twoDCB x $top.floatCB -sticky "ew"\
	    -in $top.gridF2 -padx 4 -pady 4
    grid columnconfigure $top.gridF2 1 -weight 1
    grid columnconfigure $top.gridF2 3 -weight 1

    grid $top.createB x $top.dismissB -sticky "ew" -in $top.gridF3 -pady 4
    grid columnconfigure $top.gridF3 1 -weight 1

    pack $top.gridF $top.gridF2 $top.gridF3 -side top -expand 1 -fill both\
	    -padx 8 -pady 8

    set pxy [winfo pointerxy $top]
    set x [lindex $pxy 0]
    set y [lindex $pxy 1]
    wm geometry $top +$x+$y
    wm title $top "Unix Plot Tool ($id)"
}

proc do_plot { id } {
    global mged_gui
    global pl_control

    cmd_win set $id
    set pl_cmd "_mged_pl"

    if {$pl_control($id,zclip)} {
	append pl_cmd " -zclip"
    }

    if {$pl_control($id,2d)} {
	append pl_cmd " -2d"
    }

    if {$pl_control($id,float)} {
	append pl_cmd " -float"
    }

    if {$pl_control($id,file_or_filter) == "file"} {
	if {$pl_control($id,file) != ""} {
	    if [file exists $pl_control($id,file)] {
		set result [cad_dialog .$id.plotDialog $mged_gui($id,screen)\
			"Overwrite $pl_control($id,file)?"\
			"Overwrite $pl_control($id,file)?"\
			"" 0 OK CANCEL]

		if {$result} {
		    return
		}
	    }
	} else {
	    cad_dialog .$id.plotDialog $mged_gui($id,screen)\
		    "No file name specified!"\
		    "No file name specified!"\
		    "" 0 OK

	    return
	}

	append pl_cmd " $pl_control($id,file)"
    } else {
	if {$pl_control($id,filter) == ""} {
	    cad_dialog .$id.plotDialog $mged_gui($id,screen)\
		    "No filter specified!"\
		    "No filter specified!"\
		    "" 0 OK

	    return
	}

	append pl_cmd " |$pl_control($id,filter)"
    }

    catch {eval $pl_cmd}
}

proc pl_set_file_state { id } {
    set top .$id.do_plot

    $top.fileE configure -state normal
    $top.filterE configure -state disabled

    focus $top.fileE
}

proc pl_set_filter_state { id } {
    set top .$id.do_plot

    $top.filterE configure -state normal
    $top.fileE configure -state disabled

    focus $top.filterE
}