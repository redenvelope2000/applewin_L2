AppleWin for RetroArch
========
This is an experimental build of the AppleWin libretro core from https://github.com/audetto/AppleWin. The main purpose is to re-enable the AppleWin internal debugger.

The following libraries are necessary to build the core:
minizip yaml Z

The dacap clipboard library from https://github.com/dacap/clip is optional.

The libretro info file is included.

Note that because this is a computer emulator which uses the keyboard extensively, the keyboard device must be set to "GAME FOCUS" mode. The user should set that in the RetroArch UI menu. Otherwise the keyboard cannot work properly.

Verified platforms:
macOS Monterey M1/X64 
macOS Catalina X64
Windows 10/11

Verified functions:
Internal debugger (of course!). Hit F7 to enter it.
Mockingboard PSG sounds.
DISK II floppy drive operating sounds.
Clipboard operation for text strings and the graphics screen. Use Ctrl/Shift-Ins on Windows, Cmd-C/Cmd-V on macOS.
Save/Restore.
Nuklear GUI (https://github.com/Immediate-Mode-UI/Nuklear). Press F8 key/MENU key/Right-Cmd key to enter it.

Not-working functions:
Uthernet
Apple mouse
Apple clock


