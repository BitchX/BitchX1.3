;#to contact the author of this fserve for any reason
;#(comments, suggestions, beefs) 264mlang@home.com
;#got a damn bug that makes a tcl.log file, read it and you;ll see
;#it writes that crap when you get a file, I don't know what could be wrong
;#just what youve been waiting for..a vfs for the fserve
;#implemented 1 dir level, some crappy color codes
;#configuration file saved as ~/fserve.config
bind nick - * nicktrick
bind ctcp - !trigger ahoy
bind pub - !trigger ahoy
bind hook - dcc_* flash
set dotimer off;
set maxusers 6;
set maxsends 5;
set dirnames [list 0];
set indexed [list 0];
set buffer [list 0];
set who [list 0];
set thechannel "#mychannel";
set channeldisplay "/ctcp $nickname !trigger to get on my fserve!"
set nicktrans [list 0];

proc settimer {args} {
global channeldisplay;
if {[string length $args]>0} {
set channeldisplay $args;
};#if
};#proc

proc readconfig {} {
global maxusers;
global maxsends;
global thechannel;
global dotimer;
global channeldisplay;
if {[catch {glob "~/fserve.config"}]!=1} {
set configfile [open "~/fserve.config" r]
set conf [read $configfile];
set maxusers [lindex $conf 0];
set maxsends [lindex $conf 1];
set dotimer [lindex $conf 2];
set channeldisplay [lindex $conf 3];
set thechannel [lindex $conf 4]; 
close $configfile;
};#if
};#proc

proc writeconfig {} {
global maxusers;
global maxsends;
global thechannel;
global dotimer;
global channeldisplay;
set configfile [open "~/fserve.config" w]
puts $configfile $maxusers;
puts $configfile $maxsends;
puts $configfile $dotimer;
puts $configfile "\{$channeldisplay\}";
puts $configfile $thechannel;
close $configfile;
putscr "wrote ~/fserve.config";
};#proc


proc toggletimer {} {
global dotimer;
if {[string match $dotimer on]} {
set dotimer off;
} else {set dotimer on;}
putscr "Timer turned $dotimer!";
};#proc

proc nicktrick {args} {
	global nicktrans;
	global who;
	set oldnick [lindex $args 0];
	set newnick [lindex $args 4];
	set oldmatch [lsearch $nicktrans $oldnick]
	if {$oldmatch<=0} {
		for {set counter 0} {$counter<=[expr [llength [channels]]-1]} {incr counter} {
		if {[onchan $newnick [lindex [channels] $counter]]} {
                set ip [getchanhost $newnick [lindex [channels] $counter]];
                };#if
		};#for
		set stuff [dccstats];
		foreach check $stuff {
		if {[string match [lindex $check 1] $oldnick] && [string match [lindex $check 7] "chat"] && $oldmatch<0} {
			lappend nicktrans $newnick [lindex $check 8];
		};#if
	};#foreach
	};#if
	if {$oldmatch>0} {
	set nicktrans [lreplace $nicktrans $oldmatch [expr $oldmatch+1] $newnick [lindex $nicktrans [expr $oldmatch+1]]];
	};#if
};#proc

proc thetimer {} {
	global channeldisplay;
	global dotimer;
	global thechannel;
	if {[llength [channels]]==0} {ircii /join $thechannel;}
	global nicktrans;
	global maxusers;
	global nickname;
	global who;
	set change $who;
	set hell [dccstats];
	set online 0;
	set users 0;
	if {[llength $change]>100} {set who [lreplace $who 0 end];}
foreach check $hell {
set ip no;
if {[string match [lindex $check 7] "chat"]} {
set newnick [lindex $check 1];
if {[lsearch $nicktrans [lindex $check 8]]>-1} {
        set newnick [lindex $nicktrans [expr [lsearch $nicktrans [lindex $check 8]]-1]];
        };#if
for {set counter 0} {$counter<=[expr [llength [channels]]-1]} {incr counter;} {
        if {[onchan $newnick [lindex [channels] $counter]]} {
                set ip [getchanhost $newnick [lindex [channels] $counter]];
                };#if
        };#for

if {[llength $change]>100 && ![string match $ip "no"]} {lappend who $ip [lindex $change [expr [lsearch $change $ip]+1]];}
if {[string match $ip "no"]} {
	ircii /msg =[lindex $check 1] "terminating off-channel dcc chat...";
	ircii /dcc close chat [lindex $check 1];
	set chatnumber [lsearch $nicktrans [lindex $check 8]];
	if {$chatnumber>-1} {
	set nicktrans [lreplace $nicktrans [expr $chatnumber-1] $chatnumber];
	};#if
	};#if
if {[string match [lindex $check 7] "chat"] && ![string match $ip "no"]} {incr users;}
};#if
};#foreach
	if {[string match $dotimer on]} {
	ircii /say "$channeldisplay Users $users/$maxusers";
	};#if
	ircii /timer 300 tcl thetimer;
};#proc

ircii timer 300 tcl thetimer

proc update {} {
set dircount 0;
global dirnames;
global buffer;
set dirnames [lreplace $dirnames 0 end];
global indexed;
set b .;
set a [glob *];
set vfs [open "outfile" w];
foreach filenames [concat $a] {
        file stat $filenames arr
        if {[string match [file type $filenames] "directory"]} {
        set dircount 1;
	if {[catch {glob $b/[file tail $filenames]/*}]!=1} {
	lappend dirnames [file tail $filenames];
	set filenum [glob $b/[file tail $filenames]/*];
        puts $vfs "[file tail $filenames]";
        set filecount [llength $filenum];
	foreach subfiles [concat $filenum] {
		if {[string match [file type $subfiles] "directory"]} {
		set filecount [expr $filecount-1];
		};#if
	};#foreach
	puts $vfs ",$filecount";
	foreach subfiles [concat $filenum] {
                if {[string match [file type $subfiles] "file"]} {
		puts $vfs "$subfiles"
                puts $vfs ",[file size $subfiles]";
                };#if
		};#foreach
		};#if
        };#if
};#foreach
close $vfs;
set indexed [lreplace $indexed 0 end];
set buffer [lreplace $buffer 0 end];
set vfsin [open "outfile" r];
set buffer [read $vfsin];
if {$dircount>0} {
foreach satan $dirnames {
	lappend indexed [lsearch $buffer $satan];
};#foreach
}
close $vfs;
if {$dircount==0} {putscr "ERROR: no sub dirs in ./"};
putscr "VFS has been updated with the new directory structure";
};#proc

proc ahoy {name args} {
global maxusers;
set hell [dccstats];
set no 0
set users 0;
foreach check $hell {
if {[string match [lindex $check 7] "chat"]} {incr users;}
if {[string match [lindex $check 1] $name] && [string match [lindex $check 7] "chat"]} {
	ircii /msg =$name "restarting dcc...";
	ircii /dcc close chat $name;
	ircii /timer 10 dcc chat $name;
	set no 1
};#if 
};#foreach 
if {$no==0 && $users<$maxusers} {ircii /dcc chat $name;
};#if
if {$users>=$maxusers && $no==0} {ircii /msg $name "Sorry there's over $maxusers users, try later!";}
};#proc

update;
readconfig;
proc echohelp {} {
	global dotimer;
	global thechannel;
	global maxusers;
	global maxsends;
	global nickname
	global channeldisplay;
	putscr "FSERVE SCRIPT HELP:";
	putscr "type /update to refresh the filesystem"
	putscr "type /usermax newlimit to set a new user limit"
	putscr "       Max Users: $maxusers";
	putscr "type /sendmax newlimit to set the max user gets";
	putscr "       Max Sends: $maxsends";
	putscr "type /persist #channel to set a new persistant channel";
	putscr "       Persistant channel: $thechannel";
	putscr "type /fsoff to turn the fserve off";
	putscr "And /fson to turn the fserve on!";
	putscr "Toggle timer /fstimer currently turned $dotimer!";
	putscr "type /fssay new channel advertisement to change the timer"; 
	putscr "currently: $channeldisplay";
	putscr "type /fssave to save the current fserve setup";
};#proc

proc fserveon {} {
	bind ctcp - !trigger ahoy;
	bind pub - !trigger ahoy;
	bind hook - dcc_* flash;
	putscr "Fserve turned back on....";
}
proc fserveoff {} {
	ircii nochat *;
	unbind ctcp - !trigger ahoy;
	unbind pub - !trigger ahoy;
	unbind hook - dcc_* flash;
	putscr "Fserve turned off!";
}

proc flash {request args} { 
	global who;
	regsub -nocase -all \[{}] $args "" args;
	switch $request {
        DCC_CONNECT {
			scan $args "%s%s%s%s" name type ip p;
		if {[string match CHAT $type]} {ircii /msg =$name "Please type help to learn how to use my fserve";}
		return;
		}
             DCC_CHAT {
	if {[string first " " $args]!=[string last " " $args]} {
		scan $args "%s%s%s" name args filematch;
	} else { 
	scan $args "%s%s" name args;
	set filematch *;
	}
	global nicktrans;
	global maxsends;
	global indexed;
	global dirnames;
	global buffer;
	set ip no;
	set newnick $name;
        set dccstuff [dccstats];
	foreach check $dccstuff {
        if {[lsearch $nicktrans [lindex $check 8]]>-1 && [string match [lindex $check 7] "chat"] && [string match [lindex $check 1] $name]} {
        set chatnumber [lsearch $nicktrans [lindex $check 8]];
	set newnick [lindex $nicktrans [expr $chatnumber-1]];
	};#if
        };#foreach

	for {set counter 0} {$counter<=[expr [llength [channels]]-1]} {incr counter} {
	if {[onchan $newnick [lindex [channels] $counter]]} {
		set ip [getchanhost $newnick [lindex [channels] $counter]];
		};#if
	};#for
	set exist 0;
	set bunk *;
	set bunk2 ?;
	if {$bunk==$args} {set args help;}
	if {$bunk2==$args} {set args help;}
	set fileopen 1;
	set listem 1;
	set d 0;
	set infoseek [lsearch $who $ip];
	if {$infoseek>-1} {
	set activedir [lindex $who [expr $infoseek+1]];
	};#if
	if {$infoseek<=-1} {
		set activedir ".";
	};#if
	set homedir "back";
	set command 0;
	set get get;
	set ls ls;
	set help help;
	set b [exec pwd];
	set currentdir ".";
if {[lsearch $ls $args]>=$d && ![string match $ip "no"]} {set listem 0;}
if {$listem!=1 && $activedir!=$currentdir} {ircii /msg =$name "ParentDir:  back";}
foreach fileindex [concat $buffer] {  
	if {[string index $fileindex 0]!="." && [string index $fileindex 0]!=","} {
		if {$listem!=1 && $activedir=="."} {
			ircii /msg =$name "Dir: $fileindex      files:[lindex $buffer [expr [lsearch $buffer $fileindex]+1]]";
			set command 1;
		};#if
};#foreach
};#if
if {$listem!=1 && $activedir!="." && ![string match $ip "no"]} {
	set dirstart [lindex $indexed [lsearch $dirnames $activedir]];
	set dirlength [string trim [lindex $buffer [expr $dirstart+1]] ","];
	set nameone [expr $dirstart+2];
	for {set i $nameone} {$i<[expr [expr $dirlength*2]+$dirstart+2]} {set i [expr $i+2]} {
		set size [string trim [lindex $buffer [expr $i+1]] ","];
		set testthis [string range [lindex $buffer $i] [string length ./$activedir/] [string length [lindex $buffer $i]]];
		set color 3;
		if {$size>1000000} {set color 8;}
		if {$size>5000000} {set color 4;}
		if {[string match $filematch $testthis]} {ircii /msg =$name "file:$color,1 $testthis size:$size";}
	};#for
set command 1;
};#if
if {[lsearch $help $args]>=$d} {
	ircii /msg =$name "command examples:";
        ircii /msg =$name "ls or ls *.avi      :directory listing";
 	ircii /msg =$name "[string trim [lindex $buffer 2] ./[lindex $dirnames 0]/]     :type a file name to get it";
	if {$activedir=="."} {
	ircii /msg =$name "[lindex $dirnames 0]   :type the dir name to change to it"
	} else { 
	ircii /msg =$name "back     :to go back a directory level."; }
	if {$activedir=="."} { ircii /msg =$name "*back command currently inactive.*";}
	ircii /msg =$name "All commands are one word only, have fun!"
set command 1;
};#if
set infodir [lsearch $dirnames $args];
if {$infodir>=$d && ![string match $ip "no"]} {
	if {$infoseek>0} {
		set who [lreplace $who $infoseek [expr $infoseek+1] $ip [lindex $dirnames $infodir]];
	};#if
	if {$infoseek<=0} {
		lappend who $ip [lindex $dirnames $infodir];
	};#if
	ircii /msg =$name "Database updated: [lindex $dirnames $infodir]";
set command 1;
};#if

if {[string match $args $homedir] && $activedir!=$currentdir && ![string match $ip "no"]} {
	if {$infoseek>0} {
		set who [lreplace $who $infoseek [expr $infoseek+1] $ip $currentdir];
	};#if
        if {$infoseek<=0} {
                lappend who $ip $currentdir;
	};#if
ircii /msg =$name "Database updated: $args";
set command 1;
};#if
	set dirstart [lindex $indexed [lsearch $dirnames $activedir]];
        set dirlength [string trim [lindex $buffer [expr $dirstart+1]] ","];
        set nameone [expr $dirstart+1];
set subdir [lrange $buffer $nameone [expr $dirstart+[expr $dirlength*2]]];
set info [lsearch $subdir "./$activedir/$args"];
if {$info>$d} {
if {[string match [string range [lindex $subdir $info] [string length "./$activedir/"] [string length [lindex $subdir $info]]] $args] && [lsearch $dirnames $args]<$d} {
	set sendtotal 0;
	foreach check $dccstuff {
	if {[string match [lindex $check 0] "SEND"] && [string match [lindex $check 1] $name]} {incr sendtotal}
	if {[string match [lindex $check 0] "SEND"] && [string match [lindex $check 1] $newnick]} {incr sendtotal}
	};#foreach
	set sent 0;
	if {![string match $ip "no"] && $sendtotal<$maxsends} {
		ircii /dcc send $newnick [lindex $subdir $info];
		set sent 1;
		};#if
	set command 1;
	if {$sent==0 && $sendtotal<$maxsends} {ircii /msg =$name "I can only send file $args to [channels]";} 
	if {$sendtotal>=$maxsends} {ircii /msg =$name "max active sends reached: $sendtotal";}
	set command 1;
	};#if
};#if
if {[string match $ip "no"]} { 
ircii /msg =$name please join [channels] to use fserve!";
set command 1;
};#if
if {$command!=1} { ircii /msg =$name "Bad command: type help"; };#if
return;
}
DCC_LOST {
	global nicktrans;
	set name [lindex $args 0];
	if {[string match [lindex $args 1] "CHAT"]} {
		set check [dccstats];
		foreach stuff $check {
		if {[string match [lindex $stuff 7] "chat"] && [string match [lindex $stuff 1] $name]} {
		set chatnumber [lsearch $nicktrans [lindex $stuff 8]];
		if {$chatnumber>-1} {
		set nicktrans [lreplace $nicktrans [expr $chatnumber-1] $chatnumber];
		};#if
		};#if
		};#foreach
	};#if
return;
}
default {return;}
}
};#proc
