#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION

#define NK_RETRO_SOFT_IMPLEMENTATION

#include "nuklear/nuklear.h"
#include "retro/nuklear_retro_soft.h"
#include "../gui.h"

static RSDL_Surface *screen_surface;
static void *Retro_Screen;

#define GUI_WINDOW_RECT nk_rect((560-280-16)/2,16, 280, (384-16*2))

/* macros */

#define UNUSED(a) (void)a
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a)/sizeof(a)[0])

/* GUI */
struct nk_context *ctx;
static nk_retro_Font *RSDL_font;

#include "nuklear/style.c"

//---------------------------------------------------------------------

static int
gui(struct nk_context *ctx)
{
  /* window flags */
  static int border = nk_true;
  static int resize = nk_true;
  static int movable = nk_true;
  static int no_scrollbar = nk_false;
  static nk_flags window_flags = 0;
  static int minimizable = nk_false;
  static int title = nk_true;

  /* window flags */
  window_flags = 0;

  if (border) window_flags |= NK_WINDOW_BORDER;
  if (resize) window_flags |= NK_WINDOW_SCALABLE;
  if (movable) window_flags |= NK_WINDOW_MOVABLE;
  if (no_scrollbar) window_flags |= NK_WINDOW_NO_SCROLLBAR;
  if (minimizable) window_flags |= NK_WINDOW_MINIMIZABLE;
  if (title) window_flags |= NK_WINDOW_TITLE;

  if (nk_begin(ctx,"AppleWin GUI", GUI_WINDOW_RECT, window_flags)) {

    #define DEFHSZ 16
    #define DEFWSZ 64
    
    // button toggle GUI/EMU
    nk_layout_row_dynamic(ctx, DEFHSZ, 2);
    if (nk_button_label(ctx, "Resume")){
      gui_setup_data.want_exit=1;
    }
    if (nk_button_label(ctx, "Reset")){
      gui_setup_data.want_reset=1;
    }
    
    nk_label(ctx, "Disk Drive 1:", NK_TEXT_LEFT );
    nk_layout_row_static(ctx, 20, 240, 1);
    gui_setup_data.drive1 = nk_combo(ctx, (const char **)names_in_diskset, num_items_in_diskset, gui_setup_data.drive1, 12, nk_vec2(240,200));
    
    nk_label(ctx, "Disk Drive 2:", NK_TEXT_LEFT );
    nk_layout_row_static(ctx, 20, 240, 1);
    gui_setup_data.drive2 = nk_combo(ctx, (const char **)names_in_diskset, num_items_in_diskset, gui_setup_data.drive2, 12, nk_vec2(240,200));

    nk_label(ctx, "Video Display:", NK_TEXT_LEFT );
    static const char *video_modes[] = {
      "Color (Composite Idealized)", "Color (RGB Card/Monitor)", "Color (Composite Monitor)", "Color TV", "B&W TV", 
      "Monochrome (Amber)", "Monochrome (Green)", "Monochrome (White)"};
    nk_layout_row_static(ctx, 20, 240, 1);
    gui_setup_data.video_mode = nk_combo(ctx, video_modes, LEN(video_modes), gui_setup_data.video_mode, 16, nk_vec2(240,200));

    nk_label(ctx, "Disk Access Speed:", NK_TEXT_LEFT );
    static const char *disk_speed_modes[] = {"Authentic speed", "Enhanced speed"};
    nk_layout_row_static(ctx, 20, 240, 1);
    gui_setup_data.disk_speed_mode = nk_combo(ctx, disk_speed_modes, LEN(disk_speed_modes), gui_setup_data.disk_speed_mode, 12, nk_vec2(240,200));
    
    {
      enum {EASY, MEDIUM, HARD};
      static int op = EASY;
      static float value = 0.5f;
      
      ///* fixed widget window ratio width */
      //nk_layout_row_dynamic(ctx, 60, 4);
      //if (nk_option_label(ctx, "A Class", op == EASY)) op = EASY;
      //if (nk_option_label(ctx, "B Class", op == MEDIUM)) op = MEDIUM;
      //if (nk_option_label(ctx, "C Class", op == HARD)) op = HARD;
      
      /* custom widget pixel width */
      nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
      {
        nk_layout_row_push(ctx, 140);
        static char cpu_str[24];
        gui_setup_data.cpu_speed = (int) ((value + 0.5f)*10.0);
        if (gui_setup_data.cpu_speed < 100) sprintf (cpu_str, "CPU Speed: %3.1f Mhz", ((float)gui_setup_data.cpu_speed)/10.0);
        else sprintf (cpu_str, "CPU Speed:  %d Mhz", (int)gui_setup_data.cpu_speed/10);
        nk_label(ctx, cpu_str, NK_TEXT_LEFT);
        nk_layout_row_push(ctx, 100);
        nk_slider_float(ctx, 0, &value, 9.5f, 0.5f);
      }
      nk_layout_row_end(ctx);
    }
    
    nk_layout_row_dynamic(ctx, DEFHSZ, 2);
    if (nk_button_label(ctx, "Quick Save")){
      gui_setup_data.want_qs1=1;
    }
    if (nk_button_label(ctx, "Quick Restore")){
      gui_setup_data.want_qr1=1;
    }
      
    
    nk_end(ctx);
  }

  return 0;
}

//----------------------------------------------------------------------

void * app_init(int retrow, int retroh)
{
  screen_surface = Retro_CreateRGBSurface32(retrow,retroh,32,0,0,0,0);
  Retro_Screen = screen_surface->pixels;

  RSDL_font = (nk_retro_Font*)calloc(1, sizeof(nk_retro_Font));
  if (!RSDL_font)
    return NULL;
  RSDL_font->width = 8;
  RSDL_font->height = 8;

  /* GUI */
  ctx = nk_retro_init(RSDL_font,screen_surface,retrow,retroh);

  /* style.c */
  /* THEME_BLACK, THEME_WHITE, THEME_RED, THEME_BLUE, THEME_DARK */
  set_style(ctx, THEME_WHITE);

  return Retro_Screen;
}

int app_free()
{
  //FIXME: no more memory leak here ?
  if (RSDL_font)
   free(RSDL_font);
  RSDL_font = NULL;

  nk_retro_shutdown();

  Retro_FreeSurface(screen_surface);
  if (screen_surface)
    free(screen_surface);
  screen_surface = NULL;

  return 0;
}

int app_event(int poll)
{
  int evt;
  
  nk_input_begin(ctx);
  nk_retro_handle_event(&evt,poll);
  nk_input_end(ctx);
 
  return 0;
}

int app_run(retro_input_poll_t poll_cb, retro_input_state_t state_cb)
{
  int state;
  
  input_state_cb = state_cb;
  input_poll_cb = poll_cb;

  app_event (0);

  state = gui (ctx);
  
  nk_retro_render(nk_rgba(0,0,0,0));
  
  return 0;
}

