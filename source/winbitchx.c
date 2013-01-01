#include <windows.h>
/*#include <mingw32/process.h>*/

/* Seems to have problems getting these function declarations
 * -- going to try createthread manually this time --
 */
/*unsigned long
	_beginthread	(void (*pfuncStart)(void *),
			 unsigned unStackSize, void* pArgList);
			 void	_endthread	();*/


#include "input.h"
#define         WIDTH   10

char *lastclicklinedata = NULL;
int lastclickcol, lastclickrow;

char *codeptr, *paramptr;

int menucmd;

int lastscrollerpos, lastscrollerwindow, newscrollerpos;

int contextx, contexty;

WORD MsgDlgHelp;

HWND HVTWin;

/* Start VT Code */
#define CurWidth 2

int WinWidth, WinHeight;
static BOOL Active = FALSE;
static BOOL CompletelyVisible;
HFONT VTFont[AttrFontMask+1];
int FontHeight, FontWidth, ScreenWidth, ScreenHeight;
BOOL AdjustSize;
BOOL DontChangeSize=FALSE;
static int CRTWidth, CRTHeight;
int CursorX, CursorY;

// --- scrolling status flags
int WinOrgX, WinOrgY, NewOrgX, NewOrgY;

int NumOfLines, NumOfColumns;
int PageStart, BuffEnd;

static BOOL CursorOnDBCS = FALSE;
static LOGFONT VTlf;
static BOOL SaveWinSize = FALSE;
static int WinWidthOld, WinHeightOld;
static HBRUSH Background;
static COLORREF ANSIColor[16];
static int Dx[256];

// caret variables
static int CaretStatus;
static BOOL CaretEnabled = TRUE;

// ---- device context and status flags
static HDC VTDC = NULL; /* Device context for VT window */
static BYTE DCAttr, DCAttr2;
static BOOL DCReverse;
static HFONT DCPrevFont;

// scrolling
static int ScrollCount = 0;
static int dScroll = 0;
static int SRegionTop;
static int SRegionBottom;

// for clipboard copy
static HGLOBAL CBCopyHandle = NULL;
static PCHAR CBCopyPtr = NULL;

// for clipboard paste
static HGLOBAL CBMemHandle = NULL;
static PCHAR CBMemPtr = NULL;
static LONG CBMemPtr2 = 0;
static BOOL CBAddCR = FALSE;
static BYTE CBByte;
static BOOL CBRetrySend;
static BOOL CBRetryEcho;
static BOOL CBSendCR;
static BOOL CBDDE;

/* Everything below here should be screen specific */
/* character attribute */
static BYTE CharAttr, CharAttr2;

/* various modes of VT emulation */
static BOOL RelativeOrgMode;
static BOOL ReverseColor;
static BOOL InsertMode;
static BOOL LFMode;
static BOOL AutoWrapMode;

static BOOL ESCFlag, JustAfterESC;
static BOOL Special;

static int ParseMode;


PCHAR CBOpen(LONG MemSize)
{
	if (MemSize==0) return (NULL);
	if (CBCopyHandle!=NULL) return (NULL);
	CBCopyPtr = NULL;
	CBCopyHandle = GlobalAlloc(GMEM_MOVEABLE, MemSize);
	if (CBCopyHandle == NULL)
		MessageBeep(0);
	else {
		CBCopyPtr = GlobalLock(CBCopyHandle);
		if (CBCopyPtr == NULL)
		{
			GlobalFree(CBCopyHandle);
			CBCopyHandle = NULL;
			MessageBeep(0);
		}
	}
	return (CBCopyPtr);
}

void CBClose()
{
	BOOL Empty;
	if (CBCopyHandle==NULL) return;

	Empty = FALSE;
	if (CBCopyPtr!=NULL)
    Empty = (CBCopyPtr[0]==0);

	GlobalUnlock(CBCopyHandle);
	CBCopyPtr = NULL;

	if (OpenClipboard(HVTWin))
	{
		EmptyClipboard();
		if (! Empty)
			SetClipboardData(CF_TEXT, CBCopyHandle);
		CloseClipboard();
	}
	CBCopyHandle = NULL;
}

void CBStartPaste(HWND HWin, BOOL AddCR,
		  int BuffSize, PCHAR DataPtr, int DataSize)
//
//  DataPtr and DataSize are used only for DDE
//	  For clipboard, BuffSize should be 0
//	  DataSize should be <= BuffSize
{
	UINT Cf = 0;

	CBAddCR = AddCR;

	if (BuffSize==0) // for clipboard
	{
		if (IsClipboardFormatAvailable(CF_TEXT))
			Cf = CF_TEXT;
		else if (IsClipboardFormatAvailable(CF_OEMTEXT))
			Cf = CF_OEMTEXT;
		else return;
	}

	CBMemHandle = NULL;
	CBMemPtr = NULL;
	CBMemPtr2 = 0;
	CBDDE = FALSE;
	if (BuffSize==0) //clipboard
	{
		if (OpenClipboard(HWin))
			CBMemHandle = GetClipboardData(Cf);
	}
	else { // dde
		CBMemHandle = GlobalAlloc(GHND,BuffSize);
		if (CBMemHandle != NULL)
		{
			CBDDE = TRUE;
			CBMemPtr = GlobalLock(CBMemHandle);
			if (CBMemPtr != NULL)
			{
				memcpy(CBMemPtr,DataPtr,DataSize);
				GlobalUnlock(CBMemHandle);
				CBMemPtr=NULL;
			}
		}
	}
	CBRetrySend = FALSE;
	CBRetryEcho = FALSE;
	CBSendCR = FALSE;
}

void CBSend()
{
	int c = 0;
	BOOL EndFlag;

	if (CBMemHandle==NULL) return;

	if (CBRetrySend)
	{
		/*c = CommTextOut(&cv,(PCHAR)&CBByte,1);*/
		CBRetrySend = (c==0);
		if (CBRetrySend) return;
	}

	if (CBRetryEcho)
	{
		/*c = CommTextEcho(&cv,(PCHAR)&CBByte,1);*/
		CBRetryEcho = (c==0);
		if (CBRetryEcho) return;
	}

	CBMemPtr = GlobalLock(CBMemHandle);
	if (CBMemPtr==NULL) return;

	do {
		if (CBSendCR && (CBMemPtr[CBMemPtr2]==0x0a))
			CBMemPtr2++;

		EndFlag = (CBMemPtr[CBMemPtr2]==0);
    if (! EndFlag)
	{
		CBByte = CBMemPtr[CBMemPtr2];
		CBMemPtr2++;
		// Decoding characters which are encoded by MACRO
		//   to support NUL character sending
		//
		//  [encoded character] --> [decoded character]
		//         01 01        -->     00
		//         01 02        -->     01
		if (CBByte==0x01) /* 0x01 from MACRO */
		{
			CBByte = CBMemPtr[CBMemPtr2];
			CBMemPtr2++;
			CBByte = CBByte - 1; // character just after 0x01
		}
	}
	else if (CBAddCR)
	{
		EndFlag = FALSE;
		CBAddCR = FALSE;
		CBByte = 0x0d;
	}
	else {
		CBEndPaste();
		return;
	}

	if (! EndFlag)
	{
		/*c = CommTextOut(&cv,(PCHAR)&CBByte,1);*/
		CBSendCR = (CBByte==0x0D);
		CBRetrySend = (c==0);
		if ((! CBRetrySend))
		{
			/*c = CommTextEcho(&cv,(PCHAR)&CBByte,1);*/
			CBRetryEcho = (c==0);
		}
	}
	else
		c=0;
	}
	while (c>0);

	if (CBMemPtr!=NULL)
	{
		GlobalUnlock(CBMemHandle);
		CBMemPtr=NULL;
	}
}

void CBEndPaste()
{
	if (CBMemHandle!=NULL)
	{
		if (CBMemPtr!=NULL)
			GlobalUnlock(CBMemHandle);
		if (CBDDE)
			GlobalFree(CBMemHandle);
	}
	if (!CBDDE) CloseClipboard();

	CBDDE = FALSE;
	CBMemHandle = NULL;
	CBMemPtr = NULL;
	CBMemPtr2 = 0;
	CBAddCR = FALSE;
}

LRESULT CALLBACK AfxWndProc(HWND handle,UINT mess,WPARAM parm1,LPARAM parm2)
{
	PAINTSTRUCT ps;
	HDC PaintDC;
	int Xs, Ys, Xe, Ye;

	switch(mess) {
#if 0
	case WM_PAINT:

		PaintDC = BeginPaint(handle, &ps);

		PaintWindow(PaintDC,ps.rcPaint,ps.fErase, &Xs,&Ys,&Xe,&Ye);
		LockBuffer();
		BuffUpdateRect(Xs,Ys,Xe,Ye);
		UnlockBuffer();
		DispEndPaint();

		EndPaint(handle, &ps);
		break;
#endif
	default:
		return DefWindowProc(handle,mess,parm1,parm2);
	}
	return 0;
}

void InitDisp()
{
	HDC TmpDC;
	int i;

	TmpDC = GetDC(NULL);

	ANSIColor[IdBack ]   = RGB(  0,  0,  0);
	ANSIColor[IdRed  ]     = RGB(255,  0,  0);
	ANSIColor[IdGreen]     = RGB(  0,255,  0);
	ANSIColor[IdYellow]    = RGB(255,255,  0);
	ANSIColor[IdBlue]      = RGB(  0,  0,255);
	ANSIColor[IdMagenta]   = RGB(255,  0,255);
	ANSIColor[IdCyan]      = RGB(  0,255,255);
	ANSIColor[IdFore ]   = RGB(192,192,192);

	ANSIColor[IdBack+8]    = RGB(127,127,127);
	ANSIColor[IdRed+8]     = RGB(127,  0,  0);
	ANSIColor[IdGreen+8]   = RGB(  0,127,  0);
	ANSIColor[IdYellow+8]	 = RGB(127,127,  0);
	ANSIColor[IdBlue+8]    = RGB(  0,  0,127);
	ANSIColor[IdMagenta+8] = RGB(127,  0,127);
	ANSIColor[IdCyan+8]    = RGB(  0,128,128);
	ANSIColor[IdFore+8]    = RGB(192,192,192);

	for (i = IdBack ; i <= IdFore+8 ; i++)
		ANSIColor[i] = GetNearestColor(TmpDC, ANSIColor[i]);

	/* background paintbrush */
	Background = CreateSolidBrush(RGB(0,0,0));
	/* CRT width & height */
	CRTWidth = GetDeviceCaps(TmpDC,HORZRES);
	CRTHeight = GetDeviceCaps(TmpDC,VERTRES);

	ReleaseDC(NULL, TmpDC);
}

void EndDisp()
{
	int i, j;

	if (VTDC!=NULL) DispReleaseDC();

	/* Delete fonts */
	for (i = 0 ; i <= AttrFontMask; i++)
	{
		for (j = i+1 ; j <= AttrFontMask ; j++)
			if (VTFont[j]==VTFont[i])
				VTFont[j] = 0;
		if (VTFont[i]!=0) DeleteObject(VTFont[i]);
	}

	if (Background!=0)
	{
		DeleteObject(Background);
		Background = 0;
	}
}

void DispReset()
{
  /* Cursor */
	CursorX = 0;
	CursorY = 0;

	/* Scroll status */
	ScrollCount = 0;
	dScroll = 0;

	if (IsCaretOn()) CaretOn();
	DispEnableCaret(TRUE); // enable caret
}

void DispConvWinToScreen
(int Xw, int Yw, int *Xs, int *Ys, LPBOOL Right)
// Converts window coordinate to screen cordinate
//   Xs: horizontal position in window coordinate (pixels)
//   Ys: vertical
//  Output
//	 Xs, Ys: screen coordinate
//   Right: TRUE if the (Xs,Ys) is on the right half of
//			 a character cell.
{
	if (Xs!=NULL)
		*Xs = Xw / FontWidth + WinOrgX;
	*Ys = Yw / FontHeight + WinOrgY;
	if ((Xs!=NULL) && (Right!=NULL))
		*Right = (Xw - (*Xs-WinOrgX)*FontWidth) >= FontWidth/2;
}

void SetLogFont()
{
	memset(&VTlf, 0, sizeof(LOGFONT));
	VTlf.lfWeight = FW_NORMAL;
	VTlf.lfItalic = 0;
	VTlf.lfUnderline = 0;
	VTlf.lfStrikeOut = 0;
	/*VTlf.lfWidth = ts.VTFontSize.x;
	 VTlf.lfHeight = ts.VTFontSize.y;
	 VTlf.lfCharSet = ts.VTFontCharSet;*/
	VTlf.lfOutPrecision  = OUT_CHARACTER_PRECIS;
	VTlf.lfClipPrecision = CLIP_CHARACTER_PRECIS;
	VTlf.lfQuality       = DEFAULT_QUALITY;
	VTlf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
	/*strcpy(VTlf.lfFaceName,ts.VTFont);*/
}

void ChangeFont()
{
	int i, j;
	TEXTMETRIC Metrics;
	HDC TmpDC;

	/* Delete Old Fonts */
	for (i = 0 ; i <= AttrFontMask ; i++)
	{
		for (j = i+1 ; j <= AttrFontMask ; j++)
			if (VTFont[j]==VTFont[i])
				VTFont[j] = 0;
		if (VTFont[i]!=0)
			DeleteObject(VTFont[i]);
	}

	/* Normal Font */
	SetLogFont();
	VTFont[0] = CreateFontIndirect(&VTlf);

	TmpDC = GetDC(HVTWin);

	SelectObject(TmpDC, VTFont[0]);
	GetTextMetrics(TmpDC, &Metrics);
	/*FontWidth = Metrics.tmAveCharWidth + ts.FontDW;
	 FontHeight = Metrics.tmHeight + ts.FontDH;*/

	ReleaseDC(HVTWin,TmpDC);

	/* Underline */
	VTlf.lfUnderline = 1;
	VTFont[AttrUnder] = CreateFontIndirect(&VTlf);

	/* Bold */
	VTlf.lfUnderline = 0;
	VTlf.lfWeight = FW_BOLD;
	VTFont[AttrBold] = CreateFontIndirect(&VTlf);
	/* Bold + Underline */
	VTlf.lfUnderline = 1;
	VTFont[AttrBold | AttrUnder] = CreateFontIndirect(&VTlf);

	/* Special font */
	VTlf.lfWeight = FW_NORMAL;
	VTlf.lfUnderline = 0;
	VTlf.lfWidth = FontWidth + 1; /* adjust width */
	VTlf.lfHeight = FontHeight;
	VTlf.lfCharSet = SYMBOL_CHARSET;

	strcpy(VTlf.lfFaceName,"Tera Special");
	VTFont[AttrSpecial] = CreateFontIndirect(&VTlf);
	VTFont[AttrSpecial | AttrBold] = VTFont[AttrSpecial];
	VTFont[AttrSpecial | AttrUnder] = VTFont[AttrSpecial];
	VTFont[AttrSpecial | AttrBold | AttrUnder] = VTFont[AttrSpecial];

	SetLogFont();

	for (i = 0 ; i <= 255; i++)
		Dx[i] = FontWidth;
}

void ChangeCaret()
{
	UINT T;

	if (! Active) return;
	if (CaretEnabled)
	{
		DestroyCaret();
		CreateCaret(HVTWin, 0, FontWidth, CurWidth);
		CaretStatus = 1;
	}
	CaretOn();
	if (CaretEnabled)
	{
		T = GetCaretBlinkTime() * 2 / 3;
		SetTimer(HVTWin,IdCaretTimer,T,NULL);
	}
}

void CaretOn()
// Turn on the cursor
{
	int CaretX, CaretY;

	if (! Active) return;

	CaretX = (CursorX-WinOrgX)*FontWidth;
	CaretY = (CursorY-WinOrgY)*FontHeight;

	SetCaretPos(CaretX,CaretY);

	while (CaretStatus > 0)
	{
		ShowCaret(HVTWin);
		CaretStatus--;
	}

}

void CaretOff()
{
	if (! Active) return;
	if (CaretStatus == 0)
	{
		HideCaret(HVTWin);
		CaretStatus++;
	}
}

void DispDestroyCaret()
{
	DestroyCaret();
}

BOOL IsCaretOn()
// check if caret is on
{
	return (Active && (CaretStatus==0));
}

void DispEnableCaret(BOOL On)
{
	if (! On) CaretOff();
	CaretEnabled = On;
}

BOOL IsCaretEnabled()
{
	return CaretEnabled;
}

void DispSetCaretWidth(BOOL DW)
{
	/* TRUE if cursor is on a DBCS character */
	CursorOnDBCS = DW;
}

void DispChangeWinSize(int Nx, int Ny)
{
	LONG W,H,dW,dH;
	RECT R;

	if (SaveWinSize)
	{
		WinWidthOld = WinWidth;
		WinHeightOld = WinHeight;
		SaveWinSize = FALSE;
	}
	else {
		WinWidthOld = NumOfColumns;
		WinHeightOld = NumOfLines;
	}

	WinWidth = Nx;
	WinHeight = Ny;

	ScreenWidth = WinWidth*FontWidth;
	ScreenHeight = WinHeight*FontHeight;

	AdjustScrollBar();

	GetWindowRect(HVTWin,&R);
	W = R.right-R.left;
	H = R.bottom-R.top;
	GetClientRect(HVTWin,&R);
	dW = ScreenWidth - R.right + R.left;
	dH = ScreenHeight - R.bottom + R.top;
  
	if ((dW!=0) || (dH!=0))
	{
		AdjustSize = TRUE;
		SetWindowPos(HVTWin,HWND_TOP,0,0,W+dW,H+dH,SWP_NOMOVE);
	}
	else
		InvalidateRect(HVTWin,NULL,FALSE);
}

void ResizeWindow(int x, int y, int w, int h, int cw, int ch)
{
	int dw,dh, NewX, NewY;
	POINT Point;

	if (! AdjustSize) return;
	dw = ScreenWidth - cw;
	dh = ScreenHeight - ch;
	if ((dw!=0) || (dh!=0))
		SetWindowPos(HVTWin,HWND_TOP,x,y,w+dw,h+dh,SWP_NOMOVE);
	else {
		AdjustSize = FALSE;

		NewX = x;
		NewY = y;
		if (x+w > CRTWidth)
		{
			NewX = CRTWidth-w;
			if (NewX < 0) NewX = 0;
		}
		if (y+h > CRTHeight)
		{
			NewY = CRTHeight-h;
			if (NewY < 0) NewY = 0;
		}
		if ((NewX!=x) || (NewY!=y))
			SetWindowPos(HVTWin,HWND_TOP,NewX,NewY,w,h,SWP_NOSIZE);

		Point.x = 0;
		Point.y = ScreenHeight;
		ClientToScreen(HVTWin,&Point);
		CompletelyVisible = (Point.y <= CRTHeight);
		if (IsCaretOn()) CaretOn();
	}
}

void PaintWindow(HDC PaintDC, RECT PaintRect, BOOL fBkGnd,
				 int* Xs, int* Ys, int* Xe, int* Ye)
//  Paint window with background color &
//  convert paint region from window coord. to screen coord.
//  Called from WM_PAINT handler
//    PaintRect: Paint region in window coordinate
//    Return:
//	*Xs, *Ys: upper left corner of the region
//		    in screen coord.
//	*Xe, *Ye: lower right
{
	if (VTDC!=NULL)
	DispReleaseDC();
	VTDC = PaintDC;
	DCPrevFont = SelectObject(VTDC, VTFont[0]);
	DispInitDC();
	if (fBkGnd)
		FillRect(VTDC, &PaintRect,Background);

	*Xs = PaintRect.left / FontWidth + WinOrgX;
	*Ys = PaintRect.top / FontHeight + WinOrgY;
	*Xe = (PaintRect.right-1) / FontWidth + WinOrgX;
	*Ye = (PaintRect.bottom-1) / FontHeight + WinOrgY;
}

void DispEndPaint()
{
	if (VTDC==NULL) return;
	SelectObject(VTDC,DCPrevFont);
	VTDC = NULL;
}

void DispClearWin()
{
	InvalidateRect(HVTWin,NULL,FALSE);

	ScrollCount = 0;
	dScroll = 0;
	if (WinHeight > NumOfLines)
		DispChangeWinSize(NumOfColumns,NumOfLines);
	else {
		/*if ((NumOfLines==WinHeight) && (ts.EnableScrollBuff>0))
		 {
		 SetScrollRange(HVTWin,SB_VERT,0,1,FALSE);
		 }
		 else
		 SetScrollRange(HVTWin,SB_VERT,0,NumOfLines-WinHeight,FALSE);*/

		SetScrollPos(HVTWin,SB_HORZ,0,TRUE);
		SetScrollPos(HVTWin,SB_VERT,0,TRUE);
	}
	if (IsCaretOn()) CaretOn();
}

void DispChangeBackground()
{
	DispReleaseDC();
	if (Background != NULL) DeleteObject(Background);
	Background = CreateSolidBrush(RGB(0,0,0));

	InvalidateRect(HVTWin,NULL,TRUE);
}

void DispChangeWin()
{
	/* Change caret shape */
	ChangeCaret();

	ANSIColor[IdFore ]   = RGB(255,255,255);
	ANSIColor[IdBack ]   = RGB(  0,  0,  0);

	/* change background color */
	DispChangeBackground();
}

void DispInitDC()
{
	if (VTDC==NULL)
	{
		VTDC = GetDC(HVTWin);
		DCPrevFont = SelectObject(VTDC, VTFont[0]);
	}
	else
		SelectObject(VTDC, VTFont[0]);
	SetTextColor(VTDC, RGB(192,192,192));
	SetBkColor(VTDC, RGB(0,0,0));
	SetBkMode(VTDC,OPAQUE);
	DCAttr = AttrDefault;
	DCAttr2 = AttrDefault2;
	DCReverse = FALSE;
}

void DispReleaseDC()
{
	if (VTDC==NULL) return;
	SelectObject(VTDC, DCPrevFont);
	ReleaseDC(HVTWin,VTDC);
	VTDC = NULL;
}

void DispSetupDC(BYTE Attr, BYTE Attr2, BOOL Reverse)
// Setup device context
//   Attr, Attr2: character attribute 1 & 2
//   Reverse: true if text is selected (reversed) by mouse
{
	COLORREF TextColor, BackColor;
	int i, j;

	if (VTDC==NULL)  DispInitDC();

	if ((DCAttr==Attr) && (DCAttr2==Attr2) &&
		(DCReverse==Reverse)) return;
	DCAttr = Attr;
	DCAttr2 = Attr2;
	DCReverse = Reverse;
     
	SelectObject(VTDC, VTFont[Attr & AttrFontMask]);

	if ((Attr2 & Attr2Fore) != 0)
	{
		if ((Attr & AttrBold) != 0)
			i = 0;
		else
			i = 8;
		j = Attr2 & Attr2ForeMask;
		if (j==0)
			j = 8 - i + j;
		else
			j = i + j;
		TextColor = ANSIColor[j];
	}
	/*else if ((Attr & AttrBlink) != 0)
	 BackColor = RGB(127,127,127);
	 TextColor = ts.VTBlinkColor[0];*/
	else if ((Attr & AttrBold) != 0)
		TextColor =  RGB(127,127,255);
	else
		TextColor = RGB(192,192,192);

	if ((Attr2 & Attr2Back) != 0)
	{
		if ((Attr & AttrBlink) != 0)
			i = 0;
		else
			i = 8;
		j = (Attr2 & Attr2BackMask) >> SftAttrBack;
		if (j==0)
			j = 8 - i + j;
		else
			j = i + j;
		BackColor = ANSIColor[j];
	}
	else if ((Attr & AttrBlink) != 0)
		BackColor = RGB(127,127,127);
	/*else if ((Attr & AttrBold) != 0)
	 BackColor = ts.VTBoldColor[1];*/
	else
		BackColor = RGB(0,0,0);

	if (Reverse != ((Attr & AttrReverse) != 0))
	{
		SetTextColor(VTDC,BackColor);
		SetBkColor(  VTDC,TextColor);
	}
	else {
		SetTextColor(VTDC,TextColor);
		SetBkColor(  VTDC,BackColor);
	}
}

void DispANSI(PCHAR Buff)
{
	DispStr(Buff, strlen(Buff), CursorY, CursorX);
}

void DispStr(PCHAR Buff, int Count, int Y, int* X)
// Display a string
//   Buff: points the string
//   Y: vertical position in window cordinate
//  *X: horizontal position
// Return:
//  *X: horizontal position shifted by the width of the string
{
	RECT RText;

	RText.top = Y;
	RText.bottom = Y+FontHeight;
	RText.left = *X;
	RText.right = *X + Count*FontWidth;
	/*ExtTextOut(VTDC,*X+ts.FontDX,Y+ts.FontDY,
	 ETO_CLIPPED | ETO_OPAQUE,
	 &RText,Buff,Count,&Dx[0]);*/
	*X = RText.right;
}

void DispEraseCurToEnd(int YEnd)
{
	RECT R;

	if (VTDC==NULL) DispInitDC();
	R.left = 0;
	R.right = ScreenWidth;
	R.top = (CursorY+1-WinOrgY)*FontHeight;
	R.bottom = (YEnd+1-WinOrgY)*FontHeight;
	FillRect(VTDC,&R,Background);
	R.left = (CursorX-WinOrgX)*FontWidth;
	R.bottom = R.top;
	R.top = R.bottom-FontHeight;
	FillRect(VTDC,&R,Background);
}

void DispEraseHomeToCur(int YHome)
{
	RECT R;

	if (VTDC==NULL) DispInitDC();
	R.left = 0;
	R.right = ScreenWidth;
	R.top = (YHome-WinOrgY)*FontHeight;
	R.bottom = (CursorY-WinOrgY)*FontHeight;
	FillRect(VTDC,&R,Background);
	R.top = R.bottom;
	R.bottom = R.top + FontHeight;
	R.right = (CursorX+1-WinOrgX)*FontWidth;
	FillRect(VTDC,&R,Background);
}

void DispEraseCharsInLine(int XStart, int Count)
{
	RECT R;

	if (VTDC==NULL) DispInitDC();
	R.top = (CursorY-WinOrgY)*FontHeight;
	R.bottom = R.top+FontHeight;
	R.left = (XStart-WinOrgX)*FontWidth;
	R.right = R.left + Count * FontWidth;
	FillRect(VTDC,&R,Background);
}

BOOL DispDeleteLines(int Count, int YEnd)
// return value:
//	 TRUE  - screen is successfully updated
//   FALSE - screen is not updated
{
	RECT R;

	if (Active && CompletelyVisible &&
		(YEnd+1-WinOrgY <= WinHeight))
	{
	R.left = 0;
	R.right = ScreenWidth;
	R.top = (CursorY-WinOrgY)*FontHeight;
	R.bottom = (YEnd+1-WinOrgY)*FontHeight;
	ScrollWindow(HVTWin,0,-FontHeight*Count,&R,&R);
	UpdateWindow(HVTWin);
	return TRUE;
	}
	else
		return FALSE;
}

BOOL DispInsertLines(int Count, int YEnd)
// return value:
//	 TRUE  - screen is successfully updated
//   FALSE - screen is not updated
{
	RECT R;

	if (Active && CompletelyVisible &&
		(CursorY >= WinOrgY))
	{
		R.left = 0;
		R.right = ScreenWidth;
		R.top = (CursorY-WinOrgY)*FontHeight;
		R.bottom = (YEnd+1-WinOrgY)*FontHeight;
		ScrollWindow(HVTWin,0,FontHeight*Count,&R,&R);
		UpdateWindow(HVTWin);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL IsLineVisible(int* X, int* Y)
//  Check the visibility of a line
//	called from UpdateStr()
//    *X, *Y: position of a character in the line. screen coord.
//    Return: TRUE if the line is visible.
//	*X, *Y:
//	  If the line is visible
//	    position of the character in window coord.
//	  Otherwise
//	    no change. same as input value.
{
	if ((dScroll != 0) &&
		(*Y>=SRegionTop) &&
		(*Y<=SRegionBottom))
	{
		*Y = *Y + dScroll;
		if ((*Y<SRegionTop) || (*Y>SRegionBottom))
			return FALSE;
	}

	if ((*Y<WinOrgY) ||
		(*Y>=WinOrgY+WinHeight))
		return FALSE;

	/* screen coordinate -> window coordinate */
	*X = (*X-WinOrgX)*FontWidth;
	*Y = (*Y-WinOrgY)*FontHeight;
	return TRUE;
}

//-------------- scrolling functions --------------------

void AdjustScrollBar() /* called by ChangeWindowSize() */
{
	LONG XRange, YRange;
	int ScrollPosX, ScrollPosY;

	if (NumOfColumns-WinWidth>0)
		XRange = NumOfColumns-WinWidth;
	else
		XRange = 0;

	if (BuffEnd-WinHeight>0)
		YRange = BuffEnd-WinHeight;
	else
		YRange = 0;

	ScrollPosX = GetScrollPos(HVTWin,SB_HORZ);
	ScrollPosY = GetScrollPos(HVTWin,SB_VERT);
	if (ScrollPosX > XRange)
		ScrollPosX = XRange;
	if (ScrollPosY > YRange)
    ScrollPosY = YRange;

	WinOrgX = ScrollPosX;
	WinOrgY = ScrollPosY-PageStart;
	NewOrgX = WinOrgX;
	NewOrgY = WinOrgY;

	DontChangeSize = TRUE;

	SetScrollRange(HVTWin,SB_HORZ,0,XRange,FALSE);

	/*if ((YRange == 0) && (ts.EnableScrollBuff>0))
	 {
	 SetScrollRange(HVTWin,SB_VERT,0,1,FALSE);
	 }
	 else {
	 SetScrollRange(HVTWin,SB_VERT,0,YRange,FALSE);
	 }

	 SetScrollPos(HVTWin,SB_HORZ,ScrollPosX,TRUE);
	 SetScrollPos(HVTWin,SB_VERT,ScrollPosY,TRUE);*/

	DontChangeSize = FALSE;
}

void DispScrollToCursor(int CurX, int CurY)
{
	if (CurX < NewOrgX)
		NewOrgX = CurX;
	else if (CurX >= NewOrgX+WinWidth)
		NewOrgX = CurX + 1 - WinWidth;

	if (CurY < NewOrgY)
		NewOrgY = CurY;
	else if (CurY >= NewOrgY+WinHeight)
		NewOrgY = CurY + 1 - WinHeight;
}

void DispScrollNLines(int Top, int Bottom, int Direction)
//  Scroll a region of the window by Direction lines
//    updates window if necessary
//  Top: top line of scroll region
//  Bottom: bottom line
//  Direction: +: forward, -: backward
{
	if (((dScroll*Direction <0) ||
		 (dScroll*Direction >0)) &&
		((SRegionTop!=Top) ||
		 (SRegionBottom!=Bottom)))
		DispUpdateScroll();
	SRegionTop = Top;
	SRegionBottom = Bottom;
	dScroll = dScroll + Direction;
	if (Direction>0)
		DispCountScroll(Direction);
	else
		DispCountScroll(-Direction);
}

void DispCountScroll(int n)
{
	ScrollCount = ScrollCount + n;
	/*if (ScrollCount>=ts.ScrollThreshold) DispUpdateScroll();*/
}

void DispUpdateScroll()
{
	int d;
	RECT R;

	ScrollCount = 0;

	/* Update partial scroll */
	if (dScroll != 0)
	{
		d = dScroll * FontHeight;
		R.left = 0;
		R.right = ScreenWidth;
		R.top = (SRegionTop-WinOrgY)*FontHeight;
		R.bottom = (SRegionBottom+1-WinOrgY)*FontHeight;
		ScrollWindow(HVTWin,0,-d,&R,&R);
		/* if ((SRegionTop==0) && (dScroll>0))
		 { // update scroll bar if BuffEnd is changed
		 if ((BuffEnd==WinHeight) &&
		 (ts.EnableScrollBuff>0))
		 SetScrollRange(HVTWin,SB_VERT,0,1,TRUE);
		 else
		 SetScrollRange(HVTWin,SB_VERT,0,BuffEnd-WinHeight,FALSE);
		 SetScrollPos(HVTWin,SB_VERT,WinOrgY+PageStart,TRUE);
		 }*/
		dScroll = 0;
	}

	/* Update normal scroll */
	if (NewOrgX < 0) NewOrgX = 0;
	if (NewOrgX>NumOfColumns-WinWidth)
		NewOrgX = NumOfColumns-WinWidth;
	if (NewOrgY < -PageStart) NewOrgY = -PageStart;
	if (NewOrgY>BuffEnd-WinHeight-PageStart)
		NewOrgY = BuffEnd-WinHeight-PageStart;

	if ((NewOrgX==WinOrgX) &&
		(NewOrgY==WinOrgY)) return;

	if (NewOrgX==WinOrgX)
	{
		d = (NewOrgY-WinOrgY) * FontHeight;
		ScrollWindow(HVTWin,0,-d,NULL,NULL);
	}
	else if (NewOrgY==WinOrgY)
	{
		d = (NewOrgX-WinOrgX) * FontWidth;
		ScrollWindow(HVTWin,-d,0,NULL,NULL);
	}
	else
		InvalidateRect(HVTWin,NULL,TRUE);

	/* Update scroll bars */
	if (NewOrgX!=WinOrgX)
		SetScrollPos(HVTWin,SB_HORZ,NewOrgX,TRUE);

	if (NewOrgY!=WinOrgY)
	{
		/* if ((BuffEnd==WinHeight) &&
		 (ts.EnableScrollBuff>0))
		 SetScrollRange(HVTWin,SB_VERT,0,1,TRUE);
		 else
		 SetScrollRange(HVTWin,SB_VERT,0,BuffEnd-WinHeight,FALSE);
		 SetScrollPos(HVTWin,SB_VERT,NewOrgY+PageStart,TRUE);*/
	}

	WinOrgX = NewOrgX;
	WinOrgY = NewOrgY;

	if (IsCaretOn()) CaretOn();
}

void DispScrollHomePos()
{
	NewOrgX = 0;
	NewOrgY = 0;
	DispUpdateScroll();
}

void DispAutoScroll(POINT p)
{
	int X, Y;

	X = (p.x + FontWidth / 2) / FontWidth;
	Y = p.y / FontHeight;
	if (X<0)
		NewOrgX = WinOrgX + X;
	else if (X>=WinWidth)
		NewOrgX = NewOrgX + X - WinWidth + 1;
	if (Y<0)
		NewOrgY = WinOrgY + Y;
	else if (Y>=WinHeight)
		NewOrgY = NewOrgY + Y - WinHeight + 1;

	DispUpdateScroll();
}

void DispHScroll(int Func, int Pos)
{
	switch (Func) {
	case SCROLL_BOTTOM:
		NewOrgX = NumOfColumns-WinWidth;
		break;
	case SCROLL_LINEDOWN: NewOrgX = WinOrgX + 1; break;
	case SCROLL_LINEUP: NewOrgX = WinOrgX - 1; break;
	case SCROLL_PAGEDOWN:
		NewOrgX = WinOrgX + WinWidth - 1;
		break;
	case SCROLL_PAGEUP:
		NewOrgX = WinOrgX - WinWidth + 1;
		break;
	case SCROLL_POS: NewOrgX = Pos; break;
	case SCROLL_TOP: NewOrgX = 0; break;
	}
	DispUpdateScroll();
}

void DispVScroll(int Func, int Pos)
{
	switch (Func) {
	case SCROLL_BOTTOM:
		NewOrgY = BuffEnd-WinHeight-PageStart;
		break;
	case SCROLL_LINEDOWN: NewOrgY = WinOrgY + 1; break;
	case SCROLL_LINEUP: NewOrgY = WinOrgY - 1; break;
	case SCROLL_PAGEDOWN:
		NewOrgY = WinOrgY + WinHeight - 1;
		break;
	case SCROLL_PAGEUP:
		NewOrgY = WinOrgY - WinHeight + 1;
		break;
	case SCROLL_POS: NewOrgY = Pos-PageStart; break;
	case SCROLL_TOP: NewOrgY = -PageStart; break;
	}
	DispUpdateScroll();
}

//-------------- end of scrolling functions --------

void DispSetupFontDlg()
//  Popup the Setup Font dialogbox and
//  reset window
{
	BOOL Ok = 0;

	/*(if (! LoadTTDLG()) return;
	 SetLogFont();
	 Ok = ChooseFontDlg(HVTWin,&VTlf,&ts);
	 FreeTTDLG(); */
	if (! Ok) return;

	/*strcpy(ts.VTFont,VTlf.lfFaceName);
	 ts.VTFontSize.x = VTlf.lfWidth;
	 ts.VTFontSize.y = VTlf.lfHeight;
	 ts.VTFontCharSet = VTlf.lfCharSet;*/

	ChangeFont();

	DispChangeWinSize(WinWidth,WinHeight);

	ChangeCaret();
}

void DispRestoreWinSize()
//  Restore window size by double clik on caption bar
{
	if ((WinWidth==NumOfColumns) && (WinHeight==NumOfLines))
	{
		if (WinWidthOld > NumOfColumns)
			WinWidthOld = NumOfColumns;
		if (WinHeightOld > BuffEnd)
			WinHeightOld = BuffEnd;
		DispChangeWinSize(WinWidthOld,WinHeightOld);
	}
	else {
		SaveWinSize = TRUE;
		DispChangeWinSize(NumOfColumns,NumOfLines);
	}
}

void DispSetWinPos()
{
	POINT Point;
	RECT R;

	GetWindowRect(HVTWin,&R);

	Point.x = 0;
	Point.y = ScreenHeight;
	ClientToScreen(HVTWin,&Point);
	CompletelyVisible = (Point.y <= CRTHeight);
}

void DispSetActive(BOOL ActiveFlag)
{
	Active = ActiveFlag;
	if (Active)
	{
		SetFocus(HVTWin);
	}
}
/* End VT code */
/* Start Scrollback code */
// status line
int StatusLine;	//0: none 1: shown
/* top & bottom margin */
int CursorTop, CursorBottom;
BOOL Selected;
BOOL Wrap;

static WORD TabStops[256];
static int NTabStops;

static WORD BuffLock = 0;
static HANDLE HCodeBuff = 0;
static HANDLE HAttrBuff = 0;
static HANDLE HAttrBuff2 = 0;

static PCHAR CodeBuff;  /* Character code buffer */
static PCHAR AttrBuff;  /* Attribute buffer */
static PCHAR AttrBuff2; /* Color attr buffer */
static PCHAR CodeLine;
static PCHAR AttrLine;
static PCHAR AttrLine2;
static LONG LinePtr;
static LONG BufferSize;
static int NumOfLinesInBuff;
static int BuffStartAbs, BuffEndAbs;
static POINT SelectStart, SelectEnd, SelectEndOld;
static BOOL BoxSelect;
static POINT DblClkStart, DblClkEnd;

static int StrChangeStart, StrChangeCount;

LONG GetLinePtr(int Line)
{
	LONG Ptr;

	Ptr = (LONG)(BuffStartAbs + Line) *
		(LONG)(NumOfColumns);
	while (Ptr>=BufferSize)
		Ptr = Ptr - BufferSize;
	return Ptr;
}

LONG NextLinePtr(LONG Ptr)
{
	Ptr = Ptr + (LONG)NumOfColumns;
	if (Ptr >= BufferSize)
		Ptr = Ptr - BufferSize;
	return Ptr;
}

LONG PrevLinePtr(LONG Ptr)
{
	Ptr = Ptr - (LONG)NumOfColumns;
	if (Ptr < 0) Ptr = Ptr + BufferSize;
	return Ptr;
}

BOOL ChangeBuffer(int Nx, int Ny)
{
	HANDLE HCodeNew, HAttrNew, HAttr2New;
	LONG NewSize;
	int NxCopy, NyCopy, i;
	PCHAR CodeDest, AttrDest, AttrDest2;
	PCHAR Ptr;
	LONG SrcPtr, DestPtr;
	WORD LockOld;

	if (Nx > BuffXMax) Nx = BuffXMax;
	/*if (ts.ScrollBuffMax > BuffYMax)
	 ts.ScrollBuffMax = BuffYMax;
	 if (Ny > ts.ScrollBuffMax) Ny = ts.ScrollBuffMax;*/

	if ( (LONG)Nx * (LONG)Ny > BuffSizeMax )
		Ny = BuffSizeMax / Nx;

	NewSize = (LONG)Nx * (LONG)Ny;

	HCodeNew = GlobalAlloc(GMEM_MOVEABLE, NewSize);
	if ( HCodeNew==0 ) return FALSE;
	Ptr = GlobalLock(HCodeNew);
	if ( Ptr==NULL )
	{
		GlobalFree(HCodeNew);
		return FALSE;
	}
	CodeDest = Ptr;

	HAttrNew = GlobalAlloc(GMEM_MOVEABLE, NewSize);
	if ( HAttrNew==0 )
	{
		GlobalFree(HCodeNew);
		return FALSE;
	}
	Ptr = GlobalLock(HAttrNew);
	if ( Ptr==NULL )
	{
		GlobalFree(HCodeNew);
		GlobalFree(HAttrNew);
		return FALSE;
	}
	AttrDest = Ptr;

	HAttr2New = GlobalAlloc(GMEM_MOVEABLE, NewSize);
	if ( HAttr2New==0 )
	{
		GlobalFree(HCodeNew);
		GlobalFree(HAttrNew);
		return FALSE;
	}
	Ptr = GlobalLock(HAttr2New);
	if ( Ptr==NULL )
	{
		GlobalFree(HCodeNew);
		GlobalFree(HAttrNew);
		GlobalFree(HAttr2New);
		return FALSE;
	}
	AttrDest2 = Ptr;

	memset(&CodeDest[0], 0x20, NewSize);
	memset(&AttrDest[0], AttrDefault, NewSize);
	memset(&AttrDest2[0], AttrDefault2, NewSize);
	if ( HCodeBuff!=0 )
	{
		if ( NumOfColumns > Nx )
			NxCopy = Nx;
		else NxCopy = NumOfColumns;
		if ( BuffEnd > Ny )
			NyCopy = Ny;
		else NyCopy = BuffEnd;
		LockOld = BuffLock;
		LockBuffer();
		SrcPtr = GetLinePtr(BuffEnd-NyCopy);
		DestPtr = 0;
		for (i = 1 ; i <= NyCopy ; i++)
		{
			memcpy(&CodeDest[DestPtr],&CodeBuff[SrcPtr],NxCopy);
			memcpy(&AttrDest[DestPtr],&AttrBuff[SrcPtr],NxCopy);
			memcpy(&AttrDest2[DestPtr],&AttrBuff2[SrcPtr],NxCopy);
			SrcPtr = NextLinePtr(SrcPtr);
			DestPtr = DestPtr + (LONG)Nx;
		}
		FreeBuffer();
	}
	else {
		LockOld = 0;
		NyCopy = NumOfLines;
		Selected = FALSE;
	}

	if (Selected)
	{
		SelectStart.y =
			SelectStart.y - BuffEnd + NyCopy;
		SelectEnd.y =
			SelectEnd.y - BuffEnd + NyCopy;
		if (SelectStart.y < 0)
		{
			SelectStart.y = 0;
			SelectStart.x = 0;
		}
		if (SelectEnd.y<0)
		{
			SelectEnd.x = 0;
			SelectEnd.y = 0;
		}

		Selected = (SelectEnd.y > SelectStart.y) ||
			((SelectEnd.y=SelectStart.y) &&
			 (SelectEnd.x > SelectStart.x));
	}

	HCodeBuff = HCodeNew;
	HAttrBuff = HAttrNew;
	HAttrBuff2 = HAttr2New;
	BufferSize = NewSize;
	NumOfLinesInBuff = Ny;
	BuffStartAbs = 0;
	BuffEnd = NyCopy;

	if (BuffEnd==NumOfLinesInBuff)
		BuffEndAbs = 0;
	else
		BuffEndAbs = BuffEnd;

	PageStart = BuffEnd - NumOfLines;

	LinePtr = 0;
	if (LockOld>0)
	{
		CodeBuff = (PCHAR)GlobalLock(HCodeBuff);
		AttrBuff = (PCHAR)GlobalLock(HAttrBuff);
		AttrBuff2 = (PCHAR)GlobalLock(HAttrBuff2);
		CodeLine = CodeBuff;
		AttrLine = AttrBuff;
		AttrLine2 = AttrBuff2;
	}
	else {
		GlobalUnlock(HCodeNew);
		GlobalUnlock(HAttrNew);
	}
	BuffLock = LockOld;

	return TRUE;
}

void InitBuffer()
{
	int Ny;

	/* setup terminal */
	NumOfColumns = 80;
	NumOfLines = 25;

	Ny = NumOfLines;

	if (! ChangeBuffer(NumOfColumns,Ny))
		PostQuitMessage(0);

	StatusLine = 0;
}

void ResetTerminal() /*reset variables but don't update screen */
{
	DispReset();
	BuffReset();

	/* Attribute */
	CharAttr = AttrDefault;
	CharAttr2 = AttrDefault2;
	Special = FALSE;

	/* Various modes */
	InsertMode = FALSE;
	LFMode = IdCRLF;
	AutoWrapMode = TRUE;
	RelativeOrgMode = FALSE;
	ReverseColor = FALSE;

	/* ESC flag for device control sequence */
	ESCFlag = FALSE;
	/* for TEK sequence */
	JustAfterESC = FALSE;

	/* Parse mode */
	ParseMode = ModeFirst;
}

void NewLine(int Line)
{
	LinePtr = GetLinePtr(Line);
	CodeLine = &CodeBuff[LinePtr];
	AttrLine = &AttrBuff[LinePtr];
	AttrLine2 = &AttrBuff2[LinePtr];
}

void LockBuffer()
{
	BuffLock++;
	if (BuffLock>1) return;
	CodeBuff = (PCHAR)GlobalLock(HCodeBuff);
	AttrBuff = (PCHAR)GlobalLock(HAttrBuff);
	AttrBuff2 = (PCHAR)GlobalLock(HAttrBuff2);
	NewLine(PageStart+CursorY);
}

void UnlockBuffer()
{
	if (BuffLock==0) return;
	BuffLock--;
	if (BuffLock>0) return;
	if (HCodeBuff!=NULL)
		GlobalUnlock(HCodeBuff);
	if (HAttrBuff!=NULL)
		GlobalUnlock(HAttrBuff);
	if (HAttrBuff2!=NULL)
		GlobalUnlock(HAttrBuff2);
}

void FreeBuffer()
{
	BuffLock = 1;
	UnlockBuffer();
	if (HCodeBuff!=NULL)
	{
		GlobalFree(HCodeBuff);
		HCodeBuff = NULL;
	}
	if (HAttrBuff!=NULL)
	{
		GlobalFree(HAttrBuff);
		HAttrBuff = NULL;
	}
	if (HAttrBuff2!=NULL)
	{
		GlobalFree(HAttrBuff2);
		HAttrBuff2 = NULL;
	}
}

void BuffReset()
// Reset buffer status. don't update real display
//   called by ResetTerminal()
{
	int i;

	/* Cursor */
	NewLine(PageStart);
	WinOrgX = 0;
	WinOrgY = 0;
	NewOrgX = 0;
	NewOrgY = 0;

	/* Top/bottom margin */
	CursorTop = 0;
	CursorBottom = NumOfLines-1;

	/* Tab stops */
	NTabStops = (NumOfColumns-1) >> 3;
	for (i=1 ; i<=NTabStops ; i++)
		TabStops[i-1] = i*8;

	/* Initialize text selection region */
	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd = SelectStart;
	SelectEndOld = SelectStart;
	Selected = FALSE;

	StrChangeCount = 0;
	Wrap = FALSE;
	StatusLine = 0;
}

void BuffScroll(int Count, int Bottom)
{
	int i, n;
	LONG SrcPtr, DestPtr;
	int BuffEndOld;

	if (Count>NumOfLinesInBuff)
		Count = NumOfLinesInBuff;

	DestPtr = GetLinePtr(PageStart+NumOfLines-1+Count);
	n = Count;
	if (Bottom<NumOfLines-1)
	{
		SrcPtr = GetLinePtr(PageStart+NumOfLines-1);
		for (i=NumOfLines-1; i>=Bottom+1; i--)
		{
			memcpy(&(CodeBuff[DestPtr]),&(CodeBuff[SrcPtr]),NumOfColumns);
			memcpy(&(AttrBuff[DestPtr]),&(AttrBuff[SrcPtr]),NumOfColumns);
			memcpy(&(AttrBuff2[DestPtr]),&(AttrBuff2[SrcPtr]),NumOfColumns);
			memset(&(CodeBuff[SrcPtr]),0x20,NumOfColumns);
			memset(&(AttrBuff[SrcPtr]),AttrDefault,NumOfColumns);
			memset(&(AttrBuff2[SrcPtr]),AttrDefault2,NumOfColumns);
			SrcPtr = PrevLinePtr(SrcPtr);
			DestPtr = PrevLinePtr(DestPtr);
			n--;
		}
	}
	for (i = 1 ; i <= n ; i++)
	{
		memset(&CodeBuff[DestPtr],0x20,NumOfColumns);
		memset(&AttrBuff[DestPtr],AttrDefault,NumOfColumns);
    memset(&AttrBuff2[DestPtr],AttrDefault2,NumOfColumns);
	DestPtr = PrevLinePtr(DestPtr);
	}

	BuffEndAbs = BuffEndAbs + Count;
	if (BuffEndAbs >= NumOfLinesInBuff)
		BuffEndAbs = BuffEndAbs - NumOfLinesInBuff;
	BuffEndOld = BuffEnd;
	BuffEnd = BuffEnd + Count;
	if (BuffEnd >= NumOfLinesInBuff)
	{
		BuffEnd = NumOfLinesInBuff;
		BuffStartAbs = BuffEndAbs;
	}
	PageStart = BuffEnd-NumOfLines;

	if (Selected)
	{
		SelectStart.y = SelectStart.y - Count + BuffEnd - BuffEndOld;
		SelectEnd.y = SelectEnd.y - Count + BuffEnd - BuffEndOld;
		if ( SelectStart.y<0 )
    {
		SelectStart.x = 0;
		SelectStart.y = 0;
	}
		if ( SelectEnd.y<0 )
		{
			SelectEnd.x = 0;
			SelectEnd.y = 0;
		}
		Selected =
			(SelectEnd.y > SelectStart.y) ||
			((SelectEnd.y==SelectStart.y) &&
			 (SelectEnd.x > SelectStart.x));
	}

	NewLine(PageStart+CursorY);
}

void NextLine()
{
	LinePtr = NextLinePtr(LinePtr);
	CodeLine = &CodeBuff[LinePtr];
	AttrLine = &AttrBuff[LinePtr];
	AttrLine2 = &AttrBuff2[LinePtr];
}

void PrevLine()
{
	LinePtr = PrevLinePtr(LinePtr);
	CodeLine = &CodeBuff[LinePtr];
	AttrLine = &AttrBuff[LinePtr];
	AttrLine2 = &AttrBuff2[LinePtr];
}

void BuffInsertSpace(int Count)
// Insert space characters at the current position
//   Count: Number of characters to be inserted
{
	NewLine(PageStart+CursorY);

	if (Count > NumOfColumns - CursorX)
		Count = NumOfColumns - CursorX;

	memmove(&(CodeLine[CursorX+Count]),&(CodeLine[CursorX]),
			NumOfColumns-Count-CursorX);
	memmove(&(AttrLine[CursorX+Count]),&(AttrLine[CursorX]),
			NumOfColumns-Count-CursorX);
	memmove(&(AttrLine2[CursorX+Count]),&(AttrLine2[CursorX]),
			NumOfColumns-Count-CursorX);
	memset(&(CodeLine[CursorX]),0x20,Count);
	memset(&(AttrLine[CursorX]),AttrDefault,Count);
	memset(&(AttrLine2[CursorX]),AttrDefault2,Count);
	/* last char in current line is kanji first? */
	if ((AttrLine[NumOfColumns-1] & AttrKanji) != 0)
	{
		/* then delete it */
		CodeLine[NumOfColumns-1] = 0x20;
		AttrLine[NumOfColumns-1] = AttrDefault;
		AttrLine2[NumOfColumns-1] = AttrDefault2;
	}
	BuffUpdateRect(CursorX,CursorY,NumOfColumns-1,CursorY);
}

void BuffEraseCurToEnd()
// Erase characters from cursor to the end of screen
{
	LONG TmpPtr;
	int offset;
	int i, YEnd;

	NewLine(PageStart+CursorY);
	offset = CursorX;
	TmpPtr = GetLinePtr(PageStart+CursorY);
	YEnd = NumOfLines-1;
	if ((StatusLine>0) &&
		(CursorY<NumOfLines-1))
		YEnd--;
	for (i = CursorY ; i <= YEnd ; i++)
	{
		memset(&(CodeBuff[TmpPtr+offset]),0x20,NumOfColumns-offset);
		memset(&(AttrBuff[TmpPtr+offset]),AttrDefault,NumOfColumns-offset);
		memset(&(AttrBuff2[TmpPtr+offset]),AttrDefault2,NumOfColumns-offset);
		offset = 0;
		TmpPtr = NextLinePtr(TmpPtr);
	}
	/* update window */
	DispEraseCurToEnd(YEnd);
}

void BuffEraseHomeToCur()
// Erase characters from home to cursor
{
	LONG TmpPtr;
	int offset;
	int i, YHome;

	NewLine(PageStart+CursorY);
	offset = NumOfColumns;
	if ((StatusLine>0) && (CursorY==NumOfLines-1))
		YHome = CursorY;
	else
		YHome = 0;
	TmpPtr = GetLinePtr(PageStart+YHome);
	for (i = YHome ; i <= CursorY ; i++)
	{
		if (i==CursorY) offset = CursorX+1;
		memset(&(CodeBuff[TmpPtr]),0x20,offset);
		memset(&(AttrBuff[TmpPtr]),AttrDefault,offset);
		memset(&(AttrBuff2[TmpPtr]),AttrDefault2,offset);
		TmpPtr = NextLinePtr(TmpPtr);
	}

	/* update window */
	DispEraseHomeToCur(YHome);
}

void BuffInsertLines(int Count, int YEnd)
// Insert lines at current position
//   Count: number of lines to be inserted
//   YEnd: bottom line number of scroll region (screen coordinate)
{
	int i;
	LONG SrcPtr, DestPtr;

	BuffUpdateScroll();

	SrcPtr = GetLinePtr(PageStart+YEnd-Count);
	DestPtr = GetLinePtr(PageStart+YEnd);
	for (i= YEnd-Count ; i>=CursorY ; i--)
	{
		memcpy(&(CodeBuff[DestPtr]),&(CodeBuff[SrcPtr]),NumOfColumns);
		memcpy(&(AttrBuff[DestPtr]),&(AttrBuff[SrcPtr]),NumOfColumns);
		memcpy(&(AttrBuff2[DestPtr]),&(AttrBuff2[SrcPtr]),NumOfColumns);
		SrcPtr = PrevLinePtr(SrcPtr);
		DestPtr = PrevLinePtr(DestPtr);
	}
	for (i = 1 ; i <= Count ; i++)
	{
		memset(&(CodeBuff[DestPtr]),0x20,NumOfColumns);
		memset(&(AttrBuff[DestPtr]),AttrDefault,NumOfColumns);
		memset(&(AttrBuff2[DestPtr]),AttrDefault2,NumOfColumns);
		DestPtr = PrevLinePtr(DestPtr);
	}

	if (! DispInsertLines(Count,YEnd))
		BuffUpdateRect(WinOrgX,CursorY,WinOrgX+WinWidth-1,YEnd);
}

void BuffEraseCharsInLine(int XStart, int Count)
// erase characters in the current line
//  XStart: start position of erasing
//  Count: number of characters to be erased
{
	NewLine(PageStart+CursorY);
	memset(&(CodeLine[XStart]),0x20,Count);
	memset(&(AttrLine[XStart]),AttrDefault,Count);
	memset(&(AttrLine2[XStart]),AttrDefault2,Count);

	DispEraseCharsInLine(XStart, Count);
}

void BuffDeleteLines(int Count, int YEnd)
// Delete lines from current line
//   Count: number of lines to be deleted
//   YEnd: bottom line number of scroll region (screen coordinate)
{
	int i;
	LONG SrcPtr, DestPtr;

	BuffUpdateScroll();

	SrcPtr = GetLinePtr(PageStart+CursorY+Count);
	DestPtr = GetLinePtr(PageStart+CursorY);
	for (i=CursorY ; i<= YEnd-Count ; i++)
	{
		memcpy(&(CodeBuff[DestPtr]),&(CodeBuff[SrcPtr]),NumOfColumns);
		memcpy(&(AttrBuff[DestPtr]),&(AttrBuff[SrcPtr]),NumOfColumns);
		memcpy(&(AttrBuff2[DestPtr]),&(AttrBuff2[SrcPtr]),NumOfColumns);
		SrcPtr = NextLinePtr(SrcPtr);
		DestPtr = NextLinePtr(DestPtr);
	}
	for (i = YEnd+1-Count ; i<=YEnd ; i++)
	{
		memset(&(CodeBuff[DestPtr]),0x20,NumOfColumns);
		memset(&(AttrBuff[DestPtr]),AttrDefault,NumOfColumns);
		memset(&(AttrBuff2[DestPtr]),AttrDefault2,NumOfColumns);
		DestPtr = NextLinePtr(DestPtr);
	}

	if (! DispDeleteLines(Count,YEnd))
		BuffUpdateRect(WinOrgX,CursorY,WinOrgX+WinWidth-1,YEnd);
}

void BuffDeleteChars(int Count)
// Delete characters in current line from cursor
//   Count: number of characters to be deleted
{
	NewLine(PageStart+CursorY);

	if (Count > NumOfColumns-CursorX) Count = NumOfColumns-CursorX;
	memmove(&(CodeLine[CursorX]),&(CodeLine[CursorX+Count]),
			NumOfColumns-Count-CursorX);
	memmove(&(AttrLine[CursorX]),&(AttrLine[CursorX+Count]),
			NumOfColumns-Count-CursorX);
	memmove(&(AttrLine2[CursorX]),&(AttrLine2[CursorX+Count]),
			NumOfColumns-Count-CursorX);
	memset(&(CodeLine[NumOfColumns-Count]),0x20,Count);
	memset(&(AttrLine[NumOfColumns-Count]),AttrDefault,Count);
	memset(&(AttrLine2[NumOfColumns-Count]),AttrDefault2,Count);

	BuffUpdateRect(CursorX,CursorY,WinOrgX+WinWidth-1,CursorY);
}

void BuffEraseChars(int Count)
// Erase characters in current line from cursor
//   Count: number of characters to be deleted
{
	NewLine(PageStart+CursorY);

	if (Count > NumOfColumns-CursorX) Count = NumOfColumns-CursorX;
	memset(&(CodeLine[CursorX]),0x20,Count);
	memset(&(AttrLine[CursorX]),AttrDefault,Count);
	memset(&(AttrLine2[CursorX]),AttrDefault2,Count);

	/* update window */
	DispEraseCharsInLine(CursorX,Count);
}

void BuffFillWithE()
// Fill screen with 'E' characters
{
	LONG TmpPtr;
	int i;

	TmpPtr = GetLinePtr(PageStart);
	for (i = 0 ; i <= NumOfLines-1-StatusLine ; i++)
	{
		memset(&(CodeBuff[TmpPtr]),'E',NumOfColumns);
		memset(&(AttrBuff[TmpPtr]),AttrDefault,NumOfColumns);
		memset(&(AttrBuff2[TmpPtr]),AttrDefault2,NumOfColumns);
		TmpPtr = NextLinePtr(TmpPtr);
	}
	BuffUpdateRect(WinOrgX,WinOrgY,WinOrgX+WinWidth-1,WinOrgY+WinHeight-1);
}

void BuffDrawLine(BYTE Attr, BYTE Attr2, int Direction, int C)
{ // IO-8256 terminal
	LONG Ptr;
	int i, X, Y;

	if (C==0) return;
	Attr = Attr | AttrSpecial;

	switch (Direction) {
	case 3:
	case 4:
		if (Direction==3)
		{
			if (CursorY==0) return;
			Y = CursorY-1;
		}
		else {
			if (CursorY==NumOfLines-1-StatusLine) return;
			Y = CursorY+1;
		}
		if (CursorX+C > NumOfColumns)
			C = NumOfColumns-CursorX;
		Ptr = GetLinePtr(PageStart+Y);
		memset(&(CodeBuff[Ptr+CursorX]),'q',C);
		memset(&(AttrBuff[Ptr+CursorX]),Attr,C);
		memset(&(AttrBuff2[Ptr+CursorX]),Attr2,C);
		BuffUpdateRect(CursorX,Y,CursorX+C-1,Y);
		break;
	case 5:
	case 6:
		if (Direction==5)
		{
			if (CursorX==0) return;
			X = CursorX - 1;
		}
		else {
			if (CursorX==NumOfColumns-1)
				X = CursorX-1;
			else
				X = CursorX+1;
		}
		Ptr = GetLinePtr(PageStart+CursorY);
		if (CursorY+C > NumOfLines-StatusLine)
			C = NumOfLines-StatusLine-CursorY;
		for (i=1; i<=C; i++)
		{
			CodeBuff[Ptr+X] = 'x';
			AttrBuff[Ptr+X] = Attr;
			AttrBuff2[Ptr+X] = Attr2;
			Ptr = NextLinePtr(Ptr);
		}
		BuffUpdateRect(X,CursorY,X,CursorY+C-1);
		break;
	}
}

void BuffEraseBox
(int XStart, int YStart, int XEnd, int YEnd)
// IO-8256 terminal
{
	int C, i;
	LONG Ptr;

	if (XEnd>NumOfColumns-1)
		XEnd = NumOfColumns-1;
	if (YEnd>NumOfLines-1-StatusLine)
		YEnd = NumOfLines-1-StatusLine;
	if (XStart>XEnd) return;
	if (YStart>YEnd) return;
	C = XEnd-XStart+1;
	Ptr = GetLinePtr(PageStart+YStart);
	for (i=YStart; i<=YEnd; i++)
	{
		if ((XStart>0) &&
			((AttrBuff[Ptr+XStart-1] & AttrKanji) != 0))
		{
			CodeBuff[Ptr+XStart-1] = 0x20;
			AttrBuff[Ptr+XStart-1] = AttrDefault;
			AttrBuff2[Ptr+XStart-1] = AttrDefault2;
		}
		if ((XStart+C<NumOfColumns) &&
			((AttrBuff[Ptr+XStart+C-1] & AttrKanji) != 0))
		{
			CodeBuff[Ptr+XStart+C] = 0x20;
			AttrBuff[Ptr+XStart+C] = AttrDefault;
			AttrBuff2[Ptr+XStart+C] = AttrDefault2;
		}
		memset(&(CodeBuff[Ptr+XStart]),0x20,C);
		memset(&(AttrBuff[Ptr+XStart]),AttrDefault,C);
		memset(&(AttrBuff2[Ptr+XStart]),AttrDefault2,C);
		Ptr = NextLinePtr(Ptr);
	}
	BuffUpdateRect(XStart,YStart,XEnd,YEnd);
}

int LeftHalfOfDBCS(LONG Line, int CharPtr)
// If CharPtr is on the right half of a DBCS character,
// return pointer to the left half
//   Line: points to a line in CodeBuff
//   CharPtr: points to a char
//   return: points to the left half of the DBCS
{
	if ((CharPtr>0) &&
		((AttrBuff[Line+CharPtr-1] & AttrKanji) != 0))
		CharPtr--;
	return CharPtr;
}

int MoveCharPtr(LONG Line, int *x, int dx)
// move character pointer x by dx character unit
//   in the line specified by Line
//   Line: points to a line in CodeBuff
//   x: points to a character in the line
//   dx: moving distance in character unit (-: left, +: right)
//		One DBCS character is counted as one character.
//      The pointer stops at the beginning or the end of line.
// Output
//   x: new pointer. x points to a SBCS character or
//      the left half of a DBCS character.
//   return: actual moving distance in character unit
{
	int i;

	*x = LeftHalfOfDBCS(Line,*x);
	i = 0;
	while (dx!=0)
	{
		if (dx>0) // move right
		{
			if ((AttrBuff[Line+*x] & AttrKanji) != 0)
			{
				if (*x<NumOfColumns-2)
				{
					i++;
					*x = *x + 2;
				}
			}
			else if (*x<NumOfColumns-1)
			{
				i++;
				(*x)++;
			}
			dx--;
		}
		else { // move left
			if (*x>0)
			{
				i--;
				(*x)--;
			}
			*x = LeftHalfOfDBCS(Line,*x);
			dx++;
		}
	}
	return i;
}

void BuffCBCopy(BOOL Table)
// copy selected text to clipboard
{
	LONG MemSize;
	PCHAR CBPtr;
	LONG TmpPtr;
	int i, j, k, IStart, IEnd;
	BOOL Sp, FirstChar;
	BYTE b;

	if (! Selected) return;

	// --- open clipboard and get CB memory
	if (BoxSelect)
		MemSize = (SelectEnd.x-SelectStart.x+3)*
			(SelectEnd.y-SelectStart.y+1) + 1;
	else
		MemSize = (SelectEnd.y-SelectStart.y)*
			(NumOfColumns+2) +
			SelectEnd.x - SelectStart.x + 1;
	CBPtr = CBOpen(MemSize);
	if (CBPtr==NULL) return;

	// --- copy selected text to CB memory
	LockBuffer();

	CBPtr[0] = 0;
	TmpPtr = GetLinePtr(SelectStart.y);
	k = 0;
	for (j = SelectStart.y ; j<=SelectEnd.y ; j++)
	{
		if (BoxSelect)
		{
			IStart = SelectStart.x;
			IEnd = SelectEnd.x-1;
		}
		else {
			IStart = 0;
			IEnd = NumOfColumns-1;
			if (j==SelectStart.y) IStart = SelectStart.x;
			if (j==SelectEnd.y) IEnd = SelectEnd.x-1;
		}
		i = LeftHalfOfDBCS(TmpPtr,IStart);
		if (i!=IStart)
		{
			if (j==SelectStart.y)
				IStart = i;
			else
				IStart = i + 2;
		}

		// exclude right-side space characters
		IEnd = LeftHalfOfDBCS(TmpPtr,IEnd);
		while ((IEnd>0) && (CodeBuff[TmpPtr+IEnd]==0x20))
			MoveCharPtr(TmpPtr,&IEnd,-1);
		if ((IEnd==0) && (CodeBuff[TmpPtr]==0x20))
			IEnd = -1;
		else if ((AttrBuff[TmpPtr+IEnd] & AttrKanji) != 0) /* DBCS first byte? */
			IEnd++;

		Sp = FALSE;
		FirstChar = TRUE;
		i = IStart;
		while (i <= IEnd)
		{
			b = CodeBuff[TmpPtr+i];
			i++;
			if (! Sp)
			{
				if ((Table) && (b<=0x20))
				{
					Sp = TRUE;
					b = 0x09;
				}
				if ((b!=0x09) || (! FirstChar))
				{
					FirstChar = FALSE;
					CBPtr[k] = b;
					k++;
				}
			}
			else {
				if (b>0x20)
	{
		Sp = FALSE;
		FirstChar = FALSE;
		CBPtr[k] = b;
		k++;
	}
			}
		}

		if (j < SelectEnd.y)
		{
			CBPtr[k] = 0x0d;
			k++;
			CBPtr[k] = 0x0a;
			k++;
		}

		TmpPtr = NextLinePtr(TmpPtr);
	}
	CBPtr[k] = 0;

	UnlockBuffer();

	// --- send CB memory to clipboard
	CBClose();
	return;
}

void BuffPutChar(BYTE b, BYTE Attr, BYTE Attr2, BOOL Insert)
// Put a character in the buffer at the current position
//   b: character
//   Attr: attribute #1
//   Attr2: attribute #2
//   Insert: Insert flag
{
	int XStart;

	if (Insert)
	{
		memmove(&CodeLine[CursorX+1],&CodeLine[CursorX],NumOfColumns-1-CursorX);
		memmove(&AttrLine[CursorX+1],&AttrLine[CursorX],NumOfColumns-1-CursorX);
		memmove(&AttrLine2[CursorX+1],&AttrLine2[CursorX],NumOfColumns-1-CursorX);
		CodeLine[CursorX] = b;
		AttrLine[CursorX] = Attr;
		AttrLine2[CursorX] = Attr2;
		/* last char in current line is kanji first? */
		if ((AttrLine[NumOfColumns-1] & AttrKanji) != 0)
		{
			/* then delete it */
			CodeLine[NumOfColumns-1] = 0x20;
			AttrLine[NumOfColumns-1] = AttrDefault;
			AttrLine2[NumOfColumns-1] = AttrDefault2;
		}

		if (StrChangeCount==0) XStart = CursorX;
		else XStart = StrChangeStart;
		StrChangeCount = 0;
		BuffUpdateRect(XStart,CursorY,NumOfColumns-1,CursorY);
	}
	else {
		CodeLine[CursorX] = b;
		AttrLine[CursorX] = Attr;
		AttrLine2[CursorX] = Attr2;

		if (StrChangeCount==0)
			StrChangeStart = CursorX;
		StrChangeCount++;
	}
}

BOOL CheckSelect(int x, int y)
//  subroutine called by BuffUpdateRect
{
	LONG L, L1, L2;

	if (BoxSelect)
	{
		return ((Selected &&
				 ((SelectStart.x<=x) && (x<SelectEnd.x)) ||
				 ((SelectEnd.x<=x) && (x<SelectStart.x)) &&
				 ((SelectStart.y<=y) && (y<=SelectEnd.y)) ||
				 ((SelectEnd.y<=y) && (y<=SelectStart.y))));
	}
	else {
		L = MAKELONG(x,y);
		L1 = MAKELONG(SelectStart.x,SelectStart.y);
		L2 = MAKELONG(SelectEnd.x,SelectEnd.y);

		return (Selected &&
				(((L1<=L) && (L<L2)) || ((L2<=L) && (L<L1))));
	}
}

void BuffUpdateRect
(int XStart, int YStart, int XEnd, int YEnd)
// Display text in a rectangular region in the screen
//   XStart: x position of the upper-left corner (screen cordinate)
//   YStart: y position
//   XEnd: x position of the lower-right corner (last character)
//   YEnd: y position
{
	int i, j, count;
	int IStart, IEnd;
	int X, Y;
	LONG TmpPtr;
	BYTE CurAttr, TempAttr;
	BYTE CurAttr2, TempAttr2;
	BOOL CurSel, TempSel, Caret;

	if (XStart >= WinOrgX+WinWidth) return;
	if (YStart >= WinOrgY+WinHeight) return;
	if (XEnd < WinOrgX) return;
	if (YEnd < WinOrgY) return;

	if (XStart < WinOrgX) XStart = WinOrgX;
	if (YStart < WinOrgY) YStart = WinOrgY;
	if (XEnd >= WinOrgX+WinWidth) XEnd = WinOrgX+WinWidth-1;
	if (YEnd >= WinOrgY+WinHeight) YEnd = WinOrgY+WinHeight-1;

	TempAttr = AttrDefault;
	TempAttr2 = AttrDefault2;
	TempSel = FALSE;

	Caret = IsCaretOn();
	if (Caret) CaretOff();

	DispSetupDC(TempAttr,TempAttr2,TempSel);

	Y = (YStart-WinOrgY)*FontHeight;
	TmpPtr = GetLinePtr(PageStart+YStart);
	for (j = YStart+PageStart ; j <= YEnd+PageStart ; j++)
	{
		IStart = XStart;
		IEnd = XEnd;

		IStart = LeftHalfOfDBCS(TmpPtr,IStart);

		X = (IStart-WinOrgX)*FontWidth;

		i = IStart;
		do {
			CurAttr = AttrBuff[TmpPtr+i] & ~ AttrKanji;
			CurAttr2 = AttrBuff2[TmpPtr+i];
			CurSel = CheckSelect(i,j);
			count = 1;
			while
				(((i+count <= IEnd) &&
				  (CurAttr==
				   (AttrBuff[TmpPtr+i+count] & ~ AttrKanji)) &&
				  (CurAttr2==AttrBuff2[TmpPtr+i+count]) &&
				  (CurSel==CheckSelect(i+count,j))) ||
				 ((i+count<NumOfColumns) &&
				  ((AttrBuff[TmpPtr+i+count-1] & AttrKanji) != 0)))
				count++;

			if ((CurAttr != TempAttr) ||
				(CurAttr2 != TempAttr2) ||
				(CurSel != TempSel))
			{
				DispSetupDC(CurAttr,CurAttr2,CurSel);
				TempAttr = CurAttr;
				TempAttr2 = CurAttr2;
				TempSel = CurSel;
			}
			DispStr(&CodeBuff[TmpPtr+i],count,Y, &X);
			i = i+count;
		}
		while (i<=IEnd);
		Y = Y + FontHeight;
		TmpPtr = NextLinePtr(TmpPtr);
	}
	if (Caret) CaretOn();
}

void UpdateStr()
// Display not-yet-displayed string
{
	int X, Y;

	if (StrChangeCount==0) return;
	X = StrChangeStart;
	Y = CursorY;
	if (! IsLineVisible(&X, &Y))
	{
		StrChangeCount = 0;
		return;
	}

	DispSetupDC(AttrLine[StrChangeStart],
				AttrLine2[StrChangeStart],FALSE);
	DispStr(&CodeLine[StrChangeStart],StrChangeCount,Y, &X);
	StrChangeCount = 0;
}

void MoveCursor(int Xnew, int Ynew)
{
	UpdateStr();

	if (CursorY!=Ynew) NewLine(PageStart+Ynew);

	CursorX = Xnew;
	CursorY = Ynew;
	Wrap = FALSE;

	DispScrollToCursor(CursorX, CursorY);
}

void MoveRight()
/* move cursor right, but dont update screen.
 this procedure must be called from DispChar&DispKanji only */
{
	CursorX++;
	DispScrollToCursor(CursorX, CursorY);
}

void MoveLeft()
/* move cursor right, but dont update screen.
 this procedure must be called from DispChar&DispKanji only */
{
	CursorX--;
	DispScrollToCursor(CursorX, CursorY);
}

void BuffSetCaretWidth()
{
	BOOL DW;

	/* check whether cursor on a DBCS character */
	DW = (((BYTE)(AttrLine[CursorX]) & AttrKanji) != 0);
	DispSetCaretWidth(DW);
}

void ScrollUp1Line()
{
	int i;
	LONG SrcPtr = 0, DestPtr;

	if ((CursorTop<=CursorY) && (CursorY<=CursorBottom))
	{
		UpdateStr();

		DestPtr = GetLinePtr(PageStart+CursorBottom);
		for (i = CursorBottom-1 ; i >= CursorTop ; i--)
		{
			SrcPtr = PrevLinePtr(DestPtr);
			memcpy(&(CodeBuff[DestPtr]),&(CodeBuff[SrcPtr]),NumOfColumns);
			memcpy(&(AttrBuff[DestPtr]),&(AttrBuff[SrcPtr]),NumOfColumns);
			memcpy(&(AttrBuff2[DestPtr]),&(AttrBuff2[SrcPtr]),NumOfColumns);
			DestPtr = SrcPtr;
		}
		memset(&(CodeBuff[SrcPtr]),0x20,NumOfColumns);
		memset(&(AttrBuff[SrcPtr]),AttrDefault,NumOfColumns);
		memset(&(AttrBuff2[SrcPtr]),AttrDefault2,NumOfColumns);

		DispScrollNLines(CursorTop,CursorBottom,-1);
	}
}

void BuffScrollNLines(int n)
{
	int i;
	LONG SrcPtr, DestPtr;

	if (n<1) return;
	UpdateStr();

	if ((CursorTop == 0) && (CursorBottom == NumOfLines-1))
	{
		WinOrgY = WinOrgY-n;
		BuffScroll(n,CursorBottom);
		DispCountScroll(n);
	}
	else if ((CursorTop==0) && (CursorY<=CursorBottom))
	{
		BuffScroll(n,CursorBottom);
		DispScrollNLines(WinOrgY,CursorBottom,n);
	}
	else if ((CursorTop<=CursorY) && (CursorY<=CursorBottom))
	{
		DestPtr = GetLinePtr(PageStart+CursorTop);
		if (n<CursorBottom-CursorTop+1)
		{
			SrcPtr = GetLinePtr(PageStart+CursorTop+n);
			for (i = CursorTop+n ; i<=CursorBottom ; i++)
			{
				memmove(&(CodeBuff[DestPtr]),&(CodeBuff[SrcPtr]),NumOfColumns);
				memmove(&(AttrBuff[DestPtr]),&(AttrBuff[SrcPtr]),NumOfColumns);
				memmove(&(AttrBuff2[DestPtr]),&(AttrBuff2[SrcPtr]),NumOfColumns);
				SrcPtr = NextLinePtr(SrcPtr);
				DestPtr = NextLinePtr(DestPtr);
			}
		}
		else
			n = CursorBottom-CursorTop+1;
		for (i = CursorBottom+1-n ; i<=CursorBottom; i++)
		{
			memset(&(CodeBuff[DestPtr]),0x20,NumOfColumns);
			memset(&(AttrBuff[DestPtr]),AttrDefault,NumOfColumns);
			memset(&(AttrBuff2[DestPtr]),AttrDefault2,NumOfColumns);
			DestPtr = NextLinePtr(DestPtr);
		}
		DispScrollNLines(CursorTop,CursorBottom,n);
	}
}

void BuffClearScreen()
{ // clear screen
	if ((StatusLine>0) && (CursorY==NumOfLines-1))
		BuffScrollNLines(1); /* clear status line */
	else { /* clear main screen */
		UpdateStr();
		BuffScroll(NumOfLines-StatusLine,NumOfLines-1-StatusLine);
		DispScrollNLines(WinOrgY,NumOfLines-1-StatusLine,NumOfLines-StatusLine);
	}
}

void BuffUpdateScroll()
// Updates scrolling
{
	UpdateStr();
	DispUpdateScroll();
}

void CursorUpWithScroll()
{
	if (((0<CursorY) && (CursorY<CursorTop)) ||
		(CursorTop<CursorY))
		MoveCursor(CursorX,CursorY-1);
	else if (CursorY==CursorTop)
		ScrollUp1Line();
}

// called by BuffDblClk --- Remove this!!! Brian
//   check if a character is the word delimiter
BOOL IsDelimiter(LONG Line, int CharPtr)
{
	return 0;
}

void GetMinMax(int i1, int i2, int i3,
			   int *min, int *max)
{
	if (i1<i2)
	{
		*min = i1;
	  *max = i2;
	}
	else {
		*min = i2;
		*max = i1;
	}
	if (i3<*min)
		*min = i3;
	if (i3>*max)
		*max = i3;
}

void ChangeSelectRegion()
{
	POINT TempStart, TempEnd;
	int j, IStart, IEnd;
	BOOL Caret;

	if ((SelectEndOld.x==SelectEnd.x) &&
		(SelectEndOld.y==SelectEnd.y)) return;

	if (BoxSelect)
	{
		GetMinMax(SelectStart.x,SelectEndOld.x,SelectEnd.x,
				  (int *)&TempStart.x,(int *)&TempEnd.x);
		GetMinMax(SelectStart.y,SelectEndOld.y,SelectEnd.y,
				  (int *)&TempStart.y,(int *)&TempEnd.y);
		TempEnd.x--;
		Caret = IsCaretOn();
		if (Caret) CaretOff();
		DispInitDC();
		BuffUpdateRect(TempStart.x,TempStart.y-PageStart,
					   TempEnd.x,TempEnd.y-PageStart);
		DispReleaseDC();
		if (Caret) CaretOn();
		SelectEndOld = SelectEnd;
		return;
	}

	if ((SelectEndOld.y < SelectEnd.y) ||
		((SelectEndOld.y==SelectEnd.y) &&
		 (SelectEndOld.x<=SelectEnd.x)))
	{
		TempStart = SelectEndOld;
		TempEnd.x = SelectEnd.x-1;
		TempEnd.y = SelectEnd.y;
	}
	else {
		TempStart = SelectEnd;
		TempEnd.x = SelectEndOld.x-1;
		TempEnd.y = SelectEndOld.y;
	}
	if (TempEnd.x < 0)
	{
		TempEnd.x = NumOfColumns - 1;
		TempEnd.y--;
	}

	Caret = IsCaretOn();
	if (Caret) CaretOff();
	for (j = TempStart.y ; j <= TempEnd.y ; j++)
	{
		IStart = 0;
		IEnd = NumOfColumns-1;
		if (j==TempStart.y) IStart = TempStart.x;
		if (j==TempEnd.y) IEnd = TempEnd.x;

		if ((IEnd>=IStart) && (j >= PageStart+WinOrgY) &&
			(j < PageStart+WinOrgY+WinHeight))
		{
			DispInitDC();
			BuffUpdateRect(IStart,j-PageStart,IEnd,j-PageStart);
			DispReleaseDC();
		}
	}
	if (Caret) CaretOn();

	SelectEndOld = SelectEnd;
}

void BuffDblClk(int Xw, int Yw)
//  Select a word at (Xw, Yw) by mouse double click
//    Xw: horizontal position in window coordinate (pixels)
//    Yw: vertical
{
	int X, Y;
	int IStart, IEnd, i;
	LONG TmpPtr;
	BYTE b;
	BOOL DBCS;

	CaretOff();

	DispConvWinToScreen(Xw,Yw,&X,&Y,NULL);
	Y = Y + PageStart;
	if ((Y<0) || (Y>=BuffEnd)) return;
	if (X<0) X = 0;
	if (X>=NumOfColumns) X = NumOfColumns-1;

	BoxSelect = FALSE;
	LockBuffer();
	SelectEnd = SelectStart;
	ChangeSelectRegion();

	if ((Y>=0) && (Y<BuffEnd))
	{
		TmpPtr = GetLinePtr(Y);

		IStart = X;
		IStart = LeftHalfOfDBCS(TmpPtr,IStart);
		IEnd = IStart;

		if (IsDelimiter(TmpPtr,IStart))
		{
			b = CodeBuff[TmpPtr+IStart];
			DBCS = (AttrBuff[TmpPtr+IStart] & AttrKanji) != 0;
			while (((IStart>0) &&
					((b==CodeBuff[TmpPtr+IStart])) ||
					(DBCS &&
					 ((AttrBuff[TmpPtr+IStart] & AttrKanji)!=0))))
				MoveCharPtr(TmpPtr,&IStart,-1); // move left
			if ((b!=CodeBuff[TmpPtr+IStart]) &&
				! (DBCS &&
				   ((AttrBuff[TmpPtr+IStart] & AttrKanji)!=0)))
				MoveCharPtr(TmpPtr,&IStart,1);

			i = 1;
			while (((i!=0) &&
					((b==CodeBuff[TmpPtr+IEnd])) ||
					(DBCS &&
					 ((AttrBuff[TmpPtr+IEnd] & AttrKanji)!=0))))
				i = MoveCharPtr(TmpPtr,&IEnd,1); // move right
		}
		else {
			while ((IStart>0) &&
				   ! IsDelimiter(TmpPtr,IStart))
				MoveCharPtr(TmpPtr,&IStart,-1); // move left
			if (IsDelimiter(TmpPtr,IStart))
				MoveCharPtr(TmpPtr,&IStart,1);

			i = 1;
			while ((i!=0) &&
				   ! IsDelimiter(TmpPtr,IEnd))
				i = MoveCharPtr(TmpPtr,&IEnd,1); // move right
		}
		if (i==0)
			IEnd = NumOfColumns;

		if (IStart<=X)
		{
			SelectStart.x = IStart;
			SelectStart.y = Y;
			SelectEnd.x = IEnd;
			SelectEnd.y = Y;
			SelectEndOld = SelectStart;
			DblClkStart = SelectStart;
			DblClkEnd = SelectEnd;
			Selected = TRUE;
			ChangeSelectRegion();
		}
	}
	UnlockBuffer();
}

void BuffTplClk(int Yw)
//  Select a line at Yw by mouse tripple click
//    Yw: vertical clicked position
//			in window coordinate (pixels)
{
	int Y;

	CaretOff();

	DispConvWinToScreen(0,Yw,NULL,&Y,NULL);
	Y = Y + PageStart;
	if ((Y<0) || (Y>=BuffEnd)) return;

	LockBuffer();
	SelectEnd = SelectStart;
	ChangeSelectRegion();
	SelectStart.x = 0;
	SelectStart.y = Y;
	SelectEnd.x = NumOfColumns;
	SelectEnd.y = Y;
	SelectEndOld = SelectStart;
	DblClkStart = SelectStart;
	DblClkEnd = SelectEnd;
	Selected = TRUE;
	ChangeSelectRegion();
	UnlockBuffer();
}

void BuffStartSelect(int Xw, int Yw, BOOL Box)
//  Start text selection by mouse button down
//    Xw: horizontal position in window coordinate (pixels)
//    Yw: vertical
//    Box: Box selection if TRUE
{
	int X, Y;
	BOOL Right;
	LONG TmpPtr;

	DispConvWinToScreen(Xw,Yw, &X,&Y,&Right);
	Y = Y + PageStart;
	if ((Y<0) || (Y>=BuffEnd)) return;
	if (X<0) X = 0;
	if (X>=NumOfColumns) X = NumOfColumns-1;

	SelectEndOld = SelectEnd;
	SelectEnd = SelectStart;

	LockBuffer();
	ChangeSelectRegion();
	UnlockBuffer();

	SelectStart.x = X;
	SelectStart.y = Y;
	if (SelectStart.x<0) SelectStart.x = 0;
	if (SelectStart.x > NumOfColumns)
		SelectStart.x = NumOfColumns;
	if (SelectStart.y < 0) SelectStart.y = 0;
	if (SelectStart.y >= BuffEnd)
		SelectStart.y = BuffEnd - 1;

	TmpPtr = GetLinePtr(SelectStart.y);
	// check if the cursor is on the right half of a character
	if (((SelectStart.x>0) &&
		 ((AttrBuff[TmpPtr+SelectStart.x-1] & AttrKanji) != 0)) ||
		(((AttrBuff[TmpPtr+SelectStart.x] & AttrKanji) == 0) &&
		 Right)) SelectStart.x++;

	SelectEnd = SelectStart;
	SelectEndOld = SelectEnd;
	CaretOff();
	Selected = TRUE;
	BoxSelect = Box;
}

void BuffChangeSelect(int Xw, int Yw, int NClick)
//  Change selection region by mouse move
//    Xw: horizontal position of the mouse cursor
//			in window coordinate
//    Yw: vertical
{
	int X, Y;
	BOOL Right;
	LONG TmpPtr;
	int i;
	BYTE b;
	BOOL DBCS;

	DispConvWinToScreen(Xw,Yw,&X,&Y,&Right);
	Y = Y + PageStart;

	if (X<0) X = 0;
	if (X > NumOfColumns)
		X = NumOfColumns;
	if (Y < 0) Y = 0;
	if (Y >= BuffEnd)
		Y = BuffEnd - 1;

	TmpPtr = GetLinePtr(Y);
	LockBuffer();
	// check if the cursor is on the right half of a character
	if (((X>0) &&
		 ((AttrBuff[TmpPtr+X-1] & AttrKanji) != 0)) ||
		((X<NumOfColumns) &&
		 ((AttrBuff[TmpPtr+X] & AttrKanji) == 0) &&
		 Right)) X++;

	if (X > NumOfColumns)
		X = NumOfColumns;

	SelectEnd.x = X;
	SelectEnd.y = Y;

	if (NClick==2) // drag after double click
	{
		if ((SelectEnd.y>SelectStart.y) ||
			(SelectEnd.y==SelectStart.y) &&
			(SelectEnd.x>=SelectStart.x))
		{
			if (SelectStart.x==DblClkEnd.x)
			{
				SelectEnd = DblClkStart;
				ChangeSelectRegion();
				SelectStart = DblClkStart;
				SelectEnd.x = X;
				SelectEnd.y = Y;
			}
			MoveCharPtr(TmpPtr,&X,-1);
			if (X<SelectStart.x) X = SelectStart.x;

			i = 1;
			if (IsDelimiter(TmpPtr,X))
			{
				b = CodeBuff[TmpPtr+X];
				DBCS = (AttrBuff[TmpPtr+X] & AttrKanji) != 0;
				while (((i!=0) &&
						((b==CodeBuff[TmpPtr+SelectEnd.x])) ||
						(DBCS &&
						 ((AttrBuff[TmpPtr+SelectEnd.x] & AttrKanji)!=0))))
					i = MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,1); // move right
			}
			else {
				while ((i!=0) &&
					   ! IsDelimiter(TmpPtr,SelectEnd.x))
					i = MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,1); // move right
			}
			if (i==0)
				SelectEnd.x = NumOfColumns;
		}
		else {
			if (SelectStart.x==DblClkStart.x)
			{
				SelectEnd = DblClkEnd;
				ChangeSelectRegion();
				SelectStart = DblClkEnd;
				SelectEnd.x = X;
				SelectEnd.y = Y;
			}
			if (IsDelimiter(TmpPtr,SelectEnd.x))
			{
				b = CodeBuff[TmpPtr+SelectEnd.x];
				DBCS = (AttrBuff[TmpPtr+SelectEnd.x] & AttrKanji) != 0;
				while (((SelectEnd.x>0) &&
						((b==CodeBuff[TmpPtr+SelectEnd.x])) ||
						(DBCS &&
						 ((AttrBuff[TmpPtr+SelectEnd.x] & AttrKanji)!=0))))
					MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,-1); // move left
				if ((b!=CodeBuff[TmpPtr+SelectEnd.x]) &&
					! (DBCS &&
					   ((AttrBuff[TmpPtr+SelectEnd.x] & AttrKanji)!=0)))
					MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,1);
			}
			else {
				while ((SelectEnd.x>0) &&
					   ! IsDelimiter(TmpPtr,SelectEnd.x))
					MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,-1); // move left
				if (IsDelimiter(TmpPtr,SelectEnd.x))
					MoveCharPtr(TmpPtr,(int *)&SelectEnd.x,1);
			}
		}
	}
	else if (NClick==3) // drag after tripple click
	{
		if (((SelectEnd.y>SelectStart.y) ||
			 (SelectEnd.y==SelectStart.y)) &&
			(SelectEnd.x>=SelectStart.x))
		{
			if (SelectStart.x==DblClkEnd.x)
			{
				SelectEnd = DblClkStart;
	ChangeSelectRegion();
	SelectStart = DblClkStart;
	SelectEnd.x = X;
	SelectEnd.y = Y;
			}
			SelectEnd.x = NumOfColumns;
		}
		else {
			if (SelectStart.x==DblClkStart.x)
			{
				SelectEnd = DblClkEnd;
				ChangeSelectRegion();
				SelectStart = DblClkEnd;
				SelectEnd.x = X;
				SelectEnd.y = Y;
			}
			SelectEnd.x = 0;
		}
	}

	ChangeSelectRegion();
	UnlockBuffer();
}

void BuffEndSelect()
//  End text selection by mouse button up
{
	Selected = (SelectStart.x!=SelectEnd.x) ||
		(SelectStart.y!=SelectEnd.y);
	if (Selected)
	{
		if (BoxSelect)
		{
			if (SelectStart.x>SelectEnd.x)
			{
				SelectEndOld.x = SelectStart.x;
				SelectStart.x = SelectEnd.x;
				SelectEnd.x = SelectEndOld.x;
			}
			if (SelectStart.y>SelectEnd.y)
			{
        SelectEndOld.y = SelectStart.y;
		SelectStart.y = SelectEnd.y;
		SelectEnd.y = SelectEndOld.y;
			}
		}
		else if ((SelectEnd.y < SelectStart.y) ||
				 ((SelectEnd.y == SelectStart.y) &&
				  (SelectEnd.x < SelectStart.x)))
		{
			SelectEndOld = SelectStart;
			SelectStart = SelectEnd;
			SelectEnd = SelectEndOld;
		}
		/* copy to the clipboard */
		LockBuffer();
		BuffCBCopy(FALSE);
		UnlockBuffer();
	}
}

void BuffChangeWinSize(int Nx, int Ny)
// Change window size
//   Nx: new window width (number of characters)
//   Ny: new window hight
{
	if (Nx==0) Nx = 1;
	if (Ny==0) Ny = 1;

	if (((Nx!=NumOfColumns) || (Ny!=NumOfLines)))
	{
		LockBuffer();
		BuffChangeTerminalSize(Nx,Ny-StatusLine);
		UnlockBuffer();
		Nx = NumOfColumns;
		Ny = NumOfLines;
	}
	if (Nx>NumOfColumns) Nx = NumOfColumns;
	if (Ny>BuffEnd) Ny = BuffEnd;
	DispChangeWinSize(Nx,Ny);
}

void BuffChangeTerminalSize(int Nx, int Ny)
{
	int i, Nb, W, H;
	BOOL St;

	Ny = Ny + StatusLine;
	if (Nx < 1) Nx = 1;
	if (Ny < 1) Ny = 1;
	if (Nx > BuffXMax) Nx = BuffXMax;
	St = ((StatusLine>0) && (CursorY==NumOfLines-1));
	if ((Nx!=NumOfColumns) || (Ny!=NumOfLines))
	{
		Nb = Ny;

		if (! ChangeBuffer(Nx,Nb)) return;
		if (Ny > NumOfLinesInBuff) Ny = NumOfLinesInBuff;

		NumOfColumns = Nx;
		NumOfLines = Ny;
		/*ts.TerminalWidth = Nx;
		   ts.TerminalHeight = Ny-StatusLine;*/

		PageStart = BuffEnd - NumOfLines;
	}
	BuffScroll(NumOfLines,NumOfLines-1);
	/* Set Cursor */
	CursorX = 0;
	if (St)
	{
		CursorY = NumOfLines-1;
		CursorTop = CursorY;
		CursorBottom = CursorY;
	}
	else {
		CursorY = 0;
		CursorTop = 0;
		CursorBottom = NumOfLines-1-StatusLine;
	}

	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd = SelectStart;
	Selected = FALSE;

	/* Tab stops */
	NTabStops = (NumOfColumns-1) >> 3;
	for (i = 1 ; i <= NTabStops ; i++)
		TabStops[i-1] = i*8;

	W = NumOfColumns;
	H = NumOfLines;

	NewLine(PageStart+CursorY);

	/* Change Window Size */
	BuffChangeWinSize(W,H);
	WinOrgY = -NumOfLines;
	DispScrollHomePos();
}

void ChangeWin()
{
	int Ny;

	/* Change buffer */
	Ny = NumOfLines;

	if (NumOfLinesInBuff!=Ny)
	{
		ChangeBuffer(NumOfColumns,Ny);

		if (BuffEnd < WinHeight)
			BuffChangeWinSize(WinWidth,BuffEnd);
		else
			BuffChangeWinSize(WinWidth,WinHeight);
	}

	DispChangeWin();
}

void ClearBuffer()
{
	/* Reset buffer */
	PageStart = 0;
	BuffStartAbs = 0;
	BuffEnd = NumOfLines;
	if (NumOfLines==NumOfLinesInBuff)
		BuffEndAbs = 0;
	else
		BuffEndAbs = NumOfLines;

	SelectStart.x = 0;
	SelectStart.y = 0;
	SelectEnd = SelectStart;
	SelectEndOld = SelectStart;
	Selected = FALSE;

	NewLine(0);
	memset(&CodeBuff[0],0x20,BufferSize);
	memset(&AttrBuff[0],AttrDefault,BufferSize);
	memset(&AttrBuff2[0],AttrDefault2,BufferSize);

	/* Home position */
	CursorX = 0;
	CursorY = 0;
	WinOrgX = 0;
	WinOrgY = 0;
	NewOrgX = 0;
	NewOrgY = 0;

	/* Top/bottom margin */
	CursorTop = 0;
	CursorBottom = NumOfLines-1;

	StrChangeCount = 0;

	DispClearWin();
}

void SetTabStop()
{
	int i,j;

	if (NTabStops<NumOfColumns)
	{
		i = 0;
		while ((TabStops[i]<CursorX) && (i<NTabStops))
			i++;

		if ((i<NTabStops) && (TabStops[i]==CursorX)) return;

		for (j=NTabStops ; j>=i+1 ; j--)
			TabStops[j] = TabStops[j-1];
		TabStops[i] = CursorX;
		NTabStops++;
	}
}

void MoveToNextTab()
{
	int i;

	if (NTabStops>0)
	{
		i = -1;
		do
			i++;
		while ((TabStops[i]<=CursorX) && (i<NTabStops-1));
		if (TabStops[i]>CursorX)
			MoveCursor(TabStops[i],CursorY);
		else
			MoveCursor(NumOfColumns-1,CursorY);
	}
	else
		MoveCursor(NumOfColumns-1,CursorY);
}

void ClearTabStop(int Ps)
// Clear tab stops
//   Ps = 0: clear the tab stop at cursor
//      = 3: clear all tab stops
{
	int i,j;

	if (NTabStops>0)
		switch (Ps) {
		case 0:
			i = 0;
			while ((TabStops[i]!=CursorX) && (i<NTabStops-1))
				i++;
			if (TabStops[i] == CursorX)
			{
				NTabStops--;
				for (j=i ; j<=NTabStops ; j++)
					TabStops[j] = TabStops[j+1];
			}
			break;
		case 3: NTabStops = 0; break;
		}
}

void ShowStatusLine(int Show)
// show/hide status line
{
	int Ny = 0, Nb, W, H;

	BuffUpdateScroll();
	if (Show==StatusLine) return;
	StatusLine = Show;

	if (StatusLine==0)
	{
		NumOfLines--;
		BuffEnd--;
		BuffEndAbs=PageStart+NumOfLines;
		if (BuffEndAbs >= NumOfLinesInBuff)
			BuffEndAbs = BuffEndAbs-NumOfLinesInBuff;
		Ny = NumOfLines;
	}
	/*else
	 Ny = ts.TerminalHeight+1;*/

	Nb = Ny;

	if (! ChangeBuffer(NumOfColumns,Nb)) return;
	if (Ny > NumOfLinesInBuff) Ny = NumOfLinesInBuff;

	NumOfLines = Ny;

	if (StatusLine==1)
		BuffScroll(1,NumOfLines-1);

	W = NumOfColumns;
	H = NumOfLines;

	PageStart = BuffEnd-NumOfLines;
	NewLine(PageStart+CursorY);

	/* Change Window Size */
	BuffChangeWinSize(W,H);
	WinOrgY = -NumOfLines;
	DispScrollHomePos();

	MoveCursor(CursorX,CursorY);
}
/* End Scrollback code */

void winbx_init(void)
{
	WNDCLASS wc;
	DWORD Style;
	MSG msg;

	MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);

#if 0
	InitKeyboard();
	SetKeyMap();
#endif

	/* Initialize scroll buffer */
	InitBuffer();

	InitDisp();

	Style = WS_VSCROLL | WS_HSCROLL |
		WS_BORDER | WS_THICKFRAME |
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = AfxWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	/*wc.hInstance = AfxGetInstanceHandle();*/
	/*wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_VT));*/
	wc.hCursor = LoadCursor(NULL,IDC_IBEAM);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "VTWin";

	RegisterClass(&wc);

	HVTWin = CreateWindow("VTWin", _VERSION_, Style, 100, 100, 300, 400, NULL, NULL, &wc.hInstance, NULL);

	if (HVTWin == NULL) return;

	// set the small icon
	/* PostMessage(HVTWin,WM_SETICON,0,
	 (LPARAM)LoadImage(AfxGetInstanceHandle(),
	 MAKEINTRESOURCE(IDI_VT),
	 IMAGE_ICON,16,16,0));*/

	/* Reset Terminal */
	ResetTerminal();

	ChangeFont();

	BuffChangeWinSize(NumOfColumns,NumOfLines);

	ShowWindow(HVTWin, SW_SHOWDEFAULT);
	ChangeCaret();

	current_term->TI_cols = co = 81;
	current_term->TI_lines = li = 25;

#if 0
	while (GetMessage(&msg,NULL,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
}

void gui_init(void)
{
	winbx_init();
#if 0
	_beginthread((void *)winbx_init, 0xFFFF, NULL);
	CreateThread(NULL, 0, (void *)winbx_init, NULL, 0, NULL);
    Sleep(2000);
#endif
}

void gui_clreol(void)
{
}

void gui_gotoxy(int col, int row)
{
    /*MoveCursor(col, row);*/
}

void gui_clrscr(void)
{
/*	BuffClearScreen();*/
}

void gui_left(int num)
{
/*	int z;

	for(z=0;z<num;z++)
		MoveLeft();*/
}

void gui_right(int num)
{
/*	int z;

	for(z=0;z<num;z++)
		MoveRight();*/
}

void gui_scroll(int top, int bot, int n)
{
	/*BuffScroll(bot, n);*/
}

void gui_flush(void)
{
}

void gui_puts(unsigned char *buffer)
{
	/*DispANSI(buffer);*/
}

void gui_new_window(Screen *new, Window *win)
{
}

void gui_kill_window(Screen *killscreen)
{
}

void gui_settitle(char *titletext, Screen *screen)
{
	SetWindowText(screen->hwndFrame,titletext);
}

void gui_font_dialog(Screen *screen)
{
}

void gui_file_dialog(char *type, char *path, char *title, char *ok, char *apply, char *code, char *szButton)
{
}

void gui_properties_notebook(void)
{
}

void gui_msgbox(void)
{
}

void gui_popupmenu(char *menuname)
{
}

void gui_paste(char *args)
{
}

void gui_setfocus(Screen *screen)
{
}

void gui_scrollerchanged(Screen *screen, int position)
{
}

void gui_query_window_info(Screen *screen, char *fontinfo, int *x, int *y, int *cx, int *cy)
{
}

void gui_play_sound(char *filename)
{
}

void gui_get_sound_error(int errnum, char *errstring)
{
}

void gui_menu(Screen *screen, char *addmenu)
{
}

void gui_exit(void)
{
}

void gui_screen(Screen *new)
{
	new->hwndFrame = HVTWin;
	new->menu=(HWND)NULL;
	new->old_li=new->li;
	new->old_co=new->co;
	new->nicklist = get_int_var(NICKLIST_VAR);

	main_screen = new;
}


void gui_resize(Screen *this_screen)
{
	if(this_screen->co < 20) this_screen->old_co=this_screen->co=20;
	if(this_screen->li < 10) this_screen->old_li=this_screen->li=10;
	if(this_screen->co > 199) this_screen->old_co=this_screen->co=199;
	if(this_screen->li > 99) this_screen->old_li=this_screen->li=99;
	/*cx = this_screen->co * this_screen->VIO_font_width;
	cy = this_screen->li * this_screen->VIO_font_height;*/

	co = this_screen->co; li = this_screen->li;

	/* Recalculate some stuff that was done in input.c previously */
	this_screen->input_line = this_screen->li-1;

	this_screen->input_zone_len = this_screen->co - (WIDTH * 2);
	if (this_screen->input_zone_len < 10)
		this_screen->input_zone_len = 10;		/* Take that! */

	this_screen->input_start_zone = WIDTH;
	this_screen->input_end_zone = this_screen->co - WIDTH;
}

int gui_send_mci_string(char *mcistring, char *retstring)
{
return 0;
}

int gui_isset(Screen *screen, fd_set *rd, int what)
{
return 0;
}

int gui_putc(int c)
{
return 0;
}

int gui_read(Screen *screen, char *buffer, int maxbufsize)
{
return 0;
}

int gui_screen_width(void)
{
return 0;
}

int gui_screen_height(void)
{
return 0;
}

void gui_setwindowpos(Screen *screen, int x, int y, int cx, int cy, int top, int bottom, int min, int max, int restore, int activate, int size, int position)
{
}

void gui_font_init(void)
{
}

void gui_font_set(char *font, Screen *screen)
{
}

void BX_gui_mutex_lock(void)
{
}

void BX_gui_mutex_unlock(void)
{
}

int gui_setmenuitem(char *menuname, int refnum, char *what, char *param)
{
return 0;
}

void gui_remove_menurefs(char *menu)
{
}

void gui_mdi(Window *window, char *text, int value)
{
}

void gui_update_nicklist(char *channel)
{
}

void gui_nicklist_width(int width, Screen *this_screen)
{
}

void gui_startup(int argc, char *argv[])
{
}

