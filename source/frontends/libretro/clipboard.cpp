#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "StdAfx.h"
#include "Core.h"
#include "frontends/libretro/environment.h"
#include "linux/keyboard.h"
#include "Interface.h"
 
#include "clip.h"
#include "clipboard.h"

// -------------------- clipboard support code ------------------------
static uint8_t *clipboard_data=0;
static int clipboard_data_next=0, clipboard_data_len=0;

// Exit the clipboard chars input and release the data buffer.
void delete_clipboard_data ()
{
  if (clipboard_data) {
    delete[] clipboard_data;
    clipboard_data = NULL;
    clipboard_data_len = 0;
  }
}

// Start the clipboard chars input, call it when the Paste key is pressed.
int new_clipboard_data ()
{ 
  if (clipboard_data || clipboard_data_len > 0) { // ?? previous life memory
    clipboard_data = NULL;
    clipboard_data_len = 0;
    return -2;
  }

  int len = 0;
  char *data = NULL;
  std::string value;
  bool result = clip::get_text(value);
  if (result) {
    char *str = (char*)value.c_str();
    len = strlen (str);
    if (len > 0) {
      data = new char[len + 1];
      strcpy (data, str);
    }
  }
  
  if (!data || len <= 0) {
    return -1;
  }
  clipboard_data_len = len;
  clipboard_data = (uint8_t *)data;
  clipboard_data_next = 0;
  ra2::log_cb (RETRO_LOG_INFO, "start clipboard: data len=%d data=%x--%s\n", clipboard_data_len, clipboard_data, clipboard_data);
  return 0;
}

// Check the clipboard input data. Run it in the main loop.
int check_clipboard_data ()
{
  if (clipboard_data) {
    for (int i=0; i<16; i++) { // Send chars as many as possible.
      if (clipboard_data_len > 0 && clipboard_data[clipboard_data_next]) {
        char char_buffer[2];
        char_buffer[0] = clipboard_data[clipboard_data_next];
        char_buffer[1] = 0;
        if (char_buffer[0] == '\r') {
          clipboard_data_next++;
          clipboard_data_len--;
          break;
        }          
        if (char_buffer[0] == '\n') {
          char_buffer[0] = 0xa;
          addTextToBuffer (char_buffer);
          clipboard_data_next++;
          clipboard_data_len--;
          break;
        } else {
          addTextToBuffer (char_buffer);
          clipboard_data_next++;
          clipboard_data_len--;
        }
      } else {
        //ra2::log_cb (RETRO_LOG_INFO, "clipboard out of data len=%d\n", clipboard_data_len);
        delete_clipboard_data ();
        break;
      }
    }
  }
  return 0;
}

// Put a text block to the clipboard. Use this to dump the text screen contents when the Copy key is pressed.
int put_text_to_clipboard (char *data)
{
  std::string new_content;
  new_content = (char *)data;
  clip::set_text(new_content);
  return 0;
}

// Put a graphics image to the clipboard. Use this when the Copy key is pressed.
int put_image_to_clipboard (uint32_t *data, int width, int height, int pitch)
{
  clip::image_spec spec;
  spec.width = width;
  spec.height = height;
  spec.bits_per_pixel = 32;
  spec.bytes_per_row = spec.width*4;
  spec.red_mask =   0x00ff0000;
  spec.green_mask = 0x0000ff00;
  spec.blue_mask =  0x000000ff;
  spec.alpha_mask = 0xff000000;
  spec.red_shift = 0;
  spec.green_shift = 8;
  spec.blue_shift = 16;
  spec.alpha_shift = 24;
  clip::image img(data, spec);
  clip::set_image(img);
  return 0;
}
