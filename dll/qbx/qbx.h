#define QBX_VERSION "0.5"

struct quake_variable {
	char name[256];
	char value[256];
};

#define Q3A_COMMAND "!q3a"
#define QW_COMMAND "!qw"
#define Q2_COMMAND "!q2"
#define HL_COMMAND "!hl"

#define Q3A_GAME_PORT 27960
#define QW_GAME_PORT 27500
#define Q2_GAME_PORT 27910
#define HL_GAME_PORT 27015
