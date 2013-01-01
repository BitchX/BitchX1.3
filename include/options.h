
static const char compile_time_options[] = {
 
					'a',
#ifdef NO_BOTS
 					'b',
#endif /* NO_BOTS */
 
#ifdef BITCHX_DEBUG
 					'd',
#endif /* BITCHX_DEBUG */
 
#ifdef EXEC_COMMAND
 					'e',
#endif /* EXEC_COMMAND */
 
#ifdef INCLUDE_GLOB_FUNCTION
 					'g',
#endif /* INCLUDE_GLOB_FUNCTION */
#ifdef WANT_HEBREW
					'h',
#endif
 
#ifdef MIRC_BROKEN_DCC_RESUME
					'i',
#endif /* MIRC_BROKEN_DCC_RESUME */

#ifdef HACKED_DCC_WARNING
					'k',
#endif /* HACKED_DCC_WARNING */

#ifdef WANT_DLL
					'l',
#endif

#ifdef STRIP_EXTRANEOUS_SPACES
					's',
#endif /* STRIP_EXTRANEOUS_SPACES */

#ifdef WANT_TCL
					't',
#endif

#ifdef UNAME_HACK
 					'u',
#endif /* UNAME_HACK */
 
#ifdef ALLOW_STOP_IRC
					'z',
#endif /* ALLOW_STOP_IRC */

					'\0'
};
