// stdinc.h : 標準のシステム インクルード ファイル、
//            または参照回数が多く、かつあまり変更されない
//            プロジェクト専用のインクルード ファイルを記述します。

#if !defined(INC_STDINC_H)
#define INC_STDINC_H

#define WIN32_LEAN_AND_MEAN		// windows ヘッダーから殆ど使用されないスタッフを除外します
#define STRICT
#define _WIN32_IE	0x0200
#define DIRECTDRAW_VERSION	0x0300
#define DIRECTSOUND_VERSION	0x0300
#define DIRECTINPUT_VERSION	0x0500

// windows headers:
/*
#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <mbstring.h>
*/
// CRT headers:
/*
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <tchar.h>
*/
// local headers:



typedef unsigned char byte;
typedef unsigned short word;
typedef signed char offset;

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;

typedef unsigned char	TCHAR;

#define CHAR_BIT        8

#define CHAR_MAX        UCHAR_MAX
#define CHAR_MIN        0

#define INT_MAX         +32767
#define INT_MIN         -32767

#define LONG_MAX        +2147483647L
#define LONG_MIN        -2147483647L

#define SCHAR_MAX       +127
#define SCHAR_MIN       -127

#define SHRT_MAX        +32767
#define SHRT_MIN        -32767

#define UCHAR_MAX       255U
#define UINT_MAX        65535U
#define ULONG_MAX       4294967295UL
#define USHRT_MAX       65535U






// global definitions:
#define HANDLE_COMMAND(hwnd, id, handler) \
	case id: handler(hwnd); return 0L;

#define HANDLE_CONTROL(hwnd, id, code, handler) \
	case id: if (HIWORD(wParam)==code) {handler(hwnd); return 0L;} else

#define HANDLE_NOTIFY(hwnd, id, code, handler) \
	case code: if (((NMHDR *)lParam)->idFrom==id) {handler(hwnd, ((NMHDR *)lParam)); return 0L;} else

#define HANDLE_WM_ENTERMENULOOP(hwnd, wParam, lParam, fn) \
	((fn)((hwnd), (BOOL)(wParam)), 0L)
#define FORWARD_WM_ENTERMENULOOP(hwnd, fIsTrackPopupMenu, fn) \
	(void)(fn)((hwnd), WM_ENTERMENULOOP, (WPARAM)(BOOL)(fIsTrackPopupMenu), (LPARAM)0L)
#define HANDLE_WM_EXITMENULOOP(hwnd, wParam, lParam, fn) \
	((fn)((hwnd), (BOOL)(wParam)), 0L)
#define FORWARD_WM_EXITMENULOOP(hwnd, fIsTrackPopupMenu, fn) \
	(void)(fn)((hwnd), WM_ENTERMENULOOP, (WPARAM)(BOOL)(fIsTrackPopupMenu), (LPARAM)0L)

#if !defined(NDEBUG)
#define PRIVATE
#else // !defined(NDEBUG)
#define PRIVATE static
#endif // !defined(NDEBUG)

#if defined(_MSC_VER)
#define INLINE	__forceinline
#else // defined(MSC_VER)
#define INLINE
#endif // defined(MSC_VER)

#if defined(_MSC_VER)
#define VARCALL	__cdecl
#else // defined(MSC_VER)
#define VARCALL
#endif // defined(MSC_VER)

#if !defined(NDEBUG)
#define ASSERT(e) assert(e)
#else // !defined(NDEBUG)
#define ASSERT(e)
#endif // !defined(NDEBUG)

//{{AFX_INSERT_LOCATION}}
// for Microsoft Visual C++

#endif // !defined(INC_STDINC_H)
