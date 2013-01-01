if {![info exists blowfish_version]} {
	putscr "You MUST load the blowfish encryption module prior to loading this!"
	return
}
global crypt_timeout cryptfile
set crypt_timeout 60
set cryptfile "/home/by-tor/.BitchX/cryptkeys"

#borrowed from alltools.tcl
proc randstring {count} {
  set rs ""
  for {set j 0} {$j < $count} {incr j} {
     set x [rand 62]
     append rs [string range "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" $x $x]
  }
  unset x
  unset j
  return $rs
}

proc crop {n args} {
	global cryptkeeper cryptlist crypt_timeout
	set ln [string tolower $n]
	if {([onchan $n [_T]]) && ([isop $n [_T]])} {
		if {![info exists cryptkeeper($ln)]} {
			putscr "You must first set a pair of crypt keys before using this."
			putscr "Try /addcrypt <nick> <key1> <key2>"
			return
		}
		putserv "privmsg $n :crypt_op"
		set cryptlist($ln) [utimer $crypt_timeout "killcrypt $n"]
	} {
		putscr "$n is not on [_T], or is not an op."
	}
	return
}

proc killcrypt {n} {
	global cryptlist
	if [info exists cryptlist($n)] { unset cryptlist($n) }
}

bind msg -1 crypt_op crypt_response

proc crypt_response {n u h a} {
	global cryptkeeper cryptlist
	set ln [string tolower $n]
	if {![info exists cryptlist($ln)]} {
		putscr "$n requesting UNAUTHORIZED crypt verification!"
		return
	} {
		killutimer $cryptlist($ln)
		unset cryptlist($ln)
		putserv "privmsg $n :crypt_reply [encrypt [lindex $cryptkeeper($ln) 1] [decrypt [lindex $cryptkeeper($ln) 0] $a]]"
	}
	return
}

proc addcrypt {n k1 k2 args} {
	global cryptkeeper
	set cryptkeeper([string tolower $n]) "$k1 $k2"
	putscr "Added $n to the crypt keeper with keys $k1 and $k2."
	savecrypt
}

proc savecrypt {} {
	global cryptfile cryptkeeper filekey
	if {![info exists cryptkeeper]} { return 0 }
	if [set fd [open $cryptfile w]] {
		if [info exists filekey] {
			puts $fd "encrypted"
			puts $fd [encrypt $filekey verified]
			foreach name [array names cryptkeeper] {
				puts $fd [encrypt $filekey "$name $cryptkeeper($name)"]
			}
		} {
			foreach name [array names cryptkeeper] {
				puts $fd "$name $cryptkeeper($name)"
			}
		}
		close $fd
	}
}

proc readcrypt {args} {
	global cryptfile cryptkeeper filekey
	if [file exists $cryptfile] {
		if [info exists cryptkeeper] { unset cryptkeeper }
		set fd [open $cryptfile r]
		set text [gets $fd]
		# Is our file encrypted? (Maximum security here!)
		if {![string compare "encrypted" $text]} {
			set text [gets $fd]
			if {[llength $args] < 1} {
				return "You must supply a file key for the encrypted file."
			}
			set filekey [lindex $args 0]
			if {![string compare "verified" [decrypt $filekey $text]]} {
				while {![eof $fd]} {
					set text [decrypt $filekey [gets $fd]]
					if {[llength $text] == 3} {
						set cryptkeeper([lindex $text 0]) [lrange $text 1 end]
					}
				}
				return
			} {
				return "Invalid cryptfile key."
			}
		}
		while {![eof $fd]} {
			set text [gets $fd]
			if {[llength $text] == 3} {
				set cryptkeeper([lindex $text 0]) [lrange $text 1 end]
			}
		}
		return
	}
}

proc filekey {args} {
	global filekey cryptkeeper
	if {[llength $args] > 0} {
		set filekey [lindex $args 0]
		if [info exists cryptkeeper] { savecrypt }
		return "Set crypt file key to \"$filekey\""
	} {
		return "You must supply a key!"
	}
}

readcrypt
return
