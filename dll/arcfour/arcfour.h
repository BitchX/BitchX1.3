typedef unsigned char arcword;		/* 8-bit groups */

typedef struct {
   arcword state[256], i, j;
} arckey;

/* Prototypes */
static inline void arcfourInit(arckey *, char *, unsigned short);
static inline char *arcfourCrypt(arckey *, char *, int);
static int send_dcc_encrypt (int, int, char *, int);
static int get_dcc_encrypt (int, int, char *, int, int);
static int start_dcc_crypt (int, int, unsigned long, int);
static int end_dcc_crypt (int, unsigned long, int);
static int init_schat(char *, char *, char *, char *, char *, char *, unsigned long, unsigned int);
void dcc_sdcc (char *, char *);
