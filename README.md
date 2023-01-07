AppleWin for RetroArch
========
This is an experimental build of the AppleWin libretro core from https://github.com/audetto/AppleWin for enabling the AppleWin internal debugger.

## The following libraries are necessary to build the core:
  * minizip
  * yaml
  * Z
  * (optional) dacap clipboard library from https://github.com/dacap/clip
##
The libretro info file is included.

Because the keyboard is the main input device of the Apple II computer, the "GAME FOCUS" mode should be enabled from the RetroArch UI menu. Otherwise the keyboard cannot work properly.

## Verified platforms:
  * macOS Monterey M1/X64 
  * macOS Catalina X64
  * Windows 10/11 X64

## Verified functions:
  * Internal debugger. Press F7 to enter it. The Applewin internal debugger is an extremely powerful tool. Press F7 anytime to stop and see what the Apple is doing.
    * Space to single step the instruction.
    * Ctrl-Space to step over the instruction.
    * Up/Down to move the cursor line.
    * Ctrl-Down to run to the cursor line.
    * Ctrl-Right to set PC to the cursor line.    
    * Right to follow the branching instruction.
      * Left to return.
    * Alt-Left to move to the caller following the stack.
    * Alt-Right to move to current PC.
    * Ctrl-Left to add/remove a break point. 
      * Use bpx/bpc/bpm from the debugger command prompt for more sophisticated operations.
    * Ctrl-Shift-#digit to set a bookmark.
      * Ctrl-#digit to move to the bookmark.
    * Add symbol: symuser "symbol"=####
    * Delete symbol: symuser !"symbol"
    * Save symbol table: symuser save "symbol-table-name.sym"
      * The file is written to ~/Documents/AppleWin. 
    * Load symbol table: symuser load "symbol-table-name.sym"    
  * Mockingboard PSG sounds. Speech is still limited.
  * Floppy drive operation sounds.
  * Clipboard operations. Copy text/graphics screen to the host OS and paste back text strings to the Apple. Use Ctrl-Ins/Shift-Ins on Windows, Cmd-C/Cmd-V on macOS.
  * Save/Restore of current emualtion state under the Retroarch UI.
  * Nuklear GUI (https://github.com/Immediate-Mode-UI/Nuklear). Press F8/Windows MENU key/macOS Right-Cmd key to enter it.

## Non-working functions:
  * Uthernet. I disabled the network code for simplifying coding .
  * Apple mouse
  * Apple clock


