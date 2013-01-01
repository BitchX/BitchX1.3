
#ifndef _INC_COLOR
#define _INC_COLOR

/*
 * DEFAULT_DOUBLE STATUS  can be defined as 0 or 1
 * 0 makes the status line be 1 line only. 1 makes it 2 lines
 * this will be the default for all created windows.
 * DEFAULT_STATUS_LINES can be defined as 0 or 1
 * 0 is off, and 1 makes the window split the default for ALL windows.
 * a window split is the topic bar.
 */

#define NO_NUMERICS_STR "-:-"

#ifdef HUMBLE

#define DEFAULT_FORMAT_381_FSET "%K>%n>%W> You are now an %GIRC%n whore"
#define DEFAULT_FORMAT_391_FSET NULL

#define DEFAULT_FORMAT_471_FSET "$G $1 :Sorry, cannot join channel. %K(%WChannel is full%K)%n"
#define DEFAULT_FORMAT_473_FSET "$G $1 :Sorry, cannot join channel. %K(%WInvite only channel%K)%n"
#define DEFAULT_FORMAT_474_FSET "$G $1 :Sorry, cannot join channel. %K(%WBanned from channel%K)%n"
#define DEFAULT_FORMAT_475_FSET "$G $1 :Sorry, cannot join channel. %K(%WBad channel key%K)%n"
#define DEFAULT_FORMAT_476_FSET "$G $1 :%K(%WYou are not opped%K)%n"

/* Done ACTION */
#ifdef ONLY_STD_CHARS
#define DEFAULT_FORMAT_ACTION_FSET "%@%K* %W$1 %n$4-"
#define DEFAULT_FORMAT_ACTION_AR_FSET "%@%K* %Y$1 %n$4-"
#define DEFAULT_FORMAT_ACTION_CHANNEL_FSET "%@%K* %W$1 %n$4-"
#define DEFAULT_FORMAT_ACTION_OTHER_FSET "%@%K* %c$3%K>%W$1 %n$4-"
#define DEFAULT_FORMAT_ACTION_OTHER_AR_FSET "%@%K* %c$3%K>%Y$1 %n$4-"
#define DEFAULT_FORMAT_ACTION_USER_FSET "%@%K* %c$3%K>%W$1 %n$4-"
#define DEFAULT_FORMAT_ACTION_USER_AR_FSET "%@%K* %c$3%K>%Y$1 %n$4-"
#else /* ONLY_STD_CHARS */
#define DEFAULT_FORMAT_ACTION_FSET ansi?"%@%Kð %W$1 %n$4-":"%@ð $1 $4-"
#define DEFAULT_FORMAT_ACTION_AR_FSET ansi?"%@%Kð %Y$1 %n$4-":"%@ð $1 $4-"
#define DEFAULT_FORMAT_ACTION_CHANNEL_FSET ansi?"%@%Kð %W$1 %n$4-":"%@ð $1 $4-"
#define DEFAULT_FORMAT_ACTION_OTHER_FSET ansi?"%@%Kð %c$3%K>%W$1 %n$4-":"%@ð $3>$1 $4-"
#define DEFAULT_FORMAT_ACTION_OTHER_AR_FSET ansi?"%@%Kð %c$3%K>%Y$1 %n$4-":"%@ð $3>$1 $4-"
#define DEFAULT_FORMAT_ACTION_USER_FSET ansi?"%@%Kð %c$3%K>%W$1 %n$4-":"%@ð $3>$1 $4-"
#define DEFAULT_FORMAT_ACTION_USER_AR_FSET ansi?"%@%Kð %c$3%K>%Y$1 %n$4-":"%@ð $3>$1 $4-"
#endif /* ONLY_STD_CHARS */

/* Done ALIAS, ASSIGN */
#define DEFAULT_FORMAT_ALIAS_FSET "$G    %K$tolower($[10]0)%w $1-"
#define DEFAULT_FORMAT_ASSIGN_FSET "$G    %K$tolower($[10]0)%w $1-"

#define DEFAULT_FORMAT_AWAY_FSET "is away: ($3-) $1 $2"
#define DEFAULT_FORMAT_BACK_FSET "is back from the dead. Gone $1 hrs $2 min $3 secs"

/* Done BANS */
#define DEFAULT_FORMAT_BANS_HEADER_FSET "#  Channel    SetBy        Sec  Ban"
#define DEFAULT_FORMAT_BANS_FSET "%W$[-2]0%K:%n $[26]2 %K(%c$[20]3%K:%c$[24]4-%K)"
#define DEFAULT_FORMAT_BANS_FOOTER_FSET "%K[ %nEnd of channel %W$0 %K- %W$1 %nbans counted %K]"

#define DEFAULT_FORMAT_EBANS_HEADER_FSET "#  Channel    SetBy        Sec  ExemptBan"
#define DEFAULT_FORMAT_EBANS_FSET "%W$[-2]0%K:%n $[26]2 %K(%c$[20]3%K:%c$[24]4-%K)"
#define DEFAULT_FORMAT_EBANS_FOOTER_FSET "%K[ %nEnd of channel %W$0 %K- %W$1 %nexemptbans counted %K]"

#define DEFAULT_FORMAT_BITCH_FSET "%RBitch Mode Activated%n $1 $4 $5 on $3"
#define DEFAULT_FORMAT_BOT_HEADER_FSET "Aop Prot Bot         Channel    Hostname"
#define DEFAULT_FORMAT_BOT_FOOTER_FSET "There are $1 on the BotList"
#define DEFAULT_FORMAT_BOT_FSET "$[2]0 $[2]1 $[11]2 $[10]3 $4"

#define DEFAULT_FORMAT_BWALL_FSET "%K[%cWallOp%K/%c$2%K] %n$4-"

#define DEFAULT_FORMAT_CHANNEL_SIGNOFF_FSET "$G %nSignOff%K: %W$1 %K(%n$4-%K)"
#define DEFAULT_FORMAT_CHANNEL_URL_FSET "$G URL for %c$1%K:%n $2-"
#define DEFAULT_FORMAT_CONNECT_FSET "$G Connecting to server $1/%c$2%n"

/* Done CTCP */
#define DEFAULT_FORMAT_CTCP_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4 from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested $4 from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_FUNC_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested $4 from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_FUNC_USER_FSET " %K>%n>%W> %C$1 %K[%c$2%K]%c requested $4 from you"
#define DEFAULT_FORMAT_CTCP_CLOAK_UNKNOWN_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested unknown ctcp $4 from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_UNKNOWN_USER_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested unknown ctcp $4 from you"
#define DEFAULT_FORMAT_CTCP_CLOAK_USER_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested $4 from you"
#define DEFAULT_FORMAT_CTCP_FUNC_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4 from $3"
#define DEFAULT_FORMAT_CTCP_FUNC_USER_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4 from you"
#define DEFAULT_FORMAT_CTCP_UNKNOWN_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested unknown ctcp $4 from $3"
#define DEFAULT_FORMAT_CTCP_UNKNOWN_USER_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested unknown ctcp $4 from you"
#define DEFAULT_FORMAT_CTCP_USER_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4 from you"
#define DEFAULT_FORMAT_CTCP_REPLY_FSET "$G %nCTCP %W$3 %nreply from %n$1: $4-"

/* Done DCC */
#define DEFAULT_FORMAT_DCC_CHAT_FSET "%K[%G$1%K(%gdcc%K)] %n$3-"
#define DEFAULT_FORMAT_DCC_CHAT_AR_FSET "%K[%Y$1%K(%Ydcc%K)] %n$3-"
#define DEFAULT_FORMAT_DCC_CONNECT_FSET "$G DCC $1 %nconnection with %W$2%K[%c$4 $5%K]%n established"
#define DEFAULT_FORMAT_DCC_ERROR_FSET "$G %rDCC lost %w$1%w %rto $2 %K[%w$3-%K]"
#define DEFAULT_FORMAT_DCC_LOST_FSET "$G %nDCC %W$1%n:%g$2%n %K[%C$3%K]%n $4 $5 completed in $6 secs %K(%W$7 kb/sec%K)"
#define DEFAULT_FORMAT_DCC_REQUEST_FSET "$G DCC $1 %K(%n$2 $3%K)%n request from %W$4%K[%c$5:$6%K]"

#define DEFAULT_FORMAT_DESYNC_FSET "$G $1 is desynced from $2 at $0"
#define DEFAULT_FORMAT_DISCONNECT_FSET "$G Use %G/Server%n to connect to a server"
#define DEFAULT_FORMAT_ENCRYPTED_NOTICE_FSET "%K-%Y$1%K(%p$2%K)-%n $3-"
#define DEFAULT_FORMAT_ENCRYPTED_PRIVMSG_FSET "%K[%Y$1%K(%p$2%K)]%n $3-"
#define DEFAULT_FORMAT_FLOOD_FSET "%Y$1%n flood detected from %G$2%K(%g$3%K)%n on %K[%G$4%K]"
#define DEFAULT_FORMAT_FRIEND_JOIN_FSET "$G %R$1 %K[%c$2%K]%n has joined $3"
#define DEFAULT_FORMAT_HELP_FSET "$0-"
#define DEFAULT_FORMAT_HOOK_FSET "$0-"
#define DEFAULT_FORMAT_INVITE_FSET "%K>%n>%W> $1 Invites You to $2-"
#define DEFAULT_FORMAT_INVITE_USER_FSET "%K>%n>%W> Inviting $1 to $2-"

/* Done JOIN, LEAVE, KICK, KILL */
#define DEFAULT_FORMAT_JOIN_FSET "$G %C$1 %K[%c$2%K]%n has joined $3"
#define DEFAULT_FORMAT_LEAVE_FSET "$G $1 %K[%w$2%K]%n has left $3-"
#define DEFAULT_FORMAT_KICK_FSET "$G %n$3 was kicked off $2 by %c$1 %K(%n$4-%K)"
#define DEFAULT_FORMAT_KICK_USER_FSET "$G %WYou%n have been kicked off $2 by %c$1 %K(%n$4-%K)"
#define DEFAULT_FORMAT_KILL_FSET "$G %WYou%n have been killed by $1 for $2-"

/* Done LINKS */
#define DEFAULT_FORMAT_LINKS_FSET "$G %K[%c$[-3]0%K] [%c$[-3]3%K] [%n$[-27]1%K]%n <- %K[%n$[-27]2%K]"
#define DEFAULT_FORMAT_LINKS_HEADER_FSET "$G %K[%cNum%K] [%cHop%K]                      [%cServer%K]                         [%cUplink%K]"

/* Done LIST */
#define DEFAULT_FORMAT_LIST_FSET "$G $[12]1 $[-5]2   $[50]3-"
#define DEFAULT_FORMAT_MAIL_FSET "%K>%n>%W> You've got mail!@#"
#define DEFAULT_FORMAT_MSGCOUNT_FSET "[$0-]"

#define DEFAULT_FORMAT_MSGLOG_FSET "[$[8]0] [$1] - $2-"

/* Done MODE SMODE MODE_CHANNEL */
#define DEFAULT_FORMAT_MODE_FSET "$G %nmode%K/%c$3 %K[%W$4-%K]%n by %W$1"
#define DEFAULT_FORMAT_SMODE_FSET "$G %nmodehack%K/%c$2 %K[%W$3-%K]%n by %W$1"
#define DEFAULT_FORMAT_MODE_CHANNEL_FSET "$G %nmode%K/%c$3 %K[%W$4-%K]%n by %W$1"

/* Done MSG */
#define DEFAULT_FORMAT_MSG_FSET "%K[%P$1%K(%p$2%K)]%n $3-"
#define DEFAULT_FORMAT_MSG_AR_FSET "%K[%Y$1%K(%Y$2%K)]%n $3-"

#define DEFAULT_FORMAT_OPER_FSET "%C$1 %K[%c$2%K]%n )s now %Wan%w %GIRC%n whore"

#define DEFAULT_FORMAT_IGNORE_INVITE_FSET "%K>%n>%W> You have been invited to $1-"
#define DEFAULT_FORMAT_IGNORE_MSG_FSET "%K[%P$1%P$2%K(%p$3%K)]%n $4-"
#define DEFAULT_FORMAT_IGNORE_MSG_AWAY_FSET "%K[%P$1%P$2%K(%p$3%K)]%n $4-"
#define DEFAULT_FORMAT_IGNORE_NOTICE_FSET "%K-%P$2%K(%p$3%K)-%n $4-"
#define DEFAULT_FORMAT_IGNORE_WALL_FSET "%K%P$1%n $2-"
/* Done MSG */
#define DEFAULT_FORMAT_MSG_GROUP_FSET "%K-%P$1%K:%p$3%K-%n $4-"
#define DEFAULT_FORMAT_MSG_GROUP_AR_FSET "%K-%Y$1%K:%Y$3%K-%n $4-"

/* Done NAMES */
#define DEFAULT_FORMAT_NAMES_BANNER_FSET "$G "
#define DEFAULT_FORMAT_NAMES_FSET "$G %K[%cUsers%K(%W$1%K:%W$2%K/%W$3%K)]%c $4"
#define DEFAULT_FORMAT_NAMES_NONOP_FSET "$G %K[%cNonChanOps%K(%W$1%K:%W$2%K/%W$3%K)]%c $4"
#define DEFAULT_FORMAT_NAMES_OP_FSET "$G %K[%cChanOps%K(%W$1%K:%W$2%K/%W$3%K)]%c $4"
#define DEFAULT_FORMAT_NAMES_PROMPT_FSET "$G $0-"
#define DEFAULT_FORMAT_NAMES_VOICE_FSET "$G %K[%cVoiceUsers%K(%W$1%K:%W$2%K/%W$3%K)]%c $4"
#define DEFAULT_FORMAT_NAMES_NICK_FSET "%B$[10]0"
#define DEFAULT_FORMAT_NAMES_NICK_BOT_FSET "%G$[10]0"
#define DEFAULT_FORMAT_NAMES_NICK_FRIEND_FSET "%Y$[10]0"
#define DEFAULT_FORMAT_NAMES_NICK_ME_FSET "%W$[10]0"
#define DEFAULT_FORMAT_NAMES_NICK_SHIT_FSET "%R$[10]0"

#define DEFAULT_FORMAT_NAMES_USER_FSET "%K[ %n$1-%K]"
#define DEFAULT_FORMAT_NAMES_USER_CHANOP_FSET "%K[%C$0%n$1-%K]"
#define DEFAULT_FORMAT_NAMES_USER_IRCOP_FSET "%K[%R$0%n$1-%K]"
#define DEFAULT_FORMAT_NAMES_USER_VOICE_FSET "%K[%M$0%n$1-%K]"
/* In bx but not hades*/
#define DEFAULT_FORMAT_NAMES_BOT_FSET "$G %K[%cBots%K(%W$1%K:%W$2%K/%W$3%K)]%c $4"
#define DEFAULT_FORMAT_NAMES_FRIEND_FSET "$G %K[%cFriends%K(%W$1%K:%W$2%K/%W$3%K)]%c $4"
#define DEFAULT_FORMAT_NAMES_IRCOP_FSET "$G %K[%cIrcOps%K(%W$1%K:%W$2%K/%W$3%K)]%c $4"
#define DEFAULT_FORMAT_NAMES_SHIT_FSET "$G %K[%cShitUsers%K(%W$1%K:%W$2%K/%W$3%K)]%c $4"

#define DEFAULT_FORMAT_NETADD_FSET "$G %nAdded: %W$1 $2"
#define DEFAULT_FORMAT_NETSPLIT_FSET "$G %nNetsplit detected%K: %W$1%n split from %W$2 %K[%c$0%K]"
#define DEFAULT_FORMAT_NETJOIN_FSET "$G %nNetjoined%K: %W$1%n to %W$2"

/* nickname_fset not in hades*/
#define DEFAULT_FORMAT_NICKNAME_FSET "$G %W$1 %nis now known as %c$3"
#define DEFAULT_FORMAT_NICKNAME_OTHER_FSET "$G %W$1 %nis now known as %c$3"
#define DEFAULT_FORMAT_NICKNAME_USER_FSET "$G %WYou%K(%n$1%K)%n are now known as %c$3"

/* Done NONICK, no NONICK_FROM in bx */
#define DEFAULT_FORMAT_NONICK_FSET "$G %W$1%K:%n $3-"
/*#define DEFAULT_FORMAT_NONICK_FROM_FSET "$G %W$1%K:%n $3- %K[%Cfrom %c$2%K]"*/

#define DEFAULT_FORMAT_NOTE_FSET "($0) ($1) ($2) ($3) ($4) ($5-)"

#define DEFAULT_FORMAT_NOTICE_FSET "%K-%P$1%K-%n $2-"

#define DEFAULT_FORMAT_REL_FSET  "%K[%rmsg->$1%K]%n $4-"
#define DEFAULT_FORMAT_RELN_FSET "-$1:$3- $4-"
#define DEFAULT_FORMAT_RELM_FSET "[$1($2)] $4-"

#define DEFAULT_FORMAT_RELS_FSET "[***] $0-"
#define DEFAULT_FORMAT_RELSN_FSET "-> [notice($1)] $2-"
#define DEFAULT_FORMAT_RELSM_FSET "-> [msg($1)] $2-"

#define DEFAULT_FORMAT_NOTIFY_SIGNOFF_FSET "$G %GSignoff%n by %r$1%K!%r$2%n at $0"
#define DEFAULT_FORMAT_NOTIFY_SIGNON_FSET "$G %GSignon%n by %R$1%K!%R$2%n at $0"
#define DEFAULT_FORMAT_PASTE_FSET "%K[%W$1%K]%n $2-"

/* Done PUBLIC */
#define DEFAULT_FORMAT_PUBLIC_FSET "%b<%n$1%b>%n $3-"
#define DEFAULT_FORMAT_PUBLIC_AR_FSET "%b<%Y$1%b>%n $3-"
#define DEFAULT_FORMAT_PUBLIC_MSG_FSET "%b(%n$1%K/%n$3%b)%n $4-"
#define DEFAULT_FORMAT_PUBLIC_MSG_AR_FSET "%b(%Y$1%K/%Y$3%b)%n $4-"
#define DEFAULT_FORMAT_PUBLIC_NOTICE_FSET "%K-%P$1%K:%p$3%K-%n $5-"
#define DEFAULT_FORMAT_PUBLIC_NOTICE_AR_FSET "%K-%Y$1%K:%Y$3%K-%n $5-"
#define DEFAULT_FORMAT_PUBLIC_OTHER_FSET "%b<%n$1%K:%n$2%b>%n $3-"
#define DEFAULT_FORMAT_PUBLIC_OTHER_AR_FSET "%b<%Y$1%K:%Y$2%b>%n $3-"

/* Done SEND */
#define DEFAULT_FORMAT_SEND_ACTION_FSET "%Kð %W$1 %n$3-"
#define DEFAULT_FORMAT_SEND_ACTION_OTHER_FSET "%Kð %n-> %c$2%K>%W$1 %n$3-"
#define DEFAULT_FORMAT_SEND_DCC_CHAT_FSET "%K[%rdcc%K(%R$1%K)] %n$2-"
#define DEFAULT_FORMAT_SEND_MSG_FSET "%K[%rmsg%K(%R$1%K)] %n$3-"
#define DEFAULT_FORMAT_SEND_NOTICE_FSET "%K[%rnotice%K(%R$1%K)] %n$3-"
#define DEFAULT_FORMAT_SEND_PUBLIC_FSET "%p<%n$2%p>%n $3-"
#define DEFAULT_FORMAT_SEND_PUBLIC_OTHER_FSET "%p<%n$2%K:%n$1%p>%n $3-"
/* Not in Hades*/
#define DEFAULT_FORMAT_SEND_AWAY_FSET "[Away ($strftime($1 %a %b %d %I:%M%p %Z))] [$tdiff2(${time() - u})] [BX-MsgLog $2]"
#define DEFAULT_FORMAT_SEND_CTCP_FSET "%K[%rctcp%K(%R$1%K)] %n$2"
#define DEFAULT_FORMAT_SEND_RCTCP_FSET "%K[%rrctcp%K(%R$1%K)] %n$2-"
#define DEFAULT_FORMAT_SEND_ENCRYPTED_NOTICE_FSET "%K-%Y$1%K(%p$2%K)-%n $2-"
#define DEFAULT_FORMAT_SEND_ENCRYPTED_MSG_FSET "%K[%Y$1%K(%p$2%K)]%n $2-"


#define DEFAULT_FORMAT_SERVER_FSET "$G%n $1: $2-"
#define DEFAULT_FORMAT_SERVER_MSG1_FSET "$G%n $1: $2-"
#define DEFAULT_FORMAT_SERVER_MSG1_FROM_FSET "$G%n $1: $2-"
#define DEFAULT_FORMAT_SERVER_MSG2_FSET "$G%n $1-"
#define DEFAULT_FORMAT_SERVER_MSG2_FROM_FSET "$G%n $1-"

#define DEFAULT_FORMAT_SERVER_NOTICE_FSET "%K[%c***%K] %n$2-"
/*#define DEFAULT_FORMAT_SERVER_NOTICE_FSET "$2-"*/
#define DEFAULT_FORMAT_SERVER_NOTICE_BOT_FSET "Possible bot: %C$1 %K[%c$2-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_BOT1_FSET "Possible $1 bot: %C$2 %K[%c$3-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_BOT_ALARM_FSET "$1 alarm activated: %C$2 %K[%c$3-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_CONNECT_FSET "Client Connecting: %C$1 %K[%c$2%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_EXIT_FSET "Client Exiting: %C$1 %K[%c$2-%K] %K[%c$3-%K]"
#define DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_INVALID_FSET "Invalid username: %C$1%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_TERM_FSET "Terminating client for excess flood %C$1%K [%c$2-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_FAKE_FSET "Fake Mode detected on $1 -> $2-"
#define DEFAULT_FORMAT_SERVER_NOTICE_KILL_FSET "Foreign OperKill: %W$1%n killed %c$2%n %K(%n$3-%K)%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_KILL_LOCAL_FSET "Local OperKill: %W$1%n killed %c$2%n %K(%n$3-%K)%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_KLINE_FSET "%W$1%n added a new K-Line %K[%c$2%K]%n for $3-"
#define DEFAULT_FORMAT_SERVER_NOTICE_GLINE_FSET "%W$1%n added a new K-Line %K[%c$2%K]%n from $3 for $4-"
#define DEFAULT_FORMAT_SERVER_NOTICE_NICK_COLLISION_FSET "Nick collision %W$1%n killed %c$2%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_OPER_FSET "%C$1 %K[%c$2%K]%n is now %Wan%w %GIRC%n whore"
#define DEFAULT_FORMAT_SERVER_NOTICE_REHASH_FSET "%W$1%n is rehashing the Server config file"
#define DEFAULT_FORMAT_SERVER_NOTICE_STATS_FSET "Stats $1: %C$2 %K[%c$3-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_TRAFFIC_HIGH_FSET "Entering high-traffic mode %K(%n$1 > $2-%K)%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_TRAFFIC_NORM_FSET "Resuming standard operation %K(%n$1 $2 $3-%K)%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_UNAUTH_FSET "Unauthorized Connection from $1-"

#define DEFAULT_FORMAT_SET_FSET "$G    %K$tolower($[27]0) %w$1-"
#define DEFAULT_FORMAT_CSET_FSET "%r$[-14]1 %R$[-20.]0   %w$2-"
#define DEFAULT_FORMAT_SET_NOVALUE_FSET "$G    %K$tolower($0) has no value"
#define DEFAULT_FORMAT_SHITLIST_FSET " $[3]0 $[34]1 $[10]2  $3-"
#define DEFAULT_FORMAT_SHITLIST_FOOTER_FSET "There are $1 users on the shitlist"
#define DEFAULT_FORMAT_SHITLIST_HEADER_FSET " lvl nick!user@host                     channels   reason"
#define DEFAULT_FORMAT_SIGNOFF_FSET "$G %nSignOff%K: %W$1 %K(%n$3-%K)"

#define DEFAULT_FORMAT_SILENCE_FSET "$G %RWe are $1 silencing $2 at $0"

#define DEFAULT_FORMAT_TRACE_OPER_FSET "%R$1%n %K[%n$3%K]"
#define DEFAULT_FORMAT_TRACE_SERVER_FSET "%R$1%n $2 $3 $4 %K[%n$5%K]%n $6-"
#define DEFAULT_FORMAT_TRACE_USER_FSET "%R$1%n %K[%n$3%K]"

#define DEFAULT_FORMAT_TIMER_FSET "$G $[-5]0 $[-10]1 $[-6]2 $3-"

/* Done TOPIC */
#define DEFAULT_FORMAT_TOPIC_FSET "$G Topic for %c$1%K:%n $2-"
#define DEFAULT_FORMAT_TOPIC_CHANGE_FSET "$G %W$1 %nhas changed the topic on channel %W$2%n to%K:%n $3-"
#define DEFAULT_FORMAT_TOPIC_SETBY_FSET "$G %nTopic set by %c$2%K [%c$stime($3) ago%K]"
#define DEFAULT_FORMAT_TOPIC_UNSET_FSET "$G %nTopic unset by $1 on $2"

#define DEFAULT_FORMAT_USAGE_FSET "$G Usage: /$0  $1-"
#define DEFAULT_FORMAT_USERMODE_FSET "$G %nMode change %K[%W$4-%K]%n for user %c$3"
#define DEFAULT_FORMAT_USERMODE_OTHER_FSET "$G %nMode change %K[%W$4-%K]%n for user %c$3%n by %W$1"

#define DEFAULT_FORMAT_USERLIST_FSET "$[16]0 $[10]1 $[-10]2   $[-25]3 $[10]4"

#define DEFAULT_FORMAT_USERLIST_FOOTER_FSET "There are $1 users on the userlist"
#define DEFAULT_FORMAT_USERLIST_HEADER_FSET "level            nick       password     host                      channels"

#define DEFAULT_FORMAT_USERS_HEADER_FSET "%K[ %WC%nhannel   %K] [ %WN%wickname  %K] [ %WU%wser@host                            %K] [ %WL%wevel %K]"
#define DEFAULT_FORMAT_USERS_FSET "%K[%n$[11]1%K] [%C$5%c$[10]2%K] [%n$[38]3%K][%n$[7]0%K]"
#define DEFAULT_FORMAT_USERS_USER_FSET "%K[%n$[11]1%K] [%C$5%c$[10]2%K] [%p$[38]3%K][%n$[7]0%K]"
#define DEFAULT_FORMAT_USERS_SHIT_FSET "%K[%n$[11]1%K] [%C$5%c$[10]2%K] [%r$[38]3%K][%n$[7]0%K]"
#define DEFAULT_FORMAT_USERS_TITLE_FSET NULL


/* #define DEFAULT_FORMAT_USERS_FOOTER_FSET "%K[ %nEnd of channel %W$0 %K- %W$1 %nusers counted %K]"*/
/* #define DEFAULT_FORMAT_USERS_NONE_FSET "$G No match of %W$0%n on %c$1"*/

#define DEFAULT_FORMAT_VERSION_FSET "\002$0+$4$5\002 by panasync \002-\002 $2 $3"

#define DEFAULT_FORMAT_WALL_FSET "%G!%g$1:$2%G!%n $3-"
#define DEFAULT_FORMAT_WALL_AR_FSET "%G!%Y$1:$2%G!%n $3-"

#define DEFAULT_FORMAT_WALLOP_FSET "%K[%cWallOp%K] %n$2- ($1)"
#define DEFAULT_FORMAT_WHO_FSET "$[7]0 %W$[9]1%w $[3]2 $3@$4 ($6-)"

/* Done WHOIS WHOLEFT WHOWAS */
#define DEFAULT_FORMAT_WHOIS_AWAY_FSET "%K³ %Wa%nway     %K:%n $1-"
#define DEFAULT_FORMAT_WHOIS_BOT_FSET "%g³ %Wb%not      %K:%n A:$0 P:$1 [$2] $3-"
#define DEFAULT_FORMAT_WHOIS_CHANNELS_FSET "%g³ %Wc%nhannels %K:%n $0-"
#define DEFAULT_FORMAT_WHOIS_FRIEND_FSET "%g³ %Wf%nriend   %K:%n L:$0 A:$1 P:$2 $3-"
#define DEFAULT_FORMAT_WHOIS_HEADER_FSET "%GÚÄÄÄÄÄÄÄÄ%gÄ%GÄÄ%gÄÄ%GÄ%gÄÄÄÄÄÄÄÄÄ%KÄ%gÄÄ%KÄÄ%gÄ%KÄÄÄÄÄÄÄÄÄÄ-Ä ÄÄ  Ä   -"
#define DEFAULT_FORMAT_WHOIS_IDLE_FSET "%K. %Wi%ndle     %K:%n $0 hours $1 mins $2 secs (signon: $stime($3))"
#define DEFAULT_FORMAT_WHOIS_SHIT_FSET "%g³ %Ws%nhit     %K:%n L:$0 [$1] $2 $3-"
#define DEFAULT_FORMAT_WHOIS_SIGNON_FSET "%K: %Ws%nignon   %K:%n $0-"
#define DEFAULT_FORMAT_WHOIS_ACTUALLY_FSET "%K| %Wa%nctually %K:%n $0-"
#define DEFAULT_FORMAT_WHOIS_CALLERID_FSET "%K! %Wc%nallerid %K:%n $0-"
#define DEFAULT_FORMAT_WHOIS_SECURE_FSET "%K! %Ws%necure   %K:%n $0-"
#define DEFAULT_FORMAT_WHOIS_NAME_FSET "%G³ %Wi%nrcname  %K:%n $0-"
#define DEFAULT_FORMAT_WHOIS_NICK_FSET "%G³ %W$0 %K(%n$1@$2%K) (%n$3-%K)"
#define DEFAULT_FORMAT_WHOIS_OPER_FSET "%K| %Wo%nperator %K:%n $0 $1-"
#define DEFAULT_FORMAT_WHOIS_SERVER_FSET "%K³ %Ws%nerver   %K:%n $0 ($1-)"
#define DEFAULT_FORMAT_WHOLEFT_HEADER_FSET "%PÚÄÄÄÄÄ %WWho %PÄÄÄÄÄ%pÄÄÄ%PÄÄÄ%pÄÄ%PÄÄÄÄÄ%pÄÄ%PÄÄ %WChannel%p Ä%KÄ %WServer %KÄ%pÄÄÄÄ%KÄÄ%pÄÄ%KÄÄ-- %WSeconds"
#define DEFAULT_FORMAT_WHOLEFT_USER_FSET "%P³%n $[-10]0!$[20]1 $[10]2 $[20]4 $3"
#define DEFAULT_FORMAT_WHOLEFT_FOOTER_FSET "%PÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%pÄÄ%PÄÄÄ%pÄÄÄ%PÄÄ%pÄ%PÄ%pÄÄÄÄÄ%KÄ%pÄÄÄ%KÄÄÄ%pÄÄ%KÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ-- --  -"
#define DEFAULT_FORMAT_WHOWAS_NICK_FSET "%G³ %W$0%n was %K(%n$1@$2%K)"
/* Not convered in hades*/
#define DEFAULT_FORMAT_WHOIS_ADMIN_FSET   "%K| %Wa%ndmin    : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_SERVICE_FSET "%K| %Ws%nervice  : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_HELP_FSET    "%K| %Wh%nelp     : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_REGISTER_FSET "%K| %Wr%negister : $0 - $1-"

#define DEFAULT_FORMAT_WIDELIST_FSET "$1-"
#define DEFAULT_FORMAT_WINDOW_SET_FSET "$0-"

#define DEFAULT_FORMAT_NICK_MSG_FSET "$0 $1 $2-"

#define DEFAULT_FORMAT_NICK_COMP_FSET "$0:$1-"
#define DEFAULT_FORMAT_NICK_AUTO_FSET "$0:$1-"

#define DEFAULT_FORMAT_STATUS_FSET "%4%W$0-"
#define DEFAULT_FORMAT_STATUS1_FSET "%4%W$0-"
#define DEFAULT_FORMAT_STATUS2_FSET "%4%W$0-"
#define DEFAULT_FORMAT_STATUS3_FSET "%4%W$0-"
#define DEFAULT_FORMAT_NOTIFY_OFF_FSET "$[10]0 $[35]1 $[-6]2 $[-10]3 $4 $5"
#define DEFAULT_FORMAT_NOTIFY_ON_FSET "$[10]0 $[35]1 $[-6]2 $[-10]3 $4-"
#define DEFAULT_FORMAT_OV_FSET "%S %>[1;30m[[1;32mOper[1;37mView[1;30;41m] "
#define DEFAULT_FORMAT_DEBUG_FSET "DEBUG"

#define DEFAULT_FORMAT_COMPLETE_FSET "%K[%n$[15]0%K] [%n$[15]1%K] [%n$[15]2%K] [%n$[15]3%K]"	

#define DEFAULT_FORMAT_WHOIS_FOOTER_FSET NULL
#define DEFAULT_FORMAT_XTERM_TITLE_FSET NULL
#define DEFAULT_FORMAT_DCC_FSET NULL
#define DEFAULT_FORMAT_NAMES_FOOTER_FSET NULL
#define DEFAULT_FORMAT_NETSPLIT_HEADER_FSET NULL
#define DEFAULT_FORMAT_TOPIC_CHANGE_HEADER_FSET NULL
#define DEFAULT_FORMAT_WHOWAS_HEADER_FSET NULL


/* Color.h by Humble - Lets try the hades look =) */

#define DEFAULT_STATUS_FORMAT "[1;30;40mÚ[0;37m%T %*[1;36m%N[0;37m%# %@%C%+ %A%W%H%B%M%Q %>%D %L [1;30m]"
#define DEFAULT_STATUS_FORMAT1 "[1;30;40mÚÄ [1;37mU[0;37mser[1;30m: %*[1;36m%N[0;37m%#%A %^%H%B %>%D %S [1;30mÄ¿"
#define DEFAULT_STATUS_FORMAT2 "[1;30;40m³  [1;37mC[0;37mhannel[1;30m:[0;37m %@%C%+%W %M%Q %>%T %L [1;30mÄÙ"

#define DEFAULT_STATUS_AWAY "[1;30;40m([1;37mA[1;30m)[0;37m"
#define DEFAULT_STATUS_CHANNEL "[1;36m%C[0;37m"
#define DEFAULT_STATUS_CHANOP "[0;36m@[0;37m"
#define DEFAULT_STATUS_HALFOP "[0;36m%[0;37m"
#define DEFAULT_STATUS_CLOCK "[1;37mT[0;37mime[1;30m: [1;36m%T[0;37m"
#define DEFAULT_STATUS_HOLD " [1;30;40m--[1;37mMore[1;30m--[0;37m"
#define DEFAULT_STATUS_HOLD_LINES " [1;37mH[0;37mold[1;30m: [1;36m%B[0;37m"
#define DEFAULT_STATUS_LAG " [1;37mL[0;37mag[1;30m: [1;36m%L[0;37m"
#define DEFAULT_STATUS_MODE "[1;30;40m([1;37m%+[1;30m)[0;37m"
#define DEFAULT_STATUS_MAIL " [1;37mM[0;37mail[1;30m: [1;36m%M[0;37m"
#define DEFAULT_STATUS_NOTIFY " [1;37mN[0;37motify[1;30m: [1;36m%F[0;37m"
#define DEFAULT_STATUS_OPER "[0;36m*[0;37m"
#define DEFAULT_STATUS_VOICE "[0;36m+[0;37m"
#define DEFAULT_STATUS_QUERY " [1;37mQ[0;37muery[1;30m: [1;36m%Q[0;37m"
#define DEFAULT_STATUS_SERVER "[1;37mS[0;37merver[1;30m: [1;36m%S[0;37m"
#define DEFAULT_STATUS_UMODE "[1;30;40m([1;37m+%#[1;30m)[0;37m"
#define DEFAULT_STATUS_OPER_KILLS "[1;30;40m[[1;37mnk [1;36m%d[1;30m:[1;37mok [1;36m%d[1;30m][0;37m"
#define DEFAULT_STATUS_WINDOW "[1;30;40m[[1;37mþ[1;30m][0;37m"

#define DEFAULT_STATUS_FORMAT3 "BitchX by panasync, Hades formats by Humble"
#define DEFAULT_STATUS_INSERT ""
#define DEFAULT_STATUS_MSGCOUNT " [1;37mM[0;37msgs[1;30m: ([1;36m%^[1;30m)[0;37m"
#define DEFAULT_STATUS_NICK "%N"
#define DEFAULT_STATUS_OVERWRITE "(overtype) "
#define DEFAULT_STATUS_TOPIC "%-"
#define DEFAULT_STATUS_USER " * type /help for help "
#define DEFAULT_STATUS_DCCCOUNT  "[DCC  gets/%& sends/%&]"
#define DEFAULT_STATUS_CDCCCOUNT "[CDCC gets/%| offer/%|]"
#define DEFAULT_STATUS_USERS "[0;44m[[37mO[36m/[1;37m%! [0;44mN[36m/[1;37m%! [0;44mI[36m/[1;37m%! [0;44mV[36m/[1;37m%! [0;44mF[36m/[1;37m%![0;44m]"
#define DEFAULT_STATUS_CPU_SAVER " (%J)"

#define DEFAULT_STATUS_USER1 ""
#define DEFAULT_STATUS_USER2 ""
#define DEFAULT_STATUS_USER3 ""
#define DEFAULT_STATUS_USER4 ""
#define DEFAULT_STATUS_USER5 ""
#define DEFAULT_STATUS_USER6 ""
#define DEFAULT_STATUS_USER7 ""
#define DEFAULT_STATUS_USER8 ""
#define DEFAULT_STATUS_USER9 ""
#define DEFAULT_STATUS_USER10 ""
#define DEFAULT_STATUS_USER11 ""
#define DEFAULT_STATUS_USER12 ""
#define DEFAULT_STATUS_USER13 ""
#define DEFAULT_STATUS_USER14 ""
#define DEFAULT_STATUS_USER15 ""
#define DEFAULT_STATUS_USER16 ""
#define DEFAULT_STATUS_USER17 ""
#define DEFAULT_STATUS_USER18 ""
#define DEFAULT_STATUS_USER19 ""
#define DEFAULT_STATUS_USER20 "" 
#define DEFAULT_STATUS_USER21 ""
#define DEFAULT_STATUS_USER22 ""
#define DEFAULT_STATUS_USER23 ""
#define DEFAULT_STATUS_USER24 ""
#define DEFAULT_STATUS_USER25 ""
#define DEFAULT_STATUS_USER26 ""
#define DEFAULT_STATUS_USER27 ""
#define DEFAULT_STATUS_USER28 ""
#define DEFAULT_STATUS_USER29 ""
#define DEFAULT_STATUS_USER30 "" 
#define DEFAULT_STATUS_USER31 ""
#define DEFAULT_STATUS_USER32 ""
#define DEFAULT_STATUS_USER33 ""
#define DEFAULT_STATUS_USER34 ""
#define DEFAULT_STATUS_USER35 ""
#define DEFAULT_STATUS_USER36 ""
#define DEFAULT_STATUS_USER37 ""
#define DEFAULT_STATUS_USER38 ""
#define DEFAULT_STATUS_USER39 ""
#define DEFAULT_STATUS_FLAG ""
#define DEFAULT_STATUS_SCROLLBACK " (Scroll)"

#define DEFAULT_FORMAT_LASTLOG_FSET "$strftime($0 %H:%M) $1-"

#define DEFAULT_INPUT_PROMPT "[1;30;40mÀ-[[1;37mb[0;37mitch[0;36mx[1;30m]Ä>[0;37m "

#ifndef ONLY_STD_CHARS
#define DEFAULT_SHOW_NUMERICS_STR "[1;30mù[0m[1;36mí[1;30mù[0m"
#else
#ifndef LATIN1
#define DEFAULT_SHOW_NUMERICS_STR "***"
#else
#define DEFAULT_SHOW_NUMERICS_STR "[1;30m-[0m[1;36m:[1;30m-[0m"
#endif
#endif


#define DEFAULT_SERVER_PROMPT "%K[%c$0%K] "

#define DEFAULT_FORMAT_WATCH_SIGNOFF_FSET "$G $0!$1@$2 has signed off"
#define DEFAULT_FORMAT_WATCH_SIGNON_FSET "$G $0!$1@$2 has signed on $3"

#else 

/* default bx */

#define DEFAULT_FORMAT_381_FSET "%K>%n>%W> You are now an %GIRC%n whore"
#define DEFAULT_FORMAT_391_FSET NULL

#define DEFAULT_FORMAT_471_FSET "$G [$1] Channel is full"
#define DEFAULT_FORMAT_473_FSET "$G [$1] Invite only channel"
#define DEFAULT_FORMAT_474_FSET "$G [$1] Banned from channel"
#define DEFAULT_FORMAT_475_FSET "$G [$1] Bad channel key"
#define DEFAULT_FORMAT_476_FSET "$G [$1] You are not opped"

#ifdef ONLY_STD_CHARS
#define DEFAULT_FORMAT_ACTION_FSET ansi?"%@%K* %W$1 %n$4-":"* $1 $4-"
#define DEFAULT_FORMAT_ACTION_AR_FSET ansi?"%@%K* %Y$1 %n$4-":"* $1 $4-"
#define DEFAULT_FORMAT_ACTION_CHANNEL_FSET ansi?"%@%K* %Y$1/$3 %n$4-":"* $1/$3 $4-"
#define DEFAULT_FORMAT_ACTION_OTHER_FSET ansi?"%@%K* %n>%c$1 %n$3-":"* >$1 $3-"
#define DEFAULT_FORMAT_ACTION_OTHER_AR_FSET ansi?"%@%K* %Y>%c$1 %n$3-":"* >$1 $3-"
#define DEFAULT_FORMAT_ACTION_USER_FSET ansi?"%@%K* %n>%c$1 %n$3-":"* >$1 $3-"
#define DEFAULT_FORMAT_ACTION_USER_AR_FSET ansi?"%@%K* %Y>%c$1 %n$3-":"* >$1 $3-"
#else
#define DEFAULT_FORMAT_ACTION_FSET ansi?"%@%Kð %W$1 %n$4-":"%@ð $1 $4-"
#define DEFAULT_FORMAT_ACTION_AR_FSET ansi?"%@%Kð %Y$1 %n$4-":"%@ð $1 $4-"
#define DEFAULT_FORMAT_ACTION_CHANNEL_FSET ansi?"%@%Kð %Y$1/$3 %n$4-":"%@ð $1/$3 $4-"
#define DEFAULT_FORMAT_ACTION_OTHER_FSET ansi?"%@%K* %n>%c$1 %n$3-":"%@* >$1 $3-"
#define DEFAULT_FORMAT_ACTION_OTHER_AR_FSET ansi?"%@%K* %Y>%c$1 %n$3-":"%@* >$1 $3-"
#define DEFAULT_FORMAT_ACTION_USER_FSET ansi?"%@%K* %n>%c$1 %n$3-":"%@* >$1 $3-"
#define DEFAULT_FORMAT_ACTION_USER_AR_FSET ansi?"%@%K* %Y>%c$1 %n$3-":"%@* >$1 $3-"
#endif



#define DEFAULT_FORMAT_ALIAS_FSET "Alias $[20.]0 $1-"
#define DEFAULT_FORMAT_ASSIGN_FSET "Assign $[20.]0 $1-"
#define DEFAULT_FORMAT_AWAY_FSET "is away: ($3-) $1 $2"	
#define DEFAULT_FORMAT_BACK_FSET "is back from the dead. Gone $1 hrs $2 min $3 secs"
#define DEFAULT_FORMAT_BANS_HEADER_FSET "#  Channel    SetBy        Sec  Ban"
#define DEFAULT_FORMAT_BANS_FSET "$[2]0 $[10]1 $[10]3 $[-5]numdiff($time() $4)  $2"

#define DEFAULT_FORMAT_EBANS_HEADER_FSET "#  Channel    SetBy        Sec  ExemptBan"
#define DEFAULT_FORMAT_EBANS_FSET "$[2]0 $[10]1 $[10]3 $[-5]numdiff($time() $4)  $2"

#define DEFAULT_FORMAT_BITCH_FSET "%RBitch Mode Activated%n $1 $4 $5 on $3"
#define DEFAULT_FORMAT_BOT_HEADER_FSET "Aop Prot Bot         Channel    Hostname"
#define DEFAULT_FORMAT_BOT_FOOTER_FSET "There are $1 on the BotList"
#define DEFAULT_FORMAT_BOT_FSET "$[2]0 $[2]1 $[11]2 $[10]3 $4"

#define DEFAULT_FORMAT_BWALL_FSET "[%GBX-Wall%n/%W$1:$2%n] $4-"

#define DEFAULT_FORMAT_CHANNEL_SIGNOFF_FSET "$G %nSignOff %W$1%n: $3 %K(%n$4-%K)"
#define DEFAULT_FORMAT_CHANNEL_URL_FSET "$G URL %K(%c$1%K):%n $2-"
#define DEFAULT_FORMAT_CONNECT_FSET "$G Connecting to server $1/%c$2%n"

#define DEFAULT_FORMAT_COMPLETE_FSET "%K[%n$[15]0%K] [%n$[15]1%K] [%n$[15]2%K] [%n$[15]3%K]"	
#define DEFAULT_FORMAT_CTCP_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested $4- from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_FUNC_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested $4- from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_FUNC_USER_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested $4- from you"
#define DEFAULT_FORMAT_CTCP_CLOAK_UNKNOWN_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested unknown ctcp $4- from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_UNKNOWN_USER_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested unknown ctcp $4- from $3"
#define DEFAULT_FORMAT_CTCP_CLOAK_USER_FSET "%K>%n>%W> %C$1 %K[%c$2%K]%c requested $4- from you"
#define DEFAULT_FORMAT_CTCP_FUNC_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from $3"
#define DEFAULT_FORMAT_CTCP_FUNC_USER_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from you"
#define DEFAULT_FORMAT_CTCP_UNKNOWN_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested unknown ctcp $4- from $3"
#define DEFAULT_FORMAT_CTCP_UNKNOWN_USER_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested unknown ctcp $4- from %g$3"
#define DEFAULT_FORMAT_CTCP_USER_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from you"
#define DEFAULT_FORMAT_CTCP_REPLY_FSET "$G %nCTCP %W$3 %nreply from %n$1: $4-"


#define DEFAULT_FORMAT_DCC_CHAT_FSET ansi?"%@%K[%G$1%K(%gdcc%K)] %n$3-":"%@[$1(dcc)] $3-"
#define DEFAULT_FORMAT_DCC_CHAT_AR_FSET ansi?"%@%K[%Y$1%K(%Ydcc%K)] %n$3-":"%@[$1(dcc)] $3-"
#define DEFAULT_FORMAT_DCC_CONNECT_FSET "$G %RDCC%n $1 with %W$2%K[%c$4:$5%K]%n established"
#define DEFAULT_FORMAT_DCC_ERROR_FSET "$G %RDCC%n lost %w$1%w %rto $2 %K[%w$3-%K]"
#define DEFAULT_FORMAT_DCC_LOST_FSET "%@$G %RDCC%n %W$1%n:%g$2%n %K[%C$3%K]%n $4 $5 completed in $6 secs %K(%W$7 $8/sec%K)"
#define DEFAULT_FORMAT_DCC_REQUEST_FSET "%@$G %RDCC%n $1 %K(%n$2%K)%n request from %W$3%K[%c$4 [$5:$6]%K]%n $8 $7"
#define DEFAULT_FORMAT_DESYNC_FSET "$G $1 is desynced from $2 at $0"
#define DEFAULT_FORMAT_DISCONNECT_FSET "$G Use %G/Server%n to connect to a server"
#define DEFAULT_FORMAT_ENCRYPTED_NOTICE_FSET "%@%K-%Y$1%K(%p$2%K)-%n $3-"
#define DEFAULT_FORMAT_ENCRYPTED_PRIVMSG_FSET "%@%K[%Y$1%K(%p$2%K)]%n $3-"
#define DEFAULT_FORMAT_FLOOD_FSET "%Y$1%n flood detected from %G$2%K(%g$3%K)%n on %K[%G$4%K]"
#define DEFAULT_FORMAT_FRIEND_JOIN_FSET "$G %R$1 %K[%c$2%K]%n has joined $3"
#define DEFAULT_FORMAT_HELP_FSET "$0-"
#define DEFAULT_FORMAT_HOOK_FSET "$0-"
#define DEFAULT_FORMAT_INVITE_FSET "%K>%n>%W> $1 Invites You to $2-"
#define DEFAULT_FORMAT_INVITE_USER_FSET "%K>%n>%W> Inviting $1 to $2-"
#define DEFAULT_FORMAT_JOIN_FSET ansi?"$G %C$1 %K[%c$2%K]%n has joined $3":"$G $1 [$2] has joined $3"
#define DEFAULT_FORMAT_KICK_FSET "$G %n$3 was kicked off $2 by %c$1 %K(%n$4-%K)"
#define DEFAULT_FORMAT_KICK_USER_FSET "%K>%n>%W> %WYou%n have been kicked off %c$2%n by %c$1 %K(%n$4-%K)"
#define DEFAULT_FORMAT_KILL_FSET "%K>%n>%W> %RYou have been killed by $1 for $2-"
#define DEFAULT_FORMAT_LEAVE_FSET "$G $1 %K[%w$2%K]%n has left $3 %K[%W$4-%K]"

#ifdef ONLY_STD_CHARS
#define DEFAULT_FORMAT_LINKS_FSET "%K|%n$[24]0%K| |%n$[24]1%K| |%n$[3]2%K| |%n$[13]3%K|"
#else
#define DEFAULT_FORMAT_LINKS_FSET "%K³%n$[24]0%K³ ³%n$[24]1%K³ ³%n$[3]2%K³ ³%n$[13]3%K³"
#endif

#define DEFAULT_FORMAT_LIST_FSET "$[12]1 $[-5]2   $[40]3-"
#define DEFAULT_FORMAT_MAIL_FSET "%K>%n>%W> You've got mail!@#"
#define DEFAULT_FORMAT_MSGCOUNT_FSET "[$0-]"
	
#define DEFAULT_FORMAT_MSGLOG_FSET "[$[8]0] [$1] - $2-"
	
#define DEFAULT_FORMAT_MODE_FSET ansi?"$G %nmode%K/%c$3 %K[%W$4-%K]%n by %W$1":"$G mode/$3 [$4-] by $1"
#define DEFAULT_FORMAT_SMODE_FSET "$G %RServerMode%K/%c$3 %K[%W$4-%K]%n by %W$1"

#define DEFAULT_FORMAT_MODE_CHANNEL_FSET "$G %nmode%K/%c$3 %K[%W$4-%K]%n by %W$1"

#define DEFAULT_FORMAT_MSG_FSET ansi?"%@%K[%P$1%K(%p$2%K)]%n $3-":"%@[$1($2)] $3-"
#define DEFAULT_FORMAT_MSG_AR_FSET ansi?"%@%K[%Y$1%K(%Y$2%K)]%n $3-":"%@[$1($2)] $3-"

#define DEFAULT_FORMAT_OPER_FSET "%C$1 %K[%c$2%K]%n )s now %Wan%w %GIRC%n whore"

#define DEFAULT_FORMAT_IGNORE_INVITE_FSET "%K>%n>%W> You have been invited to $1-"
#define DEFAULT_FORMAT_IGNORE_MSG_FSET "%K[%P$1%P$2%K(%p$3%K)]%n $4-"
#define DEFAULT_FORMAT_IGNORE_MSG_AWAY_FSET "%K[%P$1%P$2%K(%p$3%K)]%n $4-"
#define DEFAULT_FORMAT_IGNORE_NOTICE_FSET "%K-%P$2%K(%p$3%K)-%n $4-"
#define DEFAULT_FORMAT_IGNORE_WALL_FSET "%K%P$1%n $2-"
#define DEFAULT_FORMAT_MSG_GROUP_FSET "%K-%P$1%K:%p$2%K-%n $3-"
#define DEFAULT_FORMAT_MSG_GROUP_AR_FSET "%K-%Y$1%K:%Y$2%K-%n $3-"

#define DEFAULT_FORMAT_NAMES_FSET ansi?"$G %K[%GUsers%K(%g$1%K:%g$2%K/%g$3%K)]%c $4":"$G [Users($1:$2/$3)] $4"
#define DEFAULT_FORMAT_NAMES_BOT_FSET ansi?"$G %K[%GBots%K(%g$1%K:%g$2%K/%g$3%K)]%c $4":"$G [Bots($1:$2/$3)] $4"
#define DEFAULT_FORMAT_NAMES_FRIEND_FSET ansi?"$G %K[%GFriends%K(%g$1%K:%g$2%K/%g$3%K)]%c $4":"$G [Friends($1:$2/$3)] $4"
#define DEFAULT_FORMAT_NAMES_NONOP_FSET ansi?"$G %K[%GNonChanOps%K(%g$1%K:%g$2%K/%g$3%K)]%c $4":"$G [NonChanOps($1:$2/$3)] $4"
#define DEFAULT_FORMAT_NAMES_OP_FSET ansi?"$G %K[%GChanOps%K(%g$1%K:%g$2%K/%g$3%K)]%c $4":"$G [ChanOps($1:$2/$3)] $4"
#define DEFAULT_FORMAT_NAMES_IRCOP_FSET ansi?"$G %K[%GIrcOps%K(%g$1%K:%g$2%K/%g$3%K)]%c $4":"$G [IrcOps($1:$2/$3)] $4"
#define DEFAULT_FORMAT_NAMES_SHIT_FSET ansi?"$G %K[%MShitUsers%K(%m$1%K:%m$2%K/%m$3%K)]%c $4":"$G [ShitUsers(_$1:$2/$3)] $4"
#define DEFAULT_FORMAT_NAMES_VOICE_FSET ansi?"$G %K[%MVoiceUsers%K(%m$1%K:%m$2%K/%m$3%K)]%c $4":"$G [VoiceUsers($1:$2/$3)] $4"
#define DEFAULT_FORMAT_NAMES_NICK_FSET ansi?"%B$[10]0":"$[10]0"
#define DEFAULT_FORMAT_NAMES_NICK_BOT_FSET ansi?"%G$[10]0":"$[10]0"
#define DEFAULT_FORMAT_NAMES_NICK_FRIEND_FSET ansi?"%Y$[10]0":"$[10]0"
#define DEFAULT_FORMAT_NAMES_NICK_ME_FSET ansi?"%W$[10]0":"$[10]0"
#define DEFAULT_FORMAT_NAMES_NICK_SHIT_FSET ansi?"%R$[10]0":"$[10]0"

#define DEFAULT_FORMAT_NAMES_USER_FSET ansi?"%K[ %n$1-%K]":"[ $1-]"
#define DEFAULT_FORMAT_NAMES_USER_CHANOP_FSET ansi?"%K[%C$0%n$1-%K]":"[$0$1-]"
#define DEFAULT_FORMAT_NAMES_USER_IRCOP_FSET ansi?"%K[%R$0%n$1-%K]":"[$0$1-]"
#define DEFAULT_FORMAT_NAMES_USER_VOICE_FSET ansi?"%K[%M$0%n$1-%K]":"[$0$1-]"

#define DEFAULT_FORMAT_NETADD_FSET "$G %nAdded: %W$1 $2"
#define DEFAULT_FORMAT_NETJOIN_FSET "$G %nNetjoined: %W$1 $2"
#define DEFAULT_FORMAT_NETSPLIT_FSET "$G %nNetSplit: %W$1%n split from %W$2 %K[%c$0%K]"
#define DEFAULT_FORMAT_NICKNAME_FSET "$G %W$1 %nis now known as %c$3"
#define DEFAULT_FORMAT_NICKNAME_OTHER_FSET "$G %W$1 %nis now known as %c$3"
#define DEFAULT_FORMAT_NICKNAME_USER_FSET "%K>%n>%W> %WYou%K(%n$1%K)%n are now known as %c$3"
#define DEFAULT_FORMAT_NONICK_FSET "%W$1%K:%n $3-"


#define DEFAULT_FORMAT_NOTE_FSET "($0) ($1) ($2) ($3) ($4) ($5-)"


#define DEFAULT_FORMAT_NOTICE_FSET ansi?"%K-%P$1%K(%p$2%K)-%n $3-":"-$1($2)- $3-"

#define DEFAULT_FORMAT_REL_FSET "%K[%rmsg->$1%K]%n $4-"
#define DEFAULT_FORMAT_RELN_FSET "%K-%P$1%K(%p$2%K)-%n $4-"
#define DEFAULT_FORMAT_RELM_FSET "%K[%P%P$1%K(%p$2%K)]%n $4-"

#define DEFAULT_FORMAT_RELSN_FSET "%K[%rnotice%K(%R$1%K)] %n$2-"
#define DEFAULT_FORMAT_RELS_FSET "$1-"
#define DEFAULT_FORMAT_RELSM_FSET "%K[%rmsg%K(%R$1%K)] %n$2-"

#define DEFAULT_FORMAT_NOTIFY_SIGNOFF_FSET "$G %GSignoff%n by %r$1%K!%r$2%n at $0"
#define DEFAULT_FORMAT_NOTIFY_SIGNON_FSET "$G %GSignon%n by %R$1%K!%R$2%n at $0"
#define DEFAULT_FORMAT_PASTE_FSET "%K[%W$1%K]%n $2-"
#define DEFAULT_FORMAT_PUBLIC_FSET ansi?"%@%B<%n$1%B>%n $3-":"%@<$1> $3-"
#define DEFAULT_FORMAT_PUBLIC_AR_FSET ansi?"%@%B<%Y$1%B>%n $3-":"%@<$1> $3-"
#define DEFAULT_FORMAT_PUBLIC_MSG_FSET ansi?"%@%b(%n$1%K/%n$3%b)%n $4-":"%@($1/$3) $4-"
#define DEFAULT_FORMAT_PUBLIC_MSG_AR_FSET ansi?"%@%b(%Y$1%K/%Y$3%b)%n $4-":"%@($1/$3) $4-"

#define DEFAULT_FORMAT_PUBLIC_NOTICE_FSET ansi?"%@%K-%P$1%K:%p$3%K-%n $4-":"%@-$1:$3- $4-"
#define DEFAULT_FORMAT_PUBLIC_NOTICE_AR_FSET ansi?"%@%K-%Y$1%K:%Y$3%K-%n $4-":"%@-$1:$3- $4-"

#define DEFAULT_FORMAT_PUBLIC_OTHER_FSET ansi?"%@%b<%n$1%K:%n$2%b>%n $3-":"%@<$1:$2> $3-"
#define DEFAULT_FORMAT_PUBLIC_OTHER_AR_FSET ansi?"%@%b<%Y$1%K:%n$2%b>%n $3-":"%@<$1:$2> $3-"

#ifdef ONLY_STD_CHARS
#define DEFAULT_FORMAT_SEND_ACTION_FSET "%K* %W$1 %n$3-"
#define DEFAULT_FORMAT_SEND_ACTION_OTHER_FSET "%K* %n-> %W$1%n/%c$2 %n$3-"
#else
#define DEFAULT_FORMAT_SEND_ACTION_FSET "%Kð %W$1 %n$3-"
#define DEFAULT_FORMAT_SEND_ACTION_OTHER_FSET "%Kð %n-> %W$1%n/%c$2 %n$3-"
#endif

#define DEFAULT_FORMAT_SEND_AWAY_FSET "[Away ($strftime($1 %a %b %d %I:%M%p %Z))] [$tdiff2(${time() - u})] [BX-MsgLog $2]"
#define DEFAULT_FORMAT_SEND_CTCP_FSET ansi?"%K[%rctcp%K(%R$1%K)] %n$2":"[ctcp($1)] $2"
#define DEFAULT_FORMAT_SEND_RCTCP_FSET ansi?"%K[%rrctcp%K(%R$1%K)] %n$2-":"^B[rctcp(^B$1^B)]^B $2-"
#define DEFAULT_FORMAT_SEND_DCC_CHAT_FSET ansi?"%K[%rdcc%K(%R$1%K)] %n$2-":"[dcc($1)] $2-"
#define DEFAULT_FORMAT_SEND_ENCRYPTED_NOTICE_FSET "%K-%Y$1%K(%p$2%K)-%n $2-"
#define DEFAULT_FORMAT_SEND_ENCRYPTED_MSG_FSET "%K[%Y$1%K(%p$2%K)]%n $2-"

#define DEFAULT_FORMAT_SEND_MSG_FSET ansi?"%K[%rmsg%K(%R$1%K)] %n$3-":"[msg($1)] $3-"
#define DEFAULT_FORMAT_SEND_NOTICE_FSET ansi?"%K[%rnotice%K(%R$1%K)] %n$3-":"[notice($1)] $3-"
#define DEFAULT_FORMAT_SEND_PUBLIC_FSET ansi?"%P<%n$2%P>%n $3-":"<$2> $3-"
#define DEFAULT_FORMAT_SEND_PUBLIC_OTHER_FSET ansi?"%p<%n$2%K:%n$1%p>%n $3-":"<$2:$1> $3-"


#define DEFAULT_FORMAT_SERVER_FSET "$G%n $1: $2-"
#define DEFAULT_FORMAT_SERVER_MSG1_FSET "$G%n $1: $2-"
#define DEFAULT_FORMAT_SERVER_MSG1_FROM_FSET "$G%n $1: $2-"
#define DEFAULT_FORMAT_SERVER_MSG2_FSET "$G%n $1-"
#define DEFAULT_FORMAT_SERVER_MSG2_FROM_FSET "$G%n $1-"

#define DEFAULT_FORMAT_SERVER_NOTICE_FSET " $2-"
#define DEFAULT_FORMAT_SERVER_NOTICE_BOT_FSET " Possible bot: %C$1 %K[%c$2-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_BOT1_FSET " Possible $1 bot: %C$2 %K[%c$3-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_BOT_ALARM_FSET " $1 alarm activated: %C$2 %K[%c$3-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_CONNECT_FSET " Client Connecting: %C$1 %K[%c$2-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_EXIT_FSET " Client Exiting: %C$1 %K[%c$2-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_INVALID_FSET " Invalid username: %C$1 %K[%c$2-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_TERM_FSET " Terminating client for excess flood %C$1%K [%c$2-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_FAKE_FSET " Fake Mode detected on $1 -> $2-"
#define DEFAULT_FORMAT_SERVER_NOTICE_KILL_FSET " Foreign OperKill: %W$1%n killed %c$2%n %K(%n$3-%K)%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_KILL_LOCAL_FSET " Local OperKill: %W$1%n killed %c$2%n %K(%n$3-%K)%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_KLINE_FSET " %W$1%n added a new K-Line %K[%c$2%K]%n for $3-"
#define DEFAULT_FORMAT_SERVER_NOTICE_GLINE_FSET " %W$1%n added a new K-Line %K[%c$2%K]%n from $3 for $4-"
#define DEFAULT_FORMAT_SERVER_NOTICE_NICK_COLLISION_FSET " Nick collision %W$1%n killed %c$2%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_OPER_FSET " %C$1 %K[%c$2%K]%n is now %Wa%w %GIRC%n whore"
#define DEFAULT_FORMAT_SERVER_NOTICE_REHASH_FSET " %W$1%n is rehashing the Server config file"
#define DEFAULT_FORMAT_SERVER_NOTICE_STATS_FSET " Stats $1: %C$2 %K[%c$3-%K]%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_TRAFFIC_HIGH_FSET " Entering high-traffic mode %K(%n$1 > $2-%K)%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_TRAFFIC_NORM_FSET " Resuming standard operation %K(%n$1 $2 $3-%K)%n"
#define DEFAULT_FORMAT_SERVER_NOTICE_UNAUTH_FSET " Unauthorized Connection from $1-"

#define DEFAULT_FORMAT_SET_FSET "%g$[-30.]0 %w$1-"
#define DEFAULT_FORMAT_CSET_FSET "%r$[-14]1 %R$[-20.]0   %w$2-"
#define DEFAULT_FORMAT_SET_NOVALUE_FSET "%g$[-30.]0 has no value"
#define DEFAULT_FORMAT_SHITLIST_FSET " $[3]0 $[34]1 $[10]2  $3-"
#define DEFAULT_FORMAT_SHITLIST_FOOTER_FSET "There are $1 users on the shitlist"
#define DEFAULT_FORMAT_SHITLIST_HEADER_FSET " lvl nick!user@host                     channels   reason"
#define DEFAULT_FORMAT_SIGNOFF_FSET "$G %nSignOff: %W$1 %K(%n$3-%K)"


#define DEFAULT_FORMAT_SILENCE_FSET "$G %RWe are $1 silencing $2 at $0"

#define DEFAULT_FORMAT_TRACE_OPER_FSET "%R$1%n %K[%n$3%K]"
#define DEFAULT_FORMAT_TRACE_SERVER_FSET "%R$1%n $2 $3 $4 %K[%n$5%K]%n $6-"
#define DEFAULT_FORMAT_TRACE_USER_FSET "%R$1%n %K[%n$3%K]"

#define DEFAULT_FORMAT_TIMER_FSET "$G $[-5]0 $[-10]1 $[-6]2 $3-"

#define DEFAULT_FORMAT_TOPIC_FSET "$G Topic %K(%c$1%K):%n $2-"
#define DEFAULT_FORMAT_TOPIC_CHANGE_FSET "$G Topic %K(%c$2%K):%n changed by %c$1%K:%n $3-"
#define DEFAULT_FORMAT_TOPIC_SETBY_FSET "$G Topic %K(%c$1%K):%n set by %c$2%n at %c$stime($3)%n"
#define DEFAULT_FORMAT_TOPIC_UNSET_FSET "$G Topic %K(%c$2%K):%n unset by %c$1%n"
	
#define DEFAULT_FORMAT_USAGE_FSET "$G Usage: /$0  $1-"
#define DEFAULT_FORMAT_USERMODE_FSET "$G %nMode change %K[%W$4-%K]%n for user %c$3"
#define DEFAULT_FORMAT_USERMODE_OTHER_FSET "$G %nMode change %K[%W$4-%K]%n for user %c$3%n by %W$1"

#define DEFAULT_FORMAT_USERLIST_FSET "$[16]0 $[10]1 $[-10]2   $[-25]3 $[10]4"

#define DEFAULT_FORMAT_USERLIST_FOOTER_FSET "There are $1 users on the userlist"
#define DEFAULT_FORMAT_USERLIST_HEADER_FSET "level            nick       password     host                      channels"

#define DEFAULT_FORMAT_USERS_FSET "%K[%C$[1]5%B$[9]2%K][%n$[41]3%K][%n$[15]0%K](%n$4%K)"
#define DEFAULT_FORMAT_USERS_USER_FSET "%K[%C$[1]5%B$[9]2%K][%n%B$[41]3%K][%n$0%K](%n$4%K)"
#define DEFAULT_FORMAT_USERS_SHIT_FSET "%K[%C$[1]5%B$[9]2%K][%n%r$[41]3%K][%n$[-15]0%K](%n$4%K)"
#define DEFAULT_FORMAT_USERS_TITLE_FSET "$G Channel userlist for %W$1%n at ($0):"
#define DEFAULT_FORMAT_USERS_HEADER_FSET NULL /*"%K[ %WC%nhannel  %K][ %WN%wickname %K][%n %Wu%wser@host                       %K][%n %Wl%wevel         %K]"*/

#define DEFAULT_FORMAT_VERSION_FSET "\002$0+$4$5\002 by panasync \002-\002 $2 $3"


#define DEFAULT_FORMAT_WALL_FSET "%@%G!%g$1:$2%G!%n $3-"
#define DEFAULT_FORMAT_WALL_AR_FSET "%@%G!%Y$1:$2%G!%n $3-"


#define DEFAULT_FORMAT_WALLOP_FSET "%G!%g$1$2%G!%n $3-"
#define DEFAULT_FORMAT_WHO_FSET "%Y$[10]0 %W$[10]1%w %c$[3]2 %w$3%R@%w$4 ($6-)"

#ifdef ONLY_STD_CHARS
#define DEFAULT_FORMAT_WHOIS_AWAY_FSET ansi?"%K| %Wa%nway     : $0 - $1-":"| away     : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_BOT_FSET ansi?"%g| %Wb%not      : A:$0 P:$1 [$2] $3-":"| bot      : A:$0 P:$1 [$2] $3-"
#define DEFAULT_FORMAT_WHOIS_CHANNELS_FSET ansi?"%g| %Wc%nhannels : $0-":"| channels : $0-"
#define DEFAULT_FORMAT_WHOIS_FRIEND_FSET ansi?"%g| %Wf%nriend   : F:$0 $1-":"| friend   : F:$0 $1-"
#define DEFAULT_FORMAT_WHOIS_HEADER_FSET ansi?"%G.--------%g-%G--%g--%G-%g---------%K-%g--%K--%g-%K------------ --  -":".----------------------------------------- --  -"
#define DEFAULT_FORMAT_WHOIS_IDLE_FSET ansi?"%K: %Wi%ndle     : $0 hours $1 mins $2 secs (signon: $stime($3))":": idle     : $0 hours $1 mins $2 secs (signon: $stime($3))"
#define DEFAULT_FORMAT_WHOIS_SHIT_FSET ansi?"%g| %Ws%nhit     : L:$0 [$1] $2 $3-":"| shit     : L:$0 [$1] $2 $3-"
#define DEFAULT_FORMAT_WHOIS_SIGNON_FSET ansi?"%K| %Ws%nignon   : $0-":"| signon   : $0-"
#define DEFAULT_FORMAT_WHOIS_ACTUALLY_FSET ansi?"%K: %Wa%nctually : $0-":": actually : $0-"
#define DEFAULT_FORMAT_WHOIS_CALLERID_FSET ansi?"%K! %Wc%nallerid : $0-":"! callerid : $0-"
#define DEFAULT_FORMAT_WHOIS_SECURE_FSET ansi?"%K! %Ws%necure   : $0-":"! secure   : $0-"
#define DEFAULT_FORMAT_WHOIS_NAME_FSET ansi?"%G: %Wi%nrcname  : $0-":": ircname  : $0-"
#define DEFAULT_FORMAT_WHOIS_NICK_FSET ansi?"%G| %W$0 %K(%n$1@$2%K) %K(%W$3-%K)":"| $0 ($1@$2) ($3-)"
#define DEFAULT_FORMAT_WHOIS_OPER_FSET ansi?"%K| %Wo%nperator : $0 $1-":"| operator : $0 $1-"
#define DEFAULT_FORMAT_WHOIS_SERVER_FSET ansi?"%K| %Ws%nerver   : $0 ($1-)":"| server   : $0 ($1-)"
#define DEFAULT_FORMAT_WHOLEFT_HEADER_FSET ansi?"%G.----- %WWho %G-----%g---%G---%g--%G-----%g--%G-- %WChannel%g--- %wServer %G-%g----%G--%g--%G----%g %wSeconds":".----- Who ---------------------- Channel--- Server ------------- Seconds"
#define DEFAULT_FORMAT_WHOLEFT_USER_FSET ansi?"%G|%n $[-10]0!$[20]1 $[10]2 $[20]4 $3":"| $[-10]0!$[20]1 $[10]2 $[20]4 $3"
#define DEFAULT_FORMAT_WHOWAS_HEADER_FSET ansi?"%G.--------%g-%G--%g--%G-%g---------%K-%g--%K--%g-%K------------ --  -":".----------------------------------------- --  -"
#define DEFAULT_FORMAT_WHOWAS_NICK_FSET ansi?"%G| %W$0%n was %K(%n$1@$2%K)":"| $0 was ($1@$2)"
#define DEFAULT_FORMAT_WHOIS_ADMIN_FSET ansi?"%K| %Wa%ndmin    : $0 - $1-":"| admin    : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_SERVICE_FSET ansi?"%K| %Ws%nervice  : $0 - $1-":"| service  : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_HELP_FSET ansi?"%K| %Wh%nelp     : $0 - $1-":"| help     : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_REGISTER_FSET ansi?"%K| %Wr%negister : $0 - $1-":"| register : $0 - $1-"
#else
#define DEFAULT_FORMAT_WHOIS_AWAY_FSET ansi?"%K| %Wa%nway     : $0 - $1-":"| away     : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_BOT_FSET ansi?"%g| %Wb%not      : A:$0 P:$1 [$2] $3-":"| bot      : A:$0 P:$1 [$2] $3-"
#define DEFAULT_FORMAT_WHOIS_CHANNELS_FSET ansi?"%g| %Wc%nhannels : $0-":"| channels : $0-"
#define DEFAULT_FORMAT_WHOIS_FRIEND_FSET ansi?"%g| %Wf%nriend   : F:$0 $1-":"| friend   : F:$0 $1-"
#define DEFAULT_FORMAT_WHOIS_HEADER_FSET ansi?"%GÚÄÄÄÄÄ---%gÄ%G--%gÄÄ%G-%gÄÄÄÄÄÄ---%KÄ%g--%KÄÄ%g-%KÄÄÄÄÄÄÄÄÄ--- --  -":"ÚÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄÄÄÄ--- --  -"
#define DEFAULT_FORMAT_WHOIS_IDLE_FSET ansi?"%K: %Wi%ndle     : $0 hours $1 mins $2 secs (signon: $stime($3))":": idle     : $0 hours $1 mins $2 secs (signon: $stime($3))"
#define DEFAULT_FORMAT_WHOIS_SHIT_FSET ansi?"%g| %Ws%nhit     : L:$0 [$1] $2 $3-":"| shit     : L:$0 [$1] $2 $3-"
#define DEFAULT_FORMAT_WHOIS_SIGNON_FSET ansi?"%K %Ws%nignon   : $0-":" signon   : $0-"
#define DEFAULT_FORMAT_WHOIS_ACTUALLY_FSET ansi?"%K| %Wa%nctually : $0-":"| actually : $0-"
#define DEFAULT_FORMAT_WHOIS_CALLERID_FSET ansi?"%K! %Wc%nallerid : $0-":"! callerid : $0-"
#define DEFAULT_FORMAT_WHOIS_SECURE_FSET ansi?"%K! %Ws%necure   : $0-":"! secure   : $0-"
#define DEFAULT_FORMAT_WHOIS_NAME_FSET ansi?"%G³ %Wi%nrcname  : $0-":"³ ircname  : $0-"
#define DEFAULT_FORMAT_WHOIS_NICK_FSET ansi?"%G| %W$0 %K(%n$1@$2%K) %K(%W$3-%K)":"| $0 ($1@$2) ($3-)"
#define DEFAULT_FORMAT_WHOIS_OPER_FSET ansi?"%K| %Wo%nperator : $0 $1-":"| operator : $0 $1-"
#define DEFAULT_FORMAT_WHOIS_SERVER_FSET ansi?"%K³ %Ws%nerver   : $0 ($1-)":"³ server   : $0 ($1-)"
#define DEFAULT_FORMAT_WHOLEFT_HEADER_FSET ansi?"%GÚÄÄÄÄÄ %WWho %GÄÄÄÄÄ%g---%GÄÄÄ%g--%GÄÄÄÄÄ%gÄ-%GÄÄ %WChannel%gÄÄÄ %wServer %G-%gÄÄ--%GÄÄ%g--%GÄÄÄÄ%g %wSeconds":"ÚÄÄÄÄÄ Who ÄÄÄÄÄ---ÄÄÄ--ÄÄÄÄÄÄ-ÄÄ ChannelÄÄÄ Server -ÄÄ--ÄÄ--ÄÄÄÄ Seconds"
#define DEFAULT_FORMAT_WHOLEFT_USER_FSET ansi?"%G|%n $[-10]0!$[20]1 $[10]2 $[20]4 $3":"| $[-10]0!$[20]1 $[10]2 $[20]4 $3"
#define DEFAULT_FORMAT_WHOWAS_HEADER_FSET ansi?"%GÚÄÄÄÄÄ---%gÄ%G--%gÄÄ%G-%gÄÄÄÄÄÄ---%KÄ%g--%KÄÄ%g-%KÄÄÄÄÄÄÄÄÄ--- --  -":"ÚÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄ---Ä--ÄÄ-ÄÄÄÄÄÄÄÄÄ--- --  -"
#define DEFAULT_FORMAT_WHOWAS_NICK_FSET ansi?"%G| %W$0%n was %K(%n$1@$2%K)":"| $0 was ($1@$2)"
#define DEFAULT_FORMAT_WHOIS_ADMIN_FSET ansi?"%K| %Wa%ndmin    : $0 - $1-":"| admin    : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_SERVICE_FSET ansi?"%K| %Ws%nervice  : $0 - $1-":"| service  : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_HELP_FSET ansi?"%K| %Wh%nelp     : $0 - $1-":"| help     : $0 - $1-"
#define DEFAULT_FORMAT_WHOIS_REGISTER_FSET ansi?"%K| %Wr%negister : $0 - $1-":"| register : $0 - $1-"
#endif

#define DEFAULT_FORMAT_WIDELIST_FSET "$1-"
#define DEFAULT_FORMAT_WINDOW_SET_FSET "$0-"
	
#define DEFAULT_FORMAT_NICK_MSG_FSET "$0 $1 $2-"

#define DEFAULT_FORMAT_NICK_COMP_FSET "$0\002:\002$1-"
#define DEFAULT_FORMAT_NICK_AUTO_FSET "$0\002:\002$1-"

#define DEFAULT_FORMAT_STATUS_FSET "%4%W$0-"
#define DEFAULT_FORMAT_STATUS1_FSET "%4%W$0-"
#define DEFAULT_FORMAT_STATUS2_FSET "%4%W$0-"
#define DEFAULT_FORMAT_STATUS3_FSET "%4%W$0-"
#define DEFAULT_FORMAT_NOTIFY_OFF_FSET "$[10]0 $[35]1 $[-6]2 $[-10]3 $4 $5"
#define DEFAULT_FORMAT_NOTIFY_ON_FSET "$[10]0 $[35]1 $[-6]2 $[-10]3 $4-"

#define DEFAULT_FORMAT_OV_FSET "[41m%S %>[1;30m[[1;32mOper[1;37mView[1;30;41m] "
#define DEFAULT_FORMAT_DEBUG_FSET "[45;1mDEBUG"

#define DEFAULT_FORMAT_WHOIS_FOOTER_FSET NULL
#define DEFAULT_FORMAT_XTERM_TITLE_FSET NULL
#define DEFAULT_FORMAT_DCC_FSET NULL
#define DEFAULT_FORMAT_NAMES_FOOTER_FSET NULL
#define DEFAULT_FORMAT_NETSPLIT_HEADER_FSET NULL
#define DEFAULT_FORMAT_TOPIC_CHANGE_HEADER_FSET NULL
#define DEFAULT_FORMAT_WHOLEFT_FOOTER_FSET NULL

#define DEFAULT_STATUS_FORMAT " [0;44;36m[[1;37m%T[0;44;36m][%*%@%=[0;44;37m%N%#%A[0;44;36m]%M [0;44;36m[%C%+%W[0;44;36m] %Q %H%B%F "
#define DEFAULT_STATUS_FORMAT1 " [0;44;36m[[1;37m%T[0;44;36m][%*[1;37m%@%G%=[0;44;37m%N%#%A[0;44;36m]%M [0;44;36m[%C%+%W[0;44;36m] %Q %H%B%F "
#define DEFAULT_STATUS_FORMAT2 " %L %! %K %>%D %J[%u:%a:%f:%b:%h]"
#define DEFAULT_STATUS_FORMAT3 "BitchX by panasync "

#define DEFAULT_STATUS_AWAY " [0;44;36m([1;32mzZzZ[1;37m %A[0;44;36m)[0;44;37m"
#define DEFAULT_STATUS_CHANNEL "[0;44;37m%C"
#define DEFAULT_STATUS_CHANOP "@"
#define DEFAULT_STATUS_HALFOP "%"
#define DEFAULT_STATUS_CLOCK "%T"
#define DEFAULT_STATUS_HOLD " -- more --"
#define DEFAULT_STATUS_HOLD_LINES " [0;44;36m([1;37m%B[0;44;36m)[0;44;37m"
#define DEFAULT_STATUS_INSERT ""
#define DEFAULT_STATUS_LAG "[0;44;36m[[0;44;37mLag [1;37m%L[0;44;36m]"
#define DEFAULT_STATUS_MODE "[1;37m([0;44;36m+[0;44;37m%+[1;37m)"
#define DEFAULT_STATUS_MAIL "[0;44;36m[[0;44;37mMail: [1;37m%M[0;44;36m]"
#define DEFAULT_STATUS_MSGCOUNT " Aw[0;44;36m[[1;37m%^[0;44;36m]"
#define DEFAULT_STATUS_NICK "%N"
#define DEFAULT_STATUS_NOTIFY " [0;44;36m[[37mAct: [1;37m%F[0;44;36m]"
#define DEFAULT_STATUS_OPER "[1;31m*[0;44;37m"
#define DEFAULT_STATUS_VOICE "[1;32m+[0;44;37m"
#define DEFAULT_STATUS_OVERWRITE "(overtype) "
#define DEFAULT_STATUS_QUERY " [0;44;36m[[37mQuery: [1;37m%Q[0;44;36m]"
#define DEFAULT_STATUS_SERVER " via %S"
#define DEFAULT_STATUS_TOPIC "%-"
#define DEFAULT_STATUS_UMODE "[1;37m([0;44;36m+[37m%#[1;37m)"
#define DEFAULT_STATUS_USER " * type /help for help "

#define DEFAULT_STATUS_DCCCOUNT  "[DCC  gets/%& sends/%&]"
#define DEFAULT_STATUS_CDCCCOUNT "[CDCC gets/%| offer/%|]"

#define DEFAULT_STATUS_OPER_KILLS "[0;44;36m[[37mnk[36m/[1;37m%K [0;44mok[36m/[1;37m%K[0;44;36m]"
#define DEFAULT_STATUS_USERS "[0;44;36m[[37mO[36m/[1;37m%! [0;44mN[36m/[1;37m%! [0;44mI[36m/[1;37m%! [0;44mV[36m/[1;37m%! [0;44mF[36m/[1;37m%![0;44;36m]"
#define DEFAULT_STATUS_CPU_SAVER " (%J)"

#define DEFAULT_STATUS_USER1 ""
#define DEFAULT_STATUS_USER2 ""
#define DEFAULT_STATUS_USER3 ""
#define DEFAULT_STATUS_USER4 ""
#define DEFAULT_STATUS_USER5 ""
#define DEFAULT_STATUS_USER6 ""
#define DEFAULT_STATUS_USER7 ""
#define DEFAULT_STATUS_USER8 ""
#define DEFAULT_STATUS_USER9 ""
#define DEFAULT_STATUS_USER10 ""
#define DEFAULT_STATUS_USER11 ""
#define DEFAULT_STATUS_USER12 ""
#define DEFAULT_STATUS_USER13 ""
#define DEFAULT_STATUS_USER14 ""
#define DEFAULT_STATUS_USER15 ""
#define DEFAULT_STATUS_USER16 ""
#define DEFAULT_STATUS_USER17 ""
#define DEFAULT_STATUS_USER18 ""
#define DEFAULT_STATUS_USER19 ""
#define DEFAULT_STATUS_USER20 ""
#define DEFAULT_STATUS_USER21 ""
#define DEFAULT_STATUS_USER22 ""
#define DEFAULT_STATUS_USER23 ""
#define DEFAULT_STATUS_USER24 ""
#define DEFAULT_STATUS_USER25 ""
#define DEFAULT_STATUS_USER26 ""
#define DEFAULT_STATUS_USER27 ""
#define DEFAULT_STATUS_USER28 ""
#define DEFAULT_STATUS_USER29 ""
#define DEFAULT_STATUS_USER30 ""
#define DEFAULT_STATUS_USER31 ""
#define DEFAULT_STATUS_USER32 ""
#define DEFAULT_STATUS_USER33 ""
#define DEFAULT_STATUS_USER34 ""
#define DEFAULT_STATUS_USER35 ""
#define DEFAULT_STATUS_USER36 ""
#define DEFAULT_STATUS_USER37 ""
#define DEFAULT_STATUS_USER38 ""
#define DEFAULT_STATUS_USER39 ""
#define DEFAULT_STATUS_WINDOW "[1;44;33m^^^^^^^^[0;44;37m"
#define DEFAULT_STATUS_FLAG ""
#define DEFAULT_STATUS_SCROLLBACK " (Scroll)"

#define DEFAULT_FORMAT_LASTLOG_FSET "$strftime($0 %H:%M) $1-"

#define DEFAULT_INPUT_PROMPT "[$C] "

#ifndef ONLY_STD_CHARS
#define DEFAULT_SHOW_NUMERICS_STR "[1;31mù[0m[1;37mí[1;31mù[0m"
#else
#ifndef LATIN1
#define DEFAULT_SHOW_NUMERICS_STR "***"
#else
#define DEFAULT_SHOW_NUMERICS_STR "[1;31m-[0m[1;37m:[1;31m-[0m"
#endif
#endif

#define DEFAULT_SERVER_PROMPT "[0m[1;32m[$0]"

#define DEFAULT_FORMAT_WATCH_SIGNOFF_FSET "$G $0!$1@$2 has signed off"
#define DEFAULT_FORMAT_WATCH_SIGNON_FSET "$G $0!$1@$2 has signed on $3"

#endif

#endif
