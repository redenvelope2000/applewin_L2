#include "StdAfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

struct diskset {
  int num_items;
  int max_items;
  char *name;
  char *folder;
  char **uname;
  char **fname;
  int d1_default;
  int d2_default;
};
// syntax of name.set: 
//; comment
//disk-name disk-path
//...
//; default disks
//D1 disk-name
//D2 disk-name

static diskset *DiskSet = NULL;
static void Reset_Diskset ()
{
  if (DiskSet->uname && DiskSet->fname) {
    for (int i=0; i<DiskSet->num_items; i++) {
      if (DiskSet->uname[i]) delete[] DiskSet->uname[i];
      if (DiskSet->fname[i]) delete[] DiskSet->fname[i];
    }
  }
  if (DiskSet->name) delete[] DiskSet->name; DiskSet->name = NULL;
  if (DiskSet->folder) delete[] DiskSet->folder; DiskSet->folder = NULL;
  DiskSet->num_items = 0;
  DiskSet->d1_default = 0;
  DiskSet->d2_default = -1;
}
static int find_item (char *u)
{
  for (int i=0; i<DiskSet->num_items; i++) {
    if (strcmp (DiskSet->uname[i], u) == 0) return i;
  }
  return -1;
}
static int add_item (char *u, char *f)
{
  if (DiskSet->num_items < DiskSet->max_items) {
    int index = DiskSet->num_items;
    DiskSet->uname[index] = new char[strlen (u) + 1]; strcpy (DiskSet->uname[index], u);
    DiskSet->fname[index] = new char[strlen (f) + 1]; strcpy (DiskSet->fname[index], f);
    DiskSet->num_items++;
    return DiskSet->num_items;
  }
  return -1;
}
int Init_Diskset (int max)
{
  if (DiskSet != NULL) //??
    DiskSet = NULL;

  DiskSet = new diskset;
  memset (DiskSet, 0, sizeof (*DiskSet));
  DiskSet->max_items = max;
  DiskSet->name = NULL;
  DiskSet->uname = new char *[max]; memset (DiskSet->uname, 0, max * sizeof(char *));
  DiskSet->fname = new char *[max]; memset (DiskSet->fname, 0, max * sizeof(char *));
  Reset_Diskset ();
  return 0;
}
// Return 0 if succ.
int Quit_Diskset ()
{
  Reset_Diskset();
  delete[] DiskSet->uname;
  delete[] DiskSet->fname;
  delete DiskSet;
  return 0;
}

// Return 0 if succ.
int DisksetOpen (char *name)
{
  if (!name) return -1;
  if (DiskSet->num_items > 0) Reset_Diskset ();
  if (name[0] == '/' || name[0] =='\\' || (isalpha(name[0]) && name[1] == ':' && (name[2] == '/' || name[2] == '\\'))) { // The name provides an absolute path.
    int folder_len = -1;
    for (int i=(int)strlen (name) - 1; i >= 0; i--) {
      if (name[i] == '/' || name[i] == '\\') {
        folder_len = i+1;
        break;
      }
    }
    DiskSet->folder = NULL;
    if (folder_len > 0) {
      char *folder = new char[folder_len + 1];
      memcpy (folder, name, folder_len);
      folder[folder_len] = 0;
      DiskSet->folder = folder;
    }
  }

  FILE *f = fopen (name, "r");
  if (!f) return -1;

  uint8_t *buf = new uint8_t[4096];
  while (!feof(f)) {
    if (fgets ((char *)buf, 1024, f) == NULL) break;
    if (buf[0] == ';' || buf[0] == '#' || (buf[0] == '/' && buf[1] == '/')) {
      // comment line. try next one.
      continue;
    }
    // remove heading spaces.
    int sb;
    for (sb=0; buf[sb] && buf[sb]<=0x20; sb++);
    if (!buf[sb] || buf[sb] <= 0x20) { // nothing found, try next line.
      continue;
    }
    // remove trailing spaces.
    int se;
    for (se = (int) strlen ((char *)&buf[sb]) - 1; se>=sb && buf[sb+se] && buf[sb+se]<=0x20; se--) buf[sb+se]=0;
    if (!(se>=sb)) { // nothing left, try next line.
      continue;
    }
    char *token1 = (char*)&buf[sb]; // 1st token.
    char *token2 = NULL; 
    int sq;
    for (sq=sb; sq<=se && buf[sq] && buf[sq]!=0x20; sq++);
    if (sq<=se) { // has second token.
      while (buf[sq]==0x20) buf[sq++] = 0;
      int sr = sq;
      if (buf[sr]) {
        token2 = (char*)&buf[sr];
      }
    }
    if (strcasecmp (token1, "d1") == 0 || strcasecmp (token1, "d2") == 0) { // D1|D2 default disk.
      if (token2) {
        int index = -1;
        if (token2[0] == '#') {
          if (sscanf (&token2[1], "%d", &index) == 1 && index > 0) index--;
        } else {
          index = find_item (token2);
        }
        if (index >= 0) {
          if (strcasecmp (token1, "d1") == 0) DiskSet->d1_default = index;
          else if (strcasecmp (token1, "d2") == 0) DiskSet->d2_default = index;
        }
      }
      continue;
    }

    if (token1 && token2) {
      add_item (token1, token2);
      //printf ("%s %s\n", token1, token2);
    }
  }
  
  delete[] buf;
  fclose (f);
  DiskSet->name = new char [strlen (name) + 1]; strcpy (DiskSet->name, name);
  return 0;
}

char *DisksetName ()
{
  return DiskSet->name;
}

char *DisksetFolder ()
{
  return DiskSet->folder;
}

int DisksetNumItems ()
{
  return DiskSet->num_items;
}
// Return 0 if succ.
int DisksetGetItem (int index, char **uname, char **fname)
{
  if (index >= 0 && index < DiskSet->num_items) {
    *uname = DiskSet->uname[index];
    *fname = DiskSet->fname[index];
    return 0;
  }
  return -1;
}
// Return 0 if succ.
int DisksetGetItem (char *uname, char **fname)
{
  int index = find_item (uname);
  if (index >= 0) {
    *fname = DiskSet->fname[index];
    return 0;
  }
  return -1;
}

void DisksetGetDefault (int *d1, int *d2)
{
  *d1 = DiskSet->d1_default;
  *d2 = DiskSet->d2_default;
}
#if 0
main ()
{
  Init_Diskset (256);
  
  DisksetOpen ((char*)"p.set");
  char *dsname = DisksetName ();
  int dsn = DisksetNumItems ();
  
  for (int i=0; i<dsn; i++) {
    char *un, *fn;
    if (DisksetGetItem (i, &un, &fn) >= 0) {
      printf ("#%d - %s:%s\n",  i, un, fn);
    }
  }
 
  int id = find_item ((char *)"disk0");
  printf ("%s:%d:%s\n", DisksetName(), id, DiskSet->fname[id]);
  
  Quit_Diskset ();
}
#endif