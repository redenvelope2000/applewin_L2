#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "StdAfx.h"
#include "Core.h"
#include "frontends/libretro/environment.h"
#include "linux/keyboard.h"
#include "Interface.h"
 
// -------------------- clipboard support code ------------------------

// Exit the clipboard chars input and release the data buffer.
void delete_clipboard_data ()
{
}

// Start the clipboard chars input, call it when the Paste key is pressed.
int new_clipboard_data ()
{ 
  return 0;
}

// Check the clipboard input data. Run it in the main loop.
int check_clipboard_data ()
{
  return 0;
}

// Put a text block to the clipboard. Use this to dump the text screen contents when the Copy key is pressed.
int put_text_to_clipboard (char *data)
{
  return 0;
}

// Put a graphics image to the clipboard. Use this when the Copy key is pressed.
int put_image_to_clipboard (uint32_t *data, int width, int height, int pitch)
{
  return 0;
}
