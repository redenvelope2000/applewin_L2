#include "StdAfx.h"
#include "frontends/libretro/game.h"
#include "frontends/libretro/retroregistry.h"
#include "frontends/libretro/retroframe.h"

#include "Common.h"
#include "CardManager.h"
#include "Core.h"
#include "Mockingboard.h"
#include "Speaker.h"
#include "Log.h"
#include "CPU.h"
#include "NTSC.h"
#include "Utilities.h"
#include "Interface.h"
#include "Debug.h"
#include "Memory.h"

#include "linux/keyboard.h"
//#include "linux/registry.h"
#include "linux/paddle.h"
#include "linux/context.h"
#include "frontends/common2/utils.h"

#include "libretro.h"
#include "clipboard.h"
#include "DiskSet.h"
#include "wavepcm.h"
#include "loadfile.h"

int Load_Diskset (char *set_name);

void test_Debuger_display (int n);
extern UINT64 g_SingleStepping_Cycles;

namespace ra2
{

  static BYTE _key_translate (unsigned keycode)
  {
    BYTE ch;
    bool ctrl_down =  KeybGetCtrlStatus ();
    bool shift_down = KeybGetShiftStatus ();
    bool caps_lock_down = KeybGetCapsStatus ();
    bool meta_down = KeybGetMetaStatus ();
    
    if (keycode >= RETROK_a && keycode <= RETROK_z) {
      if (ctrl_down) {
        ch = (keycode - RETROK_a) + 0x01; // Ctrl-A till Ctrl-Z.
      } else {
        if (caps_lock_down) {
          if (shift_down) {
            ch = (keycode - RETROK_a) + 0x01 + 0x40;
          } else {
            ch = (keycode - RETROK_a) + 0x01 + 0x60;
          }
        } else {
          if (shift_down) {
            ch = (keycode - RETROK_a) + 0x01 + 0x60;
          } else {
            ch = (keycode - RETROK_a) + 0x01 + 0x40;
          }
        }
      }
      return ch;
    } else {
      switch (keycode) {
        case RETROK_0: return shift_down ? ')' : '0'; break;
        case RETROK_1: return shift_down ? '!' : '1'; break;
        case RETROK_2: return shift_down ? '@' : '2'; break;
        case RETROK_3: return shift_down ? '#' : '3'; break;
        case RETROK_4: return shift_down ? '$' : '4'; break;
        case RETROK_5: return shift_down ? '%' : '5'; break;
        case RETROK_6: return shift_down ? '^' : '6'; break;
        case RETROK_7: return shift_down ? '&' : '7'; break;
        case RETROK_8: return shift_down ? '*' : '8'; break;
        case RETROK_9: return shift_down ? '(' : '9'; break;
        case RETROK_BACKQUOTE:    return shift_down ? '~' : '`'; break;
        case RETROK_MINUS:        return shift_down ? '_' : '-'; break;
        case RETROK_EQUALS:       return shift_down ? '+' : '='; break;
        case RETROK_LEFTBRACKET:  return shift_down ? '{' : '['; break;
        case RETROK_RIGHTBRACKET: return shift_down ? '}' : ']'; break;
        case RETROK_BACKSLASH:    return shift_down ? '|' : '\\'; break;
        case RETROK_SEMICOLON:    return shift_down ? ':' : ';'; break;
        case RETROK_QUOTE:        return shift_down ? '"' : '\''; break;
        case RETROK_COMMA:        return shift_down ? '<' : ','; break;
        case RETROK_PERIOD:       return shift_down ? '>' : '.'; break;
        case RETROK_SLASH:        return shift_down ? '?' : '/'; break;
        case RETROK_BACKSPACE:    return 0x8;
        case RETROK_RETURN:       return 0xd;
        case RETROK_TAB:          return 0x9;
        case RETROK_SPACE:        return 0x20;
        case RETROK_ESCAPE:       return 0x1b;
        default: return 0x7f;
      }
    }
    return 0x7f;
  }
  static void _update_key_modifiers (unsigned keycode, bool state)
  {
    switch (keycode) {
      case RETROK_LSHIFT:  UpdateKeyModifiers (KEYMOD_LSHIFT, state); break;
      case RETROK_RSHIFT:  UpdateKeyModifiers (KEYMOD_RSHIFT, state); break;
      case RETROK_LCTRL:   UpdateKeyModifiers (KEYMOD_LCTRL, state); break;
      case RETROK_RCTRL:   UpdateKeyModifiers (KEYMOD_RCTRL, state); break;
      case RETROK_LALT:    UpdateKeyModifiers (KEYMOD_LALT, state); break;
      case RETROK_RALT:    UpdateKeyModifiers (KEYMOD_RALT, state); break;
      case RETROK_LMETA:    UpdateKeyModifiers (KEYMOD_LMETA, state); break;
      case RETROK_RMETA:    UpdateKeyModifiers (KEYMOD_RMETA, state); break;
      case RETROK_CAPSLOCK: UpdateKeyModifiers (KEYMOD_CAPS, state); break;
    }
  }
  static bool _ProcessHotKey (UINT16 ch)
  {
    switch (ch) {
      case RETROK_F2:  // Reset key
        putKeyInBuffer(VK_RESET);
        log_cb(RETRO_LOG_INFO, "RA2: %s - add F2 reset key\n", __FUNCTION__);
        return true;
        break;
      case RETROK_F7:  // Enter debugger key
      case RETROK_MENU:
      //case RETROK_RMETA:
        log_cb(RETRO_LOG_INFO, "RA2: %s - add F7 debug key\n", __FUNCTION__);
        putKeyInBuffer(VK_DEBUG);
        return true;
        break;
      case RETROK_F9:  // Change display output mode
        log_cb(RETRO_LOG_INFO, "RA2: %s - add F9 displey key\n", __FUNCTION__);
        putKeyInBuffer(VK_DISPLAY);
        return true;
        break;
      case RETROK_F10:
        break;
      case RETROK_F11:
        break;
      case RETROK_F12:
        break;
      case RETROK_INSERT: // Shift-Ins: Paste key, Ctrl-Ins: Copy key.
        if (!KeybGetCtrlStatus() && KeybGetShiftStatus()) {
          putKeyInBuffer(VK_PASTE);
          return true;
        } else if (KeybGetCtrlStatus()) {
          putKeyInBuffer(VK_COPY);
          return true;
        }
        break;
      case RETROK_v: // Command-V: Paste key on mac
        if (KeybGetMetaStatus ()) {
          putKeyInBuffer(VK_PASTE);
          return true;
        }
        break;
      case RETROK_c: // Command-C: Copy key on mac
        if (KeybGetMetaStatus ()) {
          putKeyInBuffer(VK_COPY);
          return true;
        }
        break;
    }
    return false; // not processed.
  }
  
  unsigned Game::ourInputDevices[MAX_PADS] = {RETRO_DEVICE_NONE};

  Game::Game()
    :myButtonStates(RETRO_DEVICE_ID_JOYPAD_R3 + 1)
  {
    log_cb(RETRO_LOG_INFO, "~~~~~~~~~~~~~~~~~~~~~RA2: Game::Game()\n");
    
    myLoggerContext.reset(new LoggerContext(false));
    myFrame.reset(new ra2::RetroFrame());
    Init_Diskset (256);
  	static const char *wave_file_names [] = {
  	  "525_spin_start_empty.wav",   //00 :in use
  	  "525_spin_start_loaded.wav",  //01 
  	  "525_step_1_1.wav",           //02 
  	  "diskette_in.wav",            //03 
  	  "diskette_out.wav",           //04 
  	  "diskette_out_in.wav",        //05 
  	  NULL};
  	//std::string pcm_wave_resource_file("resource.zip");
  	//pcm_wave_resource_file = myFrame->getResourcePath(pcm_wave_resource_file);
  	init_wave_pcm (wave_file_names, "resource.zip");//pcm_wave_resource_file.c_str());

    SetFrame(myFrame);
    myFrame->Begin();

    Video & video = GetVideo();
    // should the user be allowed to tweak 0.75
    myMouse[0] = {0.0, 0.75 / video.GetFrameBufferBorderlessWidth(), RETRO_DEVICE_ID_MOUSE_X};
    myMouse[1] = {0.0, 0.75 / video.GetFrameBufferBorderlessHeight(), RETRO_DEVICE_ID_MOUSE_Y};
    framecounter = 0;
    memset (direction_keys_pressed, 0, sizeof(direction_keys_pressed));
  }

  Game::~Game()
  {
    log_cb(RETRO_LOG_INFO, "~~~~~~~~~~~~~~~~~~~~~RA2: Game::~Game()\n");
    myFrame->End();
    myFrame.reset();
    SetFrame(myFrame);
    Quit_Diskset ();
    quit_wave_pcm ();
    quit_mem_FILE ();
    
  }

  void Game::executeOneFrame()
  {
    framecounter ++;
    if (!warm_up_reset_done && framecounter == WARM_UP_RESET_FRAMES) {
      warm_up_reset_done = true;
      ResetMachineState ();
      //log_cb(RETRO_LOG_INFO, "RA2: %s - warming up reset\n", __FUNCTION__);
      return;
    }
    // Process hot keys that might change the main state here to avoid conflicts
    // by keeping important changes in one single thread.
    UINT32 preview_key = PeekKeyInBuffer ();
    switch (preview_key) {
      case VK_RESET:
        ClearKeyBuffer ();
        ResetMachineState ();
        log_cb(RETRO_LOG_INFO, "RA2: %s - reset key\n", __FUNCTION__);
        break;
      case VK_DEBUG:
        ClearKeyBuffer ();
        if (g_nAppMode == MODE_RUNNING) {
          DebugBegin ();
        } else if (g_nAppMode == MODE_STEPPING) {
          GetFrame().VideoRedrawScreen();
          DebugExitDebugger ();
          DebugBegin ();
        } else {
          DebugExitDebugger ();
          CmdGoNormalSpeed(0);
          //g_nAppMode = MODE_RUNNING;
          GetFrame().VideoRedrawScreen();
        }
        log_cb(RETRO_LOG_INFO, "RA2: %s - debugger key...%x\n", __FUNCTION__, preview_key);
        break;
      case VK_DISPLAY:
        ClearKeyBuffer ();
				GetVideo().IncVideoType();
				GetVideo().VideoReinitialize(false);
        break;
      case VK_PASTE:
        ClearKeyBuffer ();
        //addTextToBuffer ((char *)"Hello\n");
        new_clipboard_data ();
        break;
      case VK_COPY:
        if (KeybGetShiftStatus() || !GetVideo().VideoGetSWTEXT()) { // Copy graphics screen.
          int width, height, pitch;
          uint32_t *data = (uint32_t *) myFrame->CopyFrameBufferContents (&width, &height, &pitch);
          put_image_to_clipboard (data, width, height, pitch);
          delete [] data;
        } else { // Copy text screen.
          if (g_nAppMode == MODE_DEBUG) {
            char *data;
            Util_GetDebuggerText (data);
            put_text_to_clipboard (data);
          } else {
            char *data;
            Util_GetTextScreen (data);
            put_text_to_clipboard (data);
          }
        }
        ClearKeyBuffer ();
        break;
      default:
        ;
        //if (preview_key)
        //  log_cb(RETRO_LOG_INFO, "RA2: %s - not hotkey...%x\n", __FUNCTION__, preview_key);
    }
    check_clipboard_data ();
    pause_wave_pcm (g_nAppMode == MODE_DEBUG);

    if (g_nAppMode == MODE_DEBUG) {
      uint8_t wu = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP);
      if (wu) {
        addKeyToBuffer(VK_UP);
        //log_cb(RETRO_LOG_INFO, "RA2: %s - wu=%d\n", __FUNCTION__, wu);
      }
      uint32_t wd = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN);
      if (wd) {
        addKeyToBuffer(VK_DOWN);
        //log_cb(RETRO_LOG_INFO, "RA2: %s - wd=%d\n", __FUNCTION__, wd);
      }
      int mm_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      if (mm_y) {
        if (mm_y<=-1) {
          while (mm_y<0) {
            addKeyToBuffer(VK_DOWN); mm_y+=12;
          }
        } else if (mm_y>=1){
          while (mm_y>0) {
            addKeyToBuffer(VK_UP); mm_y-=12;
          }
        }
        //log_cb(RETRO_LOG_INFO, "RA2: %s - mm_y=%d\n", __FUNCTION__, mm_y);
      }
      UINT32 ch;
      while (HasKeyInBuffer ()) {
        ch = GetOneKeyFromBuffer ();
        //log_cb(RETRO_LOG_INFO, "RA2: %s - preview_key=%x ch=%x\n", __FUNCTION__, preview_key, ch);

        unsigned key_code = ch >> 16;
        unsigned char_code = ch & 0xffff;
        
        if ((key_code >= 32) && (key_code <= 127)) {
          DebuggerProcessKey (key_code);
        } else {
          DebuggerProcessKey (char_code);
        }
          
        if (char_code > 0 && char_code <= 127) {
          //log_cb(RETRO_LOG_INFO, "RA2: %s - sends %c:%x to DebuggerInputConsoleChar\n", __FUNCTION__, char_code, char_code);
          DebuggerInputConsoleChar (char_code);
        }
      }
   		if ((framecounter&0xf) == 0) { // cursor flashes every 16 frames~0.26s
   		  DebuggerCursorNext();
        DrawConsoleCursor();
      }

    } else if (g_nAppMode == MODE_STEPPING) {
      if (IsDebugSteppingAtFullSpeed ()) {
        int limit = 1000000/60*20; // to run at 20 times Apple II speed.
        g_SingleStepping_Cycles = 0;
        do {
          DebugContinueStepping ();
          if (g_SingleStepping_Cycles >= limit) {
            GetFrame().VideoRedrawScreen();
          	//CmdWindowViewOutput (0);
            break;
          }
        } while (g_nAppMode == MODE_STEPPING);

      } else {
        const bool bVideoUpdate = true;
        const UINT dwClksPerFrame = NTSC_GetCyclesPerFrame();
        uint64_t cyclesToExecute = NTSC_GetCyclesPerFrame();

	      //bool g_bFullSpeed =	 (g_dwSpeed == SPEED_MAX) || 
				//	 bScrollLock_FullSpeed ||
				//	 (GetCardMgr().GetDisk2CardMgr().IsConditionForFullSpeed() && !Spkr_IsActive() && !MB_IsActive()) ||
				//	 IsDebugSteppingAtFullSpeed();
				
        bool diskFullSpeed = GetCardMgr().GetDisk2CardMgr().IsConditionForFullSpeed();
        if (diskFullSpeed && !Spkr_IsActive()) cyclesToExecute*=10; // 10 times Apple II speed.
				g_SingleStepping_Cycles = 0;
        do {
          DebugContinueStepping ();
        } while (g_nAppMode == MODE_STEPPING && g_SingleStepping_Cycles < cyclesToExecute);
        unsigned executedCycles = g_SingleStepping_Cycles;
        
        g_dwCyclesThisFrame = (g_dwCyclesThisFrame + executedCycles) % dwClksPerFrame;
        GetCardMgr().Update(executedCycles);
        int generated = SpkrUpdate(executedCycles);        
  
      }
     
      //if (g_nAppMode != MODE_STEPPING) ClearKeyBuffer ();
      
    } else if (g_nAppMode == MODE_RUNNING) {
      const bool bVideoUpdate = true;
      const UINT dwClksPerFrame = NTSC_GetCyclesPerFrame();
      uint64_t cyclesToExecute = dwClksPerFrame;
      bool diskFullSpeed = GetCardMgr().GetDisk2CardMgr().IsConditionForFullSpeed();
      if (diskFullSpeed && !Spkr_IsActive()) cyclesToExecute*=10; // 10 times Apple II speed.
      const DWORD executedCycles = CpuExecute(cyclesToExecute, bVideoUpdate);
      g_dwCyclesThisFrame = (g_dwCyclesThisFrame + executedCycles) % dwClksPerFrame;
      GetCardMgr().Update(executedCycles);
      int generated = SpkrUpdate(executedCycles);        
    }
  }

  void Game::processInputEvents()
  {
    input_poll_cb();
    keyboardEmulation();
    mouseEmulation();
  }

  void Game::keyboardCallback(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
  {
    if (down)
    {
      //LogOutput ("key down %d c=%x m=%x\n", keycode, character, key_modifiers);
      processKeyDown(keycode, character, key_modifiers);
    }
    else
    {
      //LogOutput ("key down %d c=%x m=%x\n", keycode, character, key_modifiers);
      processKeyUp(keycode, character, key_modifiers);
    }
  }

  void Game::processKeyDown(unsigned keycode, uint32_t character, uint16_t key_modifiers)
  {
    UINT32 ch = 0;
    _update_key_modifiers (keycode, true);
    
    switch (keycode) {
      case RETROK_z:
      case RETROK_RALT:
        Paddle::setButtonPressed(Paddle::ourSolidApple);
        break;
      case RETROK_x:
      case RETROK_LMETA:
      case RETROK_RMETA:
      case RETROK_LALT:
        Paddle::setButtonPressed(Paddle::ourOpenApple);
        break;
      case RETROK_UP:
      case RETROK_KP8:
        direction_keys_pressed[0] = 1;
        Paddle::instance->putAxis(1, -1.0);
        break;
      case RETROK_DOWN:
      case RETROK_KP2:
        direction_keys_pressed[1] = 1;
        Paddle::instance->putAxis(1, 1.0);
        break;
      case RETROK_LEFT:
      case RETROK_KP4:  
        direction_keys_pressed[2] = 1;
        Paddle::instance->putAxis(0, -1.0);
        break;
      case RETROK_RIGHT:
      case RETROK_KP6:  
        direction_keys_pressed[3] = 1;
        Paddle::instance->putAxis(0, 1.0);
        break;
    }
    
    if (_ProcessHotKey (keycode)) 
      return;
    
    if (g_nAppMode == MODE_RUNNING || g_nAppMode == MODE_STEPPING) {
      if (keycode <= RETROK_DELETE) { // 127
        ch = _key_translate (keycode);
      } else {
        switch (keycode) {
          case RETROK_LEFT: ch = 0x08; break;
          case RETROK_RIGHT: ch = 0x15; break;
          case RETROK_UP: ch = 0x0b; break;
          case RETROK_DOWN: ch = 0x0a; break;
        }
      }
      if (ch) {
        //addKeyToBuffer(ch);
        putKeyInBuffer(ch);
      }
      log_cb(RETRO_LOG_INFO, "RA2: %s - running mode keycode %d=> ch$%x\n", __FUNCTION__, keycode, ch);
    } else if (g_nAppMode == MODE_DEBUG) {
      if (keycode <= RETROK_DELETE && keycode > RETROK_SPACE) { // 127..33
        ch = _key_translate (keycode);
      } else {
        switch (keycode) {
          case RETROK_SPACE:  ch = VK_SPACE; break;
          case RETROK_ESCAPE: ch = VK_ESCAPE; break;
          case RETROK_RETURN: ch = VK_RETURN; break;
          case RETROK_TAB:    ch = VK_TAB; break;
          case RETROK_BACKSPACE: ch = VK_BACK; break;
          case RETROK_LEFT:   ch = VK_LEFT; break;
          case RETROK_RIGHT:  ch = VK_RIGHT; break;
          case RETROK_UP:     ch = VK_UP; break;
          case RETROK_DOWN:   ch = VK_DOWN; break;
          case RETROK_HOME:   ch = VK_HOME; break;
          case RETROK_END:    ch = VK_END; break;
          case RETROK_PAGEUP:  ch = VK_PRIOR; break;
          case RETROK_PAGEDOWN: ch = VK_NEXT; break;
          case RETROK_F1:     ch = VK_F1 ; break;
          case RETROK_F2:     ch = VK_F2 ; break;
          case RETROK_F3:     ch = VK_F3 ; break;
          case RETROK_F4:     ch = VK_F4 ; break;
          case RETROK_F5:     ch = VK_F5 ; break;
          case RETROK_F6:     ch = VK_F6 ; break;
          case RETROK_F7:     ch = VK_F7 ; break;
          case RETROK_F8:     ch = VK_F8 ; break;
          case RETROK_F9:     ch = VK_F9 ; break;
          case RETROK_F10:    ch = VK_F10; break;
          case RETROK_F11:    ch = VK_F11; break;
          case RETROK_F12:    ch = VK_F12; break;
          case RETROK_F13:    ch = VK_F13; break;
          case RETROK_F14:    ch = VK_F14; break;
          case RETROK_F15:    ch = VK_F15; break;
        }
      }
      if (keycode == RETROK_BACKQUOTE) { keycode = 0; ch = VK_OEM_3; }
      if (ch) {
        addKeyToBuffer(ch | (keycode<<16));
        log_cb(RETRO_LOG_INFO, "RA2: %s - debug mode %d=>$%02x %x (%d,%d,%d)\n", __FUNCTION__, keycode, ch, key_modifiers, KeybGetShiftStatus(), KeybGetCtrlStatus(), KeybGetAltStatus());
      }
    }
    
  }

  uint8_t Game::direction_keys_pressed[4];
  void Game::processKeyUp(unsigned keycode, uint32_t character, uint16_t key_modifiers)
  {
    _update_key_modifiers (keycode, false);
    log_cb(RETRO_LOG_INFO, "RA2: %s - keycode %d\n", __FUNCTION__, keycode);
    switch (keycode) {
      case RETROK_z:
      case RETROK_RALT:
        Paddle::setButtonReleased(Paddle::ourSolidApple);
        break;
      case RETROK_x:
      case RETROK_LMETA:
      case RETROK_RMETA:
      case RETROK_LALT:
        Paddle::setButtonReleased(Paddle::ourOpenApple);
        break;
      case RETROK_UP:
      case RETROK_KP8:
        direction_keys_pressed[0] = 0;
        break;
      case RETROK_DOWN:
      case RETROK_KP2:
        direction_keys_pressed[1] = 0;
        break;
      case RETROK_LEFT:
      case RETROK_KP4:
        direction_keys_pressed[2] = 0;
        break;
      case RETROK_RIGHT:
      case RETROK_KP6:
        direction_keys_pressed[3] = 0;
        break;
    }
    if (!direction_keys_pressed[0] && !direction_keys_pressed[1]) Paddle::instance->putAxis(1, 0.0);
    if (!direction_keys_pressed[2] && !direction_keys_pressed[3]) Paddle::instance->putAxis(0, 0.0);
  }

  bool Game::checkButtonPressed(unsigned id)
  {
    // pressed if it is down now, but was up before
    const int value = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, id);
    const bool pressed = (value != 0) && myButtonStates[id] == 0;

    // update to avoid multiple fires
    myButtonStates[id] = value;

    return pressed;
  }


  void Game::keyboardEmulation()
  {
    if (ourInputDevices[0] != RETRO_DEVICE_NONE)
    {
      if (checkButtonPressed(RETRO_DEVICE_ID_JOYPAD_R))
      {
        // myFrame->CycleVideoType();
      }
      if (checkButtonPressed(RETRO_DEVICE_ID_JOYPAD_L))
      {
        // myFrame->Cycle50ScanLines();
      }
      if (checkButtonPressed(RETRO_DEVICE_ID_JOYPAD_START))
      {
        // ResetMachineState();
      }
    }
    else
    {
      std::fill(myButtonStates.begin(), myButtonStates.end(), 0);
    }
  }

  void Game::mouseEmulation()
  {
    for (auto & mouse : myMouse)
    {
      const int16_t x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, mouse.id);
      mouse.position += x * mouse.multiplier;
      mouse.position = std::min(1.0, std::max(mouse.position, -1.0));
    }
  }

  double Game::getMousePosition(int i) const
  {
    return myMouse[i].position;
  }

  bool Game::loadSnapshot(const std::string & path)
  {
    common2::setSnapshotFilename(path);
    myFrame->LoadSnapshot();
    return true;
  }

  DiskControl & Game::getDiskControl()
  {
    return myDiskControl;
  }

}
