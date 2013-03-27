# Copyright (c) 2005-2011 Tim Baker

# A nice feature of the element type "window" is the -clip option.
set ::clip 1

namespace eval DemoBigList {}

proc DemoBigList::Init {T} {

    variable Priv

    set Priv(noise) 0

    #
    # Configure the treectrl widget
    #

    $T configure -selectmode extended \
	-showroot no -showbuttons no -showlines no \
	-showrootlines no

    if {$::clip} {
	$T configure -xscrollincrement 4 -yscrollincrement 4
    } else {
	# Hide the borders because child windows appear on top of them
	$T configure -borderwidth 0 -highlightthickness 0
    }

    #
    # Create columns
    #

    $T column create -expand yes -text Item -itembackground {#F7F7F7} -tags colItem
    $T column create -text "Item ID" -justify center -itembackground {} -tags colID
    $T column create -text "Parent ID" -justify center -itembackground {} -tags colParent

    # Specify the column that will display the heirarchy buttons and lines
    $T configure -treecolumn colItem

    #
    # Create elements
    #

    set Priv(bg) $::SystemButtonFace
    set outline gray70

    $T item state define openW
    $T item state define openE
    $T item state define openWE

    $T element create eRectTop rect -outline $outline -fill $Priv(bg) \
	-outlinewidth 1 -open {wes openWE es openE ws openW} -rx 7
    $T element create eRectBottom rect -outline $outline -fill $Priv(bg) \
	-outlinewidth 1 -open n -rx 7

    # Title
    $T element create elemBorderTitle border -relief {sunken open raised {}} \
	-thickness 1 -filled yes -background $::SystemButtonFace
    $T element create elemTxtTitle text \
	-font [list DemoFontBold]

    # Citizen
    $T element create elemTxtItem text
    $T element create elemTxtName text \
	-fill [list blue {}]

    # Citizen info
    $T element create elemWindow window
    if {$::clip} {
	$T element configure elemWindow -clip yes
    }

    #
    # Create styles using the elements
    #

    set S [$T style create styTitle]
    $T style elements $S {elemBorderTitle elemTxtTitle}
    $T style layout $S elemTxtTitle -expand news
    $T style layout $S elemBorderTitle -detach yes -indent no -iexpand xy

    set S [$T style create styItem]
    $T style elements $S {eRectTop elemTxtItem elemTxtName}
    $T style layout $S eRectTop -detach yes -indent no -iexpand xy \
	-draw {yes open no {}} -padx {2 0}
    $T style layout $S elemTxtItem -expand ns
    $T style layout $S elemTxtName -expand ns -padx {20}

    set S [$T style create styID]
    $T style elements $S {eRectTop elemTxtItem}
    $T style layout $S eRectTop -detach yes -indent yes -iexpand xy -draw {yes open no {}}
    $T style layout $S elemTxtItem -padx 6 -expand ns

    set S [$T style create styParent]
    $T style elements $S {eRectTop elemTxtItem}
    $T style layout $S eRectTop -detach yes -indent yes -iexpand xy \
	-draw {yes open no {}} -padx {0 2}
    $T style layout $S elemTxtItem -padx 6 -expand ns

    set S [$T style create styCitizen]
    $T style elements $S {eRectBottom elemWindow}
    $T style layout $S eRectBottom -detach yes -indent no -iexpand xy \
	-padx 2 -pady {0 1}
    $T style layout $S elemWindow -pady {0 2}

    #
    # Create 10000 items. Each of these items will hold 10 child items.
    #

    set index 1
    foreach I [$T item create -count 10000 -parent root -button yes -open no \
	-height 20 -tags title] {
	set Priv(titleIndex,$I) $index
	incr index 10
    }

    # This binding will add child items to an item just before it is expanded.
    $T notify bind $T <Expand-before> {
	DemoBigList::ExpandBefore %T %I
    }

    # In this demo there are 100,000 items that display a window element.
    # It would take a lot of time and memory to create 100,000 Tk windows
    # all at once when initializing the demo list.
    # The solution is to assign item styles only when items are actually
    # visible to the user onscreen.
    #
    # This binding will assign styles to items when they are displayed and
    # clear the styles when they are no longer displayed.
    $T notify bind $T <ItemVisibility> {
	DemoBigList::ItemVisibility %T %v %h
    }

    set Priv(freeWindows) {}
    set Priv(nextWindowId) 0
    set Priv(prev) ""

    GetWindowHeight $T

    # When the Tile/Ttk theme changes, recalculate the height of styCitizen
    # windows.
    if {$::tile} {
	bind DemoBigList <<ThemeChanged>> {
	    after idle BigListThemeChanged [DemoList]
	}
    }

    bind DemoBigList <Double-ButtonPress-1> {
	if {[lindex [%W identify %x %y] 0] eq "header"} {
	    TreeCtrl::DoubleButton1 %W %x %y
	} else {
	    DemoBigList::Button1 %W %x %y
	}
	break
    }
    bind DemoBigList <ButtonPress-1> {
	DemoBigList::Button1 %W %x %y
	break
    }
    bind DemoBigList <Motion> {
	DemoBigList::Motion %W %x %y
    }

    bind DemoBigListChildWindow <Motion> {
	set x [expr {%X - [winfo rootx [DemoList]]}]
	set y [expr {%Y - [winfo rooty [DemoList]]}]
	DemoBigList::Motion [DemoList] $x $y
    }

    bindtags $T [list $T DemoBigList TreeCtrl [winfo toplevel $T] all]

    return
}

# DemoBigList::GetWindowHeight
#
# Calculate and store the height of one of the windows used to display citizen
# information.  Since item styles are assigned on-the-fly (see the
# BigListItemVisibility procedure) we need to know the height an item would
# have if it had the "styCitizen" style assigned so the scrollbars are set
# properly.
#
# Arguments:
# T		The treectrl widget.

proc DemoBigList::GetWindowHeight {T} {
    variable Priv
    # Create a new window just to get the requested size.  This will be the
    # value of the item -height option for some items.
    set w [NewWindow $T root]
    update idletasks
    if {$::clip} {
	set height [winfo reqheight [lindex [winfo children $w] 0]]
    } else {
	set height [winfo reqheight $w]
    }
    # Add 2 pixels for the border and gap
    incr height 2
    set Priv(windowHeight) $height
    FreeWindow $T $w
    return
}

# DemoBigList::ExpandBefore --
#
# Handle the <Expand-before> event.  If the item already has child items,
# then nothing happens.  Otherwise 1 or more items are created as children
# of the item being expanded.
#
# Take advantage of the <Expand-before> event to create child items
# immediately prior to expanding a parent item.
#
# Arguments:
# T		The treectrl widget.
# I		The item whose children are about to be displayed.

proc DemoBigList::ExpandBefore {T I} {

    variable Priv

    set parent [$T item parent $I]
    if {[$T item numchildren $I]} return

    # Title
    if {[$T item tag expr $I title]} {
	set index $Priv(titleIndex,$I)
	set threats {Severe High Elevated Guarded Low}
	set names1 {Bill John Jack Bob Tim Sam Mary Susan Lilian Jeff Gary
	    Neil Margaret}
	set names2 {Smith Hobbs Baker Furst Newel Gates Marshal McNoodle
	    Marley}

	# Add 10 child items to this item. Each item represents 1 citizen.
	# The styles will be assigned in BigListItemVisibility.
	foreach I [$T item create -count 10 -parent $I -open no -button yes \
		-height 20 -tags citizen] {
	    set name1 [lindex $names1 [expr {int(rand() * [llength $names1])}]]
	    set name2 [lindex $names2 [expr {int(rand() * [llength $names2])}]]
	    set Priv(itemIndex,$I) $index
	    set Priv(name,$I) "$name1 $name2"
	    set Priv(threat,$I) [lindex $threats [expr {int(rand() * 5)}]]
	    incr index
	}
	return
    }

    # Citizen
    if {[$T item tag expr $I citizen]} {

	# Add 1 child item to this item.
	# The styles will be assigned in BigListItemVisibility.
	$T item create -parent $I -height $Priv(windowHeight) -tags info
    }

    return
}

# DemoBigList::ItemVisibility --
#
# Handle the <ItemVisibility> event.  Item styles are assigned or cleared
# when item visibility changes.
#
# Take advantage of the <ItemVisibility> event to update the appearance of
# items just before they become visible onscreen.
#
# Arguments:
# T		The treectrl widget.
# visible	List of items that are now visible.
# hidden	List of items that are no longer visible.

proc DemoBigList::ItemVisibility {T visible hidden} {

    variable Priv

    # Assign styles and configure elements in each item that is now
    # visible on screen.
    foreach I $visible {
	set parent [$T item parent $I]

	# Title
	if {[$T item tag expr $I title]} {
	    set first $Priv(titleIndex,$I)
	    set last [expr {$first + 10 - 1}]
	    set first [format %06d $first]
	    set last [format %06d $last]
	    $T item span $I colItem 3
	    $T item style set $I colItem styTitle
	    $T item element configure $I \
		colItem elemTxtTitle -text "Citizens $first-$last"
	    continue
	}

	# Citizen
	if {[$T item tag expr $I citizen]} {
	    set index $Priv(itemIndex,$I)
	    $T item style set $I colItem styItem colID styID colParent styParent
	    $T item element configure $I \
		colItem elemTxtItem -text "Citizen $index" + elemTxtName \
		-textvariable ::DemoBigList::Priv(name,$I) , \
		colParent elemTxtItem -text $parent , \
		colID elemTxtItem -text $I
	    $T item state forcolumn $I colItem openE
	    $T item state forcolumn $I colID openWE
	    $T item state forcolumn $I colParent openW
	    continue
	}

	# Citizen info
	if {[$T item tag expr $I info]} {
	    set w [NewWindow $T $parent]
	    $T item style set $I colItem styCitizen
	    $T item span $I colItem 3
	    $T item element configure $I colItem \
		elemWindow -window $w
	}
    }

    # Clear the styles of each item that is no longer visible on screen.
    foreach I $hidden {

	# Citizen info
	if {[$T item tag expr $I info]} {
	    # Add this window to the list of unused windows
	    set w [$T item element cget $I colItem elemWindow -window]
	    FreeWindow $T $w
	}
	$T item style set $I colItem "" colParent "" colID ""
    }
    return
}

proc DemoBigList::NewWindow {T I} {
    variable Priv

    # Check the list of unused windows
    if {[llength $Priv(freeWindows)]} {
	set w [lindex $Priv(freeWindows) 0]
	set Priv(freeWindows) [lrange $Priv(freeWindows) 1 end]
	if {$::clip} {
	    set f $w
	    set w [lindex [winfo children $f] 0]
	}

	if {$Priv(noise)} { dbwin "reuse window $w" }

    # No unused windows exist. Create a new one.
    } else {
	set id [incr Priv(nextWindowId)]
	if {$::clip} {
	    set f [frame $T.clip$id -background blue]
	    set w [frame $f.frame$id -background $Priv(bg)]
	} else {
	    set w [frame $T.frame$id -background $Priv(bg)]
	}
	# Name: label + entry
	label $w.label1 -text "Name:" -anchor w -background $Priv(bg)
	$::entryCmd $w.entry1 -width 24

	# Threat Level: label + menubutton
	label $w.label2 -text "Threat Level:" -anchor w -background $Priv(bg)
	if {$::tile} {
	    ttk::combobox $w.mb2 -values {Severe High Elevated Guarded Low} \
		-state readonly -width [string length "Elevated"]
	} else {
	    menubutton $w.mb2 -indicatoron yes -menu $w.mb2.m \
		-width [string length Elevated] -relief raised
	    menu $w.mb2.m -tearoff no
	    foreach label {Severe High Elevated Guarded Low} {
		$w.mb2.m add radiobutton -label $label \
		    -value $label \
		    -command [list $w.mb2 configure -text $label]
	    }
	}

	# Button
	set message \
	    "After abducting and probing these people over the last\n\
	    50 years, the only thing we've learned for certain is that\n\
	    one in ten just doesn't seem to mind."
	if {$::thisPlatform ne "windows"} {
	    set message [string map {\n ""} $message]
	}
	$::buttonCmd $w.b3 -text "Anal Probe Wizard..." -command [list tk_messageBox \
	    -parent . -message $message -title "Anal Probe 2.0"]

	grid $w.label1 -row 0 -column 0 -sticky w -padx {0 8}
	grid $w.entry1 -row 0 -column 1 -sticky w -pady 4
	grid $w.label2 -row 1 -column 0 -sticky w -padx {0 8}
	grid $w.mb2 -row 1 -column 1 -sticky w -pady 4
	grid $w.b3 -row 3 -column 0 -columnspan 2 -sticky we -pady {0 4}

	AddBindTag $w DemoBigListChildWindow
	AddBindTag $w TagIdentify

	if {$Priv(noise)} { dbwin "create window $w" }
    }

    # Tie the widgets to the global variables for this citizen
    $w.entry1 configure -textvariable ::DemoBigList::Priv(name,$I)
    $w.mb2 configure -textvariable ::DemoBigList::Priv(threat,$I)
    if {!$::tile} {
	foreach label {Severe High Elevated Guarded Low} {
	    $w.mb2.m entryconfigure $label -variable ::DemoBigList::Priv(threat,$I)
	}
    }
    if {$::clip} { return $f }
    return $w
}

proc DemoBigList::FreeWindow {T w} {
    variable Priv

    # Add the window to our list of free windows. DemoClear will actually
    # delete the window when the demo changes.
    lappend Priv(freeWindows) $w
    if {$Priv(noise)} { dbwin "free window $w" }
    return
}

proc DemoBigList::Button1 {w x y} {
    variable ::TreeCtrl::Priv
    focus $w
    $w identify -array id $x $y
    set Priv(buttonMode) ""
    if {$id(where) eq "header"} {
	TreeCtrl::ButtonPress1 $w $x $y
    } elseif {$id(where) eq "item"} {
	set item $id(item)
	# click a button
	if {$id(element) eq ""} {
	    TreeCtrl::ButtonPress1 $w $x $y
	    return
	}
	if {[$w item tag expr $item !info]} {
	    $w item toggle $item
	}
    }
    return
}

proc DemoBigList::Motion {w x y} {
    variable Priv
    $w identify -array id $x $y
    if {$id(where) eq "item"} {
	set item $id(item)
	if {[$w item tag expr $item !info]} {
	    if {$item ne $Priv(prev)} {
		$w configure -cursor hand2
		set Priv(prev) $item
	    }
	    return
	}
    }
    if {$Priv(prev) ne ""} {
	$w configure -cursor ""
	set Priv(prev) ""
    }
    return
}

proc DemoBigList::ThemeChanged {T} {
    variable Priv
    GetWindowHeight $T
    if {[$T item id {first visible tag info}] ne ""} {
	$T item conf {tag info} -height $Priv(windowHeight)
    }
    return
}
