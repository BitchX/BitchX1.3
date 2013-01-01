
#include "irc.h"
static char cvsrevision[] = "$Id: tcl_public.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(tcl_public_c)
#include "ircaux.h"
#include "struct.h"
#include "commands.h"
#include "screen.h"
#include "server.h"
#include "tcl_bx.h"
#include "misc.h"
#include "userlist.h"
#include "output.h"
#include "log.h"
#include "dcc.h"
#include "timer.h"
#define MAIN_SOURCE
#include "modval.h"

cmd_t C_dcc[] =
{
	{ "act",	cmd_act,	 ADD_DCC, "Perform action on a channel"},
	{ "adduser",	cmd_adduser,	 ADD_DCC, "add a user to the userlist" },
	{ "boot",	cmd_boot,	 ADD_BAN, "boot user off the botnet" },
	{ "chat",	cmd_chat,	 0,	  "add you to the chat network" },
	{ "cmsg",	cmd_cmsg,	 0,	  "send a privmsg to someone on botnEt" },
	{ "echo",	cmd_echo,	 0,	  "turn echo on/off" },
	{ "help",	cmd_help,	 0,	  "help information [option cmd]" },
	{ "invite",	cmd_invite,	 ADD_INVITE,"invite <nick> to the chat network" },
	{ "irc",	cmd_ircii,	 ADD_TCL, "pass ircii commands to client" },
	{ "msg",	cmd_msg,	 ADD_DCC,"send msg to someone" },
	{ "op",		cmd_ops,	ADD_OPS, "ops on a channel" },
	{ "quit",	cmd_quit,	 0,	  "remove from chat network" },
	{ "say",	cmd_say,	 ADD_DCC, "say something on a channel" },
	{ "tcl",	cmd_tcl,	 ADD_TCL, "set a tcl variable" },
	{ "who",	send_who,	 0,	  "find out who is on [option botnick]" },
	{ "whoami",	cmd_whoami,	 0,	  "determines your userlevel" },
	{ "whom",	send_whom,	 0,	  "find out who is on the botnet. global" },
	{ "xlink",	send_command,	 ADD_DCC, "send command to all on link" },
	{ NULL,		NULL,		-1,	NULL} 
};

#ifdef WANT_TCL
#include <tcl.h>

/* 
 * I wish to thank vore!vore@domination.ml.org for pushing me 
 * todo something like this, although by-Tor requested 
 * something like this as well but not so succintly
 */


int tcl_bots STDVAR;
int tcl_ircii STDVAR;
int tcl_validuser STDVAR;
int tcl_pushmode STDVAR;
int tcl_flushmode STDVAR;
int Tcl_LvarpopCmd STDVAR;
int Tcl_LemptyCmd STDVAR;
int Tcl_LmatchCmd STDVAR;
int Tcl_KeyldelCmd STDVAR;
int Tcl_KeylgetCmd STDVAR;
int Tcl_KeylkeysCmd STDVAR;
int Tcl_KeylsetCmd STDVAR;
int tcl_maskhost STDVAR;
int tcl_onchansplit STDVAR;
int tcl_servers STDVAR;
int tcl_chanstruct  STDVAR;
int tcl_channel STDVAR;
int tcl_channels STDVAR;
int tcl_isop  STDVAR;
int tcl_getchanhost  STDVAR;
int matchattr  STDVAR;
int tcl_finduser  STDVAR;
int tcl_findshit  STDVAR;
int tcl_date  STDVAR;
int tcl_getcomment  STDVAR;
int tcl_setcomment  STDVAR;
int tcl_time  STDVAR;
int tcl_ctime  STDVAR;
int tcl_onchan  STDVAR;
int tcl_chanlist STDVAR;
int tcl_unixtime  STDVAR;
int tcl_putlog STDVAR;
int tcl_putloglev  STDVAR;
int tcl_rand   STDVAR;
int tcl_timer  STDVAR;
int tcl_killtimer  STDVAR;
int tcl_utimer   STDVAR;
int tcl_killutimer  STDVAR;
int tcl_timers  STDVAR;
int tcl_utimers  STDVAR;
int tcl_putserv  STDVAR;
int tcl_putscr  STDVAR;
int tcl_putdcc  STDVAR;
int tcl_putbot  STDVAR;
int tcl_putallbots  STDVAR;
int tcl_bind  STDVAR;
int tcl_tellbinds  STDVAR;
int tcl_bind  STDVAR;
int tcl_strftime  STDVAR;
int tcl_cparse  STDVAR;
int tcl_userhost  STDVAR;
int tcl_getchanmode  STDVAR;
int tcl_msg  STDVAR;
int tcl_say  STDVAR;
int tcl_desc   STDVAR;
int tcl_notice  STDVAR;
int tcl_bots  STDVAR;
int tcl_clients  STDVAR;
int tcl_alias  STDVAR;
int tcl_get_var  STDVAR;
int tcl_set_var  STDVAR;
int tcl_fget_var  STDVAR;
int tcl_fset_var  STDVAR;
int tcl_aliasvar STDVAR;
int tcl_cset STDVAR;
int tcl_dcc_stat STDVAR;
int tcl_dcc_close STDVAR;

extern void add_tcl_alias (Tcl_Interp *, void *, void *);

extern TimerList *tcl_Pending_timers;
extern TimerList *tcl_Pending_utimers;
static unsigned int timer_id = 1;


int 			msg_die (int, char *);

cmd_t C_msg[] =
{
/*	{ "die",	msg_die,	 ADD_KILL, "kill a client. Needs Userlevel KILL" },*/
	{ NULL,		NULL,		-1,	NULL}
};

cmd_t C_ctcp[] = 
{
	{ NULL,		NULL,		-1,	NULL}
};

cmd_t C_notice[] = 
{
	{ NULL,		NULL,		-1,	NULL}
};

/*
 * tclX has some keyed list functions which are useful.
 * But because tclX is not on every system, we cut and paste them
 * here for convenience sake.
 */
 
#define STREQU(str1, str2) \
        (((str1) [0] == (str2) [0]) && (strcmp (str1, str2) == 0))
#define STRNEQU(str1, str2, cnt) \
        (((str1) [0] == (str2) [0]) && (strncmp (str1, str2, cnt) == 0))
#define ISSPACE(c) (isspace ((unsigned char) c))
#define ISDIGIT(c) (isdigit ((unsigned char) c))
#define ISLOWER(c) (islower ((unsigned char) c))

extern int Tcl_KeyldelCmd (ClientData, Tcl_Interp*, int, char**);

extern	int Tcl_KeylgetCmd (ClientData, Tcl_Interp *, int, char**);

extern	int Tcl_KeylkeysCmd (ClientData, Tcl_Interp *, int, char**);

extern	int Tcl_KeylsetCmd (ClientData, Tcl_Interp *, int, char **);
extern	int TclFindElement (Tcl_Interp *, char *, char **, char **, int *, int *);
extern	int Tcl_GetKeyedListField (Tcl_Interp *, char *, char *, char **);
extern	int TclCopyAndCollapse (int, char *, char *);

char *tclXWrongArgs = "wrong # args: ";

typedef struct fieldInfo_t 
{
        int    argc;
        char **argv;
        int    foundIdx;
        char  *valuePtr;
        int    valueSize;
} fieldInfo_t;


static int CompareKeyListField (Tcl_Interp   *,
                                 char   *,
                                 char   *,
                                 char        **,
                                 int          *,
                                 int          *);

static int SplitAndFindField (Tcl_Interp  *,
                               char  *,
                               char  *,
                               fieldInfo_t *);

extern int Tcl_LmatchCmd (ClientData, Tcl_Interp*, int, char**);


/* BEGIN KEYED LIST */


/*
 *-----------------------------------------------------------------------------
 *
 * CompareKeyListField --
 *   Compare a field name to a field (keyword/value pair) to determine if
 * the field names match.
 *
 * Parameters:
 *   o interp (I/O) - Error message will be return in result if there is an
 *     error.
 *   o fieldName (I) - Field name to compare against field.
 *   o field (I) - Field to see if its name matches.
 *   o valuePtr (O) - If the field names match, a pointer to value part is
 *     returned.
 *   o valueSizePtr (O) - If the field names match, the length of the value
 *     part is returned here.
 *   o bracedPtr (O) - If the field names match, non-zero/zero to inficate
 *     that the value was/warn't in braces.
 * Returns:
 *    TCL_OK - If the field names match.
 *    TCL_BREAK - If the fields names don't match.
 *    TCL_ERROR -  If the list has an invalid format.
 *-----------------------------------------------------------------------------
 */
static int CompareKeyListField (tcl_interp, fieldName, field, valuePtr, valueSizePtr, bracedPtr)
    Tcl_Interp   *tcl_interp;
    char   *fieldName;
    char   *field;
    char        **valuePtr;
    int          *valueSizePtr; 
    int          *bracedPtr;
{
    char *elementPtr, *nextPtr;
    int   fieldNameSize, elementSize;

    if (field [0] == '\0') {
        tcl_interp->result =
            "invalid keyed list format: list contains an empty field entry";
        return TCL_ERROR;
    }
    if (TclFindElement (tcl_interp, (char *) field, &elementPtr, &nextPtr, 
                        &elementSize, NULL) != TCL_OK)
        return TCL_ERROR;
    if (elementSize == 0) {
        tcl_interp->result =
            "invalid keyed list format: list contains an empty field name";
        return TCL_ERROR;
    }
    if (nextPtr[0] == '\0') {
        Tcl_AppendResult (tcl_interp, "invalid keyed list format or inconsistent ",
                          "field name scoping: no value associated with ",
                          "field \"", elementPtr, "\"", (char *) NULL);
        return TCL_ERROR;
    }

    fieldNameSize = strlen ((char *) fieldName);
    if (!((elementSize == fieldNameSize) && 
            STRNEQU (elementPtr, ((char *) fieldName), fieldNameSize)))
        return TCL_BREAK;   /* Names do not match */

    /*
     * Extract the value from the list.
     */
    if (TclFindElement (tcl_interp, nextPtr, &elementPtr, &nextPtr, &elementSize, 
                        bracedPtr) != TCL_OK)
        return TCL_ERROR;
    if (nextPtr[0] != '\0') {
        Tcl_AppendResult (tcl_interp, "invalid keyed list format: ",
                          "trailing data following value in field: \"",
                          elementPtr, "\"", (char *) NULL);
        return TCL_ERROR;
    }
    *valuePtr = elementPtr;
    *valueSizePtr = elementSize;
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SplitAndFindField --
 *   Split a keyed list into an argv and locate a field (key/value pair)
 * in the list.
 *
 * Parameters:
 *   o tcl_interp (I/O) - Error message will be return in result if there is an
 *     error.
 *   o fieldName (I) - The name of the field to find.  Will validate that the
 *     name is not empty.  If the name has a sub-name (seperated by "."),
 *     search for the top level name.
 *   o fieldInfoPtr (O) - The following fields are filled in:
 *       o argc - The number of elements in the keyed list.
 *       o argv - The keyed list argv is returned here, even if the key was
 *         not found.  Client must free.  Will be NULL is an error occurs.
 *       o foundIdx - The argv index containing the list entry that matches
 *         the field name, or -1 if the key was not found.
 *       o valuePtr - Pointer to the value part of the found element. NULL
 *         in not found.
 *       o valueSize - The size of the value part.
 * Returns:
 *   Standard Tcl result.
 *-----------------------------------------------------------------------------
 */
static int SplitAndFindField (tcl_interp, fieldName, keyedList, fieldInfoPtr)
    Tcl_Interp  *tcl_interp;
    char  *fieldName;
    char  *keyedList;
    fieldInfo_t *fieldInfoPtr;
{
    int  idx, result, braced;

    if (fieldName == '\0') {
        tcl_interp->result = "null key not allowed";
        return TCL_ERROR;
    }

    fieldInfoPtr->argv = NULL;

    if (Tcl_SplitList (tcl_interp, (char *) keyedList, &fieldInfoPtr->argc,
                       &fieldInfoPtr->argv) != TCL_OK)
        goto errorExit;

    result = TCL_BREAK;
    for (idx = 0; idx < fieldInfoPtr->argc; idx++) {
        result = CompareKeyListField (tcl_interp, fieldName, 
                                      fieldInfoPtr->argv [idx],
                                      &fieldInfoPtr->valuePtr,
                                      &fieldInfoPtr->valueSize,
                                      &braced);
        if (result != TCL_BREAK)
            break;  /* Found or error, exit before idx is incremented. */
    }
    if (result == TCL_ERROR)
        goto errorExit;

    if (result == TCL_BREAK) {
        fieldInfoPtr->foundIdx = -1;  /* Not found */
        fieldInfoPtr->valuePtr = NULL;
    } else {
        fieldInfoPtr->foundIdx = idx;
    }
    return TCL_OK;

errorExit:
    if (fieldInfoPtr->argv != NULL)
        ckfree ((char *) fieldInfoPtr->argv);
    fieldInfoPtr->argv = NULL;
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetKeyedListKeys --
 *   Retrieve a list of keys from a keyed list.  The list is walked rather
 * than converted to a argv for increased performance.
 *
 * Parameters:
 *   o tcl_interp (I/O) - Error message will be return in result if there is an
 *     error.
 *   o subFieldName (I) - If "" or NULL, then the keys are retreved for
 *     the top level of the list.  If specified, it is name of the field who's
 *     subfield keys are to be retrieve.
 *   o keyedList (I) - The list to search for the field.
 *   o keysArgcPtr (O) - The number of keys in the keyed list is returned
 *     here.
 *   o keysArgvPtr (O) - An argv containing the key names.  It is dynamically
 *     allocated, containing both the array and the strings. A single call
 *     to ckfree will release it.
 * Returns:
 *   TCL_OK - If the field was found.
 *   TCL_BREAK - If the field was not found.
 *   TCL_ERROR - If an error occured.
 *-----------------------------------------------------------------------------
 */
int
Tcl_GetKeyedListKeys (tcl_interp, subFieldName, keyedList, keysArgcPtr,
                      keysArgvPtr)
    Tcl_Interp  *tcl_interp;
    char  *subFieldName;
    char  *keyedList;
    int         *keysArgcPtr;
    char      ***keysArgvPtr;
{
    char  *scanPtr, *subFieldList;
    int    result, keyCount, totalKeySize, idx;
    char  *fieldPtr, *keyPtr, *nextByte, *dummyPtr;
    int    fieldSize,  keySize;
    char **keyArgv;

    /*
     * Skip leading white spaces in list.  This keeps totally empty lists
     * with some white-spaces from being confused with empty field entries
     * later on in the parsing.
     */
    for (; *keyedList != '\0'; keyedList++) {
        if (ISSPACE (*keyedList) == 0)
            break;
    }

    /*
     * If the keys of a subfield are requested, the dig out that field's
     * list and then rummage through it getting the keys.
     */
    subFieldList = NULL;

    if ((subFieldName != NULL) && (subFieldName [0] != '\0')) {
        result = Tcl_GetKeyedListField (tcl_interp, subFieldName, keyedList,
                                        &subFieldList);
        if (result != TCL_OK)
            return result;
        keyedList = subFieldList;
    }

    /*
     * Walk the list count the number of field names and their length.
     */
    keyCount = 0;
    totalKeySize = 0;    
    scanPtr = (char *) keyedList;

    while (*scanPtr != '\0') {
        result = TclFindElement (tcl_interp, scanPtr, &fieldPtr, &scanPtr, 
                                 &fieldSize, NULL);
        if (result != TCL_OK)
            goto errorExit;
        result = TclFindElement (tcl_interp, fieldPtr, &keyPtr, &dummyPtr,
                                 &keySize, NULL);
        if (result != TCL_OK)
            goto errorExit;

        keyCount++;
        totalKeySize += keySize + 1;
    }

    /*
     * Allocate a structure to hold both the argv and strings.
     */
    keyArgv = (char **) ckalloc (((keyCount + 1) * sizeof (char *)) +
                                 totalKeySize);
    keyArgv [keyCount] = NULL;
    nextByte = ((char *) keyArgv) + ((keyCount + 1) * sizeof (char *));

    /*
     * Walk the list once more, copying in the strings and building up the
     * argv.
     */
    scanPtr = (char *) keyedList;
    idx = 0;

    while (*scanPtr != '\0') {
        TclFindElement (tcl_interp, scanPtr, &fieldPtr, &scanPtr, &fieldSize,
                        NULL);
        TclFindElement (tcl_interp, fieldPtr, &keyPtr, &dummyPtr, &keySize, NULL);
        keyArgv [idx++] = nextByte;
        strmcpy (nextByte, keyPtr, keySize);
        nextByte [keySize] = '\0';
        nextByte += keySize + 1; 
    }
    *keysArgcPtr = keyCount;
    *keysArgvPtr = keyArgv;
    
    if (subFieldList != NULL)
        ckfree (subFieldList);
    return TCL_OK;

  errorExit:
    if (subFieldList != NULL)
        ckfree (subFieldList);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_GetKeyedListField --
 *   Retrieve a field value from a keyed list.  The list is walked rather than
 * converted to a argv for increased performance.  This if the name contains
 * sub-fields, this function recursive.
 *
 * Parameters:
 *   o tcl_interp (I/O) - Error message will be return in result if there is an
 *     error.
 *   o fieldName (I) - The name of the field to extract.  Will recusively
 *     process sub-field names seperated by `.'.
 *   o keyedList (I) - The list to search for the field.
 *   o fieldValuePtr (O) - If the field is found, a pointer to a dynamicly
 *     allocated string containing the value is returned here.  If NULL is
 *     specified, then only the presence of the field is validated, the
 *     value is not returned.
 * Returns:
 *   TCL_OK - If the field was found.
 *   TCL_BREAK - If the field was not found.
 *   TCL_ERROR - If an error occured.
 *-----------------------------------------------------------------------------
 */
int Tcl_GetKeyedListField (tcl_interp, fieldName, keyedList, fieldValuePtr)
    Tcl_Interp  *tcl_interp;
    char  *fieldName;
    char  *keyedList;
    char       **fieldValuePtr;
{
    char *nameSeparPtr, *scanPtr, *valuePtr;
    int   valueSize, result, braced;

	if (fieldName == '\0') 
	{
		tcl_interp->result = "null key not allowed";
		return TCL_ERROR;
	}

    /*
     * Skip leading white spaces in list.  This keeps totally empty lists
     * with some white-spaces from being confused with empty field entries
     * later on in the parsing.
     */
	for (; *keyedList != 0; keyedList++)
		if (ISSPACE (*keyedList) == 0)
			break;

    /*
     * Check for sub-names, temporarly delimit the top name with a '\0'.
     */
	nameSeparPtr = strchr ((char *) fieldName, '.');
	if (nameSeparPtr != NULL)
		*nameSeparPtr = 0;

    /*
     * Walk the list looking for a field name that matches.
     */
	scanPtr = (char *) keyedList;
	result = TCL_BREAK;   /* Assume not found */

	while (*scanPtr != '\0') 
	{
		char *fieldPtr;
		int   fieldSize;
		char  saveChar = 0;

		result = TclFindElement (tcl_interp, scanPtr, &fieldPtr, &scanPtr, 
			&fieldSize, NULL);
		if (result != TCL_OK)
			break;

		saveChar = fieldPtr [fieldSize];
		fieldPtr [fieldSize] = 0;

		result = CompareKeyListField (tcl_interp, (char *) fieldName, fieldPtr,
			&valuePtr, &valueSize, &braced);
		fieldPtr [fieldSize] = saveChar;
		if (result != TCL_BREAK)
			break;  /* Found or an error */
	}

	if (result != TCL_OK)
		goto exitPoint;   /* Not found or an error */

    /*
     * If a subfield is requested, recurse to get the value otherwise allocate
     * a buffer to hold the value.
     */
	if (nameSeparPtr != NULL) 
	{
		char  saveChar = 0;

		saveChar = valuePtr [valueSize];
		valuePtr [valueSize] = 0;
		result = Tcl_GetKeyedListField (tcl_interp, nameSeparPtr+1, valuePtr, 
                                        fieldValuePtr);
		valuePtr [valueSize] = saveChar;
	} 
	else 
	{
		if (fieldValuePtr != NULL) 
		{
			char *fieldValue;

			fieldValue = ckalloc (valueSize + 1);
			if (braced) 
			{
	        		strmcpy (fieldValue, valuePtr, valueSize);
				fieldValue [valueSize] = 0;
			}
			else
				TclCopyAndCollapse(valueSize, valuePtr, fieldValue);
			*fieldValuePtr = fieldValue;
		}
	}
exitPoint:
	if (nameSeparPtr != NULL)
		*nameSeparPtr = '.';
	return result;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_SetKeyedListField --
 *   Set a field value in keyed list.
 *
 * Parameters:
 *   o tcl_interp (I/O) - Error message will be return in result if there is an
 *     error.
 *   o fieldName (I) - The name of the field to extract.  Will recusively
 *     process sub-field names seperated by `.'.
 *   o fieldValue (I) - The value to set for the field.
 *   o keyedList (I) - The keyed list to set a field value in, may be an
 *     NULL or an empty list to create a new keyed list.
 * Returns:
 *   A pointer to a dynamically allocated string, or NULL if an error
 *   occured.
 *-----------------------------------------------------------------------------
 */
char * Tcl_SetKeyedListField (tcl_interp, fieldName, fieldValue, keyedList)
    Tcl_Interp  *tcl_interp;
    char  *fieldName;
    char  *fieldValue;
    char  *keyedList;
{
    char        *nameSeparPtr;
    char        *newField = NULL, *newList;
    fieldInfo_t  fieldInfo;
    char        *elemArgv [2];

    if (fieldName [0] == '\0') {
        Tcl_AppendResult (tcl_interp, "empty field name", (char *) NULL);
        return NULL;
    }

    if (keyedList == NULL)
        keyedList = empty_string;

    /*
     * Check for sub-names, temporarly delimit the top name with a '\0'.
     */
    nameSeparPtr = strchr ((char *) fieldName, '.');
    if (nameSeparPtr != NULL)
        *nameSeparPtr = 0;

    if (SplitAndFindField (tcl_interp, fieldName, keyedList, &fieldInfo) != TCL_OK)
        goto errorExit;

    /*
     * Either recursively retrieve build the field value or just use the
     * supplied value.
     */
    elemArgv [0] = (char *) fieldName;
    if (nameSeparPtr != NULL) {
        char saveChar = 0;

        if (fieldInfo.valuePtr != NULL) {
            saveChar = fieldInfo.valuePtr [fieldInfo.valueSize];
            fieldInfo.valuePtr [fieldInfo.valueSize] = '\0';
        }
        elemArgv [1] = Tcl_SetKeyedListField (tcl_interp, nameSeparPtr+1,
                                              fieldValue, fieldInfo.valuePtr);

        if (fieldInfo.valuePtr != NULL)
            fieldInfo.valuePtr [fieldInfo.valueSize] = saveChar;
        if (elemArgv [1] == NULL)
            goto errorExit;
        newField = Tcl_Merge (2, elemArgv);
        ckfree (elemArgv [1]);
    } else {
        elemArgv [1] = (char *) fieldValue;
        newField = Tcl_Merge (2, elemArgv);
    }

    /*
     * If the field does not current exist in the keyed list, append it,
     * otherwise replace it.
     */
    if (fieldInfo.foundIdx == -1) {
        fieldInfo.foundIdx = fieldInfo.argc;
        fieldInfo.argc++;
    }

    fieldInfo.argv [fieldInfo.foundIdx] = newField;
    newList = Tcl_Merge (fieldInfo.argc, fieldInfo.argv);

    if (nameSeparPtr != NULL)
         *nameSeparPtr = '.';
    ckfree ((char *) newField);
    ckfree ((char *) fieldInfo.argv);
    return newList;

errorExit:
    if (nameSeparPtr != NULL)
         *nameSeparPtr = '.';
    if (newField != NULL)
        ckfree ((char *) newField);
    if (fieldInfo.argv != NULL)
        ckfree ((char *) fieldInfo.argv);
    return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_DeleteKeyedListField --
 *   Delete a field value in keyed list.
 *
 * Parameters:
 *   o tcl_interp (I/O) - Error message will be return in result if there is an
 *     error.
 *   o fieldName (I) - The name of the field to extract.  Will recusively
 *     process sub-field names seperated by `.'.
 *   o fieldValue (I) - The value to set for the field.
 *   o keyedList (I) - The keyed list to delete the field from.
 * Returns:
 *   A pointer to a dynamically allocated string containing the new list, or
 *   NULL if an error occured.
 *-----------------------------------------------------------------------------
 */
char *Tcl_DeleteKeyedListField (Tcl_Interp *tcl_interp, char *fieldName, char *keyedList)
{
    char        *nameSeparPtr;
    char        *newList;
    int          idx;
    fieldInfo_t  fieldInfo;
    char        *elemArgv [2];
    char        *newElement;
    /*
     * Check for sub-names, temporarly delimit the top name with a '\0'.
     */
    nameSeparPtr = strchr ((char *) fieldName, '.');
    if (nameSeparPtr != NULL)
        *nameSeparPtr = '\0';

    if (SplitAndFindField (tcl_interp, fieldName, keyedList, &fieldInfo) != TCL_OK)
        goto errorExit;

    if (fieldInfo.foundIdx == -1) {
        Tcl_AppendResult (tcl_interp, "field name not found: \"",  fieldName,
                          "\"", (char *) NULL);
        goto errorExit;
    }

    /*
     * If sub-field, recurse down to find the field to delete. If empty field
     * returned or no sub-field, delete the found entry by moving everything
     * up in the argv.
     */
    elemArgv [0] = (char *) fieldName;
    if (nameSeparPtr != NULL) {
        char saveChar = 0;

        if (fieldInfo.valuePtr != NULL) {
            saveChar = fieldInfo.valuePtr [fieldInfo.valueSize];
            fieldInfo.valuePtr [fieldInfo.valueSize] = '\0';
        }
        elemArgv [1] = Tcl_DeleteKeyedListField (tcl_interp, nameSeparPtr+1, 
                                                 fieldInfo.valuePtr);
        if (fieldInfo.valuePtr != NULL)
            fieldInfo.valuePtr [fieldInfo.valueSize] = saveChar;
        if (elemArgv [1] == NULL)
            goto errorExit;
        if (elemArgv [1][0] == '\0')
            newElement = NULL;
        else
            newElement = Tcl_Merge (2, elemArgv);
        ckfree (elemArgv [1]);
    } else
        newElement = NULL;

    if (newElement == NULL) {
        for (idx = fieldInfo.foundIdx; idx < fieldInfo.argc; idx++)
             fieldInfo.argv [idx] = fieldInfo.argv [idx + 1];
        fieldInfo.argc--;
    } else
        fieldInfo.argv [fieldInfo.foundIdx] = newElement;

    newList = Tcl_Merge (fieldInfo.argc, fieldInfo.argv);

    if (nameSeparPtr != NULL)
         *nameSeparPtr = '.';
    if (newElement != NULL)
        ckfree (newElement);
    ckfree ((char *) fieldInfo.argv);
    return newList;

errorExit:
    if (nameSeparPtr != NULL)
         *nameSeparPtr = '.';
    if (fieldInfo.argv != NULL)
         ckfree ((char *) fieldInfo.argv);
    return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_KeyldelCmd --
 *     Implements the TCL keyldel command:
 *         keyldel listvar key
 *
 * Results:
 *    Standard TCL results.
 *
 *----------------------------------------------------------------------------
 */
int Tcl_KeyldelCmd (ClientData clientData, Tcl_Interp *tcl_interp, int argc, char **argv)
{
    char  *keyedList, *newList;
    char  *varPtr;

    if (argc != 3) {
        Tcl_AppendResult (tcl_interp, tclXWrongArgs, argv [0],
                          " listvar key", (char *) NULL);
        return TCL_ERROR;
    }

    keyedList = Tcl_GetVar (tcl_interp, argv[1], TCL_LEAVE_ERR_MSG);
    if (keyedList == NULL)
        return TCL_ERROR;

    newList = Tcl_DeleteKeyedListField (tcl_interp, argv [2], keyedList);
    if (newList == NULL)
        return TCL_ERROR;

    varPtr = Tcl_SetVar (tcl_interp, argv [1], newList, TCL_LEAVE_ERR_MSG);
    ckfree ((char *) newList);

    return (varPtr == NULL) ? TCL_ERROR : TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_KeylgetCmd --
 *     Implements the TCL keylget command:
 *         keylget listvar ?key? ?retvar | {}?
 *
 * Results:
 *    Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int Tcl_KeylgetCmd (ClientData clientData, Tcl_Interp *tcl_interp, int argc, char **argv)
{
    char   *keyedList;
    char   *fieldValue;
    char  **fieldValuePtr;
    int     result;

    if ((argc < 2) || (argc > 4)) {
        Tcl_AppendResult (tcl_interp, tclXWrongArgs, argv [0],
                          " listvar ?key? ?retvar | {}?", (char *) NULL);
        return TCL_ERROR;
    }
    keyedList = Tcl_GetVar (tcl_interp, argv[1], TCL_LEAVE_ERR_MSG);
    if (keyedList == NULL)
        return TCL_ERROR;

    /*
     * Handle request for list of keys, use keylkeys command.
     */
    if (argc == 2)
        return Tcl_KeylkeysCmd (clientData, tcl_interp, argc, argv);

    /*
     * Handle retrieving a value for a specified key.
     */
    if (argv [2] == '\0') {
        tcl_interp->result = "null key not allowed";
        return TCL_ERROR;
    }
    if ((argc == 4) && (argv [3][0] == '\0'))
        fieldValuePtr = NULL;
    else
        fieldValuePtr = &fieldValue;

    result = Tcl_GetKeyedListField (tcl_interp, argv [2], keyedList,
                                    fieldValuePtr);
    if (result == TCL_ERROR)
        return TCL_ERROR;

    /*
     * Handle field name not found.
     */
    if (result == TCL_BREAK) {
        if (argc == 3) {
            Tcl_AppendResult (tcl_interp, "key \"", argv [2], 
                              "\" not found in keyed list", (char *) NULL);
            return TCL_ERROR;
        } else {
            tcl_interp->result = zero;
            return TCL_OK;
        }
    }

    /*
     * Handle field name found and return in the result.
     */
    if (argc == 3) {
        Tcl_SetResult (tcl_interp, fieldValue, TCL_DYNAMIC);
        return TCL_OK;
    }

    /*
     * Handle null return variable specified and key was found.
     */
    if (argv [3][0] == '\0') {
        tcl_interp->result = one;
        return TCL_OK;
    }

    /*
     * Handle returning the value to the variable.
     */
    if (Tcl_SetVar (tcl_interp, argv [3], fieldValue, TCL_LEAVE_ERR_MSG) == NULL)
        result = TCL_ERROR;
    else
        result = TCL_OK;
    ckfree (fieldValue);
    tcl_interp->result = one;
    return result;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_KeylkeysCmd --
 *     Implements the TCL keylkeys command:
 *         keylkeys listvar ?key?
 *
 * Results:
 *    Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int Tcl_KeylkeysCmd (ClientData clientData, Tcl_Interp *tcl_interp, int argc, char **argv)
{
    char   *keyedList, **keysArgv;
    int    result, keysArgc;

    if ((argc < 2) || (argc > 3)) {
        Tcl_AppendResult (tcl_interp, tclXWrongArgs, argv [0],
                          " listvar ?key?", (char *) NULL);
        return TCL_ERROR;
    }
    keyedList = Tcl_GetVar (tcl_interp, argv[1], TCL_LEAVE_ERR_MSG);
    if (keyedList == NULL)
        return TCL_ERROR;

    /*
     * If key argument is not specified, then argv [2] is NULL, meaning get
     * top level keys.
     */
    result = Tcl_GetKeyedListKeys (tcl_interp, argv [2], keyedList, &keysArgc,
                                   &keysArgv);
    if (result == TCL_ERROR)
        return TCL_ERROR;
    if (result  == TCL_BREAK) {
        Tcl_AppendResult (tcl_interp, "field name not found: \"",  argv [2],
                          "\"", (char *) NULL);
        return TCL_ERROR;
    }

    Tcl_SetResult (tcl_interp, Tcl_Merge (keysArgc, keysArgv), TCL_DYNAMIC);
    ckfree ((char *) keysArgv);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_KeylsetCmd --
 *     Implements the TCL keylset command:
 *         keylset listvar key value ?key value...?
 *
 * Results:
 *    Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int Tcl_KeylsetCmd (ClientData clientData, Tcl_Interp *tcl_interp, int argc, char **argv)
{
    char *keyedList, *newList, *prevList;
    char *varPtr;
    int   idx;

    if ((argc < 4) || ((argc % 2) != 0)) {
        Tcl_AppendResult (tcl_interp, tclXWrongArgs, argv [0],
                          " listvar key value ?key value...?", (char *) NULL);
        return TCL_ERROR;
    }

    keyedList = Tcl_GetVar (tcl_interp, argv[1], 0);
    
    newList = keyedList;
    for (idx = 2; idx < argc; idx += 2) {
        prevList = newList;
        newList = Tcl_SetKeyedListField (tcl_interp, argv [idx], argv [idx + 1],
                                         prevList);
        if (prevList != keyedList)
            ckfree (prevList);
        if (newList == NULL)
           return TCL_ERROR;
    }
    varPtr = Tcl_SetVar (tcl_interp, argv [1], newList, TCL_LEAVE_ERR_MSG);
    ckfree ((char *) newList);

    return (varPtr == NULL) ? TCL_ERROR : TCL_OK;
}


/* END KEYED LIST */

/* BEGIN LMATCH */


int Tcl_LmatchCmd(ClientData notUsed, Tcl_Interp *tcl_interp, int argc, char **argv)
{
#define EXACT   0
#define GLOB    1
#define REGEXP  2
    int listArgc;
    char **listArgv;
    Tcl_DString resultList;
    int i, match, mode;

    mode = GLOB;
    if (argc == 4) {
        if (STREQU(argv[1], "-exact")) {
            mode = EXACT;
        } else if (STREQU(argv[1], "-glob")) {
            mode = GLOB;
        } else if (STREQU(argv[1], "-regexp")) {
            mode = REGEXP;
        } else {
            Tcl_AppendResult(tcl_interp, "bad search mode \"", argv[1],
                    "\": must be -exact, -glob, or -regexp", (char *) NULL);
            return TCL_ERROR;
        }
    } else if (argc != 3) {
        Tcl_AppendResult(tcl_interp, "wrong # args: should be \"", argv[0],
                " ?mode? list pattern\"", (char *) NULL);
        return TCL_ERROR;
    }
    if (Tcl_SplitList(tcl_interp, argv[argc-2], &listArgc, &listArgv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (listArgc == 0) {
        ckfree ((char *) listArgv);
        return TCL_OK;
    }
    
    Tcl_DStringInit (&resultList);
    for (i = 0; i < listArgc; i++) {
        match = 0;
        switch (mode) {
            case EXACT:
                match = (STREQU (listArgv [i], argv [argc-1]));
                break;
            case GLOB:
                match = Tcl_StringMatch (listArgv [i], argv [argc-1]);
                break;
            case REGEXP:
                match = Tcl_RegExpMatch (tcl_interp, listArgv [i], argv [argc-1]);
                if (match < 0) {
                    ckfree ((char *) listArgv);
                    Tcl_DStringFree (&resultList);
                    return TCL_ERROR;
                }
                break;
        }
        if (match) {
            Tcl_DStringAppendElement (&resultList, listArgv [i]);
        }
    }
    ckfree ((char *) listArgv);
    Tcl_DStringResult (tcl_interp, &resultList);
    return TCL_OK;
}

/* LMATCH END */

int Tcl_RelativeExpr (Tcl_Interp *tcl_interp, char *cstringExpr, long stringLen, long *exprResultPtr)
{

    char *buf;
    int   exprLen, result;
    char  staticBuf [64];

    if (!(STRNEQU (cstringExpr, "end", 3) ||
          STRNEQU (cstringExpr, "len", 3))) 
    {
	return Tcl_ExprLong (tcl_interp, cstringExpr, exprResultPtr);
    }

    sprintf (staticBuf, "%ld",
             stringLen - ((cstringExpr [0] == 'e') ? 1 : 0));
    exprLen = strlen (staticBuf) + strlen (cstringExpr) - 2;

    buf = staticBuf;
    if (exprLen > sizeof (staticBuf)) {
        buf = (char *) ckalloc (exprLen);
        strcpy (buf, staticBuf);
    }
    strcat (buf, cstringExpr + 3);

    result = Tcl_ExprLong (tcl_interp, buf, exprResultPtr);

    if (buf != staticBuf)
        ckfree (buf);
    return result;
}



/*-----------------------------------------------------------------------------
 * Tcl_LvarpopCmd --
 *     Implements the TCL lvarpop command:
 *         lvarpop var ?indexExpr? ?string?
 *
 * Results:
 *      Standard TCL results.
 *-----------------------------------------------------------------------------
 */
int Tcl_LvarpopCmd (ClientData clientData, Tcl_Interp *tcl_interp, int argc, char **argv)
{
int        listArgc, idx;
long       listIdx;
char     **listArgv;
char      *varContents, *resultList, *returnElement;

    if ((argc < 2) || (argc > 4)) {
        Tcl_AppendResult (tcl_interp, tclXWrongArgs, argv [0], 
                          " var ?indexExpr? ?string?", (char *) NULL);
        return TCL_ERROR;
    }

    varContents = Tcl_GetVar (tcl_interp, argv[1], TCL_LEAVE_ERR_MSG);
    if (varContents == NULL)
        return TCL_ERROR;

    if (Tcl_SplitList (tcl_interp, varContents, &listArgc, &listArgv) == TCL_ERROR)
        return TCL_ERROR;

    if (argc == 2) {
        listIdx = 0;
    } else if (Tcl_RelativeExpr (tcl_interp, argv[2], listArgc, &listIdx)
               != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Just ignore out-of bounds requests, like standard Tcl.
     */
    if ((listIdx < 0) || (listIdx >= listArgc)) {
        goto okExit;
    }
    returnElement = listArgv [listIdx];

    if (argc == 4)
        listArgv [listIdx] = argv [3];
    else {
        listArgc--;
        for (idx = listIdx; idx < listArgc; idx++)
            listArgv [idx] = listArgv [idx+1];
    }

    resultList = Tcl_Merge (listArgc, listArgv);
    if (Tcl_SetVar (tcl_interp, argv [1], resultList, TCL_LEAVE_ERR_MSG) == NULL) {
        ckfree (resultList);
        goto errorExit;
    }
    ckfree (resultList);

    Tcl_SetResult (tcl_interp, returnElement, TCL_VOLATILE);
  okExit:
    ckfree((char *) listArgv);
    return TCL_OK;

  errorExit:
    ckfree((char *) listArgv);
    return TCL_ERROR;;
}
/*-----------------------------------------------------------------------------
 * Tcl_LemptyCmd --
 *     Implements the lempty TCL command:
 *         lempty list
 *
 * Results:
 *     Standard TCL result.
 *-----------------------------------------------------------------------------
 */
int Tcl_LemptyCmd (ClientData clientData, Tcl_Interp *tcl_interp, int argc, char **argv)
{
	char *scanPtr;

	if (argc != 2) 
	{
        	Tcl_AppendResult (tcl_interp, tclXWrongArgs, argv [0], " list", NULL);
		return TCL_ERROR;
	}

	scanPtr = argv [1];
	while ((*scanPtr != '\0') && (ISSPACE (*scanPtr)))
		scanPtr++;
	sprintf (tcl_interp->result, "%d", (*scanPtr == '\0'));
	return TCL_OK;
}





int msg_die(int idx, char *par)
{
	put_it("%s asked me to die", par);
	putlog(LOG_ALL, "*", "%s Requested we die",par);
	e_quit(NULL, "Requested DIE", NULL, NULL);
	return TCL_OK;
}

#if 0
int tcl_make_safe STDVAR
{
char *tmp = NULL, *s;
char buff[BIG_BUFFER_SIZE+1];
int i;
	if (argc == 1)
	{
		Tcl_AppendResult(irp, " ?args?", NULL);
		return TCL_ERROR;
	}
	for (i = 1; i < argc; i++)
		m_s3cat(&tmp, space, argv[i]);
	strcpy(buff, tmp);
	s = double_quote(tmp, "[]{}", buff);
	Tcl_AppendResult(irp, s, NULL);
	new_free(&tmp);
	return TCL_OK;
}
#endif




#if 0
void init_ircii_vars(Tcl_Interp *intp)
{
	for (
}
#endif

void init_public_tcl(Tcl_Interp *intp)
{
	Tcl_CreateCommand(intp, "ircii",	tcl_ircii,	NULL,NULL);
/* 
 * a couple of these have been borrowed from TclX as they are useful and not
 *  everyone has tclx installed on there system.
 */
	Tcl_CreateCommand(intp, "validuser",	tcl_validuser, NULL, NULL);
	Tcl_CreateCommand(intp, "pushmode",	tcl_pushmode, NULL, NULL);
	Tcl_CreateCommand(intp, "flushmode",	tcl_flushmode, NULL, NULL);
	Tcl_CreateCommand(intp, "lvarpop",	Tcl_LvarpopCmd, NULL, NULL);
	Tcl_CreateCommand(intp, "lempty",	Tcl_LemptyCmd, NULL, NULL);
	Tcl_CreateCommand(intp, "lmatch",	Tcl_LmatchCmd, NULL, NULL);
	Tcl_CreateCommand(intp, "keyldel",	Tcl_KeyldelCmd, NULL, NULL);
        Tcl_CreateCommand(intp, "keylget",	Tcl_KeylgetCmd, NULL, NULL);
        Tcl_CreateCommand(intp, "keylkeys",	Tcl_KeylkeysCmd, NULL, NULL);
        Tcl_CreateCommand(intp, "keylset",	Tcl_KeylsetCmd, NULL, NULL);
	Tcl_CreateCommand(intp, "maskhost",	tcl_maskhost,	NULL,NULL);
	Tcl_CreateCommand(intp, "onchansplit",	tcl_onchansplit,NULL,NULL);
	Tcl_CreateCommand(intp, "servers",	tcl_servers,	NULL,NULL);
	Tcl_CreateCommand(intp, "chanstruct",	tcl_chanstruct,	NULL,NULL);
	Tcl_CreateCommand(intp, "channel",	tcl_channel,	NULL,NULL);
	Tcl_CreateCommand(intp, "channels",	tcl_channels,	NULL,NULL);
	Tcl_CreateCommand(intp, "isop",		tcl_isop,	NULL,NULL);
	Tcl_CreateCommand(intp, "getchanhost",	tcl_getchanhost,NULL,NULL);
	Tcl_CreateCommand(intp, "matchattr",	matchattr,	NULL,NULL);
	Tcl_CreateCommand(intp, "finduser",	tcl_finduser,	NULL,NULL);
	Tcl_CreateCommand(intp, "findshit",	tcl_findshit,	NULL,NULL);
	Tcl_CreateCommand(intp, "date",		tcl_date,	NULL,NULL);
	Tcl_CreateCommand(intp, "getcomment",	tcl_getcomment, NULL,NULL);
	Tcl_CreateCommand(intp, "setcomment",	tcl_setcomment, NULL,NULL);
	Tcl_CreateCommand(intp, "time",		tcl_time,	NULL,NULL);
	Tcl_CreateCommand(intp, "ctime",	tcl_ctime,	NULL,NULL);
	Tcl_CreateCommand(intp, "onchan",	tcl_onchan,	NULL,NULL);
	Tcl_CreateCommand(intp, "chanlist",	tcl_chanlist,	NULL,NULL);
	Tcl_CreateCommand(intp, "unixtime",	tcl_unixtime,	NULL,NULL);
	Tcl_CreateCommand(intp, "putlog",	tcl_putlog,	NULL,NULL);
	Tcl_CreateCommand(intp, "putloglev",	tcl_putloglev,	NULL,NULL);
	Tcl_CreateCommand(intp, "rand",		tcl_rand,	NULL,NULL);
	Tcl_CreateCommand(intp, "timer",	tcl_timer,	NULL,NULL);
	Tcl_CreateCommand(intp, "killtimer",	tcl_killtimer,	NULL,NULL);
	Tcl_CreateCommand(intp, "utimer",	tcl_utimer,	NULL,NULL);
	Tcl_CreateCommand(intp, "killutimer",	tcl_killutimer,	NULL,NULL);
	Tcl_CreateCommand(intp, "timers",	tcl_timers,	NULL,NULL);
	Tcl_CreateCommand(intp, "utimers",	tcl_utimers,	NULL,NULL);
	Tcl_CreateCommand(intp, "putserv", 	tcl_putserv, 	NULL, NULL);
	Tcl_CreateCommand(intp, "putscr",	tcl_putscr,	NULL, NULL);
	Tcl_CreateCommand(intp, "putdcc",	tcl_putdcc,	NULL, NULL);
	Tcl_CreateCommand(intp, "putbot",	tcl_putbot,	NULL, NULL);
	Tcl_CreateCommand(intp, "putallbots",	tcl_putallbots,	NULL, NULL);
	Tcl_CreateCommand(intp, "bind",		tcl_bind,	(ClientData)0,NULL);
	Tcl_CreateCommand(intp, "binds",	tcl_tellbinds,	(ClientData)0,NULL);
	Tcl_CreateCommand(intp, "unbind",	tcl_bind,	(ClientData)1,NULL);
	Tcl_CreateCommand(intp, "strftime",	tcl_strftime,	NULL,NULL);
	Tcl_CreateCommand(intp, "cparse",	tcl_cparse,	NULL,NULL);
	Tcl_CreateCommand(intp, "userhost",	tcl_userhost,	NULL,NULL);
	Tcl_CreateCommand(intp, "getchanmode",	tcl_getchanmode,NULL,NULL);
	Tcl_CreateCommand(intp, "msg",		tcl_msg,	NULL,NULL);
	Tcl_CreateCommand(intp, "say",		tcl_say,	NULL,NULL);
	Tcl_CreateCommand(intp, "desc",		tcl_desc,	NULL,NULL);
	Tcl_CreateCommand(intp, "notice",	tcl_msg,	NULL,NULL);
	Tcl_CreateCommand(intp, "bots",		tcl_bots,	NULL, NULL);
	Tcl_CreateCommand(intp, "clients",	tcl_clients,	NULL, NULL);
	Tcl_CreateCommand(intp, "rword",	tcl_alias,	NULL, NULL);

	Tcl_CreateCommand(intp, "get_var",	tcl_get_var,	NULL, NULL);
	Tcl_CreateCommand(intp, "set_var",	tcl_set_var,	NULL, NULL);
	Tcl_CreateCommand(intp, "fget_var",	tcl_fget_var,	NULL, NULL);
	Tcl_CreateCommand(intp, "fset_var",	tcl_fset_var,	NULL, NULL);
	Tcl_CreateCommand(intp, "cset",		tcl_cset,	NULL, NULL);
	Tcl_CreateCommand(intp, "dccstats",	tcl_dcc_stat,	NULL, NULL);
	Tcl_CreateCommand(intp, "dccclose",	tcl_dcc_close,	NULL, NULL);
/*	Tcl_CreateCommand(intp, "makesafe",	tcl_make_safe,	NULL, NULL);*/
	
	add_tcl_alias(intp, tcl_alias, tcl_aliasvar);
}

typedef struct _tcl_var {
	char	*name;
	char	*var;
	int	length;
	int	flags;
} TclVars;

TclVars tcl_vars[] =
{
/*	{ "realname",	realname,	REALNAME_LEN,	TCL_GLOBAL_ONLY},*/
	{ "username",	username,	NAME_LEN,	TCL_GLOBAL_ONLY},
	{ "nickname",	nickname,	NICKNAME_LEN,	TCL_GLOBAL_ONLY},
	{ "version",	(char *)irc_version,29,		TCL_GLOBAL_ONLY},
	{ NULL,		NULL,		0,		0}
};

extern char *ircii_rem_str(ClientData *, Tcl_Interp *, char *, char *, int);
void init_public_var(Tcl_Interp *intp)
{
int i;
	for (i = 0; tcl_vars[i].name; i++)
		Tcl_SetVar(intp, tcl_vars[i].name, tcl_vars[i].var, tcl_vars[i].flags);
	if (current_window && current_window->current_channel)
		Tcl_SetVar(intp,"curchan",current_window->current_channel, TCL_GLOBAL_ONLY);
	if (from_server > -1)
	{
		Tcl_SetVar(intp,"server",get_server_name(from_server),TCL_GLOBAL_ONLY);
		Tcl_SetVar(intp,"botnick",get_server_nickname(from_server),TCL_GLOBAL_ONLY);
		Tcl_SetVar(intp,"nick",get_server_nickname(from_server),TCL_GLOBAL_ONLY);
	}
	else
		Tcl_SetVar(intp, "server", "none", TCL_GLOBAL_ONLY);
}

/* add a timer */
char *tcl_add_timer (TimerList **stack, long elapse, char *cmd, unsigned long prev_id)
{
TimerList *old = (*stack);
	*stack = (TimerList *) new_malloc(sizeof(TimerList));
	(*stack)->next = old; 

	(*stack)->interval = elapse;

	get_time(&((*stack)->time));
	(*stack)->time.tv_sec += elapse;
	(*stack)->time.tv_usec = 0;

	malloc_strcpy(&(*stack)->command, cmd);

	/* if it's just being added back and already had an id, */
	/* don't create a new one */
	if (prev_id > 0) 
	{
		(*stack)->refno = prev_id;
		strcpy((*stack)->ref, ltoa(prev_id));
	}
	else 
	{
		(*stack)->refno = timer_id;
		strcpy((*stack)->ref, ltoa(timer_id++));
	}
	return (*stack)->ref;
}

/* remove a timer, by id */
int tcl_remove_timer(TimerList **stack, unsigned long id)
{
TimerList *mark=*stack, *old; 
int ok = 0;
	*stack=NULL; 
	while (mark != NULL) 
	{
		if (strcmp(mark->ref, ltoa(id))) 
			tcl_add_timer(stack,mark->interval,mark->command,mark->refno);
		else 
			ok++;
		old = mark; 
		mark = mark->next;
		new_free(&old->command); 
		new_free((char **)&old);
	}
	return ok;
}

/* check timers, execute the ones that have expired */
void do_check_timers(TimerList **stack)
{
TimerList *mark=*stack, *old; 
Tcl_DString ds; 
int argc, i; 
char **argv;
struct timeval now1;

  /* new timers could be added by a Tcl script inside a current timer */
  /* so i'll just clear out the timer list completely, and add any */
  /* unexpired timers back on */

	*stack=NULL;
	while (mark != NULL) 
	{
		long left;
		get_time(&now1);
		if ((left = BX_time_diff(now1, mark->time)) <= 0)
		{
			int code;
			Tcl_DStringInit(&ds);
			if (Tcl_SplitList(tcl_interp,mark->command,&argc,&argv) != TCL_OK) 
				putlog(LOG_CRAP,"*","(Timer) Error for '%s': %s", mark->command, tcl_interp->result);
			else 
			{
				for (i=0; i<argc; i++) 
					Tcl_DStringAppendElement(&ds,argv[i]);
				free(argv);
				code=Tcl_Eval(tcl_interp,Tcl_DStringValue(&ds));
				/* code=Tcl_Eval(tcl_interp,mark->cmd); */
				Tcl_DStringFree(&ds);
				if (code!=TCL_OK)
					putlog(LOG_CRAP,"*","(Timer) Error for '%s': %s", mark->command, tcl_interp->result);
			}
		}
		else 
			tcl_add_timer(stack,left,mark->command,mark->refno);
		old=mark; 
		mark=mark->next;
		new_free(&old->command); 
		new_free((char **)&old);
	}
}

void check_timers(void)
{
	do_check_timers(&tcl_Pending_timers);
}

void check_utimers(void)
{
	do_check_timers(&tcl_Pending_utimers);
}

void tcl_list_timer(Tcl_Interp *irp, TimerList *stack)
{
TimerList *mark = stack; 
char *x = NULL;
struct timeval current;
long time_left;
	get_time(&current);
	for (mark = stack; mark; mark = mark->next) 
	{
		time_left = BX_time_diff(current, mark->time);
		if (time_left < 0)
			time_left = 0;
		malloc_sprintf(&x, "%u %s timer%lu", time_left, mark->command, mark->refno);
		Tcl_AppendElement(irp,x);
		new_free(&x);
	}
}

static struct timeval current;

time_t tclTimerTimeout(time_t timeout)
{
	register TimerList	*stack = NULL;
	long			this_timeout = 0;

	if (timeout == 0)
		return 0;
	get_time(&current);
	if ((stack = tcl_Pending_timers))
	{
		/* this is in minutes */
		for (; stack; stack = stack->next)
			if ((this_timeout = (BX_time_diff(current, stack->time)) * 1000) < timeout)
				timeout = this_timeout;
	}
	if ((stack = tcl_Pending_utimers))
	{
		/* this is in seconds */
		for (; stack; stack = stack->next)
			if ((this_timeout = (BX_time_diff(current, stack->time)) * 1000) < timeout)
				timeout = this_timeout;
	}
#if defined(WINNT) || defined(__EMX__)
	return (timeout == MAGIC_TIMEOUT) ? MAGIC_TIMEOUT : timeout;
#else
	return timeout;
#endif
}

#else /* WANT_TCL */

time_t tclTimerTimeout(time_t timeout)
{
#if defined(WINNT) || defined(__EMX__)
	return (timeout == MAGIC_TIMEOUT) ? MAGIC_TIMEOUT : timeout;
#else
	return timeout < MAGIC_TIMEOUT ? timeout : MAGIC_TIMEOUT;
#endif
}

int check_tcl_dcc(char *cmd, char *nick, char *host, int idx)
{
	int x, atr = 0;
	int old_server = from_server;
	char *c, *args;
	DCC_int *info;
	
	if (from_server == -1)
		from_server = get_window_server(0);
	info = get_socketinfo(idx);
	if (info->ul)
		atr = info->ul->flags;
#if 0		
	if ((n = lookup_userlevelc("*", host, "*", NULL)))
	{
		DCC_int *info;
		info = get_socketinfo(idx);
		atr = n->flags;
		info->ul = n;
	}
#endif
	if (!cmd || !*cmd)
		return 0;
	c = next_arg(cmd, &cmd);
	args = cmd;
	for (x = 0; C_dcc[x].func; x++) 
	{
		if (!my_stricmp(c, C_dcc[x].name) )
		{
			if ((C_dcc[x].access & atr) || !C_dcc[x].access)
				(C_dcc[x].func)(idx,args);
			else
				dcc_printf(idx, "Access denied.\n");
			from_server = old_server;
			return 1;
		}
	}
	dcc_printf(idx, "Invalid command [%s]\n", c);
	from_server = old_server;
	return 1;
}


#endif

