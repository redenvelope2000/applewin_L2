#pragma once

#include "wincompat.h"

void addKeyToBuffer(UINT32 key);
void putKeyInBuffer(UINT32 key);
void addTextToBuffer(const char * text);
bool HasKeyInBuffer ();
UINT32 GetOneKeyFromBuffer ();
UINT32 PeekKeyInBuffer ();
void ClearKeyBuffer ();
void KeybReset();

enum KeyModifiers {
  KEYMOD_LSHIFT, KEYMOD_RSHIFT, KEYMOD_LCTRL, KEYMOD_RCTRL, KEYMOD_LALT, KEYMOD_RALT, KEYMOD_LMETA, KEYMOD_RMETA, KEYMOD_CAPS
};
void UpdateKeyModifiers (KeyModifiers mod, bool state);

bool KeybGetCapsStatus();
bool KeybGetShiftStatus();
bool KeybGetAltStatus();
bool KeybGetCtrlStatus();
bool KeybGetMetaStatus();
