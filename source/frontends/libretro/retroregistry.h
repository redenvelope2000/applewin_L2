#pragma once

#include <memory>

class Registry;

namespace ra2
{

  void SetupRetroVariables();
  void CreateRetroRegistry();
  void QuitRetroRegistry();

}


BOOL RegLoadString (LPCTSTR section, LPCTSTR key, BOOL peruser, LPTSTR buffer, DWORD chars);
BOOL RegLoadValue (LPCTSTR section, LPCTSTR key, BOOL peruser, DWORD *value);
BOOL RegLoadValue (LPCTSTR section, LPCTSTR key, BOOL peruser, BOOL *value);
void RegSaveString (LPCTSTR section, LPCTSTR key, BOOL peruser, const std::string & buffer);
void RegSaveString (LPCTSTR section, LPCTSTR key, BOOL peruser, char *buffer);
void RegSaveValue (LPCTSTR section, LPCTSTR key, BOOL peruser, DWORD value);

// --------------------------------------------------------------------
#include <string.h>

struct simple_registry_item {
  char *key_string;
  UINT32 value;
  char *value_string;
};
class simple_registry {
public:
  simple_registry(int limit=256);
  ~simple_registry();
  bool getString (char *section, char *key, bool peruser, UINT8 *buffer, int size_buffer);
  bool putString (char *section, char *key, bool peruser, UINT8 *str);
  bool putString (char *section, char *key, bool peruser, std::string &str);
  bool getUINT32 (char *section, char *key, bool peruser, UINT32 *value);
  bool putUINT32 (char *section, char *key, bool peruser, UINT32 value);
  static simple_registry *simple_registry_instance;
 
  void test_registry (char *section, char *key, int cmd);
private:
  int find_item (char *section, char *key, bool peruser);
  int limit_size_data_base;
  int cur_size_data_base; // number of valid items, also indicates the current write pointer.
  simple_registry_item *data_base;
};
