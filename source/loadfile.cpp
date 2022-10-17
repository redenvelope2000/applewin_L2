#include "StdAfx.h"
#include "wincompat.h"
#include "Core.h"
#include "Interface.h"

#include "loadfile.h"

int load_zipped_file (const char *zipfile, const char *filename, unsigned char **buf, int *length);

// return the file size.
int FileSize(FILE *fp)
{
  int save_pos, size_of_file;

  save_pos = ftell(fp);
  fseek(fp, 0L, SEEK_END);
  size_of_file = ftell(fp);
  fseek(fp, save_pos, SEEK_SET);
  return(size_of_file);
}

// Load the file data. If the file "name" does not exist, it's loaded from the zip file "rsc_name".
// return 0 if succ.
// return the data pointer in *buffer, data size in *size.
// when the data is not needed anymore, free (*buffer) to avoid memory leak.
int LoadFile (const char *name, const char *rsc_name, UINT8 **buffer, int *size)
{
  FILE *f;
  UINT8 *buf;
  int len;

  f = fopen(name, "rb");
  if (f != NULL) {
    len = FileSize(f);
    if ((buf=(UINT8 *)malloc(len)) != NULL) {
      fread(buf, 1, len, f);
    }
    fclose(f);
    if (buf == NULL) return -1;
    *buffer = buf;
    *size = len;
    return 0;
  } else if (rsc_name != NULL) { // load from the zip file.
    if (load_zipped_file(rsc_name, name, buffer, size)) {
      return -1;
    }
    return 0;
  }
  return -1;
}

int LoadFile (const char *name, UINT8 **buffer, int *size)
{
  return LoadFile (name, NULL, buffer, size);
}

//---------------------------------------------------------------------
#define MAX_mem_files 20
static mem_FILE registered_mem_files[MAX_mem_files] = {0};
static mem_FILE mem_files[MAX_mem_files] = {0};
void init_mem_FILE ()
{
  memset (registered_mem_files, 0, sizeof(registered_mem_files));
  memset (mem_files, 0, sizeof(mem_files));
}
void quit_mem_FILE ()
{
  for (int i=0; i<MAX_mem_files; i++) {
    if (registered_mem_files[i].name) delete[] registered_mem_files[i].name;
  }
  init_mem_FILE ();
}
int register_mem_FILE (const char *name, uint8_t *mem_block, long size)
{
  for (int i=0; i<MAX_mem_files; i++) {
    if (registered_mem_files[i].name == NULL) {
      int index = i;
      int name_len = strlen (name) + 1;
      registered_mem_files[index].name = new char[name_len];
      strcpy (registered_mem_files[index].name, name);
      registered_mem_files[index].fp = 0;
      registered_mem_files[index].base = mem_block;
      registered_mem_files[index].size = size;
      ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %s %d size=%d %llx\n", __FUNCTION__, name, index, size, mem_block);
      return index;
    }
  }
  return -1;
}
int unregister_mem_FILE (const char *name)
{
  for (int i=0; i<MAX_mem_files; i++) {
    if (registered_mem_files[i].name && strcmp (registered_mem_files[i].name, name) == 0) {
      int index = i;
      ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %s %d\n", __FUNCTION__, name, index);
      delete[] registered_mem_files[index].name;
      registered_mem_files[index].name = NULL;
      return index;
    }
  }
  return -1;
}
mem_FILE *mem_fopen (const char *name, const char *mode)
{
  int index_registered = -1, index_filenum = -1;
  ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %s\n", __FUNCTION__, name);
  for (int i=0; i<MAX_mem_files; i++) {
    if (registered_mem_files[i].name && strcmp (registered_mem_files[i].name, name) == 0) {
      index_registered = i;
      break;
    }
  }
  if (index_registered < 0) return NULL;
  for (int i=0; i<MAX_mem_files; i++) {
    if (mem_files[i].name == NULL) {
      index_filenum = i;
      break;
    }
  }
  //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %s registered index=%d filenum=%d\n", __FUNCTION__, name, index_registered, index_filenum);
  mem_files[index_filenum] = registered_mem_files[index_registered];
  mem_files[index_filenum].fp = 0;
  //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %s size=%d %llx\n", __FUNCTION__, name, mem_files[index_filenum].size, mem_files[index_filenum].base);
  return &mem_files[index_filenum];
}
int mem_fclose (mem_FILE *f)
{
  f->name = NULL;
  return 0;
}
long mem_ftell(mem_FILE *stream)
{
  return stream->fp;
}
#define mem_SEEK_SET 0
#define mem_SEEK_END 1
int mem_fseek(mem_FILE *stream, long offset, int whence)
{
  if (whence == mem_SEEK_SET) {
    if (offset < 0 || offset > stream->size)
      return 1;
    stream->fp = offset;
  } else if (whence == mem_SEEK_END) {
    long pos = offset + stream->size;
    if (pos < 0 || pos > stream->size)
      return 1;
    stream->fp = pos;
  }
  return 0;
}
int mem_fread(void *p, int sz, int nitems, mem_FILE *stream)
{
  int cnt = sz*nitems;
  if (stream->fp + cnt > stream->size) {
    long remaining_size = stream->size - stream->fp;
    nitems = remaining_size/sz;
    cnt = sz*nitems;
  }
  if (cnt > 0) {
    memcpy (p, &stream->base[stream->fp], cnt);
    //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - fp=%d size=%d\n", __FUNCTION__, stream->fp, cnt);
  }
  stream->fp += cnt;
  return nitems;
}
char *mem_fgets(char *s, int n, mem_FILE *stream)
{
  int m = 0;
  if (mem_feof(stream)) {
    return NULL;
  }
  n--;
  while (m<n && !mem_feof(stream)) {
    char ch = stream->base[stream->fp];
    stream->fp++;
    if (ch == '\r') continue;
    s[m++] = ch;
    if (ch == '\n') break;
  }
  s[m] = 0; // terminate.
  return s;
}
int mem_feof(mem_FILE *stream)
{
  return stream->fp >= stream->size;
}

//---------------------------------------------------------------------
mem_FILE *mem_fopen_resource(const char *name, const char *mode)
{
  DWORD size;
  UINT8 *buf = GetFrame().GetResource(name, &size);
  mem_FILE *hFile = NULL;
  if (buf && size > 0) {
    register_mem_FILE (name, buf, size);
    hFile = mem_fopen( name, mode );
  }
  return hFile;
}
int mem_fclose_resource (mem_FILE *f)
{
  free(f->base);
  unregister_mem_FILE(f->name);
  mem_fclose(f);
  return 0;
}
