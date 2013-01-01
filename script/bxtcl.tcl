set blink {[5m}
set cl {[0m}
set blk {[0;30m}
set blu {[0;34m}
set grn {[0;32m}
set cyn {[0;36m}
set red {[0;31m}
set mag {[0;35m}
set yel {[0;33m}
set wht {[0;37m}
set hblk {[1;30m}
set hblu {[1;34m}
set hgrn {[1;32m}
set hcyn {[1;36m}
set hred {[1;31m}
set hmag {[1;35m}
set hyel {[1;33m}
set hwht {[1;37m}
set blk_b {[40m}
set red_b {[41m}
set grn_b {[42m}
set yel_b {[43m}
set blu_b {[44m}
set mag_b {[45m}
set cyn_b {[46m}
set wht_b {[47m}
set bblk {[0;[5;30m}
set bblu {[0;[5;34m}
set bgrn {[0;[5;32m}
set bcyn {[0;[5;36m}
set bred {[0;[5;31m}
set bmag {[0;[5;35m}
set byel {[0;[5;33m}
set bwht {[0;[5;37m}
proc showver {} {
global cyb version botnick
putserv "PRIVMSG $cyb(cur) :\001ACTION is throwing snowballs wif robocod's cyb\002O\002rg$cyb(ver)/($version)\001"
putscr "* $botnick is throwing snowballs wif robocod's cyb\002O\002rg$cyb(ver)/($version)"
}
if ![info exists cyb(cur)] {
set cyb(cur) ""
set cyb(ver) "0.6a1"
set cyb(lst) ""
}
proc bleh {} {
global cyb cl hblk blu red
putscr "robocod's ..............."
putscr {[1m        [30m.r    -4eeee[0m}
putscr {[1m     [30mz$P[37m         [30m"$$[37m                                                          [0m}
putscr {[1m   [30m.$$%[37m            [30m$[37m                                                          [0m}
putscr {[1m  [30mJ$$F[37m             [30m^[37m                                                          [0m}
putscr {[1m [30m4$$$[37m                                                                         [0m}
putscr {[1m [30m$$$F[37m                           [0;34m=*$[1;37m                                           [0m}
putscr {[1m [30m$$$F[37m                             [0;34m$[1;37m                                           [0m}
putscr {[1m [30m$$$F[37m                             [0;34m$[1;37m                                           [0m}
putscr {[1m [30m*$$b[37m                  [0;34m3$"[1;37m   [0;34m^$"[1;37m  [0;34m$.@" ".[1;37m    [0;34m.=" "$.[1;37m  [0;34m.e[1;37m [0;34md$$c[1;37m  [0;34m.r""*ee[1;37m        [0m}
putscr {[1m  [30m$$$[37m                   [0;34m$R[1;37m   [0;34m4"[1;37m   [0;34m$[1;37m     [0;34m^$[1;37m  [0;34mJ[1;37m      [0;34m$r[1;37m  [0;34m$"[1;37m     [0;34m.$[1;37m    [0;34m$F[1;37m        [0m}
putscr {[1m  [30m^$$$[37m             [30m$[37m    [0;34m^$[1;37m   [0;34m%[1;37m    [0;34m$[1;37m      [0;34m$[1;37m  [0;34m$[1;37m      [0;34m4$[1;37m  [0;34m$[1;37m      [0;34m^$[1;37m    [0;34m$[1;37m         [0m}
putscr {[1m    [30m*$b[37m          [30m.$$[37m     [0;34m3L[1;37m [0;34m/[1;37m     [0;34m$r[1;37m    [0;34m4$[1;37m  [0;34m$F[1;37m     [0;34m4F[1;37m  [0;34m$[1;37m        [0;34m)*""[1;37m          [0m}
putscr {[1m      [30m"$c[37m       [30md$$$[37m      [0;34m$E[1;37m      [0;34m$[1;37m  [0;34m..e$"[1;37m  [0;34m^$.[1;37m    [0;34mF[1;37m  [0;34m4$[1;37m       [0;34m$c.....[1;37m        [0m}
putscr {[1m                          [0;34m4"[1;37m      [0;34m"[1;37m           [0;34m"**"[1;37m    [0;34m4"[1;37m       [0;34m.F""""*$[1;37m       [0m}
putscr {[1m                          [0;34mF[1;37m                                   [0;34m$"[1;37m [0;31mrev[1;37m  [0;34m$[1;37m       [0m}
putscr {[1m                         [0;34m@[1;37m                                    [0;34m3L[1;37m     [0;34md"[1;37m       [0m}
putscr {[1m                       [0;34m*"[1;37m                                       [0;34m"**"[1;37m          [0m}
putscr {[1m                                                                              [0m}
putscr {[1m                                                                              [0m}
putscr "\002C\002y\002B\002o\002R\002g v$cyb(ver) -=- Advanced BitchX TCL"
}
utimer 4 bleh
proc grabhost {arg} {
global cyb
foreach channel [channels] {
if [onchan [lindex $arg 0] $channel] {
return [getchanhost [lindex $arg 0] $channel]
}}
return 0}
ircii "^set CLIENT_INFORMATION robocod's CyBoRg(TCL)v$cyb(ver) "
if [string match $version "*68f*"] {
ircii "^set status_format3 CyBoRg loaded .. "
putscr "*** Special 68f Stuff being set"
ircii "^window split on"
}
proc cybjoin {arg} {
global cyb
set cyb(cur) "[lindex $arg 0]"
}
ircii "alias sv {tcl showver}"
ircii {alias addf {tcl addf "$0-"}}
proc addf {arg} {
global cyb
set nick [lindex $arg 0]
set flags [lindex $arg 1]
set channels [lrange $arg 2 end]
if {$nick=="" || $flags=="" || $channels==""} {
putscr "*** Usage /addf <nick> <flags> <channel (all for all channels)>"
putscr "*** Flags = O for ops; V for Voice; I for Invite A for autoop; B for auto ban"
putscr "*** N for names access i.e. /ctcp yournick NAME #warez will send a names list for #warez"
putscr "*** i.e. /addf robocod OVI #livid"
return}
set host [grabhost $nick]
if {$host==0} {putscr "FUCCCCCCCK! NICK NOT FOUND! ABORTING!"}
set nig [fixhost $host]
set flagz "$flags"
set flags "[weed $flagz]"
if {$flags==""} {
putscr "*** -\002C\002- None of your flags were valid"
return}
lappend cyb(lst) "$nig $flags $channels"
putscr "*** -\002C\002- Added $nig to friends list --------->"
putscr "*** -\002C\002- $nig $flags $channels"
putscr "*** -\002C\002- Messaging him about his new access ..."
putserv "NOTICE $nick :-\002C\002-You've been added to my friend's list."
putserv "NOTICE $nick :-\002C\002- Access of +$flags on $channels"
putserv "NOTICE $nick :-\002C\002- Enjoy!"
return}
proc fixhost {host} {
if [string match "*.*.*" $host] {
set hose [lindex [split $host @] 1]
set userhost [lrange [split $hose .] 1 end]
set username [lindex [split $host @] 0]
regsub -all " " $userhost "." userhost
return "*!*$username\@*.$userhost"
} else {
return "*!*$host"}}
proc weed {flags} {
if [string match "*o*" $flags] {append flag "O"}
if [string match "*O*" $flags] {append flag "O"}
if [string match "*v*" $flags] {append flag "V"}
if [string match "*V*" $flags] {append flag "V"}
if [string match "*i*" $flags] {append flag "I"}
if [string match "*I*" $flags] {append flag "I"}
if [string match "*a*" $flags] {append flag "A"}
if [string match "*A*" $flags] {append flag "A"}
if [string match "*b*" $flags] {append flag "B"}
if [string match "*B*" $flags] {append flag "B"}
if [string match "*N*" $flags] {append flag "N"}
if [string match "*n*" $flags] {append flag "N"}
return "$flag"
}
ircii "^set flood_protection off"
ircii "^set flood_kick off"
ircii "^set flood_rate 0"
ircii "^set display_ansi on"
ircii "^set script_help ~/.BitchX/NONE"
ircii "^set exec_protection off"
ircii "^set -continued_line"
ircii "^set novice off"
ircii "set hide_private_channels off"
ircii "set tran latin_1"
ircii "set hard_uh_notify on"
ircii "set send_away_msg on"
ircii "set suppress_server_motd on"
ircii "set clock_24hour off"
ircii "set dcc_autoget on"
ircii "set lastlog_level ALL"
ircii "set msglog off"
ircii "set mail 1"
ircii "set show_channel_names on"
ircii "set msglogfile Ctoolz.away"
ircii "eval set logfile .BitchX/cyborg.chanLog"
ircii "set reasonfile BitchX.reason"
ircii "set ctoolz_dir ~/.BitchX"
ircii "set savefile BitchX.sav"
ircii "set eight on"
ircii "set beep_on_msg msgs"
ircii "set beep on"
ircii "set deop_on_deopflood 100"
ircii "set kick_on_deopflood 8"
ircii "set dcc_send_limit 10"
ircii "set dcc_get_limit 0"
ircii "set llook off"
ircii "set input_protection off"
ircii "set llook_delay 60"
ircii "set lastlog 1000"
ircii "set dcc_dldir /home/robocod/"
ircii "set indent on"
#ircii "bind meta2-3 parse_command b"
#ircii "bind meta2-2 parse_command a"
#ircii "bind ^L parse_command bind_control_l"
ircii "bind ^@ AUTOREPLY"
ircii "bind meta3-A parse_command fkey 1"
ircii "bind meta3-B parse_command fkey 2"
ircii "bind meta3-C parse_command fkey 3"
ircii "bind meta3-D parse_command fkey 4"
ircii "bind meta3-E parse_command fkey 5"
ircii "bind meta3-7 parse_command fkey 6"
ircii "bind meta3-8 parse_command fkey 7"
ircii "bind meta3-9 parse_command fkey 8"
ircii "^set status_format  <%R> %*%N%S%H%B %#%Q%A%C%+%I%O %F %^ %> %D %U "
ircii "^set status_format1  <%R> %*%N%S%H%B %#%Q%A%C%+%I%O %F %^ %> %D %U "
ircii "^set status_format2  %1 %L   online %3 %>%M  %T "
ircii "^set status_user1 \(CyBoRg\(robocod\)\)"
ircii "^eval ^set status_user2 ?"
ircii {^eval ^set status_user3 $uptime()}
ircii "^eval ^set status_mail Mail: %M"
#ircii "^eval ^set status_user \$\S\"
ircii "^eval ^set ^status_channel %C"
#ircii "^on ^001 * {^eval ^set status_user $S}"
ircii "^set status_away away"
ircii "^set status_chanop @"
ircii "^set status_clock %T"
ircii "^set status_notify (A: %F)"
#ircii "assign blink [5m"
#ircii "assign cl [0m"
#ircii "assign blk [0;30m"
#ircii "assign blu [0;34m"
#ircii "assign grn [0;32m"
#ircii "assign cyn [0;36m"
#ircii "assign red [0;31m"
#ircii "assign mag [0;35m"
#ircii "assign yel [0;33m"
#ircii "assign wht [0;37m"
#ircii "assign hblk [1;30m"
#ircii "assign hblu [1;34m"
#ircii "assign hgrn [1;32m"
#ircii "assign hcyn [1;36m"
#ircii "assign hred [1;31m"
#ircii "assign hmag [1;35m"
#ircii "assign hyel [1;33m"
#ircii "assign hwht [1;37m"
#ircii "assign blk_b [40m"
#ircii "assign red_b [41m"
#ircii "assign grn_b [42m"
#ircii "assign yel_b [43m"
#ircii "assign blu_b [44m"
#ircii "assign mag_b [45m"
#ircii "assign cyn_b [46m"
#ircii "assign wht_b [47m"
#ircii "assign bblk [0;[5;30m"
#ircii "assign bblu [0;[5;34m"
#ircii "assign bgrn [0;[5;32m"
#ircii "assign bcyn [0;[5;36m"
#ircii "assign bred [0;[5;31m"
#ircii "assign bmag [0;[5;35m"
#ircii "assign byel [0;[5;33m"
#ircii "assign bwht [0;[5;37m"
ircii "set kick_on_nickflood 6"
ircii "set kick_on_pubflood 30"
ircii "set deopflood_time 60"
ircii "set kick_on_deopflood 6"
ircii "set kick_on_nickflood 6"
ircii "set kick_on_pubflood 30"
ircii "set deopflood_time 60"
ircii "set kickflood_time 60"
ircii "set nickflood_time 60"
ircii "set pubflood_time 60"
ircii "set hacking 0"
ircii "set kick_on_kickflood 10"
ircii "set deop_on_kickfp 100"
ircii "set llook off"
ircii "set llook_delay 60"
ircii "set pubflood off"
ircii "set nickflood on"
ircii "set kickflood on"
ircii "set deopflood on"
ircii "set dcc_block_size 1024"
ircii "set dcc_limit 10"
ircii "set hacking 0"
ircii "set dcc_autoget 0"

set cyb(awaymsg) {
{Bored!}
{livid incorp rocks you}
{ircopz are uberlame}
{Automatically set away}
{getting sex}
{getting blown}
{brb}
{bbl}
{the truth is out there i just dont give a shit anymore.}
{eh?}
{bringing death}
{one can short .....}
{half empty or half full, i dont give a shit!}
{elsewhere}
{elsewhen}
{elsewhy}
{or else ..}
}
proc cybAway {arg} {
if {$arg==""} {
global cyb
set non 0 
foreach msg $cyb(awaymsg) {
incr non
}
set awayMSG [lindex $cyb(awaymsg) [rand [expr $non - 1]]]
ircii "/away $awayMSG"
foreach channel [channels] {
putserv "PRIVMSG $channel :\001ACTION is outtie. ($awayMSG)\001"
}
putscr "-\002C\002- Marking you away with messsage: $awayMSG"
return}
ircii "away $arg"
foreach channel [channels] {
putserv "PRIVMSG $channel :\001ACTION is outtie. ($arg)\001"
}
putscr "-\002C\002- Marking you away with messsage: $arg"
return}


proc STATUS_SET {} {
ircii "^set status_format  <%R> %*%N%S%H%B %#%Q%A%C%+%I%O %F %^ %> %D %U "
ircii "^set status_format1  <%R> %*%N%S%H%B %#%Q%A%C%+%I%O %F %^ %> %D %U "
ircii "^set status_format2  %1 %L online %3 %>%M  %T "
ircii "^set status_user1 \(CyBoRg\(robocod\)\)"
ircii "^eval ^set status_user2 ?"
ircii {^eval ^set status_user3 $uptime()}
ircii "^eval ^set status_mail Mail: %M"
ircii "^eval ^set ^status_channel %C"
ircii "^set status_away away"
ircii "^set status_chanop @"
ircii "^set status_clock %T"
ircii "^set status_notify (A: %F)"
utimer 60 STATUS_SET
return}
if ![info exists cyb(timer)] {
STATUS_SET
set cyb(timer) ""
}

ircii "^assign G -\002C\002-"
#finally, friends list support
proc isattr {nick attri} {
global cyb
set h [fixhost [grabhost $nick]]
if {$h=="0"} {return "0"}
set find [lsearch "$cyb(lst)" "*$h*"]
if {$find==-1} {return 0}
set results [lindex $cyb(lst) $find]
set host [lindex $results 0]
set attr [lindex $results 1]
set channels [lrange $results 2 end]
if [string match "*$attri*" "*$attr*"] {return 1}
return}
#autoop?
bind join - *!*@* parse_join
proc parse_join {nick host hand chan} {
global cyb
set h [fixhost [grabhost $nick]]
set find [lsearch "$cyb(lst)" "*$h*"]
if {$find==-1} {return}
set chanaxs [lrange [lindex $cyb(lst) $find] 2 end]
set axs 0
if {$chanaxs=="all"} {
set axs 1}
if {$axs==0} {
foreach chanl $chanaxs {
if {$chanl==$chan} {
set axs 1}
}
}
if [isattr $nick A] {
if {$axs==1} {
putserv "MODE $chan +o $nick"
putscr "-\002C\002- Autoop'ng $nick on $chan (friendzlist)"
return}}
return}
proc zday {} {
global cyb
put}
proc leet {text} {
global cyb botnick
regsub -all "you" $text "yew" text
regsub -all "man" $text "man\002G\002" text
regsub -all "too" $text "tew" text
regsub -all "to" $text "tew" text
regsub -all "two" $text "tew" text
regsub -all "elite" $text "31337" text
regsub -all "e" $text "3" text
regsub -all "o" $text "0" text
regsub -all "l" $text "1" text
regsub -all "t" $text "7" text
regsub -all "s" $text "5" text
putserv "PRIVMSG $cyb(cur) :$text"
putscr "<$botnick> $text"
}
ircii {^alias leet tcl leet "$0-"}
bind ctcp - invite frinv
proc frinv {nick uhost hand dest key arg} {
putscr "$nick u  $uhost h $hand d $dest k  $key a $arg"
}
#welcome to the lamest script EVER. change LAME mirc colors to ANSI.
#Khaled sucks a HUGE ASS DICK for using FUCKING ircle color codes
proc mirc2ansi {txt} {
regsub -all "01,00" $txt {[30;47m} txt 
regsub -all "01,0" $txt {[30;47m} txt
regsub -all "1,00" $txt {[30;47m} txt
regsub -all "1,0" $txt {[30;47m} txt
regsub -all "1" $txt {[30;47m} txt
regsub -all "02,00" $txt {[34m} txt 
regsub -all "02,0" $txt {[34m} txt 
regsub -all "2,00" $txt {[34m} txt 
regsub -all "2,0" $txt {[34m} txt 
regsub -all "2" $txt {[34m} txt 
regsub -all "03,00" $txt {[32m} txt 
regsub -all "03,0" $txt {[32m} txt 
regsub -all "3,00" $txt {[32m} txt 
regsub -all "3,0" $txt {[32m} txt 
regsub -all "3" $txt {[32m} txt 
regsub -all "04,00" $txt {[31m} txt 
regsub -all "04,0" $txt {[31m} txt 
regsub -all "4,00" $txt {[31m} txt 
regsub -all "4,0" $txt {[31m} txt 
regsub -all "4" $txt {[31m} txt 
regsub -all "05,00" $txt {[33m} txt 
regsub -all "05,0" $txt {[33m} txt 
regsub -all "5,00" $txt {[33m} txt 
regsub -all "5,0" $txt {[33m} txt 
regsub -all "5" $txt {[33m} txt 
regsub -all "06,00" $txt {[35m} txt        
regsub -all "06,0" $txt {[35m} txt        
regsub -all "6,00" $txt {[35m} txt        
regsub -all "6,0" $txt {[35m} txt        
regsub -all "6" $txt {[35m} txt        
regsub -all "08,00" $txt {[1;33m} txt
regsub -all "08,0" $txt {[1;33m} txt
regsub -all "8,00" $txt {[1;33m} txt
regsub -all "8,0" $txt {[1;33m} txt
regsub -all "8" $txt {[1;33m} txt
regsub -all "09,00" $txt {[32m} txt
regsub -all "09,0" $txt {[32m} txt
regsub -all "9,00" $txt {[32m} txt
regsub -all "9,0" $txt {[32m} txt
regsub -all "9" $txt {[32m} txt
regsub -all "10,00" $txt {[0;36;47m} txt
regsub -all "10,0" $txt {[0;36;47m} txt
regsub -all "10" $txt {[0;36;47m} txt
regsub -all "11,00" $txt {[1m} txt
regsub -all "11,0" $txt {[1m} txt
regsub -all "11" $txt {[1m} txt
regsub -all "12,00" $txt {[34m} txt
regsub -all "12,0" $txt {[34m} txt
regsub -all "12" $txt {[34m} txt
regsub -all "13,00" $txt {[35m} txt
regsub -all "13,0" $txt {[35m} txt
regsub -all "13" $txt {[35m} txt
regsub -all "01,01" $txt {[1;30m} txt
regsub -all "1,01" $txt {[1;30m} txt
regsub -all "01,1" $txt {[1;30m} txt
regsub -all "1,1" $txt {[1;30m} txt
regsub -all "02,01" $txt {[0;34m} txt
regsub -all "2,01" $txt {[0;34m} txt
regsub -all "02,1" $txt {[0;34m} txt
regsub -all "2,1" $txt {[0;34m} txt
regsub -all "03,01" $txt {[32m} txt
regsub -all "3,01" $txt {[32m} txt
regsub -all "03,1" $txt {[32m} txt
regsub -all "3,1" $txt {[32m} txt
regsub -all "04,01" $txt {[31m} txt
regsub -all "4,01" $txt {[31m} txt
regsub -all "04,1" $txt {[31m} txt
regsub -all "4,1" $txt {[31m} txt
regsub -all "05,01" $txt {[33m} txt
regsub -all "5,01" $txt {[33m} txt
regsub -all "05,1" $txt {[33m} txt
regsub -all "5,1" $txt {[33m} txt
regsub -all "06,01" $txt {[35m} txt 
regsub -all "6,01" $txt {[35m} txt 
regsub -all "06,1" $txt {[35m} txt 
regsub -all "6,1" $txt {[35m} txt 
regsub -all "07,01" $txt {[37m07} txt
regsub -all "7,01" $txt {[37m07} txt
regsub -all "07,1" $txt {[37m07} txt
regsub -all "7,1" $txt {[37m07} txt
regsub -all "08,01" $txt {[1;33m} txt
regsub -all "8,01" $txt {[1;33m} txt
regsub -all "08,1" $txt {[1;33m} txt
regsub -all "8,1" $txt {[1;33m} txt
regsub -all "09,01" $txt {[32m} txt
regsub -all "9,01" $txt {[32m} txt
regsub -all "09,1" $txt {[32m} txt
regsub -all "9,1" $txt {[32m} txt
regsub -all "10,1" $txt {[0;36m} txt
regsub -all "10,01" $txt {[0;36m} txt
regsub -all "11,01" $txt {[1m} txt 
regsub -all "11,1" $txt {[1m} txt 
regsub -all "12,01" $txt {[34m} txt
regsub -all "12,1" $txt {[34m} txt
regsub -all "13,01" $txt {[35m} txt
regsub -all "13,1" $txt {[35m} txt
regsub -all "01,01" $txt {[30;44m} txt
regsub -all "1,01" $txt {[30;44m} txt
regsub -all "01,1" $txt {[30;44m} txt
regsub -all "1,1" $txt {[30;44m} txt

01,02[34m       [32m03,02 [31m04,02 [33m05,02 [35m06,02   [37;40ma[35;44m    [1;33m08,02 [32m09,02[33m [0;36;44m10,02 [1m11,02 [34m12,02 [35m13,02[0m
[30;42m01,03[34m 02,03       [31m04,03 [33m05,03 [35m06,03  [37;40mgold[35;42m  [1;33m08,03 [32m09,03 [0;36;42m10,03 [1m11,03 [34m12,03 [35m13,03[0m
[30;41m01,04[34m 02,04 [32m03,04      [33m 05,04 [35m06,04  [37;40mcolor[35;41m [1;33m08,04 [32m09,04 [0;36;41m10,04 [1m11,04 [34m12,04 [35m13,04[0m
[30;43m01,05[34m 02,05 [32m03,05 [31m04,05       [35m06,05        [1;33m08,05 [32m09,05 [0;36;43m10,05 [1m11,05 [34m12,05 [35m13,05[0m
[30;45m01,06[34m 02,06 [32m03,06 [31m04,06 [33m05,06              [1m08,06 [32m09,06 [0;36;45m10,06 [1m11,06 [34m12,06 [35m13,06[0m

[30;46m01,10[34m 02,10 [32m03,10 [31m04,10 [33m05,10 [35m06,10        [1;33m08,10 [32m09,10       [36m11,10 [34m12,10 [35m13,10[0m
[0m
}
