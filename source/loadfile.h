#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// return the file size.
int FileSize(FILE *fp);
// Load the file data. If the file "name" does not exist, it's loaded from the zip file "rsc_name".
// return 0 if succ.
// return the data pointer in *buffer, data size in *size.
// when the data is not needed anymore, free (*buffer) to avoid memory leak.
int LoadFile (const char *name, const char *rsc_name, UINT8 **buffer, int *size);
int LoadFile (const char *name, UINT8 **buffer, int *size);

// Memory-based file functions.
struct mem_FILE {
  long fp;
  uint8_t *base;
  long size;
  char *name;
};
#define MAX_mem_files 20
void init_mem_FILE ();
void quit_mem_FILE ();
int register_mem_FILE (const char *name, uint8_t *mem_block, long size);
int unregister_mem_FILE (const char *name);
mem_FILE *mem_fopen (const char *name, const char *mode);
int mem_fclose (mem_FILE *f);
long mem_ftell(mem_FILE *stream);
#define mem_SEEK_SET 0
#define mem_SEEK_END 1
int mem_fseek(mem_FILE *stream, long offset, int whence);
int mem_fread(void *p, int sz, int nitems, mem_FILE *stream);
char *mem_fgets(char *s, int n, mem_FILE *stream);
int mem_feof(mem_FILE *stream);
mem_FILE *mem_fopen_resource(const char *name, const char *mode);
int mem_fclose_resource (mem_FILE *f);
