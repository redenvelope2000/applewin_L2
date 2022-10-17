#include "StdAfx.h"
//#include "frontends/common2/ptreeregistry.h"
#include "frontends/libretro/environment.h"
#include "frontends/libretro/retroregistry.h"
//#include "linux/registry.h"

#include "Common.h"
#include "Card.h"
#include "Video.h"
#include "Core.h"

#include "libretro.h"

#include <list>
#include <vector>
#include <sstream>

namespace
{

  const std::string ourScope = "applewin_";

  struct Variable
  {
    std::string name;
    std::string description;
    std::string section;
    std::string key;
    std::vector<std::pair<std::string, DWORD> > values;
  };

  const std::vector<Variable> ourVariables =
    {
     {
      "machine",
      "Apple ][ type",
      REG_CONFIG,
      REGVALUE_APPLE2_TYPE,
      {
       {"Enhanced Apple //e", A2TYPE_APPLE2EENHANCED},
       {"Apple ][ (Original)", A2TYPE_APPLE2},
       {"Apple ][+", A2TYPE_APPLE2PLUS},
       {"Apple ][ J-Plus", A2TYPE_APPLE2JPLUS},
       {"Apple //e", A2TYPE_APPLE2E},
       {"Pravets 82", A2TYPE_PRAVETS82},
       {"Pravets 8M", A2TYPE_PRAVETS8M},
       {"Pravets 8A", A2TYPE_PRAVETS8A},
       {"Base64A", A2TYPE_BASE64A},
       {"TK3000 //e", A2TYPE_TK30002E},
      }
     },
     {
      "slot3",
      "Card in slot 3",
      "Configuration\\Slot 3",
      REGVALUE_CARD_TYPE,
      {
       {"Empty", CT_Empty},
       {"Video HD", CT_VidHD},
      }
     },
     {
      "slot4",
      "Card in slot 4",
      "Configuration\\Slot 4",
      REGVALUE_CARD_TYPE,
      {
       {"Empty", CT_Empty},
       {"Mouse", CT_MouseInterface},
       {"Mockingboard", CT_MockingboardC},
       {"Phasor", CT_Phasor},
      }
     },
     {
      "slot5",
      "Card in slot 5",
      "Configuration\\Slot 5",
      REGVALUE_CARD_TYPE,
      {
       {"Empty", CT_Empty},
       {"CP/M", CT_Z80},
       {"Mockingboard", CT_MockingboardC},
       {"SAM/DAC", CT_SAM},
      }
     },
     {
      "video",
      "Video mode",
      REG_CONFIG,
      REGVALUE_VIDEO_MODE,
      {
       {"Color (Composite Idealized)", VT_COLOR_IDEALIZED},
       {"Color (RGB Card/Monitor)", VT_COLOR_VIDEOCARD_RGB},
       {"Color (Composite Monitor)", VT_COLOR_MONITOR_NTSC},
       {"Color TV", VT_COLOR_TV},
       {"B&W TV", VT_MONO_TV},
       {"Monochrome (Amber)", VT_MONO_AMBER},
       {"Monochrome (Green)", VT_MONO_GREEN},
       {"Monochrome (White)", VT_MONO_WHITE},
      }
     },
     {
      "disk_speed",
      "Disk access speed",
      REG_CONFIG,
      REGVALUE_ENHANCE_DISK_SPEED,
      {
       {"Authentic speed", 0},
       {"Enhanced speed", 1},
      }
     },
     {
      "hard_disk_controller",
      "Hard disk controller",
      REG_CONFIG,
      REGVALUE_HDD_ENABLED,
      {
       {"No hard disk controller", 0},
       {"Enable hard disk controller in slot7", 1},
      }
     },
    };

  std::string getKey(const Variable & var)
  {
    std::ostringstream ss;
    ss << var.description << "; ";
    for (size_t i = 0; i < var.values.size(); ++i)
    {
      if (i > 0)
      {
        ss << "|";
      }
      ss << var.values[i].first;
    }
    return ss.str();
  }

}

namespace ra2
{

  void SetupRetroVariables()
  {
    ra2::log_cb(RETRO_LOG_INFO, "--------------------RA2: %s\n", __FUNCTION__);
    const size_t numberOfVariables = ourVariables.size();
    std::vector<retro_variable> retroVariables(numberOfVariables + 1);
    std::list<std::string> workspace; // so objects do not move when it resized

    // we need to keep the char * alive till after the call to RETRO_ENVIRONMENT_SET_VARIABLES
    const auto c_str = [&workspace] (const auto & s)
                       {
                         workspace.push_back(s);
                         return workspace.back().c_str();
                       };

    for (size_t i = 0; i < numberOfVariables; ++i)
    {
      const Variable & variable = ourVariables[i];
      retro_variable & retroVariable = retroVariables[i];

      retroVariable.key = c_str(ourScope + variable.name);
      retroVariable.value = c_str(getKey(variable));
    }

    environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, retroVariables.data());

    for (const Variable & variable : ourVariables)
    {
      const std::string retroKey = ourScope + variable.name;
      retro_variable retroVariable;
      retroVariable.key = retroKey.c_str();
      retroVariable.value = nullptr;
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &retroVariable) && retroVariable.value)
      {
        const std::string value(retroVariable.value);
        const auto check = [&value] (const auto & x)
                           {
                             return x.first == value;
                           };
        const auto it = std::find_if(variable.values.begin(), variable.values.end(), check);
        if (it != variable.values.end())
        {
          RegSaveValue(variable.section.c_str(), variable.key.c_str(), true, it->second);
        }
      }
    }

    /*
    ra2::log_cb(RETRO_LOG_INFO, "--------------------RA2: %s -------TEST-START-------\n", __FUNCTION__);
    {
      UINT32 dwTmp = 1;
      RegSaveValue((char*)"Config\\Bala", (char*)"Bala type#1", TRUE, dwTmp);
      dwTmp=-1;
      simple_registry::simple_registry_instance->test_registry((char*)"Config\\Bala", (char*)"Bala type#1", 0);
      if (RegLoadValue((char*)"Config\\Bala", (char*)"Bala type#1", TRUE, &dwTmp)) {
        ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - verified #1=%dn", __FUNCTION__, dwTmp);
      }
    }
    ra2::log_cb(RETRO_LOG_INFO, "--------------------RA2: %s -------TEST--END--------\n", __FUNCTION__);
    */
  }

  void QuitRetroRegistry()
  {
    if (simple_registry::simple_registry_instance) {
      delete simple_registry::simple_registry_instance; 
      simple_registry::simple_registry_instance = NULL;
    }
  }

  void CreateRetroRegistry()
  {
    ra2::log_cb(RETRO_LOG_INFO, "--------------------RA2: %s\n", __FUNCTION__);
    if (simple_registry::simple_registry_instance == NULL) {
      simple_registry *mySimpleRegistry = new simple_registry(128);
      simple_registry::simple_registry_instance = mySimpleRegistry;  
    }
  }

}

// --------------------------------------------------------------------
simple_registry *simple_registry::simple_registry_instance = NULL;
simple_registry::simple_registry(int limit)
{
  limit_size_data_base = limit;
  cur_size_data_base = 0;
  data_base = new simple_registry_item[limit];
}
simple_registry::~simple_registry()
{
  for (int index=0; index<cur_size_data_base; index++) {
    delete[] data_base[index].key_string;
    if (data_base[index].value_string) {
      delete[] data_base[index].value_string;
    }
  }
  delete [] data_base;
}

void simple_registry::test_registry (char *section, char *key, int cmd)
{
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - self=%x limit=%d size=%d\n", __FUNCTION__, this, limit_size_data_base, cur_size_data_base);
  for (int i=0; i<cur_size_data_base; i++) {
    ra2::log_cb(RETRO_LOG_INFO, "#%d : %s\n", i, data_base[i].key_string);
  }
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - now try to find key string %s%s\n", __FUNCTION__, section, key);
  int indexx;
  {
    int result = -1;
    char *key_string = new char[strlen (section) + strlen (key) + 1];
    strcpy (key_string, section); strcat (key_string, key);
    ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - key_string=%s\n", __FUNCTION__, key_string);
    for (int index=0; index<cur_size_data_base; index++) {
      ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - strcmp %s == %s, returns %d\n", __FUNCTION__, data_base[index].key_string, key_string, strcmp (data_base[index].key_string, key_string));
      if (strcmp (data_base[index].key_string, key_string) == 0) {
        result = index;
        break;
      }
    }
    delete[] key_string;
    indexx = result;
  }
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - find_item returns %d\n", __FUNCTION__, indexx);
}

// return the valid index of data_base[]. -1 when not found.
int simple_registry::find_item (char *section, char *key, bool peruser)
{
  int result = -1;
  char *key_string = new char[strlen (section) + strlen (key) + 1];
  strcpy (key_string, section); strcat (key_string, key);
  
  for (int index=0; index<cur_size_data_base; index++) {
    if (strcmp (data_base[index].key_string, key_string) == 0) {
      result = index;
      break;
    }
  }
  delete[] key_string;
  return result;
}
bool simple_registry::getUINT32 (char *section, char *key, bool peruser, UINT32 *value)
{
  bool result = false;
  int index = find_item (section, key, peruser);
  if (index >= 0) {
    simple_registry_item *item = &data_base[index];
    *value = item->value;
    result = true;
  }
  return result;
}
bool simple_registry::getString (char *section, char *key, bool peruser, UINT8 *buffer, int size_buffer)
{
  bool result = false;
  int index = find_item (section, key, peruser);
  if (index >= 0) {
    simple_registry_item *item = &data_base[index];
    if (size_buffer > strlen (item->value_string)) {
      strcpy ((char *)buffer, item->value_string);
      result = true;
    }
  }
  return result;
}
bool simple_registry::putUINT32 (char *section, char *key, bool peruser, UINT32 value)
{
  bool result = false;
  //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - UINT32 section=%s key=%s value=%d\n", __FUNCTION__, section, key, value);
  
  int index = find_item (section, key, peruser);
  if (index >= 0) {
    simple_registry_item *item = &data_base[index];
    item->value = value;
    //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - UINT32 Replace SUCC\n", __FUNCTION__);
    result = true;
  } else {
    if (cur_size_data_base < limit_size_data_base) {
      simple_registry_item *item = &data_base[cur_size_data_base];
      item->key_string = new char[strlen (section) + strlen (key) + 1];
      item->value_string = NULL;
      strcpy (item->key_string, section);
      strcat (item->key_string, key);
      item->value = value;
      cur_size_data_base++;
      //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - UINT32 Create SUCC #%d\n", __FUNCTION__, cur_size_data_base);
      result = true;
    }
  }
  return result;
}
bool simple_registry::putString(char *section, char *key, bool peruser, UINT8 *str)
{
  bool result = false;
  int index = find_item (section, key, peruser);
  if (index >= 0) {
    simple_registry_item *item = &data_base[index];
    if (item->value_string) delete [] item->value_string;
    item->value_string = new char[strlen ((const char*)str) + 1];
    strcpy (item->value_string, (const char*)str);
    item->value = 0;
    result = true;
  } else {
    if (cur_size_data_base < limit_size_data_base) {
      simple_registry_item *item = &data_base[cur_size_data_base];
      item->key_string = new char[strlen (section) + strlen (key) + 1];
      strcpy (item->key_string, section);
      strcat (item->key_string, key);
      item->value_string = new char[strlen ((const char*)str) + 1];
      strcpy (item->value_string, (const char*)str);
      item->value = 0;
      cur_size_data_base++;
      result = true;
    }
  }
  return result;
}

// --------------------------------------------------------------------
BOOL RegLoadString (LPCTSTR section, LPCTSTR key, BOOL peruser, LPTSTR buffer, DWORD chars, LPCTSTR defaultValue)
{
  BOOL success = RegLoadString(section, key, peruser, buffer, chars);
  if (!success)
    StringCbCopy(buffer, chars, defaultValue);
  return success;
}

BOOL RegLoadValue (LPCTSTR section, LPCTSTR key, BOOL peruser, DWORD* value, DWORD defaultValue) {
  BOOL success = RegLoadValue(section, key, peruser, value);
  if (!success)
    *value = defaultValue;
  return success;
}

BOOL RegLoadValue (LPCTSTR section, LPCTSTR key, BOOL peruser, BOOL *value)
{
  return FALSE;
}

BOOL RegLoadString (LPCTSTR section, LPCTSTR key, BOOL peruser, LPTSTR buffer, DWORD chars)
{
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - String section=%s key=%s\n", __FUNCTION__, section, key);
  BOOL result = FALSE;
  
  result = 
  simple_registry::simple_registry_instance->getString ((char *)section, (char *)key, 
                                                        peruser,
                                                        (UINT8 *)buffer, chars);

  if (result) ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - String value=%s\n", __FUNCTION__, key, buffer);
  else ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - FAILED\n", __FUNCTION__);
  return result;
}

BOOL RegLoadValue (LPCTSTR section, LPCTSTR key, BOOL peruser, DWORD *value)
{
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - DWORD section=%s key=%s\n", __FUNCTION__, section, key);
  BOOL result = FALSE;
  
  result = 
  simple_registry::simple_registry_instance->getUINT32  ((char *)section, (char *)key, 
                                                        peruser,
                                                        (UINT32 *)value);

  if (result) ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - DWORD value=%d\n", __FUNCTION__, *value);
  else ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - FAILED\n", __FUNCTION__);
  return result;
}

void RegSaveString (LPCTSTR section, LPCTSTR key, BOOL peruser, char *buffer)
{
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - String section=%s key=%s data=%s\n", __FUNCTION__, section, key, buffer);
  BOOL result = FALSE;
  
  result = 
  simple_registry::simple_registry_instance->putString ((char *)section, (char *)key, 
                                                        peruser,
                                                        (UINT8 *)buffer);
  //return result;
}

void RegSaveString (LPCTSTR section, LPCTSTR key, BOOL peruser, const std::string & buffer)
{
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - std::string section=%s key=%s data=%s\n", __FUNCTION__, section, key, buffer.c_str());
  BOOL result = FALSE;
  
  result = 
  simple_registry::simple_registry_instance->putString ((char *)section, (char *)key, 
                                                        peruser,
                                                        (UINT8 *)buffer.c_str());
  //return result;
}

void RegSaveValue (LPCTSTR section, LPCTSTR key, BOOL peruser, DWORD value)
{
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - String section=%s key=%s value=%d\n", __FUNCTION__, section, key, value);
  BOOL result = FALSE;
  
  result = 
  simple_registry::simple_registry_instance->putUINT32  ((char *)section, (char *)key, 
                                                        peruser,
                                                        value);
  //return result;
}

