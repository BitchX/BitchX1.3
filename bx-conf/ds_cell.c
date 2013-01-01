/*
 * Program source is Copyright 1993 Colten Edwards.
 * Central Data Services is granted the right to use this source
 * in thier product known as Quote Menu. They are also granted the right
 * to modify this source in the advent of problems and or new features
 * which they may want to add to the program.
 */


#include <stdlib.h>
#include <ctype.h>
#include <ncurses.h>
#ifdef __EMX__
#include <sys/types.h>
#endif

#include "ds_cell.h"
#include "ds_keys.h"
#include "func_pr.h"


/*
 * This routine sets the stage for all menu actions except those that are
 * specific for a menu.
 * Home, PgUp, PgDn, End, Cursor up, Cursor Dn are defined within this source
 */

/* Get the current menu item.
 *
 * Save the current position, save the start of the list.
 * if key was not a enter key then
 *     Convert character to uppercase
 *     While double linked list is not NULL
 *       if entered key is equal to stored key value
 *            break loop
 *       else
 *            get next double linked list item
 *       set current item equal
 *
 * if double linked list item is equal to NULL
 *     restore current item to entry value.
 *     return to caller with FALSE { didn't find the a valid key}
 * Set redraw = TRUE and
 * return TRUE to indicate found valid key
 */

int get_current(CELL *c) {
dlistptr x = c->start;
dlistptr y = c->current;
    if (c->key != ENTER) {
        c->key = toupper(c->key);
        while (x)
           if (c->key == x->datainfo.other) {
               break;
           } else
               x = x->nextlistptr;
        c->current = x;
    }
    if (x == NULL) {
        c->current = y;
        return FALSE;
    }
    if(c->ListPaintProc != NULL)
        (*c->ListPaintProc)(c);
    return TRUE;
}

/*
 * Purpose is set the list start and end values. Main use is for lists that
 * stretch longer then the window settings.
 */

int set_list_end(CELL *c) {
register int x = 0;
    if (c->current == NULL)
        return x;
    c->list_end = c->list_start;
    while (x  < (c->erow - c->srow)) {
       if (c->list_end->nextlistptr)
           c->list_end = c->list_end->nextlistptr;
       else
           break;
       x++;
    }
   return x;
}

/*
   Move the cursor up a line with wrapping
*/

int wrap_cursor_up (CELL * c)
{
    if (!cursor_up (c))
       ls_end (c);
    return TRUE;
}



/*
   Move the cursor down a line with wrapping
*/
int wrap_cursor_dn (CELL * c)
{
    if (!cursor_dn (c))
       ls_home (c);
    return TRUE;
}

/*
   Move the cursor up a line.

   Returns:  TRUE if successful.
            FALSE if it fails. Fails because we are already at the
            first item of the list.
*/

int cursor_up (CELL * c)
{
   c->redraw = TRUE;
   if (c->current == NULL)
      c->redraw = FALSE;
   else {
       if (c->current == c->list_start) {
           if (c->current->prevlistptr) {
               c->current = c->list_start = c->current->prevlistptr;
               set_list_end(c);
           } else
               c->redraw = FALSE;
       } else {
           if (c->current->prevlistptr)
               c->current = c->current->prevlistptr;
           else
               c->redraw = FALSE;
       }
   }
   c->desc_start = 0;
   c->cur_pos = CURCOL;
   return c->redraw;
}

/*
   Move the cursor down a line.

   Returns:  TRUE if successful.
            FALSE if it fails. Fails because we are already at the
            first item of the list.
*/

int cursor_dn (CELL * c)
{
   c->redraw = TRUE;
   if (c->current == NULL)
       c->redraw = FALSE;
   else {
       if (c->current == c->list_end) {
               if (c->list_end->nextlistptr) {
                   c->list_end = c->current = c->list_end->nextlistptr;
                   if (c->list_start->nextlistptr)
                       c->list_start = c->list_start->nextlistptr;
               } else
                   c->redraw = FALSE;
       } else {
           if (c->current->nextlistptr != NULL)
               c->current = c->current->nextlistptr;
           else
               c->redraw = FALSE;
       }
   }
   c->desc_start = 0;
   c->cur_pos = CURCOL;
   return c->redraw;
}

/*
 * Purpose to pgup through the list values
 */
int ls_pgup (CELL * c)
{
    register int pagesize = c->erow - c->srow;
    if (c->current == NULL)
       return TRUE;
    while ((pagesize) && (c->current->prevlistptr != NULL)){
           c->current = c->current->prevlistptr;
           pagesize--;
    }
    c->list_start = c->current;
    set_list_end(c);
    c -> redraw = TRUE;
    c->desc_start = 0;
   c->cur_pos = CURCOL;
    return TRUE;
}

/*
 * Purpose to pgdn through the list values
 */

int ls_pgdn (CELL * c)
{
    register int pagesize = c->erow - c->srow;
    if (c->current == NULL)
        return TRUE;
    while ((pagesize) && (c->current->nextlistptr != NULL)){
           c->current = c->current->nextlistptr;
           pagesize--;
    }
    if (pagesize == 0)
        c->list_start = c->current;
    set_list_end(c);
    c->redraw = TRUE;
    c->desc_start = 0;
    c->cur_pos = CURCOL;
    return TRUE;
}

/*
 * purpose to set the home the list.
 */
int ls_home (CELL * c)
{
    c->current = c->list_start = c->start;
    set_list_end(c);
    c->redraw = TRUE;
    return TRUE;
}

/*
 * Purpose is to go to end of list.
 */

int ls_end (CELL * c)
{
    int x = 0;
    if (c->current == NULL)
        return TRUE;
    c->current = c->list_end = c->list_start = c->end;
    while (x < (c->erow - c->srow)){
       if (c->list_start->prevlistptr)
           c->list_start = c->list_start->prevlistptr;
       else
           break;
       x++;
    }
    c -> redraw = TRUE;
    return TRUE;
}



/*
 * This is the main routine used throughout.
 * Purpose is to be a function despatcher.
 * On entry
 *  set termkey = 0
 *  if we have a list entry procedure then
 *     exec that procedure. It usely draws the box and creates the list
 *  Make sure the list start and end are set so we do not overstep boundries
 *  While terminate key is equal to 0
 *     set function hit = false
 *     if list redraw is TRUE and we have a redraw procedure
 *        exec redraw procedure.  Usually draws only items in the list.
 *     if we have a UpdateStatusProc
 *        exec Updatestatusproc. Usually displays counts, Menus, etc.
 *     if termkey is equal to 0
 *        if othergetkey is TRUE and othergetkeyproc is not equal to NULL
 *            exec othergetkeyproc. Used to provide edit functions
 *        else
 *            call get char function
 *     Set special to 0. Indicates which function to call, iff found in list.
 *     Check menu list until end of list (-1) for a matching key value.
 *        if key and menu item key match and function is not NULL
 *           set special equal to special number
 *           exec function and save returned value in hit.
 *           hit indicates whether or not we have exec'd a function for a key
 *           break to end of loop
 *     if hit is FALSE and current_event is not equal to NULL
 *        exec current event. This allows us to check for both upper and lower
 *        case keys.
 *     loop to top and check termkey.
 *
 *     if termkey was set then
 *     we clear the list and exit function dispatch.
 *     return terminate key
 */

int ls_dispatch (CELL * c)
{
    long x;
    long hit;

    c -> termkey = 0;
    if ((*c -> ListEntryProc) != NULL)
       x = (*c -> ListEntryProc) (c);
/*
    if (c->start == NULL) {
       return c->termkey;
    }
*/
    set_list_end(c);

    while (c -> termkey == 0/* && c->start*/) {
       hit = FALSE;
       if (c->redraw && ((*c -> ListPaintProc) != NULL))
           (*c -> ListPaintProc) (c);
       if (*c -> UpdateStatusProc != NULL)
           (*c -> UpdateStatusProc) (c);
       if (c -> termkey == 0) {
           if ((*c -> OtherGetKeyProc) != NULL && c->other_getkey)
               c -> key = (*c->OtherGetKeyProc) (c);
           else
               c->key = getch();
#if 0
               c->key = get_char (c, (int)c->menu_bar);
#endif
         }
       c->special = 0;
       for (x = 0; c ->keytable[x].key != -1; x++)
           if ((c -> keytable[x].key == c->key) && ((*c->func_table[c->keytable[x].func_num].func) != NULL)) {
               c->special = c->keytable[x].func_num;
               hit = (*c->func_table[c->keytable[x].func_num].func)(c);
               break;
           }
       if (hit == FALSE && *c->current_event != NULL)
           (*c -> current_event) (c);
    }
    if (*c -> ListExitProc != NULL)
       (*c -> ListExitProc) (c);
    return c -> termkey;
}

/*
 * Terminate function. Set termkey to the keystroke entered.
 */

int ls_quit (CELL * c)
{
    c->termkey = c->key;
    return TRUE;
}


