global crypttimeout cryptlength
set crypttimeout 60
set cryptlength 50
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

bind msg o crypt_op crypt_challenge
bind msg o crypt_reply crypt_response

proc crypt_challenge {n u h a} {
	global cryptlist crypttimeout crypt_string cryptlength
	set ln [string tolower $n]
	if {[llength [getuser $h XTRA crypt]] != 2} {
		putserv "notice $n :You have no crypt keys set."
		return 0
	}
	if [info exists cryptlist($ln)] {
		putlog "Ignoring outstanding crypt-op request from $n."
		return 0
	} {
		set cryptlist($ln) [utimer $crypttimeout "unset cryptlist($ln)"]
		putserv "privmsg $ln :crypt_op [encrypt [lindex [getuser $h XTRA crypt] 0] [set crypt_string($ln) [randstring $cryptlength]]]"
	}
}

proc crypt_response {n u h a} {
	global cryptlist crypt_string
	set ln [string tolower $n]
	if {![info exists cryptlist($ln)]} {
		putlog "Ignoring unrequested or late crypt response from $n."
		return 0
	}
	if {![string compare $crypt_string($ln) [decrypt [lindex [getuser $h XTRA crypt] 1] $a]]} {
		killutimer $cryptlist($ln)
		unset cryptlist($ln)
		foreach	chan [channels] {
			if [onchan $n $chan] {
				pushmode $chan +o $n
			}
		}
		putlog "($n@$u) !$h! crypt-op"
	} {
		putlog "$n ($h) FAILED crypt authorization!"
	}
	return 0
}

bind dcc o crypt crypt_set

proc crypt_set {h i a} {
	if {[llength $a] != 2} {
		putdcc $i "Usage: crypt <key1> <key2>"
		return 0
	}
	putdcc $i "Key1: [lindex $a 0] key2: [lindex $a 1]"
	if {![string compare [lindex $a 0] [lindex $a 1]]} {
		putdcc $i "key1 and key2 MUST be different."
		return 0
	}
	setuser $h XTRA crypt $a
	putdcc $i "Set crypt keys to \"[lindex $a 0]\" and \"[lindex $a 1]\""
	putlog "#$h# crypt ..."
	return 0
}
	