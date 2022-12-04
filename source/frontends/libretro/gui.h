#pragma once

int switch_gui_mode ();
int init_gui ();
void quit_gui ();
void run_gui (void *bg_frame);
extern int gui_framebuffer_width, gui_framebuffer_height, gui_framebuffer_pitch;
extern char *gui_framebuffer;

struct gui_setup_items_t {
  uint8_t want_quit, want_exit, want_reset;
  uint8_t want_qs1, want_qr1;
  uint8_t want_qs2, want_qr2;
  uint8_t want_qs3, want_qr3;
  int video_mode;
  int disk_speed_mode;
  int drive1, drive2;
  int cpu_speed;
};

extern gui_setup_items_t gui_setup_data;
extern char **names_in_diskset;
extern int num_items_in_diskset;

