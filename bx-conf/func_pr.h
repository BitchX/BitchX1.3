/* BACK_TYP.C 06/05/93 23.13.26 */
int backup_type1 (CELL *c);
int update_back1 (CELL *c);
int update_back (CELL *c);
int select_type (CELL *c);
/* DO_DRIVE.C 06/05/93 23.13.24 */
int update_drive (CELL *c);
int Drive_Entry (CELL *c);
int drive_redraw (CELL *c);
int floppy_drive (CELL *c);
int hard_drive (CELL *c);
int get_drive (CELL *c);
/* DO_SETUP.C 11/05/93 05.57.20 */
int save_user (CELL *c);
int delete_user (CELL *c);
int validkey (long validkey, int input);
int desc_edit (CELL *c);
int eredraw (CELL *c);
int insert_edlist (CELL *c, char *ffblk, int menu_numb);
int eList_Entry (CELL *c);
int edit_user (CELL *c);
/* DO_UPDAT.C 06/05/93 23.13.32 */
int update_func (CELL *c);
/* DS_ATTIB.C 06/05/93 23.13.32 */
void attrib (int strow, int stcol, int edrow, int edcol, int color);
/* DS_BOX.C 06/05/93 23.13.32 */
/*
void box (int strow, int stcol, int edrow, int edcol, int border, int fill, int
	 lines, int shadow, int shad_color);
void hline (int strow, int stcol, int edrow, int edcol, int border, int lines);
void vline (int strow, int stcol, int edrow, int edcol, int border, int lines);
*/
/* DS_CELL.C 06/05/93 23.13.30 */
int get_current (CELL *c);
int set_list_end (CELL *c);
int wrap_cursor_up (CELL *c);
int wrap_cursor_dn (CELL *c);
int cursor_up (CELL *c);
int cursor_dn (CELL *c);
int ls_pgup (CELL *c);
int ls_pgdn (CELL *c);
int ls_home (CELL *c);
int ls_end (CELL *c);
int ls_dispatch (CELL *c);
int ls_quit (CELL *c);
/* DS_CHAR.C 06/05/93 23.13.30 */
void attrib_ch (int strow, int stcol, int edrow, int edcol, int color, char cha
	);
/* DS_CONFI.C 06/05/93 23.13.30 */
void do_color_select (CELL *c);
int do_config_write (char *argv[], int read_write);
void calc_offset (int color, int *r, int *c);
int color_select (CELL *c1, int srow, int scol, int color, char *title);
void color_chart (int srow, int scol);
int update_color (CELL *c);
int select_color (CELL *c);
/* DS_CRIT.C 06/05/93 23.13.30 */
int handle_quiet (int di, int ax, int bp, int si);
int handler (int di, int ax, int bp, int si);
int critical_errorwnd (char *msg);
/* DS_LLIST.C 06/05/93 23.13.30 */
int q_func (CELL *c);
void init_dlist (CELL *c);
int clear_dlist (CELL *c);
char *Display (CELL *c, dlistptr *thisptr);
long insert_dlist (CELL *c, char *ffblk, int menu_numb);
int do_dir (CELL *c);
int List_Entry (CELL *c);
int List_Exit (CELL *c);
int redraw (CELL *c);
void get_args (CELL *c, int argc, char *argv[]);
/* DS_PATCH.C 11/05/93 20.03.50 */
int error_message (CELL *c, int error_code, unsigned char exec_return);
int do_shell (CELL *c);
int swap_it (CELL *c, char *program, char *params, char *exit_pr, int pause,
	 int do_swap, int command_proc, int verify_flag, int change_dir);
int swap_box (CELL *c, char *program, char *comm_line, int swap_prog, int flags
	);
/* DS_SORT.C 06/05/93 23.13.28 */
int comp_name (void *a, void *b);
int sort_buffer (CELL *c);
void *next_item (void *current);
void set_next (void *first, void *second);
void *qsortl (void *list, void * (*getnext)(void *), void (*setnext)(void *,
	 void *), int (*compare)(void *, void *));
/* DS_VIDEO.C 06/05/93 23.13.28 */
void check_desqv (CELL *c);
/* DS_VREST.C 06/05/93 23.13.28 */
void scr_rest (int strow, int stcol, int edrow, int edcol, char *buffer);
/* DS_VSAVE.C 06/05/93 23.13.28 */
void scr_save (int strow, int stcol, int edrow, int edcol, char *buffer);
/* FILE_CPY.C 11/05/93 19.40.46 */
int delete_dlist (CELL *c);
int change_dir (CELL *c);
int status_update (CELL *c);
int toggle (CELL *c);
int plus_it (CELL *c);
int minus_it (CELL *c);
/*int insert_fdlist (CELL *c, struct ffblk *ff_blk);*/
int File_Entry (CELL *c);
char *pad_name (char *fn);
char *fDisplay (dlistptr *ptr);
int fredraw (CELL *c);
int file_routine (CELL *c);
int change_dest_src (CELL *c);
int update_select (CELL *c);
int dest_src_select (CELL *c);
int file_copyr (char *from, char *to);
int file_settime (char *src, char *dst);
int copy_file (CELL *c);
int delete_file (CELL *c);
int in_default (_default *defaults, char answer);
int do_getyn (char *prompt, _default *defaults, char def_answer, int wrow, int
	 wcol);
int edit_path (CELL *c);
char *uDisplay (CELL *c, char *str);
int uredraw (CELL *c);
int ueList_Entry (CELL *c);
int setuserpath (CELL *c);
int dosExpandPath (char *path, char *filename, char *new_path);
void dosRelativePath (char *path);
int isDir (char *filename);
int GetUserPath (CELL *c);
int do_getuser (CELL *c, char *buff);
/* FILE_UTI.C 06/05/93 23.13.26 */
int update_file (CELL *c);
int do_file_util (CELL *c);
/* FMT_LONG.C 06/05/93 23.13.26 */
char *fmt_long (unsigned long int val);

void cursor (int x, int y);
void savecursor (unsigned newcursor);
void restorecursor (void);
void rscroll (int updown, int lines, int strow, int stcol, int endrow, int
	 endcol, int attribute);
void curr_cursor (int *x, int *y);
void set_cursor_type (int t);
void clear_screen (void);
void clear_ch (void);
int vmode (void);
int scroll_lock (void);
void os_version (char *error_str);
void string (int color, int asciichar, int repeats);
void pushkey (int key);
int isxkeybrd (void);
int get_char (CELL *c, int menu);
int get_num_floppy (void);
char *volume_label (void);
int ren_wild (char *old, char *new);
int kb_hit (void);
/* PRNSTR1.C 06/05/93 23.13.26 */
int prnstr (char *text, int row, int col, unsigned color);
/* USER_FUN.C 06/05/93 23.13.26 */
int user_key (CELL *c);
int do_user (CELL *c);
int UList_Entry (CELL *c);
int user_func (CELL *c);
int play_mp3(CELL *c);