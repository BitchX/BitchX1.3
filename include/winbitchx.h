
#ifndef WIN_BitchX_h
#define WIN_BitchX_h

#include <windows.h>

/* prototypes */
void VTActivate();
void ChangeTitle();
void SwitchMenu();
void SwitchTitleBar();
void OpenHelp(HWND HWin, UINT Command, DWORD Data);
void ResetTerminal();
void ResetCharSet();
void HideStatusLine();
void ChangeTerminalSize(int Nx, int Ny);
int VTParse();

void EnableDlgItem(HWND HDlg, int FirstId, int LastId);
void DisableDlgItem(HWND HDlg, int FirstId, int LastId);
void ShowDlgItem(HWND HDlg, int FirstId, int LastId);
void SetRB(HWND HDlg, int R, int FirstId, int LastId);
void GetRB(HWND HDlg, LPWORD R, int FirstId, int LastId);
void SetDlgNum(HWND HDlg, int id_Item, LONG Num);
void SetDlgPercent(HWND HDlg, int id_Item, LONG a, LONG b);
/*void SetDropDownList(HWND HDlg, int Id_Item, PCHAR far *List, int nsel);*/
LONG GetCurSel(HWND HDlg, int Id_Item);

void InitDisp();
void EndDisp();
void DispReset();
void DispConvWinToScreen
  (int Xw, int Yw, int *Xs, int *Ys, LPBOOL Right);
void SetLogFont();
void ChangeFont();
void ResetIME();
void ChangeCaret();
void CaretOn();
void CaretOff();
void DispDestroyCaret();
BOOL IsCaretOn();
void DispEnableCaret(BOOL On);
BOOL IsCaretEnabled();
void DispSetCaretWidth(BOOL DW);
void DispChangeWinSize(int Nx, int Ny);
void ResizeWindow(int x, int y, int w, int h, int cw, int ch);
void PaintWindow(HDC PaintDC, RECT PaintRect, BOOL fBkGnd,
		 int* Xs, int* Ys, int* Xe, int* Ye);
void DispEndPaint();
void DispClearWin();
void DispChangeBackground();
void DispChangeWin();
void DispInitDC();
void DispReleaseDC();
void DispSetupDC(BYTE Attr, BYTE Attr2, BOOL Reverse);
void DispStr(PCHAR Buff, int Count, int Y, int* X);
void DispEraseCurToEnd(int YEnd);
void DispEraseHomeToCur(int YHome);
void DispEraseCharsInLine(int XStart, int Count);
BOOL DispDeleteLines(int Count, int YEnd);
BOOL DispInsertLines(int Count, int YEnd);
BOOL IsLineVisible(int* X, int* Y);
void AdjustScrollBar();
void DispScrollToCursor(int CurX, int CurY);
void DispScrollNLines(int Top, int Bottom, int Direction);
void DispCountScroll();
void DispUpdateScroll();
void DispScrollHomePos();
void DispAutoScroll(POINT p);
void DispHScroll(int Func, int Pos);
void DispVScroll(int Func, int Pos);
void DispSetupFontDlg();
void DispRestoreWinSize();
void DispSetWinPos();
void DispSetActive(BOOL ActiveFlag);

  /* Character attribute bit masks */
#define AttrDefault 0x00
#define AttrDefault2 0x00
#define AttrBold 0x01
#define AttrUnder 0x02
#define AttrSpecial 0x04
#define AttrFontMask 0x07
#define AttrBlink 0x08
#define AttrReverse 0x10
#define AttrKanji 0x80

extern int WinWidth, WinHeight;
extern HFONT VTFont[AttrFontMask+1];
extern int FontHeight, FontWidth, ScreenWidth, ScreenHeight;
extern BOOL AdjustSize, DontChangeSize;
extern int CursorX, CursorY;
extern int WinOrgX, WinOrgY, NewOrgX, NewOrgY;
extern int NumOfLines, NumOfColumns;
extern int PageStart, BuffEnd;

#define SCROLL_BOTTOM	1
#define SCROLL_LINEDOWN	2
#define SCROLL_LINEUP	3
#define SCROLL_PAGEDOWN	4
#define SCROLL_PAGEUP	5
#define SCROLL_POS	6
#define SCROLL_TOP	7

  /* Parsing modes */
#define ModeFirst 0
#define ModeESC   1
#define ModeDCS   2
#define ModeDCUserKey 3
#define ModeSOS   4
#define ModeCSI   5
#define ModeXS	  6
#define ModeDLE   7
#define ModeCAN   8

extern HWND HVTWin;
extern HWND HTEKWin;
extern int ActiveWin; /* IdVT, IdTEK */
extern int TalkStatus; /* IdTalkKeyb, IdTalkCB, IdTalkTextFile */
extern BOOL KeybEnabled; /* keyboard switch */
extern BOOL Connecting;

/* 'help' button on dialog box */
extern WORD MsgDlgHelp;
extern LONG HelpId;

/*extern TTTSet ts;
extern TComVar cv;*/

/* pointers to window objects */
extern void* pVTWin;
extern void* pTEKWin;
/* instance handle */
extern HINSTANCE hInst;

extern int SerialNo;

#define IdBreakTimer 1
#define IdDelayTimer 2
#define IdProtoTimer 3
#define IdDblClkTimer 4
#define IdScrollTimer 5
#define IdComEndTimer 6
#define IdCaretTimer 7
#define IdPrnStartTimer 8
#define IdPrnProcTimer 9

  /* Window Id */
#define IdVT 1
#define IdTEK 2

  /* Talker mode */
#define IdTalkKeyb 0
#define IdTalkCB 1
#define IdTalkFile 2
#define IdTalkQuiet 3

  /* Character sets */
#define IdASCII 0
#define IdKatakana 1
#define IdKanji 2
#define IdSpecial 3

  /* Color attribute bit masks */
#define Attr2Fore 0x08
#define Attr2ForeMask 0x07
#define Attr2Back 0x80
#define Attr2BackMask 0x70
#define SftAttrBack 4

  /* Color codes */
#define IdBack   0
#define IdRed    1
#define IdGreen  2
#define IdYellow 3
#define IdBlue   4
#define IdMagenta 5
#define IdCyan   6
#define IdFore   7

  /* Kermit function id */
#define IdKmtReceive 1
#define IdKmtGet 2
#define IdKmtSend 3
#define IdKmtFinish 4

  /* XMODEM function id */
#define IdXReceive 1
#define IdXSend 2

  /* ZMODEM function id */
#define IdZReceive 1
#define IdZSend 2
#define IdZAuto 3

  /* B-Plus function id */
#define IdBPReceive 1
#define IdBPSend 2
#define IdBPAuto 3

  /* Quick-VAN function id */
#define IdQVReceive 1
#define IdQVSend 2

#define HostNameMaxLength 80

  /* internal WM_USER messages */
#define WM_USER_ACCELCOMMAND WM_USER+1
#define WM_USER_CHANGEMENU WM_USER+2
#define WM_USER_CLOSEIME WM_USER+3
#ifdef TERATERM32
#define WM_USER_COMMNOTIFY WM_USER+4
#else
#define WM_USER_COMMNOTIFY WM_COMMNOTIFY
#endif
#define WM_USER_COMMOPEN WM_USER+5
#define WM_USER_COMMSTART WM_USER+6
#define WM_USER_DLGHELP2 WM_USER+7
#define WM_USER_GETHOST WM_USER+8
#define WM_USER_FTCANCEL WM_USER+9
#define WM_USER_PROTOCANCEL WM_USER+10
#define WM_USER_CHANGETBAR WM_USER+11
#define WM_USER_KEYCODE WM_USER+12
#define WM_USER_GETSERIALNO WM_USER+13

#define WM_USER_DDEREADY WM_USER+21
#define WM_USER_DDECMNDEND WM_USER+22
#define WM_USER_DDECOMREADY WM_USER+23
#define WM_USER_DDEEND WM_USER+24

  /* port type ID */
#define IdTCPIP 1
#define IdSerial 2
#define IdFile  3

  /* XMODEM option */
#define XoptCheck 1
#define XoptCRC  2
#define Xopt1K   3

  /* Language */
#define IdEnglish 1
#define IdJapanese 2
#define IdRussian 3

// log flags (used in ts.LogFlag) 
#define LOG_TEL	1
#define LOG_KMT	2
#define LOG_X	4
#define LOG_Z	8
#define LOG_BP	16
#define LOG_QV	32

// file transfer flags (used in ts.FTFlag)
#define FT_ZESCCTL	1
#define FT_ZAUTO	2
#define FT_BPESCCTL	4
#define FT_BPAUTO	8
#define FT_RENAME	16

// menu flags (used in ts.MenuFlag)
#define MF_NOSHOWMENU	1
#define MF_NOPOPUP		2
#define MF_NOLANGUAGE	4
#define MF_SHOWWINMENU  8

// Terminal flags (used in ts.TermFlag)
#define TF_FIXEDJIS	1
#define TF_AUTOINVOKE	2
#define TF_CTRLINKANJI	8
#define TF_ALLOWWRONGSEQUENCE 16
#define TF_ACCEPT8BITCTRL 32
#define TF_ENABLESLINE	64
#define TF_BACKWRAP	128

// ANSI color flags (used in ts.ColorFlag)
#define CF_FULLCOLOR	1
#define CF_USETEXTCOLOR 2

// port flags (used in ts.PortFlag)
#define PF_CONFIRMDISCONN 1
#define PF_BEEPONCONNECT  2

#define IdCR   1
#define IdCRLF 2

  /* Terminal ID */
#define IdVT100 1
#define IdVT100J 2
#define IdVT101 3
#define IdVT102 4
#define IdVT102J 5
#define IdVT220J 6
#define IdVT282 7
#define IdVT320 8
#define IdVT382 9

  /* Kanji Code ID */
#define IdSJIS 1
#define IdEUC 2
#define IdJIS 3

// Russian code sets
#define IdWindows	1
#define IdKOI8		2
#define Id866		3
#define IdISO		4

  /* KanjiIn modes */
#define IdKanjiInA 1
#define IdKanjiInB 2
  /* KanjiOut modes */
#define IdKanjiOutB 1
#define IdKanjiOutJ 2
#define IdKanjiOutH 3

#define TermWidthMax 300
#define TermHeightMax 200

  /* Cursor shapes */
#define IdBlkCur 1
#define IdVCur 2
#define IdHCur 3

#define IdBS 1
#define IdDEL 2

  /* Serial port ID */
#define IdCOM1 1
#define IdCOM2 2
#define IdCOM3 3
#define IdCOM4 4
  /* Baud rate ID */
#define IdBaud110 1
#define IdBaud300 2
#define IdBaud600 3
#define IdBaud1200 4
#define IdBaud2400 5
#define IdBaud4800 6
#define IdBaud9600 7
#define IdBaud14400 8
#define IdBaud19200 9
#define IdBaud38400 10
#define IdBaud57600 11
#define IdBaud115200 12

  /* Parity ID */
#define IdParityEven 1
#define IdParityOdd 2
#define IdParityNone 3
  /* Data bit ID */
#define IdDataBit7 1
#define IdDataBit8 2
  /* Stop bit ID */
#define IdStopBit1 1
#define IdStopBit2 2
  /* Flow control ID */
#define IdFlowX 1
#define IdFlowHard 2
#define IdFlowNone 3

/* GetHoerm internal key codes */
#define IdUp     1
#define IdDown   2
#define IdRight  3
#define IdLeft   4
#define Id0      5
#define Id1      6
#define Id2      7
#define Id3      8
#define Id4      9
#define Id5     10
#define Id6     11
#define Id7     12
#define Id8     13
#define Id9     14
#define IdMinus 15
#define IdComma 16
#define IdPeriod 17
#define IdEnter 18
#define IdPF1   19
#define IdPF2   20
#define IdPF3   21
#define IdPF4   22
#define IdFind  23
#define IdInsert 24
#define IdRemove 25
#define IdSelect 26
#define IdPrev  27
#define IdNext  28
#define IdHold  29
#define IdPrint 30
#define IdBreak 31
#define IdF6    32
#define IdF7    33
#define IdF8    34
#define IdF9    35
#define IdF10   36
#define IdF11   37
#define IdF12   38
#define IdF13   39
#define IdF14   40
#define IdHelp  41
#define IdDo    42
#define IdF17   43
#define IdF18   44
#define IdF19   45
#define IdF20   46
#define IdUDK6  47
#define IdUDK7  48
#define IdUDK8  49
#define IdUDK9  50
#define IdUDK10 51
#define IdUDK11 52
#define IdUDK12 53
#define IdUDK13 54
#define IdUDK14 55
#define IdUDK15 56
#define IdUDK16 57
#define IdUDK17 58
#define IdUDK18 59
#define IdUDK19 60
#define IdUDK20 61
#define IdXF1	62
#define IdXF2	63
#define IdXF3	64
#define IdXF4	65
#define IdXF5	66
#define IdCmdEditCopy 67
#define IdCmdEditPaste 68
#define IdCmdEditPasteCR 69
#define IdCmdEditCLS 70
#define IdCmdEditCLB 71
#define IdCmdCtrlOpenTEK 72
#define IdCmdCtrlCloseTEK 73
#define IdCmdLineUp 74
#define IdCmdLineDown 75
#define IdCmdPageUp 76
#define IdCmdPageDown 77
#define IdCmdBuffTop 78
#define IdCmdBuffBottom 79
#define IdCmdNextWin 80
#define IdCmdPrevWin 81
#define IdCmdLocalEcho 82
#define IdUser1 83
#define NumOfUserKey 99
#define IdKeyMax IdUser1+NumOfUserKey-1

// key code for macro commands
#define IdCmdDisconnect 1000
#define IdCmdLoadKeyMap 1001
#define IdCmdRestoreSetup 1002

#define KeyStrMax 1023

// (user) key type IDs
#define IdBinary 0  // transmit text without any modification
#define IdText   1  // transmit text with new-line & DBCS conversions
#define IdMacro  2  // activate macro
#define IdCommand 3 // post a WM_COMMAND message

/* Control Characters */

#define NUL 0x00
#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define ENQ 0x05
#define ACK 0x06
#define BEL 0x07
#define BS  0x08
#define HT  0x09
#define LF  0x0A
#define VT  0x0B
#define FF  0x0C
#define CR  0x0D
#define SO  0x0E
#define SI  0x0F
#define DLE 0x10
#define DC1 0x11
	#define XON 0x11
#define DC2 0x12
#define DC3 0x13
	#define XOFF 0x13
#define DC4 0x14
#define NAK 0x15
#define SYN 0x16
#define ETB 0x17
#define CAN 0x18
#define EM  0x19
#define SUB 0x1A
#define ESC 0x1B
#define FS  0x1C
#define GS  0x1D
#define RS  0x1E
#define US  0x1F

#define SP  0x20

#define DEL 0x7F

#define IND 0x84
#define NEL 0x85
#define SSA 0x86
#define ESA 0x87
#define HTS 0x88
#define HTJ 0x89
#define VTS 0x8A
#define PLD 0x8B
#define PLU 0x8C
#define RI  0x8D
#define SS2 0x8E
#define SS3 0x8F
#define DCS 0x90
#define PU1 0x91
#define PU2 0x92
#define STS 0x93
#define CCH 0x94
#define MW  0x95
#define SPA 0x96
#define EPA 0x97
/*#define SOS 0x98*/


#define CSI 0x9B
#define ST  0x9C
#define OSC 0x9D
#define PM  0x9E
#define APC 0x9F

#define InBuffSize 1024
#define OutBuffSize 1024

#define ID_FILE 0
#define ID_EDIT 1
#define ID_SETUP 2
#define ID_CONTROL 3
#define ID_HELPMENU 4
#define ID_WINDOW_1 50801
#define ID_WINDOW_WINDOW 50810

#define ID_TRANSFER 4
#define ID_SHOWMENUBAR 995

#define BuffXMax 300
#define BuffYMax 100000
#define BuffSizeMax 8000000

HDC PrnBox(HWND HWin, LPBOOL Sel);
BOOL PrnStart(LPSTR DocumentName);
void PrnStop();

#define IdPrnCancel 0
#define IdPrnScreen 1
#define IdPrnSelectedText 2
#define IdPrnScrollRegion 4
#define IdPrnFile 8

int VTPrintInit(int PrnFlag);
void PrnSetAttr(BYTE Attr, BYTE Attr2);
void PrnOutText(PCHAR Buff, int Count);
void PrnNewLine();
void VTPrintEnd();

void PrnFileDirectProc();
void PrnFileStart();
void OpenPrnFile();
void ClosePrnFile();
void WriteToPrnFile(BYTE b, BOOL Write);

void InitBuffer();
void LockBuffer();
void UnlockBuffer();
void FreeBuffer();
void BuffReset();
void BuffScroll(int Count, int Bottom);
void BuffInsertSpace(int Count);
void BuffEraseCurToEnd();
void BuffEraseHomeToCur();
void BuffInsertLines(int Count, int YEnd);
void BuffEraseCharsInLine(int XStart, int Count);
void BuffDeleteLines(int Count, int YEnd);
void BuffDeleteChars(int Count);
void BuffEraseChars(int Count);
void BuffFillWithE();
void BuffDrawLine(BYTE Attr, BYTE Attr2, int Direction, int C);
void BuffEraseBox(int XStart, int YStart, int XEnd, int YEnd);
void BuffCBCopy(BOOL Table);
void BuffPrint(BOOL ScrollRegion);
void BuffDumpCurrentLine(BYTE TERM);
void BuffPutChar(BYTE b, BYTE Attr, BYTE Attr2, BOOL Insert);
void BuffPutKanji(WORD w, BYTE Attr, BYTE Attr2, BOOL Insert);
void BuffUpdateRect(int XStart, int YStart, int XEnd, int YEnd);
void UpdateStr();
void MoveCursor(int Xnew, int Ynew);
void MoveRight();
void BuffSetCaretWidth();
void BuffScrollNLines(int n);
void BuffClearScreen();
void BuffUpdateScroll();
void CursorUpWithScroll();
void BuffDblClk(int Xw, int Yw);
void BuffTplClk(int Yw);
void BuffStartSelect(int Xw, int Yw, BOOL Box);
void BuffChangeSelect(int Xw, int Yw, int NClick);
void BuffEndSelect();
void BuffChangeWinSize(int Nx, int Ny);
void BuffChangeTerminalSize(int Nx, int Ny);
void ChangeWin();
void ClearBuffer();
void SetTabStop();
void MoveToNextTab();
void ClearTabStop(int Ps);
void ShowStatusLine(int Show);

PCHAR CBOpen(LONG MemSize);
void CBClose();
void CBStartPaste(HWND HWin, BOOL AddCR,
		  int BuffSize, PCHAR DataPtr, int DataSize);
void CBSend();
void CBEndPaste();

#endif

