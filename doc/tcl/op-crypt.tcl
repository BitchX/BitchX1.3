if {![info exists blowfish_version]} {
	putscr "You MUST load the blowfish encryption module prior to loading this!"
	return 0
}

set crypt_timeout 60

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
	if [onchan $n [_T]] {
		if {![info exists cryptkeeper([string tolower $n])]} {
			putscr "You must first set a pair of crypt keys before using this."
			putscr "Try /addcrypt <nick> <key1> <key2>"
			return 0
		}
		putserv "privmsg $n :crypt_op"
		set cryptlist([string tolower $n]) [utimer $crypt_timeout "unset cryptlist([string tolower $n])"]
	}
	return 0
}

bind msg -1 crypt_op crypt_response

proc crypt_response {n u h a} {
	global cryptkeeper cryptlist
	if {![info exists cryptlist([string tolower $n])]} {
		putscr "$n requesting UNAUTHORIZED crypt verification!"
		return 1
	} {
		unset cryptlist([string tolower $n])
		putserv "privmsg $n :crypt_reply [encrypt [lindex $cryptkeeper([string tolower $n]) 1] [decrypt [lindex $cryptkeeper([string tolower $n]) 0] $a]]"
	}
	return 1
}

proc addcrypt {n k1 k2 args} {
	global cryptkeeper
	set cryptkeeper([string tolower $n]) "$k1 $k2"
	putscr "Added $n to the crypt keeper with keys $k1 and $k2."
}
return 0
