array set inihandler {}

proc IniOpen { iniFile } {
	# handler is a list of two parts: 
	# 1. list of sections
	# 2. list of list of keys  and valuesper section 
	
	# read data file
	if {[file readable $iniFile] ==  1} {
		set fp [open $iniFile r]
		set contents [read $fp]
		close $fp
	} else {
		puts "echo Can not read file '$iniFile'"
		return ''
	}
		
	set contents [split $contents "\n"]
	
	set han_key ""
	set is_section 0
	set is_key 0
	set old_section ""
	set one_shot 1
	
	array unset ::inihandler	
	
	#go over the line in fill sections
	foreach line $contents {
		# check if this is a new key		
		set is_key [regexp {([^=]*)=(.*)} $line all key value]
	
		if {$is_key} {
			# Add section#key to array
			set key [string trim $key]
			set value [string trim $value]
			array set ::inihandler [list "$section#$key" $value]	
		} else {
			set is_section [regexp {^\[(.*)\]$} $line all section]
			if {$is_section} {
				set section [string trim $section]
			}
		}
	}
}

# handler is currently unused - left for compatibility
proc IniGetKey {handler section key} {
	set ret ""
	set key [string trim $key]
	set section [string trim $section]
	if {[catch {set ret $::inihandler($section#$key)}]} {
		set ret ""
	}
	return $ret
}


# handler is currently unused - left for compatibility
# Get a list of alls key/values in a section
proc IniGetSection {handler section} {
	set ret ""
	set sec [array names ::inihandler "$section*"]
	foreach key $sec {
		# parse the key name, without the section name
		set keyname [lindex [split $key "#"] 1]
		#puts $keyname

		# get the key value
		set val $::inihandler($key)
		lappend ret [list $keyname $val]
	}
	return $ret
}
