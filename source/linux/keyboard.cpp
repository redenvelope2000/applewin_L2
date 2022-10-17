#include "StdAfx.h"
#include "linux/keyboard.h"

#include "Core.h"
#include "YamlHelper.h"

#include <queue>

namespace
{
  std::queue<UINT32> keys;
  BYTE keycode = 0;
  bool g_bCapsLock = false;//true; //Caps lock key for Apple2 and Lat/Cyr lock for Pravets8
  bool g_bCtrlDown = false;
  bool g_bShiftDown = false;
  bool g_bMetaDown = false;
  bool g_bAltDown = false;
  bool g_bLAltDown = false;
  bool g_bRAltDown = false;
  bool g_bLShiftDown = false;
  bool g_bRShiftDown = false;
  bool g_bLCtrlDown = false;
  bool g_bRCtrlDown = false;
  bool g_bLMetaDown = false;
  bool g_bRMetaDown = false;
}

volatile static BYTE _KB_Buffer_door = 0;
static void wait_and_close_KB_Buffer ()
{
  while (_KB_Buffer_door) {}
  _KB_Buffer_door = 1;
}
static void open_KB_Buffer ()
{
  _KB_Buffer_door = 0;
}
void addKeyToBuffer(UINT32 key)
{
  wait_and_close_KB_Buffer ();
  keys.push(key);
  open_KB_Buffer ();
}
bool HasKeyInBuffer ()
{
  return !keys.empty();
}
UINT32 PeekKeyInBuffer ()
{
  UINT32 data = 0;
  wait_and_close_KB_Buffer ();
  if (!keys.empty())
  {
    data = keys.front();
  }
  open_KB_Buffer ();
  return data;
}
UINT32 GetOneKeyFromBuffer ()
{
  UINT32 data = 0;
  wait_and_close_KB_Buffer ();
  if (!keys.empty())
  {
    data = keys.front();
    keys.pop();
  }
  open_KB_Buffer ();
  return data;
}
void ClearKeyBuffer ()
{
  KeybReset();
  //while (HasKeyInBuffer ()) GetOneKeyFromBuffer ();
}
void putKeyInBuffer(UINT32 key)
{
  wait_and_close_KB_Buffer ();
  
  if (keys.empty())
  {
    keys.push(key);
  }
  else
  {
    keys.front() = key;
  }
  
  open_KB_Buffer ();
}

void UpdateKeyModifiers (KeyModifiers mod, bool state)
{
  switch (mod) {
    case KEYMOD_LSHIFT: g_bLShiftDown = state; break;
    case KEYMOD_RSHIFT: g_bRShiftDown = state; break;
    case KEYMOD_LCTRL:  g_bLCtrlDown = state; break;
    case KEYMOD_RCTRL:  g_bRCtrlDown = state; break; 
    case KEYMOD_LALT:   g_bLAltDown = state; break;
    case KEYMOD_RALT:   g_bRAltDown = state; break;
    case KEYMOD_LMETA:  g_bLMetaDown = state; break;
    case KEYMOD_RMETA:  g_bRMetaDown = state; break;
    case KEYMOD_CAPS:   g_bCapsLock = state; break;
  }
  g_bCtrlDown = g_bLCtrlDown | g_bRCtrlDown;
  g_bAltDown = g_bLAltDown | g_bRAltDown;
  g_bShiftDown = g_bLShiftDown | g_bRShiftDown;
  g_bMetaDown = g_bLMetaDown | g_bRMetaDown;
}

bool KeybGetCapsStatus()
{
  return g_bCapsLock;
}

BYTE KeybGetKeycode()
{
  return keycode;
}

BYTE KeybReadData()
{
  LogFileTimeUntilFirstKeyRead();

  if (keys.empty())
  {
    return keycode;
  }
  else
  {
    keycode = keys.front() & 0x7f;
    const BYTE result = keycode | 0x80;
    return result;
  }
}

BYTE KeybReadFlag()
{
  if (!keys.empty())
  {
    wait_and_close_KB_Buffer ();
    keys.pop();
    open_KB_Buffer ();
  }

  return KeybReadData();
}

#define SS_YAML_KEY_LASTKEY "Last Key"
#define SS_YAML_KEY_KEYWAITING "Key Waiting"

static std::string KeybGetSnapshotStructName(void)
{
  static const std::string name("Keyboard");
  return name;
}

void KeybSaveSnapshot(YamlSaveHelper& yamlSaveHelper)
{
  YamlSaveHelper::Label state(yamlSaveHelper, "%s:\n", KeybGetSnapshotStructName().c_str());
  yamlSaveHelper.SaveHexUint8(SS_YAML_KEY_LASTKEY, keycode);
  yamlSaveHelper.SaveBool(SS_YAML_KEY_KEYWAITING, keys.empty() ? false : false);
}

void KeybLoadSnapshot(YamlLoadHelper& yamlLoadHelper, UINT version)
{
  if (!yamlLoadHelper.GetSubMap(KeybGetSnapshotStructName()))
    return;

  keycode = (BYTE) yamlLoadHelper.LoadUint(SS_YAML_KEY_LASTKEY);

  bool keywaiting = false;
  if (version >= 2)
    keywaiting = yamlLoadHelper.LoadBool(SS_YAML_KEY_KEYWAITING);

  KeybReset();
  //nao- addKeyToBuffer(keycode);

  yamlLoadHelper.PopMap();
}

void KeybReset()
{
  std::queue<UINT32>().swap(keys);
}

bool KeybGetShiftStatus()
{
  return g_bShiftDown;
}

bool KeybGetAltStatus()
{
  return g_bAltDown;
}

bool KeybGetCtrlStatus()
{
  return g_bCtrlDown;
}

bool KeybGetMetaStatus()
{
  return g_bMetaDown;
}

void addTextToBuffer(const char * text)
{
   while (*text)
   {
     switch (*text)
     {
       case '\n':
       {
        addKeyToBuffer(0x0d);
        break;
       }
       case 0x20 ... 0x7e:
       {
        addKeyToBuffer(*text);
        break;
       }
     }
     ++text;
   }
}
