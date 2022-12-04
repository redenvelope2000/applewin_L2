#pragma once

#include "winhandles.h"

typedef void * HCURSOR;

#define IDC_WAIT "IDC_WAIT"
#define CB_ERR              (-1)
#define CB_ADDSTRING             0x0143
#define CB_RESETCONTENT          0x014b
#define CB_SETCURSEL             0x014e

HCURSOR LoadCursor(HINSTANCE hInstance, LPCSTR lpCursorName);
HCURSOR SetCursor(HCURSOR hCursor);

typedef VOID    (CALLBACK *TIMERPROC)(HWND,UINT,UINT_PTR,DWORD);

UINT_PTR SetTimer(HWND,UINT_PTR,UINT,TIMERPROC);
BOOL KillTimer(HWND hWnd, UINT uIDEvent);
HWND        WINAPI GetDlgItem(HWND,INT);
LRESULT     WINAPI SendMessage(HWND,UINT,WPARAM,LPARAM);
void        WINAPI PostQuitMessage(INT);

#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_RETURN         0x0D
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_ESCAPE         0x1B
#define VK_SPACE          0x20
#define VK_PRIOR          0x7021
#define VK_NEXT           0x7022
#define VK_END            0x7023
#define VK_HOME           0x7024
#define VK_LEFT           0x7025
#define VK_UP             0x7026
#define VK_RIGHT          0x7027
#define VK_DOWN           0x7028
#define VK_DELETE         0x7029
#define VK_INSERT         0x702a
#define VK_PAUSE          0x702b
#define VK_OEM_3          0xC0   // '`~' for US
#define VK_OEM_8          0xDF

#define KF_EXTENDED       0x0100
#define VK_F1             0x8001
#define VK_F2             0x8002
#define VK_F3             0x8003
#define VK_F4             0x8004
#define VK_F5             0x8005
#define VK_F6             0x8006
#define VK_F7             0x8007
#define VK_F8             0x8008
#define VK_F9             0x8009
#define VK_F10            0x800a
#define VK_F11            0x800b
#define VK_F12            0x800c
#define VK_F13            0x800d
#define VK_F14            0x800e
#define VK_F15            0x800f

#define VK_COPY           0x8101
#define VK_PASTE          0x8102
#define VK_RESET          0x8103
#define VK_DEBUG          0x8104
#define VK_DISPLAY        0x8105
#define VK_GUI            0x8106
